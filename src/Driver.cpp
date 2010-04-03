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
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "Defs.h"
#include "Driver.h"
#include "Manager.h"
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
#include "ValueBool.h"
#include "ValueByte.h"
#include "ValueDecimal.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueShort.h"
#include "ValueString.h"

#include "ValueStore.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <Driver::Driver>
// Constructor
//-----------------------------------------------------------------------------
Driver::Driver
( 
	string const& _serialPortName
):
	m_serialPortName( _serialPortName ),
	m_homeId( 0 ),
	m_pollInterval( 30 ),					// By default, every polled device is queried once every 30 seconds
	m_waitingForAck( false ),
	m_expectedReply( 0 ),
	m_expectedCallbackId( 0 ),
	m_exit( false ),
	m_driverThread( new Thread() ),
	m_exitEvent( new Event() ),	
	m_serialPort( new SerialPort() ),
	m_serialMutex( new Mutex() ),
	m_sendThread( new Thread() ),
	m_sendMutex( new Mutex() ),
	m_sendEvent( new Event() ),
	m_pollThread( new Thread() ),	
	m_pollMutex( new Mutex() ),
	m_infoMutex( new Mutex() ),
	m_capabilities( 0 )
{
	// Clear the nodes array
	memset( m_nodes, 0, sizeof(Node*) * 256 );
}

