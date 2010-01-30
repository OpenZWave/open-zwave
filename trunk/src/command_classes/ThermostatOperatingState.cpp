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
#include "ThermostatOperatingState.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum ThermostatOperatingStateCmd
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
	Msg* pMsg = new Msg( "Request Supported Thermostat Operating States", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	pMsg->Append( GetNodeId() );
	pMsg->Append( 2 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( ThermostatOperatingStateCmd_SupportedGet );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( pMsg);
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
	Msg* pMsg = new Msg( "Request Current Thermostat Operating State", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	pMsg->Append( GetNodeId() );
	pMsg->Append( 2 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( ThermostatOperatingStateCmd_Get );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatOperatingState::HandleMsg
(
	uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			if( ThermostatOperatingStateCmd_Report == (ThermostatOperatingStateCmd)_pData[0] )
			{
				// We have received the thermostat operating state from the Z-Wave device
				if( ValueList* pValueList = static_cast<ValueList*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					pValueList->OnValueChanged( c_stateName[_pData[1]] );
				}
				return true;
			}
			else if( _pData[1] == ThermostatOperatingStateCmd_SupportedReport )
			{
				// We have received the supported thermostat modes from the Z-Wave device
				m_supportedStates.clear();
				for( uint32 i=2; i<_length; ++i )
				{
					for( int32 bit=0; bit<8; ++bit )
					{
						if( ( _pData[i] & (1<<bit) ) != 0 )
						{
							m_supportedStates.push_back( c_stateName[i+bit-2] );
						}
					}
				}

				CreateVars( _instance );
				return true;
			}
		}
	}

	// Not handled
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

	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			Value* pValue;
			
			pValue = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, "State", true, m_supportedStates, m_supportedStates[0] );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}

