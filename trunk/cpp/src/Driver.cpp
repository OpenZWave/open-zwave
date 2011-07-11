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
#include "Options.h"
#include "Manager.h"
#include "Node.h"
#include "Msg.h"
#include "Notification.h"

#include "Event.h"
#include "Mutex.h"
#include "IController.h"
#include "SerialController.h"
#include "HidController.h"
#include "Thread.h"
#include "Log.h"

#include "CommandClasses.h"
#include "ApplicationStatus.h"
#include "ControllerReplication.h"
#include "WakeUp.h"
#include "SwitchAll.h"

#include "ValueID.h"
#include "Value.h"

#include <algorithm>

using namespace OpenZWave;

// Version numbering for saved configurations. Any change that will invalidate
// previously saved configurations must be accompanied by an increment to the
// version number, and a comment explaining the date of, and reason for, the change.
//
// 01: 12-31-2010 - Introduced config version numbering due to ValueID format change.
// 02: 01-12-2011 - Command class m_afterMark sense corrected, and attribute named to match.
//
uint32 const c_configVersion = 2;


static char const* c_libraryTypeNames[] = 
{
	"Unknown",					// library type 0
	"Static Controller",		// library type 1
	"Controller",       		// library type 2
	"Enhanced Slave",   		// library type 3
	"Slave",            		// library type 4
	"Installer",				// library type 5
	"Routing Slave",			// library type 6
	"Bridge Controller",    	// library type 7
	"Device Under Test"			// library type 8
};

//-----------------------------------------------------------------------------
// <Driver::Driver>
// Constructor
//-----------------------------------------------------------------------------
Driver::Driver
( 
	string const& _controllerPath,
    ControllerInterface const& _interface
):
	m_driverThread( new Thread( "driver" ) ),
	m_wakeEvent( new Event() ),	
	m_exitEvent( new Event() ), 
	m_exit( false ),
	m_init( false ),
	m_awakeNodesQueried( false ),
	m_allNodesQueried( false ),
	m_controllerPath( _controllerPath ),
	m_homeId( 0 ),
	m_controllerThread( new Thread( "serial" ) ),
	m_initCaps( 0 ),
	m_controllerCaps( 0 ),
	m_nodeMutex( new Mutex() ),
	m_controllerReplication( NULL ),
	m_sendMutex( new Mutex() ),
	m_waitingForAck( false ),
	m_expectedCallbackId( 0 ),
	m_expectedReply( 0 ),
	m_pollThread( new Thread( "poll" ) ),
	m_pollMutex( new Mutex() ),
	m_pollInterval( 30 ),                   // By default, every polled device is queried once every 30 seconds
	m_queryMutex( new Mutex() ),
	m_controllerState( ControllerState_Normal ),
	m_controllerCommand( ControllerCommand_None ),
	m_controllerCallback( NULL ),
	m_controllerCallbackContext( NULL ),
	m_controllerAdded( false ),
	m_controllerCommandNode( 0 )
{
	// Clear the nodes array
	memset( m_nodes, 0, sizeof(Node*) * 256 );
    
    switch (_interface)
    {
    case ControllerInterface_Serial:
        {
            m_controller = (IController*) new SerialController();
            break;
        }
    case ControllerInterface_Hid:
        {
            m_controller = (IController*) new HidController();
            break;
        }
    default:
        {
            m_controller = (IController*) new SerialController();
            break;
        }
    }
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
	m_wakeEvent->Set();
	m_pollThread->Stop();
	m_controllerThread->Stop();
	m_driverThread->Stop();

	delete m_controller;

	delete m_pollThread;
	delete m_controllerThread;
	delete m_driverThread;

	delete m_sendMutex;
	delete m_pollMutex;
	delete m_queryMutex;

	delete m_wakeEvent;
	m_wakeEvent = NULL;
	delete m_exitEvent;
	m_exitEvent = NULL;

	// Clear the send Queue
	while( !m_sendQueue.empty() )
	{
		Msg *msg = m_sendQueue.front();
		m_sendQueue.pop_front();
		delete msg;
	}

	// Clear the node data
	LockNodes();
	for( int i=0; i<256; ++i )
	{
		if( GetNodeUnsafe( i ) )
		{
			Notification* notification = new Notification( Notification::Type_NodeRemoved );
			notification->SetHomeAndNodeIds( m_homeId, i );
			QueueNotification( notification ); 

			delete m_nodes[i];
			m_nodes[i] = NULL;
		}
	}
	ReleaseNodes();

	NotifyWatchers();
	delete m_nodeMutex;
    
    // Unsure at what point this is safe to do?
    delete m_controllerReplication;
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
				// Check for exit
				if( m_exit )
				{
					return;
				}

				// Consume any available messages
				bool workDone = ReadMsg();

				// Send any pending notifications
				NotifyWatchers();

				int32 timeout = 5000;	// Set the timeout for replies to 5 seconds

				if( !( m_waitingForAck || m_expectedCallbackId || m_expectedReply ) )
				{
					// We are not waiting for an ACK, callback or a specific message
					// type, so we are free to send any queued messages.
					workDone |= WriteMsg();
					timeout = Event::Timeout_Infinite;
				}

				if( !workDone )
				{
					// We had nothing to do this frame, so wait for something to occur
					if( !m_wakeEvent->Wait( timeout ) )
					{
						// Timeout expired.  If we were waiting for a response to our message, we're done.
						if( m_waitingForAck || m_expectedCallbackId || m_expectedReply )
						{
							// Timed out waiting for a response
							if( !m_sendQueue.empty() )
							{
								m_sendMutex->Lock();
								Msg* msg = m_sendQueue.front();
								m_sendMutex->Release();

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

										uint8 targetNode = m_sendQueue.front()->GetTargetNodeId();
										if( Node* node = GetNodeUnsafe( targetNode ) )
										{
											if( !node->IsListeningDevice() )
											{
												// Node is a sleeping device.  If we're not waiting for a callback,
												// we'll just dump the message.  This deals with certain remote
												// controls that respond to the ZW_SEND but fail to then reply.
												if( m_expectedCallbackId )
												{
													// We assume the node is asleep
													MoveMessagesToWakeUpQueue( targetNode );
												}
												else
												{
													Log::Write( "ERROR: Dropping command, node did not reply");
													RemoveMsg();
												}
											}
											else
											{
												// Node is listening, so we'll just retry sending the message.
												Log::Write( "Resending message (attempt %d)", msg->GetSendAttempts() );
											}
										}
									}
								}
							}

							m_waitingForAck = 0;	
							m_expectedCallbackId = 0;
							m_expectedReply = 0;
							m_expectedCommandClassId = 0;
						}

					}
					else
					{
						// m_wakeEvent exited as a result of a "Set," not a timeout, so reset the event
						m_wakeEvent->Reset();
					}
				}
			}
		}

		++attempts;
		uint32 maxAttempts = 0;
		Options::Get()->GetOptionAsInt("DriverMaxAttempts", (int32 *)&maxAttempts);
		if (maxAttempts && attempts >= maxAttempts)
		{
			Manager::Get()->Manager::SetDriverReady(this, false);
			NotifyWatchers();
			break;
		}

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

	// Open the controller
	Log::Write( "  Opening controller %s", m_controllerPath.c_str() );

	if( !m_controller->Open( m_controllerPath ) )
	{
	 	Log::Write( "Failed to init the controller (attempt %d)", _attempts );
		return false;
	}

	// Controller opened successfully, so we need to start all the worker threads
	m_controllerThread->Start( Driver::ControllerThreadEntryPoint, this );
	m_pollThread->Start( Driver::PollThreadEntryPoint, this );

	// Send a NAK to the ZWave device
	uint8 nak = NAK;
	m_controller->Write( &nak, 1 );

    // Get/set ZWave controller information in its preferred initialization order
    list<Msg*>* const pMsgInitArray = m_controller->GetMsgInitializationSequence();
    for (list<Msg*>::iterator it = pMsgInitArray->begin(); it != pMsgInitArray->end(); it++)
    {
        SendMsg(*it);
    }

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
	string userPath;
	Options::Get()->GetOptionAsString( "UserPath", &userPath );
	
	snprintf( str, sizeof(str), "zwcfg_0x%08x.xml", m_homeId );
	string filename =  userPath + string(str);

	TiXmlDocument doc;
	if( !doc.LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		return false;
	}

	TiXmlElement const* driverElement = doc.RootElement();

	// Version
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "version", &intVal ) )
	{
		if( (uint32)intVal != c_configVersion )
		{
			Log::Write( "Driver::ReadConfig - %s is from an older version of OpenZWave and cannot be loaded.", filename.c_str() );
			return false;
		}
	}
	else
	{
		Log::Write( "Driver::ReadConfig - %s is from an older version of OpenZWave and cannot be loaded.", filename.c_str() );
		return false;
	}

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

	snprintf( str, sizeof(str), "%d", c_configVersion );
	driverElement->SetAttribute( "version", str );

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

	string userPath;
	Options::Get()->GetOptionAsString( "UserPath", &userPath );

	snprintf( str, sizeof(str), "zwcfg_0x%08x.xml", m_homeId );
	string filename =  userPath + string(str);

	doc.SaveFile( filename.c_str() );
}

