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
#include "ThermostatMode.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum ThermostatModeCmd
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
	"Auto Changeover",
	"Heat Econ",
	"Cool Econ"
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
	Msg* msg = new Msg( "Request Supported Thermostat Modes", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ThermostatModeCmd_SupportedGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <ThermostatMode::RequestState>
// Get the dynamic thermostat mode details from the device
//-----------------------------------------------------------------------------
void ThermostatMode::RequestState
(
	bool const _poll
)
{
	// Request the current mode
	Msg* msg = new Msg( "Request Current Thermostat Mode", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ThermostatModeCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <ThermostatMode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatMode::HandleMsg
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
			if( ThermostatModeCmd_Report == (ThermostatModeCmd)_data[0] )
			{
				// We have received the thermostat mode from the Z-Wave device
				if( ValueList* valueList = static_cast<ValueList*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					valueList->OnValueChanged( _data[1] );
					Log::Write( "Received thermostat mode from node %d: %s", GetNodeId(), valueList->GetAsString().c_str() );		
				}
				handled = true;
			}
			else if( ThermostatModeCmd_SupportedReport == (ThermostatModeCmd)_data[0] )
			{
				// We have received the supported thermostat modes from the Z-Wave device
				Log::Write( "Received supported thermostat modes from node %d", GetNodeId() );		

				m_supportedModes.clear();
				for( uint32 i=1; i<_length; ++i )
				{
					for( int32 bit=0; bit<8; ++bit )
					{
						if( ( _data[i] & (1<<bit) ) != 0 )
						{
							ValueList::Item item;
							item.m_value = (int32)((i-1)<<3) + bit;
							item.m_label = c_modeName[item.m_value];
							m_supportedModes.push_back( item );

							Log::Write( "    Added mode: %s", c_modeName[item.m_value] );
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
// <ThermostatMode::SetValue>
// Set the device's thermostat mode
//-----------------------------------------------------------------------------
bool ThermostatMode::SetValue
(
	Value const& _value
)
{
	if( ValueList const* value = static_cast<ValueList const*>(&_value) )
	{
		uint8 state = (uint8)value->GetPending().m_value;

		Msg* msg = new Msg( "Set Thermostat Mode", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatModeCmd_Set );
		msg->Append( state );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( msg );
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

	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			Value* value;
			
			value = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Mode", false, m_supportedModes, 0 );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}
