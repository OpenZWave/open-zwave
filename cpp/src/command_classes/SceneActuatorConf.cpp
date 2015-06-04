//-----------------------------------------------------------------------------
//
//	SceneActuatorConf.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SCENE_ACTUATOR_CONF
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
#include "command_classes/SceneActuatorConf.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

using namespace OpenZWave;

enum SceneActuatorConfCmd
{
	SceneActuatorConfCmd_Set 	= 0x01,
	SceneActuatorConfCmd_Get 	= 0x02,
	SceneActuatorConfCmd_Report = 0x03
};


//-----------------------------------------------------------------------------
// <SceneActuatorConf::RequestState>
// Nothing to do for SceneActuatorConf
//-----------------------------------------------------------------------------
bool SceneActuatorConf::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Dynamic ) )
	{
//		RequestValue( _requestFlags, 0x01, _instance, _queue );
//		SetScene( 0x00, 0xff, 0xff );
//		RequestValue( _requestFlags, 0x01, _instance, _queue );
//		RequestValue( _requestFlags, 0x02, _instance, _queue );
//		RequestValue( _requestFlags, 0x03, _instance, _queue );
//		RequestValue( _requestFlags, 0x04, _instance, _queue );
		return RequestValue( _requestFlags, 0, _instance, _queue ); // request active scene
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SceneActuatorConf::RequestValue>
// Nothing to do for Association
//-----------------------------------------------------------------------------
bool SceneActuatorConf::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _sceneId,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}

	Msg* msg = new Msg( "SceneActuatorConfCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( SceneActuatorConfCmd_Get );
	msg->Append( _sceneId );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}


void SceneActuatorConf::SetScene
(
	uint8 const _sceneId,
	uint8 const _level,
	uint8 const _duration
)
{
	// once a actuator has activated the scene it can only be undone by a reset
	// TODO _bReplyRequired is false else we get a timeout. Why is this needed?
	Msg* msg = new Msg( "SceneActuatorConfCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, false, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 6 );
	msg->Append( GetCommandClassId() );
	msg->Append( SceneActuatorConfCmd_Set );
	msg->Append( _sceneId );
	msg->Append( _duration );
	msg->Append( 0x80 ); // use our value not the current nodes level
	msg->Append( _level );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
}

//-----------------------------------------------------------------------------
// <SceneActuatorConf::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SceneActuatorConf::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( SceneActuatorConfCmd_Report == _data[0] )
	{
		// We have received a hail from the Z-Wave device.
		uint8 sceneId  = _data[1];
		uint8 level    = _data[2];
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

		if( sceneId == 0 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Scene Actuator Configuration Report, no active scenes on node %d.", GetNodeId() );
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Scene Actuator Configuration Report from node %d: scene id=%d level=%d %s.", GetNodeId(), sceneId, level, msg );
		}
		return true;
	}
	return false;
}

