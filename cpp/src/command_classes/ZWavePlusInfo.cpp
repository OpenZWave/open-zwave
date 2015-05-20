//-----------------------------------------------------------------------------
//
//	ZWavePlusInfo.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ZWAVE_PLUS_INFO
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
#include "command_classes/ZWavePlusInfo.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"
#include "value_classes/ValueShort.h"
#include "value_classes/ValueList.h"

using namespace OpenZWave;

enum ZWavePlusInfoCmdEnum
{
	ZWavePlusInfoCmd_Get    = 0x01,
	ZWavePlusInfoCmd_Report
};

enum
{
	ZWavePlusInfoIndex_Version = 0,
	ZWavePlusInfoIndex_Role,
	ZWavePlusInfoIndex_Node,
	ZWavePlusInfoIndex_InstallerIcon,
	ZWavePlusInfoIndex_UserIcon
};

enum
{
	RoleType_Central_Controller = 1,
	RoleType_Sub_Controller,
	RoleType_Portable_Controller,
	RoleType_Portable_Reporting_Controller,
	RoleType_Portable_Slave,
	RoleType_Always_On_Slave,
	RoleType_Reporting_Sleeping_Slave,
	RoleType_Listening_Sleeping_Slave,
};

static char const* c_roleTypeName[] =
{
	"Central Controller",
	"Sub Controller",
	"Portable Controller",
	"Portable Reporting Controller",
	"Portable Slave",
	"Always On Slave",
	"Reporting Sleeping Slave",
	"Listening Sleeping Slave"
};

enum
{
	NodeType_Node = 1,
	NodeType_IP_router,
	NodeType_IP_gateway,
	NodeType_IP_client_and_IP_node,
	NodeType_IP_client_and_zwave_node
};

static char const* c_nodeTypeName[] =
{
	"Z-Wave+ node",
	"Z-Wave+ IP router",
	"Z-Wave+ IP gateway",
	"Z-Wave+ IP client and IP node",
	"Z-Wave+ IP client and Zwave node"	
};


//-----------------------------------------------------------------------------
// <ZWavePlusInfo::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool ZWavePlusInfo::RequestState
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
// <ZWavePlusInfo::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool ZWavePlusInfo::RequestValue
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
		Msg* msg = new Msg( "ZWavePlusInfoCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ZWavePlusInfoCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "ZWavePlusInfoCmd_Get Not Supported on this node");
	}
	return false;
}


//-----------------------------------------------------------------------------
// <ZWavePlusInfo::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ZWavePlusInfo::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( ZWavePlusInfoCmd_Report == _data[0] )
	{
		// We have received a report from the Z-Wave device
		Log::Write( LogLevel_Info, GetNodeId(), "Received ZWavePlusInfo command from node %d", GetNodeId() );

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, ZWavePlusInfoIndex_Version ) ) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		}
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, ZWavePlusInfoIndex_Role ) ) )
		{
			value->OnValueRefreshed( _data[2] );
			value->Release();
		}
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, ZWavePlusInfoIndex_Node ) ) )
		{
			value->OnValueRefreshed(  _data[3] );
			value->Release();
		}
		if( ValueShort* value = static_cast<ValueShort*>( GetValue( _instance, ZWavePlusInfoIndex_InstallerIcon ) ) )
		{
			value->OnValueRefreshed(  (_data[4] << 8)  + _data[5]);
			value->Release();
		}
		if( ValueShort* value = static_cast<ValueShort*>( GetValue( _instance, ZWavePlusInfoIndex_UserIcon ) ) )
		{
			value->OnValueRefreshed(  (_data[6] << 8) + _data[7] );
			value->Release();
		}
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// <ZWavePlusInfo::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ZWavePlusInfo::CreateVars
(
		uint8 const _instance
)
{
	// version:01 role:AlwaysOnSlave node:Z-Wave+Node installerIcon:0600 userIcon:0600 

	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ZWavePlusInfoIndex_Version, "Z‐Wave Plus Version", "", true, false, 0, 0 );

		vector<ValueList::Item> roleItems;
		for( int i=0; i<8; ++i )
		{
			ValueList::Item item;
			item.m_label = c_roleTypeName[i];
			item.m_value = i;
			roleItems.push_back( item );
		}
		
		node->CreateValueList( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ZWavePlusInfoIndex_Role, "Role Type", "", true, false, 5, roleItems, 0, 0 );

		vector<ValueList::Item> nodeItems;
		for( int i=0; i<5; ++i )
		{
			ValueList::Item item;
			item.m_label = c_nodeTypeName[i];
			item.m_value = i;
			nodeItems.push_back( item );
		}
		
		node->CreateValueList( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ZWavePlusInfoIndex_Node, "Node Type", "", true, false, 0, nodeItems, 0, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ZWavePlusInfoIndex_InstallerIcon, "Installer Icon", "", true, false, 0, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ZWavePlusInfoIndex_UserIcon, "User Icon", "", true, false, 0, 0 );
	}
}
