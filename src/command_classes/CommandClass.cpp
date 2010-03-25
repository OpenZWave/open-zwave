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
#include "Driver.h"
#include "Manager.h"
#include "Log.h"

using namespace OpenZWave;

static uint8 const	c_sizeMask			= 0x07;
static uint8 const	c_scaleMask			= 0x18;
static uint8 const	c_scaleShift		= 0x03;
static uint8 const	c_precisionMask		= 0xe0;
static uint8 const	c_precisionShift	= 0x05;


//-----------------------------------------------------------------------------
// <CommandClass::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* CommandClass::GetDriver
(
)const
{
	return( Manager::Get()->GetDriver( m_driverId ) );
}

//-----------------------------------------------------------------------------
// <CommandClass::GetNode>
// Get a pointer to our node
//-----------------------------------------------------------------------------
Node* CommandClass::GetNode
(
)const
{
	return( GetDriver()->GetNode( m_nodeId ) );
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
		// Enlarge the polled instance array
		bool* newPolledInstances = new bool[_instances];
		memcpy( newPolledInstances, m_polledInstances, sizeof(bool)*m_instances );
		delete [] m_polledInstances;
		m_polledInstances = newPolledInstances;

		// Create the new value instances
		for( uint8 i=m_instances; i<_instances; ++i )
		{
			m_polledInstances[i] = false;
			CreateVars( i+1 );
			RequestState( i+1 );
		}

		m_instances = _instances;
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::SaveStatic>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void CommandClass::SaveStatic
( 
	FILE* _file	
)
{
	fprintf( _file, "	<CommandClass id=\"%d\" name=\"%s\" version=\"%d\" />\n", GetNodeId(), GetCommandClassId(), GetCommandClassName().c_str(), GetVersion() );
}

//-----------------------------------------------------------------------------
// <CommandClass::Poll>
// Request the state of any instance marked for polling
//-----------------------------------------------------------------------------
void CommandClass::Poll
( 
)
{
	for( uint8 i=0; i<m_instances; ++i )
	{
		if( m_polledInstances[i] )
		{
			RequestState( i+1 );
		}
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::SetPolled>
// Set the polled state of an instance of this command class
//-----------------------------------------------------------------------------
void CommandClass::SetPolled
(
	uint8 const _instance,
	bool const _state
)
{
	if( ( _instance == 0 ) || ( _instance > m_instances ) )
	{
		return;
	}

	m_polledInstances[_instance-1] = _state;
}

//-----------------------------------------------------------------------------
// <CommandClass::RequiresPolling>
// Return whether any instance is set to be polled
//-----------------------------------------------------------------------------
bool CommandClass::RequiresPolling
( 
)
{
	for( uint8 i=0; i<m_instances; ++i )
	{
		if( m_polledInstances[i] )
		{
			return true;
		}
	}

	return false;
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
