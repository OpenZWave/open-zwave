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

#include <math.h>
#include "tinyxml.h"
#include "CommandClass.h"
#include "Msg.h"
#include "Driver.h"
#include "Log.h"

using namespace OpenZWave;

static uint8 const	c_sizeMask			= 0x07;
static uint8 const	c_scaleMask			= 0x18;
static uint8 const	c_scaleShift		= 0x07;
static uint8 const	c_precisionMask		= 0xe0;
static uint8 const	c_precisionShift	= 0x05;


//-----------------------------------------------------------------------------
// <CommandClass::GetNode>
// Get a pointer to our node
//-----------------------------------------------------------------------------
Node* CommandClass::GetNode
(
)
{
	return( Driver::Get()->GetNode( m_nodeId ) );
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
			CreateVars( i );
		}
	}

	m_instances = _instances;
}

//-----------------------------------------------------------------------------
// <CommandClass::SaveStatic>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void CommandClass::SaveStatic
( 
	FILE* _pFile	
)
{
	fprintf( _pFile, "    <CommandClass id=\"%d\" name=\"%s\" version=\"%d\" />\n", GetNodeId(), GetCommandClassId(), GetCommandClassName().c_str(), GetVersion() );
}

//-----------------------------------------------------------------------------
// <CommandClass::ExtractValue>
// Read a value from a variable length sequence of bytes
//-----------------------------------------------------------------------------
float32 CommandClass::ExtractValue
(
	uint8 const* _pData,
	uint8* _pScale
)const
{
	uint8 const size = _pData[0] & c_sizeMask;
	uint8 const precision = (_pData[0] & c_precisionMask) >> c_precisionShift;

	if( _pScale )
	{
		*_pScale = (_pData[0] & c_scaleMask) >> c_scaleShift;
	}

	uint32 value = 0;
	for( uint8 i=0; i<size; ++i )
	{
		value <<= 8;
		value |= (uint32)_pData[i];
	}

	if( precision == 0 )
	{
		return (float32)(signed long)value;
	}

	return ((float32)(signed long)value) / pow( 10.0f, precision );
}

//-----------------------------------------------------------------------------
// <CommandClass::AppendValue>
// Add a value to a message as a sequence of bytes
//-----------------------------------------------------------------------------
void CommandClass::AppendValue
(
	Msg* _pMsg,
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

	_pMsg->Append( (_precision<<c_precisionShift) | (_scale<<c_scaleShift) | size );

	for( int32 i=size-1; i>=0; --i )
	{
		_pMsg->Append( (uint8)((iValue >> (size<<3)) & 0xff) );
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
