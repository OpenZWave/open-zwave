//-----------------------------------------------------------------------------
//
//	ValueStore.cpp
//
//	Container for Value objects
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

#include "ValueStore.h"
#include "Value.h"
#include "Manager.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueStore::ValueStore>
// Destructor
//-----------------------------------------------------------------------------
ValueStore::~ValueStore
(
)
{
	map<ValueID,Value*>::iterator it = m_values.begin();
	while( !m_values.empty() )
	{
		it->second->Release();
		m_values.erase( it );
		it = m_values.begin();
	}
}

//-----------------------------------------------------------------------------
// <ValueStore::AddValue>
// Add a value to the store
//-----------------------------------------------------------------------------
bool ValueStore::AddValue
(
	Value* _value
)
{
	if( !_value )
	{
		return false;
	}

	map<ValueID,Value*>::iterator it = m_values.find( _value->GetID() );
	if( it != m_values.end() )
	{
		// There is already a value in the store with this ID, so we give up.
		return false;
	}

	m_values[_value->GetID()] = _value;
	_value->AddRef();

	// Notify the watchers of the new value
	Manager::Notification* notification = new Manager::Notification();		
	notification->m_type = Manager::NotificationType_ValueAdded;
	notification->m_id = _value->GetID();
	notification->m_groupIdx = 0;
	Manager::Get()->NotifyWatchers( notification ); 
	delete notification;

	return true;
}

//-----------------------------------------------------------------------------
// <ValueStore::RemoveValue>
// Remove a value from the store
//-----------------------------------------------------------------------------
bool ValueStore::RemoveValue
(
	ValueID const& _id
)
{
	map<ValueID,Value*>::iterator it = m_values.find( _id );
	if( it != m_values.end() )
	{
		// Value found, so remove it
		it->second->Release();
		m_values.erase( it );

		// Notify the watchers of the new value
		Manager::Notification* notification = new Manager::Notification();	
		notification->m_type = Manager::NotificationType_ValueRemoved;
		notification->m_id = it->second->GetID();
		notification->m_groupIdx = 0;
		Manager::Get()->NotifyWatchers( notification ); 
		delete notification;

		return true;
	}

	// Value not found in the store
	return false;
}

//-----------------------------------------------------------------------------
// <ValueStore::GetValue>
// Get a value from the store
//-----------------------------------------------------------------------------
Value* ValueStore::GetValue
(
	ValueID const& _id
)const
{
	Value* value = NULL;

	map<ValueID,Value*>::const_iterator it = m_values.find( _id );
	if( it != m_values.end() )
	{
		value = it->second;
		if( value )
		{
			// Add a reference to the value.  The caller must
			// call Release on the value when they are done with it.
			value->AddRef();
		}
	}

	return value;
}
