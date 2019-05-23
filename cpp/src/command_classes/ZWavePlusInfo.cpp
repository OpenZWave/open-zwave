//-----------------------------------------------------------------------------
//
//	ZWavePlusInfo.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ZWAVEPLUS_INFO
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




//-----------------------------------------------------------------------------
// <ZWavePlusInfo::ZWavePlusInfo>
// Constructor
//-----------------------------------------------------------------------------
ZWavePlusInfo::ZWavePlusInfo
(
    uint32 const _homeId,
    uint8 const _nodeId
):
    CommandClass( _homeId, _nodeId )
{
    SetStaticRequest( StaticRequest_Values );
}

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
	if (_requestFlags & RequestFlag_Static)
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
	uint16 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED) )
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
		uint8 role    = _data[2];
		uint8 nodeType    = _data[3];
		uint16 installerIcon = (_data[4]<< 8) | _data[5];
		uint16 deviceType		 = (_data[6]<< 8) | _data[7];

		/* Only set the role, NodeType and DeviceType on Instance 1 Reports. The other instances
		 * Just have unique Icons for each endpoint */
		if (_instance == 1) { 
			if( Node* node = GetNodeUnsafe() )
			{
				node->SetPlusDeviceClasses(	role, nodeType, deviceType );
			}
//			ClearStaticRequest( StaticRequest_Values );
		}
		ValueByte* value;
		if( (value = static_cast<ValueByte*>( GetValue( _instance, ValueID_Index_ZWavePlusInfo::Version ) )) )
		{
			value->OnValueRefreshed( version );
			value->Release();
		}

		ValueShort* svalue;
		if( (svalue = static_cast<ValueShort*>( GetValue( _instance, ValueID_Index_ZWavePlusInfo::InstallerIcon ) )) )
		{
			svalue->OnValueRefreshed( installerIcon );
			svalue->Release();
		}

		if( (svalue = static_cast<ValueShort*>( GetValue( _instance, ValueID_Index_ZWavePlusInfo::UserIcon ) )) )
		{
			svalue->OnValueRefreshed( deviceType );
			svalue->Release();
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
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ZWavePlusInfo::Version, "ZWave+ Version", "", true, false, 0, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ZWavePlusInfo::InstallerIcon, "InstallerIcon", "", true, false, 0, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ZWavePlusInfo::UserIcon, "UserIcon", "", true, false, 0, 0 );

	}
}