//-----------------------------------------------------------------------------
// <Driver::Driver>
// Destructor
//-----------------------------------------------------------------------------
Driver::~Driver
(
)
{
	m_exitEvent->Set();

	delete m_pollThread;
	delete m_sendThread;
	delete m_driverThread;

	delete m_serialPort;
	delete m_serialMutex;
	
	delete m_sendMutex;
	delete m_pollMutex;
	delete m_infoMutex;

	delete m_sendEvent;
	delete m_exitEvent;

	// Clear the node data
	for( int i=0; i<256; ++i )
	{
		if( m_nodes[i] )
		{
			delete m_nodes[i];
			m_nodes[i] = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::Start>
// Start the driver thread
//-----------------------------------------------------------------------------
void Driver::Start
(
)
{
	// Start the thread that will handle communications with the Z-Wave network
	m_driverThread->Start( Driver::DriverThreadEntryPoint, this );
}

//-----------------------------------------------------------------------------
// <Driver::DriverThreadEntryPoint>
// Entry point of the thread for creating and managing the worker threads
//-----------------------------------------------------------------------------
void Driver::DriverThreadEntryPoint
( 
	void* _context 
)
{
	Driver* driver = (Driver*)_context;
	if( driver )
	{
		driver->DriverThreadProc();
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
				m_serialMutex->Lock();
					
				// Consume any available messages
				while( ReadMsg() );

				// Release our lock on the serial port 
				m_serialMutex->Release();

				// Wait for more data to appear at the serial port
				m_serialPort->Wait( Event::Timeout_Infinite );
				
				// Check for exit
				if( m_exit )
				{
					return;
				}
			}
		}

		++attempts;
		if( attempts < 25 )		
		{
			// Retry every 5 seconds for the first two minutes			
			if( m_exitEvent->Wait( 5000 ) )
			{
				// Exit signalled.
				break;
			}
		}
		else
		{
			// Retry every 30 seconds after that
			if( m_exitEvent->Wait( 30000 ) )
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
	m_waitingForAck = false;

	// Open the serial port
	Log::Write( "Opening serial port %s", m_serialPortName.c_str() );

	if( !m_serialPort->Open( m_serialPortName, 115200, SerialPort::Parity_None, SerialPort::StopBits_One ) )
	{
	 	Log::Write( "Failed to init the controller (attempt %d)", _attempts );
		return false;
	}

	// Serial port opened successfully, so we need to start all the worker threads
	m_sendThread->Start( Driver::SendThreadEntryPoint, this );
	m_pollThread->Start( Driver::PollThreadEntryPoint, this );

	// Send a NAK to the ZWave device
	uint8 nak = NAK;
	m_serialPort->Write( &nak, 1 );

	// Send commands to retrieve properties from the Z-Wave interface 
	Msg* msg;

	Log::Write( "Get version" );
	msg = new Msg( "Get version", 0xff, REQUEST, ZW_GET_VERSION, false );
	SendMsg( msg );

	Log::Write( "Get home/node id" );
	msg = new Msg( "Get home/node id", 0xff, REQUEST, ZW_MEMORY_GET_ID, false );
	SendMsg( msg );

	Log::Write( "Get capabilities" );
	msg = new Msg( "Get capabilities", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_CAPABILITIES, false );
	SendMsg( msg );
	
	//Log::Write( "Get SUC node id" );
	//msg = new Msg( "Get SUC node id", 0xff, REQUEST, FUNC_ID_ZW_GET_SUC_NODE_ID, false );
	//SendMsg( msg );
	
	Log::Write( "Get init data" );
	msg = new Msg( "Get init data", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false );
	SendMsg( msg );

	// Init successful
	return true;
}

//-----------------------------------------------------------------------------
//	Configuration
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::ReadConfig>
// Read our configuration from an XML document
//-----------------------------------------------------------------------------
bool Driver::ReadConfig
(
)
{
	char str[32];
	int32 intVal;

	// Load the XML document that contains the driver configuration
	snprintf( str, 32, "zwcfg_0x%08x.xml", m_homeId );
	string filename =  Manager::Get()->GetUserPath() + string(str);

	TiXmlDocument doc;
	if( !doc.LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		return false;
	}

	TiXmlElement const* driverElement = doc.RootElement();

	// Home ID
	char const* homeIdStr = driverElement->Attribute( "home_id" );
	if( homeIdStr )
	{
		char* p;
		uint32 homeId = (uint32)strtol( homeIdStr, &p, 0 );

		if( homeId != m_homeId )
		{
			Log::Write( "Driver::ReadConfig - Home ID in file %s is incorrect", filename.c_str() );
			return false;
		}
	}
	else
	{
		Log::Write( "Driver::ReadConfig - Home ID is missing from file %s", filename.c_str() );
		return false;
	}

	// Node ID
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "node_id", &intVal ) )
	{
		if( (uint8)intVal != m_nodeId )
		{
			Log::Write( "Driver::ReadConfig - Controller Node ID in file %s is incorrect", filename.c_str() );
			return false;
		}
	}
	else
	{
		Log::Write( "Driver::ReadConfig - Node ID is missing from file %s", filename.c_str() );
		return false;
	}

	// Capabilities
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "capabilities", &intVal ) )
	{
		m_capabilities = (uint8)intVal;
	}

	// Poll Interval
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "poll_interval", &intVal ) )
	{
		m_pollInterval = intVal;
	}

	// Read the nodes
	TiXmlElement const* nodeElement = driverElement->FirstChildElement();
	while( nodeElement )
	{
		char const* str = nodeElement->Value();
		if( str && !strcmp( str, "Node" ) )
		{
			// Get the node Id from the XML
			if( TIXML_SUCCESS == nodeElement->QueryIntAttribute( "id", &intVal ) )
			{
				uint8 nodeId = (uint8)intVal;
				Node* node = new Node( m_homeId, nodeId );
				m_nodes[nodeId] = node;

				// Read the rest of the node configuration from the XML
				node->ReadXML( nodeElement );
			}
		}

		nodeElement = nodeElement->NextSiblingElement();
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Driver::WriteConfig>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Driver::WriteConfig
(
)
{
	char str[32];

	// Create a new XML document to contain the driver configuration
	TiXmlDocument doc;
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
    TiXmlElement* driverElement = new TiXmlElement( "Driver" );
    doc.LinkEndChild( decl );
    doc.LinkEndChild( driverElement );  

	snprintf( str, 32, "0x%.8x", m_homeId );
	driverElement->SetAttribute( "home_id", str );

	snprintf( str, 32, "%d", m_nodeId );
	driverElement->SetAttribute( "node_id", str );

	snprintf( str, 32, "%d", m_capabilities );
	driverElement->SetAttribute( "capabilities", str );

	snprintf( str, 32, "%d", m_pollInterval );
	driverElement->SetAttribute( "poll_interval", str );

	for( int i=0; i<256; ++i )
	{
		if( m_nodes[i] )
		{
			m_nodes[i]->WriteXML( driverElement );
		}
	}

	snprintf( str, 32, "zwcfg_0x%08x.xml", m_homeId );
	string filename =  Manager::Get()->GetUserPath() + string(str);

	doc.SaveFile( filename.c_str() );
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
	Msg* _msg
)
{
	_msg->Finalize();

	if( Node* node = m_nodes[_msg->GetTargetNodeId()] )
	{
		if( !node->IsListeningDevice() )
		{
			if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				if( !wakeUp->IsAwake() )
				{
					Log::Write( "" );
					Log::Write( "Queuing Wake-Up Command: %s", _msg->GetAsString().c_str() );
					wakeUp->QueueMsg( _msg );
					return;
				}
			}
		}
	}

	Log::Write( "Queuing command: %s", _msg->GetAsString().c_str() );
	m_sendMutex->Lock();
	m_sendQueue.push_back( _msg );
	m_sendMutex->Release();
	UpdateEvents();
}

