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
	"Unused 0",
	"Heating 1",
	"Cooling 1",
	"Unused 3",
	"Unused 4",
	"Unused 5",
	"Unused 6",
	"Furnace",
	"Dry Air",
	"Moist Air",
	"Auto Changeover",
	"Heating Econ",
	"Cooling Econ",
	"Away Heating"
};


//-----------------------------------------------------------------------------
// <ThermostatSetpoint::ThermostatSetpoint>
// Constructor
//-----------------------------------------------------------------------------
ThermostatSetpoint::ThermostatSetpoint
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId )
{
	memset( m_supportedSetpoints, 0, sizeof(bool)*ThermostatSetpoint_Count );
}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::RequestState>
// Get the static thermostat setpoint details from the device
//-----------------------------------------------------------------------------
void ThermostatSetpoint::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Static )
	{
		// Request the supported setpoints
		Msg* msg = new Msg( "Request Supported Thermostat Setpoints", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatSetpointCmd_SupportedGet );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}

	if( _requestFlags & RequestFlag_Session )
	{
		for( uint8 i=0; i<ThermostatSetpoint_Count; ++i )
		{
			if( m_supportedSetpoints[i] )
			{
				// Request the setpoint value
				Msg* msg = new Msg( "Request Current Thermostat Setpoint", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
				msg->Append( GetNodeId() );
				msg->Append( 3 );
				msg->Append( GetCommandClassId() );
				msg->Append( ThermostatSetpointCmd_Get );
				msg->Append( i );
				msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
				GetDriver()->SendMsg( msg );
			}
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
	uint32 const _instance	// = 1
)
{
	bool handled = false;

	if( Node* node = GetNode() )
	{
		if( ThermostatSetpointCmd_Report == (ThermostatSetpointCmd)_data[0] )
		{
			// We have received a thermostat setpoint value from the Z-Wave device
			if( ValueDecimal* value = node->GetValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, _data[1] ) )
			{
				uint8 scale;
				string temperature = ExtractValueAsString( &_data[2], &scale );

				value->SetUnits( scale ? "F" : "C" );
				value->OnValueChanged( temperature );

				Log::Write( "Received thermostat setpoint report from node %d: Setpoint %s = %s%s", GetNodeId(), value->GetLabel().c_str(), value->GetAsString().c_str(), value->GetUnits().c_str() );		
			}
			handled = true;
		}
		else if( ThermostatSetpointCmd_SupportedReport == (ThermostatSetpointCmd)_data[0] )
		{
			// We have received the supported thermostat setpoints from the Z-Wave device
			Log::Write( "Received supported thermostat setpoints from node %d", GetNodeId() );		

			// Parse the data for the supported setpoints
			memset( m_supportedSetpoints, 0, sizeof(bool)*ThermostatSetpoint_Count );			

			for( uint32 i=1; i<_length-1; ++i )
			{
				for( int32 bit=0; bit<8; ++bit )
				{
					if( ( _data[i] & (1<<bit) ) != 0 )
					{
						// Add supported setpoint
						int32 index = (int32)((i-1)<<3) + bit;
						if( index < ThermostatSetpoint_Count )
						{
							m_supportedSetpoints[index] = true;
							node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, index, c_setpointName[index], "C", false, "0.0"  );

							Log::Write( "    Added setpoint: %s", c_setpointName[index] );
						}
					}
				}
			}

			// Request the current state of all the supported setpoints
			RequestState( 0 );
			handled = true;
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
	if( ValueID::ValueType_Decimal == _value.GetID().GetType() )
	{
		ValueDecimal const* value = static_cast<ValueDecimal const*>(&_value);
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
		GetDriver()->SendMsg( msg );
		return true;
	}

	return false;
}
