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
// todo check this
#include "Manager.h"
#include "time.h"

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
	bool const _writeOnly,
	string const& _value,
	uint8 const _pollIntensity
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_String, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
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
//		SetIsSet();
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
//	OnValueChanged( _value );

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
		Log::Write("Refreshed Value: old value=%s, new value=%s",m_value.c_str(), _value.c_str());
	m_refreshTime = time( NULL );

//	Log::Write("IsCheckingChange() is %s",IsCheckingChange()?"true":"false");
	if (IsCheckingChange())
		if (strcmp(m_value.c_str(), _value.c_str())==0)
		{
			Log::Write("WARNING: Spurious value change was noted.");
			SetCheckingChange(false);
		}

	// see if the reported value has changed
	if (strcmp(m_value.c_str(), _value.c_str())!=0)
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
			if (strcmp(m_valueCheck.c_str(), _value.c_str())==0)
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

