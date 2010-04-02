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

#include "tinyxml.h"
#include "CommandClasses.h"
#include "Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Group.h"
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
// <Association::ReadXML>
// Read the saved association data
//-----------------------------------------------------------------------------
void Association::ReadXML
( 
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	if( Node* node = GetNode() )
	{
		TiXmlElement const* associationsElement = _ccElement->FirstChildElement();
		while( associationsElement )
		{
			char const* str = associationsElement->Value();
			if( str && !strcmp( str, "Associations" ) )
			{
				TiXmlElement const* groupElement = associationsElement->FirstChildElement();
				while( groupElement )
				{
					Group* group = new Group( GetHomeId(), GetNodeId(), groupElement );
					node->AddGroup( group );

					groupElement = groupElement->NextSiblingElement();
				}

				break;
			}

			associationsElement = associationsElement->NextSiblingElement();
		}
	}
}

//-----------------------------------------------------------------------------
// <Association::WriteXML>
// Save the association data
//-----------------------------------------------------------------------------
void Association::WriteXML
( 
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );

	if( Node* node = GetNode() )
	{
		TiXmlElement* associationsElement = new TiXmlElement( "Associations" );
		_ccElement->LinkEndChild( associationsElement );
		node->WriteGroups( associationsElement ); 
	}
}

//-----------------------------------------------------------------------------
// <Association::RequestState>
// Nothing to do for Association
//-----------------------------------------------------------------------------
void Association::RequestState
(
	uint8 const _instance
)
{
	// Request all the association groups
	Msg* msg = new Msg( "Get Association Groupings", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_GroupingsGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Association::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Association::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	bool handled = false;

	Node* node = GetNode();
	if( node )
	{
		if( AssociationCmd_GroupingsReport == (AssociationCmd)_data[0] )
		{	
			// Retreive the number of groups this device supports,
			// and request the members of each group in turn.
			uint8 numGroups = _data[1];

			Log::Write( "Received Association Groupings report from node %d: Number of Groups=%d", GetNodeId(), numGroups );

			for( uint8 i=0; i<numGroups; ++i )
			{
				Msg* msg = new Msg( "Get Associations", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
				msg->Append( GetNodeId() );
				msg->Append( 3 );
				msg->Append( GetCommandClassId() );
				msg->Append( AssociationCmd_Get );
				msg->Append( i+1 );
				msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
				GetDriver()->SendMsg( msg );
			}
			handled = true;
		}
		else if( AssociationCmd_Report == (AssociationCmd)_data[0] )
		{
			// Get the group memebers
			uint8 groupIdx = _data[1];
	//		uint8 maxNodes = _data[2];

			Group* group = node->GetGroup( groupIdx );
			if( NULL == group )
			{
				// Group has not been created yet
				group = new Group( GetHomeId(), GetNodeId(), groupIdx );
				node->AddGroup( group );
			}

//			uint8 numNodes = _data[3];	- should be this value, but it always appears to be zero.
			if( _length > 5 )
			{
				uint8 numNodes = _length - 5;
				Log::Write( "Received Association report from node %d, group %d: Number of associations=%d", GetNodeId(), groupIdx, numNodes );

				group->OnGroupChanged( numNodes, &_data[4] );
			}

			handled = true;
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <Association::Set>
// Add an association between devices
//-----------------------------------------------------------------------------
void Association::Set
(
	uint8 _groupIdx,
	uint8 _targetNodeId
)
{
	Log::Write( "Association::Set - Adding node %d to group %d of node %d", _targetNodeId, _groupIdx, GetNodeId() );

	Msg* msg = new Msg( "Association Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_Set );
	msg->Append( _groupIdx );
	msg->Append( _targetNodeId );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Association::Remove>
// Remove an association between devices
//-----------------------------------------------------------------------------
void Association::Remove
(
	uint8 _groupIdx,
	uint8 _targetNodeId
)
{
	Log::Write( "Association::Remove - Removing node %d from group %d of node %d", _targetNodeId, _groupIdx, GetNodeId() );

	Msg* msg = new Msg( "Association Remove", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_Remove );
	msg->Append( _groupIdx );
	msg->Append( _targetNodeId );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

