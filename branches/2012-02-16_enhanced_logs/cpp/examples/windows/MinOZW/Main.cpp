//-----------------------------------------------------------------------------
//
//	Main.cpp
//
//	Minimal application to test OpenZWave.
//
//	Creates an OpenZWave::Driver and the waits.  In Debug builds
//	you should see verbose logging to the console, which will
//	indicate that communications with the Z-Wave network are working.
//
//	Copyright (c) 2010 Mal Lansell <mal@openzwave.com>
//
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

#include "Windows.h"
#include "Options.h"
#include "Manager.h"
#include "Node.h"
#include "Group.h"
#include "Notification.h"
#include "ValueStore.h"
#include "Value.h"
#include "ValueBool.h"

using namespace OpenZWave;

static uint32 g_homeId = 0;
bool g_nodesQueried = false;

typedef struct
{
	uint32			m_homeId;
	uint8			m_nodeId;
	bool			m_polled;
	list<ValueID>	m_values;
}NodeInfo;

static list<NodeInfo*> g_nodes;
static CRITICAL_SECTION g_criticalSection;

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this notification
//-----------------------------------------------------------------------------
NodeInfo* GetNodeInfo
(
	Notification const* _notification
)
{
	uint32 const homeId = _notification->GetHomeId();
	uint8 const nodeId = _notification->GetNodeId();
	for( list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it )
	{
		NodeInfo* nodeInfo = *it;
		if( ( nodeInfo->m_homeId == homeId ) && ( nodeInfo->m_nodeId == nodeId ) )
		{
			return nodeInfo;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void OnNotification
(
	Notification const* _notification,
	void* _context
)
{
	// Must do this inside a critical section to avoid conflicts with the main thread
	EnterCriticalSection( &g_criticalSection );

	switch( _notification->GetType() )
	{
		case Notification::Type_AllNodesQueried:
		{
			printf( "Notificaton:  AllNodesQueried\n" );
			g_nodesQueried = true;
			break;
		}
		case Notification::Type_AwakeNodesQueried:
		{
			printf( "Notificaton:  AwakeNodesQueried\n" );
			g_nodesQueried = true;
			break;
		}
		case Notification::Type_DriverReady:
		{
			printf( "Notification:  DriverReady\n" );
			g_homeId = _notification->GetHomeId();
			break;
		}
		case Notification::Type_DriverReset:
		{
			printf( "Notification:  DriverReset\n" );
			break;
		}
		case Notification::Type_Group:
		{
			printf( "Notification:  Group\n" );

			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// One of the node's association groups has changed
				// TBD...
			}
			break;
		}
		case Notification::Type_MsgComplete:
		{
			printf( "Notificaton:  MsgComplete\n" );
			break;
		}
		case Notification::Type_NodeAdded:
		{
			printf( "Notification:  NodeAdded\n" );
			// Add the new node to our list
			NodeInfo* nodeInfo = new NodeInfo();
			nodeInfo->m_homeId = _notification->GetHomeId();
			nodeInfo->m_nodeId = _notification->GetNodeId();
			nodeInfo->m_polled = false;		
			g_nodes.push_back( nodeInfo );
			break;
		}
		case Notification::Type_NodeEvent:
		{
			printf( "Notification:  Event\n" );
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// We have received an event from the node, caused by a
				// basic_set or hail message.
				// TBD...
			}
			break;
		}
		case Notification::Type_NodeNaming:
		{
			printf( "Notification:  NodeNaming\n" );
			break;
		}
		case Notification::Type_NodeProtocolInfo:
		{
			printf( "Notification:  NodeProtocolInfo\n" );
			break;
		}
		case Notification::Type_NodeQueriesComplete:
		{
			printf( "Notification:  NodeQueriesComplete\n" );
			break;
		}
		case Notification::Type_NodeRemoved:
		{
			printf( "Notification:  NodeRemoved\n" );
			// Remove the node from our list
			uint32 const homeId = _notification->GetHomeId();
			uint8 const nodeId = _notification->GetNodeId();
			for( list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it )
			{
				NodeInfo* nodeInfo = *it;
				if( ( nodeInfo->m_homeId == homeId ) && ( nodeInfo->m_nodeId == nodeId ) )
				{
					g_nodes.erase( it );
					delete nodeInfo;
					break;
				}
			}
			break;
		}
		case Notification::Type_PollingDisabled:
		{
			printf( "Notification:  PollingDisabled\n" );
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = false;
			}
			break;
		}
		case Notification::Type_PollingEnabled:
		{
			printf( "Notification:  PollingEnabled\n" );
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = true;
			}
			break;
		}
		case Notification::Type_ValueAdded:
		{
			printf( "\nNotification:  ValueAdded" );
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// Add the new value to our list
				nodeInfo->m_values.push_back( _notification->GetValueID() );
			}
			break;
		}
		case Notification::Type_ValueChanged:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				printf( "\nValue changed." );
				printf( "\nHome ID: 0x%.8x  Node ID: %d,  Polled: %s", nodeInfo->m_homeId, nodeInfo->m_nodeId, nodeInfo->m_polled?"true":"false" );
				ValueID valueid = _notification->GetValueID();
				printf( "\nValue is: \n part of command class: 0x%.2x\n of genre: %d\n with index %d\n and type %d", 
					valueid.GetCommandClassId(), valueid.GetGenre(), valueid.GetIndex(), valueid.GetInstance(), valueid.GetType() );
				switch( valueid.GetType() )
				{
				case ValueID::ValueType_Bool:
					bool bTestBool;
					Manager::Get()->GetValueAsBool( valueid, &bTestBool );
					printf( "\nValue is: %s", bTestBool?"true":"false" );
					break;
/*				case ValueID::ValueType_Button:
					printf( "\nButton value not implemented" );
					break;
*/
				case ValueID::ValueType_Byte:
					uint8 bTestByte;
					Manager::Get()->GetValueAsByte( valueid, &bTestByte );
					printf( "\nValue is: 0x%.2x", bTestByte );
					break;
				case ValueID::ValueType_Decimal:
					float bTestFloat;
					Manager::Get()->GetValueAsFloat( valueid, &bTestFloat );
					printf( "\nValue is: %.2f", bTestFloat );
					break;
				case ValueID::ValueType_Int:
					int32 bTestInt;
					Manager::Get()->GetValueAsInt( valueid, &bTestInt );
					printf( "\nValue is: %d", bTestInt );
					break;
				case ValueID::ValueType_List:
				case ValueID::ValueType_Max:
				case ValueID::ValueType_Schedule:
				case ValueID::ValueType_Short:
					int16 bTestShort;
					Manager::Get()->GetValueAsShort( valueid, &bTestShort );
					printf( "\nValue is: %d", bTestShort );
					break;
				case ValueID::ValueType_String:
					string bTestString;
					Manager::Get()->GetValueAsString( valueid, &bTestString );
					printf( "\nValue is: %s", bTestString.c_str() );
					break;
					break;
				}
			}
			else
			{
				// ValueChanged notification for a node that doesn't appear to exist in our g_nodes list
				printf( "\nERROR: Value changed notification for an unidentified node." );
			}
			break;
		}
		case Notification::Type_ValueRemoved:
		{
			printf( "\nNotification:  ValueRemoved" );
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// Remove the value from out list
				for( list<ValueID>::iterator it = nodeInfo->m_values.begin(); it != nodeInfo->m_values.end(); ++it )
				{
					if( (*it) == _notification->GetValueID() )
					{
						nodeInfo->m_values.erase( it );
						break;
					}
				}
			}
			break;
		}
	}

	LeaveCriticalSection( &g_criticalSection );
}

