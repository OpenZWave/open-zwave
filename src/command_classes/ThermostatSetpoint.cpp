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
#include "ThermostatSetpoint.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueDecimal.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum ThermostatSetpointCmd
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
	Msg* pMsg = new Msg( "Request Supported Thermostat Setpoints", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	pMsg->Append( GetNodeId() );
	pMsg->Append( 2 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( ThermostatSetpointCmd_SupportedGet );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( pMsg );
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
			Msg* pMsg = new Msg( "Request Current Thermostat Setpoint", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
			pMsg->Append( GetNodeId() );
			pMsg->Append( 3 );
			pMsg->Append( GetCommandClassId() );
			pMsg->Append( ThermostatSetpointCmd_Get );
			pMsg->Append( i );
			pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			Driver::Get()->SendMsg( pMsg );
		}
	}
}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatSetpoint::HandleMsg
(
	uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if( ThermostatSetpointCmd_Report == (ThermostatSetpointCmd)_pData[0] )
	{
		// We have received a thermostat setpoint value from the Z-Wave device
		ThermostatSetpointEnum setpoint = (ThermostatSetpointEnum)_pData[1];
		
		uint8 scale;
		float32 temperature = ExtractValue( &_pData[2], &scale );
		m_setpoints[setpoint] = temperature;
		m_scales[setpoint] = (ThermostatSetpointScaleEnum)scale; 
		
		// Send an xPL message reporting the setpoint value
		return true;
	}
	else if( _pData[1] == ThermostatSetpointCmd_SupportedReport )
	{
		// We have received the supported thermostat setpoints from the Z-Wave device
		for( uint8 i=0; i<ThermostatSetpoint_Count; ++i )
		{
			m_supportedSetpoints[i] = ( ( _pData[2] & (1<<i) ) != 0 );
			if( m_supportedSetpoints[i] )
			{
				Node* pNode = GetNode();
				if( pNode )
				{
					ValueStore* pStore = pNode->GetValueStore();
					if( pStore )
					{
						Value* pValue = new ValueDecimal( GetNodeId(), GetCommandClassId(), _instance, i, c_setpointName[i], false, "0.0"  );
						pStore->AddValue( pValue );
						pValue->Release();
					}
				}
			}
		}
		return true;
	}

	// Not handled
	return false;
}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::Set>
// Set a thermostat setpoint temperature
//-----------------------------------------------------------------------------
void ThermostatSetpoint::Set
(
	string const& setpoint,
	float32 const _temperature,
	ThermostatSetpointScaleEnum const _scale
)
{
	for( int32 i=0; i<ThermostatSetpoint_Count; ++i )
	{
		if( !_stricmp( setpoint.c_str(), c_setpointName[i] ) )
		{
			Msg* pMsg = new Msg( "Set Thermostat Setpoint", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
			pMsg->Append( GetNodeId() );
			pMsg->Append( 3 + GetAppendValueSize( _temperature, 0 ) );
			pMsg->Append( GetCommandClassId() );
			pMsg->Append( ThermostatSetpointCmd_Set );
			pMsg->Append( i );
			AppendValue( pMsg, _temperature, 0, (uint8)_scale );
			pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			Driver::Get()->SendMsg( pMsg );
			return;
		}
	}
}
