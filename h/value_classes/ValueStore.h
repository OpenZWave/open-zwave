//-----------------------------------------------------------------------------
//
//	ValueStore.h
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

#ifndef _ValueStore_H
#define _ValueStore_H

#include <map>
#include "Defs.h"
#include "ValueID.h"

namespace OpenZWave
{
	class Mutex;
	class Value;

	class ValueStore
	{
	public:
		// Custom iterator for the value store, which includes automatic serialisation of
		// access to prevent values being added or removed while iterating.
		class Iterator 
		{
		public:
			Iterator( ValueStore* _pStore );
			~Iterator();

			Iterator& operator = (const Iterator& _other){ m_it = _other.m_it; return( *this ); }
			bool operator == ( const Iterator& _other ){ return( m_it == _other.m_it ); }
			bool operator != ( const Iterator& _other ){ return( m_it != _other.m_it ); }
			Iterator& operator++(){ ++m_it; return( *this ); }
			Iterator& operator++(int){ m_it++; return( *this ); }

			ValueID const& operator*()
			{
				return( m_it->first );
			}

		private:
			map<ValueID,Value*>::iterator	m_it;
			ValueStore*						m_pStore;
		};

		ValueStore();
		~ValueStore();

		bool AddValue( Value* _pValue );
		bool RemoveValue( ValueID const& _id );
		Value* GetValue( ValueID const& _id )const;

	private:
		map<ValueID,Value*>	m_values;
		Mutex*				m_pMutex;
	};

} // namespace OpenZWave

#endif



