//-----------------------------------------------------------------------------
//
//	Value.cpp
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
#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "Value.h"
#include "Msg.h"
#include "Log.h"
#include "CommandClass.h"

using namespace OpenZWave;

static char* const c_genreName[] = 
{
	"all",
	"user",
	"config",
	"system"
};

static char* const c_typeName[] = 
{
	"bool",
	"byte",
	"decimal",
	"int",
	"list",
	"short",
	"string"
};

//-----------------------------------------------------------------------------
// <Value::Value>
// Constructor
//-----------------------------------------------------------------------------
Value::Value
(
	uint32 const _homeId,
	uint8 const _nodeId,
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	ValueID::ValueType const _type,
	string const& _label,
	string const& _units,
	bool const _readOnly
):
	m_refs( 1 ),
	m_id( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, _type ),
	m_label( _label ),
	m_units( _units ),
	m_readOnly( _readOnly ),
	m_poll( false )
{
}

//-----------------------------------------------------------------------------
// <Value::Value>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Value::Value
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
):
	m_refs( 1 ),
	m_readOnly( false ),
	m_poll( false )
{
	int intVal;

	ValueID::ValueGenre genre = Value::GetGenreEnumFromName( _valueElement->Attribute( "genre" ) );
	ValueID::ValueType type = Value::GetTypeEnumFromName( _valueElement->Attribute( "type" ) );

	uint8 instance = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "instance", &intVal ) )
	{
		instance = (uint8)intVal;
	}

	uint8 index = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "index", &intVal ) )
	{
		index = (uint8)intVal;
	}

	m_id = ValueID( _homeId, _nodeId, genre, _commandClassId, instance, index, type );

	char const* label = _valueElement->Attribute( "label" );
	if( label )
	{
		m_label = label;
	}

	char const* units = _valueElement->Attribute( "units" );
	if( units )
	{
		m_units = units;
	}

	char const* readOnly = _valueElement->Attribute( "read_only" );
	if( readOnly )
	{
		m_readOnly = !strcmp( readOnly, "true" );
	}

	char const* poll = _valueElement->Attribute( "poll" );
	if( poll )
	{
		// Use the Manager Enable/Disable poll methods so that everything gets set up correctly
		if( strcmp( poll, "true" ) )
		{
			Manager::Get()->DisablePoll( m_id );
		}
		else
		{
			Manager::Get()->EnablePoll( m_id );
		}
	}
}

//-----------------------------------------------------------------------------
// <Value::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Value::WriteXML
(
	TiXmlElement* _valueElement
)
{
	char str[8];

	_valueElement->SetAttribute( "type", GetTypeNameFromEnum(m_id.GetType()) );
	_valueElement->SetAttribute( "genre", GetGenreNameFromEnum(m_id.GetGenre()) );

	snprintf( str, 8, "%d", m_id.GetInstance() );
	_valueElement->SetAttribute( "instance", str );

	snprintf( str, 8, "%d", m_id.GetIndex() );
	_valueElement->SetAttribute( "index", str );

	_valueElement->SetAttribute( "label", m_label.c_str() );
	_valueElement->SetAttribute( "units", m_units.c_str() );
	_valueElement->SetAttribute( "read_only", m_readOnly ? "true" : "false" );
	_valueElement->SetAttribute( "poll", m_poll ? "true" : "false" );
}

//-----------------------------------------------------------------------------
// <Value::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool Value::Set
(
)
{
	if( IsReadOnly() )
	{
		return false;
	}

	if( Node* node = GetNode() )
	{
		if( CommandClass* cc = node->GetCommandClass( m_id.GetCommandClassId() ) )
		{
			return( cc->SetValue( *this ) );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Value::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void Value::OnValueChanged
(
)
{
	// Notify the watchers
	Manager::Notification* notification = new Manager::Notification();
	
	notification->m_type = Manager::NotificationType_ValueChanged;
	notification->m_id = m_id;
	notification->m_groupIdx = 0;

	Manager::Get()->NotifyWatchers( notification ); 

	delete notification;
}

//-----------------------------------------------------------------------------
// <Value::GetNode>
// Get a pointer to our node
//-----------------------------------------------------------------------------
Node* Value::GetNode
(
)const
{
	if( Driver* driver = Manager::Get()->GetDriver( m_id.GetHomeId() ) )
	{
		return driver->GetNode( m_id.GetNodeId() );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Value::SetPolled>
// Set this value as polled
//-----------------------------------------------------------------------------
void Value::SetPolled
(
	bool const _state	
)
{
	if( _state != m_poll )
	{
		m_poll = _state;

		// Notify the watchers
		Manager::Notification* notification = new Manager::Notification();
		
		notification->m_type = m_poll ? Manager::NotificationType_PollingEnabled : Manager::NotificationType_PollingDisabled;
		notification->m_id = m_id;
		notification->m_groupIdx = 0;

		Manager::Get()->NotifyWatchers( notification ); 

		delete notification;
	}
}

//-----------------------------------------------------------------------------
// <Value::GetGenreEnumFromName>
// Static helper to get a genre enum from a string
//-----------------------------------------------------------------------------
ValueID::ValueGenre Value::GetGenreEnumFromName
(
	char const* _name	
)
{
	ValueID::ValueGenre genre = ValueID::ValueGenre_System;
	if( _name )
	{
		for( int i=0; i<(int)ValueID::ValueGenre_Count; ++i )
		{
			if( !strcmp( _name, c_genreName[i] ) )
			{
				genre = (ValueID::ValueGenre)i;
				break;
			}
		}
	}

	return genre;
}

//-----------------------------------------------------------------------------
// <Value::GetGenreNameFromEnum>
// Static helper to get a genre enum as a string
//-----------------------------------------------------------------------------
char const* Value::GetGenreNameFromEnum
(
	ValueID::ValueGenre _genre
)
{
	return c_genreName[_genre];
}

//-----------------------------------------------------------------------------
// <Value::GetTypeEnumFromName>
// Static helper to get a type enum from a string
//-----------------------------------------------------------------------------
ValueID::ValueType Value::GetTypeEnumFromName
(
	char const* _name	
)
{
	ValueID::ValueType type = ValueID::ValueType_Bool;
	if( _name )
	{
		for( int i=0; i<(int)ValueID::ValueType_Count; ++i )
		{
			if( !strcmp( _name, c_typeName[i] ) )
			{
				type = (ValueID::ValueType)i;
				break;
			}
		}
	}

	return type;
}

//-----------------------------------------------------------------------------
// <Value::GetTypeNameFromEnum>
// Static helper to get a type enum as a string
//-----------------------------------------------------------------------------
char const* Value::GetTypeNameFromEnum
(
	ValueID::ValueType _type
)
{
	return c_typeName[_type];
}


