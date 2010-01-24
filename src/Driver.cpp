//-----------------------------------------------------------------------------
//
//	Driver.h
//
//	Communicates with a Z-Wave network
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include <comutil.h>
#include <math.h>

#include "Defs.h"
#include "Driver.h"
#include "Node.h"
#include "Msg.h"

#include "Event.h"
#include "Mutex.h"
#include "SerialPort.h"
#include "Thread.h"
#include "Log.h"

#include "CommandClasses.h"
#include "WakeUp.h"

#include "ValueID.h"
#include "Value.h"
#include "ValueStore.h"


using namespace OpenZWave;

Driver* Driver::s_pInstance = NULL;


//-----------------------------------------------------------------------------
//	<Driver::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
Driver* Driver::Create
(
	string const& _serialPortName,
	string const& _configPath
)
{
	if( NULL == s_pInstance )
	{
		s_pInstance = new Driver( _serialPortName, _configPath );
	}

	return s_pInstance;
}

//-----------------------------------------------------------------------------
//	<Driver::Destroy>
//	Static method to destroy the singleton.
//-----------------------------------------------------------------------------
void Driver::Destroy
(
)
{
	delete s_pInstance;
	s_pInstance = NULL;
}

//-----------------------------------------------------------------------------
// <Driver::Driver>
// Constructor
//-----------------------------------------------------------------------------
Driver::Driver
( 
	string const& _serialPortName,
	string const& _configPath
):
	m_serialPortName( _serialPortName ),
	m_configPath( _configPath ),
	m_pollInterval( 30 ),					// By default, every polled device is queried once every 30 seconds
	m_bWaitingForAck( false ),
	m_expectedReply( 0 ),
	m_expectedCallbackId( 0 ),
	m_bExit( false ),
	m_pDriverThread( new Thread() ),
	m_pExitEvent( new Event() ),	
	m_pSerialPort( new SerialPort() ),
	m_pSerialMutex( new Mutex() ),
	m_pSendThread( new Thread() ),	
	m_pSendMutex( new Mutex() ),	
	m_pSendEvent( new Event() ),	
	m_pPollThread( new Thread() ),	
	m_pPollMutex( new Mutex() ),
	m_pInfoMutex( new Mutex() )
{
	// Create the log file
	Log::Create( "OZW_Log.txt" );

	CommandClasses::RegisterCommandClasses();

	// Ensure the singleton instance is set
	s_pInstance = this;

	// Clear the nodes array
	memset( m_nodes, 0, sizeof(Node*) * 256 );

	// Start the thread that will handle communications with the Z-Wave network
	m_pDriverThread->Start( Driver::DriverThreadEntryPoint, this );
}

//-----------------------------------------------------------------------------
// <Driver::Driver>
// Destructor
//-----------------------------------------------------------------------------
Driver::~Driver
(
)
{
	m_pExitEvent->Set();

	delete m_pPollThread;
	delete m_pSendThread;
	delete m_pDriverThread;

	delete m_pSerialPort;
	delete m_pSerialMutex;
	
	delete m_pSendMutex;
	delete m_pPollMutex;
	delete m_pInfoMutex;

	delete m_pSendEvent;
	delete m_pExitEvent;
}

//-----------------------------------------------------------------------------
// <Driver::DriverThreadEntryPoint>
// Entry point of the thread for creating and managing the worker threads
//-----------------------------------------------------------------------------
void Driver::DriverThreadEntryPoint
( 
	void* _pContext 
)
{
	Driver* pDriver = (Driver*)_pContext;
	if( pDriver )
	{
		pDriver->DriverThreadProc();
	}
}

