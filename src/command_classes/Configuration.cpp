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

static enum ConfigurationCmd
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
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if (ConfigurationCmd_Report == (ConfigurationCmd)_pData[0])
    {
		// Extract the parameter index and value
		uint8 parameter = _pData[1];
		uint8 size = _pData[2] & 0x07;
		uint32 value = 0;
		for( uint8 i=0; i<size; ++i )
		{
			value <<= 8;
			value |= (uint32)_pData[i+3];
		}

		// Get the value object for this parameter, or create one if it does not yet exist
		ValueID id( GetNodeId(), GetCommandClassId(), _instance, parameter );

		Node* pNode = GetNode();
		if( pNode )
		{
			ValueStore* pStore = pNode->GetValueStore();
			if( pStore )
			{
				if( Value* pValue = pStore->GetValue( id ) )
				{
					// Cast the value to the correct type, and change 
					// its value to the one we just received.
					uint8 const valueType = pValue->GetValueTypeId();
					if( valueType == ValueByte::StaticGetValueTypeId() )
					{
						ValueByte* pValueByte = static_cast<ValueByte*>( pValue );
						pValueByte->OnValueChanged( (uint8)value );
					}
					else if( valueType == ValueInt::StaticGetValueTypeId() )
					{
						ValueInt* pValueInt = static_cast<ValueInt*>( pValue );
						pValueInt->OnValueChanged( value );
					}
					else if( valueType == ValueList::StaticGetValueTypeId() )
					{
						ValueList* pValueList = static_cast<ValueList*>( pValue );
						int32 valueIdx = pValueList->GetItemIdxByValue( value );
						if( valueIdx >= 0 )
						{
							pValueList->OnValueChanged( valueIdx );
						}
					}
				}
				else
				{
					// Create a new value
					char label[16];
					snprintf( label, 16, "Parameter #%d", parameter );
					ValueInt* pValueInt = new ValueInt( GetNodeId(), GetCommandClassId(), _instance, parameter, Value::Genre_Config, label, false, value );
					pStore->AddValue( pValueInt );
				}

				pNode->ReleaseValueStore();
			}
		}

		Log::Write( "Received Configuration report from node %d: Parameter=%d, Value=%d", GetNodeId(), parameter, (signed long)value );
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
    Msg* pMsg = new Msg( "ConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 3 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( ConfigurationCmd_Get );
    pMsg->Append( _parameter );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
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

	Msg* pMsg = new Msg( "ConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	if( _value & 0xffff0000 ) 
	{
		// four byte value
		pMsg->Append( GetNodeId() );
		pMsg->Append( 8 );
		pMsg->Append( GetCommandClassId() );
		pMsg->Append( ConfigurationCmd_Set );
		pMsg->Append( _parameter );
		pMsg->Append( 4 );
		pMsg->Append( (uint8)((_value>>24)&0xff) );
		pMsg->Append( (uint8)((_value>>16)&0xff) );
		pMsg->Append( (uint8)((_value>>8)&0xff) );
		pMsg->Append( (_value & 0xff) );
		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	}
	else if( _value & 0x0000ff00 )
	{
		// two byte value
		pMsg->Append( GetNodeId() );
		pMsg->Append( 6 );
		pMsg->Append( GetCommandClassId() );
		pMsg->Append( ConfigurationCmd_Set );
		pMsg->Append( _parameter );
		pMsg->Append( 2 );
		pMsg->Append( (uint8)((_value>>8)&0xff) );
		pMsg->Append( (uint8)(_value&0xff) );
		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	}
	else
	{
		// one byte value
		pMsg->Append( GetNodeId() );
		pMsg->Append( 5 );
		pMsg->Append( GetCommandClassId() );
		pMsg->Append( ConfigurationCmd_Set );
		pMsg->Append( _parameter );
		pMsg->Append( 1 );
		pMsg->Append( (uint8)_value );
		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	}
}

