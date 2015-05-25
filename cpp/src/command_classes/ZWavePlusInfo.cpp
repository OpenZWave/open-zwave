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
	ZWavePlusInfoIndex_Version = 0x00,
	ZWavePlusInfoIndex_Role,
	ZWavePlusInfoIndex_Node,
	ZWavePlusInfoIndex_InstallerIcon,
	ZWavePlusInfoIndex_InstallerIconSpecific,
	ZWavePlusInfoIndex_UserIcon,
	ZWavePlusInfoIndex_UserIconSpecific
};

enum RoleTypeEnum
{
	RoleType_Central_Controller = 0x00,
	RoleType_Sub_Controller,
	RoleType_Portable_Controller,
	RoleType_Portable_Reporting_Controller,
	RoleType_Portable_Slave,
	RoleType_Always_On_Slave,
	RoleType_Reporting_Sleeping_Slave,
	RoleType_Listening_Sleeping_Slave
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

enum NodeTypeEnum
{
	NodeType_Node = 0x00,
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

static char const* c_iconTypeName[] =
{
	"Unknown Type",
	"Central Controller",
	"Display Simple",
	"Door Lock Keypad",
	"Fan Switch",
	"Gateway",
	"Light Dimmer Switch",
	"On/Off Power Switch",
	"Power Strip",
	"Remote Control AV",
	"Remote Control Multi Purpose",
	"Sensor Notification",
	"Sensor Multilevel",
	"Set Top Box",
	"Siren",
	"Sub Energy Meter",
	"Sub System Controller",
	"Thermostat HVAC",
	"Thermostat Setback",
	"TV",
	"Valve Open/Close",
	"Wall Controller",
	"Whole Home Meter Simple",
	"Window Covering No Position/Endpoint",
	"Window Covering Endpoint Aware",
	"Window Covering Position/Endpoint Aware"
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
		uint8 version = _data[1];
		RoleTypeEnum role    = (RoleTypeEnum)_data[2];
		NodeTypeEnum nodeType    = (NodeTypeEnum)_data[3];
		uint16 installerIcon = (_data[4]<< 8) | _data[5];
		uint16 userIcon		 = (_data[6]<< 8) | _data[7];

		// We have received a report from the Z-Wave device
		// make sure we have the strings for the values recieved.
		if( role <= RoleType_Listening_Sleeping_Slave && nodeType <= NodeType_IP_client_and_zwave_node )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received ZWavePlusInfo report: version:0x%.2x role:%s node:%s installerIcon:0x%.4x userIcon:0x%.4x", 
				       version, c_roleTypeName[role], c_nodeTypeName[nodeType], installerIcon ,userIcon );
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received ZWavePlusInfo report: version:0x%.2x role:0x%.2x node:0x%.2x installerIcon:0x%.4x userIcon:0x%.4x", 
				       version, role, nodeType, installerIcon ,userIcon );
		}

		if( Node* node = GetNodeUnsafe() )
		{
			node->SetDeviceClasses(	role, nodeType );
			node->SetIcon( _data[6] );
			if( _data[6] < 36 )
			{
				node->SetIconName( c_iconTypeName[_data[6]] );
			} 
			else
			{
				node->SetIconName( c_iconTypeName[0] );
			}
		}
		return true;
	}
	return false;
}
