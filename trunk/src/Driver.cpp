//-----------------------------------------------------------------------------
//
//	Driver.cpp
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
#include "Notification.h"

#include "Event.h"
#include "Mutex.h"
#include "SerialPort.h"
#include "Thread.h"
#include "Log.h"

#include "CommandClasses.h"
#include "WakeUp.h"
#include "ControllerReplication.h"

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

static char const* c_libraryTypeNames[] = 
{
	"Unknown",
	"Static Controller",
	"Controller",       
	"Enhanced Slave",   
	"Slave",            
	"Installer",
	"Routing Slave",
	"Bridge Controller",    
	"Device Under Test"
};

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
	m_init( false ),
	m_driverThread( new Thread( "driver" ) ),
	m_exitEvent( new Event() ),	
	m_serialPort( new SerialPort() ),
	m_serialMutex( new Mutex() ),
	m_sendThread( new Thread( "send" ) ),
	m_sendMutex( new Mutex() ),
	m_sendEvent( new Event() ),
	m_pollThread( new Thread( "poll" ) ),
	m_pollMutex( new Mutex() ),
	m_infoMutex( new Mutex() ),
	m_nodeMutex( new Mutex() ),
	m_initCaps( 0 ),
	m_controllerCaps( 0 ),
	m_controllerReplication( NULL ),
	m_controllerState( ControllerState_Normal ),
	m_controllerCommand( ControllerCommand_None ),
	m_controllerCallback( NULL ),
	m_controllerCallbackContext( NULL ),
	m_controllerAdded( false ),
	m_controllerCommandNode( 0 )
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
	// The order of the statements below has been achieved by mitigating freed memory
  	//references using a memory allocator checker. Do not rearrange unless you are
  	//certain memory won't be referenced out of order. --Greg Satz, April 2010
  	
	m_exit = true;
	m_exitEvent->Set();
	m_sendEvent->Set();
	m_pollThread->Stop();
	m_sendThread->Stop();
	m_driverThread->Stop();

	delete m_serialPort;

	delete m_pollThread;
	delete m_sendThread;
	delete m_driverThread;

	delete m_serialMutex;
	
	delete m_sendMutex;
	delete m_pollMutex;
	delete m_infoMutex;

	delete m_sendEvent;
	delete m_exitEvent;

	// Clear the send Queue
	while( !m_sendQueue.empty() )
	{
		Msg *msg = m_sendQueue.front();
		m_sendQueue.pop_front();
		delete msg;
	}

	// Clear the node data
	for( int i=0; i<256; ++i )
	{
		if( GetNode( i ) )
		{
			Notification* notification = new Notification( Notification::Type_NodeRemoved );
			notification->SetHomeAndNodeIds( m_homeId, i );
			QueueNotification( notification ); 

			delete m_nodes[i];
			m_nodes[i] = NULL;
			ReleaseNodes();
		}
	}

	NotifyWatchers();
	delete m_nodeMutex;
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

				// Send any pending notifications
				NotifyWatchers();

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

	Log::Write( "Get Version" );
	msg = new Msg( "FUNC_ID_ZW_GET_VERSION", 0xff, REQUEST, FUNC_ID_ZW_GET_VERSION, false );
	SendMsg( msg );

	Log::Write( "Get Home and Node IDs" );
	msg = new Msg( "FUNC_ID_ZW_MEMORY_GET_ID", 0xff, REQUEST, FUNC_ID_ZW_MEMORY_GET_ID, false );
	SendMsg( msg );

	Log::Write( "Get Controller Capabilities" );
	msg = new Msg( "FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES", 0xff, REQUEST, FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES, false );
	SendMsg( msg );

	Log::Write( "Get Serial API Capabilities" );
	msg = new Msg( "FUNC_ID_SERIAL_API_GET_CAPABILITIES", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_CAPABILITIES, false );
	SendMsg( msg );

	//Log::Write( "Get SUC node id" );
	//msg = new Msg( "Get SUC node id", 0xff, REQUEST, FUNC_ID_ZW_GET_SUC_NODE_ID, false );
	//SendMsg( msg );
	
	Log::Write( "Get Init Data" );
	msg = new Msg( "FUNC_ID_SERIAL_API_GET_INIT_DATA", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false );
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
	snprintf( str, sizeof(str), "zwcfg_0x%08x.xml", m_homeId );
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
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "api_capabilities", &intVal ) )
	{
		m_initCaps = (uint8)intVal;
	}

	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "controller_capabilities", &intVal ) )
	{
		m_controllerCaps = (uint8)intVal;
	}

	// Poll Interval
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "poll_interval", &intVal ) )
	{
		m_pollInterval = intVal;
	}

	// Read the nodes
	LockNodes();
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

				Notification* notification = new Notification( Notification::Type_NodeAdded );
				notification->SetHomeAndNodeIds( m_homeId, nodeId );
				QueueNotification( notification ); 

				// Read the rest of the node configuration from the XML
				node->ReadXML( nodeElement );
			}
		}

		nodeElement = nodeElement->NextSiblingElement();
	}
	
	ReleaseNodes();
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

	snprintf( str, sizeof(str), "0x%.8x", m_homeId );
	driverElement->SetAttribute( "home_id", str );

	snprintf( str, sizeof(str), "%d", m_nodeId );
	driverElement->SetAttribute( "node_id", str );

	snprintf( str, sizeof(str), "%d", m_initCaps );
	driverElement->SetAttribute( "api_capabilities", str );

	snprintf( str, sizeof(str), "%d", m_controllerCaps );
	driverElement->SetAttribute( "controller_capabilities", str );

	snprintf( str, sizeof(str), "%d", m_pollInterval );
	driverElement->SetAttribute( "poll_interval", str );

	LockNodes();
	for( int i=0; i<256; ++i )
	{
		if( m_nodes[i] )
		{
			m_nodes[i]->WriteXML( driverElement );
		}
	}
	ReleaseNodes();

	snprintf( str, sizeof(str), "zwcfg_0x%08x.xml", m_homeId );
	string filename =  Manager::Get()->GetUserPath() + string(str);

	doc.SaveFile( filename.c_str() );
}

