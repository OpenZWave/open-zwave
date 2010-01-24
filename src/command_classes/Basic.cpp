//-----------------------------------------------------------------------------
//
//	Basic.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BASIC
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
#include "Basic.h"
#include "Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum BasicCmd
{
    BasicCmd_Set	= 0x01,
    BasicCmd_Get	= 0x02,
    BasicCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <Basic::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void Basic::RequestState
(
)
{
	Log::Write( "Requesting the basic level from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "BasicCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( BasicCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Basic::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Basic::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if( BasicCmd_Report == (BasicCmd)_pData[0] )
    {
		GetNode()->SetLevel( _pData[1] );
		Log::Write( "Received Basic report from node %d: level=%d", GetNodeId(), _pData[1] );
		return true;
	}
    if( BasicCmd_Set == (BasicCmd)_pData[0] )
    {
		GetNode()->SetLevel( _pData[1] );
		Log::Write( "Received Basic set from node %d: level=%d", GetNodeId(), _pData[1] );
		
		if( Node const* pNode = GetNode() )
		{
			if( ( Node::GenericType_SwitchRemote == pNode->GetGeneric() ) && ( Node::BasicType_RoutingSlave == pNode->GetBasic() ) )
			{
				if( Association* pAssociation = static_cast<Association*>( pNode->GetCommandClass( Association::StaticGetCommandClassId() ) ) )
				{
					Log::Write( "This is a powered remote switch, so we traverse the associations and request reports" );
					for( uint8 i=1; i<=pAssociation->GetNumGroups(); ++i )
					{
						vector<uint8> const& group = pAssociation->GetGroup( i );
						for( vector<uint8>::const_iterator it = group.begin(); it != group.end(); ++it )
						{
							Msg* pMsg = new Msg( "BasicCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
							pMsg->Append( *it );
							pMsg->Append( 2 );
							pMsg->Append( GetCommandClassId() );
							pMsg->Append( BasicCmd_Get );
							pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
							Driver::Get()->SendMsg( pMsg );
						}
					}
				}
			}
		}
		return true;
	}
    return false;
}

//-----------------------------------------------------------------------------
// <Basic::Set>
// Set the level on a device
//-----------------------------------------------------------------------------
void Basic::Set
(
	uint8 const _level
)
{
	Log::Write( "Basic::Set - Setting node %d to level %d", GetNodeId(), _level );
	Msg* pMsg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( 3 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( BasicCmd_Set );
	pMsg->Append( _level );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <Basic::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Basic::CreateVars
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
			Value* pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 0, "Level", false, 0 );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}
