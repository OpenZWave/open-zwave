//-----------------------------------------------------------------------------
//
//	NodeNaming.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_NODE_NAMING
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
#include "NodeNaming.h"
#include "Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueStore.h"
#include "ValueString.h"

using namespace OpenZWave;

static enum NodeNamingCmd
{
    NodeNamingCmd_Set		= 0x01,
    NodeNamingCmd_Get		= 0x02,
    NodeNamingCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <NodeNaming::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void NodeNaming::RequestState
(
)
{
	Log::Write( "Requesting the name from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "NodeNamingCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( NodeNamingCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <NodeNaming::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool NodeNaming::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if( NodeNamingCmd_Report == (NodeNamingCmd)_pData[0] )
    {
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <NodeNaming::Set>
// Set the level on a device
//-----------------------------------------------------------------------------
void NodeNaming::Set
(
	uint8 const _level
)
{
	//Log::Write( "NodeNaming::Set - Setting node %d to level %d", GetNodeId(), _level );
	//Msg* pMsg = new Msg( "NodeNaming Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	//pMsg->Append( GetNodeId() );
	//pMsg->Append( 3 );
	//pMsg->Append( GetCommandClassId() );
	//pMsg->Append( NodeNamingCmd_Set );
	//pMsg->Append( _level );
	//pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <NodeNaming::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void NodeNaming::CreateVars
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
			Value* pValue = new ValueString( GetNodeId(), GetCommandClassId(), _instance, 0, "Node Name", false, "Unknown"  );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}