//-----------------------------------------------------------------------------
// <Driver::SendThreadEntryPoint>
// Entry point of the thread for sending Z-Wave messages
//-----------------------------------------------------------------------------
void Driver::SendThreadEntryPoint
( 
	void* _context 
)
{
	Driver* driver = (Driver*)_context;
	if( driver )
	{
		driver->SendThreadProc();
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
		bool eventSet = m_sendEvent->Wait( timeout );

		// Check for exit
		if( m_exit )
		{
			return;
		}

		if( eventSet )
		{
			// We have a message to send.

			// Get the serial port mutex so we don't conflict with the read thread.
			m_serialMutex->Lock();

			// Get the send mutex so we can access the message queue.
			m_sendMutex->Lock();

			if( !m_sendQueue.empty() )
			{
				Msg* msg = m_sendQueue.front();
				
				m_expectedCallbackId = msg->GetCallbackId();
				m_expectedReply = msg->GetExpectedReply();
				m_expectedCommandClassId = msg->GetExpectedCommandClassId();
				m_waitingForAck = true;
				m_sendEvent->Reset();

				timeout = 5000;	// retry in 5 seconds

				msg->SetSendAttempts( msg->GetSendAttempts() + 1 );

				Log::Write( "" );
				Log::Write( "Sending command (Callback ID=%d, Expected Reply=%d) - %s", msg->GetCallbackId(), msg->GetExpectedReply(), msg->GetAsString().c_str() );

				m_serialPort->Write( msg->GetBuffer(), msg->GetLength() );
			}
			else
			{
				// Check for nodes requiring info requests
				uint8 nodeId = GetInfoRequest();
				if( nodeId != 0xff )
				{
					Msg* msg = new Msg( "Get Node Protocol Info", nodeId, REQUEST, FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO, false );
					msg->Append( nodeId );	
					SendMsg( msg ); 
				}
			}

			m_sendMutex->Release();
			m_serialMutex->Release();
		}
		else
		{
			// Timed out.  Set the timeout back to infinity
			timeout = Event::Timeout_Infinite;

			// Get the serial port mutex so we don't conflict with the read thread.
			m_serialMutex->Lock();

			// Get the send mutex so we can access the message queue.
			m_sendMutex->Lock();

			if( m_waitingForAck || m_expectedCallbackId || m_expectedReply )
			{
				// Timed out waiting for a response
				if( !m_sendQueue.empty() )
				{
					Msg* msg = m_sendQueue.front();

					if( msg->GetSendAttempts() > 2 )
					{
						// Give up
						Log::Write( "ERROR: Dropping command, expected response not received after three attempts");
						RemoveMsg();
					}
					else
					{
						// Resend
						if( msg->GetSendAttempts() > 0 )
						{
							Log::Write( "Timeout" );

							// In case this is a sleeping node, we first try to move 
							// its pending messages to its wake-up queue.
							if( !MoveMessagesToWakeUpQueue( msg->GetTargetNodeId() ) )
							{
								// The attempt failed, so the node is either not a sleeping node, or the move
								// failed for another reason.  We'll just retry sending the message.
								Log::Write( "Resending message (attempt %d)", msg->GetSendAttempts() );
							}
						}
					}

					m_waitingForAck = 0;	
					m_expectedCallbackId = 0;
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					UpdateEvents();
				}
			}

			m_sendMutex->Release();
			m_serialMutex->Release();
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
	m_sendMutex->Lock();
	
	delete m_sendQueue.front();
	m_sendQueue.pop_front();
	if( m_sendQueue.empty() )
	{
		//No messages left, so clear the event
		m_sendEvent->Reset();
	}
	
	m_sendMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::TriggerResend>
// Cause a message to be resent immediately
//-----------------------------------------------------------------------------
void Driver::TriggerResend
( 
)
{
	m_sendMutex->Lock();
	
	Msg* msg = m_sendQueue.front();
	msg->SetSendAttempts( 0 );

	m_waitingForAck = 0;	
	m_expectedCallbackId = 0;
	m_expectedReply = 0;
	m_expectedCommandClassId = 0;

	m_sendEvent->Set();

	m_sendMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::MoveMessagesToWakeUpQueue>
// Move messages for a sleeping device to its wake-up queue
//-----------------------------------------------------------------------------
bool Driver::MoveMessagesToWakeUpQueue
(
	uint8 const _targetNodeId
)
{
	// If the target node is one that goes to sleep, transfer
	// all messages for it to its Wake-Up queue.
	if( Node* node = m_nodes[_targetNodeId] )
	{
		if( !node->IsListeningDevice() )
		{
			if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				// Mark the node as asleep
				wakeUp->SetAwake( false );

				// Move all messages for this node to the wake-up queue									
				m_sendMutex->Lock();

				list<Msg*>::iterator it = m_sendQueue.begin();
				while( it != m_sendQueue.end() )
				{
					if( _targetNodeId == (*it)->GetTargetNodeId() )
					{
						// This message is for the unresponsive node
						Log::Write( "Node not responding - moving message to Wake-Up queue: %s", (*it)->GetAsString().c_str() );
						wakeUp->QueueMsg( *it );
						m_sendQueue.erase( it++ );
					}
					else
					{
						++it;
					}
				}

				m_sendMutex->Release();
				
				// Move completed successfully
				return true;
			}
		}
	}

	// Failed to move messages
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeAwake>
// Mark a node as being awake
//-----------------------------------------------------------------------------
void Driver::SetNodeAwake
(
	uint8 const _nodeId
)
{
	if( m_nodes[_nodeId] )
	{
		if( !m_nodes[_nodeId]->IsListeningDevice() )
		{
			if( WakeUp* pWakeUp = static_cast<WakeUp*>( m_nodes[_nodeId]->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				pWakeUp->SetAwake( true );
			}
		}
	}
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
	uint8 buffer[1024];

	if( !m_serialPort->Read( buffer, 1 ) )
	{
		// Nothing to read
		return false;
	}

	switch( buffer[0] )
	{
		case SOF:
		{
			if( m_waitingForAck )
			{
				Log::Write( "SOF received when waiting for ACK...triggering resend" );
				TriggerResend();
			}

			// Read the length byte
			if ( !m_serialPort->Read( &buffer[1], 1 ))
			{
				break; // can't proceed
			}

			// Read the rest of the frame
			uint32 bytesRemaining = buffer[1];
			while( bytesRemaining )
			{
				bytesRemaining -= m_serialPort->Read( &buffer[2+(uint32)buffer[1]-bytesRemaining], bytesRemaining );
			}

			uint32 length = buffer[1] + 2;

			// Log the data
			string str = "";
			for( uint32 i=0; i<length; ++i ) 
			{
				if( i )
				{
					str += ", ";
				}

				char byteStr[8];
				snprintf( byteStr, sizeof(byteStr), "0x%.2x", buffer[i] );
				str += byteStr;
			}
			Log::Write( "" );
			Log::Write( "Received: %s", str.c_str() );

			// Verify checksum
			uint8 checksum = 0xff;
			for( uint32 i=1; i<(length-1); ++i ) 
			{
				checksum ^= buffer[i];
			}

			if( buffer[length-1] == checksum )
			{
				// Checksum correct - send ACK
				uint8 ack = ACK;
				m_serialPort->Write( &ack, 1 );

				// Process the received message
				ProcessMsg( &buffer[2] );
			}
			else
			{
				Log::Write( "Checksum incorrect - sending NAK" );
				uint8 nak = NAK;
				m_serialPort->Write( &nak, 1 );
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
			Log::Write( "ACK received CallbackId %d Reply %d", m_expectedCallbackId, m_expectedReply );
			m_waitingForAck = false;
			
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
			Log::Write( "ERROR! Out of frame flow! (0x%.2x)", buffer[0] );
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
	uint8* _data
)
{
	bool handleCallback = true;

	if( RESPONSE == _data[0] )
	{
		switch( _data[1] )
		{
			case ZW_GET_VERSION:
			{
				Log::Write( "Received reply to ZW_GET_VERSION: %s", ((int8*)&_data[2]) );
				break;
			}
			case FUNC_ID_SERIAL_API_GET_CAPABILITIES:
			{
				HandleGetCapabilitiesResponse( _data );
				break;
			}
			case FUNC_ID_ZW_ENABLE_SUC:
			{
				HandleEnableSUCResponse( _data );
				break;
			}
			case FUNC_ID_ZW_SET_SUC_NODE_ID:
			{
				HandleSetSUCNodeIdResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_SUC_NODE_ID:
			{
				HandleGetSUCNodeIdResponse( _data );
				break;
			}
			case ZW_MEMORY_GET_ID:
			{
				HandleMemoryGetIdResponse( _data );
				break;
			}
			case FUNC_ID_SERIAL_API_GET_INIT_DATA:
			{
				HandleSerialAPIGetInitDataResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO:
			{
				HandleGetNodeProtocolInfoResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_INFO:
			{
				Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NODE_INFO" );
				break;
			}
			case FUNC_ID_ZW_SEND_DATA:
			{
				HandleSendDataResponse( _data );
				handleCallback = false;			// Skip the callback handling - a subsequent FUNC_ID_ZW_SEND_DATA request will deal with that
				break;
			}
			default:
			{
				Log::Write( "TODO: handle response for %d", _data[1] );
				break;
			}
		}
	} 
	else if( REQUEST == _data[0] )
	{
		switch( _data[1] )
		{
			case FUNC_ID_ZW_SEND_DATA:
			{
				handleCallback = !HandleSendDataRequest( _data );
				break;
			}
			case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
			{
				HandleAddNodeToNetworkRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
			{
				HandleRemoveNodeFromNetworkRequest( _data );
				break;
			}
			case FUNC_ID_APPLICATION_COMMAND_HANDLER:
			{
				HandleApplicationCommandHandlerRequest( _data );
				break;
			}
			case FUNC_ID_ZW_APPLICATION_UPDATE:
			{
				handleCallback = !HandleApplicationUpdateRequest( _data );
				break;
			}
			default:
			{
				break;
			}	
		}
	}

	// Generic callback handling
	if( handleCallback )
	{
		if( ( m_expectedCallbackId || m_expectedReply ) )
		{
			if( m_expectedCallbackId )
			{
				if( m_expectedCallbackId == _data[2] )
				{
					Log::Write( "Expected callbackId was received" );
					m_expectedCallbackId = 0;
				}
			}
			if( m_expectedReply )
			{
				if( m_expectedReply == _data[1] )
				{
					if( m_expectedCommandClassId && ( m_expectedReply == FUNC_ID_APPLICATION_COMMAND_HANDLER ) )
					{
						if( m_expectedCommandClassId == _data[5] )
						{
							Log::Write( "Expected reply and command class was received" );
							m_expectedReply = 0;
							m_expectedCommandClassId = 0;
						}
					}
					else
					{
						Log::Write( "Expected reply was received" );
						m_expectedReply = 0;
					}
				}
			}

			if( !( m_expectedCallbackId || m_expectedReply ) )
			{
				Log::Write( "Message transaction complete" );
				RemoveMsg();
			}
		}
	}

	UpdateEvents();
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetCapabilitiesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetCapabilitiesResponse
(
	uint8* _data
)
{
	Log::Write( "Received reply to GetCapabilities.  Node ID = %d", _data[0] );
	//2009-06-14 11:41:14:585 Received: 0x01, 0x2b, 0x01, 0x07, 0x02, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xfe, 0x80, 0xfe, 0x80, 0x03, 0x00, 0x00, 0x00, 0xfb, 0x9f, 0x3b, 0x80, 0x07, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59
}

//-----------------------------------------------------------------------------
// <Driver::HandleEnableSUCResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleEnableSUCResponse
(
	uint8* _data
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
	uint8* _data
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
	uint8* _data
)
{
	Log::Write( "Received reply to GET_SUC_NODE_ID.  Node ID = %d", _data[2] );

	if( _data[2] == 0)
	{
		Log::Write( "No SUC, so we become SUC" );
		
		Msg* msg;

		msg = new Msg( "Enable SUC", m_nodeId, REQUEST, FUNC_ID_ZW_ENABLE_SUC, false );
		msg->Append( 1 );	
//		msg->Append( ZW_SUC_FUNC_BASIC_SUC );			// SUC
		msg->Append( ZW_SUC_FUNC_NODEID_SERVER );		// SIS
		SendMsg( msg ); 

		msg = new Msg( "Set SUC node ID", m_nodeId, REQUEST, FUNC_ID_ZW_SET_SUC_NODE_ID, false );
		msg->Append( m_nodeId );
		msg->Append( 1 );								// TRUE, we want to be SUC/SIS
		msg->Append( 0 );								// no low power
		msg->Append( ZW_SUC_FUNC_NODEID_SERVER );
		SendMsg( msg ); 
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleMemoryGetIdResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleMemoryGetIdResponse
(
	uint8* _data
)
{
	Log::Write( "Received reply to ZW_MEMORY_GET_ID. Home ID = 0x%02x%02x%02x%02x.  Our node ID = %d", _data[2], _data[3], _data[4], _data[5], _data[6] );
	m_homeId = (((uint32)_data[2])<<24) | (((uint32)_data[3])<<16) | (((uint32)_data[4])<<8) | ((uint32)_data[5]);
	m_nodeId = _data[6];
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialAPIGetInitDataResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSerialAPIGetInitDataResponse
(
	uint8* _data
)
{
	int32 i;

	// Clear any existing node data
	for( i=0; i<256; ++i )
	{
		if( m_nodes[i] )
		{
			delete m_nodes[i];
			m_nodes[i] = NULL;
		}
	}

	// Mark the driver as ready (we have to do this first or 
	// all the code handling notifications will go awry).
	Manager::Get()->SetDriverReady( this );

	// Read the config file first, to get the last known state
	ReadConfig();

	Log::Write( "Received reply to FUNC_ID_SERIAL_API_GET_INIT_DATA:" );
	m_capabilities = _data[3];

	if( _data[4] == NUM_NODE_BITFIELD_BYTES )
	{
		for( i=0; i<NUM_NODE_BITFIELD_BYTES; ++i)
		{
			for( int32 j=0; j<8; ++j )
			{
				uint8 nodeId = (i*8)+j+1;
				if( _data[i+5] & (0x01 << j) )
				{
					if( NULL == m_nodes[nodeId] )
					{
						// This node was not in the config
						Log::Write( "Found new node %d", nodeId );

						// Create the node
						m_nodes[nodeId] = new Node( m_homeId, nodeId );

						// Request the node protocol info
						AddInfoRequest( nodeId );		
					}
					else
					{
						// The node was read in from the config, so we 
						// only need to get its current state
						m_nodes[nodeId]->RequestState( CommandClass::RequestFlag_Session | CommandClass::RequestFlag_Dynamic );
					}
				}
				else
				{
					if( m_nodes[nodeId] )
					{
						// This node was in the config, but no longer
						// exists in the Z-Wave network
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
	uint8* _data
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
		m_nodes[nodeId]->UpdateProtocolInfo( &_data[2] );
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendDataResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendDataResponse
(
	uint8* _data
)
{
	if( 0 == _data[2] )
	{
		Log::Write("ERROR: ZW_SEND could not be delivered to Z-Wave stack");
	}
	else if( 1 == _data[2] )
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
bool Driver::HandleSendDataRequest
(
	uint8* _data
)
{
	bool messageRemoved = false;

	Log::Write( "ZW_SEND Request with callback ID %d received (expected %d)", _data[2], m_expectedCallbackId );

	if( _data[2] != m_expectedCallbackId )
	{
		// Wrong callback ID
		Log::Write( "ERROR: Callback ID is invalid" );
	}
	else 
	{
		// Callback ID matches our expectation
		switch( _data[3] )
		{
			case 0:
			{
				// command reception acknowledged by node
				if( m_expectedReply == 0 )
				{
					// We're not waiting for any particular reply, so we're done.
					Log::Write("ZW_SEND was successful, removing command");	
					RemoveMsg();
					messageRemoved = true;
				}
				m_expectedCallbackId = 0;
				break;
			}
			case 1:
			{
				// ZW_SEND failed
				Log::Write( "Error: ZW_SEND failed." );
				m_sendMutex->Lock();
				if( !m_sendQueue.empty() )
				{
					if( m_sendQueue.front()->GetSendAttempts() >= 3 )
					{
						// Can't deliver message, so abort
						Log::Write( "Removing message after three tries" );
						RemoveMsg();
						messageRemoved = true;
					}
					else 
					{
						// In case the failure is due to the target being a sleeping node, we 
						// first try to move its pending messages to its wake-up queue.
						MoveMessagesToWakeUpQueue( m_sendQueue.front()->GetTargetNodeId() );

						// We need do no more here.  If the move failed, the node is probably not
						// a sleeping node and the message will automatically be resent.
					}
				}
				else
				{
					// Message queue should not be empty
					assert(0);
				}

				m_waitingForAck = 0;	
				m_expectedCallbackId = 0;
				m_expectedReply = 0;
				m_expectedCommandClassId = 0;

				m_sendMutex->Release();		
				break;
			}
			default:
			{
				Log::Write( "ERROR: ZW_SEND Response is invalid" );
			}
		}

		UpdateEvents();
	}

	return messageRemoved;
}

//-----------------------------------------------------------------------------
// <Driver::HandleAddNodeToNetworkRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleAddNodeToNetworkRequest
(
	uint8* _data
)
{
	Log::Write( "FUNC_ID_ZW_ADD_NODE_TO_NETWORK:" );
	
	switch( _data[3] )
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
			Log::Write( "Adding node ID %d", _data[4] );
			break;
		}
		case ADD_NODE_STATUS_ADDING_CONTROLLER:
		{
			Log::Write( "ADD_NODE_STATUS_ADDING_CONTROLLER");
			Log::Write( "Adding node ID %d", _data[4] );
			break;
		}
		case ADD_NODE_STATUS_PROTOCOL_DONE:
		{
			Log::Write( "ADD_NODE_STATUS_PROTOCOL_DONE" );
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
	uint8* _data
)
{
	Log::Write( "FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:" );
	
	switch( _data[3] ) 
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
	uint8* _data
)
{
	uint8 nodeId = _data[3];
	uint8 ClassId = _data[5];
	uint8 ClassIdMinor = _data[6];
	uint8 ValueFromDevice = _data[7];
	CommandClass* pfnDeviceEventVectorClass;
	uint8 StatusData[10];

	SetNodeAwake( nodeId ); 
	
	switch (ClassId)
	{
		case COMMAND_CLASS_BASIC:
		{			
			switch (ClassIdMinor)
			{
			   	case BASIC_SET:
					//get the class function pointer
					if( pfnDeviceEventVectorClass = CommandClasses::CreateCommandClass( ClassId, m_homeId, nodeId ) )
					{
						StatusData[0] = 0x01; StatusData[1]=ValueFromDevice;
						pfnDeviceEventVectorClass->HandleMsg( StatusData,0x02,0x00);
					}
					break;
				
				case BASIC_REPORT:
					break;
			}
			break;
		}
		case COMMAND_CLASS_HAIL:
		{			
			if( pfnDeviceEventVectorClass = CommandClasses::CreateCommandClass(ClassId, m_homeId, nodeId ) )
			{
				StatusData[0] = 0x01; StatusData[1]=ValueFromDevice;
				pfnDeviceEventVectorClass->HandleMsg( StatusData,0x02,0x00);
			}
			break;
		}
		case COMMAND_CLASS_APPLICATION_STATUS:
		{
			break;	//TODO: Test this class function or implement
		}
		default:
		{			
			if( m_nodes[nodeId] )
		 	{
				m_nodes[nodeId]->ApplicationCommandHandler( _data );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleApplicationUpdateRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleApplicationUpdateRequest
(
	uint8* _data
)
{
	bool messageRemoved = false;

	uint8 nodeId = _data[3];

	switch( _data[2] )
	{
		case UPDATE_STATE_NODE_INFO_RECEIVED:
		{
			Log::Write( "UPDATE_STATE_NODE_INFO_RECEIVED from node %d", nodeId );
			SetNodeAwake( nodeId );
			if( Node* node = m_nodes[nodeId] )
			{
				node->UpdateNodeInfo( &_data[8], _data[4] - 3 );
			}
			break;
		}

		case UPDATE_STATE_NODE_INFO_REQ_FAILED:
		{
			Log::Write( "FUNC_ID_ZW_APPLICATION_UPDATE: UPDATE_STATE_NODE_INFO_REQ_FAILED received" );
			
			// Just in case the failure was due to the node being asleep, we try
			// to move its pending messages to its wakeup queue.  If it is not
			// a sleeping device, this will have no effect.
			m_sendMutex->Lock();
			if( !m_sendQueue.empty() )
			{
				if( MoveMessagesToWakeUpQueue( m_sendQueue.front()->GetTargetNodeId() ) )
				{
					// The messages were removed, so set the flag so 
					// the caller doesn't try to remove it again.
					messageRemoved = true;

					m_waitingForAck = 0;	
					m_expectedCallbackId = 0;
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
				}
			}
			m_sendMutex->Release();
			break;
		}
		case UPDATE_STATE_NEW_ID_ASSIGNED:
		{
			Log::Write( "** Network change **: ID %d was assigned to a new Z-Wave node", nodeId );
			delete m_nodes[nodeId];
			m_nodes[nodeId] = new Node( m_homeId, nodeId );

			// Request the node protocol info
			AddInfoRequest( nodeId );		
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

	return messageRemoved;
}

//-----------------------------------------------------------------------------
//	Polling Z-Wave devices
//-----------------------------------------------------------------------------
	
//-----------------------------------------------------------------------------
// <Driver::EnablePoll>
// Enable polling of a node
//-----------------------------------------------------------------------------
bool Driver::EnablePoll
( 
	uint8 const _nodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		// Add the node to the polling list
		m_pollMutex->Lock();

		// See if the node is already in the poll list.
		for( list<uint8>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
		{
			if( *it == _nodeId )
			{
				// It is already in the poll list, so we have nothing to do.
				m_pollMutex->Release();
				return true;
			}
		}

		// Not in the list, so we add it
		m_pollList.push_back( _nodeId );
		m_pollMutex->Release();
		return true;
	}

	Log::Write( "EnablePoll failed - node %d not found", _nodeId );
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::DisablePoll>
// Disable polling of a node
//-----------------------------------------------------------------------------
bool Driver::DisablePoll
( 
	uint8 const _nodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		m_pollMutex->Lock();

		// See if the node is already in the poll list.
		for( list<uint8>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
		{
			if( *it == _nodeId )
			{
				// Found it
				m_pollList.erase( it );
				m_pollMutex->Release();
				return true;
			}
		}

		// Not in the list
		m_pollMutex->Release();
	}

	Log::Write( "DisablePoll failed - node %d not found", _nodeId );
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::PollThreadEntryPoint>
// Entry point of the thread for poll Z-Wave devices
//-----------------------------------------------------------------------------
void Driver::PollThreadEntryPoint
( 
	void* _context 
)
{
	Driver* driver = (Driver*)_context;
	if( driver )
	{
		driver->PollThreadProc();
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
			m_pollMutex->Lock();
			
			if( !m_pollList.empty() )
			{
				// Get the next node to be polled
				uint8 nodeId = m_pollList.front();
			
				// Move it to the back of the list
				m_pollList.pop_front();
				m_pollList.push_back( nodeId );

				// Calculate the time before the next poll, so that all polls 
				// can take place within the user-specified interval.
				pollInterval /= m_pollList.size();

				// Request the state of the value from the node to which it belongs
				if( Node* node = GetNode( nodeId ) )
				{
					bool requestState = true;
					if( !node->IsListeningDevice() )
					{
						// The device is not awake all the time.  If it is not awake, we mark it
						// as requiring a poll.  The poll will be done next time the node wakes up.
						if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
						{
							if( !wakeUp->IsAwake() )
							{
								wakeUp->SetPollRequired();
								requestState = false;
							}
						}
					}

					if( requestState )
					{
						// Request an update of the value
						node->RequestState( CommandClass::RequestFlag_Dynamic );
					}
				}
			}

			m_pollMutex->Release();
		}

		// Wait for the interval to expire, while watching for exit events
		if( m_exitEvent->Wait( pollInterval ) )
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
	m_infoMutex->Lock();
	m_infoQueue.push_back( _nodeId );
	UpdateEvents();
	m_infoMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::RemoveInfoRequest>
// Remove a node from the info queue
//-----------------------------------------------------------------------------
void Driver::RemoveInfoRequest
( 
)
{
	m_infoMutex->Lock();
	if( !m_infoQueue.empty() )
	{
		m_infoQueue.pop_front();
		if( m_infoQueue.empty() )
		{
			UpdateEvents();
		}
	}
	m_infoMutex->Release();
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

	m_infoMutex->Lock();
	if( !m_infoQueue.empty() )
	{
		nodeId = m_infoQueue.front();
	}
	m_infoMutex->Release();
	return nodeId;
}

//-----------------------------------------------------------------------------
// <Driver::GetValueBool>
// Get the bool value object with the specified ID
//-----------------------------------------------------------------------------
ValueBool* Driver::GetValueBool
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* node = m_nodes[nodeId] )
	{
		return( node->GetValueBool( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetValueByte>
// Get the byte value object with the specified ID
//-----------------------------------------------------------------------------
ValueByte* Driver::GetValueByte
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* node = m_nodes[nodeId] )
	{
		return( node->GetValueByte( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetValueDecimal>
// Get the decimal value object with the specified ID
//-----------------------------------------------------------------------------
ValueDecimal* Driver::GetValueDecimal
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* node = m_nodes[nodeId] )
	{
		return( node->GetValueDecimal( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetValueInt>
// Get the int value object with the specified ID
//-----------------------------------------------------------------------------
ValueInt* Driver::GetValueInt
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* node = m_nodes[nodeId] )
	{
		return( node->GetValueInt( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetValueList>
// Get the list value object with the specified ID
//-----------------------------------------------------------------------------
ValueList* Driver::GetValueList
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* node = m_nodes[nodeId] )
	{
		return( node->GetValueList( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetValueShort>
// Get the short value object with the specified ID
//-----------------------------------------------------------------------------
ValueShort* Driver::GetValueShort
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* node = m_nodes[nodeId] )
	{
		return( node->GetValueShort( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetValueString>
// Get the string value object with the specified ID
//-----------------------------------------------------------------------------
ValueString* Driver::GetValueString
(
	ValueID const& _id
)
{
	// Get the node that stores this value
	uint8 nodeId = _id.GetNodeId();
	if( Node* node = m_nodes[nodeId] )
	{
		return( node->GetValueString( _id ) );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Controller commands
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::ResetController>
// Reset controller and erase all node information
//-----------------------------------------------------------------------------
void Driver::ResetController
(
)
{
	Log::Write( "Reset controller and erase all node information");
	Msg* msg = new Msg( "Reset controller and erase all node information", 0xff, REQUEST, FUNC_ID_ZW_SET_DEFAULT, true );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::SoftReset>
// Soft-reset the Z-Wave controller chip
//-----------------------------------------------------------------------------
void Driver::SoftReset
(
)
{
	Log::Write( "Soft-resetting the Z-Wave controller chip");
	Msg* msg = new Msg( "Soft-resetting the Z-Wave controller chip", 0xff, REQUEST, FUNC_ID_SERIAL_API_SOFT_RESET, false, false );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::RequestNodeNeighborUpdate>
// 
//-----------------------------------------------------------------------------
void Driver::RequestNodeNeighborUpdate
(
	uint8 _nodeId
)
{
	Log::Write( "Requesting Neighbour Update for node %d", _nodeId );
	Msg* msg = new Msg( "Requesting Neighbour Update", _nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE, true );
	msg->Append( _nodeId );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::AssignReturnRoute>
// 
//-----------------------------------------------------------------------------
void Driver::AssignReturnRoute
(
	uint8 _nodeId,
	uint8 _targetNodeId
)
{
	Log::Write( "Assign Return Route for Node %d, Target Node %d", _nodeId, _targetNodeId );
	Msg* msg = new Msg( "Assign Return Route", _nodeId, REQUEST, FUNC_ID_ZW_ASSIGN_RETURN_ROUTE, true );		
	msg->Append( _nodeId );
	msg->Append( _targetNodeId );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	SendMsg( msg );
} 

//-----------------------------------------------------------------------------
// <Driver::BeginAddNode>
// Set the controller into AddNode mode
//-----------------------------------------------------------------------------
void Driver::BeginAddNode
(
	bool _highPower // = false
)
{
	Log::Write( "Add Node - Begin" );
	Msg* msg = new Msg( "Add Node - Begin", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
	msg->Append( _highPower ? ADD_NODE_ANY | ADD_NODE_OPTION_HIGH_POWER : ADD_NODE_ANY );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::BeginAddController>
// Set the controller into AddController mode
//-----------------------------------------------------------------------------
void Driver::BeginAddController
(
	bool _highPower // = false
)
{
	Log::Write( "Add Controller - Begin" );
	Msg* msg = new Msg( "Add Controller - Begin", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
	msg->Append( _highPower ? ADD_NODE_CONTROLLER | ADD_NODE_OPTION_HIGH_POWER : ADD_NODE_CONTROLLER );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::EndAddNode>
// Take the controller out of AddNode mode
//-----------------------------------------------------------------------------
void Driver::EndAddNode
(
)
{
	Log::Write( "Add Node - End" );
	Msg* msg = new Msg( "Add Node - End", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, false, false );
	msg->Append( ADD_NODE_STOP );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::BeginRemoveNode>
// Set the controller into RemoveNode mode
//-----------------------------------------------------------------------------
void Driver::BeginRemoveNode
(
)
{
	Log::Write( "Remove Node - Begin" );
	Msg* msg = new Msg( "Remove Node - Begin", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
	msg->Append( REMOVE_NODE_ANY );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::EndRemoveNode>
// Take the controller out of RemoveNode mode
//-----------------------------------------------------------------------------
void Driver::EndRemoveNode
(
)
{
	Log::Write( "Remove Node - End" );
	Msg* msg = new Msg( "Remove Node - End", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, false, false );
	msg->Append( REMOVE_NODE_STOP );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::BeginReplicateController>
// Set the controller into ReplicateController mode
//-----------------------------------------------------------------------------
void Driver::BeginReplicateController
(
)
{
	Log::Write( "Replicate Controller - Begin" );
	Msg* msg = new Msg( "Replicate Controller - Begin", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
	msg->Append( 1 );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::EndReplicateController>
// Take the controller out of ReplicateController mode
//-----------------------------------------------------------------------------
void Driver::EndReplicateController
(
)
{
	Log::Write( "Replicate Controller - End" );
	Msg* msg = new Msg(  "Replicate Controller - End", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
	msg->Append( 0 );
	SendMsg( msg );

	Log::Write( "Get new init data after replication" );
	msg = new Msg( "Get new init data after replication", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::UpdateEvents>
// Set and reset events according to the states of various queues and flags
//-----------------------------------------------------------------------------
void Driver::UpdateEvents
(
)
{
	if( m_waitingForAck || m_expectedCallbackId || m_expectedReply )
	{
		// Waiting for ack, callback or a specific message type, so we can't transmit anything yet.
		m_sendEvent->Reset();
	}
	else
	{
		// Allow transmissions to occur
		if( ( !m_sendQueue.empty() ) || ( !m_infoQueue.empty() ) )
		{
			m_sendEvent->Set();
		}
	}
}




