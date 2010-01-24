//-----------------------------------------------------------------------------
//
//	ValueInt.h
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

#ifndef _ValueInt_H
#define _ValueInt_H

#include <string>
#include "tinyxml.h"
#include "Defs.h"
#include "Value.h"

namespace OpenZWave
{
	class Msg;
	class Node;

	class ValueInt: public Value
	{
	public:
		ValueInt( uint8 const _nodeId, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, bool const _bReadOnly, int32 const _value );
		ValueInt( TiXmlElement* _pValueElement );
		virtual ~ValueInt(){}

		bool Set( int32 const _value );

		static uint8 const StaticGetValueTypeId(){ return 0x03; }
		static string const StaticGetValueTypeName(){ return "VALUE_INT"; }

		// From Value
		virtual void WriteXML( TiXmlElement* _pValueElement );
		virtual uint8 const GetValueTypeId()const{ return StaticGetValueTypeId(); }
		virtual string const GetValueTypeName()const{ return StaticGetValueTypeName(); }

		virtual string GetAsString()const;

	private:
		int32	m_value;
	};

} // namespace OpenZWave

#endif



