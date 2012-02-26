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
#include "SerialController.h"
#include "HidController.h"
#include "Thread.h"
#include "Log.h"
#include "TimeStamp.h"

#include "CommandClasses.h"
#include "ApplicationStatus.h"
#include "ControllerReplication.h"
#include "WakeUp.h"
#include "SwitchAll.h"
#include "ManufacturerSpecific.h"

#include "ValueID.h"
#include "Value.h"
#include "ValueStore.h"

#include <algorithm>

using namespace OpenZWave;

// Version numbering for saved configurations. Any change that will invalidate
// previously saved configurations must be accompanied by an increment to the
// version number, and a comment explaining the date of, and reason for, the change.
//
// 01: 12-31-2010 - Introduced config version numbering due to ValueID format change.
// 02: 01-12-2011 - Command class m_afterMark sense corrected, and attribute named to match.
// 03: 08-04-2011 - Changed command class instance handling for non-sequential MultiChannel endpoints.
//
uint32 const c_configVersion = 3;

static char const* c_libraryTypeNames[] = 
{
	"Unknown",			// library type 0
	"Static Controller",		// library type 1
	"Controller",       		// library type 2
	"Enhanced Slave",   		// library type 3
	"Slave",            		// library type 4
	"Installer",			// library type 5
	"Routing Slave",		// library type 6
	"Bridge Controller",    	// library type 7
	"Device Under Test"		// library type 8
};

