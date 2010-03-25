//-----------------------------------------------------------------------------
//
//	ValueString.h
//
//	Represents a string value
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

#ifndef _ValueString_H
#define _ValueString_H

#include <string>
#include "Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	class ValueString: public Value
	{
	public:
		ValueString( uint8 const _driverId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, string const& _units, bool const _readOnly, string const& _value );
		ValueString( uint8 const _driverId, uint8 const _nodeId, TiXmlElement* _valueElement );
		virtual ~ValueString(){}

		bool Set( string const& _value );
		void OnValueChanged( string const& _value );

		// From Value
		virtual void WriteXML( TiXmlElement* _valueElement );
		virtual string GetAsString()const{ return m_value; }

		string GetValue()const{ return m_value; }
		string GetPending()const{ return m_pending; }

	private:
		string	m_value;
		string	m_pending;
	};

} // namespace OpenZWave

#endif



