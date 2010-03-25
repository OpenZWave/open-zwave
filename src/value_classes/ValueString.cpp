//-----------------------------------------------------------------------------
//
//	ValueStore.cpp
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

#include "tinyxml.h"
#include "ValueString.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueString::ValueString>
// Constructor
//-----------------------------------------------------------------------------
ValueString::ValueString
(
	uint8 const _driverId,
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
	Value( _driverId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_String, _label, _units, _readOnly ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueString::ValueString>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueString::ValueString
(
	uint8 const _driverId,
	uint8 const _nodeId,
	TiXmlElement* _valueElement
):
	Value( _driverId, _nodeId, _valueElement )
{
	char const* str = _valueElement->Attribute( "value" );
	if( str )
	{
		m_value = str;
	}
}

//-----------------------------------------------------------------------------
// <ValueString::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueString::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
	_valueElement->SetAttribute( "value", m_value.c_str() );
}

//-----------------------------------------------------------------------------
// <ValueString::Set>
// Set a nwe value in the device
//-----------------------------------------------------------------------------
bool ValueString::Set
(
	string const& _value
)
{
	m_pending = _value;
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueString::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void ValueString::OnValueChanged
(
	string const& _value
)
{
	if( _value == m_value )
	{
		// Value already set
		return;
	}

	m_value = _value;
	Value::OnValueChanged();
}

