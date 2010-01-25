//-----------------------------------------------------------------------------
//
//	Lock.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_LOCK
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
#include "Lock.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueBool.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum LockCmd
{
    LockCmd_Set		= 0x01,
    LockCmd_Get		= 0x02,
    LockCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <Lock::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void Lock::RequestState
(
)
{
	Log::Write( "Requesting the lock state from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "LockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( LockCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Lock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Lock::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if( LockCmd_Report == (LockCmd)_pData[0] )
    {
        // We have received a report from the Z-Wave device
        m_state = (LockStateEnum)_pData[1];
		Log::Write( "Received Lock report from node %d: Lock is %s", GetNodeId(), (m_state == LockState_Unlocked) ? "Unlocked" : "Locked" );
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// <Lock::Set>
// Set the device's 
//-----------------------------------------------------------------------------
void Lock::Set
(
	LockStateEnum _state
)
{
	Log::Write( "Lock::Set - Requesting the node %d lock to be %s", GetNodeId(), (_state==LockState_Unlocked) ? "Unlocked" : "Locked" );
    Msg* pMsg = new Msg( "LockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 3 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( LockCmd_Set );
    pMsg->Append( _state );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Lock::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Lock::CreateVars
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
			Value* pValue = new ValueBool( GetNodeId(), GetCommandClassId(), _instance, 0, "Locked", false, false );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}


