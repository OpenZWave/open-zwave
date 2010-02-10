//-----------------------------------------------------------------------------
//
//	Association.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ASSOCIATION
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
#include "Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Log.h"

using namespace OpenZWave;

enum AssociationCmd
{
	AssociationCmd_Set				= 0x01,
	AssociationCmd_Get				= 0x02,
	AssociationCmd_Report			= 0x03,
	AssociationCmd_Remove			= 0x04,
	AssociationCmd_GroupingsGet		= 0x05,
	AssociationCmd_GroupingsReport	= 0x06
};


//-----------------------------------------------------------------------------
// <Association::RequestState>
// Nothing to do for Association
//-----------------------------------------------------------------------------
void Association::RequestState
(
)
{
	// Request all the association groups
	Msg* msg = new Msg( "Get Association Groupings", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_GroupingsGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg ); 
}

//-----------------------------------------------------------------------------
// <Association::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Association::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if( AssociationCmd_GroupingsReport == (AssociationCmd)_data[0] )
	{	
		// Clear the existing groups
		delete [] m_groups;

		// Retreive the number of groups this device supports,
		// and request the members of each group in turn.
		m_numGroups = _data[1];
		m_groups = new vector<uint8>[m_numGroups];

		Log::Write( "Received Association Groupings report from node %d: Number of Groups=%d", GetNodeId(), m_numGroups );

		for( uint8 i=0; i<m_numGroups; ++i )
		{
			Msg* msg = new Msg( "Get Associations", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( AssociationCmd_Get );
			msg->Append( i+1 );
			msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			Driver::Get()->SendMsg( msg );
		}
		return true;
	}

	if( AssociationCmd_Report == (AssociationCmd)_data[0] )
	{
		// Get the group memebers
		uint8 groupIdx = _data[1];
//		uint8 maxNodes = _data[2];
		
		vector<uint8>* group = &m_groups[groupIdx-1];
		group->clear();

		uint8 numNodes = _data[3];

		Log::Write( "Received Association report from node %d, group %d: Number of associations=%d", GetNodeId(), groupIdx, numNodes );

		for( uint8 i=0; i<numNodes; ++i )
		{
			Log::Write( "  Node %d", _data[i+4] );
			group->push_back( _data[i+4] );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Association::Set>
// Set an association between devices
//-----------------------------------------------------------------------------
void Association::Set
(
	uint8 _group,
	uint8 _targetNodeId
)
{
	Log::Write( "Association::Set - Node=%d, Group=%d, Target=%d", GetNodeId(), _group, _targetNodeId );

	Msg* msg = new Msg( "Association Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_Set );
	msg->Append( _group );
	msg->Append( _targetNodeId );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Association::Remove>
// Remove an association between devices
//-----------------------------------------------------------------------------
void Association::Remove
(
	uint8 _group,
	uint8 _targetNodeId
)
{
	Log::Write( "Association::Remove - Node=%d, Group=%d, Target=%d", GetNodeId(), _group, _targetNodeId );

	Msg* msg = new Msg( "Association Remove", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_Remove );
	msg->Append( _group );
	msg->Append( _targetNodeId );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

