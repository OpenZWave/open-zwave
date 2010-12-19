//-----------------------------------------------------------------------------
//
//	CommandClass.cpp
//
//	Base class for all Z-Wave Command Classes
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

#include <math.h>
#include "tinyxml.h"
#include "CommandClass.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Manager.h"
#include "Log.h"
#include "ValueStore.h"

using namespace OpenZWave;

static uint8 const	c_sizeMask			= 0x07;
static uint8 const	c_scaleMask			= 0x18;
static uint8 const	c_scaleShift		= 0x03;
static uint8 const	c_precisionMask		= 0xe0;
static uint8 const	c_precisionShift	= 0x05;


//-----------------------------------------------------------------------------
// <CommandClass::CommandClass>
// Constructor
//-----------------------------------------------------------------------------
CommandClass::CommandClass
(
	uint32 const _homeId, 
	uint8 const _nodeId 
): 
	m_homeId( _homeId ),
	m_nodeId( _nodeId ),
	m_version( 1 ),
	m_instances( 0 ),
	m_staticRequests( 0 )
{
}

//-----------------------------------------------------------------------------
// <CommandClass::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* CommandClass::GetDriver
(
)const
{
	return( Manager::Get()->GetDriver( m_homeId ) );
}

//-----------------------------------------------------------------------------
// <CommandClass::GetNode>
// Get a pointer to our node without locking the mutex
//-----------------------------------------------------------------------------
Node* CommandClass::GetNodeUnsafe
(
)const
{
	return( GetDriver()->GetNodeUnsafe( m_nodeId ) );
}