//-----------------------------------------------------------------------------
//	Controller
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::GetNodeUnsafe>
// Returns a pointer to the requested node without locking.
// Only to be used by main thread code.
//-----------------------------------------------------------------------------
Node* Driver::GetNodeUnsafe
(
	uint8 _nodeId
)
{
	if( Node* node = m_nodes[_nodeId] )
	{
		return node;
	}
	return NULL;
}

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
			}
		}

		ReleaseNodes();
	}

	Log::Write( "Queuing command: %s", _msg->GetAsString().c_str() );
	m_sendMutex->Lock();
	if( IsControllerCommand( _msg->GetExpectedReply() ) )
		// give priority to controller commands by putting at front of queue (right behind the current message)
		m_sendQueue.push_front( _msg );
	else
		// everything else goes to back of queue
		m_sendQueue.push_back( _msg );
	m_sendMutex->Release();

	// if the controller isn't already in the middle of another "dialogue" indicate that there is something to do
	if( !m_waitingForAck && !m_expectedCallbackId && !m_expectedReply )
	{
		m_wakeEvent->Set();
	}
}

//-----------------------------------------------------------------------------
// <Driver::WriteMsg>
// Transmit a queued message to the Z-Wave controller
//-----------------------------------------------------------------------------
bool Driver::WriteMsg()
{
	bool dataWritten = false;

	if( !m_sendQueue.empty() )
	{
		// there are messages to send, so get the one at the front of the queue
		m_sendMutex->Lock();
		Msg* msg = m_sendQueue.front();
		m_sendMutex->Release();
		
		// only send new messages if 1) there is no controller command executing or 2) there is one
		// in process, but this is also a controller command
		uint8 msgcmd = msg->GetExpectedReply();
		if(( m_controllerCommand == ControllerCommand_None ) || IsControllerCommand( msgcmd ) )
		{
			m_expectedCallbackId = msg->GetCallbackId();
			m_expectedCommandClassId = msg->GetExpectedCommandClassId();
			m_expectedReply = msgcmd;
			m_waitingForAck = true;

			msg->SetSendAttempts( msg->GetSendAttempts() + 1 );

			Log::Write( "" );
			Log::Write( "Sending command (Callback ID=0x%.2x, Expected Reply=0x%.2x) - %s", msg->GetCallbackId(), msg->GetExpectedReply(), msg->GetAsString().c_str() );
			m_controller->Write( msg->GetBuffer(), msg->GetLength() );
			dataWritten = true;
		}
		else
			Log::Write("Not sending a queued message because controller is busy.  m_controllerCommand = %d",m_controllerCommand);
	}
	else
	{
		// Check for nodes requiring queries
		uint8 nodeId = GetCurrentNodeQuery();
		if( nodeId != 0xff )
		{
			if( Node* node = GetNodeUnsafe( nodeId ) )
			{
				node->AdvanceQueries();
				if( node->AllQueriesCompleted() )
				{
					RemoveNodeQuery( nodeId );
				}
			}
		}
	}

	return dataWritten;
}

