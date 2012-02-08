//-----------------------------------------------------------------------------
//
//	ValueShort.cpp
//
//	Represents a 16-bit value
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

#include <sstream>
#include <limits.h>
#include "tinyxml.h"
#include "ValueShort.h"
#include "Msg.h"
#include "Log.h"
// todo check this
#include "Manager.h"
#include "time.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueShort::ValueShort>
// Constructor
//-----------------------------------------------------------------------------
ValueShort::ValueShort
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
	bool const _writeOnly,
	int16 const _value,
	uint8 const _pollIntensity
):
  	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Byte, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_value( _value )
{
	m_min = SHRT_MIN;
	m_max = SHRT_MAX;
}

string const ValueShort::GetAsString
(
) const
{
	stringstream ss;
	ss << GetValue();
	return ss.str();
}

bool ValueShort::SetFromString
(
	string const& _value
)
{
	uint32 val = (uint32)atoi( _value.c_str() );
	if( val < 32768 )
	{
		return Set( (int16)val );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ValueShort::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueShort::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );

	int intVal;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "value", &intVal ) )
	{
		m_value = (int16)intVal;
//		SetIsSet();
	}
	else
	{
		Log::Write( "Missing default short value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
}

//-----------------------------------------------------------------------------
// <ValueShort::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueShort::WriteXML
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
// <ValueShort::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueShort::Set
(
	int16 const _value
)
{
	// Set the value in our records.
	OnValueChanged( _value );

	// Set the value in the device.
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueShort::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void ValueShort::OnValueChanged
(
	int16 const _value
)
{
	// if this is the first read of a value, assume it is valid (and notify as a change)
	if (!IsSet())
	{
		Log::Write("Initial read of value");			
		SetIsSet();
		m_value = _value;
		Value::OnValueChanged();
		return;
	}
	else
		Log::Write("Refreshed Value: old value=%d, new value=%d",m_value, _value);
	m_refreshTime = time( NULL );

//	Log::Write("IsCheckingChange() is %s",IsCheckingChange()?"true":"false");
	if (IsCheckingChange())
		if (m_value == _value)
		{
			Log::Write("ERROR: Spurious value change was noted.");
			SetCheckingChange(false);
		}

	// see if the reported value has changed
	if (m_value != _value)
	{
		// if strings are not equal, and this is the first indication of a change, check it again
		if (!IsCheckingChange())
		{
			// identify this as a second refresh of the value and send the refresh request
			Log::Write("Changed value (possible)--rechecking");
			SetCheckingChange( true );
			m_valueCheck = _value;
			Manager::Get()->RefreshValue( GetID() );
			return;
		}
		else
		{
			// this is a "checked" value
			if (m_valueCheck == _value)
			{
				// same as the changed value being checked?  if so, confirm the change
				Log::Write("Changed value--confirmed");
				SetCheckingChange( false );
				SetIsSet();
				m_value = _value;
				Value::OnValueChanged();
			}
			else
			{
				// the second read is different than both the original value and the checked value...retry
				Log::Write("Changed value (changed again)--rechecking");
				SetCheckingChange( true );
				m_valueCheck = _value;
				Manager::Get()->RefreshValue( GetID() );
				return;
			}
		}
	}
}