//-----------------------------------------------------------------------------
// <main>
// Create the driver and then wait
//-----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	InitializeCriticalSection( &g_criticalSection );

	// Create the OpenZWave Manager.
	// The first argument is the path to the config files (where the manufacturer_specific.xml file is located
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL 
	// the log file will appear in the program's working directory.
	Options::Create( "../../../../../config/", "", "" );
	Options::Get()->Lock();

	Manager::Create();

	// Add a callback handler to the manager.  The second argument is a context that
	// is passed to the OnNotification method.  If the OnNotification is a method of
	// a class, the context would usually be a pointer to that class object, to
	// avoid the need for the notification handler to be a static.
	Manager::Get()->AddWatcher( OnNotification, NULL );

	// Add a Z-Wave Driver
	// Modify this line to set the correct serial port for your PC interface.
	Manager::Get()->AddDriver( "\\\\.\\COM3" );
  //Manager::Get()->AddDriver( "HID Controller", Driver::ControllerInterface_Hid );

	// Now we just wait for the driver to become ready.
	// In a normal app, we would be handling notifications and building a UI for the user.
	while( !g_homeId )
	{
		Sleep( 1000 );
	}

	// Since the configuration file contains command class information that is only 
	// known after the nodes on the network are queried, wait until all of the nodes 
	// on the network have been queried (at least the "listening" ones) before
	// writing the configuration file.  (Maybe write again after sleeping nodes have
	// been queried as well.)
	while( !g_nodesQueried )
	{
		Sleep( 1000 );
	}
	Manager::Get()->WriteConfig( g_homeId );
	
	// If we want to access our NodeInfo list, that has been built from all the
	// notification callbacks we received from the library, we have to do so
	// from inside a Critical Section.  This is because the callbacks occur on other 
	// threads, and we cannot risk the list being changed while we are using it.  
	// We must hold the critical section for as short a time as possible, to avoid
	// stalling the OpenZWave drivers.
	while( true )
	{
		EnterCriticalSection( &g_criticalSection );
		// Do stuff
		Sleep(6000);
		LeaveCriticalSection( &g_criticalSection );
	}

	// program exit (clean up)
	Manager::Destroy();
	Options::Destroy();
	DeleteCriticalSection( &g_criticalSection );
	return 0;
}
