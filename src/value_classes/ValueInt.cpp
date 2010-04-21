//-----------------------------------------------------------------------------
//
//	ValueInt.cpp
//
//	Represents a 32-bit value
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
#include "ValueInt.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueInt::ValueInt>
// Constructor
//-----------------------------------------------------------------------------
ValueInt::ValueInt
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
	int32 const _value
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Int, _label, _units, _readOnly, false ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueInt::ValueInt>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueInt::ValueInt
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
		m_value = (int32)intVal;
		SetIsSet();
	}
}

//-----------------------------------------------------------------------------
// <ValueInt::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueInt::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );

	if ( !IsSet() )
		_valueElement->SetAttribute( "value", "" );
	else {
		char str[16];
		snprintf( str, sizeof(str), "%d", m_value );
		_valueElement->SetAttribute( "value", str );
	}
}

//-----------------------------------------------------------------------------
// <ValueInt::GetAsString>
// Convert the value to string form
//-----------------------------------------------------------------------------
string ValueInt::GetAsString
(
)const
{
	char str[16];
	snprintf( str, 16, "%d", m_value );
	return( string( str ) );
}

//-----------------------------------------------------------------------------
// <ValueInt::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueInt::Set
(
	int32 const _value
)
{
	m_pending = _value;
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueInt::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void ValueInt::OnValueChanged
(
	int32 const _value
)
{
	if( IsSet() && _value == m_value )
	{
		// Value already set
		return;
	}

	m_value = _value;
	Value::OnValueChanged();
}



