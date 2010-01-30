//-----------------------------------------------------------------------------
//
//	SwitchMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_MULTILEVEL
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
#include "SwitchMultilevel.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum SwitchMultilevelCmd
{
    SwitchMultilevelCmd_Set					= 0x01,
    SwitchMultilevelCmd_Get					= 0x02,
    SwitchMultilevelCmd_Report				= 0x03,
    SwitchMultilevelCmd_StartLevelChange	= 0x04,
    SwitchMultilevelCmd_StopLevelChange		= 0x05,
    SwitchMultilevelCmd_DoLevelChange		= 0x06
};


//-----------------------------------------------------------------------------
// <SwitchMultilevel::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void SwitchMultilevel::RequestState
(
)
{
    Msg* pMsg = new Msg( "SwitchMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( SwitchMultilevelCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchMultilevel::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if (SwitchMultilevelCmd_Report == (SwitchMultilevelCmd)_pData[0])
	{
		Node* pNode = GetNode();
		if( pNode )
		{
			ValueStore* pStore = pNode->GetValueStore();
			if( pStore )
			{
				if( ValueByte* pValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					pValue->OnValueChanged( _pData[1] );
				}
				pNode->ReleaseValueStore();

				Log::Write( "Received SwitchMultiLevel report from node %d: level=%d", GetNodeId(), _pData[1] );
				return true;
			}
		}
	}

    return false;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetValue>
// Set the level on a device
//-----------------------------------------------------------------------------
bool SwitchMultilevel::SetValue
(
	Value const& _value
)
{
	if( ValueByte const* value = static_cast<ValueByte const*>(&_value) )
	{
		Log::Write( "SwitchMultilevel::Set - Setting node %d to level %d", GetNodeId(), value->GetPending() );
		Msg* pMsg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
		pMsg->Append( GetNodeId() );
		pMsg->Append( 3 );
		pMsg->Append( GetCommandClassId() );
		pMsg->Append( SwitchMultilevelCmd_Set );
		pMsg->Append( value->GetPending() );
		pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SwitchMultilevelCmd_StartLevelChange>
// Start the level changing
//-----------------------------------------------------------------------------
void SwitchMultilevel::StartLevelChange
(
	SwitchMultilevelDirection const _direction,
	bool const _bIgnoreStartLevel,
	bool const _bRollover
)
{
	uint8 param = (uint8)_direction;
	param |= ( _bIgnoreStartLevel ? 0x20 : 0x00 );
	param |= ( _bRollover ? 0x80 : 0x00 );

	Log::Write( "SwitchMultilevel::StartLevelChange - Starting a level change on node %d, Direction=%d, IgnoreStartLevel=%s and rollover=%s", GetNodeId(), (_direction==SwitchMultilevelDirection_Up) ? "Up" : "Down", _bIgnoreStartLevel ? "True" : "False", _bRollover ? "True" : "False" );
	Msg* pMsg = new Msg( "SwitchMultilevel StartLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( 3 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( SwitchMultilevelCmd_StartLevelChange );
	pMsg->Append( param );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::StopLevelChange>
// Stop the level changing
//-----------------------------------------------------------------------------
void SwitchMultilevel::StopLevelChange
(
)
{
	Log::Write( "SwitchMultilevel::StopLevelChange - Stopping the level change on node %d", GetNodeId() );
	Msg* pMsg = new Msg( "SwitchMultilevel StopLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( 2 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( SwitchMultilevelCmd_StopLevelChange );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::EnableLevelChange>
// Enable of disable the level change commands
//-----------------------------------------------------------------------------
void SwitchMultilevel::EnableLevelChange
(
	bool const _bState
)
{
	Log::Write( "SwitchMultilevel::DoLevelChange - %s level changing on node %d", _bState ? "Enabling" : "Disabling", GetNodeId() );
	Msg* pMsg = new Msg( "SwitchMultilevel DoLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( 3 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( SwitchMultilevelCmd_DoLevelChange );
	pMsg->Append( _bState ? 0xff : 0x00 );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchMultilevel::CreateVars
(
	uint8 const _instance
)
{
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			Value* pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Level", false, 0  );
			pStore->AddValue( pValue );
			pValue->Release();

			pNode->ReleaseValueStore();
		}
	}
}



