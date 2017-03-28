//-----------------------------------------------------------------------------
//
//	MultiChannelAssociation.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION
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
#include "command_classes/CommandClasses.h"
#include "command_classes/MultiChannelAssociation.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "platform/Log.h"

using namespace OpenZWave;

enum MultiChannelAssociationCmd
{
	MultiChannelAssociationCmd_Set				= 0x01,
	MultiChannelAssociationCmd_Get				= 0x02,
	MultiChannelAssociationCmd_Report			= 0x03,
	MultiChannelAssociationCmd_Remove			= 0x04,
	MultiChannelAssociationCmd_GroupingsGet	= 0x05,
	MultiChannelAssociationCmd_GroupingsReport	= 0x06
};

// <MultiChannelAssociation::MultiChannelAssociation>
// Constructor
//-----------------------------------------------------------------------------
MultiChannelAssociation::MultiChannelAssociation
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_queryAll(false),
	m_numGroups(0),
	m_alwaysSetInstance(false)
{
	SetStaticRequest( StaticRequest_Values );
}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::ReadXML>
// Read the saved association data
//-----------------------------------------------------------------------------
void MultiChannelAssociation::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	TiXmlElement const* associationsElement = _ccElement->FirstChildElement();
	while( associationsElement )
	{
		char const* str = associationsElement->Value();
		if( str && !strcmp( str, "Associations" ) )
		{
			int intVal;
			if( TIXML_SUCCESS == associationsElement->QueryIntAttribute( "num_groups", &intVal ) )
			{
				m_numGroups = (uint8)intVal;
			}

			TiXmlElement const* groupElement = associationsElement->FirstChildElement();
			while( groupElement )
			{
				if( Node* node = GetNodeUnsafe() )
				{
					Group* group = new Group( GetHomeId(), GetNodeId(), groupElement );
					node->AddGroup( group );
				}

				groupElement = groupElement->NextSiblingElement();
			}

			break;
		}

		associationsElement = associationsElement->NextSiblingElement();
	}
	char const*  str = _ccElement->Attribute("ForceInstances");
	if( str )
	{
                m_alwaysSetInstance = !strcmp( str, "true");
	}

}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::WriteXML>
// Save the association data
//-----------------------------------------------------------------------------
void MultiChannelAssociation::WriteXML
(
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );

	if( Node* node = GetNodeUnsafe() )
	{
		TiXmlElement* associationsElement = new TiXmlElement( "Associations" );

		char str[8];
		snprintf( str, 8, "%d", m_numGroups );
		associationsElement->SetAttribute( "num_groups", str );

		_ccElement->LinkEndChild( associationsElement );
		node->WriteGroups( associationsElement );
	}
	if (m_alwaysSetInstance) {
		_ccElement->SetAttribute("ForceInstances", "true");
	}
}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::RequestState>
// Nothing to do for Association
//-----------------------------------------------------------------------------
bool MultiChannelAssociation::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		// Request the supported group info
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::RequestValue>
// Nothing to do for Association
//-----------------------------------------------------------------------------
bool MultiChannelAssociation::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}
	// Request the supported group info
	Msg* msg = new Msg( "MultiChannelAssociationCmd_GroupingsGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( MultiChannelAssociationCmd_GroupingsGet );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <Association::RequestAllGroups>
// Request the contents of each group in turn
//-----------------------------------------------------------------------------
void MultiChannelAssociation::RequestAllGroups
(
	uint32 const _requestFlags
)
{
	m_queryAll = true;

	// Request the contents of the individual groups in turn.
	if( m_numGroups == 0xff )
	{
		// We start with group 255, and will then move to group 1, 2 etc and stop when we find a group with a maxAssociations of zero.
		Log::Write( LogLevel_Info, GetNodeId(), "Number of association groups reported for node %d is 255, which requires special case handling.", GetNodeId() );
		QueryGroup( 0xff, _requestFlags );
	}
	else
	{
		// We start with group 1, and will then move to group 2, 3 etc and stop when the group index is greater than m_numGroups.
		Log::Write( LogLevel_Info, GetNodeId(), "Number of association groups reported for node %d is %d.", GetNodeId(), m_numGroups );
		QueryGroup( 1, _requestFlags );
	}
}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MultiChannelAssociation::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	bool handled = false;
	uint32 i;

	if( Node* node = GetNodeUnsafe() )
	{
		if( MultiChannelAssociationCmd_GroupingsReport == (MultiChannelAssociationCmd)_data[0] )
		{
			// Retrieve the number of groups this device supports.
			// The groups will be queried with the session data.
			m_numGroups = _data[1];
			Log::Write( LogLevel_Info, GetNodeId(), "Received Multi Instance Association Groupings report from node %d. Number of groups is %d", GetNodeId(), m_numGroups );
			ClearStaticRequest( StaticRequest_Values );
			handled = true;
		}
		else if( MultiChannelAssociationCmd_Report == (MultiChannelAssociationCmd)_data[0] )
		{
			// Get the group info
			uint8 groupIdx = _data[1];
			uint8 maxAssociations = _data[2];		// If the maxAssociations is zero, this is not a supported group.
			uint8 numReportsToFollow = _data[3];	// If a device supports a lot of associations, they may come in more than one message.

			if( maxAssociations )
			{
				if( _length >= 5 )
				{
					// format:
					//   node A
					//   node B
					//   0x00 Marker
					//   node C 
					//   instance #
					//   node D
					//   instance #
					Log::Write( LogLevel_Info, GetNodeId(), "Received Multi Instance Association report from node %d, group %d", GetNodeId(), groupIdx );
					Log::Write( LogLevel_Info, GetNodeId(), "  The group contains:" );
					bool pastMarker = false;
					for( i=0; i < _length-5; ++i )
					{	
						if (_data[i+4] == 0x00)
						{
							pastMarker = true;
						} 
						else 
						{
							if (!pastMarker)
							{
								Log::Write( LogLevel_Info, GetNodeId(), "    Node %d",  _data[i+4] );
								InstanceAssociation association;
								association.m_nodeId=_data[i+4];
								association.m_instance=0x00;
								m_pendingMembers.push_back( association );
							} 
							else 
							{
								Log::Write( LogLevel_Info, GetNodeId(), "    Node %d instance %d",  _data[i+4], _data[i+5] );
								InstanceAssociation association;
								association.m_nodeId=_data[i+4];
								association.m_instance=_data[i+5];								
								m_pendingMembers.push_back( association );
								i++;
							}
						}
					}
				}

				if( numReportsToFollow )
				{
					// We're expecting more reports for this group
					Log::Write( LogLevel_Info, GetNodeId(), "%d more association reports expected for node %d, group %d", numReportsToFollow, GetNodeId(), groupIdx );
					return true;
				}
				else
				{
					// No more reports to come for this group, so we can apply the pending list
					Group* group = node->GetGroup( groupIdx );
					if( NULL == group )
					{
						// Group has not been created yet
						group = new Group( GetHomeId(), GetNodeId(), groupIdx, maxAssociations );
						node->AddGroup( group );
					}
					group->SetMultiInstance( true );

					// Update the group with its new contents
					group->OnGroupChanged( m_pendingMembers );
					m_pendingMembers.clear();
				}
			}
			else
			{
				// maxAssociations is zero, so we've reached the end of the query process
				Log::Write( LogLevel_Info, GetNodeId(), "Max associations for node %d, group %d is zero.  Querying associations for this node is complete.", GetNodeId(), groupIdx );
				node->AutoAssociate();
				m_queryAll = false;
			}

			if( m_queryAll )
			{
				// Work out which is the next group we will query.
				// If we are currently on group 255, the next group will be 1.
				uint8 nextGroup = groupIdx + 1;
				if( !nextGroup )
				{
					nextGroup = 1;
				}

				if( nextGroup <= m_numGroups )
				{
					// Query the next group
					QueryGroup( nextGroup, 0 );
				}
				else
				{
					// We're all done
					Log::Write( LogLevel_Info, GetNodeId(), "Querying associations for node %d is complete.", GetNodeId() );
					node->AutoAssociate();
					m_queryAll = false;
				}
			}

			handled = true;
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::QueryGroup>
// Request details of an association group
//-----------------------------------------------------------------------------
void MultiChannelAssociation::QueryGroup
(
	uint8 _groupIdx,
	uint32 const _requestFlags
)
{
	if ( IsGetSupported() )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Get MultiChannelAssociation for group %d of node %d", _groupIdx, GetNodeId() );
		Msg* msg = new Msg( "MultiChannelAssociationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelAssociationCmd_Get );
		msg->Append( _groupIdx );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "MultiChannelAssociationCmd_Get Not Supported on this node");
	}
	return;
}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::Set>
// Add an association between devices
//-----------------------------------------------------------------------------
void MultiChannelAssociation::Set
(
	uint8 _groupIdx,
	uint8 _targetNodeId,
 	uint8 _instance
)
{

	/* for Qubino devices, we should always set a Instance if its the ControllerNode, so MultChannelEncap works.  - See Bug #857 */
	if ( ( m_alwaysSetInstance  == true )
			&& ( _instance == 0 )
			&& ( GetDriver()->GetControllerNodeId() == _targetNodeId ) )
	{
		_instance = 0x01;
	}

	Log::Write( LogLevel_Info, GetNodeId(), "MultiChannelAssociation::Set - Adding instance %d on node %d to group %d of node %d", 
	           _instance, _targetNodeId, _groupIdx, GetNodeId() );

	if ( _instance == 0x00 )
	{
		Msg* msg = new Msg( "MultiChannelAssociationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelAssociationCmd_Set );
		msg->Append( _groupIdx );
		msg->Append( _targetNodeId );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
	else 
	{
		Msg* msg = new Msg( "MultiChannelAssociationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 6 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelAssociationCmd_Set );
		msg->Append( _groupIdx );
		msg->Append( 0x00 ); // marker
		msg->Append( _targetNodeId );
		msg->Append( _instance );	
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
}

//-----------------------------------------------------------------------------
// <MultiChannelAssociation::Remove>
// Remove an association between devices
//-----------------------------------------------------------------------------
void MultiChannelAssociation::Remove
(
	uint8 _groupIdx,
	uint8 _targetNodeId,
 	uint8 _instance
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "MultiChannelAssociation::Remove - Removing instance %d on node %d from group %d of node %d",
	           _instance, _targetNodeId, _groupIdx, GetNodeId());

	if ( _instance == 0x00 )
	{
		Msg* msg = new Msg( "MultiChannelAssociationCmd_Remove", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelAssociationCmd_Remove );
		msg->Append( _groupIdx );
		msg->Append( _targetNodeId );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
	else
	{
		Msg* msg = new Msg( "MultiChannelAssociationCmd_Remove", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 6 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelAssociationCmd_Remove );
		msg->Append( _groupIdx );
		msg->Append( 0x00 ); // marker
		msg->Append( _targetNodeId );
		msg->Append( _instance );	
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
}

