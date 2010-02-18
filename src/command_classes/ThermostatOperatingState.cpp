//-----------------------------------------------------------------------------
//
//	ThermostatOperatingState.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_OPERATING_STATE
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
#include "ThermostatOperatingState.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum ThermostatOperatingStateCmd
{
	ThermostatOperatingStateCmd_Get				= 0x02,
	ThermostatOperatingStateCmd_Report			= 0x03,
	ThermostatOperatingStateCmd_SupportedGet	= 0x04,
	ThermostatOperatingStateCmd_SupportedReport	= 0x05
};

static char* const c_stateName[] = 
{
	"Idle",
	"Heating",
	"Cooling",
	"FanOnly",
	"PendingHeat",
	"PendingCool",
	"VentOrEconomizer"
};


//-----------------------------------------------------------------------------
// <ThermostatOperatingState::RequestStatic>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatOperatingState::RequestStatic
(
)
{
	// Request the supported modes
	Msg* msg = new Msg( "Request Supported Thermostat Operating States", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ThermostatOperatingStateCmd_SupportedGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg);
}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::RequestState>
// Get the dynamic thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatOperatingState::RequestState
(
)
{
	// Request the current mode
	Msg* msg = new Msg( "Request Current Thermostat Operating State", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ThermostatOperatingStateCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatOperatingState::HandleMsg
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
			if( ThermostatOperatingStateCmd_Report == (ThermostatOperatingStateCmd)_data[0] )
			{
				// We have received the thermostat operating state from the Z-Wave device
				if( ValueList* valueList = static_cast<ValueList*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					valueList->OnValueChanged( _data[1] );
				}
				handled = true;
			}
			else if( _data[1] == ThermostatOperatingStateCmd_SupportedReport )
			{
				// We have received the supported thermostat modes from the Z-Wave device
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
// <ThermostatOperatingState::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ThermostatOperatingState::CreateVars
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

