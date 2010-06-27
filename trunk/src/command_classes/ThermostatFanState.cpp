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
#include "ThermostatFanState.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"

using namespace OpenZWave;

enum ThermostatFanStateCmd
{
	ThermostatFanStateCmd_Get				= 0x02,
	ThermostatFanStateCmd_Report			= 0x03,
	ThermostatFanStateCmd_SupportedGet		= 0x04,
	ThermostatFanStateCmd_SupportedReport	= 0x05
};

static char const* c_stateName[] = 
{
	"Idle",
	"Running Low",
	"Running High"
};


//-----------------------------------------------------------------------------
// <ThermostatFanState::RequestState>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatFanState::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Static )
	{
		// Request the supported states
		Msg* msg = new Msg( "Request Supported Thermostat Fan States", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatFanStateCmd_SupportedGet );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		// Request the current state
		Msg* msg = new Msg( "Request Current Thermostat Fan State", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatFanStateCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <ThermostatFanState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatFanState::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( ThermostatFanStateCmd_Report == (ThermostatFanStateCmd)_data[0] )
	{
		// We have received the thermostat fan state from the Z-Wave device
		if( ValueList* valueList = m_state.GetInstance( _instance ) )
		{
			valueList->OnValueChanged( _data[1] );
			Log::Write( "Received thermostat fan state from node %d: %s", GetNodeId(), valueList->GetItem().m_label.c_str() );		
			valueList->Release();
		}
		return true;
	}
	
	if( ThermostatFanStateCmd_SupportedReport == (ThermostatFanStateCmd)_data[0] )
	{
		// We have received the supported thermostat fan states from the Z-Wave device
		Log::Write( "Received supported thermostat fan states from node %d", GetNodeId() );		

		m_supportedStates.clear();
		for( uint32 i=1; i<_length-1; ++i )
		{
			for( int32 bit=0; bit<8; ++bit )
			{
				if( ( _data[i] & (1<<bit) ) != 0 )
				{
					ValueList::Item item;
					item.m_value = (int32)((i-1)<<3) + bit;
					item.m_label = c_stateName[item.m_value];
					m_supportedStates.push_back( item );

					Log::Write( "    Added fan state: %s", c_stateName[item.m_value] );
				}
			}
		}

		CreateVars( _instance );
		return true;
	}
		
	return false;
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

	if( Node* node = GetNode() )
	{
		m_state.AddInstance( _instance, node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "State", "", true, m_supportedStates, m_supportedStates[0].m_value ) );
		ReleaseNode();
	}
}

