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
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_String, _label, _units, _readOnly, false ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueString::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueString::ReadXML
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
	else
	{
		Log::Write( "Missing default string value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
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
	if ( !IsSet() )
		_valueElement->SetAttribute( "value", "" );
	else
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
	// Set the value in our records.
	OnValueChanged( _value );

	// Set the value in the device.
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
	m_value = _value;
	Value::OnValueChanged();
}

