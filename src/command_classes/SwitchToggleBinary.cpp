//-----------------------------------------------------------------------------
//
//	SwitchToggleBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_TOGGLE_BINARY
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
#include "SwitchToggleBinary.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueBool.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum SwitchToggleBinaryCmd
{
    SwitchToggleBinaryCmd_Set		= 0x01,
    SwitchToggleBinaryCmd_Get		= 0x02,
    SwitchToggleBinaryCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <SwitchToggleBinary::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void SwitchToggleBinary::RequestState
(
)
{
    Msg* pMsg = new Msg( "SwitchToggleBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( SwitchToggleBinaryCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <SwitchToggleBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchToggleBinary::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if (SwitchToggleBinaryCmd_Report == (SwitchToggleBinaryCmd)_pData[0])
    {
		Log::Write( "Received SwitchToggleBinary report from node %d: level=%d", GetNodeId(), _pData[1] );

		GetNode()->SetLevel( _pData[1] );
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// <SwitchToggleBinary::Set>
// Toggle the state of the switch
//-----------------------------------------------------------------------------
void SwitchToggleBinary::Set
(
)
{
	Log::Write( "SwitchToggleBinary::Set - Toggling the state of node %d", GetNodeId() );
	Msg* pMsg = new Msg( "SwitchToggleBinary Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( 2 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( SwitchToggleBinaryCmd_Set );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchToggleBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchToggleBinary::CreateVars
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
			Value* pValue = new ValueBool( GetNodeId(), GetCommandClassId(), _instance, 0, "Toggle Switch", false, false );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}


