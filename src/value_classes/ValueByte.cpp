//-----------------------------------------------------------------------------
//
//	ValueByte.cpp
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

#include "tinyxml.h"
#include "ValueByte.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueByte::ValueByte>
// Constructor
//-----------------------------------------------------------------------------
ValueByte::ValueByte
(
	uint8 const _nodeId,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	string const& _label,
	bool const _bReadOnly,
	uint8 const _value
):
	Value( _nodeId, _commandClassId, _instance, _index, _label, _bReadOnly ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueByte::ValueByte>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueByte::ValueByte
(
	TiXmlElement* _pValueElement
):
	Value( _pValueElement )
{
	int intVal;
	if( TIXML_SUCCESS == _pValueElement->QueryIntAttribute( "value", &intVal ) )
	{
		m_value = (uint8)intVal;
	}
}

//-----------------------------------------------------------------------------
// <ValueByte::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueByte::WriteXML
(
	TiXmlElement* _pValueElement
)
{
	Value::WriteXML( _pValueElement );

	char str[8];
	snprintf( str, sizeof(str), "%d", m_value );
	_pValueElement->SetAttribute( "value", str );
}

//-----------------------------------------------------------------------------
// <ValueByte::GetAsString>
// Convert the value to string form
//-----------------------------------------------------------------------------
string ValueByte::GetAsString
(
)const
{
	char str[8];
	snprintf( str, 8, "%d", m_value );
	return( str );
}

//-----------------------------------------------------------------------------
// <ValueByte::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueByte::Set
(
	uint8 const _value
)
{
	if( IsReadOnly() )
	{
		return false;
	}

	if( _value == m_value )
	{
		return true;
	}

	return false;
}


