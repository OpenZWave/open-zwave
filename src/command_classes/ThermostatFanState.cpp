//-----------------------------------------------------------------------------
//
//	ThermostatFanState.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_FAN_STATE
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
#include "ThermostatFanState.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum ThermostatFanStateCmd
{
	ThermostatFanStateCmd_Get				= 0x02,
	ThermostatFanStateCmd_Report			= 0x03,
	ThermostatFanStateCmd_SupportedGet		= 0x04,
	ThermostatFanStateCmd_SupportedReport	= 0x05
};

static char* const c_stateName[] = 
{
	"Idle",
	"Running Low",
	"Running High"
};


//-----------------------------------------------------------------------------
// <ThermostatFanState::RequestStatic>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatFanState::RequestStatic
(
)
{
	// Request the supported modes
	Msg* msg = new Msg( "Request Supported Thermostat Fan States", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ThermostatFanStateCmd_SupportedGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <ThermostatFanState::RequestState>
// Get the dynamic thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatFanState::RequestState
(
)
{
	// Request the current mode
	Msg* msg = new Msg( "Request Current Thermostat Fan State", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ThermostatFanStateCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <ThermostatFanState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatFanState::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	bool handled = false;

	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			if( ThermostatFanStateCmd_Report == (ThermostatFanStateCmd)_data[0] )
			{
				// We have received the thermostat fan state from the Z-Wave device
				if( ValueList* valueList = static_cast<ValueList*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					valueList->OnValueChanged( _data[1] );
				}
				handled = true;
			}
			else if( _data[1] == ThermostatFanStateCmd_SupportedReport )
			{
				// We have received the supported thermostat fan states from the Z-Wave device
				m_supportedStates.clear();
				for( uint32 i=2; i<_length; ++i )
				{
					for( int32 bit=0; bit<8; ++bit )
					{
						if( ( _data[i] & (1<<bit) ) != 0 )
						{
							ValueList::Item item;
							item.m_value = i + bit - 2;
							item.m_label = c_stateName[item.m_value];
							m_supportedStates.push_back( item );
						}
					}
				}

				CreateVars( _instance );
				handled = true;
			}

			node->ReleaseValueStore();
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <ThermostatFanState::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ThermostatFanState::CreateVars
(
	uint8 const _instance
)
{
	if( m_supportedStates.empty() )
	{
		return;
	}

	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			Value* value;
			
			value = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "State", true, m_supportedStates, 0 );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}

