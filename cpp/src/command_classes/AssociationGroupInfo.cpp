//-----------------------------------------------------------------------------
//
//	AssociationGroupInfo.cpp
//
//	Implementation of the Z-Wave COMMAND_ASSOCIATION_GRP_INFO
//
//	Copyright (c) 2015
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

#include "command_classes/CommandClasses.h"
#include "command_classes/Association.h"
#include "command_classes/AssociationGroupInfo.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Group.h"
#include "Driver.h"
#include "platform/Log.h"

using namespace OpenZWave;

enum AssociationGroupInfoCmd
{
	AssociationGroupInfoCmd_Name_Get            = 0x01, 
	AssociationGroupInfoCmd_Name_Report         = 0x02, 
	AssociationGroupInfoCmd_Info_Get            = 0x03, 
	AssociationGroupInfoCmd_Info_Report         = 0x04,
	AssociationGroupInfoCmd_Command_List_Get    = 0x05,
	AssociationGroupInfoCmd_Command_List_Report = 0x06
};

// other values to be discoverd
enum
{
	AssociationGroupInfoReportProfile_General = 0x00,
};

enum
{
	AssociationGroupInfo_Get_ListModeMask			= 0x40,
	AssociationGroupInfo_Get_RefreshCacheMask		= 0x80,