//-----------------------------------------------------------------------------
// <CommandClass::SetInstances>
// Set the number of instances of this command class that the node contains
//-----------------------------------------------------------------------------
void CommandClass::SetInstances
( 
	uint8 const _instances
)
{
	// Create a set of reported variables for each new instance
	if( _instances > m_instances )
	{
		// Create the new value instances
		for( uint8 i=m_instances; i<_instances; ++i )
		{
			CreateVars( i+1 );
		}

		m_instances = _instances;
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::ReadXML>
// Read the saved command class data
//-----------------------------------------------------------------------------
void CommandClass::ReadXML
( 
	TiXmlElement const* _ccElement
)
{
	int32 intVal;

	m_version = 1;
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "version", &intVal ) )
	{
		m_version = (uint8)intVal;
	}

	uint8 instances = 1;
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "instances", &intVal ) )
	{
		instances = (uint8)intVal;
	}

	m_staticRequests = 0;
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "request_flags", &intVal ) )
	{
		m_staticRequests = (uint8)intVal;
	}

	// Setting the instance count will create all the values.
	SetInstances( instances );

	// Apply any differences from the saved XML to the values
	TiXmlElement const* child = _ccElement->FirstChildElement();
	while( child )
	{
		char const* str = child->Value();
		if( str )
		{
			if( !strcmp( str, "Value" ) )
			{
				GetNodeUnsafe()->ReadValueFromXML( GetCommandClassId(), child );
			}
		}

		child = child->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::WriteXML>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void CommandClass::WriteXML
( 
	TiXmlElement* _ccElement
)
{
	char str[32];

	snprintf( str, 32, "%d", GetCommandClassId() );
	_ccElement->SetAttribute( "id", str );
	_ccElement->SetAttribute( "name", GetCommandClassName().c_str() );

	snprintf( str, 32, "%d", GetVersion() );
	_ccElement->SetAttribute( "version", str );

	snprintf( str, 32, "%d", GetInstances() );
	_ccElement->SetAttribute( "instances", str );

	if( m_staticRequests )
	{
		snprintf( str, 32, "%d", m_staticRequests );
		_ccElement->SetAttribute( "request_flags", str );
	}

	// Write out the values for this command class
	ValueStore* store = GetNodeUnsafe()->GetValueStore();
	for( ValueStore::Iterator it = store->Begin(); it != store->End(); ++it )
	{
		Value* value = it->second;
		if( value->GetID().GetCommandClassId() == GetCommandClassId() )
		{
			TiXmlElement* valueElement = new TiXmlElement( "Value" );
			_ccElement->LinkEndChild( valueElement );
			value->WriteXML( valueElement );
		}
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::ExtractValue>
// Read a value from a variable length sequence of bytes
//-----------------------------------------------------------------------------
float32 CommandClass::ExtractValue
(
	uint8 const* _data,
	uint8* _scale
)const
{
	uint8 const size = _data[0] & c_sizeMask;
	uint8 const precision = (_data[0] & c_precisionMask) >> c_precisionShift;

	if( _scale )
	{
		*_scale = (_data[0] & c_scaleMask) >> c_scaleShift;
	}

	uint32 value = 0;
	uint8 i;
	for( i=0; i<size; ++i )
	{
		value <<= 8;
		value |= (uint32)_data[i+1];
	}

	// Deal with sign extension.  Anything larger than a byte is assumed to be signed.
	if( _data[1] & 0x80 )
	{
		if( size == 2 )
		{
			value |= 0xffff0000;
		}
		else if( size == 3 )
		{
			value |= 0xff000000;
		}
	}

	if( precision == 0 )
	{
		return (float32)(signed long)value;
	}

	return ((float32)(signed long)value) / pow( 10.0f, precision );
}

//-----------------------------------------------------------------------------
// <CommandClass::ExtractValue>
// Read a value from a variable length sequence of bytes
//-----------------------------------------------------------------------------
string CommandClass::ExtractValueAsString
(
	uint8 const* _data,
	uint8* _scale
)const
{
	float32 value = ExtractValue( _data, _scale );

	char str[16];
	snprintf( str, 16, "%.3f", value );

	return str;
}

//-----------------------------------------------------------------------------
// <CommandClass::AppendValue>
// Add a value to a message as a sequence of bytes
//-----------------------------------------------------------------------------
void CommandClass::AppendValue
(
	Msg* _msg,
	float32 const _value,
	uint8 const _precision,
	uint8 const _scale
)const
{
	int32 iValue = _precision ? (int32)(_value * pow( 10.0f, _precision )) : (int32)_value;

	uint8 size = 1;
	if( iValue & 0xffff0000 )
	{
		size = 4;
	}
	else if( iValue & 0x0000ff00 )
	{
		size = 2;
	}

	_msg->Append( (_precision<<c_precisionShift) | (_scale<<c_scaleShift) | size );

	for( int32 i=size-1; i>=0; --i )
	{
		_msg->Append( (uint8)((iValue >> (size<<3)) & 0xff) );
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::GetAppendValueSize>
// Get the number of bytes that would be added by a call to AppendValue
//-----------------------------------------------------------------------------
uint8 const CommandClass::GetAppendValueSize
(
	float32 const _value,
	uint8 const _precision
)const
{
	int32 iValue = _precision ? (int32)(_value * pow( 10.0f, _precision )) : (int32)_value;

	uint8 size = 1;
	if( iValue & 0xffff0000 )
	{
		size = 4;
	}
	else if( iValue & 0x0000ff00 )
	{
		size = 2;
	}

	return size + 1;
}

//-----------------------------------------------------------------------------
// <CommandClass::ClearStaticRequest>
// The static data for this command class has been read from the device
//-----------------------------------------------------------------------------
void CommandClass::ClearStaticRequest
( 
	uint8 _request
)
{ 
	m_staticRequests &= ~_request;
	
	if( Node* node = GetNodeUnsafe() )
	{
		if( _request & StaticRequest_Version )
		{
			node->QueryStageRetry( Node::QueryStage_Versions );
			return;
		}

		if( _request & StaticRequest_Instances )
		{
			node->QueryStageRetry( Node::QueryStage_Instances );
			return;
		}

		if( _request & StaticRequest_Values )
		{
			node->QueryStageRetry( Node::QueryStage_Static );
			return;
		}
	}
}
