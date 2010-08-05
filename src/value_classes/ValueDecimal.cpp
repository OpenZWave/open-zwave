//-----------------------------------------------------------------------------
//
//	ValueDecimal.cpp
//
//	Represents a value that may have a fractional component
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
#include "ValueDecimal.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueDecimal::ValueDecimal>
// Constructor
//-----------------------------------------------------------------------------
ValueDecimal::ValueDecimal
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
	string const& _value
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Decimal, _label, _units, _readOnly, false ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueDecimal::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueDecimal::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );

	char const* str = _valueElement->Attribute( "value" );
	if( str )
	{
		m_value = str;
		SetIsSet();
	}
}

//-----------------------------------------------------------------------------
// <ValueDecimal::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueDecimal::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
	if ( !IsSet() )
		_valueElement->SetAttribute( "value", "" );
	else
		_valueElement->SetAttribute( "value", m_value.c_str() );
}

//-----------------------------------------------------------------------------
// <ValueDecimal::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueDecimal::Set
(
	string const& _value
)
{
	// Set the value in our records.
	OnValueChanged( _value );

	// Set the value in the device.
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueDecimal::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void ValueDecimal::OnValueChanged
(
	string const& _value
)
{
	m_value = _value;
	Value::OnValueChanged();
}



