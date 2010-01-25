//-----------------------------------------------------------------------------
//
//	SwitchBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_BINARY
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
#include "SwitchBinary.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueBool.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum SwitchBinaryCmd
{
    SwitchBinaryCmd_Set		= 0x01,
    SwitchBinaryCmd_Get		= 0x02,
    SwitchBinaryCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <SwitchBinary::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void SwitchBinary::RequestState
(
)
{
    Msg* pMsg = new Msg( "SwitchBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( SwitchBinaryCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <SwitchBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchBinary::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if (SwitchBinaryCmd_Report == (SwitchBinaryCmd)_pData[0])
    {
		Log::Write( "Received SwitchBinary report from node %d: level=%d", GetNodeId(), _pData[1] );
		GetNode()->SetLevel( _pData[1] );
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// <SwitchBinary::Set>
// Set the state of the switch
//-----------------------------------------------------------------------------
void SwitchBinary::Set
(
	bool const _bState
)
{
	Log::Write( "SwitchBinary::Set - Setting node %d to %s", GetNodeId(), _bState ? "on" : "off" );
	Msg* pMsg = new Msg( "SwitchBinary Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( 3 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( SwitchBinaryCmd_Set );
	pMsg->Append( _bState ? 0xff : 0x00 );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <SwitchBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchBinary::CreateVars
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
			Value* pValue = new ValueBool( GetNodeId(), GetCommandClassId(), _instance, 0, "Switch", false, false );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}
