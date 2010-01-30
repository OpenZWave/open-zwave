//-----------------------------------------------------------------------------
//
//	ValueString.h
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

#ifndef _ValueString_H
#define _ValueString_H

#include <string>
#include "tinyxml.h"
#include "Defs.h"
#include "Value.h"

namespace OpenZWave
{
	class Msg;
	class Node;

	class ValueString: public Value
	{
	public:
		ValueString( uint8 const _nodeId, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, uint32 const _genre, string const& _label, bool const _bReadOnly, string const& _value );
		ValueString( TiXmlElement* _pValueElement );
		virtual ~ValueString(){}

		bool Set( string const& _value );
		void OnValueChanged( string const& _value );

		static uint8 const StaticGetValueTypeId(){ return 0x05; }
		static string const StaticGetValueTypeName(){ return "VALUE_STRING"; }

		// From Value
		virtual void WriteXML( TiXmlElement* _pValueElement );
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



