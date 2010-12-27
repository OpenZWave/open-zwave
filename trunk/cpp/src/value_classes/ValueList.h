//-----------------------------------------------------------------------------
//
//	ValueList.h
//
//	Represents a list of items
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

	/** \brief List of values sent to/received from a node.
	 */
	class ValueList: public Value
	{
	public:
		/** \brief An item (element) in the list of values.
		*/
		struct Item
		{
			string	m_label;
			int32	m_value;
		};

		ValueList( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, string const& _units, bool const _readOnly, vector<Item> const& _items, int32 const _valueIdx );
		ValueList(){}
		virtual ~ValueList(){}

		bool SetByLabel( string const& _label );
		bool SetByValue( int32 const _value );

		void OnValueChanged( int32 const _valueIdx );

		// From Value
		virtual bool GetAsInt( int32& _value ) const { _value = GetItem().m_value; return true; }
		virtual bool SetFromInt( int32 const _value ) { SetByValue( _value ); return true; }
		virtual string const GetAsString() const { return GetItem().m_label; }
		virtual bool SetFromString( string const& _value ) { return SetByLabel( _value ); }
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		Item const& GetItem()const{ return m_items[m_valueIdx]; }

		int32 const GetItemIdxByLabel( string const& _label );
		int32 const GetItemIdxByValue( int32 const _value );

		bool GetItemLabels( vector<string>* o_items );

	private:
		vector<Item>	m_items;
		int32			m_valueIdx;
	};

} // namespace OpenZWave

#endif



