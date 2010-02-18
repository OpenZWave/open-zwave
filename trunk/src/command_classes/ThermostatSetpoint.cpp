//-----------------------------------------------------------------------------
//
//	ThermostatSetpoint.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_SETPOINT
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
#include "ThermostatSetpoint.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueDecimal.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum ThermostatSetpointCmd
{
	ThermostatSetpointCmd_Set				= 0x01,
	ThermostatSetpointCmd_Get				= 0x02,
	ThermostatSetpointCmd_Report			= 0x03,
	ThermostatSetpointCmd_SupportedGet		= 0x04,
	ThermostatSetpointCmd_SupportedReport	= 0x05
};

static char* const c_setpointName[] = 
{
	"Heating #1",
	"Cooling #1",
	"Furnace",
	"Dry Air",
	"Moist Air",
	"Auto Changeover"
};


//-----------------------------------------------------------------------------
// <ThermostatSetpoint::RequestStatic>
// Get the static thermostat setpoint details from the device
//-----------------------------------------------------------------------------
void ThermostatSetpoint::RequestStatic
(
)
{
	// Request the supported setpoints
	Msg* msg = new Msg( "Request Supported Thermostat Setpoints", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ThermostatSetpointCmd_SupportedGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::RequestState>
// Get the dynamic thermostat setpoint details from the device
//-----------------------------------------------------------------------------
void ThermostatSetpoint::RequestState
(
)
{
	for( uint8 i=0; i<ThermostatSetpoint_Count; ++i )
	{
		if( m_supportedSetpoints[i] )
		{
			// Request the setpoint value
			Msg* msg = new Msg( "Request Current Thermostat Setpoint", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( ThermostatSetpointCmd_Get );
			msg->Append( i );
			msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			Driver::Get()->SendMsg( msg );
		}
	}
}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatSetpoint::HandleMsg
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
			if( ThermostatSetpointCmd_Report == (ThermostatSetpointCmd)_data[0] )
			{
				// We have received a thermostat setpoint value from the Z-Wave device
				if( ValueDecimal* value = static_cast<ValueDecimal*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, _data[1] ) ) ) )
				{
					uint8 scale;
					string temperature = ExtractValueAsString( &_data[2], &scale );

					value->SetUnits( scale ? "F" : "C" );
					value->OnValueChanged( temperature );
				}
				handled = true;
			}
			else if( _data[1] == ThermostatSetpointCmd_SupportedReport )
			{
				// We have received the supported thermostat setpoints from the Z-Wave device
				for( uint8 i=0; i<ThermostatSetpoint_Count; ++i )
				{
					m_supportedSetpoints[i] = (( _data[2] & (1<<i) ) != 0 );
					if( m_supportedSetpoints[i] )
					{
						Value* value = new ValueDecimal( GetNodeId(), GetCommandClassId(), _instance, i, Value::Genre_User, c_setpointName[i], false, "0.0"  );
						store->AddValue( value );
						value->Release();
					}
				}
				handled = true;
			}

			node->ReleaseValueStore();
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::SetValue>
// Set a thermostat setpoint temperature
//-----------------------------------------------------------------------------
bool ThermostatSetpoint::SetValue
(
	Value const& _value
)
{
	if( ValueDecimal const* value = static_cast<ValueDecimal const*>(&_value) )
	{
		float32 floatVal = (float32)atof( value->GetPending().c_str() );
		uint8 scale = strcmp( "C", value->GetUnits().c_str() ) ? 1 : 0;

		Msg* msg = new Msg( "Set Thermostat Setpoint", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 3 + GetAppendValueSize( floatVal, 0 ) );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatSetpointCmd_Set );
		msg->Append( value->GetID().GetIndex() );
		AppendValue( msg, floatVal, 0, scale );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( msg );
		return true;
	}

	return false;
}
