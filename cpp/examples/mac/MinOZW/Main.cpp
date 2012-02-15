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

#include <unistd.h>
#include <pthread.h>
#include "Options.h"
#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "Notification.h"
#include "ValueStore.h"
#include "Value.h"
#include "ValueBool.h"

using namespace OpenZWave;

static uint32 g_homeId = 0;
static bool   g_initFailed = false;

typedef struct
{
	uint32			m_homeId;
	uint8			m_nodeId;
	bool			m_polled;
	list<ValueID>	m_values;
}NodeInfo;

static list<NodeInfo*> g_nodes;
static pthread_mutex_t g_criticalSection;
static pthread_cond_t  initCond  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;

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
	pthread_mutex_lock( &g_criticalSection );

	switch( _notification->GetType() )
	{
		case Notification::Type_ValueAdded:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// Add the new value to our list
				nodeInfo->m_values.push_back( _notification->GetValueID() );
			}
			break;
		}

		case Notification::Type_ValueRemoved:
		{
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

		case Notification::Type_ValueChanged:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// One of the node values has changed
				// TBD...
				nodeInfo = nodeInfo;
			}
			break;
		}

		case Notification::Type_Group:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// One of the node's association groups has changed
				// TBD...
				nodeInfo = nodeInfo;
			}
			break;
		}

		case Notification::Type_NodeAdded:
		{
			// Add the new node to our list
			NodeInfo* nodeInfo = new NodeInfo();
			nodeInfo->m_homeId = _notification->GetHomeId();
			nodeInfo->m_nodeId = _notification->GetNodeId();
			nodeInfo->m_polled = false;		
			g_nodes.push_back( nodeInfo );
			break;
		}

		case Notification::Type_NodeRemoved:
		{
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

		case Notification::Type_NodeEvent:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// We have received an event from the node, caused by a
				// basic_set or hail message.
				// TBD...
				nodeInfo = nodeInfo;
			}
			break;
		}

		case Notification::Type_PollingDisabled:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = false;
			}
			break;
		}

		case Notification::Type_PollingEnabled:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = true;
			}
			break;
		}

		case Notification::Type_DriverReady:
		{
			g_homeId = _notification->GetHomeId();
			break;
		}


		case Notification::Type_DriverFailed:
		{
			g_initFailed = true;
			pthread_cond_broadcast(&initCond);
			break;
		}

		case Notification::Type_AwakeNodesQueried:
		case Notification::Type_AllNodesQueried:
		{
				pthread_cond_broadcast(&initCond);
				break;
		}

		default:
		{
		}
	}

	pthread_mutex_unlock( &g_criticalSection );
}
//-----------------------------------------------------------------------------
// <main>
// Create the driver and then wait
//-----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	pthread_mutexattr_t mutexattr;

	pthread_mutexattr_init ( &mutexattr );
	pthread_mutexattr_settype( &mutexattr, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init( &g_criticalSection, &mutexattr );
	pthread_mutexattr_destroy( &mutexattr );

	pthread_mutex_lock( &initMutex );

	// Create the OpenZWave Manager.
	// The first argument is the path to the config files (where the manufacturer_specific.xml file is located
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL 
	// the log file will appear in the program's working directory.
	Options::Create( "../../../../config/", "", "" );
	Options::Get()->Lock();

	Manager::Create();

	// Add a callback handler to the manager.  The second argument is a context that
	// is passed to the OnNotification method.  If the OnNotification is a method of
	// a class, the context would usually be a pointer to that class object, to
	// avoid the need for the notification handler to be a static.
	Manager::Get()->AddWatcher( OnNotification, NULL );

	// Add a Z-Wave Driver
	// Modify this line to set the correct serial port for your PC interface.

	string port = "/dev/cu.usbserial";

	Manager::Get()->AddDriver( ( argc > 1 ) ? argv[1] : port );
	//Manager::Get()->AddDriver( "HID Controller", Driver::ControllerInterface_Hid );

	// Now we just wait for the driver to become ready, and then write out the loaded config.
	// In a normal app, we would be handling notifications and building a UI for the user.
	pthread_cond_wait( &initCond, &initMutex );

	if( !g_initFailed )
	{

		//Manager::Get()->BeginAddNode( g_homeId );
		//sleep(10);
		//Manager::Get()->EndAddNode( g_homeId );
		//Manager::Get()->BeginRemoveNode( g_homeId );
		//sleep(10);
		//Manager::Get()->EndRemoveNode( g_homeId );
		//sleep(10);

		Manager::Get()->WriteConfig( g_homeId );

		Driver::DriverData data;
		Manager::Get()->GetDriverStatistics( g_homeId, &data );
		printf("SOF: %d ACK Waiting: %d Read Aborts: %d Bad Checksums: %d\n", data.s_SOFCnt, data.s_ACKWaiting, data.s_readAborts, data.s_badChecksum);
		printf("Reads: %d Writes: %d CAN: %d NAK: %d ACK: %d Out of Frame: %d\n", data.s_readCnt, data.s_writeCnt, data.s_CANCnt, data.s_NAKCnt, data.s_ACKCnt, data.s_OOFCnt);
		printf("Dropped: %d Retries: %d\n", data.s_dropped, data.s_retries);

		while( true )
		{
			sleep(10);

			pthread_mutex_lock( &g_criticalSection );
			// Do stuff
			pthread_mutex_unlock( &g_criticalSection );
			sleep(5);
		}
	}

	// program exit (clean up)
	Manager::Destroy();
	Options::Destroy();
	pthread_mutex_destroy( &g_criticalSection );
	return 0;
}
