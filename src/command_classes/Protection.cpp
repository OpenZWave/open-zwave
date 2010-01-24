//-----------------------------------------------------------------------------
//
//	Protection.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_PROTECTION
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

#include <vector>

#include "CommandClasses.h"
#include "Protection.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum ProtectionCmd
{
    ProtectionCmd_Set		= 0x01,
    ProtectionCmd_Get		= 0x02,
    ProtectionCmd_Report	= 0x03
};

static char* const c_protectionStateNames[] = 
{
	"Unprotected",
    "Protection by Sequence",
    "No Operation Possible"
};


//-----------------------------------------------------------------------------
// <Protection::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void Protection::RequestState
(
)
{
    Msg* pMsg = new Msg( "ProtectionCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( ProtectionCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Protection::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Protection::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if (ProtectionCmd_Report == (ProtectionCmd)_pData[0])
    {
		ProtectionEnum protectionState = (ProtectionEnum)_pData[1];	
		Log::Write( "Received a Protection report from node %d: %s", GetNodeId(), c_protectionStateNames[protectionState] );
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// <Protection::Set>
// Set the device's 
//-----------------------------------------------------------------------------
void Protection::Set
(
	ProtectionEnum _state
)
{
	Log::Write( "Protection::Set - Setting protection state on node %d to '%s'", GetNodeId(), c_protectionStateNames[_state] );
	Msg* pMsg = new Msg( "Protection Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( 3 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( ProtectionCmd_Set );
	pMsg->Append( (uint8)_state );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <Protection::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Protection::CreateVars
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
			vector<string> items;
			items.push_back( c_protectionStateNames[0] ); 
			items.push_back( c_protectionStateNames[1] ); 
			items.push_back( c_protectionStateNames[2] ); 

			Value* pValue = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, "Protection", false, items, c_protectionStateNames[0] );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}
