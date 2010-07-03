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
	uint32 const _homeId,
	uint8 const _nodeId,
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	uint8 const _value
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Byte, _label, _units, _readOnly, false ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueByte::ValueByte>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueByte::ValueByte
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
):
	Value( _homeId, _nodeId, _commandClassId, _valueElement )
{
	int intVal;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "value", &intVal ) )
	{
		m_value = (uint8)intVal;
		SetIsSet();
	}
}

//-----------------------------------------------------------------------------
// <ValueByte::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueByte::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );

	if ( !IsSet() )
		_valueElement->SetAttribute( "value", "" );
	else {
		char str[8];
		snprintf( str, sizeof(str), "%d", m_value );
		_valueElement->SetAttribute( "value", str );
	}
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
	// Set the value in our records.
	OnValueChanged( _value );

	// Set the value in the device.
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueByte::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void ValueByte::OnValueChanged
(
	uint8 const _value
)
{
	m_value = _value;
	Value::OnValueChanged();
}
