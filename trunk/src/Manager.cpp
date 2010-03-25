//-----------------------------------------------------------------------------
//
//	Manager.h
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
#include "Manager.h"
#include "Driver.h"
#include "Node.h"

#include "Event.h"
#include "Log.h"

#include "CommandClasses.h"

#include "ValueID.h"
#include "ValueBool.h"
#include "ValueByte.h"
#include "ValueDecimal.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueShort.h"
#include "ValueString.h"

using namespace OpenZWave;

Manager* Manager::s_instance = NULL;


//-----------------------------------------------------------------------------
//	Construction
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	<Manager::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
Manager* Manager::Create
(
	string const& _configPath
)
{
	if( NULL == s_instance )
	{
		s_instance = new Manager( _configPath );
	}

	return s_instance;
}

//-----------------------------------------------------------------------------
//	<Manager::Destroy>
//	Static method to destroy the singleton.
//-----------------------------------------------------------------------------
void Manager::Destroy
(
)
{
	delete s_instance;
	s_instance = NULL;
}

//-----------------------------------------------------------------------------
// <Manager::Manager>
// Constructor
//-----------------------------------------------------------------------------
Manager::Manager
( 
	string const& _configPath
):
	m_configPath( _configPath ),
	m_exitEvent( new Event() )
{
	// Clear the drivers array
	memset( m_drivers, 0, sizeof(Driver*)*MaxDrivers );

	// Create the log file
	Log::Create( "OZW_Log.txt" );

	CommandClasses::RegisterCommandClasses();

	// Ensure the singleton instance is set
	s_instance = this;
}

//-----------------------------------------------------------------------------
// <Manager::Manager>
// Destructor
//-----------------------------------------------------------------------------
Manager::~Manager
(
)
{
	for( int i=0; i<MaxDrivers; ++i )
	{
		if( m_drivers[i] )
		{
			delete m_drivers[i];
		}
	}
}

//-----------------------------------------------------------------------------
//	Drivers
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::AddDriver>
// Add a new Z-Wave PC Interface
//-----------------------------------------------------------------------------
bool Manager::AddDriver
(
	string const& _serialPortName,
	uint8* o_driverId
)
{
	if( NULL == o_driverId )
	{
		Log::Write( "Add Driver failed - o_driverId pointer is NULL" );
		return false;
	}

	// Make sure we don't already have a driver for this serial port
	int firstEmptySlot = -1;
	for( int i=0; i<MaxDrivers; ++i )
	{
		if( Driver* driver = m_drivers[i] )
		{
			if( driver->GetSerialPortName() == _serialPortName )
			{
				Log::Write( "Add Driver failed - Serial Port '%s' already has a driver", _serialPortName.c_str() );
				return false;
			}
		}
		else if( firstEmptySlot < 0 )
		{
			firstEmptySlot = i;
		}
	}

	if( firstEmptySlot < 0 )
	{
		Log::Write( "Add Driver failed - Maximum number of drivers (%d) already added", MaxDrivers );
		return false;
	}

	m_drivers[firstEmptySlot] = new Driver( _serialPortName, (uint8)firstEmptySlot );
	*o_driverId = (uint8)firstEmptySlot;

	return true;
}

//-----------------------------------------------------------------------------
// <Manager::AddDriver>
// Add a new Z-Wave PC Interface
//-----------------------------------------------------------------------------
bool Manager::RemoveDriver
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		delete driver;
		m_drivers[_driverId] = NULL;
		return true;
	}

	Log::Write( "RemoveDriver() failed - Driver %d not found", _driverId );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddDriver>
// Add a new Z-Wave PC Interface
//-----------------------------------------------------------------------------
Driver* Manager::GetDriver
(
	uint8 const _driverId
)
{
	if( _driverId >= MaxDrivers )
	{
		assert(0);
		Log::Write( "GetDriver() failed - _driverId %d is out of range (must be between 0 and %d)", _driverId, MaxDrivers-1 );
		return NULL;
	}

	return m_drivers[_driverId];
}

//-----------------------------------------------------------------------------
// <Manager::IsSlave>
// 
//-----------------------------------------------------------------------------
bool Manager::IsSlave
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		return driver->IsSlave();
	}

	Log::Write( "IsSlave() failed - _driverId %d not found", _driverId );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::HasTimerSupport>