//-----------------------------------------------------------------------------
// <Driver::DriverThreadProc>
// Create and manage the worker threads
//-----------------------------------------------------------------------------
void Driver::DriverThreadProc
( 
)
{
	uint32 attempts = 0;
	while(1)
	{
		if( Init( attempts ) )
		{
			// Driver has been initialised

			// Wait for messages from the Z-Wave network
			while( true )
			{
				// Get exclusive access to the serial port
				m_pSerialMutex->Lock();
					
				// Consume any available messages
				while( ReadMsg() );

				// Release our lock on the serial port 
				m_pSerialMutex->Release();

				// Wait for more data to appear at the serial port
				m_pSerialPort->Wait( Event::Timeout_Infinite );
				
				// Check for exit
				if( m_bExit )
				{
					return;
				}
			}
		}

		++attempts;
		if( attempts < 25 )		
		{
			// Retry every 5 seconds for the first two minutes			
			if( m_pExitEvent->Wait( 5000 ) )
			{
				// Exit signalled.
				break;
			}
		}
		else
		{
			// Retry every 30 seconds after that
			if( m_pExitEvent->Wait( 30000 ) )
			{
				// Exit signalled.
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::Init>
// Initialize the controller
//-----------------------------------------------------------------------------
bool Driver::Init
(
	uint32 _attempts
)
{
	m_nodeId = -1;
	m_bWaitingForAck = false;

	// Open the serial port
	Log::Write( "Opening serial port %s", m_serialPortName.c_str() );

	if( !m_pSerialPort->Open( m_serialPortName, 115200, SerialPort::Parity_None, SerialPort::StopBits_One ) )
	{
	 	Log::Write( "Failed to init the controller (attempt %d)", _attempts );
		return false;
	}

	// Serial port opened successfully, so we need to start all the worker threads
	m_pSendThread->Start( Driver::SendThreadEntryPoint, this );
	m_pPollThread->Start( Driver::PollThreadEntryPoint, this );

	// Send a NAK to the ZWave device
	uint8 nak = NAK;
	m_pSerialPort->Write( &nak, 1 );

	// Send commands to retrieve properties from the Z-Wave interface 
	Msg* pMsg;

	Log::Write( "Get version" );
	pMsg = new Msg( "Get version", 0xff, REQUEST, ZW_GET_VERSION, false );
	SendMsg( pMsg );

	Log::Write( "Get home/node id" );
	pMsg = new Msg( "Get home/node id", 0xff, REQUEST, ZW_MEMORY_GET_ID, false );
	SendMsg( pMsg );

	Log::Write( "Get capabilities" );
	pMsg = new Msg( "Get capabilities", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_CAPABILITIES, false );
	SendMsg( pMsg );
	
	//Log::Write( "Get SUC node id" );
	//pMsg = new Msg( "Get SUC node id", 0xff, REQUEST, FUNC_ID_ZW_GET_SUC_NODE_ID, false );
	//SendMsg( pMsg );
	
	Log::Write( "Get init data" );
	pMsg = new Msg( "Get init data", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false );
	SendMsg( pMsg );

	// Init successful
	return true;
}

//-----------------------------------------------------------------------------
//	Sending Z-Wave messages
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::SendMsg>
// Queue a message to be sent to the Z-Wave PC Interface
//-----------------------------------------------------------------------------
void Driver::SendMsg
( 
	Msg* _pMsg
)
{
	_pMsg->Finalize();

	Node* pNode = m_nodes[_pMsg->GetTargetNodeId()];
	if( ( pNode ) && ( !pNode->IsListeningDevice() ) )
	{
		if( WakeUp* pWakeUp = static_cast<WakeUp*>( pNode->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
		{
			if( !pWakeUp->IsAwake() )
			{
				Log::Write( "" );
				Log::Write( "Wake-Up Command: %s", _pMsg->GetAsString().c_str() );
				pWakeUp->QueueMsg( _pMsg );
				return;
			}
		}
	}

	Log::Write( "Queuing command: %s", _pMsg->GetAsString().c_str() );
	m_pSendMutex->Lock();
	m_sendQueue.push_back( _pMsg );
	m_pSendMutex->Release();
	UpdateEvents();
}

//-----------------------------------------------------------------------------
// <Driver::SendThreadEntryPoint>
// Entry point of the thread for sending Z-Wave messages
//-----------------------------------------------------------------------------
void Driver::SendThreadEntryPoint
( 
	void* _pContext 
)
{
	Driver* pDriver = (Driver*)_pContext;
	if( pDriver )
	{
		pDriver->SendThreadProc();
	}
}

//-----------------------------------------------------------------------------
// <Driver::SendThreadProc>
// Thread for sending Z-Wave messages
//-----------------------------------------------------------------------------
void Driver::SendThreadProc()
{
	int32 timeout = Event::Timeout_Infinite;
	while( true )
	{
		bool bEventSet = m_pSendEvent->Wait( timeout );

		// Check for exit
		if( m_bExit )
		{
			return;
		}

		if( bEventSet )
		{
			// We have a message to send.

			// Get the serial port mutex so we don't conflict with the read thread.
			m_pSerialMutex->Lock();

			// Get the send mutex so we can access the message queue.
			m_pSendMutex->Lock();

			if( !m_sendQueue.empty() )
			{
				Msg* pMsg = m_sendQueue.front();
				
				m_expectedCallbackId = pMsg->GetCallbackId();
				m_expectedReply = pMsg->GetExpectedReply();
				m_bWaitingForAck = true;
				m_pSendEvent->Reset();

				timeout = 5000;	// retry in 5 seconds

				pMsg->SetSendAttempts( pMsg->GetSendAttempts() + 1 );

				Log::Write( "" );
				Log::Write( "Sending command (Callback ID=%d, Expected Reply=%d) - %s", pMsg->GetCallbackId(), pMsg->GetExpectedReply(), pMsg->GetAsString().c_str() );

				m_pSerialPort->Write( pMsg->GetBuffer(), pMsg->GetLength() );
			}
			else
			{
				// Check for nodes requiring info requests
				uint8 nodeId = GetInfoRequest();
				if( nodeId != 0xff )
				{
					Msg* pMsg = new Msg( "Get Node Protocol Info", nodeId, REQUEST, FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO, false );
					pMsg->Append( nodeId );	
					SendMsg( pMsg ); 
				}
			}

			m_pSendMutex->Release();
			m_pSerialMutex->Release();
		}
		else
		{
			// Timed out.  Set the timeout back to infinity
			timeout = Event::Timeout_Infinite;

			// Get the serial port mutex so we don't conflict with the read thread.
			m_pSerialMutex->Lock();

			// Get the send mutex so we can access the message queue.
			m_pSendMutex->Lock();

			if( m_bWaitingForAck || m_expectedCallbackId || m_expectedReply )
			{
				// Timed out waiting for a response
				if( !m_sendQueue.empty() )
				{
					Msg* pMsg = m_sendQueue.front();

					if( pMsg->GetSendAttempts() > 2 )
					{
						// Give up
						Log::Write( "ERROR: Dropping command, expected response not received after three attempts");
						RemoveMsg();
					}
					else
					{
						// Resend
						if( pMsg->GetSendAttempts() > 0 )
						{
							Log::Write( "Timeout - resending" );
						}
						
						m_bWaitingForAck = 0;	
						m_expectedCallbackId = 0;
						m_expectedReply = 0;
						m_pSendEvent->Set();
					}
				}
			}

			m_pSendMutex->Release();
			m_pSerialMutex->Release();
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::RemoveMsg>
// Remove a message from the send queue
//-----------------------------------------------------------------------------
void Driver::RemoveMsg
( 
)
{
	m_pSendMutex->Lock();
	
	delete m_sendQueue.front();
	m_sendQueue.pop_front();
	if( m_sendQueue.empty() )
	{
		//No messages left, so clear the event
		m_pSendEvent->Reset();
	}
	
	m_pSendMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::TriggerResend>
// Cause a message to be resent immediately
//-----------------------------------------------------------------------------
void Driver::TriggerResend
( 
)
{
	m_pSendMutex->Lock();
	
	Msg* pMsg = m_sendQueue.front();
	pMsg->SetSendAttempts( 0 );

	m_bWaitingForAck = 0;	
	m_expectedCallbackId = 0;
	m_expectedReply = 0;

	m_pSendEvent->Set();

	m_pSendMutex->Release();
}

//-----------------------------------------------------------------------------
//	Receiving Z-Wave messages
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::ReadMsg>
// Read data from the serial port
//-----------------------------------------------------------------------------
bool Driver::ReadMsg
(
)
{
	uint8 pBuffer[1024];

	if( !m_pSerialPort->Read( pBuffer, 1 ) )
	{
		// Nothing to read
		return false;
	}

	switch( pBuffer[0] )
	{
		case SOF:
		{
			// Read the length byte
			m_pSerialPort->Read( &pBuffer[1], 1 );

			// Read the rest of the frame
			uint32 bytesRemaining = pBuffer[1];
			while( bytesRemaining )
			{
				bytesRemaining -= m_pSerialPort->Read( &pBuffer[2+(uint32)pBuffer[1]-bytesRemaining], bytesRemaining );
			}

			uint32 length = pBuffer[1] + 2;

			// Log the data
			string str = "";
			for( uint32 i=0; i<length; ++i ) 
			{
				if( i )
				{
					str += ", ";
				}

				char byteStr[8];
				snprintf( byteStr, sizeof(byteStr), "0x%.2x", pBuffer[i] );
				str += byteStr;
			}
			Log::Write( "" );
			Log::Write( "Received: %s", str.c_str() );

			// Verify checksum
			uint8 checksum = 0xff;
			for( uint32 i=1; i<(length-1); ++i ) 
			{
				checksum ^= pBuffer[i];
			}

			if( pBuffer[length-1] == checksum )
			{
				// Checksum correct - send ACK
				uint8 ack = ACK;
				m_pSerialPort->Write( &ack, 1 );

				// Process the received message
				ProcessMsg( &pBuffer[2] );
			}
			else
			{
				Log::Write( "Checksum incorrect - sending NAK" );
				uint8 nak = NAK;
				m_pSerialPort->Write( &nak, 1 );
			}
			break;
		}
			
		case CAN:
		{
			Log::Write( "CAN received...triggering resend" );
			TriggerResend();
			break;
		}
		
		case NAK:
		{
			Log::Write( "NAK received...triggering resend" );
			TriggerResend();
			break;
		}

		case ACK:
		{
			Log::Write( "ACK received" );
			m_bWaitingForAck = false;
			
			if( ( 0 == m_expectedCallbackId ) && ( 0 == m_expectedReply ) )
			{
				// Remove the message from the queue, now that it has been acknowledged.
				RemoveMsg();
				UpdateEvents();
			}
			break;
		}
		
		default:
		{
			Log::Write( "ERROR! Out of frame flow! (0x%.2x)", pBuffer[0] );
			break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Driver::ProcessMsg>
// Process data received from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::ProcessMsg
(
	uint8* pData
)
{
	bool bHandleCallback = true;

	if( RESPONSE == pData[0] )
	{
		switch( pData[1] )
		{
			case ZW_GET_VERSION:
			{
				Log::Write( "Received reply to ZW_GET_VERSION: %s", ((int8*)&pData[2]) );
				break;
			}
			case FUNC_ID_SERIAL_API_GET_CAPABILITIES:
			{
				HandleGetCapabilitiesResponse( pData );
				break;
			}
			case FUNC_ID_ZW_ENABLE_SUC:
			{
				HandleEnableSUCResponse( pData );
				break;
			}
			case FUNC_ID_ZW_SET_SUC_NODE_ID:
			{
				HandleSetSUCNodeIdResponse( pData );
				break;
			}
			case FUNC_ID_ZW_GET_SUC_NODE_ID:
			{
				HandleGetSUCNodeIdResponse( pData );
				break;
			}
			case ZW_MEMORY_GET_ID:
			{
				HandleMemoryGetIdResponse( pData );
				break;
			}
			case FUNC_ID_SERIAL_API_GET_INIT_DATA:
			{
				HandleSerialAPIGetInitDataResponse( pData );
				break;
			}
			case FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO:
			{
				HandleGetNodeProtocolInfoResponse( pData );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_INFO:
			{
				Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NODE_INFO" );
				break;
			}
			case FUNC_ID_ZW_SEND_DATA:
			{
				HandleSendDataResponse( pData );
				bHandleCallback = false;			// Skip the callback handling - a subsequent FUNC_ID_ZW_SEND_DATA request will deal with that
				break;
			}
			default:
			{
				Log::Write( "TODO: handle response for %d", pData[1] );
				break;
			}
		}
	} 
	else if( REQUEST == pData[0] )
	{
		switch( pData[1] )
		{
			case FUNC_ID_ZW_SEND_DATA:
			{
				HandleSendDataRequest( pData );
				break;
			}
			case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
			{
				HandleAddNodeToNetworkRequest( pData );
				break;
			}
			case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
			{
				HandleRemoveNodeFromNetworkRequest( pData );
				break;
			}
			case FUNC_ID_APPLICATION_COMMAND_HANDLER:
			{
				HandleApplicationCommandHandlerRequest( pData );
				break;
			}
			case FUNC_ID_ZW_APPLICATION_UPDATE:
			{
				HandleApplicationUpdateRequest( pData );
				break;
			}
			default:
			{
				break;
			}	
		}
	}

	// Generic callback handling
	if( bHandleCallback )
	{
		if( m_expectedCallbackId )
		{
			if( m_expectedCallbackId == pData[2] )
			{
				Log::Write( "Message transaction (callback=%d) complete", pData[2] );
				RemoveMsg();
				m_expectedCallbackId = 0;
			}
		}
		if( m_expectedReply )
		{
			if( m_expectedReply == pData[1] )
			{
				Log::Write( "Message transaction complete" );
				RemoveMsg();
				m_expectedReply = 0;
			}
		}

		UpdateEvents();
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetCapabilitiesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetCapabilitiesResponse
(
	uint8* pData
)
{
	Log::Write( "Received reply to GetCapabilities.  Node ID = %d", pData[2] );
	//2009-06-14 11:41:14:585 Received: 0x01, 0x2b, 0x01, 0x07, 0x02, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xfe, 0x80, 0xfe, 0x80, 0x03, 0x00, 0x00, 0x00, 0xfb, 0x9f, 0x3b, 0x80, 0x07, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59
}

//-----------------------------------------------------------------------------
// <Driver::HandleEnableSUCResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleEnableSUCResponse
(
	uint8* pData
)
{
	Log::Write( "Received reply to Enable SUC." );
}

//-----------------------------------------------------------------------------
// <Driver::HandleSetSUCNodeIdResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSetSUCNodeIdResponse
(
	uint8* pData
)
{
	Log::Write( "Received reply to SET_SUC_NODE_ID." );
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetSUCNodeIdResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetSUCNodeIdResponse
(
	uint8* pData
)
{
	Log::Write( "Received reply to GET_SUC_NODE_ID.  Node ID = %d", pData[2] );

	if( pData[2] == 0)
	{
		Log::Write( "No SUC, so we become SUC" );
		
		Msg* pMsg;

		pMsg = new Msg( "Enable SUC", m_nodeId, REQUEST, FUNC_ID_ZW_ENABLE_SUC, false );
		pMsg->Append( 1 );	
//		pMsg->Append( ZW_SUC_FUNC_BASIC_SUC );			// SUC
		pMsg->Append( ZW_SUC_FUNC_NODEID_SERVER );		// SIS
		SendMsg( pMsg ); 

		pMsg = new Msg( "Set SUC node ID", m_nodeId, REQUEST, FUNC_ID_ZW_SET_SUC_NODE_ID, false );
		pMsg->Append( m_nodeId );
		pMsg->Append( 1 );								// TRUE, we want to be SUC/SIS
		pMsg->Append( 0 );								// no low power
		pMsg->Append( ZW_SUC_FUNC_NODEID_SERVER );
		SendMsg( pMsg ); 
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleMemoryGetIdResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleMemoryGetIdResponse
(
	uint8* pData
)
{
	Log::Write( "Received reply to ZW_MEMORY_GET_ID. Home ID = 0x%02x%02x%02x%02x.  Our node ID = %d", pData[2], pData[3], pData[4], pData[5], pData[6] );
	m_nodeId = pData[6];
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialAPIGetInitDataResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSerialAPIGetInitDataResponse
(
	uint8* pData
)
{
	int32 i;
	Log::Write( "Received reply to FUNC_ID_SERIAL_API_GET_INIT_DATA:" );
	
	if( pData[4] == NUM_NODE_BITFIELD_BYTES )
	{
		for( i=0; i<NUM_NODE_BITFIELD_BYTES; ++i)
		{
			for( int32 j=0; j<8; ++j )
			{
				uint8 nodeId = (i*8)+j+1;
				if( pData[i+5] & (0x01 << j) )
				{
					Log::Write( "Found node %d", nodeId );

					// Create the node
					delete m_nodes[nodeId];
					m_nodes[nodeId] = new Node( nodeId );
				}
				else
				{
					if( m_nodes[nodeId] )
					{
						delete m_nodes[nodeId];
						m_nodes[nodeId] = NULL;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetNodeProtocolInfoResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetNodeProtocolInfoResponse
(
	uint8* _pData
)
{
	uint8 nodeId = GetInfoRequest();

	if( nodeId == 0xff )
	{
		Log::Write("ERROR: Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO but there are no node Ids in the info queue");
		return;
	}

	Log::Write("Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO for node %d", nodeId );

	// Update the node with the protocol info
	if( m_nodes[nodeId] )
	{
		m_nodes[nodeId]->UpdateProtocolInfo( &_pData[2] );
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendDataResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendDataResponse
(
	uint8* pData
)
{
	if( 0 == pData[2] )
	{
		Log::Write("ERROR: ZW_SEND could not be delivered to Z-Wave stack");
	}
	else if( 1 == pData[2] )
	{
		Log::Write( "ZW_SEND delivered to Z-Wave stack" );
	}
	else
	{	
		Log::Write("ERROR: ZW_SEND Response is invalid!");
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendDataRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendDataRequest
(
	uint8* pData
)
{
	Log::Write( "ZW_SEND Request with callback ID %d received (expected %d)", pData[2], m_expectedCallbackId );

	if( ( pData[2] != m_expectedCallbackId ) || ( pData[1] != m_expectedReply ) )
	{
		// Wrong callback ID
		Log::Write( "ERROR: Callback ID is invalid" );
	}
	else 
	{
		// Callback ID matches our expectation
		switch( pData[3] )
		{
			case 0:
			{
				// command reception acknowledged by node
				Log::Write("ZW_SEND was successful, removing command");
				RemoveMsg();
				m_expectedCallbackId = 0;
				m_expectedReply = 0;
				break;
			}
			case 1:
			{
				// ZW_SEND failed
				Log::Write( "Error: ZW_SEND failed, removing command" );
				m_pSendMutex->Lock();
				if( !m_sendQueue.empty() )
				{
					if( m_sendQueue.front()->GetSendAttempts() >= 3 )
					{
						// Can't deliver message, so abort
						Log::Write( "Error: ZW_SEND failed, removing message after three tries" );
						RemoveMsg();
					}
				} 
				else 
				{
					Log::Write( "Error: ZW_SEND failed, retrying" );
					
					// Trigger resend
					m_bWaitingForAck = false;
				}
				m_pSendMutex->Release();
				
				m_expectedCallbackId = 0;
				m_expectedReply = 0;
				break;
			}
			default:
			{
				Log::Write( "ERROR: ZW_SEND Response is invalid" );
			}
		}

		UpdateEvents();
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleAddNodeToNetworkRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleAddNodeToNetworkRequest
(
	uint8* pData
)
{
	Log::Write( "FUNC_ID_ZW_ADD_NODE_TO_NETWORK:" );
	
	switch( pData[3] )
	{
		case ADD_NODE_STATUS_LEARN_READY:
		{
			Log::Write( "ADD_NODE_STATUS_LEARN_READY" );
			break;
		}
		case ADD_NODE_STATUS_NODE_FOUND:
		{
			Log::Write( "ADD_NODE_STATUS_NODE_FOUND" );
			break;
		}
		case ADD_NODE_STATUS_ADDING_SLAVE:
		{
			Log::Write( "ADD_NODE_STATUS_ADDING_SLAVE" );			
			Log::Write( "Adding node ID %d", pData[4] );
			
			if( ( pData[7] == 8) && ( pData[8] == 3) )
			{
				Log::Write( "Setback schedule thermostat detected, triggering configuration" );
				//SetWakeupInterval( pData[4], 15, true );
			}
			
			// Finish adding node	
			//AddNode( 0, false );
			break;
		}
		case ADD_NODE_STATUS_ADDING_CONTROLLER:
		{
			Log::Write( "ADD_NODE_STATUS_ADDING_CONTROLLER");
			
			Log::Write( "Adding node ID %d", pData[4] );
			break;
		}
		case ADD_NODE_STATUS_PROTOCOL_DONE:
		{
			Log::Write( "ADD_NODE_STATUS_PROTOCOL_DONE" );
			
			// we send no replication info for now
			//AddNode( 0, false );
			break;
		}
		case ADD_NODE_STATUS_DONE:
		{
			Log::Write( "ADD_NODE_STATUS_DONE" );
			break;
		}
		case ADD_NODE_STATUS_FAILED:
		{
			Log::Write( "ADD_NODE_STATUS_FAILED" );
			break;
		}
		default:
		{
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleRemoveNodeFromNetworkRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleRemoveNodeFromNetworkRequest
(
	uint8* pData
)
{
	Log::Write( "FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:" );
	
	switch( pData[3] ) 
	{
		case REMOVE_NODE_STATUS_LEARN_READY:
		{
			Log::Write( "REMOVE_NODE_STATUS_LEARN_READY" );
			break;
		}
		case REMOVE_NODE_STATUS_NODE_FOUND:
		{
			Log::Write( "REMOVE_NODE_STATUS_NODE_FOUND" );
			break;
		}
		case REMOVE_NODE_STATUS_ADDING_SLAVE:
		{
			Log::Write( "REMOVE_NODE_STATUS_ADDING_SLAVE" );
			//RemoveNode( 0 );
			break;
		}
		case REMOVE_NODE_STATUS_ADDING_CONTROLLER:
		{
			Log::Write( "REMOVE_NODE_STATUS_ADDING_CONTROLLER" );
			break;
		}
		case REMOVE_NODE_STATUS_PROTOCOL_DONE:
		{
			Log::Write( "REMOVE_NODE_STATUS_PROTOCOL_DONE" );
			//RemoveNode( 0 );
			break;
		}
		case REMOVE_NODE_STATUS_DONE:
		{
			Log::Write( "REMOVE_NODE_STATUS_DONE" );
			break;
		}
		case REMOVE_NODE_STATUS_FAILED:
		{
			Log::Write( "REMOVE_NODE_STATUS_FAILED" );
			break;
		}
		default:
		{
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleApplicationCommandHandlerRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleApplicationCommandHandlerRequest
(
	uint8* _pData
)
{
	uint8 nodeId = _pData[3];
	if( m_nodes[nodeId] )
	{
		m_nodes[nodeId]->ApplicationCommandHandler( _pData );
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleApplicationUpdateRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleApplicationUpdateRequest
(
	uint8* _pData
)
{
	uint8 nodeId = _pData[3];

	switch( _pData[2] )
	{
		case UPDATE_STATE_NODE_INFO_RECEIVED:
		{
			Log::Write( "UPDATE_STATE_NODE_INFO_RECEIVED from node %d", nodeId );
			if( Node* pNode = m_nodes[nodeId] )
			{
				pNode->UpdateNodeInfo( &_pData[8], _pData[4] - 3 );

				if( !pNode->IsListeningDevice() )
				{
					if( WakeUp* pWakeUp = static_cast<WakeUp*>( pNode->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
					{
						pWakeUp->SendPending();
					}
				}
			}
			break;
		}

		case UPDATE_STATE_NODE_INFO_REQ_FAILED:
		{
			Log::Write( "FUNC_ID_ZW_APPLICATION_UPDATE: UPDATE_STATE_NODE_INFO_REQ_FAILED received" );
			break;
		}
		case UPDATE_STATE_NEW_ID_ASSIGNED:
		{
			Log::Write( "** Network change **: ID %d was assigned to a new Z-Wave node", nodeId );
			delete m_nodes[nodeId];
			m_nodes[nodeId] = new Node( nodeId );
			break;
		}
		case UPDATE_STATE_DELETE_DONE:
		{
			Log::Write( "** Network change **: Z-Wave node %d was removed", nodeId );
			delete m_nodes[nodeId];
			m_nodes[nodeId] = NULL;
			break;
		}
	}	
}

//-----------------------------------------------------------------------------
//	Polling Z-Wave devices
//-----------------------------------------------------------------------------
	
//-----------------------------------------------------------------------------
// <Driver::EnablePoll>
// Enable poll of a node
//-----------------------------------------------------------------------------
void Driver::EnablePoll
( 
	uint8 _id
)
{
	Node* pNode = m_nodes[_id];
	if( !pNode )
	{
		// Node does not exist
		return;
	}

	// Set the node state to polled
	pNode->SetPolled( true );

	if( !pNode->IsListeningDevice() )
	{
		// The device is not awake all the time, so we will request state when 
		// it wakes up, rather than at polling intervals.  This is handled in
		// WakeUp command class - nothing more for us to do here.
		return;
	}

	// Add the node to the poll list
	m_pPollMutex->Lock();
	
	// See if the node is already in the poll list.
	for( list<uint8>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
	{
		if( *it == _id )
		{
			// Node is already in the poll list, so we have nothing to do.
			m_pPollMutex->Release();
			return;	
		}
	}

	// Not in the list, so we add it
	m_pollList.push_back( _id );
	m_pPollMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::DisablePoll>
// Disable poll of a node
//-----------------------------------------------------------------------------
void Driver::DisablePoll
( 
	uint8 _id
)
{
	Node* pNode = m_nodes[_id];
	if( !pNode )
	{
		// Node does not exist
		return;
	}

	// Set the node state to not-polled
	pNode->SetPolled( false );

	m_pPollMutex->Lock();

	// Find the node in the poll list.
	for( list<uint8>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
	{
		if( *it == _id )
		{
			// Remove the node from the poll list.
			m_pollList.erase( it );		
			break;
		}
	}
	
	m_pPollMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::PollThreadEntryPoint>
// Entry point of the thread for poll Z-Wave devices
//-----------------------------------------------------------------------------
void Driver::PollThreadEntryPoint
( 
	void* _pContext 
)
{
	Driver* pDriver = (Driver*)_pContext;
	if( pDriver )
	{
		pDriver->PollThreadProc();
	}
}

//-----------------------------------------------------------------------------
// <Driver::PollThreadProc>
// Thread for poll Z-Wave devices
//-----------------------------------------------------------------------------
void Driver::PollThreadProc()
{
	while( 1 )
	{
		int32 pollInterval = m_pollInterval * 1000;	// Get the time in milliseconds in which we are to poll all the devices

		if( !m_pollList.empty() )
		{
			// We only bother getting the lock if the pollList is not empty
			m_pPollMutex->Lock();
			
			if( !m_pollList.empty() )
			{
				// Get the next node to be polled
				uint8 id = m_pollList.front();
			
				// Move it to the back of the list
				m_pollList.pop_front();
				m_pollList.push_back( id );

				// Calculate the time before the next poll, so that all polls 
				// can take place within the user-specified interval.
				pollInterval /= m_pollList.size();

				// Request a report from the node
				//RequestBasicReport( id );
			}

			m_pPollMutex->Release();
		}

		// Wait for the interval to expire, while watching for exit events
		if( m_pExitEvent->Wait( pollInterval ) )
		{
			// Exit has been called
			return;
		}
	}
}

//-----------------------------------------------------------------------------
//	Retrieving Node information
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::AddInfoRequest>
// Queue a node to be interrogated for its setup details
//-----------------------------------------------------------------------------
void Driver::AddInfoRequest
( 
	uint8 const _nodeId
)
{
	m_pInfoMutex->Lock();
	m_infoQueue.push_back( _nodeId );
	UpdateEvents();
	m_pInfoMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::RemoveInfoRequest>
// Remove a node from the info queue
//-----------------------------------------------------------------------------
void Driver::RemoveInfoRequest
( 
)
{
	m_pInfoMutex->Lock();
	if( !m_infoQueue.empty() )
	{
		m_infoQueue.pop_front();
		if( m_infoQueue.empty() )
		{
			UpdateEvents();
		}
	}
	m_pInfoMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::GetInfoRequest>
// Get the next node that needs its setup info requesting
//-----------------------------------------------------------------------------
uint8 Driver::GetInfoRequest
( 
)
{
	uint8 nodeId = 0xff;

	m_pInfoMutex->Lock();
	if( !m_infoQueue.empty() )
	{
		nodeId = m_infoQueue.front();
	}
	m_pInfoMutex->Release();
	return nodeId;
}

//-----------------------------------------------------------------------------
// <Driver::GetValue>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
Value* Driver::GetValue
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* pNode = m_nodes[nodeId] )
	{
		return( pNode->GetValue( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//	Notifications
//-----------------------------------------------------------------------------


////-----------------------------------------------------------------------------
//// <Driver::SetBasic>
//// 
////-----------------------------------------------------------------------------
//void Driver::SetBasic
//(
//	uint8 _nodeId, 
//	uint8 _level
//)
//{
//	Log::Write( "SetBasic: Node=%d, Level=%d", _nodeId, _level );
//
//	NodeMapIteratorType it = m_nodeMap.find( _nodeId );
//	if( it != m_nodeMap.end() )
//	{
//		Node* pNode = it->second;							
//
//		// Check if it is a setback schedule thermostat
//		if( ( Node::GenericType_Thermostat == pNode->GetGenericType() ) && ( 3 == pNode->GetSpecificType() ) )
//		{
//			// Only set the stateBasic, later gets sent as multi command on wakeup
//			Log::Write( "Setback schedule override prepared" );
//			pNode->SetBasicState( (_level == 0) ? 0 : 0xff );
//		}
//		else
//		{
//			Msg* pMsg = new Msg( "Basic Set", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
//			pMsg->Append( _nodeId );
//			pMsg->Append( 3 );
//			pMsg->Append( COMMAND_CLASS_BASIC );
//			pMsg->Append( BASIC_SET );
//			pMsg->Append( _level );
//			pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//			SendMsg( pMsg );
//		}
//	}
//}
//
////-----------------------------------------------------------------------------
//// <Driver::SendBasicReport>
//// 
////-----------------------------------------------------------------------------
//void Driver::SendBasicReport
//(
//	uint8 _nodeId
//)
//{
//	Log::Write( "SendBasicReport: Node=%d", _nodeId );
//
//	Msg* pMsg = new Msg( "Basic Report", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
//	pMsg->Append( _nodeId );
//	pMsg->Append( 3 );
//	pMsg->Append( COMMAND_CLASS_BASIC );
//	pMsg->Append( BASIC_REPORT );
//	pMsg->Append( 0 );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::GetAssociation>
//// 
////-----------------------------------------------------------------------------
//void Driver::GetAssociation
//(
//	uint8 _nodeId,
//	uint8 _group
//)
//{
//	Log::Write( "GetAssociation: Node=%d, Group=%d", _nodeId, _group );
//
//	Msg* pMsg = new Msg( "Association Get", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
//	pMsg->Append( _nodeId );
//	pMsg->Append( 3 );
//	pMsg->Append( COMMAND_CLASS_ASSOCIATION );
//	pMsg->Append( ASSOCIATION_GET );
//	pMsg->Append( _group );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//
//	if( IsSleepingNode( _nodeId ) )
//	{
//		Log::Write( "Postpone GetAssociation until device wakes up" );
//		SendSleepingMsg( _nodeId, pMsg );
//	}
//	else
//	{
//		SendMsg( pMsg );
//	}
//}
//
////-----------------------------------------------------------------------------
//// <Driver::SetAssociation>
//// 
////-----------------------------------------------------------------------------
//void Driver::SetAssociation
//(
//	uint8 _nodeId,
//	uint8 _group,
//	uint8 _targetNodeId
//)
//{
//	Log::Write( "SetAssociation: Node=%d, Group=%d, Target=%d", _nodeId, _group, _targetNodeId );
//
//	Msg* pMsg = new Msg( "Association Set", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
//	pMsg->Append( _nodeId );
//	pMsg->Append( 4 );
//	pMsg->Append( COMMAND_CLASS_ASSOCIATION );
//	pMsg->Append( ASSOCIATION_SET );
//	pMsg->Append( _group );
//	pMsg->Append( _targetNodeId );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//
//	if( IsSleepingNode( _nodeId ) )
//	{
//		Log::Write( "Postpone SetAssociation until device wakes up" );
//		SendSleepingMsg( _nodeId, pMsg );
//	}
//	else
//	{
//		SendMsg( pMsg );
//	}
//}
//
////-----------------------------------------------------------------------------
//// <Driver::RemoveAssociation>
//// 
////-----------------------------------------------------------------------------
//void Driver::RemoveAssociation
//(
//	uint8 _nodeId,
//	uint8 _group,
//	uint8 _targetNodeId
//)
//{
//	Log::Write( "RemoveAssociation: Node=%d, Group=%d, Target=%d", _nodeId, _group, _targetNodeId );
//
//	Msg* pMsg = new Msg( "Association Remove", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
//	pMsg->Append( _nodeId );
//	pMsg->Append( 4 );
//	pMsg->Append( COMMAND_CLASS_ASSOCIATION );
//	pMsg->Append( ASSOCIATION_REMOVE );
//	pMsg->Append( _group );
//	pMsg->Append( _targetNodeId );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//
//	if( IsSleepingNode( _nodeId ) )
//	{
//		Log::Write( "Postpone RemoveAssociation until device wakes up" );
//		SendSleepingMsg( _nodeId, pMsg );
//	}
//	else
//	{
//		SendMsg( pMsg );
//	}
//}
//
////-----------------------------------------------------------------------------
//// <Driver::AssignReturnRoute>
//// 
////-----------------------------------------------------------------------------
//void Driver::AssignReturnRoute
//(
//	uint8 _nodeId,
//	uint8 _targetNodeId
//)
//{
//	Msg* pMsg = new Msg( "Assign Return Route", _nodeId, REQUEST, FUNC_ID_ZW_ASSIGN_RETURN_ROUTE, true );		
//	pMsg->Append( _nodeId );
//	pMsg->Append( _targetNodeId );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	SendMsg( pMsg );
//} 
//
////-----------------------------------------------------------------------------
//// <Driver::ReplicateController>
//// 
////-----------------------------------------------------------------------------
//void Driver::ReplicateController
//(
//	bool _bBegin
//)
//{
//	Msg* pMsg;
//
//	if( _bBegin )
//	{
//		Log::Write( "Replicate controller begin" );
//		pMsg = new Msg( "Replicate controller begin", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
//		pMsg->Append( 1 );
//		SendMsg( pMsg );
//	}
//	else
//	{
//		Log::Write( "Replicate controller end" );
//		pMsg = new Msg(  "Replicate controller end", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
//		pMsg->Append( 0 );
//		SendMsg( pMsg );
//
//		Log::Write( "Get new init data after replication" );
//		pMsg = new Msg( "Get new init data after replication", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false );
//		SendMsg( pMsg );
//	}
//}
//
////-----------------------------------------------------------------------------
//// <Driver::ResetController>
//// Reset controller and erase all node information
////-----------------------------------------------------------------------------
//void Driver::ResetController
//(
//)
//{
//	Log::Write( "Reset controller and erase all node information");
//	Msg* pMsg = new Msg( "Reset controller and erase all node information", 0xff, REQUEST, FUNC_ID_ZW_SET_DEFAULT, true );
//	SendMsg( pMsg );
//}

////-----------------------------------------------------------------------------
//// <Driver::SoftReset>
//// Soft-reset the Z-Wave controller chip
////-----------------------------------------------------------------------------
//void Driver::SoftReset
//(
//)
//{
//	Log::Write( "Soft-resetting the Z-Wave controller chip");
//	Msg* pMsg = new Msg( "Soft-resetting the Z-Wave controller chip", 0xff, REQUEST, FUNC_ID_SERIAL_API_SOFT_RESET, false, false );
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::AddNode>
//// Set the controller in and out of AddNode mode
////-----------------------------------------------------------------------------
//void Driver::AddNode
//(
//	bool _bBegin,
//	bool _bHighpower // = false
//)
//{
//	Msg* pMsg;
//
//	if( _bBegin )
//	{	
//		Log::Write( "Add Node - begin" );
//		pMsg = new Msg( "Add node - begin", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
//		pMsg->Append( _bHighpower ? ADD_NODE_ANY | ADD_NODE_OPTION_HIGH_POWER : ADD_NODE_ANY );
//	}
//	else
//	{
//		Log::Write( "Add Node - end" );
//		pMsg = new Msg( "Add node - end", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, false, false );
//		pMsg->Append( ADD_NODE_STOP );
//	}
//
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::RemoveNode>
//// Set the controller in and out of RemoveNode mode
////-----------------------------------------------------------------------------
//void Driver::RemoveNode
//(
//	bool _bBegin
//)
//{
//	Msg* pMsg;
//
//	if( _bBegin )
//	{	
//		Log::Write( "Remove Node - begin" );
//		pMsg = new Msg( "Remove node - begin", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
//		pMsg->Append( REMOVE_NODE_ANY );
//	}
//	else
//	{
//		Log::Write( "Remove Node - end" );
//		pMsg = new Msg( "Add node - end", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, false, false );
//		pMsg->Append( REMOVE_NODE_STOP );
//	}
//
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::SetConfiguration>
//// Set a configuration parameter of a device
////-----------------------------------------------------------------------------
//void Driver::SetConfiguration
//(
//	uint8 _nodeId,
//	uint8 _parameter,
//	uint32 _value
//)
//{
//	Log::Write( "SetConfiguration: Node=%d, Parameter=%d, Value=%d", _nodeId, _parameter, _value );
//
//	Msg* pMsg = new Msg( "Configuration Set", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
//
//	if( _value <= 0xff) 
//	{
//		// one byte value
//		pMsg->Append( _nodeId );
//		pMsg->Append( 5 );
//		pMsg->Append( COMMAND_CLASS_CONFIGURATION );
//		pMsg->Append( CONFIGURATION_SET );
//		pMsg->Append( _parameter );
//		pMsg->Append( 1 );
//		pMsg->Append( (uint8)_value );
//		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	}
//	else if( _value <= 0xffff )
//	{
//		// two byte value
//		pMsg->Append( _nodeId );
//		pMsg->Append( 6 );
//		pMsg->Append( COMMAND_CLASS_CONFIGURATION );
//		pMsg->Append( CONFIGURATION_SET );
//		pMsg->Append( _parameter );
//		pMsg->Append( 2 );
//		pMsg->Append( (uint8)((_value>>8)&0xff) );
//		pMsg->Append( (uint8)(_value&0xff) );
//		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	}
//	else
//	{
//		// four byte value
//		pMsg->Append( _nodeId );
//		pMsg->Append( 8 );
//		pMsg->Append( COMMAND_CLASS_CONFIGURATION );
//		pMsg->Append( CONFIGURATION_SET );
//		pMsg->Append( _parameter );
//		pMsg->Append( 4 );
//		pMsg->Append( (uint8)((_value>>24)&0xff) );
//		pMsg->Append( (uint8)((_value>>16)&0xff) );
//		pMsg->Append( (uint8)((_value>>8)&0xff) );
//		pMsg->Append( (_value & 0xff) );
//		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	}
//
//	if( IsSleepingNode( _nodeId ) )
//	{
//		Log::Write( "Postpone SetConfiguration until device wakes up" );
//		SendSleepingMsg( _nodeId, pMsg );
//	}
//	else
//	{
//		SendMsg( pMsg );
//	}
//}
//
////-----------------------------------------------------------------------------
//// <Driver::SetWakeupInterval>
//// Set the wakeup interval of a sleeping device
////-----------------------------------------------------------------------------
//void Driver::SetWakeupInterval
//(
//	uint8 _nodeId,
//	uint8 _interval,	// wakeup interval in minutes
//	bool _bMulti		// = false
//)
//{
//	uint32 interval = _interval * 60;	// convert minutes to seconds
//
//	Log::Write( "SetWakeup: Node=%d, Interval=%d minutes, Multi=%s", _nodeId, interval, _bMulti ? "true" : "false" );
//
//	Msg* pMsg = new Msg( "Wakeup Interval Set", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
//
//	if( _bMulti )
//	{
//		pMsg->Append( _nodeId );
//		pMsg->Append( 10 );
//		pMsg->Append( COMMAND_CLASS_MULTI_CMD );
//		pMsg->Append( MULTI_CMD_ENCAP );
//		pMsg->Append( 1 );					// 1 command
//		pMsg->Append( 6 );
//		pMsg->Append( COMMAND_CLASS_WAKE_UP );
//		pMsg->Append( WAKE_UP_INTERVAL_SET );
//		pMsg->Append( (uint8)((interval>>16)&0xff) ); 
//		pMsg->Append( (uint8)((interval>>8)&0xff) );	 
//		pMsg->Append( (uint8)(interval&0xff) );		
//		pMsg->Append( m_nodeId );
//		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	}
//	else
//	{
//		pMsg->Append( _nodeId );
//		pMsg->Append( 6 );
//		pMsg->Append( COMMAND_CLASS_WAKE_UP );
//		pMsg->Append( WAKE_UP_INTERVAL_SET );
//		pMsg->Append( (uint8)((interval>>16)&0xff) ); 
//		pMsg->Append( (uint8)((interval>>8)&0xff) );	 
//		pMsg->Append( (uint8)(interval&0xff) );		
//		pMsg->Append( m_nodeId );
//		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	}
//
//	if( IsSleepingNode( _nodeId ) )
//	{
//		Log::Write( "Postpone SetWakeupInterval until device wakes up" );
//		SendSleepingMsg( _nodeId, pMsg );
//	}
//	else
//	{
//		SendMsg( pMsg );
//	}
//}
//
////-----------------------------------------------------------------------------
//// <Driver::RequestNodeNeighborUpdate>
//// 
////-----------------------------------------------------------------------------
//void Driver::RequestNodeNeighborUpdate
//(
//	uint8 _nodeId
//)
//{
//	Log::Write( "Requesting Neighbour Update for node %d", _nodeId );
//
//	Msg* pMsg = new Msg( "Requesting Neighbour Update", _nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE, true );
//	pMsg->Append( _nodeId );
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::RequestMultilevelSensorReport>
//// 
////-----------------------------------------------------------------------------
//void Driver::RequestMultilevelSensorReport
//(
//	uint8 _nodeId
//)
//{
//	Log::Write( "Requesting Multilevel Sensor Report for node %d", _nodeId );
//
//	Msg* pMsg = new Msg( "Request Multilevel Sensor Report", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
//	pMsg->Append( _nodeId );
//	pMsg->Append( 2 );
//	pMsg->Append( COMMAND_CLASS_SENSOR_MULTILEVEL );
//	pMsg->Append( SENSOR_MULTILEVEL_GET );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::RequestMultilevelSensorReportInstance>
//// 
////-----------------------------------------------------------------------------
//void Driver::RequestMultilevelSensorReportInstance
//(
//	uint8 _nodeId,
//	uint8 _instance
//)
//{
//	Log::Write( "Requesting Multilevel Sensor Report Instance for node %d", _nodeId );
//
//	Msg* pMsg = new Msg( "Request Multilevel Sensor Report Instance", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
//	pMsg->Append( _nodeId );
//	pMsg->Append( 5 );
//	pMsg->Append( COMMAND_CLASS_MULTI_INSTANCE );
//	pMsg->Append( MULTI_INSTANCE_CMD_ENCAP );
//	pMsg->Append( _instance );
//	pMsg->Append( COMMAND_CLASS_SENSOR_MULTILEVEL );
//	pMsg->Append( SENSOR_MULTILEVEL_GET );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::GetMultiInstance>
//// 
////-----------------------------------------------------------------------------
//void Driver::GetMultiInstance
//(
//	uint8 _nodeId,
//	uint8 _commandClass
//)
//{
//	Log::Write( "Get Multi Instance for node %d", _nodeId );
//
//	Msg* pMsg = new Msg( "Get Multi Instance", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
//	pMsg->Append( _nodeId );
//	pMsg->Append( 3 );
//	pMsg->Append( COMMAND_CLASS_MULTI_INSTANCE );
//	pMsg->Append( MULTI_INSTANCE_GET );
//	pMsg->Append( _commandClass );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	SendMsg( pMsg );
//}
//
////-----------------------------------------------------------------------------
//// <Driver::GetMeter>
//// 
////-----------------------------------------------------------------------------
//void Driver::GetMeter
//(
//	uint8 _nodeId
//)
//{
//	Log::Write( "Get Meter on node %d", _nodeId );
//
//	Msg* pMsg = new Msg( "Get Meter", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
//	pMsg->Append( _nodeId );
//	pMsg->Append( 2 );
//	pMsg->Append( COMMAND_CLASS_METER );
//	pMsg->Append( METER_GET );
//	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	SendMsg( pMsg );
//}

//-----------------------------------------------------------------------------
// <Driver::ReadMemory>
// 
//-----------------------------------------------------------------------------
void Driver::ReadMemory
(
	uint16 _offset
)
{
	Log::Write( "Reading eeprom at offset %d", _offset );

	Msg* pMsg = new Msg( "Read Memory", 0xff, REQUEST, FUNC_ID_ZW_READ_MEMORY, false );
	pMsg->Append( (uint8)((_offset>>8)&0xff) );
	pMsg->Append( (uint8)(_offset&0xff) );
	pMsg->Append( 64 );
	SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Driver::UpdateEvents>
// Set and reset events according to the states of various queues and flags
//-----------------------------------------------------------------------------
void Driver::UpdateEvents
(
)
{
	if( m_bWaitingForAck || m_expectedCallbackId || m_expectedReply )
	{
		// Waiting for ack, callback or a specific message type, so we can't transmit anything yet.
		m_pSendEvent->Reset();
	}
	else
	{
		// Allow transmissions to occur
		if( ( !m_sendQueue.empty() ) || ( !m_infoQueue.empty() ) )
		{
			m_pSendEvent->Set();
		}
	}
}




