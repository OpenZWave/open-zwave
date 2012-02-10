//-----------------------------------------------------------------------------
//
//	ValueList.cpp
//
//	Represents a list of items
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
#include "ValueList.h"
#include "Msg.h"
#include "Log.h"
// todo check this
#include "Manager.h"
#include "time.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueList::ValueList>
// Constructor
//-----------------------------------------------------------------------------
ValueList::ValueList
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
	vector<Item> const& _items,
	int32 const _valueIdx,
	uint8 const _pollIntensity
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_List, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_items( _items ),
	m_valueIdx( _valueIdx )
{
}

//-----------------------------------------------------------------------------
// <ValueList::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueList::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );

	// Read the items
	m_items.clear();
	TiXmlElement const* itemElement = _valueElement->FirstChildElement();
	while( itemElement )
	{
		char const* str = itemElement->Value();
		if( str && !strcmp( str, "Item" ) )
		{
			char const* labelStr = itemElement->Attribute( "label" );

			int value = 0;
			itemElement->QueryIntAttribute( "value", &value );

			Item item;
			item.m_label = labelStr;
			item.m_value = value;

			m_items.push_back( item );
		}

		itemElement = itemElement->NextSiblingElement();
	}

	// Set the value
	int intVal;
	m_valueIdx = 0;
	if ( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "value", &intVal ) )
	{
		m_valueIdx = (int32)intVal;
//		SetIsSet();
	}
	else
	{
		Log::Write( "Missing default list value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
}

//-----------------------------------------------------------------------------
// <ValueList::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueList::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
	
	char str[16] = "";
	if ( IsSet() )
    {
		snprintf( str, 16, "%d", m_valueIdx );
    }
	_valueElement->SetAttribute( "value", str );

	for( vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it )
	{
		TiXmlElement* pItemElement = new TiXmlElement( "Item" );
		pItemElement->SetAttribute( "label", (*it).m_label.c_str() );
		
		snprintf( str, 16, "%d", (*it).m_value );
		pItemElement->SetAttribute( "value", str );

		_valueElement->LinkEndChild( pItemElement );
	}
}

//-----------------------------------------------------------------------------
// <ValueList::SetByLabel>
// Set a new value in the device, selected by item label
//-----------------------------------------------------------------------------
bool ValueList::SetByLabel
(
	string const& _label
)
{
	// Ensure the value is one of the options
	int32 index = GetItemIdxByLabel( _label );
	if( index < 0 )
	{
		// Item not found
		return false;
	}

	// Set the value in our records.
//	OnValueChanged( m_items[index].m_value );
	m_newValueIdx = index;

	// Set the value in the device.
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueList::SetByValue>
// Set a new value in the device, selected by item value
//-----------------------------------------------------------------------------
bool ValueList::SetByValue
(
	int32 const _value
)
{
	// Set the value in our records.
//	OnValueChanged( _value );

	// Set the value in the device.
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueList::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void ValueList::OnValueChanged
(
	int32 const _value
)
{
	// Ensure the value is one of the options
	int32 index = GetItemIdxByValue( _value );
	if( index < 0 )
	{
		// Item not found
		return;
	}

	// if this is the first read of a value, assume it is valid (and notify as a change)
	if (!IsSet())
	{
		Log::Write("Initial read of value");			
		SetIsSet();
		m_valueIdx = index;
		Value::OnValueChanged();
		return;
	}
	else
		Log::Write("Refreshed Value: old value index=%d, new value index=%d",m_valueIdx, index);
	m_refreshTime = time( NULL );

//	Log::Write("IsCheckingChange() is %s",IsCheckingChange()?"true":"false");
	if (IsCheckingChange())
		if (m_valueIdx == index)
		{
			Log::Write("WARNING: Spurious value change was noted.");
			SetCheckingChange(false);
		}

	// see if the reported value has changed
	if (m_valueIdx != index)
	{
		// if so, and this is the first indication of a change, check it again
		if (!IsCheckingChange())
		{
			// identify this as a second refresh of the value and send the refresh request
			Log::Write("Changed value (possible)--rechecking");
			SetCheckingChange( true );
			m_valueIdxCheck = index;
			Manager::Get()->RefreshValue( GetID() );
			return;
		}
		else
		{
			// this is a "checked" value
			if (m_valueIdxCheck == index)
			{
				// same as the changed value being checked?  if so, confirm the change
				Log::Write("Changed value--confirmed");
				SetCheckingChange( false );
				SetIsSet();
				m_valueIdx = index;
				Value::OnValueChanged();
			}
			else
			{
				// the second read is different than both the original value and the checked value...retry
				Log::Write("Changed value (changed again)--rechecking");
				SetCheckingChange( true );
				m_valueIdxCheck = index;
				Manager::Get()->RefreshValue( GetID() );
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <ValueList::GetItemIdxByLabel>
// Get the index of an item from its label
//-----------------------------------------------------------------------------
int32 const ValueList::GetItemIdxByLabel
(
	string const& _label
)
{
	for( int32 i=0; i<(int32)m_items.size(); ++i )
	{
		if( _label == m_items[i].m_label )
		{
			return i;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// <ValueList::GetItemIdxByValue>
// Get the index of an item from its value
//-----------------------------------------------------------------------------
int32 const ValueList::GetItemIdxByValue
(
	int32 const _value
)
{
	for( int32 i=0; i<(int32)m_items.size(); ++i )
	{
		if( _value == m_items[i].m_value )
		{
			return i;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// <ValueList::GetItemLabels>
// Fill a vector with the item labels
//-----------------------------------------------------------------------------
bool ValueList::GetItemLabels
(
	vector<string>* o_items
)
{
	if( o_items )
	{
		for( vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it )
		{
			o_items->push_back( (*it).m_label );
		}

		return true;
	}

	return false;
}