//-----------------------------------------------------------------------------
// <Driver::RemoveMsg>
// Remove a message from the send queue
//-----------------------------------------------------------------------------
void Driver::RemoveMsg
( 
)
{
	// get current message from the queue
  	if ( m_sendQueue.size() == 0 )
		return;
	m_sendMutex->Lock();
	Msg *msg = m_sendQueue.front();

	// if this is the last message in the queue for this node and it's not a "retry"
	// then signal that this query stage is complete
	uint8 nodeId = msg->GetTargetNodeId();
	if( nodeId != 0xff  && nodeId != 0 )
	{
		// check to see if this is a "retry."  If so, don't signal query stage complete
		Node* node = GetNodeUnsafe( nodeId );
		if( node != NULL && !node->m_queryRetries )
		{
			// look for more messages for this node in the send queue (to see if query stage is complete)
			bool bMoreForThisNode = false;
			list<Msg*>::iterator it = m_sendQueue.begin();
			++it;
			while( it != m_sendQueue.end() )
			{
				Msg* msg = *it;
				if( msg->GetTargetNodeId() == nodeId )
				{
					bMoreForThisNode = true;
					break;
				}
				++it;
			}

			// if there are no more messages for this node in the queue, signal complete
			if( !bMoreForThisNode ) 
			{
				node->QueryStageComplete( node->m_queryStage );
			}
		}
	}

	m_sendQueue.pop_front();
	delete msg;

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

	m_wakeEvent->Set();

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
	if( Node* node = GetNodeUnsafe(_targetNodeId) )
	{
	  // Exclude controllers from battery check
	  if( !node->IsListeningDevice() && node->GetBasic() != 0x01 && node->GetBasic() != 0x02 )
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
						it = m_sendQueue.erase( it );
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
// <Driver::IsControllerCommand>
// Identify controller (as opposed to node) commands...especially blocking ones
//-----------------------------------------------------------------------------
bool Driver::IsControllerCommand
(
	const uint8 _command
)
{
	// ranges of commands are used to enhance performance
	// the commands identified as "Controller Commands" needs to be reviewed as we
	// understand the protocol better and implement handlers
	if( ( _command >= FUNC_ID_ZW_SET_DEFAULT ) && 
		( _command <= FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE ) )
			return true;
	if( ( _command >= FUNC_ID_ZW_ADD_NODE_TO_NETWORK ) &&
		( _command <= FUNC_ID_ZW_SET_LEARN_MODE ) )
			return true;
	if( ( _command >= FUNC_ID_ZW_REMOVE_FAILED_NODE_ID ) &&
		( _command <= FUNC_ID_ZW_REPLACE_FAILED_NODE ) )
			return true;

	return false;
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

    if( !m_controller->Read( buffer, 1, IController::ReadPacketSegment_FrameType ) )
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
				Log::Write( "Unsolicited message received while waiting for ACK." );
			}

			// Read the length byte.  Keep trying until we get it.
			uint8 loops = 0;
			while( true )
			{
                if( m_controller->Read( &buffer[1], 1, IController::ReadPacketSegment_FrameLength )) 
				{
					break;
				}
				
				m_driverThread->Sleep( 10 );
				if( ++loops == 10 )
				{
					break;
				}
			}
			if( loops == 10 )
			{
				Log::Write( "100ms passed without finding the length byte...aborting frame read");
				break;
			}

			// Read the rest of the frame
			loops = 0;
			uint32 bytesRemaining = buffer[1];
			do
			{
                bytesRemaining -= m_controller->Read( &buffer[2+(uint32)buffer[1]-bytesRemaining], bytesRemaining, IController::ReadPacketSegment_FrameData );
				if( bytesRemaining ) 
				{
					m_driverThread->Sleep( 10 );
					if( ++loops == 50 )
					{
						break;
					}
				}
			}
			while( bytesRemaining );
			
			if( loops == 50 )
			{
				Log::Write( "500ms passed without reading the rest of the frame...aborting frame read" );
				break;
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
			Log::Write( "  Received: %s", str.c_str() );

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
				m_controller->Write( &ack, 1 );

				// Process the received message
				ProcessMsg( &buffer[2] );
			}
			else
			{
				Log::Write( "Checksum incorrect - sending NAK" );
				uint8 nak = NAK;
				m_controller->Write( &nak, 1 );
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
			Log::Write( "  ACK received CallbackId 0x%.2x Reply 0x%.2x", m_expectedCallbackId, m_expectedReply );
			m_waitingForAck = false;
			
			if( ( 0 == m_expectedCallbackId ) && ( 0 == m_expectedReply ) )
			{
				// Remove the message from the queue, now that it has been acknowledged.
				RemoveMsg();
			}
			break;
		}
		
		default:
		{
			Log::Write( "ERROR! Out of frame flow! (0x%.2x).  Sending NAK.", buffer[0] );
			uint8 nak = NAK;
			m_controller->Write( &nak, 1 );
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
			case FUNC_ID_SERIAL_API_GET_INIT_DATA:
			{
				Log::Write( "" );
				HandleSerialAPIGetInitDataResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:
			{
				Log::Write( "" );
				HandleGetControllerCapabilitiesResponse( _data );
				break;
			}
			case FUNC_ID_SERIAL_API_GET_CAPABILITIES:
			{
				Log::Write( "" );
				HandleGetSerialAPICapabilitiesResponse( _data );
				break;
			}
			case FUNC_ID_ZW_SEND_DATA:
			{
				HandleSendDataResponse( _data, false );
				handleCallback = false;			// Skip the callback handling - a subsequent FUNC_ID_ZW_SEND_DATA request will deal with that
				break;
			}
			case FUNC_ID_ZW_GET_VERSION:
			{
				Log::Write( "" );
				HandleGetVersionResponse( _data );
				break;
			}
			case FUNC_ID_ZW_MEMORY_GET_ID:
			{
				Log::Write( "" );
				HandleMemoryGetIdResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO:
			{
				Log::Write( "" );
				HandleGetNodeProtocolInfoResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REPLICATION_SEND_DATA:
			{
				HandleSendDataResponse( _data, true );
				handleCallback = false;			// Skip the callback handling - a subsequent FUNC_ID_ZW_REPLICATION_SEND_DATA request will deal with that
				break;
			}
			case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
			{
				Log::Write( "" );
				if( !HandleAssignReturnRouteResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
			{
				Log::Write( "" );
				if( !HandleDeleteReturnRouteResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_ENABLE_SUC:
			{
				Log::Write( "" );
				HandleEnableSUCResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
			{
				Log::Write( "" );
				if( !HandleNetworkUpdateResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_SET_SUC_NODE_ID:
			{
				Log::Write( "" );
				HandleSetSUCNodeIdResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_SUC_NODE_ID:
			{
				Log::Write( "" );
				HandleGetSUCNodeIdResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_INFO:
			{
				Log::Write( "" );
				Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NODE_INFO" );
				break;
			}
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				Log::Write( "" );
				if( !HandleRemoveFailedNodeResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_IS_FAILED_NODE_ID:
			{
				Log::Write( "" );
				HandleIsFailedNodeResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				Log::Write( "" );
				if( !HandleReplaceFailedNodeResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_GET_ROUTING_INFO:
			{
				Log::Write( "" );
				HandleGetRoutingInfoResponse( _data );
				break;
			}
			case FUNC_ID_ZW_R_F_POWER_LEVEL_SET:
			{
				Log::Write( "" );
				HandleRfPowerLevelSetResponse( _data );
                break;
			}
			case FUNC_ID_ZW_READ_MEMORY:
			{
				Log::Write( "" );
				HandleReadMemoryResponse( _data );
                break;
			}
			case FUNC_ID_SERIAL_API_SET_TIMEOUTS:
			{
				Log::Write( "" );
                HandleSerialApiSetTimeoutsResponse( _data );
				break;
			}
			case FUNC_ID_MEMORY_GET_BYTE:
			{
				Log::Write( "" );
				HandleMemoryGetByteResponse( _data );
				break;
			}
			default:
			{
				Log::Write( "" );
				Log::Write( "**TODO: handle response for 0x%.2x**", _data[1] );
				break;
			}
		}
	} 
	else if( REQUEST == _data[0] )
	{
		switch( _data[1] )
		{
			case FUNC_ID_APPLICATION_COMMAND_HANDLER:
			{
				Log::Write( "" );
				HandleApplicationCommandHandlerRequest( _data );
				break;
			}
			case FUNC_ID_ZW_SEND_DATA:
			{
				handleCallback = !HandleSendDataRequest( _data, false );
				break;
			}
			case FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE:
			{
				if( m_controllerReplication )
				{
					Log::Write( "" );
					m_controllerReplication->SendNextData( m_controllerCommandNode );
				}
				break;
			}
			case FUNC_ID_ZW_REPLICATION_SEND_DATA:
			{
				handleCallback = !HandleSendDataRequest( _data, true );
				break;
			}
			case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
			{
				Log::Write( "" );
				HandleAssignReturnRouteRequest( _data );
				break;
			}
			case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
			{
				Log::Write( "" );
				HandleDeleteReturnRouteRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE:
			{
				Log::Write( "" );
				HandleNodeNeighborUpdateRequest( _data );
				break;
			}
			case FUNC_ID_ZW_APPLICATION_UPDATE:
			{
				Log::Write( "" );
				handleCallback = !HandleApplicationUpdateRequest( _data );
				break;
			}
			case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
			{
				Log::Write( "" );
				HandleAddNodeToNetworkRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
			{
				Log::Write( "" );
				HandleRemoveNodeFromNetworkRequest( _data );
				break;
			}
			case FUNC_ID_ZW_CREATE_NEW_PRIMARY:
			{
				Log::Write( "" );
				HandleCreateNewPrimaryRequest( _data );
				break;
			}
			case FUNC_ID_ZW_CONTROLLER_CHANGE:
			{
				Log::Write( "" );
				HandleControllerChangeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_SET_LEARN_MODE:
			{
				Log::Write( "" );
				HandleSetLearnModeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
			{
				Log::Write( "" );
				HandleNetworkUpdateRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				Log::Write( "" );
				HandleRemoveFailedNodeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				Log::Write( "" );
				HandleReplaceFailedNodeRequest( _data );
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
					Log::Write( "  Expected callbackId was received" );
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
							Log::Write( "  Expected reply and command class was received" );
							m_expectedReply = 0;
							m_expectedCommandClassId = 0;
						}
					}
					else
					{
						Log::Write( "  Expected reply was received" );
						m_expectedReply = 0;
					}
				}
			}

			if( !( m_expectedCallbackId || m_expectedReply ) )
			{
				Log::Write( "  Message transaction complete" );
				Log::Write( "" );
				RemoveMsg();

				bool notify = false;
				Options::Get()->GetOptionAsBool( "NotifyTransactions", &notify );
				if( notify )
				{
					Notification notification( Notification::Type_MsgComplete );
					notification.SetHomeAndNodeIds( m_homeId, 0xff );
					Manager::Get()->NotifyWatchers( &notification );
				}
			}
		}
	}
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

	char str[256];
	if( m_controllerCaps & ControllerCaps_SIS )
	{
		Log::Write( "    There is a SUC ID Server (SIS) in this network." );
		snprintf( str, 256, "    The PC controller is an inclusion %s%s%s", 
			( m_controllerCaps & ControllerCaps_SUC ) ? " static update controller (SUC)" : " controller",
			( m_controllerCaps & ControllerCaps_OnOtherNetwork ) ? " which is using a Home ID from another network" : "",
			( m_controllerCaps & ControllerCaps_RealPrimary ) ? " and was the original primary before the SIS was added." : "." );
		Log::Write( str );

	}
	else
	{
		Log::Write( "    There is no SUC ID Server (SIS) in this network." );
		snprintf( str, 256, "    The PC controller is a %s%s%s", 
			( m_controllerCaps & ControllerCaps_Secondary ) ? "secondary" : "primary",
			( m_controllerCaps & ControllerCaps_SUC ) ? " static update controller (SUC)" : " controller",
			( m_controllerCaps & ControllerCaps_OnOtherNetwork ) ? " which is using a Home ID from another network." : "." );
		Log::Write( str );
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
	m_manufacturerId = (((uint16)_data[4])<<8) | (uint16)_data[5];
	m_productType = (((uint16)_data[6])<<8) | (uint16)_data[7];
	m_productId = (((uint16)_data[8])<<8) | (uint16)_data[9];
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
// <Driver::HandleNetworkUpdateResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleNetworkUpdateResponse
(
	uint8* _data
)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if( _data[2] )
	{
		Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE - command in progress" );
	}
	else
	{
		// Failed
		Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	return res; 
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
		Log::Write( "  No SUC, so we become SUC" );
		
		Msg* msg;

		msg = new Msg( "Enable SUC", m_nodeId, REQUEST, FUNC_ID_ZW_ENABLE_SUC, false );
		msg->Append( 1 );	
//		msg->Append( SUC_FUNC_BASIC_SUC );			// SUC
		msg->Append( SUC_FUNC_NODEID_SERVER );		// SIS
		SendMsg( msg ); 

		msg = new Msg( "Set SUC node ID", m_nodeId, REQUEST, FUNC_ID_ZW_SET_SUC_NODE_ID, false );
		msg->Append( m_nodeId );
		msg->Append( 1 );								// TRUE, we want to be SUC/SIS
		msg->Append( 0 );								// no low power
		msg->Append( SUC_FUNC_NODEID_SERVER );
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
		Manager::Get()->SetDriverReady( this, true );

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
						node = node;
						Log::Write( "    Node %d - Known", nodeId );
						if( !m_init )
						{
							// The node was read in from the config, so we 
							// only need to get its current state
							AddNodeQuery( nodeId, Node::QueryStage_Associations );
						}

						ReleaseNodes();
					}
					else
					{
						// This node is new
						Log::Write( "    Node %.3d - New", nodeId );
						Notification* notification = new Notification( Notification::Type_NodeNew );
						notification->SetHomeAndNodeIds( m_homeId, nodeId );
						QueueNotification( notification ); 

						// Create the node and request its info
						InitNode( nodeId );		
					}
				}
				else
				{
					if( GetNode(nodeId) )
					{
						// This node no longer exists in the Z-Wave network
						Log::Write( "    Node %.3d: Removed", nodeId );
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
	uint8 nodeId = GetCurrentNodeQuery();

	if( nodeId == 0xff )
	{
		Log::Write("ERROR: Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO but there are no node Ids in the info queue");
		return;
	}

	Log::Write("Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO for node %d", nodeId );

	// Update the node with the protocol info
	if( Node* node = GetNodeUnsafe( nodeId ) )
	{
		node->UpdateProtocolInfo( &_data[2] );
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleAssignReturnRouteResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleAssignReturnRouteResponse
(
	uint8* _data
)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if( _data[2] )
	{
		Log::Write("Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE - command in progress" );
	}
	else
	{
		// Failed
		Log::Write("Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	return res; 
}

//-----------------------------------------------------------------------------
// <Driver::HandleDeleteReturnRouteResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleDeleteReturnRouteResponse
(
	uint8* _data
)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if( _data[2] )
	{
		Log::Write("Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE - command in progress" );
	}
	else
	{
		// Failed
		Log::Write("Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	return res; 
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
	uint8* _data,
	bool _replication
)
{
	if( _data[2] )
	{
		Log::Write( "  %s delivered to Z-Wave stack", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
	}
	else
	{
		Log::Write("ERROR: %s could not be delivered to Z-Wave stack", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetRoutingInfoResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetRoutingInfoResponse
(
	uint8* _data
)
{
	Log::Write("Received reply to FUNC_ID_ZW_GET_ROUTING_INFO" );

	if( Node* node = GetNode( m_controllerCommandNode ) )
	{
		// copy the 29-byte bitmap received (29*8=232 possible nodes) into this node's neighbors member variable
		memcpy( node->m_neighbors, &_data[2], 29 );
		ReleaseNodes();
		Log::Write( "    Neighbors of this node are:" );
		bool bNeighbors = false;
		for( int by=0; by<29; by++ )
		{
			for( int bi=0; bi<8; bi++ )
			{
				if( (_data[2+by] & (0x01<<bi)) )
				{
					Log::Write( "    Node %d", (by<<3)+bi+1 );
					bNeighbors = true;
				}
			}
		}
		if( !bNeighbors )
			Log::Write( "    (none reported)" );
	}


	if( m_controllerCallback )
	{
		m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
	}
	m_controllerCommand = ControllerCommand_None;
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendDataRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleSendDataRequest
(
	uint8* _data,
	bool _replication
)
{
	bool messageRemoved = false;

	Log::Write( "  %s Request with callback ID 0x%.2x received (expected 0x%.2x)", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA", _data[2], m_expectedCallbackId );

	if( _data[2] != m_expectedCallbackId )
	{
		// Wrong callback ID
		Log::Write( "ERROR: Callback ID is invalid" );
	}
	else 
	{
		// Callback ID matches our expectation
		if( _data[3] )
		{
			// Failed
			Log::Write( "Error: %s failed.", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
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
					if( !_replication )
					{
						// In case the failure is due to the target being a sleeping node, we 
						// first try to move its pending messages to its wake-up queue.
						if( MoveMessagesToWakeUpQueue( m_sendQueue.front()->GetTargetNodeId() ) )
						{
							messageRemoved = true;
						}
					}

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
		}
		else
		{
			// Command reception acknowledged by node
			if( m_expectedReply == 0 )
			{
				// We're not waiting for any particular reply, so we're done.
				Log::Write("%s was successful, removing command", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );	
				RemoveMsg();
				messageRemoved = true;
			}
			m_expectedCallbackId = 0;
		}
	}

	return messageRemoved;
}

//-----------------------------------------------------------------------------
// <Driver::HandleNetworkUpdateRequest>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleNetworkUpdateRequest
(
	uint8* _data
)
{
	ControllerState state = ControllerState_Failed;
	switch( _data[3] )
	{
		case SUC_UPDATE_DONE:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Success" );
			state = ControllerState_Completed;
			break;
		}
		case SUC_UPDATE_ABORT:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - Error. Process aborted." );
			break;
		}
		case SUC_UPDATE_WAIT:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - SUC is busy." );
			break;
		}
		case SUC_UPDATE_DISABLED:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - SUC is disabled." );
			break;
		}
		case SUC_UPDATE_OVERFLOW:
		{
			Log::Write("Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - Overflow. Full replication required." );
			break;
		}
		default:
		{
		}
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	m_controllerCommand = ControllerCommand_None;
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
	CommonAddNodeStatusRequestHandler( FUNC_ID_ZW_ADD_NODE_TO_NETWORK, _data );
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
			m_controllerCommandNode = 0;
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
			Log::Write( "Removing node ID %d", _data[4] );
			m_controllerCommandNode = _data[4];
			break;
		}
		case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
		{
			Log::Write( "REMOVE_NODE_STATUS_REMOVING_CONTROLLER" );
			m_controllerCommandNode = _data[4];
			if( m_controllerCommandNode == 0 ) // Some controllers don't return node number
			{
				if( _data[5] >= 3 )
				{
					for( int i=0; i<256; i++ )
					{
						if( m_nodes[i] == NULL )
						{
							continue;
						}
						// Ignore primary controller
						if( m_nodes[i]->m_nodeId == m_nodeId )
						{
							continue;
						}
						// See if we can match another way
						if( m_nodes[i]->m_basic == _data[6] &&
						    m_nodes[i]->m_generic == _data[7] &&
						    m_nodes[i]->m_specific == _data[8] )
						{
							if( m_controllerCommandNode != 0 )
							{
								Log::Write( "Alternative controller lookup found more then one match. Using the first one found.");
							}
							else
							{
								m_controllerCommandNode = m_nodes[i]->m_nodeId;
							}
						}
					}
				}
				else
				{
					Log::Write( "Node is 0 but not enough data to perform alternative match.");
				}
			}
			else
			{
				m_controllerCommandNode = _data[4];
			}
			Log::Write( "Removing controller ID %d", m_controllerCommandNode );
			break;
		}
		case REMOVE_NODE_STATUS_DONE:
		{
			Log::Write( "REMOVE_NODE_STATUS_DONE" );
			
			if ( m_controllerCommandNode == 0 ) // never received "removing" update
			{
				if ( _data[4] != 0 ) // but message has the clue
					m_controllerCommandNode = _data[4];
			}

			if ( m_controllerCommandNode != 0 )
			{
				Notification* notification = new Notification( Notification::Type_NodeRemoved );
				notification->SetHomeAndNodeIds( m_homeId, m_controllerCommandNode );
				QueueNotification( notification ); 
			
				LockNodes();
				delete m_nodes[m_controllerCommandNode];
				m_nodes[m_controllerCommandNode] = NULL;
				ReleaseNodes();
			}

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
// <Driver::HandleControllerChangeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleControllerChangeRequest
(
	uint8* _data
)
{
	Log::Write( "FUNC_ID_ZW_CONTROLLER_CHANGE:" );
	CommonAddNodeStatusRequestHandler( FUNC_ID_ZW_CONTROLLER_CHANGE, _data );
}

//-----------------------------------------------------------------------------
// <Driver::HandleCreateNewPrimaryRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleCreateNewPrimaryRequest
(
	uint8* _data
)
{
	Log::Write( "FUNC_ID_ZW_CREATE_NEW_PRIMARY:" );
	CommonAddNodeStatusRequestHandler( FUNC_ID_ZW_CREATE_NEW_PRIMARY, _data );
}

//-----------------------------------------------------------------------------
// <Driver::HandleSetLearnModeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSetLearnModeRequest
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
			InitAllNodes();
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
			InitAllNodes();
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
			InitNode( m_controllerCommandNode );
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
	uint8 classId = _data[5];

	if( ApplicationStatus::StaticGetCommandClassId() == classId )
	{
		//TODO: Test this class function or implement
	}
	else if( ControllerReplication::StaticGetCommandClassId() == classId )
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
	}
	else
	{
		// Allow the node to handle the message itself
		if( Node* node = GetNodeUnsafe( nodeId)  )
	 	{
			node->ApplicationCommandHandler( _data );
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleAssignReturnRouteRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleAssignReturnRouteRequest
(
	uint8* _data
)
{
	if( _data[3] )
	{
		// Failed
		Log::Write("Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE for node %d - FAILED", m_controllerCommandNode );
		if( m_controllerCallback )
		{
			m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
		}
	}
	else
	{
		// Success
		Log::Write("Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE for node %d - SUCCESS", m_controllerCommandNode );
		if( m_controllerCallback )
		{
			m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
		}
	}

	m_controllerCommand = ControllerCommand_None;
}

//-----------------------------------------------------------------------------
// <Driver::HandleDeleteReturnRouteRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleDeleteReturnRouteRequest
(
	uint8* _data
)
{
	if( _data[3] )
	{
		// Failed
		Log::Write("Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE for node %d - FAILED", m_controllerCommandNode );
		if( m_controllerCallback )
		{
			m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
		}
	}
	else
	{
		// Success
		Log::Write("Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE for node %d - SUCCESS", m_controllerCommandNode );
		if( m_controllerCallback )
		{
			m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
		}
	}

	m_controllerCommand = ControllerCommand_None;
}

//-----------------------------------------------------------------------------
// <Driver::HandleNodeNeighborUpdateRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleNodeNeighborUpdateRequest
(
	uint8* _data
)
{
	switch( _data[3] )
	{
		case REQUEST_NEIGHBOR_UPDATE_STARTED:
		{
			Log::Write( "REQUEST_NEIGHBOR_UPDATE_STARTED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_InProgress, m_controllerCallbackContext );
			}
			break;
		}
		case REQUEST_NEIGHBOR_UPDATE_DONE:
		{
			Log::Write( "REQUEST_NEIGHBOR_UPDATE_DONE" );

			// We now request the neighbour information from the
			// controller and store it in our node object.
			RequestNodeNeighbors( m_controllerCommandNode );
			break;
		}
		case REQUEST_NEIGHBOR_UPDATE_FAILED:
		{
			Log::Write( "REQUEST_NEIGHBOR_UPDATE_FAILED" );
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
		case UPDATE_STATE_SUC_ID:
		{
			Log::Write( "UPDATE_STATE_SUC_ID from node %d", nodeId );
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
		case UPDATE_STATE_NEW_ID_ASSIGNED:
		{
			Log::Write( "** Network change **: ID %d was assigned to a new Z-Wave node", nodeId );
			
			// Request the node protocol info (also removes any existing node and creates a new one)
			InitNode( nodeId );		
			break;
		}
		case UPDATE_STATE_ROUTING_PENDING:
		{
			Log::Write( "UPDATE_STATE_ROUTING_PENDING from node %d", nodeId );
			break;
		}
		case UPDATE_STATE_NODE_INFO_REQ_FAILED:
		{
			Log::Write( "FUNC_ID_ZW_APPLICATION_UPDATE: UPDATE_STATE_NODE_INFO_REQ_FAILED received" );
	
			// Note: Unhelpfully, the nodeId is always zero in this message.  However, we
			// only ever do this request from a node query, so we can use that instead.
			Node* node = GetNodeUnsafe( GetCurrentNodeQuery() );
			if( node )
			{
				// Retry the query up to three times
				node->QueryStageRetry( Node::QueryStage_NodeInfo, 3 );

				// Just in case the failure was due to the node being asleep, we try
				// to move its pending messages to its wakeup queue.  If it is not
				// a sleeping device, this will have no effect.
				if( MoveMessagesToWakeUpQueue( node->GetNodeId() ) )
				{
					messageRemoved = true;
				}
			}
			break;
		}
		case UPDATE_STATE_NODE_INFO_REQ_DONE:
		{
			Log::Write( "UPDATE_STATE_NODE_INFO_REQ_DONE from node %d", nodeId );
			break;
		}
		case UPDATE_STATE_NODE_INFO_RECEIVED:
		{
			Log::Write( "UPDATE_STATE_NODE_INFO_RECEIVED from node %d", nodeId );
			if( Node* node = GetNodeUnsafe( nodeId ) )
			{
				node->UpdateNodeInfo( &_data[8], _data[4] - 3 );
			}
			break;
		}
	}

	if( messageRemoved )
	{
		m_waitingForAck = 0;	
		m_expectedCallbackId = 0;
		m_expectedReply = 0;
		m_expectedCommandClassId = 0;
	}

	return messageRemoved;
}

//-----------------------------------------------------------------------------
// <Driver::CommonAddNodeStatusRequestHandler>
// Handle common AddNode processing for many similar commands
//-----------------------------------------------------------------------------
void Driver::CommonAddNodeStatusRequestHandler
(
	uint8 _funcId,
	uint8* _data
)
{
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
			Log::Write( "Adding controller ID %d", _data[4] );
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
				m_controllerReplication->StartReplication( m_controllerCommandNode, _funcId );
			}
			else
			{
				// We added a device.
				// Get the controller out of add mode to avoid accidentally adding other devices.
				Msg* msg = new Msg( "Add Node Mode Stop", 0xff, REQUEST, _funcId, true );
				msg->Append( ADD_NODE_STOP );
				SendMsg( msg );
			}
			break;
		}
		case ADD_NODE_STATUS_DONE:
		{
			Log::Write( "ADD_NODE_STATUS_DONE" );

			if( m_controllerCommandNode != 0xff )
				InitNode( m_controllerCommandNode );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;

			// If the added device was a controller, we should check whether to make it a SUC or SIS
			// TBD...
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

			// Remove the AddNode command from the queue
			RemoveMsg();

			// Get the controller out of add mode to avoid accidentally adding other devices.
			Msg* msg = new Msg( "Add Node Stop (Failed)", 0xff, REQUEST, _funcId, true );
			msg->Append( ADD_NODE_STOP_FAILED );
			SendMsg( msg );
			break;
		}
		default:
		{
			break;
		}
	}
}

//-----------------------------------------------------------------------------
//	Polling Z-Wave devices
//-----------------------------------------------------------------------------
	
//-----------------------------------------------------------------------------
// <Driver::EnablePoll>
// Enable polling of a value
//-----------------------------------------------------------------------------
bool Driver::EnablePoll
( 
	ValueID const _valueId
)
{
	// confirm that this node exists
	uint8 nodeId = _valueId.GetNodeId();
    Node* node = GetNode( nodeId );
	if( node != NULL )
	{
		// confirm that this value is in the node's value store
		if( Value* value = node->GetValue( _valueId ) )
		{
			value = value;
			// Add the valueid to the polling list
			m_pollMutex->Lock();

			// See if the node is already in the poll list.
			for( list<ValueID>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
			{
				if( *it == _valueId )
				{
					// It is already in the poll list, so we have nothing to do.
					m_pollMutex->Release();
					ReleaseNodes();
					return true;
				}
			}

			// Not in the list, so we add it
			m_pollList.push_back( _valueId );
			m_pollMutex->Release();
			ReleaseNodes();
			return true;
		}

		Log::Write( "EnablePoll failed - value not found for node %d", nodeId );
		return false;
	}

	Log::Write( "EnablePoll failed - node %d not found", nodeId );
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::DisablePoll>
// Disable polling of a node
//-----------------------------------------------------------------------------
bool Driver::DisablePoll
( 
	ValueID const _valueId
)
{
	uint8 nodeId = _valueId.GetNodeId();
	Node* node = GetNode( nodeId );
	if( node != NULL)
	{
		m_pollMutex->Lock();

		// See if the value is already in the poll list.
		for( list<ValueID>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
		{
			if( *it == _valueId )
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
		Log::Write( "DisablePoll failed - value not on list");
		return false;
	}

	Log::Write( "DisablePoll failed - node %d not found", nodeId );
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::isPolled>
// Check polling status of a value
//-----------------------------------------------------------------------------
bool Driver::isPolled
( 
	ValueID const _valueId
)
{
	uint8 nodeId = _valueId.GetNodeId();
	Node* node = GetNode( nodeId );
	if( node != NULL)
	{
		m_pollMutex->Lock();

		// See if the value is already in the poll list.
		for( list<ValueID>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
		{
			if( *it == _valueId )
			{
				// Found it
				m_pollMutex->Release();
				ReleaseNodes();
				return true;
			}
		}

		// Not in the list
		m_pollMutex->Release();
		ReleaseNodes();
		return false;
	}

	Log::Write( "isPolled failed - node %d not found", nodeId );
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
				ValueID valueId = m_pollList.front();
			
				// Move it to the back of the list
				m_pollList.pop_front();
				m_pollList.push_back( valueId );

				// Calculate the time before the next poll, so that all polls 
				// can take place within the user-specified interval.
				pollInterval /= m_pollList.size();

				// Request the state of the value from the node to which it belongs
				if( Node* node = GetNode( valueId.GetNodeId() ) )
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
						CommandClass* cc = node->GetCommandClass( valueId.GetCommandClassId() );
						uint8 index = valueId.GetIndex();
						uint8 instance = valueId.GetInstance();
						Log::Write( "Polling node %d: %s index = %d instance = %d (send queue has %d messages)", node->m_nodeId, cc->GetCommandClassName().c_str(), index, instance, m_sendQueue.size() );
						cc->RequestValue( index, instance );
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
//	Serial Port Thread - watching for data arriving
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::ControllerThreadEntryPoint>
// Entry point of the thread for watching the controller for new data
//-----------------------------------------------------------------------------
void Driver::ControllerThreadEntryPoint
( 
	void* _context 
)
{
	Driver* driver = (Driver*)_context;
	if( driver )
	{
		driver->ControllerThreadProc();
	}
}

//-----------------------------------------------------------------------------
// <Driver::ControllerThreadProc>
// Thread for watching the controller for new data
//-----------------------------------------------------------------------------
void Driver::ControllerThreadProc()
{
	while( 1 )
	{
		if( m_exit )
		{
			return;
		}

		// Wait for data to arrive at the controller
		if( m_controller->Wait( Event::Timeout_Infinite ) )
		{
			// New data has arrived, so wake the driver thread
			m_wakeEvent->Set();
		}
	}
}

//-----------------------------------------------------------------------------
//	Retrieving Node information
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::InitAllNodes>
// Delete all nodes and fetch new node data from the Z-Wave network
//-----------------------------------------------------------------------------
void Driver::InitAllNodes
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
	Msg* msg = new Msg( "InitAllNodes", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false );
	SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Driver::InitNode>
// Queue a node to be interrogated for its setup details
//-----------------------------------------------------------------------------
void Driver::InitNode
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
	// Do controller specific node initializations
	if( _nodeId == m_nodeId )
	{
		char str[64];

		snprintf( str, sizeof(str), "%.4x", m_manufacturerId );
		m_nodes[_nodeId]->SetManufacturerId( str );

		snprintf( str, sizeof(str), "%.4x", m_productType );
		m_nodes[_nodeId]->SetProductType( str );

		snprintf( str, sizeof(str), "%.4x", m_productId );
		m_nodes[_nodeId]->SetProductId( str );

		snprintf( str, sizeof(str), "Unknown: id=%.4x", m_manufacturerId );
		if( m_nodes[_nodeId]->GetManufacturerName() == "" )
		{
			m_nodes[_nodeId]->SetManufacturerName( str );
		}

		snprintf( str, sizeof(str), "Unknown: type=%.4x, id=%.4x", m_productType, m_productId );
		if( m_nodes[_nodeId]->GetProductName() == "" )
		{
			m_nodes[_nodeId]->SetProductName( str );
		}
	}
	ReleaseNodes();

	Notification* notification = new Notification( Notification::Type_NodeAdded );
	notification->SetHomeAndNodeIds( m_homeId, _nodeId );
	QueueNotification( notification ); 

	// Request the node info
	AddNodeQuery( _nodeId, Node::QueryStage_None );
}

//-----------------------------------------------------------------------------
// <Driver::GetCurrentNodeQuery>
// Get the awake node that is nearest the front of the list for queries
//-----------------------------------------------------------------------------
uint8 Driver::GetCurrentNodeQuery
( 
)
{
	uint8 nodeId = 0xff;
	bool sleepingNodes = false;

	m_queryMutex->Lock();			// make sure there are not changes to the query list while processing

	// search for the next query for an awake node
	for( list<uint8>::iterator it = m_nodeQueries.begin(); it != m_nodeQueries.end(); ++it )
	{
		if( Node* node = GetNodeUnsafe( *it ) )
		{
			// this node can sleep, so check to see if it is awake
			if( !node->IsListeningDevice() )
			{
				if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
				{
					if( !wakeUp->IsAwake() )
					{
						// Node is asleep
						sleepingNodes = true;
						continue;
					}
				}
			}

			// Found a node that is awake
			nodeId = *it;
			break;
		}
	}
	m_queryMutex->Release();

	// if this is the first (initialization) run of the queries, check to see if it has completed
	if( !m_allNodesQueried )
	{
		if( nodeId == 0xff )	// no node was found (either we're done or all remaining nodes to query are asleep)
		{
			if( sleepingNodes )
			{
				if (!m_awakeNodesQueried ) 
				{
					// only sleeping nodes remain, so signal awake nodes queried complete
					Log::Write( "Node query processing complete except for sleeping nodes." );
					Notification* notification = new Notification( Notification::Type_AwakeNodesQueried );
					notification->SetHomeAndNodeIds( m_homeId, nodeId );
					QueueNotification( notification ); 

					m_awakeNodesQueried = true;
				}
			}
			else
			{
				// no sleeping nodes, no more nodes in the queue, so...All done
				Log::Write( "Node query processing complete." );
				Notification* notification = new Notification( Notification::Type_AllNodesQueried );
				notification->SetHomeAndNodeIds( m_homeId, nodeId );
				QueueNotification( notification ); 

				m_awakeNodesQueried = true;
				m_allNodesQueried = true;
			}
		}
	}
	return nodeId;
}

//-----------------------------------------------------------------------------
// <Driver::AddNodeQuery>
// Add a node to the query queue
//-----------------------------------------------------------------------------
void Driver::AddNodeQuery
(
	uint8 const _nodeId,
	Node::QueryStage const _stage
)
{
	if( Node* node = m_nodes[_nodeId] )
	{
		m_queryMutex->Lock();

		// Add the node to the queue if is not already there
		bool found = false;
		for( list<uint8>::iterator it = m_nodeQueries.begin(); it != m_nodeQueries.end(); ++it )
		{
			if( _nodeId == *it )
			{
				found = true;
				break;
			}
		}
		if( !found )
		{
			m_nodeQueries.push_back( _nodeId );
		}

		// Set the query stage in the node
		node->GoBackToQueryStage( _stage );

		m_queryMutex->Release();
		m_wakeEvent->Set();
	}
}

//-----------------------------------------------------------------------------
// <Driver::RemoveNodeQuery>
// Remove a node to the query queue
//-----------------------------------------------------------------------------
void Driver::RemoveNodeQuery
(
	uint8 const _nodeId
)
{
	m_queryMutex->Lock();

	for( list<uint8>::iterator it = m_nodeQueries.begin(); it != m_nodeQueries.end(); ++it )
	{
		if( _nodeId == *it )
		{
			// Found
			m_nodeQueries.erase( it );
			break;
		}
	}

	m_queryMutex->Release();
}

//-----------------------------------------------------------------------------
// <Driver::RequestNodeState>
// Request command class data
//-----------------------------------------------------------------------------
void Driver::RequestNodeState
(
	 uint8 const _nodeId
)
{
	AddNodeQuery( _nodeId, Node::QueryStage_Associations );
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
// <Driver::GetNodeNeighbors>
// Gets the neighbors for a node
//-----------------------------------------------------------------------------
uint32 Driver::GetNodeNeighbors
( 
	uint8 const _nodeId,
	uint8** o_neighbors
)
{
	uint32 numNeighbors = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		numNeighbors = node->GetNeighbors(o_neighbors );
		ReleaseNodes();
	}

	return numNeighbors;
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
	if( Node* node = GetNode( _nodeId ) )
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
	if( Node* node = GetNode( _nodeId ) )
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
	if( Node* node = GetNode( _nodeId ) )
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
// <Driver::SetNodeOn>
// Helper to set the node on through the basic command class
//-----------------------------------------------------------------------------
void Driver::SetNodeOn
( 
    uint8 const _nodeId
)
{
    if( Node* node = GetNode( _nodeId ) )
    {
        node->SetNodeOn();
        ReleaseNodes();
    }
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeOff>
// Helper to set the node off through the basic command class
//-----------------------------------------------------------------------------
void Driver::SetNodeOff
( 
    uint8 const _nodeId
)
{
    if( Node* node = GetNode( _nodeId ) )
    {
        node->SetNodeOff();
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
// <Driver::RequestNodeNeighbors>
// Get the neighbour information for a node from the controller
//-----------------------------------------------------------------------------
void Driver::RequestNodeNeighbors
( 
	uint8 const _nodeId
)
{
	// Note: This is not the same as RequestNodeNeighbourUpdate.  This method
	// merely requests the controller's current neighbour information and
	// the reply will be copied into the relevant Node object for later use.
	m_controllerCommandNode = _nodeId;
	Log::Write( "Requesting routing info (neighbor list) for Node %d", _nodeId );
	Msg* msg = new Msg( "Get Routing Info", _nodeId, REQUEST, FUNC_ID_ZW_GET_ROUTING_INFO, false );
	msg->Append( _nodeId );
	msg->Append( 1 );		// Exclude bad links
	msg->Append( 1 );		// Exclude non-routing neighbors
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
			Log::Write( "CreateNewPrimary" );
			Msg* msg = new Msg( "CreateNewPrimary", 0xff, REQUEST, FUNC_ID_ZW_CREATE_NEW_PRIMARY, true );
			msg->Append( CREATE_PRIMARY_START );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_ReceiveConfiguration:
		{
			Log::Write( "ReceiveConfiguration" );
			Msg* msg = new Msg( "ReceiveConfiguration", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, true );
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
		case ControllerCommand_RemoveFailedNode:
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
			Msg* msg = new Msg( "TransferPrimaryRole", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true );
			msg->Append( CONTROLLER_CHANGE_START );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_RequestNetworkUpdate:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( "RequestNetworkUpdate" );
			Msg* msg = new Msg( "RequestNetworkUpdate", 0xff, REQUEST, FUNC_ID_ZW_REQUEST_NETWORK_UPDATE, true );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_RequestNodeNeighborUpdate:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( "Requesting Neighbor Update for node %d", _nodeId );
			Msg* msg = new Msg( "Requesting Neighbor Update", _nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE, true );
			msg->Append( _nodeId );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_AssignReturnRoute:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( "Assigning return route from node %d", _nodeId );
			Msg* msg = new Msg( "Assigning return route", _nodeId, REQUEST, FUNC_ID_ZW_ASSIGN_RETURN_ROUTE, true );
			msg->Append( _nodeId );		// from the node
			msg->Append( m_nodeId );	// to the controller
			SendMsg( msg );
			break;
		}
		case ControllerCommand_DeleteAllReturnRoutes:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( "Deleting all return routes from node %d", _nodeId );
			Msg* msg = new Msg( "Deleting return routes", _nodeId, REQUEST, FUNC_ID_ZW_DELETE_RETURN_ROUTE, true );
			msg->Append( _nodeId );		// from the node
			SendMsg( msg );
			break;
		}
        case ControllerCommand_None:
        {
            // To keep gcc quiet
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
			m_controllerCommandNode = 0xff;		// identify the fact that there is no new node to initialize
			Msg* msg = new Msg( "CancelAddController", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( ADD_NODE_STOP );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_AddDevice:
		{
			Log::Write( "CancelAddDevice" );
			m_controllerCommandNode = 0xff;		// identify the fact that there is no new node to initialize
			Msg* msg = new Msg( "CancelAddDevice", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( ADD_NODE_STOP );
			SendMsg( msg );
			break;
		}
		case ControllerCommand_CreateNewPrimary:
		{
			Log::Write( "CancelCreateNewPrimary" );
			Msg* msg = new Msg( "CancelCreateNewPrimary", 0xff, REQUEST, FUNC_ID_ZW_CREATE_NEW_PRIMARY, true );
			msg->Append( CREATE_PRIMARY_STOP );
			SendMsg( msg );
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
		case ControllerCommand_RemoveFailedNode:
		case ControllerCommand_HasNodeFailed:
		case ControllerCommand_ReplaceFailedNode:
		{
			// Cannot cancel
			return false;
		}
		case ControllerCommand_TransferPrimaryRole:
		{
			Log::Write( "CancelTransferPrimaryRole" );
			Msg* msg = new Msg( "CancelTransferPrimaryRole", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true );
			msg->Append( CONTROLLER_CHANGE_STOP );
			SendMsg( msg );
			break;
		}
		
		case ControllerCommand_None:
		case ControllerCommand_RequestNetworkUpdate:
		case ControllerCommand_RequestNodeNeighborUpdate:
		case ControllerCommand_AssignReturnRoute:
		case ControllerCommand_DeleteAllReturnRoutes:
		{
			// To keep gcc quiet
      break;
     }
	}

	m_controllerCommand = ControllerCommand_None;
	return true;
}

//-----------------------------------------------------------------------------
//	SwitchAll
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::SwitchAllOn>
// All devices that support the SwitchAll command class will be turned on
//-----------------------------------------------------------------------------
void Driver::SwitchAllOn
(
)
{
	SwitchAll::On( this, 0xff );

	LockNodes();
	for( int i=0; i<256; ++i )
	{
		if( GetNodeUnsafe( i ) )
		{
			if( m_nodes[i]->GetCommandClass( SwitchAll::StaticGetCommandClassId() ) )
			{
				SwitchAll::On( this, (uint8)i );
			}
		}
	}
	ReleaseNodes();
}

//-----------------------------------------------------------------------------
// <Driver::SwitchAllOff>
// All devices that support the SwitchAll command class will be turned off
//-----------------------------------------------------------------------------
void Driver::SwitchAllOff
(
)
{
	SwitchAll::Off( this, 0xff );

	LockNodes();
	for( int i=0; i<256; ++i )
	{
		if( GetNodeUnsafe( i ) )
		{
			if( m_nodes[i]->GetCommandClass( SwitchAll::StaticGetCommandClassId() ) )
			{
				SwitchAll::Off( this, (uint8)i );
			}
		}
	}
	ReleaseNodes();
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
// <Driver::GetMaxAssociations>
// Gets the maximum number of associations for a group
//-----------------------------------------------------------------------------
uint8 Driver::GetMaxAssociations
( 
	uint8 const _nodeId,
	uint8 const _groupIdx
)
{
	uint8 maxAssociations = 0;
	if( Node* node = GetNode( _nodeId ) )
	{
		maxAssociations = node->GetMaxAssociations( _groupIdx );
		ReleaseNodes();
	}

	return maxAssociations;
}

//-----------------------------------------------------------------------------
// <Driver::GetGroupLabel>
// Gets the label for a particular group
//-----------------------------------------------------------------------------
string Driver::GetGroupLabel
( 
	uint8 const _nodeId,
	uint8 const _groupIdx
)
{
	string label = "";
	if( Node* node = GetNode( _nodeId ) )
	{
		label = node->GetGroupLabel( _groupIdx );
		ReleaseNodes();
	}

	return label;
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
// <Driver::QueueNotification>
// Add a notification to the queue to be sent at a later, safe time.
//-----------------------------------------------------------------------------
void Driver::QueueNotification
(
	Notification* _notification
)
{
	m_notifications.push_back( _notification );
	if ( m_wakeEvent )
		m_wakeEvent->Set();
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

//-----------------------------------------------------------------------------
// <Driver::HandleRfPowerLevelSetResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleRfPowerLevelSetResponse
(
	uint8* _data
)
{
	bool res = true;
    // the meaning of this command is currently unclear, and there
    // isn't any returned response data, so just log the function call
	Log::Write("Received reply to FUNC_ID_ZW_R_F_POWER_LEVEL_SET");

	return res; 
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialApiSetTimeoutsResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleSerialApiSetTimeoutsResponse
(
	uint8* _data
)
{
    // the meaning of this command and its response is currently unclear
    bool res = true;
	Log::Write("Received reply to FUNC_ID_SERIAL_API_SET_TIMEOUTS");
    return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleMemoryGetByteResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleMemoryGetByteResponse
(
	uint8* _data
)
{
	bool res = true;
    // the meaning of this command and its response is currently unclear
    // it seems to return three bytes of data, so print them out
    Log::Write("Received reply to FUNC_ID_ZW_MEMORY_GET_BYTE, returned data: 0x%02hx 0x%02hx 0x%02hx",
               _data[0], _data[1], _data[2]);

	return res; 
}

//-----------------------------------------------------------------------------
// <Driver::HandleReadMemoryResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleReadMemoryResponse
(
	uint8* _data
)
{
    // the meaning of this command and its response is currently unclear
	bool res = true;
	Log::Write("Received reply to FUNC_ID_MEMORY_GET_BYTE");
	return res; 
}
