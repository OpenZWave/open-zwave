//-----------------------------------------------------------------------------
//
//	ValueList.h
//
//	Base class for all OpenZWave Value Classes
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

#ifndef _ValueList_H
#define _ValueList_H

#include <string>
#include <vector>
#include "Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	class ValueList: public Value
	{
	public:
		struct Item
		{
			string	m_label;
			int32	m_value;
		};

		ValueList( uint8 const _nodeId, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, uint32 const _genre, string const& _label, bool const _bReadOnly, vector<Item> const& _items, int32 const _valueIdx );
		ValueList( uint8 const _nodeId, TiXmlElement* _valueElement );
		virtual ~ValueList(){}

		bool SetByLabel( string const& _label );
		bool SetByValue( int32 const _value );

		void OnValueChanged( int32 const _valueIdx );

		static uint8 const StaticGetValueTypeId(){ return 0x06; }
		static string const StaticGetValueTypeName(){ return "VALUE_LIST"; }

		// From Value
		virtual void WriteXML( TiXmlElement* _valueElement );
		virtual uint8 const GetValueTypeId()const{ return StaticGetValueTypeId(); }
		virtual string const GetValueTypeName()const{ return StaticGetValueTypeName(); }

		virtual string GetAsString()const{ return m_items[m_valueIdx].m_label; }

		Item const& GetItem()const{ return m_items[m_valueIdx]; }
		Item const& GetPending()const{ return m_items[m_pendingIdx]; }

		int32 const GetItemIdxByLabel( string const& _label );
		int32 const GetItemIdxByValue( int32 const _value );

	private:
		vector<Item>	m_items;
		int32			m_valueIdx;
		int32			m_pendingIdx;
	};

} // namespace OpenZWave

#endif



