//-----------------------------------------------------------------------------
//
//	Configuration.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CONFIGURATION
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

#include "CommandClasses.h"
#include "Configuration.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"
#include "ValueBool.h"
#include "ValueByte.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueShort.h"

using namespace OpenZWave;

enum ConfigurationCmd
{
	ConfigurationCmd_Set	= 0x04,
	ConfigurationCmd_Get	= 0x05,
	ConfigurationCmd_Report	= 0x06
};

//-----------------------------------------------------------------------------
// <Configuration::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Configuration::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (ConfigurationCmd_Report == (ConfigurationCmd)_data[0])
	{
		// Extract the parameter index and value
		uint8 parameter = _data[1];
		uint8 size = _data[2] & 0x07;
		int32 paramValue = 0;
		for( uint8 i=0; i<size; ++i )
		{
			paramValue <<= 8;
			paramValue |= (int32)_data[i+3];
		}

		if ( Value* value = GetValue( 1, parameter ) ) 
		{
			switch ( value->GetID().GetType() ) 
			{
				case ValueID::ValueType_Byte:
				{
					ValueByte* valueByte = static_cast<ValueByte*>( value );
					valueByte->OnValueChanged( (uint8)paramValue );
					break;
				}
				case ValueID::ValueType_Short:
				{
					ValueShort* valueShort = static_cast<ValueShort*>( value );
					valueShort->OnValueChanged( (int16)paramValue );
					break;
				}
				case ValueID::ValueType_Int:
				{
					ValueInt* valueInt = static_cast<ValueInt*>( value );
					valueInt->OnValueChanged( paramValue );
					break;
				}
				default:
				{
					Log::Write( "Invalid type for configuration parameter %d", parameter );
				}
			}
		}
		else
		{
			char label[16];
			snprintf( label, 16, "Parameter #%d", parameter );

			// Create a new value
			if( Node* node = GetNodeUnsafe() )
			{
				switch( size )
				{
					case 1:
					{
						node->CreateValueByte( ValueID::ValueGenre_Config, GetCommandClassId(), _instance, parameter, label, "", false, (uint8)paramValue );
						break;
					}
					case 2:
					{
						node->CreateValueShort( ValueID::ValueGenre_Config, GetCommandClassId(), _instance, parameter, label, "", false, (int16)paramValue );
						break;
					}
					case 4:
					{
						node->CreateValueInt( ValueID::ValueGenre_Config, GetCommandClassId(), _instance, parameter, label, "", false, (int32)paramValue );
						break;
					}
					default:
					{
						Log::Write( "Invalid size of %d bytes for configuration parameter %d", size, parameter );
					}
				}
			}
		}

		Log::Write( "Received Configuration report from node %d: Parameter=%d, Value=%d", GetNodeId(), parameter, paramValue );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Configuration::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
bool Configuration::SetValue
(
	Value const& _value
)
{
	uint8 param = _value.GetID().GetIndex();
	switch( _value.GetID().GetType() )
	{
		case ValueID::ValueType_Byte:
		{
			ValueByte const& valueByte = static_cast<ValueByte const&>( _value );
			Set( param, (int32)valueByte.GetValue() );
			return true;
		}
		case ValueID::ValueType_Short:
		{
			ValueShort const& valueShort = static_cast<ValueShort const&>( _value );
			Set( param, (int32)valueShort.GetValue() );
			return true;
		}
		case ValueID::ValueType_Int:
		{
			ValueInt const& valueInt = static_cast<ValueInt const&>( _value );
			Set( param, valueInt.GetValue() );
			return true;
		}
		default:
		{
		}
	}

	Log::Write( "Configuration::Set failed (bad value or value type) - Node=%d, Parameter=%d", GetNodeId(), param );
	return false;
}

//-----------------------------------------------------------------------------
// <Configuration::RequestValue>												   
// Request current parameter value from the device									   
//-----------------------------------------------------------------------------
void Configuration::RequestValue
(
	uint8 const _parameter
)
{
	Msg* msg = new Msg( "ConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( ConfigurationCmd_Get );
	msg->Append( _parameter );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Configuration::Set>
// Set the device's 
//-----------------------------------------------------------------------------
void Configuration::Set
(
	uint8 const _parameter,
	int32 const _value
)
{
	Log::Write( "Configuration::Set - Node=%d, Parameter=%d, Value=%d", GetNodeId(), _parameter, _value );

	int size = 4;
	if( _value < 0 )
	{
		if( ( _value & 0xffffff80 ) == 0xffffff80 )
		{
			size = 1;
		}
		else if( ( _value & 0xffff8000 ) == 0xffff8000 )
		{
			size = 2;
		}
	}
	else
	{
		if( ( _value & 0xffffff00 ) == 0 )
		{
			size = 1;
		}
		else if( ( _value & 0xffff0000 ) == 0 )
		{
			size = 2;
		}
	}

	Msg* msg = new Msg( "ConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 4 + size );
	msg->Append( GetCommandClassId() );
	msg->Append( ConfigurationCmd_Set );
	msg->Append( _parameter );
	msg->Append( size );
	if( size > 2 )
	{
		msg->Append( (uint8)(_value>>24) );
		msg->Append( (uint8)(_value>>16) );
	}
	if( size > 1 ) 
	{
		msg->Append( (uint8)(_value>>8) );
	}
	msg->Append( (uint8)_value );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}