static char const* c_transmitStatusNames[] =
{
	"Transmit OK",
	"No acknowledgement",
	"Network busy",
	"Routing not available",
	"No route"
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
	m_exit( false ),
	m_init( false ),
	m_awakeNodesQueried( false ),
	m_allNodesQueried( false ),
	m_notifytransactions( false ),
	m_controllerPath( _controllerPath ),
	m_controller( NULL ),
	m_homeId( 0 ),
	m_libraryVersion( "" ),
	m_libraryTypeName( "" ),
	m_libraryType( 0 ),
	m_manufacturerId( 0 ),
	m_productType( 0 ),
	m_productId ( 0 ),
	m_initVersion( 0 ),
	m_initCaps( 0 ),
	m_controllerCaps( 0 ),
	m_nodeMutex( new Mutex() ),
	m_controllerReplication( NULL ),
	m_sendMutex( new Mutex() ),
	m_currentMsg( NULL ),
	m_waitingForAck( false ),
	m_expectedCallbackId( 0 ),
	m_expectedReply( 0 ),
	m_expectedCommandClassId( 0 ),
	m_expectedNodeId( 0 ),
	m_pollThread( new Thread( "poll" ) ),
	m_pollMutex( new Mutex() ),
	m_pollInterval( 30000 ),						// By default, every polled device is attempted to be queried once every 30 seconds
	m_bIntervalBetweenPolls( false ),				// if set to true (via SetPollInterval), the pollInterval will be interspersed between each poll (so a much smaller m_pollInterval like 100, 500, or 1,000 may be appropriate)
	m_controllerState( ControllerState_Normal ),
	m_controllerCommand( ControllerCommand_None ),
	m_controllerCallback( NULL ),
	m_controllerCallbackContext( NULL ),
	m_controllerAdded( false ),
	m_controllerCommandNode( 0 ),
	m_controllerCommandArg( 0 ),
	m_virtualNeighborsReceived( false ),
	m_notificationsEvent( new Event() ),
	m_SOFCnt( 0 ),
	m_ACKWaiting( 0 ),
	m_readAborts( 0 ),
	m_badChecksum( 0 ),
	m_readCnt( 0 ),
	m_writeCnt( 0 ),
	m_CANCnt( 0 ),
	m_NAKCnt( 0 ),
	m_ACKCnt( 0 ),
	m_OOFCnt( 0 ),
	m_dropped( 0 ),
	m_retries( 0 ),
	m_controllerReadCnt( 0 ),
	m_controllerWriteCnt( 0 )
{
	// set a timestamp to indicate when this driver started
	TimeStamp m_startTime;

	// Create the message queue events
	for( int32 i=0; i<MsgQueue_Count; ++i )
	{
		m_queueEvent[i] = new Event();
	}

	// Clear the nodes array
	memset( m_nodes, 0, sizeof(Node*) * 256 );
    
	// Clear the virtual neighbors array
	memset( m_virtualNeighbors, 0, NUM_NODE_BITFIELD_BYTES );

	if( ControllerInterface_Hid == _interface )
	{
		m_controller = new HidController();
	}
	else
	{
		m_controller = new SerialController();
	}
	m_controller->SetSignalThreshold( 1 );

	Options::Get()->GetOptionAsBool( "NotifyTransactions", &m_notifytransactions );
	Options::Get()->GetOptionAsInt( "PollInterval", &m_pollInterval );
	Options::Get()->GetOptionAsBool( "IntervalBetweenPolls", &m_bIntervalBetweenPolls );
}

//-----------------------------------------------------------------------------
// <Driver::Driver>
// Destructor
//-----------------------------------------------------------------------------
Driver::~Driver
(
)
{
	// Save the driver config before deleting anything else
	bool save;
	if( Options::Get()->GetOptionAsBool( "SaveConfiguration", &save) )
	{
		if( save )
		{
			WriteConfig();
		}
	}

	// The order of the statements below has been achieved by mitigating freed memory
	//references using a memory allocator checker. Do not rearrange unless you are
	//certain memory won't be referenced out of order. --Greg Satz, April 2010
	m_exit = true;

	m_pollThread->Stop();
	m_pollThread->Release();

	m_driverThread->Stop();
	m_driverThread->Release();

	m_sendMutex->Release();
	m_pollMutex->Release();

	m_controller->Close();
	m_controller->Release();

	if( m_currentMsg != NULL )
	{
		RemoveCurrentMsg();
	}

	// Clear the send Queue
	for( int32 i=0; i<MsgQueue_Count; ++i )
	{
		while( !m_msgQueue[i].empty() )
		{
			MsgQueueItem const& item = m_msgQueue[i].front();
			if( MsgQueueCmd_SendMsg == item.m_command )
			{
				delete item.m_msg;
			}
			m_msgQueue[i].pop_front();
		}

		m_queueEvent[i]->Release();
	}

	// Clear the node data
	LockNodes();
	for( int i=0; i<256; ++i )
	{
		if( GetNodeUnsafe( i ) )
		{
			delete m_nodes[i];
			m_nodes[i] = NULL;
			Notification* notification = new Notification( Notification::Type_NodeRemoved );
			notification->SetHomeAndNodeIds( m_homeId, i );
			QueueNotification( notification ); 
		}
	}
	ReleaseNodes();

	NotifyWatchers();
	m_notificationsEvent->Release();
	m_nodeMutex->Release();
    
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
	Event* _exitEvent,
	void* _context 
)
{
	Driver* driver = (Driver*)_context;
	if( driver )
	{
		driver->DriverThreadProc( _exitEvent );
	}
}

//-----------------------------------------------------------------------------
// <Driver::DriverThreadProc>
// Create and manage the worker threads
//-----------------------------------------------------------------------------
void Driver::DriverThreadProc
( 
	Event* _exitEvent
)
{
	uint32 attempts = 0;
	while( true )
	{
		if( Init( attempts ) )
		{
			// Driver has been initialised
			Wait* waitObjects[8];
			waitObjects[0] = _exitEvent;						// Thread must exit.
			waitObjects[1] = m_notificationsEvent;				// Notifications waiting to be sent.
			waitObjects[2] = m_controller;						// Controller has received data.
			waitObjects[3] = m_queueEvent[MsgQueue_Command];	// A controller command is in progress.
			waitObjects[4] = m_queueEvent[MsgQueue_WakeUp];		// A node has woken. Pending messages should be sent.
			waitObjects[5] = m_queueEvent[MsgQueue_Send];		// Ordinary requests to be sent.
			waitObjects[6] = m_queueEvent[MsgQueue_Query];		// Node queries are pending.
			waitObjects[7] = m_queueEvent[MsgQueue_Poll];		// Poll request is waiting.

			TimeStamp retryTimeStamp;

			while( true )
			{
				Log::Write( LogLevel_Debug, "Top of DriverThreadProc loop." );
				uint32 count = 8;
				int32 timeout = Wait::Timeout_Infinite;

				// If we're waiting for a message to complete, we can only
				// handle incoming data, notifications and exit events.
				if( m_waitingForAck || m_expectedCallbackId || m_expectedReply )
				{
					count = 3;
					timeout = retryTimeStamp.TimeRemaining();
					if( timeout < 0 )
					{
						timeout = 0;
					}
				}
				else
					Log::QueueClear();							// clear the log queue when starting a new message

				// Wait for something to do
				int32 res = Wait::Multiple( waitObjects, count, timeout );
				switch( res )
				{
					case -1:	
					{
						// Wait has timed out - time to resend
						if( WriteMsg( "Wait Timeout" ) )
						{
							retryTimeStamp.SetTime( RETRY_TIMEOUT );
						}
						break;
					}
					case 0:
					{
						// Exit has been signalled
						return;
					}
					case 1:
					{
						// Notifications are waiting to be sent
						NotifyWatchers();			
						break;
					}
					case 2:
					{
						// Data has been received
						ReadMsg();
						break;
					}
					default:
					{
						// All the other events are sending message queue items
						if( WriteNextMsg( (MsgQueue)(res-3) ) )
						{
							retryTimeStamp.SetTime( RETRY_TIMEOUT );
						}
						break;
					}
				}
			}
		}

		++attempts;
		
		uint32 maxAttempts = 0;
		Options::Get()->GetOptionAsInt("DriverMaxAttempts", (int32 *)&maxAttempts);
		if( maxAttempts && (attempts >= maxAttempts) )
		{
			Manager::Get()->Manager::SetDriverReady(this, false);
			NotifyWatchers();
			break;
		}
			
		if( attempts < 25 )
		{
			// Retry every 5 seconds for the first two minutes
			if( Wait::Single( _exitEvent, 5000 ) == 0 )
			{
				// Exit signalled.
				return;
			}
		}
		else
		{
			// Retry every 30 seconds after that
			if( Wait::Single( _exitEvent, 30000 ) == 0 )
			{
				// Exit signalled.
				return;
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
	Log::Write( LogLevel_Info, "  Opening controller %s", m_controllerPath.c_str() );

	if( !m_controller->Open( m_controllerPath ) )
	{
	 	Log::Write( LogLevel_Warning, "WARNING: Failed to init the controller (attempt %d)", _attempts );
		return false;
	}

	// Controller opened successfully, so we need to start all the worker threads
	m_pollThread->Start( Driver::PollThreadEntryPoint, this );

	// Send a NAK to the ZWave device
	uint8 nak = NAK;
	m_controller->Write( &nak, 1 );
 
	// Get/set ZWave controller information in its preferred initialization order
	m_controller->PlayInitSequence( this );

	//If we ever want promiscuous mode uncomment this code.
	//Msg* msg = new Msg( "FUNC_ID_ZW_SET_PROMISCUOUS_MODE", 0xff, REQUEST, FUNC_ID_ZW_SET_PROMISCUOUS_MODE, false, false );
	//msg->Append( 0xff );
	//SendMsg( msg );
	
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
	if( TIXML_SUCCESS != driverElement->QueryIntAttribute( "version", &intVal ) || (uint32)intVal != c_configVersion )
	{
		Log::Write( LogLevel_Warning, "WARNING: Driver::ReadConfig - %s is from an older version of OpenZWave and cannot be loaded.", filename.c_str() );
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
			Log::Write( LogLevel_Warning, "WARNING: Driver::ReadConfig - Home ID in file %s is incorrect", filename.c_str() );
			return false;
		}
	}
	else
	{
		Log::Write( LogLevel_Warning, "WARNING: Driver::ReadConfig - Home ID is missing from file %s", filename.c_str() );
		return false;
	}

	// Node ID
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "node_id", &intVal ) )
	{
		if( (uint8)intVal != m_nodeId )
		{
			Log::Write( LogLevel_Warning, "WARNING: Driver::ReadConfig - Controller Node ID in file %s is incorrect", filename.c_str() );
			return false;
		}
	}
	else
	{
		Log::Write( LogLevel_Warning, "WARNING: Driver::ReadConfig - Node ID is missing from file %s", filename.c_str() );
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

	// Poll Interval--between polls or period for polling the entire pollList?
	if( TIXML_SUCCESS == driverElement->QueryIntAttribute( "poll_interval_between", &intVal ) )
	{
		m_bIntervalBetweenPolls = ( intVal != 0 );
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

	// restore the previous state (for now, polling) for the nodes/values just retrieved
	for( int i=0; i<256; i++ )
	{
		if( m_nodes[i] != NULL )
		{
			ValueStore* vs = m_nodes[i]->m_values;
			for( ValueStore::Iterator it = vs->Begin(); it != vs->End(); ++it )
			{
				Value* value = it->second;
				if( value->m_pollIntensity != 0 )
					EnablePoll( value->GetID(), value->m_pollIntensity );
			}
		}
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

	if (!m_homeId) {
		Log::Write( LogLevel_Warning, "WARNING: Tried to write driver config with no home ID set");
		return;
	}
	
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

	snprintf( str, sizeof(str), "%d", (int) m_bIntervalBetweenPolls );
	driverElement->SetAttribute( "poll_interval_between", str );

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
	m_nodeMutex->Unlock();
}

//-----------------------------------------------------------------------------
//	Sending Z-Wave messages
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::SendQueryStageComplete>
// Queue an item on the query queue that indicates a stage is complete
//-----------------------------------------------------------------------------
void Driver::SendQueryStageComplete
( 
	uint8 const _nodeId,
	Node::QueryStage const _stage,
	MsgQueue const _queue 
)
{
	MsgQueueItem item;
	item.m_command = MsgQueueCmd_QueryStageComplete;
	item.m_nodeId = _nodeId;
	item.m_queryStage = _stage;

	if( Node* node = GetNode( _nodeId ) )
	{
		if( !node->IsListeningDevice() )
		{
			if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				if( !wakeUp->IsAwake() )
				{
					// If the message is for a sleeping node, we queue it in the node itself.
					Log::Write( LogLevel_Info, "" );
					Log::Write( LogLevel_Detail, "%s, Queuing Wake-Up Command: Query Stage Complete (%s)", GetNodeString( node->GetNodeId() ).c_str(), node->GetQueryStageName( _stage ).c_str() );
					wakeUp->QueueMsg( item );
					ReleaseNodes();
					return;
				}
			}
		}

		// Non-sleeping node 
		Log::Write( LogLevel_Detail, "%s, Queuing Command: Query Stage Complete (%s)", GetNodeString( node->GetNodeId() ).c_str(), node->GetQueryStageName( _stage ).c_str() );
		m_sendMutex->Lock();
		m_msgQueue[MsgQueue_Query].push_back( item );
		m_queueEvent[MsgQueue_Query]->Set();
		m_sendMutex->Unlock();

		ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Driver::SendMsg>
// Queue a message to be sent to the Z-Wave PC Interface
//-----------------------------------------------------------------------------
void Driver::SendMsg
( 
	Msg* _msg,
	MsgQueue const _queue 
)
{
	_msg->Finalize();

	MsgQueueItem item;
	item.m_command = MsgQueueCmd_SendMsg;
	item.m_msg = _msg;

	// If the message is for a sleeping node, we queue it in the node itself.
	if( Node* node = GetNode(_msg->GetTargetNodeId()) )
	{
		if( !node->IsListeningDevice() )
		{
			if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				if( !wakeUp->IsAwake() )
				{
					Log::Write( LogLevel_Detail, "" );
					Log::Write( LogLevel_Detail, "%s, Queuing Wake-Up Command: %s", GetNodeString( _msg ).c_str(), _msg->GetAsString().c_str() );
					wakeUp->QueueMsg( item );
					ReleaseNodes();
					return;
				}
			}
		}

		ReleaseNodes();
	}

	Log::Write( LogLevel_Detail, "%s, Queuing command: %s", GetNodeString( _msg ).c_str(), _msg->GetAsString().c_str() );
	m_sendMutex->Lock();
	m_msgQueue[_queue].push_back( item );
	m_queueEvent[_queue]->Set();
	m_sendMutex->Unlock();
}

//-----------------------------------------------------------------------------
// <Driver::WriteNextMsg>
// Transmit a queued message to the Z-Wave controller
//-----------------------------------------------------------------------------
bool Driver::WriteNextMsg
(
	MsgQueue const _queue 
)
{
	// There are messages to send, so get the one at the front of the queue
	m_sendMutex->Lock();
	MsgQueueItem item = m_msgQueue[_queue].front();
	
	if( MsgQueueCmd_SendMsg == item.m_command )
	{
		// Send a message 
		m_currentMsg = item.m_msg;
		m_msgQueue[_queue].pop_front();
		if( m_msgQueue[_queue].empty() )
		{
			m_queueEvent[_queue]->Reset();
		}
		m_sendMutex->Unlock();
		return WriteMsg( "WriteNextMsg" );
	}
	
	if( MsgQueueCmd_QueryStageComplete == item.m_command )
	{
		// Move to the next query stage
		m_currentMsg = NULL;
		Node::QueryStage stage = item.m_queryStage;
		m_msgQueue[_queue].pop_front();
		if( m_msgQueue[_queue].empty() )
		{
			m_queueEvent[_queue]->Reset();
		}
		m_sendMutex->Unlock();

		Node* node = GetNodeUnsafe( item.m_nodeId );
		if( node != NULL )
		{	
			Log::Write( LogLevel_Detail, "%s, Query Stage Complete (%s)", GetNodeString( node->GetNodeId() ).c_str(), node->GetQueryStageName( stage ).c_str() );
			node->QueryStageComplete( stage );
			node->AdvanceQueries();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Driver::WriteMsg>
// Transmit the current message to the Z-Wave controller
//-----------------------------------------------------------------------------
bool Driver::WriteMsg
(
	string const msg
)
{
	Log::Write( LogLevel_Detail, "WriteMsg %s m_currentMsg=%08x", msg.c_str(), m_currentMsg );
	if( !m_currentMsg )
	{
		return false;
	}

	uint8 attempts = m_currentMsg->GetSendAttempts();
	uint8 nodeId = m_currentMsg->GetTargetNodeId();
	if( attempts >= m_currentMsg->GetMaxSendAttempts() )
	{
		// That's it - already tried to send GetMaxSendAttempt() times.
		Log::Write( LogLevel_Error, "%s, ERROR: Dropping command, expected response not received after %d attempt(s)", GetNodeString( nodeId ).c_str(), m_currentMsg->GetMaxSendAttempts() );
		delete m_currentMsg;
		m_currentMsg = NULL;

		m_expectedCallbackId = 0;
		m_expectedCommandClassId = 0;
		m_expectedNodeId = 0;
		m_expectedReply = 0;
		m_waitingForAck = false;
		return false;
	}

	m_currentMsg->SetSendAttempts( ++attempts );
	m_expectedCallbackId = m_currentMsg->GetCallbackId();
	m_expectedCommandClassId = m_currentMsg->GetExpectedCommandClassId();
	m_expectedNodeId = m_currentMsg->GetTargetNodeId();
	m_expectedReply = m_currentMsg->GetExpectedReply();
	m_waitingForAck = true;
	string attemptsstr = "";
	if( attempts > 1 )
	{
		char buf[15];
		snprintf( buf, sizeof(buf), "Attempt %d, ", attempts );
		attemptsstr = buf;
	}

	Log::Write( LogLevel_Detail, "" );
	Log::Write( LogLevel_Info, "%s, Sending command (%sCallback ID=0x%.2x, Expected Reply=0x%.2x) - %s", GetNodeString( nodeId ).c_str(), attemptsstr.c_str(), m_currentMsg->GetCallbackId(), m_currentMsg->GetExpectedReply(), m_currentMsg->GetAsString().c_str() );

	m_controller->Write( m_currentMsg->GetBuffer(), m_currentMsg->GetLength() );
	m_writeCnt++;
	
	if( nodeId == 0xff )
	{
		m_controllerWriteCnt++;
	}
	else
	{
		Node* node = GetNodeUnsafe( nodeId );
		if( node != NULL )
		{
			node->m_writeCnt++;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Driver::RemoveCurrentMsg>
// Delete the current message
//-----------------------------------------------------------------------------
void Driver::RemoveCurrentMsg
(
)
{
	delete m_currentMsg;
	m_currentMsg = NULL;

	m_expectedCallbackId = 0;
	m_expectedCommandClassId = 0;
	m_expectedNodeId = 0;
	m_expectedReply = 0;
	m_waitingForAck = false;
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
		if( !node->IsListeningDevice() && !node->IsFrequentListeningDevice() && !node->IsController() )
		{
			if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				// Mark the node as asleep
				wakeUp->SetAwake( false );

				// Move all messages for this node to the wake-up queue									
				m_sendMutex->Lock();

				// Try the current message first
				if( m_currentMsg )
				{
					if( _targetNodeId == m_currentMsg->GetTargetNodeId() )
					{
						// This message is for the unresponsive node
						// We do not move any "Wake Up No More Information"
						// commands to the pending queue.
						if( !m_currentMsg->IsWakeUpNoMoreInformationCommand() )
						{
							Log::Write( LogLevel_Info, "%s, Node not responding - moving message to Wake-Up queue: %s", GetNodeString( m_currentMsg ).c_str(), m_currentMsg->GetAsString().c_str() );
							MsgQueueItem item;
							item.m_command = MsgQueueCmd_SendMsg;
							item.m_msg = m_currentMsg;
							wakeUp->QueueMsg( item );
						}
						else
						{
							delete m_currentMsg;
						}

						m_currentMsg = NULL;
						m_expectedCallbackId = 0;
						m_expectedCommandClassId = 0;
						m_expectedNodeId = 0;
						m_expectedReply = 0;
						m_waitingForAck = false;
					}
				}

				// Now the message queues
				for( int i=0; i<MsgQueue_Count; ++i )
				{
					list<MsgQueueItem>::iterator it = m_msgQueue[i].begin();
					while( it != m_msgQueue[i].end() )
					{
						bool remove = false;
						MsgQueueItem const& item = *it;
						if( MsgQueueCmd_SendMsg == item.m_command )
						{
							if( _targetNodeId == item.m_msg->GetTargetNodeId() )
							{
								// This message is for the unresponsive node
								// We do not move any "Wake Up No More Information"
								// commands to the pending queue.
								if( !item.m_msg->IsWakeUpNoMoreInformationCommand() )
								{
									Log::Write( LogLevel_Info, "%s, Node not responding - moving message to Wake-Up queue: %s", GetNodeString( item.m_msg ).c_str(), item.m_msg->GetAsString().c_str() );
									wakeUp->QueueMsg( item );
								}
								else
								{
									delete item.m_msg;
								}
								remove = true;
							}
						}
						if( MsgQueueCmd_QueryStageComplete == item.m_command )
						{
							if( _targetNodeId == item.m_nodeId )
							{
								Log::Write( LogLevel_Info, "%s, Node not responding - moving QueryStageComplete command to Wake-Up queue", GetNodeString( item.m_nodeId ).c_str() );
								wakeUp->QueueMsg( item );
								remove = true;
							}
						}

						if( remove )
						{
							it = m_msgQueue[i].erase( it );
						}
						else
						{
							++it;
						}
					}

					// If the queue is now empty, we need to clear its event
					if( m_msgQueue[i].empty() )
					{
						m_queueEvent[i]->Reset();
					}
				}

				m_sendMutex->Unlock();
				
				// Move completed successfully
				return true;
			}
		}
	}

	// Failed to move messages
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::CheckCompletedNodeQueries>
// Identify controller (as opposed to node) commands...especially blocking ones
//-----------------------------------------------------------------------------
void Driver::CheckCompletedNodeQueries
(
)
{
	if( !m_allNodesQueried )
	{
		bool all = true;
		bool sleepingOnly = true;

		LockNodes();
		for( int i=0; i<256; ++i )
		{
			if( m_nodes[i] )
			{
				if( m_nodes[i]->GetCurrentQueryStage() != Node::QueryStage_Complete )
				{
					all = false;
					if( m_nodes[i]->IsListeningDevice() )
					{
						sleepingOnly = false;
					}
				}
			}
		}
		ReleaseNodes();

		if( all )
		{
			// no sleeping nodes, no more nodes in the queue, so...All done
			Log::Write( LogLevel_Info, "         Node query processing complete." );
			Notification* notification = new Notification( Notification::Type_AllNodesQueried );
			notification->SetHomeAndNodeIds( m_homeId, 0xff );
			QueueNotification( notification ); 
			m_awakeNodesQueried = true;
			m_allNodesQueried = true;
		}
		else if( sleepingOnly )
		{
			if (!m_awakeNodesQueried ) 
			{
				// only sleeping nodes remain, so signal awake nodes queried complete
				Log::Write( LogLevel_Info, "         Node query processing complete except for sleeping nodes." );
				Notification* notification = new Notification( Notification::Type_AwakeNodesQueried );
				notification->SetHomeAndNodeIds( m_homeId, 0xff );
				QueueNotification( notification ); 
				m_awakeNodesQueried = true;
			}
		}
	}
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
	if( _command == FUNC_ID_SERIAL_API_SOFT_RESET ) // 0x08
		return true;
	if( ( _command >= FUNC_ID_ZW_SET_DEFAULT ) && // 0x42
	    ( _command <= FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE ) ) // 0x48
		return true;
	if( ( _command >= FUNC_ID_ZW_ADD_NODE_TO_NETWORK ) && // 0x4a
	    ( _command <= FUNC_ID_ZW_GET_SUC_NODE_ID ) )   // 0x56
		return true;
	if( ( _command >= FUNC_ID_ZW_REMOVE_FAILED_NODE_ID ) && // 0x61
	    ( _command <= FUNC_ID_ZW_REPLACE_FAILED_NODE ) ) // 0x63
		return true;
	if( _command == FUNC_ID_ZW_GET_ROUTING_INFO ) // 0x80
		return true;
	if( _command == FUNC_ID_SERIAL_API_SLAVE_NODE_INFO ) // 0xA0
		return true;
	if( _command == FUNC_ID_ZW_SEND_SLAVE_NODE_INFO ) // 0xA2
		return true;
	if( ( _command >= FUNC_ID_ZW_SET_SLAVE_LEARN_MODE ) && // 0xA4
	    ( _command <= FUNC_ID_ZW_IS_VIRTUAL_NODE ) ) // 0xA6
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

	if( !m_controller->Read( buffer, 1 ) )
	{
		// Nothing to read
		return false;
	}

	switch( buffer[0] )
	{
		case SOF:
		{
			m_SOFCnt++;
			if( m_waitingForAck )
			{
				// This can happen on any normal network when a transmission overlaps an unexpected
				// reception and the data in the buffer doesn't contain the ACK. The controller will
				// notice and send us a CAN to retransmit.
				Log::Write( LogLevel_Detail, "Unsolicited message received while waiting for ACK." );
				m_ACKWaiting++;
			}

			// Read the length byte.  Keep trying until we get it.
			m_controller->SetSignalThreshold( 1 );
			int32 response = Wait::Single( m_controller, 50 );
			if( response < 0 )
			{
				Log::Write( LogLevel_Warning, "WARNING: 50ms passed without finding the length byte...aborting frame read");
				m_readAborts++;
				break;
			}

			m_controller->Read( &buffer[1], 1 );
			m_controller->SetSignalThreshold( buffer[1] );
			if( Wait::Single( m_controller, 500 ) < 0 )
			{
				Log::Write( LogLevel_Warning, "WARNING: 500ms passed without reading the rest of the frame...aborting frame read" );
				m_readAborts++;
				m_controller->SetSignalThreshold( 1 );
				break;
			}
			
			m_controller->Read( &buffer[2], buffer[1] );
			m_controller->SetSignalThreshold( 1 );

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
			uint8 nodeId = NodeFromMessage( buffer );
			Log::Write( LogLevel_Detail, "%s,  Received: %s", nodeId ? GetNodeString( nodeId ).c_str() : GetNodeString( m_currentMsg ).c_str(), str.c_str() );

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
				m_readCnt++;
			
				// Process the received message
				ProcessMsg( &buffer[2] );
			}
			else
			{
				Log::Write( LogLevel_Warning, "WARNING: Checksum incorrect - sending NAK" );
				m_badChecksum++;
				uint8 nak = NAK;
				m_controller->Write( &nak, 1 );
				m_controller->Purge();
			}
			break;
		}
			
		case CAN:
		{
			// This is the other side of an unsolicited ACK. As mentioned there if we receive a message
			// just after we transmitted one, the controller will notice and tell us to retransmit here.
			// Don't increment the transmission counter as it is possible the message will never get out
			// on very busy networks with lots of unsolicited messages being received. Increase the amount
			// of retries but only up to a limit so we don't stay here forever.
			Log::Write( LogLevel_Detail, "%s, CAN received...triggering resend", GetNodeString( m_currentMsg ).c_str() );
			m_CANCnt++;
			m_currentMsg->SetMaxSendAttempts( m_currentMsg->GetMaxSendAttempts() + 1 );
			WriteMsg( "CAN" );
			break;
		}
		
		case NAK:
		{
			Log::Write( LogLevel_Warning, "WARNING: NAK received...triggering resend" );
			m_NAKCnt++;
			WriteMsg( "NAK" );
			break;
		}

		case ACK:
		{
			m_ACKCnt++;
			m_waitingForAck = false;		
			if( m_currentMsg == NULL )
			{
				Log::Write( LogLevel_Detail, "contrlr,  ACK received" );
			}
			else
			{
				Log::Write( LogLevel_Detail, "%s,  ACK received CallbackId 0x%.2x Reply 0x%.2x", GetNodeString( m_currentMsg ).c_str(), m_expectedCallbackId, m_expectedReply );
				if( ( 0 == m_expectedCallbackId ) && ( 0 == m_expectedReply ) )
				{
					// Remove the message from the queue, now that it has been acknowledged.
					RemoveCurrentMsg();
				}
			}
			break;
		}
		
		default:
		{
			Log::Write( LogLevel_Warning, "WARNING: Out of frame flow! (0x%.2x).  Sending NAK.", buffer[0] );
			m_OOFCnt++;
			uint8 nak = NAK;
			m_controller->Write( &nak, 1 );
			m_controller->Purge();
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
				Log::Write( LogLevel_Detail, "" );
				HandleSerialAPIGetInitDataResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleGetControllerCapabilitiesResponse( _data );
				break;
			}
			case FUNC_ID_SERIAL_API_GET_CAPABILITIES:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleGetSerialAPICapabilitiesResponse( _data );
				break;
			}
			case FUNC_ID_SERIAL_API_SOFT_RESET:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleSerialAPISoftResetResponse( _data );
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
				Log::Write( LogLevel_Detail, "" );
				HandleGetVersionResponse( _data );
				break;
			}
			case FUNC_ID_ZW_MEMORY_GET_ID:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleMemoryGetIdResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO:
			{
				Log::Write( LogLevel_Detail, "" );
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
				Log::Write( LogLevel_Detail, "" );
				if( !HandleAssignReturnRouteResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
			{
				Log::Write( LogLevel_Detail, "" );
				if( !HandleDeleteReturnRouteResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_ENABLE_SUC:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleEnableSUCResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
			{
				Log::Write( LogLevel_Detail, "" );
				if( !HandleNetworkUpdateResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_SET_SUC_NODE_ID:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleSetSUCNodeIdResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_SUC_NODE_ID:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleGetSUCNodeIdResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_INFO:
			{
				Log::Write( LogLevel_Detail, "" );
				if( _data[2] )
				{
					Log::Write( LogLevel_Info, "%s, FUNC_ID_ZW_REQUEST_NODE_INFO Request successful.", GetNodeString( m_currentMsg ).c_str() );
				}
				else
				{
					Log::Write( LogLevel_Info, "%s, FUNC_ID_ZW_REQUEST_NODE_INFO Request failed.", GetNodeString( m_currentMsg ).c_str() );
				}
				break;
			}
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				Log::Write( LogLevel_Detail, "" );
				if( !HandleRemoveFailedNodeResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_IS_FAILED_NODE_ID:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleIsFailedNodeResponse( _data );
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				Log::Write( LogLevel_Detail, "" );
				if( !HandleReplaceFailedNodeResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_GET_ROUTING_INFO:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleGetRoutingInfoResponse( _data );
				break;
			}
			case FUNC_ID_ZW_R_F_POWER_LEVEL_SET:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleRfPowerLevelSetResponse( _data );
				break;
			}
			case FUNC_ID_ZW_READ_MEMORY:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleReadMemoryResponse( _data );
				break;
			}
			case FUNC_ID_SERIAL_API_SET_TIMEOUTS:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleSerialApiSetTimeoutsResponse( _data );
				break;
			}
			case FUNC_ID_MEMORY_GET_BYTE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleMemoryGetByteResponse( _data );
				break;
			}
			case FUNC_ID_ZW_GET_VIRTUAL_NODES:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleGetVirtualNodesResponse( _data );
				break;
			}
			case FUNC_ID_ZW_SET_SLAVE_LEARN_MODE:
			{
				Log::Write( LogLevel_Detail, "" );
				if( !HandleSetSlaveLearnModeResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_SEND_SLAVE_NODE_INFO:
			{
				Log::Write( LogLevel_Detail, "" );
				if( !HandleSendSlaveNodeInfoResponse( _data ) )
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			default:
			{
				Log::Write( LogLevel_Detail, "" );
				Log::Write( LogLevel_Info, "**TODO: handle response for 0x%.2x**", _data[1] );
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
				Log::Write( LogLevel_Detail, "" );
				HandleApplicationCommandHandlerRequest( _data );
				break;
			}
			case FUNC_ID_ZW_SEND_DATA:
			{
				HandleSendDataRequest( _data, false );
				break;
			}
			case FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE:
			{
				if( m_controllerReplication )
				{
					Log::Write( LogLevel_Detail, "" );
					m_controllerReplication->SendNextData( m_controllerCommandNode );
				}
				break;
			}
			case FUNC_ID_ZW_REPLICATION_SEND_DATA:
			{
				HandleSendDataRequest( _data, true );
				break;
			}
			case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleAssignReturnRouteRequest( _data );
				break;
			}
			case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleDeleteReturnRouteRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleNodeNeighborUpdateRequest( _data );
				break;
			}
			case FUNC_ID_ZW_APPLICATION_UPDATE:
			{
				Log::Write( LogLevel_Detail, "" );
				handleCallback = !HandleApplicationUpdateRequest( _data );
				break;
			}
			case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleAddNodeToNetworkRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleRemoveNodeFromNetworkRequest( _data );
				break;
			}
			case FUNC_ID_ZW_CREATE_NEW_PRIMARY:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleCreateNewPrimaryRequest( _data );
				break;
			}
			case FUNC_ID_ZW_CONTROLLER_CHANGE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleControllerChangeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_SET_LEARN_MODE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleSetLearnModeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleNetworkUpdateRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleRemoveFailedNodeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleReplaceFailedNodeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_SET_SLAVE_LEARN_MODE:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleSetSlaveLearnModeRequest( _data );
				break;
			}
			case FUNC_ID_ZW_SEND_SLAVE_NODE_INFO:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleSendSlaveNodeInfoRequest( _data );
				break;
			}
			case FUNC_ID_APPLICATION_SLAVE_COMMAND_HANDLER:
			{
				Log::Write( LogLevel_Detail, "" );
				HandleApplicationSlaveCommandRequest( _data );
				break;
			}
			case FUNC_ID_PROMISCUOUS_APPLICATION_COMMAND_HANDLER:
			{
				Log::Write( LogLevel_Detail, "" );
				HandlePromiscuousApplicationCommandHandlerRequest( _data );
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
					Log::Write( LogLevel_Detail, "%s,  Expected callbackId was received", GetNodeString( m_currentMsg ).c_str() );
					m_expectedCallbackId = 0;
				}
			}
			if( m_expectedReply )
			{
				if( m_expectedReply == _data[1] )
				{
					if( m_expectedCommandClassId && ( m_expectedReply == FUNC_ID_APPLICATION_COMMAND_HANDLER ) )
					{
						if( m_expectedCommandClassId == _data[5] && m_expectedNodeId == _data[3] )
						{
							Log::Write( LogLevel_Detail, "%s,  Expected reply and command class was received", GetNodeString( m_currentMsg ).c_str() );
							m_waitingForAck = false;
							m_expectedReply = 0;
							m_expectedCommandClassId = 0;
							m_expectedNodeId = 0;
						}
					}
					else
					{
						Log::Write( LogLevel_Detail, "%s,  Expected reply was received", GetNodeString( m_currentMsg ).c_str() );
						m_expectedReply = 0;
					}
				}
			}

			if( !( m_expectedCallbackId || m_expectedReply ) )
			{
				Log::Write( LogLevel_Detail, "%s,  Message transaction complete", GetNodeString( m_currentMsg ).c_str() );
				Log::Write( LogLevel_Detail, "" );
				delete m_currentMsg;
				m_currentMsg = NULL;

				if( m_notifytransactions )
				{
					Notification* notification = new Notification( Notification::Type_MsgComplete );
					notification->SetHomeAndNodeIds( m_homeId, 0xff );
					QueueNotification( notification ); 
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
	
	m_libraryType = _data[m_libraryVersion.size()+3];
	if( m_libraryType < 9 )
	{
		m_libraryTypeName = c_libraryTypeNames[m_libraryType];
	}
	Log::Write( LogLevel_Info, "%s, Received reply to FUNC_ID_ZW_GET_VERSION:", GetNodeString( m_currentMsg ).c_str() );
	Log::Write( LogLevel_Info, "%s,    %s library, version %s", GetNodeString( m_currentMsg ).c_str(), m_libraryTypeName.c_str(), m_libraryVersion.c_str() );
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

	Log::Write( LogLevel_Info, "%s, Received reply to FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:", GetNodeString( m_currentMsg ).c_str() );

	char str[256];
	if( m_controllerCaps & ControllerCaps_SIS )
	{
		Log::Write( LogLevel_Info, "%s,    There is a SUC ID Server (SIS) in this network.", GetNodeString( m_currentMsg ).c_str() );
		snprintf( str, 256, "%s,    The PC controller is an inclusion %s%s%s",
			  GetNodeString( m_currentMsg ).c_str(),
			  ( m_controllerCaps & ControllerCaps_SUC ) ? " static update controller (SUC)" : " controller",
			  ( m_controllerCaps & ControllerCaps_OnOtherNetwork ) ? " which is using a Home ID from another network" : "",
			  ( m_controllerCaps & ControllerCaps_RealPrimary ) ? " and was the original primary before the SIS was added." : "." );
		Log::Write( LogLevel_Info, str );

	}
	else
	{
		Log::Write( LogLevel_Info, "%s,    There is no SUC ID Server (SIS) in this network.", GetNodeString( m_currentMsg ).c_str() );
		snprintf( str, 256, "%s,    The PC controller is a %s%s%s", 
			  GetNodeString( m_currentMsg ).c_str(),
			  ( m_controllerCaps & ControllerCaps_Secondary ) ? "secondary" : "primary",
			  ( m_controllerCaps & ControllerCaps_SUC ) ? " static update controller (SUC)" : " controller",
			  ( m_controllerCaps & ControllerCaps_OnOtherNetwork ) ? " which is using a Home ID from another network." : "." );
		Log::Write( LogLevel_Info, str );
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
	Log::Write( LogLevel_Info, "%s, Received reply to FUNC_ID_SERIAL_API_GET_CAPABILITIES", GetNodeString( m_currentMsg ).c_str() );
	Log::Write( LogLevel_Info, "%s,    Serial API Version:   %d.%d", GetNodeString( m_currentMsg ).c_str(), _data[2], _data[3] );
	Log::Write( LogLevel_Info, "%s,    Manufacturer ID:      0x%.2x%.2x", GetNodeString( m_currentMsg ).c_str(), _data[4], _data[5] );
	Log::Write( LogLevel_Info, "%s,    Product Type:         0x%.2x%.2x", GetNodeString( m_currentMsg ).c_str(), _data[6], _data[7] );
	Log::Write( LogLevel_Info, "%s,    Product ID:           0x%.2x%.2x", GetNodeString( m_currentMsg ).c_str(), _data[8], _data[9] );

	// _data[10] to _data[41] are a 256-bit bitmask with one bit set for 
	// each FUNC_ID_ method supported by the controller.
	// Bit 0 is FUNC_ID_ 1.  So FUNC_ID_SERIAL_API_GET_CAPABILITIES (0x07) will be bit 6 of the first byte.
	m_manufacturerId = (((uint16)_data[4])<<8) | (uint16)_data[5];
	m_productType = (((uint16)_data[6])<<8) | (uint16)_data[7];
	m_productId = (((uint16)_data[8])<<8) | (uint16)_data[9];
	memcpy( m_apiMask, &_data[10], sizeof( m_apiMask ) );

	if( IsBridgeController() )
	{
		SendMsg( new Msg( "FUNC_ID_ZW_GET_VIRTUAL_NODES", 0xff, REQUEST, FUNC_ID_ZW_GET_VIRTUAL_NODES, false ), Driver::MsgQueue_Command);
	}
	SendMsg( new Msg( "FUNC_ID_SERIAL_API_GET_INIT_DATA", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false ), Driver::MsgQueue_Command);
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialAPISoftResetResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSerialAPISoftResetResponse
(
	uint8* _data
)
{

	Log::Write( LogLevel_Info, "Received reply to Soft Reset.");
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
	Log::Write( LogLevel_Info, "Received reply to Enable SUC." );
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
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE - command in progress" );
	}
	else
	{
		// Failed
		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE - command failed" );
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
	Log::Write( LogLevel_Info, "Received reply to SET_SUC_NODE_ID." );
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
	Log::Write( LogLevel_Info, "Received reply to GET_SUC_NODE_ID.  Node ID = %d", _data[2] );

	if( _data[2] == 0)
	{
		Log::Write( LogLevel_Info, "  No SUC, so we become SUC" );
		
		Msg* msg;

		msg = new Msg( "Enable SUC", m_nodeId, REQUEST, FUNC_ID_ZW_ENABLE_SUC, false );
		msg->Append( 1 );	
//		msg->Append( SUC_FUNC_BASIC_SUC );			// SUC
		msg->Append( SUC_FUNC_NODEID_SERVER );		// SIS
		SendMsg( msg, MsgQueue_Send ); 

		msg = new Msg( "Set SUC node ID", m_nodeId, REQUEST, FUNC_ID_ZW_SET_SUC_NODE_ID, false );
		msg->Append( m_nodeId );
		msg->Append( 1 );								// TRUE, we want to be SUC/SIS
		msg->Append( 0 );								// no low power
		msg->Append( SUC_FUNC_NODEID_SERVER );
		SendMsg( msg, MsgQueue_Send ); 
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
	Log::Write( LogLevel_Info, "%s, Received reply to FUNC_ID_ZW_MEMORY_GET_ID. Home ID = 0x%02x%02x%02x%02x.  Our node ID = %d", GetNodeString( m_currentMsg ).c_str(), _data[2], _data[3], _data[4], _data[5], _data[6] );
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

	string nodestr = GetNodeString( m_currentMsg );
	Log::Write( LogLevel_Info, "%s, Received reply to FUNC_ID_SERIAL_API_GET_INIT_DATA:", nodestr.c_str() );
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
					if( IsVirtualNode( nodeId ) )
					{
						Log::Write( LogLevel_Info, "%s,    Node %.3d - Virtual (ignored)", nodestr.c_str(), nodeId );
					}
					else
					{
						Node* node = GetNode( nodeId );
						if( node )
						{
							Log::Write( LogLevel_Info, "%s,    Node %.3d - Known", nodestr.c_str() );
							if( !m_init )
							{
								// The node was read in from the config, so we 
								// only need to get its current state
								node->SetQueryStage( Node::QueryStage_Associations );
							}

							ReleaseNodes();
						}
						else
						{
							// This node is new
							Log::Write( LogLevel_Info, "%s,    Node %.3d - New", nodestr.c_str(), nodeId );
							Notification* notification = new Notification( Notification::Type_NodeNew );
							notification->SetHomeAndNodeIds( m_homeId, nodeId );
							QueueNotification( notification ); 

							// Create the node and request its info
							InitNode( nodeId );
						}
					}
				}
				else
				{
					if( GetNode(nodeId) )
					{
						// This node no longer exists in the Z-Wave network
						Log::Write( LogLevel_Info, "%s,    Node %.3d: Removed", nodestr.c_str(), nodeId );
						delete m_nodes[nodeId];
						m_nodes[nodeId] = NULL;
						Notification* notification = new Notification( Notification::Type_NodeRemoved );
						notification->SetHomeAndNodeIds( m_homeId, nodeId );
						QueueNotification( notification ); 

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
	// The node that the protocol info response is for is not included in the message.
	// We have to assume that the node is the same one as in the most recent request.
	if( !m_currentMsg )
	{
		Log::Write( LogLevel_Warning, "WARNING: Received unexpected FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO message - ignoring.");
		return;
	}

	uint8 nodeId = m_currentMsg->GetTargetNodeId();
	Log::Write( LogLevel_Info, "%s, Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO for node %d", GetNodeString( nodeId ).c_str(), nodeId );

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
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE - command in progress" );
	}
	else
	{
		// Failed
		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE - command failed" );
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
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE - command in progress" );
	}
	else
	{
		// Failed
		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE - command failed" );
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
		string reason;
		switch( _data[2] )
		{
			case FAILED_NODE_NOT_FOUND:
			{
				reason = "Node not found";
				break;
			}
			case FAILED_NODE_REMOVE_PROCESS_BUSY:
			{
				reason = "Remove process busy";
				break;
			}
			case FAILED_NODE_REMOVE_FAIL:
			{
				reason = "Remove failed";
				break;
			}
			default:
			{
				reason = "Command failed";
				break;
			}
		}

		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - %s", reason.c_str() );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
	}
	else
	{
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - Command in progress" );
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
	Log::Write( _data[2]?LogLevel_Warning:LogLevel_Info, "%s Received reply to FUNC_ID_ZW_IS_FAILED_NODE_ID - node %d has %s", _data[2]?"WARNING:":"", m_controllerCommandNode, _data[2] ? "failed" : "not failed" );
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
		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
	}
	else
	{
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - command in progress" );
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
		Log::Write( LogLevel_Detail, "%s,  %s delivered to Z-Wave stack", GetNodeString( m_currentMsg ).c_str(), _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
	}
	else
	{
		Log::Write( LogLevel_Error, "%s, ERROR: %s could not be delivered to Z-Wave stack", GetNodeString( m_currentMsg ).c_str(), _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
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
	Log::Write( LogLevel_Info, "%s, Received reply to FUNC_ID_ZW_GET_ROUTING_INFO", GetNodeString( m_currentMsg ).c_str() );

	if( Node* node = GetNode( m_controllerCommandNode ) )
	{
		// copy the 29-byte bitmap received (29*8=232 possible nodes) into this node's neighbors member variable
		memcpy( node->m_neighbors, &_data[2], 29 );
		ReleaseNodes();
		Log::Write( LogLevel_Info, "%s,    Neighbors of this node are:", GetNodeString( m_currentMsg ).c_str() );
		bool bNeighbors = false;
		for( int by=0; by<29; by++ )
		{
			for( int bi=0; bi<8; bi++ )
			{
				if( (_data[2+by] & (0x01<<bi)) )
				{
					Log::Write( LogLevel_Info, "%s,    Node %d", GetNodeString( m_currentMsg ).c_str(), (by<<3)+bi+1 );
					bNeighbors = true;
				}
			}
		}
		
		if( !bNeighbors )
		{
		  Log::Write( LogLevel_Info, "%s,    (none reported)", GetNodeString( m_currentMsg ).c_str() );
		}
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
void Driver::HandleSendDataRequest
(
	uint8* _data,
	bool _replication
)
{
	string nodestr = GetNodeString( m_currentMsg );
	Log::Write( LogLevel_Detail, "%s,  %s Request with callback ID 0x%.2x received (expected 0x%.2x)", nodestr.c_str(), _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA", _data[2], m_expectedCallbackId );

	if( _data[2] != m_expectedCallbackId )
	{
		// Wrong callback ID
		Log::Write( LogLevel_Warning, "%s, WARNING: Callback ID is invalid", nodestr.c_str() );
	}
	else 
	{
		// Callback ID matches our expectation
		if( ( _data[3] & TRANSMIT_COMPLETE_NOROUTE ) != 0 )
		{
			Log::Write( LogLevel_Info, "%s, ERROR: %s failed.  No route available.", nodestr.c_str(), _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
			RemoveCurrentMsg();
		}
		else if( ( _data[3] & TRANSMIT_COMPLETE_NO_ACK ) != 0 )
		{
			Log::Write( LogLevel_Info, "%s, ERROR: %s failed. No ACK received - device may be asleep.", nodestr.c_str(), _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
			if( m_currentMsg )
			{
				if( !_replication )
				{
					// In case the failure is due to the target being a sleeping node, we 
					// first try to move its pending messages to its wake-up queue.
					if( MoveMessagesToWakeUpQueue( m_currentMsg->GetTargetNodeId() ) )
					{
						return;
					}

					Log::Write( LogLevel_Warning, "%s, WARNING: Device is not a sleeping node - retrying the send.", nodestr.c_str() );
				}
			}
		}
		else if( ( _data[3] & TRANSMIT_COMPLETE_FAIL ) != 0 )
		{
			Log::Write( LogLevel_Info, "%s, ERROR: %s failed. Network is busy.", nodestr.c_str(), _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA" );
		}
		else
		{
			// Command reception acknowledged by node
			m_expectedCallbackId = 0;
		}
	}
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
			Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Success" );
			state = ControllerState_Completed;
			break;
		}
		case SUC_UPDATE_ABORT:
		{
			Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - Error. Process aborted." );
			break;
		}
		case SUC_UPDATE_WAIT:
		{
			Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - SUC is busy." );
			break;
		}
		case SUC_UPDATE_DISABLED:
		{
			Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - SUC is disabled." );
			break;
		}
		case SUC_UPDATE_OVERFLOW:
		{
			Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - Overflow. Full replication required." );
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
	Log::Write( LogLevel_Info, "FUNC_ID_ZW_ADD_NODE_TO_NETWORK:" );
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
	Log::Write( LogLevel_Info, "FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:" );
	
	switch( _data[3] ) 
	{
		case REMOVE_NODE_STATUS_LEARN_READY:
		{
			Log::Write( LogLevel_Info, "REMOVE_NODE_STATUS_LEARN_READY" );
			m_controllerCommandNode = 0;
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Waiting, m_controllerCallbackContext );
			}
			break;
		}
		case REMOVE_NODE_STATUS_NODE_FOUND:
		{
			Log::Write( LogLevel_Info, "REMOVE_NODE_STATUS_NODE_FOUND" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_InProgress, m_controllerCallbackContext );
			}
			break;
		}
		case REMOVE_NODE_STATUS_REMOVING_SLAVE:
		{
			Log::Write( LogLevel_Info, "REMOVE_NODE_STATUS_REMOVING_SLAVE" );
			Log::Write( LogLevel_Info, "Removing node ID %d", _data[4] );
			m_controllerCommandNode = _data[4];
			break;
		}
		case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
		{
			Log::Write( LogLevel_Info, "REMOVE_NODE_STATUS_REMOVING_CONTROLLER" );
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
								Log::Write( LogLevel_Info, "Alternative controller lookup found more then one match. Using the first one found.");
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
					Log::Write( LogLevel_Warning, "WARNING: Node is 0 but not enough data to perform alternative match.");
				}
			}
			else
			{
				m_controllerCommandNode = _data[4];
			}
			Log::Write( LogLevel_Info, "Removing controller ID %d", m_controllerCommandNode );
			break;
		}
		case REMOVE_NODE_STATUS_DONE:
		{
			Log::Write( LogLevel_Info, "REMOVE_NODE_STATUS_DONE" );
			
			if ( m_controllerCommandNode == 0 ) // never received "removing" update
			{
				if ( _data[4] != 0 ) // but message has the clue
					m_controllerCommandNode = _data[4];
			}

			if ( m_controllerCommandNode != 0 )
			{
				LockNodes();
				delete m_nodes[m_controllerCommandNode];
				m_nodes[m_controllerCommandNode] = NULL;
				ReleaseNodes();

				Notification* notification = new Notification( Notification::Type_NodeRemoved );
				notification->SetHomeAndNodeIds( m_homeId, m_controllerCommandNode );
				QueueNotification( notification ); 
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
			Log::Write( LogLevel_Warning, "WARNING: REMOVE_NODE_STATUS_FAILED" );
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
	Log::Write( LogLevel_Info, "FUNC_ID_ZW_CONTROLLER_CHANGE:" );
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
	Log::Write( LogLevel_Info, "FUNC_ID_ZW_CREATE_NEW_PRIMARY:" );
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
	Log::Write( LogLevel_Info, "FUNC_ID_ZW_SET_LEARN_MODE:" );
	
	switch( _data[3] ) 
	{
		case LEARN_MODE_STARTED:
		{
			Log::Write( LogLevel_Info, "LEARN_MODE_STARTED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Waiting, m_controllerCallbackContext );
			}
			break;
		}
		case LEARN_MODE_DONE:
		{
			Log::Write( LogLevel_Info, "LEARN_MODE_DONE" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Completed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;

			// Stop learn mode
			Msg* msg = new Msg( "End Learn Mode", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
			msg->Append( 0 );
			SendMsg( msg, MsgQueue_Command );

			// Rebuild all the node info.  Group and scene data that we stored 
			// during replication will be applied as we discover each node.
			InitAllNodes();
			break;
		}
		case LEARN_MODE_FAILED:
		{
			Log::Write( LogLevel_Warning, "WARNING: LEARN_MODE_FAILED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;

			// Controller change failed
			Msg* msg = new Msg(  "Controller change failed", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true, false );
			msg->Append( CONTROLLER_CHANGE_STOP_FAILED );
			SendMsg( msg, MsgQueue_Command );

			// Rebuild all the node info, since it may have been partially
			// updated by the failed command.  Group and scene data that we
			// stored during replication will be applied as we discover each node.
			InitAllNodes();
			break;
		}
		case LEARN_MODE_DELETED:
		{
			Log::Write( LogLevel_Info, "LEARN_MODE_DELETED" );
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
			Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - Node %d is OK, so command failed", m_controllerCommandNode );
			state = ControllerState_NodeOK;
			break;
		}
		case FAILED_NODE_REMOVED:
		{
			Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - node %d successfully moved to failed nodes list", m_controllerCommandNode );
			state = ControllerState_Completed;
			m_controllerCommand = ControllerCommand_None;

			Notification* notification = new Notification( Notification::Type_NodeRemoved );
			notification->SetHomeAndNodeIds( m_homeId, m_controllerCommandNode );
			QueueNotification( notification ); 
 
			break;
		}
		case FAILED_NODE_NOT_REMOVED:
		{
			Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - unable to move node %d to failed nodes list", m_controllerCommandNode );
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
			Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Node %d is OK, so command failed", m_controllerCommandNode );
			state = ControllerState_NodeOK;
			m_controllerCommand = ControllerCommand_None;
			break;
		}
		case FAILED_NODE_REPLACE_WAITING:
		{
			Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Waiting for new node" );
			state = ControllerState_Waiting;
			break;
		}
		case FAILED_NODE_REPLACE_DONE:
		{
			Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - node %d successfully replaced", m_controllerCommandNode );
			state = ControllerState_Completed;
			m_controllerCommand = ControllerCommand_None;

			// Request new node info for this device
			InitNode( m_controllerCommandNode );
			break;
		}
		case FAILED_NODE_REPLACE_FAILED:
		{
			Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - node %d replacement failed", m_controllerCommandNode );
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
// <Driver::HandlePromiscuousApplicationCommandHandlerRequest>
// Process a request from the Z-Wave PC interface when in promiscuous mode.
//-----------------------------------------------------------------------------
void Driver::HandlePromiscuousApplicationCommandHandlerRequest
(
	uint8* _data
)
{
	//uint8 nodeId = _data[3];
	//uint8 len = _data[4];
	//uint8 classId = _data[5];
	//uint8 destNodeId = _data[5+len];
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
		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE for node %d - FAILED: %s", m_controllerCommandNode, c_transmitStatusNames[_data[3]] );
		if( m_controllerCallback )
		{
			m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
		}
	}
	else
	{
		// Success
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE for node %d - SUCCESS", m_controllerCommandNode );
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
		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE for node %d - FAILED: %s", m_controllerCommandNode, c_transmitStatusNames[_data[3]] );
		if( m_controllerCallback )
		{
			m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
		}
	}
	else
	{
		// Success
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE for node %d - SUCCESS", m_controllerCommandNode );
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
			Log::Write( LogLevel_Info, "REQUEST_NEIGHBOR_UPDATE_STARTED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_InProgress, m_controllerCallbackContext );
			}
			break;
		}
		case REQUEST_NEIGHBOR_UPDATE_DONE:
		{
			Log::Write( LogLevel_Info, "REQUEST_NEIGHBOR_UPDATE_DONE" );

			// We now request the neighbour information from the
			// controller and store it in our node object.
			RequestNodeNeighbors( m_controllerCommandNode, 0 );
			break;
		}
		case REQUEST_NEIGHBOR_UPDATE_FAILED:
		{
			Log::Write( LogLevel_Warning, "WARNING: REQUEST_NEIGHBOR_UPDATE_FAILED" );
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
			Log::Write( LogLevel_Info, "UPDATE_STATE_SUC_ID from node %d", nodeId );
			break;
		}
		case UPDATE_STATE_DELETE_DONE:
		{
			Log::Write( LogLevel_Info, "** Network change **: Z-Wave node %d was removed", nodeId );

			LockNodes();
			delete m_nodes[nodeId];
			m_nodes[nodeId] = NULL;
			ReleaseNodes();

			Notification* notification = new Notification( Notification::Type_NodeRemoved );
			notification->SetHomeAndNodeIds( m_homeId, nodeId );
			QueueNotification( notification ); 
			break;
		}
		case UPDATE_STATE_NEW_ID_ASSIGNED:
		{
			Log::Write( LogLevel_Info, "** Network change **: ID %d was assigned to a new Z-Wave node", nodeId );
			
			// Request the node protocol info (also removes any existing node and creates a new one)
			InitNode( nodeId );		
			break;
		}
		case UPDATE_STATE_ROUTING_PENDING:
		{
			Log::Write( LogLevel_Info, "UPDATE_STATE_ROUTING_PENDING from node %d", nodeId );
			break;
		}
		case UPDATE_STATE_NODE_INFO_REQ_FAILED:
		{
			Log::Write( LogLevel_Warning, "%s, WARNING: FUNC_ID_ZW_APPLICATION_UPDATE: UPDATE_STATE_NODE_INFO_REQ_FAILED received", GetNodeString( m_currentMsg ).c_str() );
	
			// Note: Unhelpfully, the nodeId is always zero in this message.  We have to 
			// assume the message came from the last node to which we sent a request.
			if( m_currentMsg )
			{
				Node* node = GetNodeUnsafe( m_currentMsg->GetTargetNodeId() );
				if( node )
				{
					// Retry the query up to three times
					node->QueryStageRetry( Node::QueryStage_NodeInfo, m_currentMsg->GetMaxSendAttempts() );

					// Just in case the failure was due to the node being asleep, we try
					// to move its pending messages to its wakeup queue.  If it is not
					// a sleeping device, this will have no effect.
					if( MoveMessagesToWakeUpQueue( node->GetNodeId() ) )
					{
						messageRemoved = true;
					}
				}
			}
			break;
		}
		case UPDATE_STATE_NODE_INFO_REQ_DONE:
		{
			Log::Write( LogLevel_Info, "%s, UPDATE_STATE_NODE_INFO_REQ_DONE from node %d", GetNodeString( nodeId ).c_str(), nodeId );
			break;
		}
		case UPDATE_STATE_NODE_INFO_RECEIVED:
		{
			Log::Write( LogLevel_Info, "%s, UPDATE_STATE_NODE_INFO_RECEIVED from node %d", GetNodeString( nodeId ).c_str(), nodeId );
			if( Node* node = GetNodeUnsafe( nodeId ) )
			{
				node->UpdateNodeInfo( &_data[8], _data[4] - 3 );
			}
			break;
		}
	}

	if( messageRemoved )
	{
		m_waitingForAck = false;
		m_expectedCallbackId = 0;
		m_expectedReply = 0;
		m_expectedCommandClassId = 0;
		m_expectedNodeId = 0;
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
			Log::Write( LogLevel_Info, "ADD_NODE_STATUS_LEARN_READY" );
			m_controllerAdded = false;
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Waiting, m_controllerCallbackContext );
			}
			break;
		}
		case ADD_NODE_STATUS_NODE_FOUND:
		{
			Log::Write( LogLevel_Info, "ADD_NODE_STATUS_NODE_FOUND" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_InProgress, m_controllerCallbackContext );
			}
			break;
		}
		case ADD_NODE_STATUS_ADDING_SLAVE:
		{
			Log::Write( LogLevel_Info, "ADD_NODE_STATUS_ADDING_SLAVE" );			
			Log::Write( LogLevel_Info, "Adding node ID %d", _data[4] );
			m_controllerAdded = false;
			m_controllerCommandNode = _data[4];
			break;
		}
		case ADD_NODE_STATUS_ADDING_CONTROLLER:
		{
			Log::Write( LogLevel_Info, "ADD_NODE_STATUS_ADDING_CONTROLLER");
			Log::Write( LogLevel_Info, "Adding controller ID %d", _data[4] );
			m_controllerAdded = true;
			m_controllerCommandNode = _data[4];
			break;
		}
		case ADD_NODE_STATUS_PROTOCOL_DONE:
		{
			Log::Write( LogLevel_Info, "ADD_NODE_STATUS_PROTOCOL_DONE" );
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
				SendMsg( msg, MsgQueue_Command );
			}
			break;
		}
		case ADD_NODE_STATUS_DONE:
		{
			Log::Write( LogLevel_Info, "ADD_NODE_STATUS_DONE" );

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
			Log::Write( LogLevel_Info, "ADD_NODE_STATUS_FAILED" );
			if( m_controllerCallback )
			{
				m_controllerCallback( ControllerState_Failed, m_controllerCallbackContext );
			}
			m_controllerCommand = ControllerCommand_None;

			// Remove the AddNode command from the queue
			RemoveCurrentMsg();

			// Get the controller out of add mode to avoid accidentally adding other devices.
			Msg* msg = new Msg( "Add Node Stop (Failed)", 0xff, REQUEST, _funcId, true );
			msg->Append( ADD_NODE_STOP_FAILED );
			SendMsg( msg, MsgQueue_Command );
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
	ValueID const _valueId,
	uint8 const _intensity
)
{
	// make sure the polling thread doesn't lock the node while we're in this function
	m_pollMutex->Lock();

	// confirm that this node exists
	uint8 nodeId = _valueId.GetNodeId();
	Node* node = GetNode( nodeId );
	if( node != NULL )
	{
		// confirm that this value is in the node's value store
		if( Value* value = node->GetValue( _valueId ) )
		{
			// update the value's pollIntensity
			value->SetPollIntensity( _intensity );

			// Add the valueid to the polling list
			// See if the node is already in the poll list.
			for( list<PollEntry>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
			{
				if( (*it).m_id == _valueId )
				{
					// It is already in the poll list, so we have nothing to do.
					value->Release();
					m_pollMutex->Unlock();
					ReleaseNodes();
					return true;
				}
			}

			// Not in the list, so we add it
			PollEntry pe;
			pe.m_id = _valueId;
			pe.m_pollCounter = value->GetPollIntensity();
			m_pollList.push_back( pe );
			value->Release();
			m_pollMutex->Unlock();
			ReleaseNodes();

			// send notification to indicate polling is enabled
			Notification* notification = new Notification( Notification::Type_PollingEnabled );
			notification->SetHomeAndNodeIds( m_homeId, _valueId.GetNodeId() );
			QueueNotification( notification ); 
			Log::Write( LogLevel_Info, "Node%03d, EnablePoll for HomeID 0x%.8x, value(cc=0x%02x,in=0x%02x,id=0x%02x)", 
				_valueId.GetNodeId(),_valueId.GetHomeId(), _valueId.GetCommandClassId(), _valueId.GetIndex(), _valueId.GetInstance() );
			return true;
		}

		// allow the poll thread to continue
		m_pollMutex->Unlock();

		Log::Write( LogLevel_Info, "EnablePoll failed - value not found for node %d", nodeId );
		return false;
	}

	Log::Write( LogLevel_Info, "EnablePoll failed - node %d not found", nodeId );
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
	// make sure the polling thread doesn't lock the node while we're in this function
	m_pollMutex->Lock();

	// confirm that this node exists
	uint8 nodeId = _valueId.GetNodeId();
	Node* node = GetNode( nodeId );
	if( node != NULL)
	{
		// See if the value is already in the poll list.
		for( list<PollEntry>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
		{
			if( (*it).m_id == _valueId )
			{
				// Found it
				// remove it from the poll list
				m_pollList.erase( it );

				// get the value object and reset pollIntensity to zero (indicating no polling)
				Value* value = GetValue( _valueId );
				value->SetPollIntensity( 0 );
				value->Release();
				m_pollMutex->Unlock();
				ReleaseNodes();

				// send notification to indicate polling is disabled
				Notification* notification = new Notification( Notification::Type_PollingDisabled );
				notification->SetHomeAndNodeIds( m_homeId, _valueId.GetNodeId() );
				QueueNotification( notification ); 
				Log::Write( LogLevel_Info, "Node%03d, DisablePoll for HomeID 0x%.8x, value(cc=0x%02x,in=0x%02x,id=0x%02x)", 
					_valueId.GetNodeId(),_valueId.GetHomeId(), _valueId.GetCommandClassId(), _valueId.GetIndex(), _valueId.GetInstance() );
				return true;
			}
		}

		// Not in the list
		m_pollMutex->Unlock();
		ReleaseNodes();
		Log::Write( LogLevel_Info, "DisablePoll failed - value not on list");
		return false;
	}

	// allow the poll thread to continue
	m_pollMutex->Unlock();

	Log::Write( LogLevel_Info, "DisablePoll failed - node %d not found", nodeId );
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
	bool bPolled;

	// make sure the polling thread doesn't lock the node while we're in this function
	m_pollMutex->Lock();

	Value* value = GetValue( _valueId );
	if( value->GetPollIntensity() != 0 ) 
	{
		bPolled = true;
	}
	else
	{
		bPolled = false;
	}

	value->Release();

	/*
	 * This code is retained for the moment as a belt-and-suspenders test to confirm that
	 * the pollIntensity member of each value and the pollList contents do not get out
	 * of sync.
	 */
	// confirm that this node exists
	uint8 nodeId = _valueId.GetNodeId();
	Node* node = GetNode( nodeId );
	if( node != NULL)
	{

		// See if the value is already in the poll list.
		for( list<PollEntry>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it )
		{
			if( (*it).m_id == _valueId )
			{
				// Found it
				m_pollMutex->Unlock();
				ReleaseNodes();
				if( bPolled )
				{
					return true;
				}
				else
				{
					Log::Write( LogLevel_Error, "IsPolled setting for valueId 0x%016x is not consistent with the poll list", _valueId.GetId() );
				}
			}
		}

		// Not in the list
		m_pollMutex->Unlock();
		ReleaseNodes();
				if( !bPolled )
				{
					return false;
				}
				else
				{
					Log::Write( LogLevel_Error, "IsPolled setting for valueId 0x%016x is not consistent with the poll list", _valueId.GetId() );
				}
	}

	// allow the poll thread to continue
	m_pollMutex->Unlock();

	Log::Write( LogLevel_Info, "isPolled failed - node %d not found (the value reported that it is%s polled)", nodeId, bPolled?"":" not" );
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::SetPollIntensity>
// Set the intensity with which this value is polled
//-----------------------------------------------------------------------------
void Driver::SetPollIntensity
( 
	ValueID const _valueId,
	uint8 const _intensity
)
{
	// make sure the polling thread doesn't lock the value while we're in this function
	m_pollMutex->Lock();

	Value* value = GetValue( _valueId );
	value->SetPollIntensity( _intensity );

	value->Release();
	m_pollMutex->Unlock();
}

//-----------------------------------------------------------------------------
// <Driver::PollThreadEntryPoint>
// Entry point of the thread for poll Z-Wave devices
//-----------------------------------------------------------------------------
void Driver::PollThreadEntryPoint
( 
	Event* _exitEvent,
	void* _context 
)
{
	Driver* driver = (Driver*)_context;
	if( driver )
	{
		driver->PollThreadProc( _exitEvent );
	}
}

//-----------------------------------------------------------------------------
// <Driver::PollThreadProc>
// Thread for poll Z-Wave devices
//-----------------------------------------------------------------------------
void Driver::PollThreadProc
(
	Event* _exitEvent
)
{
	while( 1 )
	{
		int32 pollInterval = m_pollInterval;

		if( m_awakeNodesQueried )
		{
			if( !m_pollList.empty() )
			{
				// We only bother getting the lock if the pollList is not empty
				m_pollMutex->Lock();
			
				// Get the next value to be polled
				PollEntry pe = m_pollList.front();
				m_pollList.pop_front();
				ValueID  valueId = pe.m_id;
			
				// only execute this poll if pe.m_pollCounter == 1; otherwise decrement the counter and process the next polled value
				if( pe.m_pollCounter != 1)
				{
					pe.m_pollCounter--;
					m_pollList.push_back( pe );
					m_pollMutex->Unlock();
					continue;
				}

				// reset the poll counter to the full pollIntensity value and push it at the end of the list
				// release the value object referenced; call GetNode to ensure the node objects are locked during this period
				(void)GetNode( valueId.GetNodeId() );
				Value* value = GetValue( valueId );
				pe.m_pollCounter = value->GetPollIntensity();
				m_pollList.push_back( pe );
				value->Release();
				ReleaseNodes();

				// If the polling interval is for the whole poll list, calculate the time before the next poll, 
				// so that all polls can take place within the user-specified interval.
				if( !m_bIntervalBetweenPolls )
				{
					if( pollInterval < 100 )
					{
						Log::Write( LogLevel_Info, "The pollInterval setting is only %d, which appears to be a legacy setting.  Multiplying by 1000 to convert to ms.", pollInterval );
						pollInterval *= 1000;
					}
					pollInterval /= m_pollList.size();
				}

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
						Log::Write( LogLevel_Detail, "%s, Polling: %s index = %d instance = %d (poll queue has %d messages)", GetNodeString( node->m_nodeId ).c_str(), cc->GetCommandClassName().c_str(), index, instance, m_msgQueue[MsgQueue_Poll].size() );
						cc->RequestValue( 0, index, instance, MsgQueue_Poll );
					}

					ReleaseNodes();
				}

				m_pollMutex->Unlock();

				// Polling messages are only sent when there are no other messages waiting to be sent
				// While this makes the polls much more variable and uncertain if some other activity dominates
				// a send queue, that may be appropriate
				// TODO we can have a debate about whether to test all four queues or just the Poll queue
				while( !m_msgQueue[MsgQueue_Poll].empty()
					|| !m_msgQueue[MsgQueue_Send].empty()
					|| !m_msgQueue[MsgQueue_Command].empty()
					|| !m_msgQueue[MsgQueue_Query].empty() )
				{
					// Wait for the interval to expire, while watching for exit events
					int32 i32 = Wait::Single( _exitEvent, pollInterval );
					if( i32 == 0 )
					{
						// Exit has been called
						return;
					}
				}
			}
			else		// poll list is empty or awake nodes haven't been fully queried yet
			{
				// don't poll just yet, wait for the pollInterval or exit before re-checking to see if the pollList has elements
				int32 i32 = Wait::Single( _exitEvent, pollInterval );
				if( i32 == 0 )
				{
					// Exit has been called
					return;
				}
			}
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
	SendMsg( msg, MsgQueue_Send );
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
		delete m_nodes[_nodeId];
		Notification* notification = new Notification( Notification::Type_NodeRemoved );
		notification->SetHomeAndNodeIds( m_homeId, _nodeId );
		QueueNotification( notification ); 
	}

	// Add the new node
	m_nodes[_nodeId] = new Node( m_homeId, _nodeId );
	// Do controller specific node initializations
	if( _nodeId == m_nodeId )
	{
		ManufacturerSpecific::SetProductDetails(m_nodes[_nodeId], m_manufacturerId, m_productType, m_productId);
	}
	ReleaseNodes();

	Notification* notification = new Notification( Notification::Type_NodeAdded );
	notification->SetHomeAndNodeIds( m_homeId, _nodeId );
	QueueNotification( notification ); 

	// Request the node info
	m_nodes[_nodeId]->SetQueryStage( Node::QueryStage_ProtocolInfo );
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
// <Driver::IsNodeFrequentListeningDevice>
// Get whether the node is a listening device that does not go to sleep
//-----------------------------------------------------------------------------
bool Driver::IsNodeFrequentListeningDevice
(
	 uint8 const _nodeId
)
{
	bool res = false;
	if( Node* node = GetNode( _nodeId ) )
	{
		res = node->IsFrequentListeningDevice();
		ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeBeamingDevice>
// Get whether the node is a beam capable device.
//-----------------------------------------------------------------------------
bool Driver::IsNodeBeamingDevice
(
	 uint8 const _nodeId
)
{
	bool res = false;
	if( Node* node = GetNode( _nodeId ) )
	{
		res = node->IsBeamingDevice();
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
// <Driver::IsNodeSecurityDevice>
// Get the security attribute for a node
//-----------------------------------------------------------------------------
bool Driver::IsNodeSecurityDevice
(
	 uint8 const _nodeId
)
{
	bool security = false;
	if( Node* node = GetNode( _nodeId ) )
	{
		security = node->IsSecurityDevice();
		ReleaseNodes();
	}

	return security;
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
// Get the security byte of a node
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

	return "";
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

	return "";
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

	return "";
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
	Log::Write( LogLevel_Info, "Reset controller and erase all node information");
	Msg* msg = new Msg( "Reset controller and erase all node information", 0xff, REQUEST, FUNC_ID_ZW_SET_DEFAULT, true );
	SendMsg( msg, MsgQueue_Command );
}

//-----------------------------------------------------------------------------
// <Driver::SoftReset>
// Soft-reset the Z-Wave controller chip
//-----------------------------------------------------------------------------
void Driver::SoftReset
(
)
{
	Log::Write( LogLevel_Info, "Soft-resetting the Z-Wave controller chip");
	Msg* msg = new Msg( "Soft-resetting the Z-Wave controller chip", 0xff, REQUEST, FUNC_ID_SERIAL_API_SOFT_RESET, false, false );
	SendMsg( msg, MsgQueue_Command );
}

//-----------------------------------------------------------------------------
// <Driver::RequestNodeNeighbors>
// Get the neighbour information for a node from the controller
//-----------------------------------------------------------------------------
void Driver::RequestNodeNeighbors
( 
	uint8 const _nodeId,
	uint32 const _requestFlags
)
{
	//Log::Write( LogLevel_Detail, "IsAPICallSupported( FUNC_ID_ZW_GET_ROUTING_INFO ) = %s", IsAPICallSupported( FUNC_ID_ZW_GET_ROUTING_INFO ) ? "true" : "false" );
	// Handle casse where some controllers actually support but don't claim to. Z-Troller
	if( IsAPICallSupported( FUNC_ID_ZW_GET_ROUTING_INFO ) )
	{
		// Note: This is not the same as RequestNodeNeighbourUpdate.  This method
		// merely requests the controller's current neighbour information and
		// the reply will be copied into the relevant Node object for later use.
		m_controllerCommandNode = _nodeId;
		Log::Write( LogLevel_Detail, "Node%03d, Requesting routing info (neighbor list)", _nodeId );
		Msg* msg = new Msg( "Get Routing Info", _nodeId, REQUEST, FUNC_ID_ZW_GET_ROUTING_INFO, false );
		msg->Append( _nodeId );
		msg->Append( 1 );		// Exclude bad links
		msg->Append( 1 );		// Exclude non-routing neighbors
		SendMsg( msg, MsgQueue_Command );
	}
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
	uint8 _nodeId,
	uint8 _arg
)
{
	if( ControllerCommand_None != m_controllerCommand )
	{
		// Already busy doing something else
		return false;
	}

	bool res = true;
	m_controllerCallback = _callback;
	m_controllerCallbackContext = _context;
	m_controllerCommand = _command;

	switch( m_controllerCommand )
	{
		case ControllerCommand_AddController:
		{
			Log::Write( LogLevel_Info, "AddController" );
			Msg* msg = new Msg( "AddController", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( _highPower ? ADD_NODE_CONTROLLER | OPTION_HIGH_POWER : ADD_NODE_CONTROLLER );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_AddDevice:
		{
			Log::Write( LogLevel_Info, "AddDevice" );
			Msg* msg = new Msg( "AddDevice", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( _highPower ? ADD_NODE_SLAVE | OPTION_HIGH_POWER : ADD_NODE_SLAVE );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_CreateNewPrimary:
		{
			Log::Write( LogLevel_Info, "CreateNewPrimary" );
			Msg* msg = new Msg( "CreateNewPrimary", 0xff, REQUEST, FUNC_ID_ZW_CREATE_NEW_PRIMARY, true );
			msg->Append( CREATE_PRIMARY_START );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_ReceiveConfiguration:
		{
			Log::Write( LogLevel_Info, "ReceiveConfiguration" );
			Msg* msg = new Msg( "ReceiveConfiguration", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, true );
			msg->Append( 0xff );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_RemoveController:
		{
			Log::Write( LogLevel_Info, "RemoveController" );
			Msg* msg = new Msg( "RemoveController", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( _highPower ? REMOVE_NODE_ANY | OPTION_HIGH_POWER : REMOVE_NODE_ANY );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_RemoveDevice:
		{
			Log::Write( LogLevel_Info, "RemoveDevice" );
			Msg* msg = new Msg( "RemoveDevice", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( _highPower ? REMOVE_NODE_ANY | OPTION_HIGH_POWER : REMOVE_NODE_ANY );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_HasNodeFailed:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( LogLevel_Info, "Requesting whether node %d has failed", _nodeId );
			Msg* msg = new Msg( "Has Node Failed?", 0xff, REQUEST, FUNC_ID_ZW_IS_FAILED_NODE_ID, false );		
			msg->Append( _nodeId );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_RemoveFailedNode:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( LogLevel_Info, "Marking node %d as having failed", _nodeId );
			Msg* msg = new Msg( "Mark Node As Failed", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_FAILED_NODE_ID, true );		
			msg->Append( _nodeId );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_ReplaceFailedNode:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( LogLevel_Info, "Replace Failed Node %d", _nodeId );
			Msg* msg = new Msg( "ReplaceFailedNode", 0xff, REQUEST, FUNC_ID_ZW_REPLACE_FAILED_NODE, true );
			msg->Append( _nodeId );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_TransferPrimaryRole:
		{
			Log::Write( LogLevel_Info, "TransferPrimaryRole" );
			Msg* msg = new Msg( "TransferPrimaryRole", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true );
			msg->Append( CONTROLLER_CHANGE_START );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_RequestNetworkUpdate:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( LogLevel_Info, "RequestNetworkUpdate" );
			Msg* msg = new Msg( "RequestNetworkUpdate", 0xff, REQUEST, FUNC_ID_ZW_REQUEST_NETWORK_UPDATE, true );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_RequestNodeNeighborUpdate:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( LogLevel_Info, "Requesting Neighbor Update for node %d", _nodeId );
			Msg* msg = new Msg( "Requesting Neighbor Update", _nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE, true );
			msg->Append( _nodeId );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_AssignReturnRoute:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( LogLevel_Info, "Assigning return route from node %d", _nodeId );
			Msg* msg = new Msg( "Assigning return route", _nodeId, REQUEST, FUNC_ID_ZW_ASSIGN_RETURN_ROUTE, true );
			msg->Append( _nodeId );		// from the node
			msg->Append( m_nodeId );	// to the controller
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_DeleteAllReturnRoutes:
		{
			m_controllerCommandNode = _nodeId;
			Log::Write( LogLevel_Info, "Deleting all return routes from node %d", _nodeId );
			Msg* msg = new Msg( "Deleting return routes", _nodeId, REQUEST, FUNC_ID_ZW_DELETE_RETURN_ROUTE, true );
			msg->Append( _nodeId );		// from the node
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_CreateButton:
		{
			if( IsBridgeController() )
			{
				Node* node = GetNodeUnsafe( _nodeId );
				if( node != NULL )
				{
					if( node->m_buttonMap.find( _arg ) == node->m_buttonMap.end() && m_virtualNeighborsReceived )
					{
						bool found = false;
						for( uint8 n = 1; n <= 232 && !found; n++ )
						{
							if( !IsVirtualNode( n ))
								continue;

							map<uint8,uint8>::iterator it = node->m_buttonMap.begin();
							for( ; it != node->m_buttonMap.end(); it++ )
							{
								// is virtual node already in map?
								if( it->second == n )
									break;
							}
							if( it == node->m_buttonMap.end() ) // found unused virtual node
							{
								m_controllerCommandNode = _nodeId;
								m_controllerCommandArg = _arg;
								node->m_buttonMap[_arg] = n;
								SendVirtualNodeInfo( n, _nodeId );
								found = true;
							}
						}
						if( !found ) // create a new virtual node
						{
							m_controllerCommandNode = _nodeId;
							m_controllerCommandArg = _arg;
							Log::Write( LogLevel_Info, "AddVirtualNode" );
							Msg* msg = new Msg( "Slave Node Information", 0xff, REQUEST, FUNC_ID_SERIAL_API_SLAVE_NODE_INFO, false, false );
							msg->Append( 0 );		// node 0
							msg->Append( 1 );		// listening
							msg->Append( 0x09 );		// genericType window covering
							msg->Append( 0x00 );		// specificType undefined
							msg->Append( 0 );		// length
							SendMsg( msg, MsgQueue_Command );

							msg = new Msg( "Add Virtual Node", 0xff, REQUEST, FUNC_ID_ZW_SET_SLAVE_LEARN_MODE, true );
							msg->Append( 0 );		// node 0 to add
							if( IsPrimaryController() || IsInclusionController() )
							{
								msg->Append( SLAVE_LEARN_MODE_ADD );
							}
							else
							{
								msg->Append( SLAVE_LEARN_MODE_ENABLE );
							}
							SendMsg( msg, MsgQueue_Command );
						}
					}
					else
						res = false; // button id already used
				}
				else
					res = false; // node not found
			} else
				res = false; // not bridge controller
			break;
		}
		case ControllerCommand_DeleteButton:
		{
			if( IsBridgeController() )
			{
				Node* node = GetNodeUnsafe( _nodeId );
				if( node != NULL )
				{
					// Make sure button is allocated to a virtual node.
					m_controllerCommandNode = _nodeId;
					if( node->m_buttonMap.find( _arg ) != node->m_buttonMap.end() )
					{
#ifdef notdef
						// We would need a reference count to decide when to free virtual nodes
						// We could do this by making the bitmap if virtual nodes into a map that also holds a reference count.
						Log::Write( LogLevel_Info, "RemoveVirtualNode %d", _nodeId );
						Msg* msg = new Msg( "Remove Virtual Node", 0xff, REQUEST, FUNC_ID_ZW_SET_SLAVE_LEARN_MODE, true );
						msg->Append( _nodeId );		// from the node
						if( IsPrimaryController() || IsInclusionController() )
							msg->Append( SLAVE_LEARN_MODE_REMOVE );
						else
							msg->Append( SLAVE_LEARN_MODE_ENABLE );
						SendMsg( msg );
#endif
						node->m_buttonMap.erase( _arg );
						SaveButtons();
					
						Notification* notification = new Notification( Notification::Type_DeleteButton );
						notification->SetHomeAndNodeIds( m_homeId, m_controllerCommandNode );
						notification->SetButtonId( _arg );
						QueueNotification( notification );
					}
					else
						res = false; // button id not found
				}
				else
					res = false; // node not found
			}
			else
				res = false; // not bridge controller
			break;
		}
        	case ControllerCommand_None:
		{
			// To keep gcc quiet
			break;
		}
	}

	return res;
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
			Log::Write( LogLevel_Info, "CancelAddController" );
			m_controllerCommandNode = 0xff;		// identify the fact that there is no new node to initialize
			Msg* msg = new Msg( "CancelAddController", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( ADD_NODE_STOP );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_AddDevice:
		{
			Log::Write( LogLevel_Info, "CancelAddDevice" );
			m_controllerCommandNode = 0xff;		// identify the fact that there is no new node to initialize
			Msg* msg = new Msg( "CancelAddDevice", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true );
			msg->Append( ADD_NODE_STOP );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_CreateNewPrimary:
		{
			Log::Write( LogLevel_Info, "CancelCreateNewPrimary" );
			Msg* msg = new Msg( "CancelCreateNewPrimary", 0xff, REQUEST, FUNC_ID_ZW_CREATE_NEW_PRIMARY, true );
			msg->Append( CREATE_PRIMARY_STOP );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_ReceiveConfiguration:
		{
			Log::Write( LogLevel_Info, "CancelReceiveConfiguration" );
			Msg* msg = new Msg( "CancelReceiveConfiguration", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false );
			msg->Append( 0 );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_RemoveController:
		{
			Log::Write( LogLevel_Info, "CancelRemoveController" );
			Msg* msg = new Msg( "CancelRemoveController", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( REMOVE_NODE_STOP );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_RemoveDevice:
		{
			Log::Write( LogLevel_Info, "CancelRemoveDevice" );
			Msg* msg = new Msg( "CancelRemoveDevice", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true );
			msg->Append( REMOVE_NODE_STOP );
			SendMsg( msg, MsgQueue_Command );
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
			Log::Write( LogLevel_Info, "CancelTransferPrimaryRole" );
			Msg* msg = new Msg( "CancelTransferPrimaryRole", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true );
			msg->Append( CONTROLLER_CHANGE_STOP );
			SendMsg( msg, MsgQueue_Command );
			break;
		}
		case ControllerCommand_CreateButton:
		case ControllerCommand_DeleteButton:
		{
			if( m_controllerCommandNode != 0 )
			{
				SendSlaveLearnModeOff();
			}
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
	int32 _value,
	uint8 _size
)
{
	bool res = false;
	if( Node* node = GetNode( _nodeId ) )
	{
		res = node->SetConfigParam( _param, _value, _size );
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
	m_notificationsEvent->Set();
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
	m_notificationsEvent->Reset();
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
	Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_R_F_POWER_LEVEL_SET");

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
	Log::Write( LogLevel_Info, "Received reply to FUNC_ID_SERIAL_API_SET_TIMEOUTS");
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
    Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_MEMORY_GET_BYTE, returned data: 0x%02hx 0x%02hx 0x%02hx",
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
	Log::Write( LogLevel_Info, "Received reply to FUNC_ID_MEMORY_GET_BYTE");
	return res; 
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetVirtualNodesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetVirtualNodesResponse
(
	uint8* _data
)
{
	Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_GET_VIRTUAL_NODES" );
	memcpy( m_virtualNeighbors, &_data[2], 29 );
	m_virtualNeighborsReceived = true;
	bool bNeighbors = false;
	for( int by=0; by<29; by++ )
	{
		for( int bi=0; bi<8; bi++ )
		{
			if( (_data[2+by] & (0x01<<bi)) )
			{
				Log::Write( LogLevel_Info, "    Node %d", (by<<3)+bi+1 );
				bNeighbors = true;
			}
		}
	}
	if( !bNeighbors )
		Log::Write( LogLevel_Info, "    (none reported)" );
}

//-----------------------------------------------------------------------------
// <Driver::GetVirtualNeighbors>
// Gets the virtual neighbors for a network
//-----------------------------------------------------------------------------
uint32 Driver::GetVirtualNeighbors
(
	uint8** o_neighbors
)
{
	int i;
	uint32 numNeighbors = 0;
	if( !m_virtualNeighborsReceived )
	{
		*o_neighbors = NULL;
		return 0;
	}
	for( i = 0; i < 29; i++ )
	{
		for( unsigned char mask = 0x80; mask != 0; mask >>= 1 )
			if( m_virtualNeighbors[i] & mask )
				numNeighbors++;
	}

	// handle the possibility that no neighbors are reported
	if( !numNeighbors )
	{
		*o_neighbors = NULL;
		return 0;
	}

	// create and populate an array with neighbor node ids
	uint8* neighbors = new uint8[numNeighbors];
	uint32 index = 0;
	for( int by=0; by<29; by++ )
	{
		for( int bi=0; bi<8; bi++ )
		{
			if( (m_virtualNeighbors[by] & (0x01<<bi)) )
				neighbors[index++] = ( (by<<3) + bi + 1 );
		}
	}

	*o_neighbors = neighbors;
	return numNeighbors;
}

//-----------------------------------------------------------------------------
// <Driver::RequestVirtualNeighbors>
// Get the virtual neighbour information from the controller
//-----------------------------------------------------------------------------
void Driver::RequestVirtualNeighbors
( 
	MsgQueue const _queue
)
{
	Msg* msg = new Msg( "Get Virtual Neighbor List", 0xff, REQUEST, FUNC_ID_ZW_GET_VIRTUAL_NODES, false );
	SendMsg( msg, _queue );
}

//-----------------------------------------------------------------------------
// <Driver::SendVirtualNodeInfo>
// Send node info frame on behalf of a virtual node.
//-----------------------------------------------------------------------------
void Driver::SendVirtualNodeInfo
(
	uint8 const _FromNodeId,
	uint8 const _ToNodeId
)
{
	char str[80];

	snprintf( str, sizeof(str), "Send Virtual Node Info from %d to %d", _FromNodeId, _ToNodeId );
	Msg* msg = new Msg( str, 0xff, REQUEST, FUNC_ID_ZW_SEND_SLAVE_NODE_INFO, true );
	msg->Append( _FromNodeId );		// from the virtual node
	msg->Append( _ToNodeId );		// to the handheld controller
	msg->Append( TRANSMIT_OPTION_ACK );
	SendMsg( msg, MsgQueue_Command );
}

//-----------------------------------------------------------------------------
// <Driver::SendSlaveLearnModeOff>
// Disable Slave Learn Mode.
//-----------------------------------------------------------------------------
void Driver::SendSlaveLearnModeOff
(
)
{
	if( !( IsPrimaryController() || IsInclusionController() ) )
	{
		Msg* msg = new Msg( "Set Slave Learn Mode Off ", 0xff, REQUEST, FUNC_ID_ZW_SET_SLAVE_LEARN_MODE, true );
		msg->Append( 0 );	// filler node id
		msg->Append( SLAVE_LEARN_MODE_DISABLE );
		SendMsg( msg, MsgQueue_Command  );
	}
}

//-----------------------------------------------------------------------------
// <Driver::SaveButtons>
// Save button info into file.
//-----------------------------------------------------------------------------
void Driver::SaveButtons
( 
)
{
	char str[16];

	// Create a new XML document to contain the driver configuration
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
	TiXmlElement* nodesElement = new TiXmlElement( "Nodes" );
	doc.LinkEndChild( decl );
	doc.LinkEndChild( nodesElement );

	snprintf( str, sizeof(str), "%d", 1 );
	nodesElement->SetAttribute( "version", str);

	LockNodes();
	for( int i = 1; i < 256; i++ )
	{
		if( m_nodes[i] == NULL || m_nodes[i]->m_buttonMap.empty() )
		{
			continue;
		}

		TiXmlElement* nodeElement = new TiXmlElement( "Node" );

		snprintf( str, sizeof(str), "%d", i );
		nodeElement->SetAttribute( "id", str );

		for( map<uint8,uint8>::iterator it = m_nodes[i]->m_buttonMap.begin(); it != m_nodes[i]->m_buttonMap.end(); ++it )
		{
			TiXmlElement* valueElement = new TiXmlElement( "Button" );

			snprintf( str, sizeof(str), "%d", it->first );
			valueElement->SetAttribute( "id", str );

			snprintf( str, sizeof(str), "%d", it->second );
			TiXmlText* textElement = new TiXmlText( str );
			valueElement->LinkEndChild( textElement );

			nodeElement->LinkEndChild( valueElement );
		}

		nodesElement->LinkEndChild( nodeElement );
	}
	ReleaseNodes();

	string userPath;
	Options::Get()->GetOptionAsString( "UserPath", &userPath );

	string filename =  userPath + "zwbutton.xml";

	doc.SaveFile( filename.c_str() );
}
//-----------------------------------------------------------------------------
// <Driver::ReadButtons>
// Read button info per node from file.
//-----------------------------------------------------------------------------
void Driver::ReadButtons
( 
	uint8 const _nodeId
)
{
	int32 intVal;
	int32 nodeId;
	int32 buttonId;
	char const* str;

	// Load the XML document that contains the driver configuration
	string userPath;
	Options::Get()->GetOptionAsString( "UserPath", &userPath );
	
	string filename =  userPath + "zwbutton.xml";

	TiXmlDocument doc;
	if( !doc.LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		Log::Write( LogLevel_Info, "Driver::ReadButtons - zwbutton.xml file not found.");
		return;
	}

	TiXmlElement const* nodesElement = doc.RootElement();
	str = nodesElement->Value();
	if( str && strcmp( str, "Nodes" ))
	{
		Log::Write( LogLevel_Warning, "WARNING: Driver::ReadButtons - zwbutton.xml is malformed");
		return;
	}

	// Version
	if( TIXML_SUCCESS == nodesElement->QueryIntAttribute( "version", &intVal ) )
	{
		if( (uint32)intVal != 1 )
		{
			Log::Write( LogLevel_Info, "Driver::ReadButtons - %s is from an older version of OpenZWave and cannot be loaded.", "zwbutton.xml" );
			return;
		}
	}
	else
	{
		Log::Write( LogLevel_Warning, "WARNING: Driver::ReadButtons - zwbutton.xml is from an older version of OpenZWave and cannot be loaded." );
		return;
	}

	TiXmlElement const* nodeElement = nodesElement->FirstChildElement();
	while( nodeElement )
	{
		str = nodeElement->Value();
		if( str && !strcmp( str, "Node" ))
		{
			Node* node = NULL;
			if( TIXML_SUCCESS == nodeElement->QueryIntAttribute( "id", &intVal ) )
			{
				if( _nodeId == intVal )
				{
					node = GetNodeUnsafe( intVal );
				}
			}
			if( node != NULL )
			{
				TiXmlElement const* buttonElement = nodeElement->FirstChildElement();
				while( buttonElement )
				{
					str = buttonElement->Value();
					if( str && !strcmp( str, "Button"))
					{
						if (TIXML_SUCCESS != buttonElement->QueryIntAttribute( "id", &buttonId ) )
						{
							Log::Write( LogLevel_Warning, "WARNING: Driver::ReadButtons - cannot find Button Id for node %d", _nodeId );
							return;
						}
						str = buttonElement->GetText();
						if( str )
						{
							char *p;
							nodeId = (int32)strtol( str, &p, 0 );
						}
						else
						{
							Log::Write( LogLevel_Info, "Driver::ReadButtons - missing virtual node value for node %d button id %d", _nodeId, buttonId );
							return;
						}
						node->m_buttonMap[buttonId] = nodeId;
						Notification* notification = new Notification( Notification::Type_CreateButton );
						notification->SetHomeAndNodeIds( m_homeId, nodeId );
						notification->SetButtonId( buttonId );
						QueueNotification( notification );
					}
					buttonElement = buttonElement->NextSiblingElement();
				}
			}
		}
		nodeElement = nodeElement->NextSiblingElement();
	}
}
//-----------------------------------------------------------------------------
// <Driver::HandleSetSlaveLearnModeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleSetSlaveLearnModeResponse
(
	uint8* _data
)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if( _data[2] )
	{
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_SET_SLAVE_LEARN_MODE - command in progress" );
	}
	else
	{
		// Failed
		Log::Write( LogLevel_Warning, "WARNING: Received reply to FUNC_ID_ZW_SET_SLAVE_LEARN_MODE - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		res = false;
		SendSlaveLearnModeOff();
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	return res; 
}

//-----------------------------------------------------------------------------
// <Driver::HandleSetSlaveLearnModeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSetSlaveLearnModeRequest
(
	uint8* _data
)
{
	ControllerState state = ControllerState_Waiting;

	SendSlaveLearnModeOff();
	switch( _data[3] )
	{
		case SLAVE_ASSIGN_COMPLETE:
		{
			Log::Write( LogLevel_Info, "SLAVE_ASSIGN_COMPLETE" );
			if( _data[4] == 0 ) // original node is 0 so adding
			{
				Log::Write( LogLevel_Info, "Adding virtual node ID %d", _data[5] );
				Node* node = GetNodeUnsafe( m_controllerCommandNode );
				if( node != NULL )
				{
					node->m_buttonMap[m_controllerCommandArg] = _data[5];
					SendVirtualNodeInfo( _data[5], m_controllerCommandNode );
				}
			}
			else
				if( _data[5] == 0 )
				{
					Log::Write( LogLevel_Info, "Removing virtual node ID %d", _data[4] );
				}
			break;
		}
		case SLAVE_ASSIGN_NODEID_DONE:
		{
			Log::Write( LogLevel_Info, "SLAVE_ASSIGN_NODEID_DONE" );
			if( _data[4] == 0 ) // original node is 0 so adding
			{
				Log::Write( LogLevel_Info, "Adding virtual node ID %d", _data[5] );
				Node* node = GetNodeUnsafe( m_controllerCommandNode );
				if( node != NULL )
				{
					node->m_buttonMap[m_controllerCommandArg] = _data[5];
					SendVirtualNodeInfo( _data[5], m_controllerCommandNode );
				}
			}
			else
				if( _data[5] == 0 )
				{
					Log::Write( LogLevel_Info, "Removing virtual node ID %d", _data[4] );
				}
			break;
		}
		case SLAVE_ASSIGN_RANGE_INFO_UPDATE:
		{
			Log::Write( LogLevel_Info, "SLAVE_ASSIGN_RANGE_INFO_UPDATE" );
			break;
		}
	}
	m_controllerAdded = false;

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendSlaveNodeInfoResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleSendSlaveNodeInfoResponse
(
	uint8* _data
)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if( _data[2] )
	{
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_SEND_SLAVE_NODE_INFO - command in progress" );
	}
	else
	{
		// Failed
		Log::Write( LogLevel_Info, "Received reply to FUNC_ID_ZW_SEND_SLAVE_NODE_INFO - command failed" );
		state = ControllerState_Failed;
		m_controllerCommand = ControllerCommand_None;
		// Undo button map settings
		Node* node = GetNodeUnsafe( m_controllerCommandNode );
		if( node != NULL )
		{
			node->m_buttonMap.erase( m_controllerCommandArg );
		}
		res = false;
	}

	if( m_controllerCallback )
	{
		m_controllerCallback( state, m_controllerCallbackContext );
	}
	return res; 
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendSlaveNodeInfoRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendSlaveNodeInfoRequest
(
	uint8* _data
)
{
	Log::Write( LogLevel_Info, "SEND_SLAVE_NODE_INFO_COMPLETE %s", c_transmitStatusNames[_data[3]] );
	if( _data[3] == 0 )	// finish up
	{
		ControllerState state = ControllerState_Completed;

		SaveButtons();
		Notification* notification = new Notification( Notification::Type_CreateButton );
		notification->SetHomeAndNodeIds( m_homeId, m_controllerCommandNode );
		notification->SetButtonId( m_controllerCommandArg );
		QueueNotification( notification );

		if( m_controllerCallback )
		{
			m_controllerCallback( state, m_controllerCallbackContext );
		}
		m_controllerCommand = ControllerCommand_None;
		RequestVirtualNeighbors( MsgQueue_Send );
	}
	else			// error. try again
	{
		Node* node = GetNodeUnsafe( m_controllerCommandNode );
		if( node != NULL)
		{
			SendVirtualNodeInfo( node->m_buttonMap[m_controllerCommandArg], m_controllerCommandNode );
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleApplicationSlaveCommandRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleApplicationSlaveCommandRequest
(
	uint8* _data
)
{
	Log::Write( LogLevel_Info, "APPLICATION_SLAVE_COMMAND_HANDLER rxStatus %x dest %d source %d len %d", _data[2], _data[3], _data[4], _data[5] );
	Node* node = GetNodeUnsafe( _data[4] );
	if( node != NULL && _data[5] == 3 && _data[6] == 0x20 && _data[7] == 0x01 ) // only support Basic Set for now
	{
		map<uint8,uint8>::iterator it = node->m_buttonMap.begin();
		for( ; it != node->m_buttonMap.end(); it++ )
		{
			if( it->second == _data[3] )
				break;
		}
		if( it != node->m_buttonMap.end() )
		{
			Notification *notification;
			if( _data[8] == 0 )
			{
				notification = new Notification( Notification::Type_ButtonOff );
			}
			else
			{
				notification = new Notification( Notification::Type_ButtonOn );
			}
			notification->SetHomeAndNodeIds( m_homeId, _data[4] );
			notification->SetButtonId( it->first );
			QueueNotification( notification );
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::NodeFromMessage>
// See if we can get node from incoming message data
//-----------------------------------------------------------------------------
uint8 Driver::NodeFromMessage
(
	uint8 const* buffer
)
{
	uint8 nodeId = 0;

	if( buffer[1] >= 5 )
	{
		switch( buffer[3] )
		{
			case FUNC_ID_APPLICATION_COMMAND_HANDLER:		nodeId = buffer[5];	break;
			case FUNC_ID_ZW_APPLICATION_UPDATE:			nodeId = buffer[5];	break;
		}
	}
	return nodeId;
}
//-----------------------------------------------------------------------------
// <Driver::GetDriverStatistics>
// Return driver statistics
//-----------------------------------------------------------------------------
void Driver::GetDriverStatistics
(
	DriverData* _data
)
{
	_data->s_SOFCnt = m_SOFCnt;
	_data->s_ACKWaiting = m_ACKWaiting;
	_data->s_readAborts = m_readAborts;
	_data->s_badChecksum = m_badChecksum;
	_data->s_readCnt = m_readCnt;
	_data->s_writeCnt = m_writeCnt;
	_data->s_CANCnt = m_CANCnt;
	_data->s_NAKCnt = m_NAKCnt;
	_data->s_ACKCnt = m_ACKCnt;
	_data->s_OOFCnt = m_OOFCnt;
	_data->s_dropped = m_dropped;
	_data->s_retries = m_retries;
	_data->s_controllerReadCnt = m_controllerReadCnt;
	_data->s_controllerWriteCnt = m_controllerWriteCnt;
}

//-----------------------------------------------------------------------------
// <Driver::LogDriverStatistics>
// Report driver statistics to the driver's log
//-----------------------------------------------------------------------------
void Driver::LogDriverStatistics
(
)
{
	DriverData data;

	GetDriverStatistics(&data);
	int32 totalElapsed = -m_startTime.TimeRemaining();
	int32 days = totalElapsed / (1000*60*60*24);

	totalElapsed -= days*1000*60*60*24;
	int32 hours = totalElapsed/(1000*60*60);

	totalElapsed -= hours*1000*60*60;
	int32 minutes = totalElapsed/(1000*60);

	Log::Write( LogLevel_Always, "***************************************************************************" );
	Log::Write( LogLevel_Always, "*********************  Cumulative Network Statistics  *********************" );
	Log::Write( LogLevel_Always, "*** General" );
	Log::Write( LogLevel_Always, "Driver run time: . .  . %ld days, %ld hours, %ld minutes", days, hours, minutes);
	Log::Write( LogLevel_Always, "Frames processed: . . . . . . . . . . . . . . . . . . . . %ld", data.s_SOFCnt );
	Log::Write( LogLevel_Always, "[Device] Messages successfully received:  . . . . . . . . %ld", data.s_readCnt );
	Log::Write( LogLevel_Always, "[Device] Messages successfully sent:  . . . . . . . . . . %ld", data.s_writeCnt );
	Log::Write( LogLevel_Always, "ACKs received from controller:  . . . . . . . . . . . . . %ld", data.s_ACKCnt );
	Log::Write( LogLevel_Always, "Controller messages received: . . . . . . . . . . . . . . %ld", data.s_controllerReadCnt );
	Log::Write( LogLevel_Always, "Controller messages sent: . . . . . . . . . . . . . . . . %ld", data.s_controllerWriteCnt );
	// Consider tracking and adding:
	//		Initialization messages
	//		Ad-hoc command messages
	//		Polling messages
	//		Messages inititated by network
	//		Others?
	Log::Write( LogLevel_Always, "*** Errors" );
	Log::Write( LogLevel_Always, "Unsolicited messages received while waiting for ACK:  . . %ld", data.s_ACKWaiting );
	Log::Write( LogLevel_Always, "Reads aborted due to timeouts:  . . . . . . . . . . . . . %ld", data.s_readAborts );
	Log::Write( LogLevel_Always, "Bad checksum errors:  . . . . . . . . . . . . . . . . . . %ld", data.s_badChecksum );
	Log::Write( LogLevel_Always, "CANs received from controller:  . . . . . . . . . . . . . %ld", data.s_CANCnt );
	Log::Write( LogLevel_Always, "NAKs received from controller:  . . . . . . . . . . . . . %ld", data.s_NAKCnt );
	Log::Write( LogLevel_Always, "Out of frame data flow errors:  . . . . . . . . . . . . . %ld", data.s_OOFCnt );
	Log::Write( LogLevel_Always, "Messages retransmitted: . . . . . . . . . . . . . . . . . %ld", data.s_retries );
	Log::Write( LogLevel_Always, "Messages dropped and not delivered: . . . . . . . . . . . %ld", data.s_dropped );
	Log::Write( LogLevel_Always, "***************************************************************************" );
}
