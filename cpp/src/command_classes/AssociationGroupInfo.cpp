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
#include "Options.h"
#include "Utils.h"
#include <map>

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
	string associateCC;
	Options::Get()->GetOptionAsString("AssociateCC", &associateCC );
	vector<string> elems;
	OpenZWave::split(elems, associateCC, ",", true);
	unsigned int cc;
	for (std::vector<std::string>::iterator it = elems.begin(); it != elems.end(); it++)
	{
		if (sscanf(OpenZWave::trim(*it).c_str(), "%x", &cc))
		{
			m_autoAssociateCCs[(cc & 0xFF)] = (cc & 0xFF);
		}
	}
	Log::Write(  LogLevel_Info, GetNodeId(), "AssociationGroupInfo bla %d", m_autoAssociateCCs.size() );

}

//-----------------------------------------------------------------------------
// <AssociationGroupInfo::IsAutoAssociateCC>
// Does the CommandClass require auto association
//-----------------------------------------------------------------------------
bool const AssociationGroupInfo::IsAutoAssociateCC
(
	uint8 const _cc
)
{
	map<uint8,uint8>::const_iterator it = m_autoAssociateCCs.find( _cc );
	if( it != m_autoAssociateCCs.end() )
	{
		return true;
	}

	return false;
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
	if( _requestFlags & RequestFlag_Static )
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
			// lets request all information for all groups we know
			// we need the number of groups to retrieve, while associations have not been retrieved yet
			// we can not use group count from node.
			if( Association* cc = static_cast<Association*>( node->GetCommandClass( Association::StaticGetCommandClassId() ) ) )
			{
				Log::Write(  LogLevel_Info, GetNodeId(), "AssociationGroupInfo RequestValue for %d groups", cc->GetNumGroups() );
				for(int groupIdx=1 ; groupIdx<=cc->GetNumGroups() ; groupIdx++ )
				{
					GetGroupName( groupIdx );
					GetGroupInfo( groupIdx );
					GetGroupCmdInfo( groupIdx );
				}
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
		Log::Write( LogLevel_Info, GetNodeId(), "Received AssociationGroupInfoCmd_Name_Report from node %d", GetNodeId() );
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
				else
				{
					Log::Write( LogLevel_Warning, GetNodeId(), "   Group:%d does not exist (yet).", groupIdx );
				}
			}
		}
		return true;
	}
	else if( AssociationGroupInfoCmd_Command_List_Report == _data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received AssociationGroupInfoCmd_Command_List_Report from node %d", GetNodeId() );
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
			bool autoAssociate = false;
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
				autoAssociate |= IsAutoAssociateCC( _data[i+3] );
			}
			if( Group* group = node->GetGroup(  _data[1] ) )
			{
				if ( autoAssociate )
				{
					group->SetAuto( true );
					Log::Write( LogLevel_Info, GetNodeId(), " Marking group for auto subscription");
				}
			}
		}
		return true;
	}
	return false;
}
