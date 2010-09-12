//-----------------------------------------------------------------------------
//
//	ValueByte.h
//
//	Represents an 8-bit value
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

#ifndef _ValueByte_H
#define _ValueByte_H

#include <string>
#include "Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	class ValueByte: public Value
	{
	public:
		ValueByte( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, string const& _units, bool const _readOnly, uint8 const _value );
		ValueByte(){}
		virtual ~ValueByte(){}

		bool Set( uint8 const _value );
		void OnValueChanged( uint8 const _value );

		// From Value
		virtual bool GetAsInt( int32& _value ) const { _value = (int32)GetValue(); return true; }
		virtual bool SetFromInt( int32 const _value ) { Set( (uint8)_value ); return true; }
		virtual string const GetAsString() const;
		virtual bool SetFromString( string const& _value );
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		uint8 GetValue()const{ return m_value; }

	private:
		uint8	m_value;
	};

} // namespace OpenZWave

#endif