	AssociationGroupInfo_Report_GroupCountMask		= 0x3f,
	AssociationGroupInfo_Report_DynamicInfoMask		= 0x40,
	AssociationGroupInfo_Report_ListModeMask		= 0x80,

};

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::AssociationGroupInfo>
// Constructor
//-----------------------------------------------------------------------------
AssociationGroupInfo::AssociationGroupInfo
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId )
{
}

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool AssociationGroupInfo::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( /*_requestFlags & RequestFlag_Static ||*/ _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool AssociationGroupInfo::RequestValue
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

	if ( IsGetSupported() )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			Log::Write(  LogLevel_Info, GetNodeId(), "AssociationGroupInfo RequestValue for %d groups", node->GetNumGroups() );

			// lets request all information for all groups we know
			// The group names are requested upon group discovery 
			for( uint8 groupIdx = 1; groupIdx <= node->GetNumGroups( ); groupIdx++ )
			{
				// GetGroupName( groupIdx );
				GetGroupInfo( groupIdx );
				GetGroupCmdInfo( groupIdx );
			}
		}
		return true;
	}
	else
	{
		Log::Write(  LogLevel_Info, GetNodeId(), "AssociationGroupInfoCmd_Name_Report Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::GetGroupNames>
// Request all group names
//-----------------------------------------------------------------------------
void AssociationGroupInfo::GetGroupNames
(
)
{
	if( IsGetSupported() )
	{
		if( Node* node = GetNodeUnsafe() )
		{   // we need the number of groups to retrieve, while associations have not been retrieved yet 
			// we can not use group count from node. 
			if( Association* cc = static_cast<Association*>( node->GetCommandClass( Association::StaticGetCommandClassId() ) ) )
			{
				for(int i=1 ; i<=cc->GetNumGroups() ; i++ )
				{
					GetGroupName( i );
				}
			}
		}
	}	
}

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::GetGroupName>
// Request the name of the group.
//-----------------------------------------------------------------------------
void AssociationGroupInfo::GetGroupName
(
	uint8 _groupIdx
)
{
	if( IsGetSupported() )
	{
		Log::Write( LogLevel_Detail, GetNodeId(), "Get name for group %d of node %d", _groupIdx, GetNodeId() );
		Msg* msg = new Msg( "AssociationGroupInfoCmd_Name_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( AssociationGroupInfoCmd_Name_Get );
		msg->Append( _groupIdx );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Query);
	}
	else
	{
		Log::Write(  LogLevel_Info, GetNodeId(), "AssociationGroupInfoCmd_Name_Get Not Supported on this node");
	}
}

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::GetGroupInfo>
// Request the info of the group.
//-----------------------------------------------------------------------------
void AssociationGroupInfo::GetGroupInfo
(
	uint8 _groupIdx
)
{
	if( IsGetSupported() )
	{
		Log::Write( LogLevel_Detail, GetNodeId(), "Get info for group %d of node %d", _groupIdx, GetNodeId() );
		Msg* msg = new Msg( "AssociationGroupInfoCmd_Info_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( AssociationGroupInfoCmd_Info_Get );
		if( _groupIdx == 0x00 )
		{	// request all groups
			msg->Append( AssociationGroupInfo_Get_ListModeMask );
		}
		else
		{
			msg->Append( 0x00 );
		}
		msg->Append( _groupIdx );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Query);
	}
	else
	{
		Log::Write( LogLevel_Info, GetNodeId(), "AssociationGroupInfoCmd_Info_Get Not Supported on this node");
	}
}


//-----------------------------------------------------------------------------
// <AssociationGroupInfo::GetGroupCmdInfo>
// Request the info of the group.
//-----------------------------------------------------------------------------
void AssociationGroupInfo::GetGroupCmdInfo
(
	uint8 _groupIdx
)
{
	if ( IsGetSupported() )
	{
		Log::Write( LogLevel_Detail, GetNodeId(), "Get cmd info for group %d of node %d", _groupIdx, GetNodeId() );
		Msg* msg = new Msg( "AssociationGroupInfoCmd_Command_List_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( AssociationGroupInfoCmd_Command_List_Get );
		msg->Append( 0x00 ); // bool allowCache
		msg->Append( _groupIdx );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Query);
	}
	else
	{
		Log::Write( LogLevel_Info, GetNodeId(), "AssociationGroupInfoCmd_Command_List_Get Not Supported on this node");
	}
}

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool AssociationGroupInfo::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	// We have received a message from the Z-Wave device.
	if( AssociationGroupInfoCmd_Name_Report == _data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received AssociationGroupInfoCmd_Name_Report command report from node %d", GetNodeId() );
		uint8 groupIdx = _data[1];
		string name = string( (char *)&_data[3], _data[2] );
		Log::Write( LogLevel_Info, GetNodeId(), "  Name for group %d: %s", groupIdx, name.c_str() );

		if( Node* node = GetNodeUnsafe() )
		{
			Group* group = node->GetGroup( groupIdx );
			if( group == NULL )
			{
				group = new Group( GetHomeId(), GetNodeId(), groupIdx, 0x00, &name );
				node->AddGroup( group );
			}
			else
			{
				// update the name of the group
				group->SetLabel( name );
			}
		}
		return true;
	} 
	else if( AssociationGroupInfoCmd_Info_Report == _data[0] )
	{
		bool listMode = ((_data[1] & AssociationGroupInfo_Report_ListModeMask) != 0);
		bool dynamicInfo = ((_data[1] & AssociationGroupInfo_Report_DynamicInfoMask) != 0);

		// number of group info elements in this message. Not the total number of groups on the device.
		// The device can send multiple reports in list mode
		uint8 groupCount = _data[1] & AssociationGroupInfo_Report_GroupCountMask;
		Log::Write( LogLevel_Info, GetNodeId(), "AssociationGroupInfoCmd_Info_Report: count:%d listMode:%s dynamicInfo:%s", groupCount, listMode ? "true": "false", dynamicInfo ? "true": "false");
		for( int i=0; i<groupCount; i++)
		{
			uint8 groupIdx = _data[2+i*7];
			uint8 mode = _data[3+i*7];
			uint8 profile_msb = _data[4+i*7];
			uint8 profile_lsb = _data[5+i*7];
			uint16 profile = (profile_msb << 8 | profile_lsb);
			Log::Write( LogLevel_Info, GetNodeId(), "   Group=%d, Profile=%.4x, mode:%.2x", groupIdx, profile, mode);

			if( Node* node = GetNodeUnsafe() )
			{
				if( Group* group = node->GetGroup( groupIdx ) )
				{
					group->SetProfile( profile );
				}
			}
		}
		return true;
	}
	else if( AssociationGroupInfoCmd_Command_List_Report == _data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received AssociationGroupInfoCmd_Command_List_Report command report from node %d", GetNodeId() );
		// format: 
		//	  groupId
		//	  size
		//	  command class id
		//	  command id
		if( Node* node = GetNodeUnsafe() )
		{
			// list the CommandClasses and commands that will be send to the associated nodes in this group.
			// for now just log it, later this could be used auto associate to the group
			// ie always associate when we find a battery command class
			Log::Write( LogLevel_Info, GetNodeId()," Supported Command classes and commands for group:%d  :", _data[1]);  
			for( uint8 i=0; i<_data[2]; i+=2)
			{
				// check if this node actually supports this Command Class
				if( CommandClass* cc = node->GetCommandClass(_data[i+3]) )
				{
					Log::Write( LogLevel_Info, GetNodeId(), "    %s 0x%.2x", cc->GetCommandClassName().c_str(), _data[i+4] );
				}
				else
				{
					Log::Write( LogLevel_Info, GetNodeId(), "    unsupported 0x%.2x 0x%.2x", _data[i+3], _data[i+4] );
				}
			}
		}

		return true;
	}
	return false;
}
