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

using namespace OpenZWave;

enum ThermostatOperatingStateCmd
{
	ThermostatOperatingStateCmd_Get				= 0x02,
	ThermostatOperatingStateCmd_Report			= 0x03,
	ThermostatOperatingStateCmd_SupportedGet	= 0x04,
	ThermostatOperatingStateCmd_SupportedReport	= 0x05
};

static char const* c_stateName[] = 
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
// <ThermostatOperatingState::RequestState>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatOperatingState::RequestState
(
	uint32 const _requestFlags
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		// Request the supported states
		Msg* msg = new Msg( "Request Supported Thermostat Operating States", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatOperatingStateCmd_SupportedGet );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg);
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		// Request the current state
		Msg* msg = new Msg( "Request Current Thermostat Operating State", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatOperatingStateCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatOperatingState::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( ThermostatOperatingStateCmd_Report == (ThermostatOperatingStateCmd)_data[0] )
	{
		// We have received the thermostat operating state from the Z-Wave device
		if( !m_supportedStates.empty() )
		{
			if( ValueList* valueList = m_state.GetInstance( _instance ) )
			{
				valueList->OnValueChanged( _data[1] );
				Log::Write( "Received thermostat operating state from node %d: %s", GetNodeId(), valueList->GetItem().m_label.c_str() );		
				valueList->Release();
			}
		}
		return true;
	}
	
	if( ThermostatOperatingStateCmd_SupportedReport == (ThermostatOperatingStateCmd)_data[0] )
	{
		// We have received the supported thermostat operating states from the Z-Wave device
		Log::Write( "Received supported thermostat operating states from node %d", GetNodeId() );		

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

					Log::Write( "    Added operating state: %s", c_stateName[item.m_value] );
				}
			}
		}

		ClearStaticRequest( StaticRequest_Values );
		CreateVars( _instance );
		return true;
	}

	return false;
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

	if( Node* node = GetNode() )
	{
		m_state.AddInstance( _instance, node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "State", "", true, m_supportedStates, m_supportedStates[0].m_value ) );
		ReleaseNode();
	}
}

