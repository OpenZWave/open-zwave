//-----------------------------------------------------------------------------
//
//	ValueBitSet.cpp
//
//	Represents a boolean value
//
//	Copyright (c) 2017 Justin Hammond <justin@dynam.ac>
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
#include "value_classes/ValueBitSet.h"
#include "Driver.h"
#include "Node.h"
#include "platform/Log.h"
#include "Manager.h"
#include <ctime>

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueBitSet::ValueBitSet>
// Constructor
//-----------------------------------------------------------------------------
ValueBitSet::ValueBitSet
(
	uint32 const _homeId,
	uint8 const _nodeId,
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint16 const _index,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	uint32 const _value,
	uint8 const _pollIntensity
):
  	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Bool, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_value( _value ),
	m_valueCheck( false ),
	m_newValue( false )
{
}

bool ValueBitSet::SetFromString
(
	string const& _value
)
{
	int32 val = atoi( _value.c_str() );
	return Set( val );
	return false;
}

string const ValueBitSet::GetAsString
(
) const
{
	stringstream ss;
	ss << GetValue();
	return ss.str();
}

uint32 ValueBitSet::GetValue
(
) const
{
	return m_value.GetValue();
}
bool ValueBitSet::GetBit
(
		uint8 _idx
) const
{
	return m_value.IsSet(_idx);
}



//-----------------------------------------------------------------------------
// <ValueBitSet::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueBitSet::ReadXML
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
		m_value.SetValue((uint32)intVal);
	}
	else
	{
		Log::Write( LogLevel_Info, "Missing default integer value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
}

//-----------------------------------------------------------------------------
// <ValueBitSet::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueBitSet::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
	char str[16];
	snprintf( str, sizeof(str), "%d", m_value.GetValue() );
	_valueElement->SetAttribute( "value", str );
}

//-----------------------------------------------------------------------------
// <ValueBitSet::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueBitSet::Set
(
	uint32 const _value
)
{
	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueBitSet* tempValue = new ValueBitSet( *this );
	tempValue->m_value = _value;

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}

bool ValueBitSet::SetBit
(
		uint8 const _idx
)
{
	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueBitSet* tempValue = new ValueBitSet( *this );
	tempValue->m_value.Set(_idx);

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}
bool ValueBitSet::ClearBit
(
		uint8 const _idx
)
{
	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueBitSet* tempValue = new ValueBitSet( *this );
	tempValue->m_value.Clear(_idx);

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}


//-----------------------------------------------------------------------------
// <ValueBitSet::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
void ValueBitSet::OnValueRefreshed
(
	uint32 const _value
)
{
	switch( VerifyRefreshedValue( (void*) &m_value, (void*) &m_valueCheck, (void*) &_value, ValueID::ValueType_Bool) )
	{
	case 0:		// value hasn't changed, nothing to do
		break;
	case 1:		// value has changed (not confirmed yet), save _value in m_valueCheck
		m_valueCheck = _value;
		break;
	case 2:		// value has changed (confirmed), save _value in m_value
		m_value = _value;
		break;
	case 3:		// all three values are different, so wait for next refresh to try again
		break;
	}
}
