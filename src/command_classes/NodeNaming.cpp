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
#include "NodeNaming.h"
#include "Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueStore.h"

using namespace OpenZWave;

enum NodeNamingCmd
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
	uint8 const _instance
)
{
	Log::Write( "Requesting the name from node %d", GetNodeId() );
	Msg* msg = new Msg( "NodeNamingCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( NodeNamingCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <NodeNaming::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool NodeNaming::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( NodeNamingCmd_Report == (NodeNamingCmd)_data[0] )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <NodeNaming::Set>
// Set the name of the device
//-----------------------------------------------------------------------------
void NodeNaming::Set
(
	string const& _name
)
{
	Log::Write( "NodeNaming::Set - Naming node %d as ''", GetNodeId(), _name.c_str() );
	Msg* msg = new Msg( "NodeNaming Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( NodeNamingCmd_Set );
	msg->Append( 0 );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

