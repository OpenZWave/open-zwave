//-----------------------------------------------------------------------------
//
//	SwitchToggleMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_TOGGLE_MULTILEVEL
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
#include "SwitchToggleMultilevel.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum SwitchToggleMultilevelCmd
{
	SwitchToggleMultilevelCmd_Set				= 0x01,
	SwitchToggleMultilevelCmd_Get				= 0x02,
	SwitchToggleMultilevelCmd_Report			= 0x03,
	SwitchToggleMultilevelCmd_StartLevelChange	= 0x04,
	SwitchToggleMultilevelCmd_StopLevelChange	= 0x05
};


//-----------------------------------------------------------------------------
// <SwitchToggleMultilevel::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void SwitchToggleMultilevel::RequestState
(
)
{
	Msg* msg = new Msg( "SwitchToggleMultilevelCmd_StartLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchToggleMultilevelCmd_StartLevelChange );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <SwitchToggleMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchToggleMultilevel::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if (SwitchToggleMultilevelCmd_Report == (SwitchToggleMultilevelCmd)_data[0])
	{
		Node* node = GetNode();
		if( node )
		{
			ValueStore* store = node->GetValueStore();
			if( store )
			{
				if( ValueByte* value = static_cast<ValueByte*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					value->OnValueChanged( _data[1] );
				}
				node->ReleaseValueStore();

				Log::Write( "Received SwitchToggleMultiLevel report from node %d: level=%d", GetNodeId(), _data[1] );
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchToggleMultilevel::SetValue>
// Toggle the state of the switch
//-----------------------------------------------------------------------------
bool SwitchToggleMultilevel::SetValue
(
	Value const& _value
)
{
	Log::Write( "SwitchToggleMultilevel::Set - Toggling the state of node %d", GetNodeId() );
	Msg* msg = new Msg( "SwitchToggleMultilevel Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchToggleMultilevelCmd_Set );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchToggleMultilevel::StartLevelChange>
// Start the level changing
//-----------------------------------------------------------------------------
void SwitchToggleMultilevel::StartLevelChange
(
	SwitchToggleMultilevelDirection const _direction,
	bool const _bIgnoreStartLevel,
	bool const _bRollover
)
{
	uint8 param = (uint8)_direction;
	param |= ( _bIgnoreStartLevel ? 0x20 : 0x00 );
	param |= ( _bRollover ? 0x80 : 0x00 );

	Log::Write( "SwitchMultilevel::StartLevelChange - Starting a level change on node %d, Direction=%d, IgnoreStartLevel=%s and rollover=%s", GetNodeId(), (_direction==SwitchToggleMultilevelDirection_Up) ? "Up" : "Down", _bIgnoreStartLevel ? "True" : "False", _bRollover ? "True" : "False" );
	Msg* msg = new Msg( "SwitchMultilevel StartLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchToggleMultilevelCmd_StartLevelChange );
	msg->Append( param );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchToggleMultilevel::StopLevelChange>
// Stop the level changing
//-----------------------------------------------------------------------------
void SwitchToggleMultilevel::StopLevelChange
(
)
{
	Log::Write( "SwitchToggleMultilevel::StopLevelChange - Stopping the level change on node %d", GetNodeId() );
	Msg* msg = new Msg( "SwitchToggleMultilevel StopLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchToggleMultilevelCmd_StopLevelChange );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchToggleMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchToggleMultilevel::CreateVars
(
	uint8 const _instance
)
{
	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			Value* value = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Level", false, 0  );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}