// 
//-----------------------------------------------------------------------------
bool Manager::HasTimerSupport
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		return driver->HasTimerSupport();
	}

	Log::Write( "HasTimerSupport() failed - _driverId %d not found", _driverId );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::IsPrimaryController>
// 
//-----------------------------------------------------------------------------
bool Manager::IsPrimaryController
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		return driver->IsPrimaryController();
	}

	Log::Write( "IsPrimaryController() failed - _driverId %d not found", _driverId );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::IsStaticUpdateController>
// 
//-----------------------------------------------------------------------------
bool Manager::IsStaticUpdateController
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		return driver->IsStaticUpdateController();
	}

	Log::Write( "IsStaticUpdateController() failed - _driverId %d not found", _driverId );
	return false;
}

//-----------------------------------------------------------------------------
//	Polling Z-Wave values
//-----------------------------------------------------------------------------
				  		
//-----------------------------------------------------------------------------
// <Manager::SetPollInterval>
// Set the polling interval on all drivers
//-----------------------------------------------------------------------------
void Manager::SetPollInterval
(
	int32 _seconds
)
{
	for( int i=0; i<MaxDrivers; ++i )
	{
		if( Driver* driver = m_drivers[i] )
		{
			driver->SetPollInterval( _seconds );
		}
	}
}

//-----------------------------------------------------------------------------
// <Manager::EnablePoll>
// Enable polling of a value
//-----------------------------------------------------------------------------
bool Manager::EnablePoll
( 
	ValueID const& _id 
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return( driver->EnablePoll( _id ) );
	}

	// No driver found for this ValueID
	Log::Write( "EnablePoll failed - Driver for this value no longer exists" );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::DisablePoll>
// Disable polling of a value
//-----------------------------------------------------------------------------
bool Manager::DisablePoll
( 
	ValueID const& _id 
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return( driver->DisablePoll( _id ) );
	}

	// No driver found for this ValueID
	Log::Write( "DisablePoll failed - Driver for this value no longer exists" );
	return false;
}

