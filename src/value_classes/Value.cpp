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

//-----------------------------------------------------------------------------
// <Value::Value>
// Constructor
//-----------------------------------------------------------------------------
Value::Value
(
	uint8 const _nodeId,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	uint32 const _genre,
	string const& _label,
	bool const _bReadOnly
):
	m_refs( 1 ),
	m_id( _nodeId, _commandClassId, _instance, _index ),
	m_genre( _genre ),
	m_label( _label ),
	m_bReadOnly( _bReadOnly )
{
}

//-----------------------------------------------------------------------------
// <Value::Value>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Value::Value
(
	uint8 const _nodeId,
	TiXmlElement* _valueElement
):
	m_refs( 1 ),
	m_bReadOnly( false ),
	m_genre( Genre_System )
{
	int intVal;

	uint8 commandClassId = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "command_class", &intVal ) )
	{
		commandClassId = (uint8)intVal;
	}

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

	m_id = ValueID( _nodeId, commandClassId, instance, index );

	char const* genre = _valueElement->Attribute( "genre" );
	for( int i=0; i<4; ++i )
	{
		if( !strcmp( genre, c_genreName[i] ) )
		{
			m_genre = i;
			break;
		}
	}

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
		m_bReadOnly = !strcmp( readOnly, "true" );
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

	snprintf( str, 8, "%d", m_id.GetCommandClassId() );
	_valueElement->SetAttribute( "command_class", str );

	snprintf( str, 8, "%d", m_id.GetInstance() );
	_valueElement->SetAttribute( "instance", str );

	snprintf( str, 8, "%d", m_id.GetIndex() );
	_valueElement->SetAttribute( "index", str );

	_valueElement->SetAttribute( "genre", c_genreName[m_genre] );
	_valueElement->SetAttribute( "label", m_label.c_str() );
	_valueElement->SetAttribute( "units", m_units.c_str() );
	_valueElement->SetAttribute( "read_only", m_bReadOnly ? "true" : "false" );
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

	if( Node* node = Driver::Get()->GetNode( m_id.GetNodeId() ) )
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
	Driver::Notification* notification = new Driver::Notification();
	
	notification->m_type = Driver::NotificationType_Value;
	notification->m_id = m_id;
	notification->m_nodeId = m_id.GetNodeId();
	notification->m_groupIdx = 0;

	Driver::Get()->NotifyWatchers( notification ); 

	delete notification;
}




