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
	class Value;

	class ValueStore
	{
	public:
		
		typedef map<ValueID,Value*>::const_iterator Iterator;

		Iterator Begin(){ return m_values.begin(); }
		Iterator End(){ return m_values.begin(); }
		
		ValueStore(){}
		~ValueStore();

		bool AddValue( Value* _pValue );
		bool RemoveValue( ValueID const& _id );
		Value* GetValue( ValueID const& _id )const;

	private:
		map<ValueID,Value*>	m_values;
	};

} // namespace OpenZWave

#endif



