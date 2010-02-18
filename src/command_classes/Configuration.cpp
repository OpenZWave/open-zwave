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
#include "ValueByte.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueStore.h"

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
	uint32 const _instance	// = 0
)
{
	if (ConfigurationCmd_Report == (ConfigurationCmd)_data[0])
	{
		// Extract the parameter index and value
		uint8 parameter = _data[1];
		uint8 size = _data[2] & 0x07;
		uint32 paramValue = 0;
		for( uint8 i=0; i<size; ++i )
		{
			paramValue <<= 8;
			paramValue |= (uint32)_data[i+3];
		}

		// Get the value object for this parameter, or create one if it does not yet exist
		ValueID id( GetNodeId(), GetCommandClassId(), _instance, parameter );

		Node* node = GetNode();
		if( node )
		{
			ValueStore* store = node->GetValueStore();
			if( store )
			{
				if( Value* value = store->GetValue( id ) )
				{
					// Cast the value to the correct type, and change 
					// its value to the one we just received.
					uint8 const valueType = value->GetValueTypeId();
					if( valueType == ValueByte::StaticGetValueTypeId() )
					{
						ValueByte* valueByte = static_cast<ValueByte*>( value );
						valueByte->OnValueChanged( (uint8)paramValue );
					}
					else if( valueType == ValueInt::StaticGetValueTypeId() )
					{
						ValueInt* valueInt = static_cast<ValueInt*>( value );
						valueInt->OnValueChanged( paramValue );
					}
					else if( valueType == ValueList::StaticGetValueTypeId() )
					{
						ValueList* valueList = static_cast<ValueList*>( value );
						int32 valueIdx = valueList->GetItemIdxByValue( paramValue );
						if( valueIdx >= 0 )
						{
							valueList->OnValueChanged( valueIdx );
						}
					}
				}
				else
				{
					// Create a new value
					char label[16];
					snprintf( label, 16, "Parameter #%d", parameter );
					ValueInt* valueInt = new ValueInt( GetNodeId(), GetCommandClassId(), _instance, parameter, Value::Genre_Config, label, false, paramValue );
					store->AddValue( valueInt );
				}

				node->ReleaseValueStore();
			}
		}

		Log::Write( "Received Configuration report from node %d: Parameter=%d, Value=%d", GetNodeId(), parameter, paramValue );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Configuration::Get>												   
// Request current parameter value from the device									   
//-----------------------------------------------------------------------------
void Configuration::Get
(
	uint8 const _parameter
)
{
	Msg* msg = new Msg( "ConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( ConfigurationCmd_Get );
	msg->Append( _parameter );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
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

	Msg* msg = new Msg( "ConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	if( _value & 0xffff0000 ) 
	{
		// four byte value
		msg->Append( GetNodeId() );
		msg->Append( 8 );
		msg->Append( GetCommandClassId() );
		msg->Append( ConfigurationCmd_Set );
		msg->Append( _parameter );
		msg->Append( 4 );
		msg->Append( (uint8)((_value>>24)&0xff) );
		msg->Append( (uint8)((_value>>16)&0xff) );
		msg->Append( (uint8)((_value>>8)&0xff) );
		msg->Append( (_value & 0xff) );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	}
	else if( _value & 0x0000ff00 )
	{
		// two byte value
		msg->Append( GetNodeId() );
		msg->Append( 6 );
		msg->Append( GetCommandClassId() );
		msg->Append( ConfigurationCmd_Set );
		msg->Append( _parameter );
		msg->Append( 2 );
		msg->Append( (uint8)((_value>>8)&0xff) );
		msg->Append( (uint8)(_value&0xff) );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	}
	else
	{
		// one byte value
		msg->Append( GetNodeId() );
		msg->Append( 5 );
		msg->Append( GetCommandClassId() );
		msg->Append( ConfigurationCmd_Set );
		msg->Append( _parameter );
		msg->Append( 1 );
		msg->Append( (uint8)_value );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	}
}

