//-----------------------------------------------------------------------------
//
//	SceneControllerConf.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SCENE_CONTROLLER_CONF
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
#include "command_classes/SceneControllerConf.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include "value_classes/ValueByte.h"

using namespace OpenZWave;

enum SceneControllerConfCmd
{
	SceneControllerConfCmd_Set 		= 0x01,
	SceneControllerConfCmd_Get 		= 0x02,
	SceneControllerConfCmd_Report 	= 0x03
};


//-----------------------------------------------------------------------------
// <SceneControllerConf::RequestState>
// Request button bindings from the device
//-----------------------------------------------------------------------------
bool SceneControllerConf::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool res = false;

	if( ( _requestFlags & RequestFlag_Session ) )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			for( int i = 1; i <= node->GetNumGroups() ; i++ )
			{
				res |= RequestValue( _requestFlags, i, _instance, _queue );
			}
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
// <SceneControllerConf::RequestValue>
// request the state from the device
//-----------------------------------------------------------------------------
bool SceneControllerConf::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _group,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}

	Msg* msg = new Msg( "SceneControllerConfCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( SceneControllerConfCmd_Get );
	msg->Append( _group );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <SceneControllerConf::Set>
// Bind the scene to a button
//-----------------------------------------------------------------------------
void SceneControllerConf::Set
(
	uint8 const _group,
	uint8 const _sceneId,
	uint8 const _duration
)
{
	// TODO _bReplyRequired is false else we get a timeout. Why is this needed?
	Msg* msg = new Msg( "SceneControllerConfCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, false, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 5 );
	msg->Append( GetCommandClassId() );
	msg->Append( SceneControllerConfCmd_Set );
	msg->Append( _group );
	msg->Append( _sceneId );
	msg->Append( _duration );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
}

//-----------------------------------------------------------------------------
// <SceneControllerConf::SetValue>
// Bind the scene to a button
//-----------------------------------------------------------------------------
bool SceneControllerConf::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Byte == _value.GetID().GetType() )
	{
		ValueByte const* value = static_cast<ValueByte const*>(&_value);
		Set( _value.GetID().GetIndex(),  value->GetValue(), 0x00 );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SceneControllerConf::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SceneControllerConf::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( SceneControllerConfCmd_Report == _data[0] )
	{
		// We have received a hail from the Z-Wave device.
		uint8 group    = _data[1];
		uint8 sceneId  = _data[2];
		uint8 duration = _data[3];

		char msg[64];

		if( duration == 0 )
			snprintf( msg, sizeof(msg), "now" );
		else if( duration <= 0x7F )
			snprintf( msg, sizeof(msg), "%d seconds", duration );
		else if( duration <= 0xFE )
			snprintf( msg, sizeof(msg), "%d minutes", duration );
		else
			snprintf( msg, sizeof(msg), "via configuration" );

		if( Node* node = GetNodeUnsafe() )
		{
			if( sceneId == 0 )
			{
				Log::Write( LogLevel_Info, GetNodeId(), "Received Scene Controller Configuration Report, group/button %d on node %d has no active scene.", group,  GetNodeId() );
			}
			else
			{
				Log::Write( LogLevel_Info, GetNodeId(), "Received Scene Controller Configuration Report, group/button %d on node %d triggers scene id=%d with duration %s.", group, GetNodeId(), sceneId, msg );
			}
			ValueByte* value = static_cast<ValueByte*>( GetValue ( _instance, group ) );
			if( value == NULL )
			{
				char lbl[64];
				snprintf(lbl, 64, "Button %d", group);
				node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, group, lbl, "", false, false, 0, 0 );
				value = static_cast<ValueByte*>( GetValue ( _instance, group ) );
			}
			value->OnValueRefreshed( sceneId );
			value->Release();
			// what about duration?
		}
		return true;
	}
	return false;
}
