//-----------------------------------------------------------------------------
//
//	ValueDecimal.h
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

#ifndef _ValueDecimal_H
#define _ValueDecimal_H

#include <string>
#include "Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	class ValueDecimal: public Value
	{
	public:
		ValueDecimal( uint8 const _nodeId, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, uint32 const _genre, string const& _label, bool const _bReadOnly, string const& _value );
		ValueDecimal( uint8 const _nodeId, TiXmlElement* _valueElement );
		virtual ~ValueDecimal(){}

		bool Set( string const& _value );
		void OnValueChanged( string const& _value );

		static uint8 const StaticGetValueTypeId(){ return 0x04; }
		static string const StaticGetValueTypeName(){ return "VALUE_DECIMAL"; }

		// From Value
		virtual void WriteXML( TiXmlElement* _valueElement );
		virtual uint8 const GetValueTypeId()const{ return StaticGetValueTypeId(); }
		virtual string const GetValueTypeName()const{ return StaticGetValueTypeName(); }

		virtual string GetAsString()const{ return m_value; }

		string GetValue()const{ return m_value; }
		string GetPending()const{ return m_pending; }

	private:
		string	m_value;
		string	m_pending;
	};

} // namespace OpenZWave

#endif



