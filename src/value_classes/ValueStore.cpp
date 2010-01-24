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

#include "ValueStore.h"
#include "Value.h"
#include "Mutex.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueStore::ValueStore>
// Constructor
//-----------------------------------------------------------------------------
ValueStore::ValueStore
(
)
{
	m_pMutex = new Mutex();
}

//-----------------------------------------------------------------------------
// <ValueStore::ValueStore>
// Destructor
//-----------------------------------------------------------------------------
ValueStore::~ValueStore
(
)
{
	delete m_pMutex;

	map<ValueID,Value*>::iterator it = m_values.begin();
	while( it != m_values.end() )
	{
		it->second->Release();
		it = m_values.erase( it );
	}
}

//-----------------------------------------------------------------------------
// <ValueStore::AddValue>
// Add a value to the store
//-----------------------------------------------------------------------------
bool ValueStore::AddValue
(
	Value* _pValue
)
{
	if( !_pValue )
	{
		return false;
	}

	bool bRes = false;

	m_pMutex->Lock();

	map<ValueID,Value*>::iterator it = m_values.find( _pValue->GetID() );
	if( it == m_values.end() )
	{
		m_values[_pValue->GetID()] = _pValue;
		_pValue->AddRef();
		bRes = true;
	}

	m_pMutex->Release();
	return bRes;
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
	bool bRes = false;

	m_pMutex->Lock();

	map<ValueID,Value*>::iterator it = m_values.find( _id );
	if( it != m_values.end() )
	{
		Value* pValue = it->second;
		if( pValue )
		{
			pValue->Release();
		}
		m_values.erase( it );
		bRes = true;
	}

	m_pMutex->Release();
	return bRes;
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
	Value* pValue = NULL;

	m_pMutex->Lock();

	map<ValueID,Value*>::const_iterator it = m_values.find( _id );
	if( it != m_values.end() )
	{
		pValue = it->second;
		if( pValue )
		{
			pValue->AddRef();
		}
	}

	m_pMutex->Release();

	return pValue;
}

//-----------------------------------------------------------------------------
// <ValueStore::Iterator::Iterator>
// Iterator constructor
//-----------------------------------------------------------------------------
ValueStore::Iterator::Iterator
(
	ValueStore* _pStore
):
	m_pStore( _pStore )
{
	// Prevent modifications to the map while we are iterating
	m_pStore->m_pMutex->Lock();
}

//-----------------------------------------------------------------------------
// <ValueStore::Iterator::~Iterator>
// Iterator destructor
//-----------------------------------------------------------------------------
ValueStore::Iterator::~Iterator
(
)
{
	// We're done iterating, so modifications to the map are now ok
	m_pStore->m_pMutex->Release();
}

