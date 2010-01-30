//-----------------------------------------------------------------------------
//
//	ThermostatMode.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_MODE
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
#include "ThermostatMode.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum ThermostatModeCmd
{
	ThermostatModeCmd_Set				= 0x01,
	ThermostatModeCmd_Get				= 0x02,
	ThermostatModeCmd_Report			= 0x03,
	ThermostatModeCmd_SupportedGet		= 0x04,
	ThermostatModeCmd_SupportedReport	= 0x05
};

static char* const c_modeName[] = 
{
	"Off",
	"Heat",
	"Cool",
	"Auto",
	"Aux Heat",
	"Resume",
	"Fan Only",
	"Furnace",
	"Dry Air",
	"Moist Air",
	"Auto Changeover"
};


//-----------------------------------------------------------------------------
// <ThermostatMode::RequestStatic>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatMode::RequestStatic
(
)
{
	// Request the supported modes
	Msg* pMsg = new Msg( "Request Supported Thermostat Modes", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	pMsg->Append( GetNodeId() );
	pMsg->Append( 2 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( ThermostatModeCmd_SupportedGet );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <ThermostatMode::RequestState>
// Get the dynamic thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatMode::RequestState
(
)
{
	// Request the current mode
	Msg* pMsg = new Msg( "Request Current Thermostat Mode", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	pMsg->Append( GetNodeId() );
	pMsg->Append( 2 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( ThermostatModeCmd_Get );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <ThermostatMode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatMode::HandleMsg
(
	uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	bool handled = false;
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			if( ThermostatModeCmd_Report == (ThermostatModeCmd)_pData[0] )
			{
				// We have received the thermostat mode from the Z-Wave device
				if( ValueList* pValueList = static_cast<ValueList*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					pValueList->OnValueChanged( c_modeName[_pData[1]] );
				}
				handled = true;
			}
			else if( _pData[1] == ThermostatModeCmd_SupportedReport )
			{
				// We have received the supported thermostat modes from the Z-Wave device
				m_supportedModes.clear();
				for( uint32 i=2; i<_length; ++i )
				{
					for( int32 bit=0; bit<8; ++bit )
					{
						if( ( _pData[i] & (1<<bit) ) != 0 )
						{
							m_supportedModes.push_back( c_modeName[i+bit-2] );
						}
					}
				}

				CreateVars( _instance );
				handled = true;
			}

			pNode->ReleaseValueStore();
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <ThermostatMode::SetValue>
// Set the device's thermostat mode
//-----------------------------------------------------------------------------
bool ThermostatMode::SetValue
(
	Value const& _value
)
{
	if( ValueList const* pValue = static_cast<ValueList const*>(&_value) )
	{
		// Convert the selected option to an index
		uint8 state = 0;
		for( int i=0; i<4; ++i )
		{
			if( !strcmp( c_modeName[i], pValue->GetPending().c_str() ) )
			{
				state = i;
				break;
			}
		}

		Msg* pMsg = new Msg( "Set Thermostat Mode", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		pMsg->Append( GetNodeId() );
		pMsg->Append( 3 );
		pMsg->Append( GetCommandClassId() );
		pMsg->Append( ThermostatModeCmd_Set );
		pMsg->Append( state );
		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( pMsg );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ThermostatMode::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ThermostatMode::CreateVars
(
	uint8 const _instance
)
{
	if( m_supportedModes.empty() )
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
			
			pValue = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Mode", false, m_supportedModes, m_supportedModes[0] );
			pStore->AddValue( pValue );
			pValue->Release();

			pNode->ReleaseValueStore();
		}
	}
}