//-----------------------------------------------------------------------------
//	Controller
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::GetNode>
// Locks the nodes and returns a pointer to the requested one
//-----------------------------------------------------------------------------
Node* Driver::GetNode
(
	uint8 _nodeId
)
{
	LockNodes();
	if( Node* node = m_nodes[_nodeId] )
	{
		return node;
	}

	ReleaseNodes();
	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::LockNodes>
// Lock the nodes so that no other thread can modify them
//-----------------------------------------------------------------------------
void Driver::LockNodes
(
)
{
	m_nodeMutex->Lock();
}

//-----------------------------------------------------------------------------
// <Driver::ReleaseNodes>
// Unlock the nodes so that other threads can modify them
//-----------------------------------------------------------------------------
void Driver::ReleaseNodes
(
)
{
	m_nodeMutex->Release();
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

	if( Node* node = GetNode(_msg->GetTargetNodeId()) )
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
					ReleaseNodes();
					return;
				}

				if( _msg->IsWakeUpNoMoreInformationCommand() )
				{
					// This message will send the node to sleep
					wakeUp->SetAwake( false );
				}
			}
		}

		ReleaseNodes();
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
				uint8 nodeId = GetNodeInfoRequest();
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

							// We can't have the send mutex locked until deeper into the move 
							// messages method to avoid deadlocks with the node mutex.
							m_sendMutex->Release();
							if( !MoveMessagesToWakeUpQueue( msg->GetTargetNodeId() ) )
							{
								// The attempt failed, so the node is either not a sleeping node, or the move
								// failed for another reason.  We'll just retry sending the message.
								Log::Write( "Resending message (attempt %d)", msg->GetSendAttempts() );
							}
							m_sendMutex->Lock();
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
	
	Msg *msg = m_sendQueue.front();
	m_sendQueue.pop_front();
	delete msg;
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
	if( Node* node = GetNode(_targetNodeId) )
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
					Msg* msg = *it;
					if( _targetNodeId == msg->GetTargetNodeId() )
					{
						// This message is for the unresponsive node
						// We do not move any "Wake Up No More Information"
						// commands to the pending queue.
						if( !msg->IsWakeUpNoMoreInformationCommand() )
						{
							Log::Write( "Node not responding - moving message to Wake-Up queue: %s", (*it)->GetAsString().c_str() );
							wakeUp->QueueMsg( msg );
						}
						else
						{
							delete msg;
						}
						m_sendQueue.erase( it++ );
					}
					else
					{
						++it;
					}
				}

				m_sendMutex->Release();
				
				// Move completed successfully
				ReleaseNodes();
				return true;
			}
		}

		ReleaseNodes();
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
	if( Node* node = GetNode( _nodeId ) )
	{
		if( !node->IsListeningDevice() )
		{
			if( WakeUp* pWakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				pWakeUp->SetAwake( true );
			}
		}

		ReleaseNodes();
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
			case FUNC_ID_ZW_GET_VERSION:
			{
				HandleGetVersionResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:
			{
				HandleGetControllerCapabilitiesResponse( _data );
				break;
			}
			case FUNC_ID_SERIAL_API_GET_CAPABILITIES:
			{
				HandleGetSerialAPICapabilitiesResponse( _data );
				break;
			}
			case FUNC_ID_ZW_ENABLE_SUC:
			{
				HandleEnableSUCResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
			{
				HandleRequestNetworkUpdate( _data );
				break;
			}
			case FUNC_ID_ZW_CONTROLLER_CHANGE:
			{
				HandleControllerChange( _data );
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
			case FUNC_ID_ZW_MEMORY_GET_ID:
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
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				if( !HandleRemoveFailedNodeResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
				}
				break;
			}
			case FUNC_ID_ZW_IS_FAILED_NODE_ID:
			{
				HandleIsFailedNodeResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				if( !HandleReplaceFailedNodeResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
				}
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
			case FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE:
			{
				//if( m_controllerReplication )
				//{
				//	m_controllerReplication->SendNextData( m);
				//}
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
			case FUNC_ID_ZW_CREATE_NEW_PRIMARY:
			{
				HandleCreateNewPrimary( _data );
				break;
			}
			case FUNC_ID_ZW_CONTROLLER_CHANGE:
			{
				HandleControllerChange( _data );
				break;
			}
			case FUNC_ID_ZW_SET_LEARN_MODE:
			{
				HandleSetLearnMode( _data );
				break;
			}
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				HandleRemoveFailedNodeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				HandleReplaceFailedNodeRequest( _data );
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
		Log::Write("ProcessMsg: m_expectedCallbackId=%x _data[2]=%x m_expectedReply=%x _data[1]=%x m_expectedCommandClassId=%x _data[5]=%x", m_expectedCallbackId, _data[2], m_expectedReply, _data[1], m_expectedCommandClassId, _data[5]);
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
				if( _data[1] == FUNC_ID_ZW_SEND_DATA )
				{
					m_sendMutex->Lock();
					Msg *msg = m_sendQueue.front();
					uint8 n = msg->GetTargetNodeId();
					uint8 clsid = msg->GetExpectedCommandClassId();
					Node *node = m_nodes[n];
					if( node != NULL )
					{
						CommandClass *cc = node->GetCommandClass(clsid);
					}
					m_sendMutex->Release();
				}
				RemoveMsg();
			}
		}
	}

	UpdateEvents();
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetVersionResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetVersionResponse
(
	uint8* _data
)
{
	m_libraryVersion = (char*)&_data[2];
	
	uint8 m_libraryType = _data[m_libraryVersion.size()+3];
	if( m_libraryType < 9 )
	{
		m_libraryTypeName = c_libraryTypeNames[m_libraryType];
	}
	Log::Write( "Received reply to FUNC_ID_ZW_GET_VERSION:" );
	Log::Write( "    %s library, version %s", m_libraryTypeName.c_str(), m_libraryVersion.c_str() );
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetControllerCapabilitiesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetControllerCapabilitiesResponse
(
	uint8* _data
)
{
	m_controllerCaps = _data[2];

	Log::Write( "Received reply to FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:" );

	if( m_controllerCaps & ControllerCaps_SIS )
	{
		Log::Write( "    There is a SUC ID Server (SIS) in this network." );
		Log::Write( "    The PC controller is an inclusion controller" );

		if( m_controllerCaps & ControllerCaps_RealPrimary )
		{
			Log::Write( "    and was the primary before the SIS was added." );
		}
		else
		{
			Log::Write( "    and was a secondary before the SIS was added." );
		}
	}
	else
	{
		Log::Write( "    There is no SUC ID Server in the network." );
		if( m_controllerCaps & ControllerCaps_Secondary )
		{
			Log::Write( "    The PC controller is a secondary controller." );
		}
		else
		{
			Log::Write( "    The PC controller is a primary controller." );
		}
	}

	if( m_controllerCaps & ControllerCaps_SIS )
	{
		Log::Write( "    The PC controller is also a Static Update Controller." );
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetSerialAPICapabilitiesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetSerialAPICapabilitiesResponse
(
	uint8* _data
)
{
	Log::Write( "Received reply to FUNC_ID_SERIAL_API_GET_CAPABILITIES" );
	Log::Write( "    Application Version:  %d", _data[2] );
	Log::Write( "    Application Revision: %d", _data[3] );
	Log::Write( "    Manufacturer ID:      0x%.2x%.2x", _data[4], _data[5] );
	Log::Write( "    Product Type:         0x%.2x%.2x", _data[6], _data[7] );
	Log::Write( "    Product ID:           0x%.2x%.2x", _data[8], _data[9] );

	// _data[10] to _data[41] are a 256-bit bitmask with one bit set for 
	// each FUNC_ID_ method supported by the controller.
	// Bit 0 is FUNC_ID_ 1.  So FUNC_ID_SERIAL_API_GET_CAPABILITIES (0x07) will be bit 6 of the first byte.
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
// <Driver::HandleRequestNetworkUpdate>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleRequestNetworkUpdate
(
	uint8* _data
)
{
	Log::Write( "Received reply to Request Network Update." );
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
	Log::Write( "Received reply to FUNC_ID_ZW_MEMORY_GET_ID. Home ID = 0x%02x%02x%02x%02x.  Our node ID = %d", _data[2], _data[3], _data[4], _data[5], _data[6] );
	m_homeId = (((uint32)_data[2])<<24) | (((uint32)_data[3])<<16) | (((uint32)_data[4])<<8) | ((uint32)_data[5]);
	m_nodeId = _data[6];
	m_controllerReplication = static_cast<ControllerReplication*>(ControllerReplication::Create( m_homeId, m_nodeId ));
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

	if( !m_init )
	{
		// Mark the driver as ready (we have to do this first or 
		// all the code handling notifications will go awry).
		Manager::Get()->SetDriverReady( this );

		// Read the config file first, to get the last known state
		ReadConfig();
	}

	Log::Write( "Received reply to FUNC_ID_SERIAL_API_GET_INIT_DATA:" );
	m_initVersion = _data[2];
	m_initCaps = _data[3];

	if( _data[4] == NUM_NODE_BITFIELD_BYTES )
	{
		for( i=0; i<NUM_NODE_BITFIELD_BYTES; ++i)
		{
			for( int32 j=0; j<8; ++j )
			{
				uint8 nodeId = (i*8)+j+1;
				if( _data[i+5] & (0x01 << j) )
				{					
					if( Node* node = GetNode( nodeId ) )
					{
						Log::Write( "  Node %d - Known", nodeId );
						if( !m_init )
						{
							// The node was read in from the config, so we 
							// only need to get its current state
							node->RequestState( CommandClass::RequestFlag_Session | CommandClass::RequestFlag_Dynamic );
						}

						ReleaseNodes();
					}
					else
					{
						// This node is new
						Log::Write( "  Node %.3d - New", nodeId );

						// Create the node and request its info
						AddNodeInfoRequest( nodeId );		
					}
				}
				else
				{
					if( GetNode(nodeId) )
					{
						Log::Write( "  Node %.3d: Removed", nodeId );

						// This node no longer exists in the Z-Wave network
						Notification* notification = new Notification( Notification::Type_NodeRemoved );
						notification->SetHomeAndNodeIds( m_homeId, nodeId );
						QueueNotification( notification ); 

						delete m_nodes[nodeId];
						m_nodes[nodeId] = NULL;
						ReleaseNodes();
					}
				}
			}
		}
	}

	m_init = true;
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
	uint8 nodeId = GetNodeInfoRequest();

	if( nodeId == 0xff )
	{
		Log::Write("ERROR: Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO but there are no node Ids in the info queue");
		return;
	}

	Log::Write("Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO for node %d", nodeId );

	// Update the node with the protocol info
	if( Node* node = GetNode( nodeId ) )
	{
		node->UpdateProtocolInfo( &_data[2] );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleRemoveFailedNodeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleRemoveFailedNodeResponse
(
	uint8* _data
)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if( _data[2] )
	{
		// Failed
		Log::Write("Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
	}
	else
	{
		Log::Write("Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - command in progress" );
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	return res; 
}

//-----------------------------------------------------------------------------
// <Driver::HandleIsFailedNodeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleIsFailedNodeResponse
(
	uint8* _data
)
{
	Log::Write("Received reply to FUNC_ID_ZW_IS_FAILED_NODE_ID - node %d has %s", m_controllerCommandNode, _data[2] ? "failed" : "not failed" );
	if( m_controllerCallback )
	{
		m_controllerCallback( _data[2] ? ControllerState_NodeFailed : ControllerState_NodeOK, m_controllerCallbackContext );
	}

	m_controllerCommand = ControllerCommand_None;
}

//-----------------------------------------------------------------------------
// <Driver::HandleReplaceFailedNodeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleReplaceFailedNodeResponse
(
	uint8* _data
)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if( _data[2] )
	{
		// Command failed
		Log::Write("Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
	}
	else
	{
		Log::Write("Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - command in progress" );
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	return res;
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
			m_controllerAdded = false;
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Waiting, m_controllerCallbackContext );
			}
			break;
		}
		case ADD_NODE_STATUS_NODE_FOUND:
		{
			Log::Write( "ADD_NODE_STATUS_NODE_FOUND" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_InProgress, m_controllerCallbackContext );
			}
			break;
		}
		case ADD_NODE_STATUS_ADDING_SLAVE:
		{
			Log::Write( "ADD_NODE_STATUS_ADDING_SLAVE" );			
			Log::Write( "Adding node ID %d", _data[4] );
			m_controllerAdded = false;
			m_controllerCommandNode = _data[4];
			break;
		}
		case ADD_NODE_STATUS_ADDING_CONTROLLER:
		{
			Log::Write( "ADD_NODE_STATUS_ADDING_CONTROLLER");
			Log::Write( "Adding node ID %d", _data[4] );
			m_controllerAdded = true;
			m_controllerCommandNode = _data[4];
			break;
		}
		case ADD_NODE_STATUS_PROTOCOL_DONE:
		{
			Log::Write( "ADD_NODE_STATUS_PROTOCOL_DONE" );

			if( m_controllerAdded && m_controllerReplication)
			{
				// We added a controller, now is the time to replicate our data to it
				m_controllerReplication->StartReplication( m_controllerCommandNode );
			}
			else
			{
				// We added a device.
				// Get the controller out of add mode to avoid accidentally adding other devices.
				Msg* msg = new Msg( "Stop Add Node Mode", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
				msg->Append( ADD_NODE_STOP );
				SendMsg( msg );
			}
			break;
		}
		case ADD_NODE_STATUS_DONE:
		{
			Log::Write( "ADD_NODE_STATUS_DONE" );

			AddNodeInfoRequest( m_controllerCommandNode );

			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;
			break;
		}
		case ADD_NODE_STATUS_FAILED:
		{
			Log::Write( "ADD_NODE_STATUS_FAILED" );

			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;
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
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Waiting, m_controllerCallbackContext );
			}
			break;
		}
		case REMOVE_NODE_STATUS_NODE_FOUND:
		{
			Log::Write( "REMOVE_NODE_STATUS_NODE_FOUND" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_InProgress, m_controllerCallbackContext );
			}
			break;
		}
		case REMOVE_NODE_STATUS_REMOVING_SLAVE:
		{
			Log::Write( "REMOVE_NODE_STATUS_REMOVING_SLAVE" );
			m_controllerCommandNode = _data[4];
			break;
		}
		case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
		{
			Log::Write( "REMOVE_NODE_STATUS_REMOVING_CONTROLLER" );
			m_controllerCommandNode = _data[4];
			break;
		}
		case REMOVE_NODE_STATUS_DONE:
		{
			Log::Write( "REMOVE_NODE_STATUS_DONE" );
			
			Notification* notification = new Notification( Notification::Type_NodeRemoved );
			notification->SetHomeAndNodeIds( m_homeId, m_controllerCommandNode );
			QueueNotification( notification ); 
			
			LockNodes();
			delete m_nodes[m_controllerCommandNode];
			m_nodes[m_controllerCommandNode] = NULL;
			ReleaseNodes();

			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;
			break;
		}
		case REMOVE_NODE_STATUS_FAILED:
		{
			Log::Write( "REMOVE_NODE_STATUS_FAILED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;
			break;
		}
		default:
		{
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleControllerChange>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleControllerChange
(
	uint8* _data
)
{
	Log::Write( "FUNC_ID_ZW_CONTROLLER_CHANGE:" );
	
	switch( _data[3] ) 
	{
		case LEARN_MODE_STARTED:
		{
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleCreateNewPrimary>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleCreateNewPrimary
(
	uint8* _data
)
{
	Log::Write( "FUNC_ID_ZW_CREATE_NEW_PRIMARY:" );
	
	switch( _data[3] ) 
	{
		case LEARN_MODE_STARTED:
		{
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleSetLearnMode>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSetLearnMode
(
	uint8* _data
)
{
	Log::Write( "FUNC_ID_ZW_SET_LEARN_MODE:" );
	
	switch( _data[3] ) 
	{
		case LEARN_MODE_STARTED:
		{
			Log::Write( "LEARN_MODE_STARTED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Waiting, m_controllerCallbackContext );
			}
			break;
		}
		case LEARN_MODE_DONE:
		{
			Log::Write( "LEARN_MODE_DONE" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;

			// Stop learn mode
			Msg* msg = new Msg( "End Learn Mode", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
			msg->Append( 0 );
			SendMsg( msg );

			// Rebuild all the node info.  Group and scene data that we stored 
			// during replication will be applied as we discover each node.
			RefreshNodeInfo();
			break;
		}
		case LEARN_MODE_FAILED:
		{
			Log::Write( "LEARN_MODE_FAILED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;

			// Controller change failed
			Msg* msg = new Msg(  "Controller change failed", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true, false );
			msg->Append( CONTROLLER_CHANGE_STOP_FAILED );
			SendMsg( msg );

			// Rebuild all the node info, since it may have been partially
			// updated by the failed command.  Group and scene data that we
			// stored during replication will be applied as we discover each node.
			RefreshNodeInfo();
			break;
		}
		case LEARN_MODE_DELETED:
		{
			Log::Write( "LEARN_MODE_DELETED" );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleRemoveFailedNodeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleRemoveFailedNodeRequest
(
	uint8* _data
)
{
	ControllerState state = ControllerState_Completed;
	switch( _data[3] )
	{
		case FAILED_NODE_OK:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - Node %d is OK, so command failed", m_controllerCommandNode );
			state = ControllerState_NodeOK;
			break;
		}
		case FAILED_NODE_REMOVED:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - node %d successfully moved to failed nodes list", m_controllerCommandNode );
			state = ControllerState_Completed;
			m_controllerCommand = ControllerCommand_None;
			break;
		}
		case FAILED_NODE_NOT_REMOVED:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - unable to move node %d to failed nodes list", m_controllerCommandNode );
			state = ControllerState_Failed;
			m_controllerCommand = ControllerCommand_None;
			break;
		}
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	m_controllerCommand = ControllerCommand_None;
}

//-----------------------------------------------------------------------------
// <Driver::HandleReplaceFailedNodeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleReplaceFailedNodeRequest
(
	uint8* _data
)
{
	ControllerState state = ControllerState_Completed;
	switch( _data[3] )
	{
		case FAILED_NODE_OK:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Node %d is OK, so command failed", m_controllerCommandNode );
			state = ControllerState_NodeOK;
			m_controllerCommand = ControllerCommand_None;
			break;
		}
		case FAILED_NODE_REPLACE_WAITING:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Waiting for new node" );
			state = ControllerState_Waiting;
			break;
		}
		case FAILED_NODE_REPLACE_DONE:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - node %d successfully replaced", m_controllerCommandNode );
			state = ControllerState_Completed;
			m_controllerCommand = ControllerCommand_None;

			// Request new node info for this device
			AddNodeInfoRequest( m_controllerCommandNode );
			break;
		}
		case FAILED_NODE_REPLACE_FAILED:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - node %d replacement failed", m_controllerCommandNode );
			state = ControllerState_Failed;
			m_controllerCommand = ControllerCommand_None;
			break;
		}
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
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
#ifdef notdef
		case COMMAND_CLASS_BASIC:
		{			
			switch (ClassIdMinor)
			{
			   	case BASIC_SET:
				case BASIC_REPORT:
					//get the class function pointer
					if( pfnDeviceEventVectorClass = CommandClasses::CreateCommandClass( ClassId, m_homeId, nodeId ) )
					{
						StatusData[0] = ClassIdMinor; StatusData[1]=ValueFromDevice;
						pfnDeviceEventVectorClass->HandleMsg( StatusData,0x02,0x00 );

						//send notification to application
						Notification* notification = new Notification( Notification::Type_NodeStatus );
						notification->SetHomeAndNodeIds( GetHomeId(),nodeId );
						notification->SetStatus( StatusData[1] );
						Manager::Get()->QueueNotification( notification );
					}
					break;
			}
			break;
		}
#endif
		case COMMAND_CLASS_HAIL:
		{			
			if( pfnDeviceEventVectorClass = CommandClasses::CreateCommandClass(ClassId, m_homeId, nodeId ) )
			{
				StatusData[0] = 0x01; StatusData[1]=ValueFromDevice;
				pfnDeviceEventVectorClass->HandleMsg( StatusData,0x02,0x00);

				//send notification to application
				Notification* notification = new Notification(Notification::Type_NodeStatus);
				notification->SetHomeAndNodeIds(GetHomeId(),nodeId);
				notification->SetStatus(StatusData[1]);
				QueueNotification( notification );
			}
			break;
		}
		case COMMAND_CLASS_APPLICATION_STATUS:
		{
			break;	//TODO: Test this class function or implement
		}
		case COMMAND_CLASS_CONTROLLER_REPLICATION:
		{
			if( m_controllerReplication && ( ControllerCommand_ReceiveConfiguration == m_controllerCommand ) )
			{
				m_controllerReplication->HandleMsg( &_data[6], _data[4] );
				if( m_controllerCallback )
				{
					m_controllerCallback( ControllerState_InProgress, m_controllerCallbackContext );
				}		
			}
			else
			{

			}
			break;
		}
		default:
		{			
			// Allow the node to handle the message itself
			if( Node* node = GetNode( nodeId)  )
		 	{
				node->ApplicationCommandHandler( _data );
				ReleaseNodes();
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
			if( Node* node = GetNode( nodeId ) )
			{
				node->UpdateNodeInfo( &_data[8], _data[4] - 3 );
				ReleaseNodes();
			}
			break;
		}

		case UPDATE_STATE_NODE_INFO_REQ_FAILED:
		{
			Log::Write( "FUNC_ID_ZW_APPLICATION_UPDATE: UPDATE_STATE_NODE_INFO_REQ_FAILED received" );
			
			// In case the device does not report any optional command classes,
			// we mark the ones we do have so that their static data is requested.
			if( Node* node = GetNode( nodeId ) )
			{
				if( !node->NodeInfoReceived() )
				{
					node->SetStaticRequests();
					node->RequestEntireNodeState();
				}
			}

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
			
			// Request the node protocol info (also removes any existing node and creates a new one)
			AddNodeInfoRequest( nodeId );		
			break;
		}
		case UPDATE_STATE_DELETE_DONE:
		{
			Log::Write( "** Network change **: Z-Wave node %d was removed", nodeId );

			Notification* notification = new Notification( Notification::Type_NodeRemoved );
			notification->SetHomeAndNodeIds( m_homeId, nodeId );
			QueueNotification( notification ); 

			LockNodes();
			delete m_nodes[nodeId];
			m_nodes[nodeId] = NULL;
			ReleaseNodes();
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
				ReleaseNodes();
				return true;
			}
		}

		// Not in the list, so we add it
		m_pollList.push_back( _nodeId );
		m_pollMutex->Release();
		ReleaseNodes();
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
				ReleaseNodes();
				return true;
			}
		}

		// Not in the list
		m_pollMutex->Release();
		ReleaseNodes();
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

					ReleaseNodes();
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
// <Driver::RefreshNodeInfo>
// Delete all nodes and fetch new node data from the Z-Wave network
//-----------------------------------------------------------------------------
void Driver::RefreshNodeInfo
(
)
{
	// Delete all the node data
	LockNodes();
	for( int i=0; i<256; ++i )
	{
		if( m_nodes[i] )
		{
			delete m_nodes[i];
			m_nodes[i] = NULL;
		}
	}
	ReleaseNodes();

	// Notify the user that all node and value information has been deleted
	Notification* notification = new Notification( Notification::Type_DriverReset );
	notification->SetHomeAndNodeIds( m_homeId, 0 );
	QueueNotification( notification ); 

	// Fetch new node data from the Z-Wave network
	Log::Write( "RefreshNodeInfo" );
	Msg* msg = new Msg( "RefreshNodeInfo", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::AddNodeInfoRequest>
// Queue a node to be interrogated for its setup details
//-----------------------------------------------------------------------------
void Driver::AddNodeInfoRequest
( 
	uint8 const _nodeId
)
{
	// Delete any existing node and replace it with a new one
	LockNodes();
	if( m_nodes[_nodeId] )
	{
		// Remove the original node
		Notification* notification = new Notification( Notification::Type_NodeRemoved );
		notification->SetHomeAndNodeIds( m_homeId, _nodeId );
		QueueNotification( notification ); 
		delete m_nodes[_nodeId];
	}

	// Add the new node
	m_nodes[_nodeId] = new Node( m_homeId, _nodeId );
	ReleaseNodes();

	Notification* notification = new Notification( Notification::Type_NodeAdded );
	notification->SetHomeAndNodeIds( m_homeId, _nodeId );
	QueueNotification( notification ); 

	// Request the node info
	m_infoMutex->Lock();
	m_infoQueue.push_back( _nodeId );
	UpdateEvents();
	m_infoMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::RemoveNodeInfoRequest>
// Remove a node from the info queue
//-----------------------------------------------------------------------------
void Driver::RemoveNodeInfoRequest
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
// <Driver::GetNodeInfoRequest>
// Get the next node that needs its setup info requesting
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeInfoRequest
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
// <Driver::RequestNodeState>
// Request command class data
//-----------------------------------------------------------------------------
void Driver::RequestNodeState
(
	 uint8 const _nodeId,
	 uint32 const _flags
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->RequestState( _flags );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeListeningDevice>
// Get whether the node is a listening device that does not go to sleep
//-----------------------------------------------------------------------------
bool Driver::IsNodeListeningDevice
(
	 uint8 const _nodeId
)
{
	bool res = false;
	if( Node* node = GetNode( _nodeId ) )
	{
		res = node->IsListeningDevice();
		ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeRoutingDevice>
// Get whether the node is a routing device that passes messages to other nodes
//-----------------------------------------------------------------------------
bool Driver::IsNodeRoutingDevice
(
	 uint8 const _nodeId
)
{
	bool res = false;
	if( Node* node = GetNode( _nodeId ) )
	{
		res = node->IsRoutingDevice();
		ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeMaxBaudRate>
// Get the maximum baud rate of a node's communications
//-----------------------------------------------------------------------------
uint32 Driver::GetNodeMaxBaudRate
(
	 uint8 const _nodeId
)
{
	uint32 baud = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		baud = node->GetMaxBaudRate();
		ReleaseNodes();
	}

	return baud;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeVersion>
// Get the version number of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeVersion
(
	 uint8 const _nodeId
)
{
	uint8 version = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		version = node->GetVersion();
		ReleaseNodes();
	}

	return version;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeSecurity>
// Get the security byte for a node (bit meanings still to be determined)
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeSecurity
(
	 uint8 const _nodeId
)
{
	uint8 security = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		security = node->GetSecurity();
		ReleaseNodes();
	}

	return security;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeBasic>
// Get the basic type of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeBasic
(
	 uint8 const _nodeId
)
{
	uint8 basic = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		basic = node->GetBasic();
		ReleaseNodes();
	}

	return basic;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeGeneric>
// Get the generic type of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeGeneric
(
	 uint8 const _nodeId
)
{
	uint8 genericType = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		genericType = node->GetGeneric();
		ReleaseNodes();
	}

	return genericType;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeSpecific>
// Get the specific type of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeSpecific
(
	 uint8 const _nodeId
)
{
	uint8 specific = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		specific = node->GetSpecific();
		ReleaseNodes();
	}

	return specific;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeType>
// Get the basic/generic/specific type of the specified node
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeType
(
	uint8 const _nodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		string str = node->GetType();
		ReleaseNodes();
		return str;
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeManufacturerName>
// Get the manufacturer name for the node with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeManufacturerName
(
	uint8 const _nodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		string str = node->GetManufacturerName();
		ReleaseNodes();
		return str;
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeProductName>
// Get the product name for the node with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeProductName
(
	uint8 const _nodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		string str = node->GetProductName();
		ReleaseNodes();
		return str;
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeName>
// Get the user-editable name for the node with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeName
(
	uint8 const _nodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		string str = node->GetNodeName();
		ReleaseNodes();
		return str;
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeLocation>
// Get the user-editable string for location of the specified node
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeLocation
(
	uint8 const _nodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		string str = node->GetLocation();
		ReleaseNodes();
		return str;
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeManufacturerId>
// Get the manufacturer Id string value with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeManufacturerId
(
	uint8 const _nodeId
)
{
	if( Node* node = m_nodes[_nodeId] )
	{
		string str = node->GetManufacturerId();
		ReleaseNodes();
		return str;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeProductType>
// Get the product type string value with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeProductType
(
	uint8 const _nodeId
)
{
	if( Node* node = m_nodes[_nodeId] )
	{
		string str = node->GetProductType();
		ReleaseNodes();
		return str;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeProductId>
// Get the product Id string value with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeProductId
(
	uint8 const _nodeId
)
{
	if( Node* node = m_nodes[_nodeId] )
	{
		string str = node->GetProductId();
		ReleaseNodes();
		return str;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeManufacturerName>
// Set the manufacturer name for the node with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeManufacturerName
(
	uint8 const _nodeId,
	string const& _manufacturerName
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->SetManufacturerName( _manufacturerName );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeProductName>
// Set the product name string value with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeProductName
(
	uint8 const _nodeId,
	string const& _productName
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->SetProductName( _productName );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeName>
// Set the node name string value with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeName
(
	uint8 const _nodeId,
	string const& _nodeName
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->SetNodeName( _nodeName );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeLocation>
// Set the location string value with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeLocation
(
	uint8 const _nodeId,
	string const& _location
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->SetLocation( _location );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeLevel>
// Helper to set the node level through the basic command class
//-----------------------------------------------------------------------------
void Driver::SetNodeLevel
( 
	uint8 const _nodeId,
	uint8 const _level
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->SetLevel( _level );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::GetValue>
// Get a pointer to a Value object for the specified ValueID
//-----------------------------------------------------------------------------
Value* Driver::GetValue
(
	ValueID const& _id
)
{
	// This method is only called by code that has already locked the node
	if( Node* node = m_nodes[_id.GetNodeId()] )
	{
		return node->GetValue( _id );
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
	if( IsInclusionController() )
	{
		// We must call Request Network Update first, to ensure we have
		// an up-to-date routing table from the SIS.
	}
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
// <Driver::BeginControllerCommand>
// Start the controller performing one of its network management functions
//-----------------------------------------------------------------------------
bool Driver::BeginControllerCommand
( 
	ControllerCommand _command,
	pfnControllerCallback_t _callback,
	void* _context,
	bool _highPower,
	uint8 _nodeId
)
{
	if( ControllerCommand_None != m_controllerCommand )
	{
		// Already busy doing something else
		return false;
	}

	m_controllerCallback = _callback;
	m_controllerCallbackContext = _context;
	m_controllerCommand = _command;

	switch( m_controllerCommand )
	{
		case ControllerCommand_AddController:
		{
			Log::Write( "AddController" );
			Msg* msg = new Msg( "AddController", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( _highPower ? ADD_NODE_CONTROLLER | OPTION_HIGH_POWER : ADD_NODE_CONTROLLER );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_AddDevice:
		{
			Log::Write( "AddDevice" );
			Msg* msg = new Msg( "AddDevice", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( _highPower ? ADD_NODE_SLAVE | OPTION_HIGH_POWER : ADD_NODE_SLAVE );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_CreateNewPrimary:
		{
			break;
		}
		case ControllerCommand_ReceiveConfiguration:
		{
			Log::Write( "ReceiveConfiguration" );
			Msg* msg = new Msg( "ReceiveConfiguration", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
			msg->Append( 0xff );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_RemoveController:
		{
			Log::Write( "RemoveController" );
			Msg* msg = new Msg( "RemoveController", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( _highPower ? REMOVE_NODE_ANY | OPTION_HIGH_POWER : REMOVE_NODE_ANY );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_RemoveDevice:
		{
			Log::Write( "RemoveDevice" );
			Msg* msg = new Msg( "RemoveDevice", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( _highPower ? REMOVE_NODE_ANY | OPTION_HIGH_POWER : REMOVE_NODE_ANY );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_HasNodeFailed:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( "Requesting whether node %d has failed", _nodeId );
			Msg* msg = new Msg( "Has Node Failed?", 0xff, REQUEST, FUNC_ID_ZW_IS_FAILED_NODE_ID, false );		
			msg->Append( _nodeId );
			msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_MarkNodeAsFailed:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( "Marking node %d as having failed", _nodeId );
			Msg* msg = new Msg( "Mark Node As Failed", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_FAILED_NODE_ID, true );		
			msg->Append( _nodeId );
			msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_ReplaceFailedNode:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( "Replace Failed Node %d", _nodeId );
			Msg* msg = new Msg( "ReplaceFailedNode", 0xff, REQUEST, FUNC_ID_ZW_REPLACE_FAILED_NODE, true );
			msg->Append( _nodeId );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_TransferPrimaryRole:
		{
			Log::Write( "TransferPrimaryRole" );
			Msg* msg = new Msg(  "TransferPrimaryRole", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true, false );
			msg->Append( CONTROLLER_CHANGE_START );
			SendMsg( msg );
			break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Driver::CancelControllerCommand>
// Stop the current controller function
//-----------------------------------------------------------------------------
bool Driver::CancelControllerCommand
( 
)
{
	if( ControllerCommand_None == m_controllerCommand )
	{
		// Controller is not doing anything
		return false;
	}

	switch( m_controllerCommand )
	{
		case ControllerCommand_AddController:
		{
			Log::Write( "CancelAddController" );
			Msg* msg = new Msg( "CancelAddController", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( ADD_NODE_STOP );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_AddDevice:
		{
			Log::Write( "CancelAddDevice" );
			Msg* msg = new Msg( "CancelAddDevice", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( ADD_NODE_STOP );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_CreateNewPrimary:
		{
			break;
		}
		case ControllerCommand_ReceiveConfiguration:
		{
			Log::Write( "CancelReceiveConfiguration" );
			Msg* msg = new Msg( "CancelReceiveConfiguration", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
			msg->Append( 0 );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_RemoveController:
		{
			Log::Write( "CancelRemoveController" );
			Msg* msg = new Msg( "CancelRemoveController", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( REMOVE_NODE_STOP );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_RemoveDevice:
		{
			Log::Write( "CancelRemoveDevice" );
			Msg* msg = new Msg( "CancelRemoveDevice", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( REMOVE_NODE_STOP );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_MarkNodeAsFailed:
		case ControllerCommand_HasNodeFailed:
		case ControllerCommand_ReplaceFailedNode:
		{
			// Cannot cancel
			return false;
		}
		case ControllerCommand_TransferPrimaryRole:
		{
			break;
		}
	}

	m_controllerCommand = ControllerCommand_None;
	return true;
}

//-----------------------------------------------------------------------------
// <Driver::RequestNetworkUpdate>
// Request a network update from other controllers
//-----------------------------------------------------------------------------
void Driver::RequestNetworkUpdate
(
)
{
	Log::Write( "Request Network update" );
	Msg* msg = new Msg(  "Request Network Update", 0xff, REQUEST, FUNC_ID_ZW_REQUEST_NETWORK_UPDATE, false, false );
	msg->Append( 1 );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::SetConfigParam>
// Set the value of one of the configuration parameters of a device
//-----------------------------------------------------------------------------
bool Driver::SetConfigParam
(
	uint8 const _nodeId,
	uint8 const _param,
	int32 _value
)
{
	bool res = false;
	if( Node* node = GetNode( _nodeId ) )
	{
		res = node->SetConfigParam( _param, _value );
		ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::RequestConfigParam>
// Request the value of one of the configuration parameters of a device
//-----------------------------------------------------------------------------
void Driver::RequestConfigParam
(
	uint8 const _nodeId,
	uint8 const _param
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->RequestConfigParam( _param );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::GetNumGroups>
// Gets the number of association groups reported by this node
//-----------------------------------------------------------------------------
uint8 Driver::GetNumGroups
(
	uint8 const _nodeId
)
{
	uint8 numGroups = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		numGroups = node->GetNumGroups();
		ReleaseNodes();
	}

	return numGroups;
}

//-----------------------------------------------------------------------------
// <Driver::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Driver::GetAssociations
( 
	uint8 const _nodeId,
	uint8 const _groupIdx,
	uint8** o_associations
)
{
	uint32 numAssociations = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		numAssociations = node->GetAssociations( _groupIdx, o_associations );
		ReleaseNodes();
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <Driver::AddAssociation>
// Adds a node to an association group
//-----------------------------------------------------------------------------
void Driver::AddAssociation
(
	uint8 const _nodeId,
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->AddAssociation( _groupIdx, _targetNodeId );
		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::RemoveAssociation>
// Removes a node from an association group
//-----------------------------------------------------------------------------
void Driver::RemoveAssociation
(
	uint8 const _nodeId,
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
	if( Node* node = GetNode( _nodeId ) )
	{
		node->RemoveAssociation( _groupIdx, _targetNodeId );
		ReleaseNodes();
	}
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

//-----------------------------------------------------------------------------
// <Driver::QueueNotification>
// Add a notification to the queue to be sent at a later, safe time.
//-----------------------------------------------------------------------------
void Driver::QueueNotification
(
	Notification* _notification
)
{
	m_notifications.push_back( _notification );
}

//-----------------------------------------------------------------------------
// <Driver::NotifyWatchers>
// Notify any watching objects of a value change
//-----------------------------------------------------------------------------
void Driver::NotifyWatchers
(
)
{
	list<Notification*>::iterator nit = m_notifications.begin();
	while( nit != m_notifications.end() )
	{
		Notification* notification = m_notifications.front();
		m_notifications.pop_front();

		Manager::Get()->NotifyWatchers( notification );

		delete notification;
		nit = m_notifications.begin();
	}
}