//-----------------------------------------------------------------------------
//	Retrieving Node information
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::GetValueBool>
// Get the bool value object with the specified ID
//-----------------------------------------------------------------------------
ValueBool* Manager::GetValueBool
(
	ValueID const& _id
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return driver->GetValueBool( _id );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueByte>
// Get the byte value object with the specified ID
//-----------------------------------------------------------------------------
ValueByte* Manager::GetValueByte
(
	ValueID const& _id
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return driver->GetValueByte( _id );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueDecimal>
// Get the decimal value object with the specified ID
//-----------------------------------------------------------------------------
ValueDecimal* Manager::GetValueDecimal
(
	ValueID const& _id
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return driver->GetValueDecimal( _id );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueInt>
// Get the int value object with the specified ID
//-----------------------------------------------------------------------------
ValueInt* Manager::GetValueInt
(
	ValueID const& _id
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return driver->GetValueInt( _id );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueList>
// Get the list value object with the specified ID
//-----------------------------------------------------------------------------
ValueList* Manager::GetValueList
(
	ValueID const& _id
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return driver->GetValueList( _id );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueShort>
// Get the short value object with the specified ID
//-----------------------------------------------------------------------------
ValueShort* Manager::GetValueShort
(
	ValueID const& _id
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return driver->GetValueShort( _id );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueString>
// Get the string value object with the specified ID
//-----------------------------------------------------------------------------
ValueString* Manager::GetValueString
(
	ValueID const& _id
)
{
	if( Driver* driver = m_drivers[_id.GetDriverId()] )
	{
		return driver->GetValueString( _id );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//	Notifications
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::AddWatcher>
// Add a watcher to the list
//-----------------------------------------------------------------------------
bool Manager::AddWatcher
(
	pfnOnNotification_t _pWatcher,
	void* _context
)
{
	// Ensure this watcher is not already on the list
	for( list<Watcher*>::iterator it = m_watchers.begin(); it != m_watchers.end(); ++it )
	{
		if( ((*it)->m_callback == _pWatcher ) && ( (*it)->m_context == _context ) )
		{
			// Already in the list
			return false;
		}
	}

	m_watchers.push_back( new Watcher( _pWatcher, _context ) );
	return true;
}

//-----------------------------------------------------------------------------
// <Manager::RemoveWatcher>
// Remove a watcher from the list
//-----------------------------------------------------------------------------
bool Manager::RemoveWatcher
(
	pfnOnNotification_t _pWatcher,
	void* _context
)
{
	list<Watcher*>::iterator it = m_watchers.begin();
	while( it != m_watchers.end() )
	{
		if( ((*it)->m_callback == _pWatcher ) && ( (*it)->m_context == _context ) )
		{
			delete (*it);
			m_watchers.erase( it );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Manager::NotifyWatchers>
// Notify any watching objects of a value change
//-----------------------------------------------------------------------------
void Manager::NotifyWatchers
(
	Notification const* _notification
)
{
	for( list<Watcher*>::iterator it = m_watchers.begin(); it != m_watchers.end(); ++it )
	{
		Watcher* pWatcher = *it;
		pWatcher->m_callback( _notification, pWatcher->m_context );
	}
}


//-----------------------------------------------------------------------------
//	Controller commands
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::ResetController>
// Reset controller and erase all node information
//-----------------------------------------------------------------------------
void Manager::ResetController
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->ResetController();
	}
}

//-----------------------------------------------------------------------------
// <Manager::SoftReset>
// Soft-reset the Z-Wave controller chip
//-----------------------------------------------------------------------------
void Manager::SoftReset
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->SoftReset();
	}
}

//-----------------------------------------------------------------------------
// <Manager::RequestNodeNeighborUpdate>
// 
//-----------------------------------------------------------------------------
void Manager::RequestNodeNeighborUpdate
(
	uint8 const _driverId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->RequestNodeNeighborUpdate( _nodeId );
	}
}

//-----------------------------------------------------------------------------
// <Manager::AssignReturnRoute>
// 
//-----------------------------------------------------------------------------
void Manager::AssignReturnRoute
(
	uint8 const _driverId,
	uint8 const _nodeId,
	uint8 const _targetNodeId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->AssignReturnRoute( _nodeId, _targetNodeId );
	}
} 

//-----------------------------------------------------------------------------
// <Manager::BeginAddNode>
// Set the controller into AddNode mode
//-----------------------------------------------------------------------------
void Manager::BeginAddNode
(
	uint8 const _driverId,
	bool const _highPower // = false
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->BeginAddNode( _highPower );
	}
}

//-----------------------------------------------------------------------------
// <Manager::BeginAddController>
// Set the controller into AddController mode
//-----------------------------------------------------------------------------
void Manager::BeginAddController
(
	uint8 const _driverId,
	bool const _highPower // = false
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->BeginAddController( _highPower );
	}
}

//-----------------------------------------------------------------------------
// <Manager::EndAddNode>
// Take the controller out of AddNode mode
//-----------------------------------------------------------------------------
void Manager::EndAddNode
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->EndAddNode();
	}
}

//-----------------------------------------------------------------------------
// <Manager::BeginRemoveNode>
// Set the controller into RemoveNode mode
//-----------------------------------------------------------------------------
void Manager::BeginRemoveNode
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->BeginRemoveNode();
	}
}

//-----------------------------------------------------------------------------
// <Manager::EndRemoveNode>
// Take the controller out of RemoveNode mode
//-----------------------------------------------------------------------------
void Manager::EndRemoveNode
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->EndRemoveNode();
	}
}

//-----------------------------------------------------------------------------
// <Manager::BeginReplicateController>
// Set the controller into ReplicateController mode
//-----------------------------------------------------------------------------
void Manager::BeginReplicateController
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->BeginReplicateController();
	}
}

//-----------------------------------------------------------------------------
// <Manager::EndReplicateController>
// Take the controller out of ReplicateController mode
//-----------------------------------------------------------------------------
void Manager::EndReplicateController
(
	uint8 const _driverId
)
{
	if( Driver* driver = GetDriver( _driverId ) )
	{
		driver->EndReplicateController();
	}
}

