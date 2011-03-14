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
#include <locale.h>
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
	m_staticRequests( 0 ),
	m_afterMark( false )
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
// <CommandClass::GetValue>
// Get a pointer to a value by its instance and index
//-----------------------------------------------------------------------------
Value* CommandClass::GetValue
(
	uint8 const _instance,
	uint8 const _index
)
{
	Value* value = NULL;
	if( Node* node = GetNodeUnsafe() )
	{
		value = node->GetValue( GetCommandClassId(), _instance, _index );
	}
	return value;
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
		if( !m_afterMark )
		{	
			for( uint8 i=m_instances; i<_instances; ++i )
			{
				CreateVars( i+1 );
			}
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
	char const* str;

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

	m_afterMark = false;
	str = _ccElement->Attribute( "after_mark" );
	if( str )
	{
		m_afterMark = !strcmp( str, "true" );
	}

	// Setting the instance count will create all the values.
	SetInstances( instances );

	// Apply any differences from the saved XML to the values
	TiXmlElement const* child = _ccElement->FirstChildElement();
	while( child )
	{
		str = child->Value();
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

	if( m_afterMark )
	{
		_ccElement->SetAttribute( "after_mark", "true" );
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
string CommandClass::ExtractValue
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

	// Deal with sign extension.  All values are signed
	string res;
	if( _data[1] & 0x80 )
	{
		res = "-";

		// MSB is signed
		if( size == 1 )
		{
			value |= 0xffffff00;
		}
		else if( size == 2 )
		{
			value |= 0xffff0000;
		}
	}

	// Convert the integer to a decimal string.  We avoid
	// using floats to prevent accuracy issues.
	char numBuf[12];

	if( precision == 0 )
	{
		// The precision is zero, so we can just print the number directly into the string.
		snprintf( numBuf, 12, "%d", (signed long)value );
		res = numBuf;
	}
	else
	{
		// We'll need to insert a decimal point and include any necessary leading zeros.

		// Fill the buffer with the value padded with leading zeros.
		snprintf( numBuf, 12, "%011d", (signed long)value );

		// Calculate the position of the decimal point in the buffer
		int32 decimal = 10-precision;

		// Shift the characters to make space for the decimal point.
		// We don't worry about overwriting any minus sign since that is
		// already written into the res string. While we're shifting, we 
		// also look for the real starting position of the number so we 
		// can copy it into the res string later.
		int32 start = -1;
		for( int32 i=0; i<decimal; ++i )
		{
			numBuf[i] = numBuf[i+1];
			if( ( start<0 ) && ( numBuf[i] != '0' ) )
			{
				start = i;
			}
		}
		if( start<0 )
		{
			start = decimal-1;
		}

		// Insert the decimal point
		struct lconv const* locale = localeconv();
		numBuf[decimal] = *(locale->decimal_point);

		// Copy the buffer into res
		res += &numBuf[start];
	}

	return res;
}

//-----------------------------------------------------------------------------
// <CommandClass::AppendValue>
// Add a value to a message as a sequence of bytes
//-----------------------------------------------------------------------------
void CommandClass::AppendValue
(
	Msg* _msg,
	string const& _value,
	uint8 const _scale
)const
{
	uint8 precision;
	uint8 size;
	int32 val = ValueToInteger( _value, &precision, &size );

	_msg->Append( (precision<<c_precisionShift) | (_scale<<c_scaleShift) | size );

	int32 shift = (size-1)<<3;
	for( int32 i=size; i>0; --i, shift-=8 )
	{
		_msg->Append( (uint8)(val >> shift) );
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::GetAppendValueSize>
// Get the number of bytes that would be added by a call to AppendValue
//-----------------------------------------------------------------------------
uint8 const CommandClass::GetAppendValueSize
(
	string const& _value
)const
{
	uint8 size;
	ValueToInteger( _value, NULL, &size );
	return size;
}

//-----------------------------------------------------------------------------
// <CommandClass::ValueToInteger>
// Convert a decimal string to an integer and report the precision and
// number of bytes required to store the value.
//-----------------------------------------------------------------------------
int32 CommandClass::ValueToInteger
(
	string const& _value,
	uint8* o_precision,
	uint8* o_size
)const
{
	int32 val;
	
	// Find the decimal point
	int32 pos = _value.find_first_of( "." );
	if( pos == string::npos )
	{
		// No decimal point
		if( o_precision )
		{
			*o_precision = 0;
		}

		// Convert the string to an integer
		val = atol( _value.c_str() );
	}
	else
	{
		// Remove the decimal point and convert to an integer
		if( o_precision )
		{
			*o_precision = (_value.size()-pos)-1;
		}

		string str = _value.substr( 0, pos ) + _value.substr( pos+1 );
		val = atol( str.c_str() );
	}

	if( o_size )
	{
		// Work out the size as either 1, 2 or 4 bytes
		*o_size = 4;
		if( val < 0 )
		{
			if( ( val & 0xffffff80 ) == 0xffffff80 )
			{
				*o_size = 1;
			}
			else if( ( val & 0xffff8000 ) == 0xffff8000 )
			{
				*o_size = 2;
			}
		}
		else
		{
			if( ( val & 0xffffff00 ) == 0 )
			{
				*o_size = 1;
			}
			else if( ( val & 0xffff0000 ) == 0 )
			{
				*o_size = 2;
			}
		}
	}

	return val;
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
