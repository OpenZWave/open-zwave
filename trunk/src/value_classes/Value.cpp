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
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
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
	string const& _label,
	bool const _bReadOnly
):
	m_refs( 1 ),
	m_id( _nodeId, _commandClassId, _instance, _index ),
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
	TiXmlElement* _pValueElement
):
	m_refs( 1 )
{
	int intVal;

	uint8 nodeId = 0;
	if( TIXML_SUCCESS == _pValueElement->QueryIntAttribute( "node", &intVal ) )
	{
		nodeId = (uint8)intVal;
	}

	uint8 commandClassId = 0;
	if( TIXML_SUCCESS == _pValueElement->QueryIntAttribute( "command_class", &intVal ) )
	{
		commandClassId = (uint8)intVal;
	}

	uint8 instance = 0;
	if( TIXML_SUCCESS == _pValueElement->QueryIntAttribute( "instance", &intVal ) )
	{
		instance = (uint8)intVal;
	}

	uint8 index = 0;
	if( TIXML_SUCCESS == _pValueElement->QueryIntAttribute( "index", &intVal ) )
	{
		index = (uint8)intVal;
	}

	m_id = ValueID( nodeId, commandClassId, instance, index );

	char const* label = _pValueElement->Attribute( "label" );
	if( label )
	{
		m_label = label;
	}

	char const* units = _pValueElement->Attribute( "units" );
	if( units )
	{
		m_units = units;
	}

	char const* readOnly = _pValueElement->Attribute( "read_only" );
	if( readOnly )
	{
		m_bReadOnly = !strcmp( readOnly, "True" );
	}
}

//-----------------------------------------------------------------------------
// <Value::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Value::WriteXML
(
	TiXmlElement* _pValueElement
)
{

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

	if( Node* pNode = Driver::Get()->GetNode( m_id.GetNodeId() ) )
	{
		if( CommandClass* pCC = pNode->GetCommandClass( m_id.GetCommandClassId() ) )
		{
			return( pCC->SetValue( *this ) );
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
	Driver::Get()->NotifyWatchers( m_id ); 
}




