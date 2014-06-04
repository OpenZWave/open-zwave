//-----------------------------------------------------------------------------
//
//	DoorLock.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_DOOR_LOCK
//
//	Copyright (c) 2014 Justin Hammond <justin@dynam.ac>
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

#include "CommandClasses.h"
#include "DoorLock.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueBool.h"

using namespace OpenZWave;

enum DoorLockCmd
{
	DoorLockCmd_Set					= 0x01,
	DoorLockCmd_Get					= 0x02,
	DoorLockCmd_Report				= 0x03,
	DoorLockCmd_Configuration_Set	= 0x04,
	DoorLockCmd_Configuration_Get	= 0x05,
	DoorLockCmd_Configuration_Report= 0x06
};

//-----------------------------------------------------------------------------
// <DoorLock::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool DoorLock::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <DoorLock::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool DoorLock::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "DoorLockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "DoorLockCmd_Get Not Supported on this node");
	}
	return false;
}


//-----------------------------------------------------------------------------
// <DoorLock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool DoorLock::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( DoorLockCmd_Report == (DoorLockCmd)_data[0] )
	{
		std::string str;
		switch (_data[1]) {
			case 0x00: 	str = "Unsecured"; break;
			case 0x01:  str = "Unsecured With Timeout"; break;
			case 0x10:  str = "Unsecured for Inside Door Handles"; break;
			case 0x11:  str = "Unsecured for Inside Door Handles with Timeout"; break;
			case 0x20:	str = "Unsecured for Outside Door Handles"; break;
			case 0x21:  str = "Unsecured for Outside Door Handles with Timeout"; break;
			case 0xFF:  str = "Secured"; break;
		}
		Log::Write( LogLevel_Info, GetNodeId(), "Received DoorLock report: DoorLock is %s", str.c_str() );

		if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
		{
			value->OnValueRefreshed( _data[1] == 0xFF );
			value->Release();
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <DoorLock::SetValue>
// Set the lock's state
//-----------------------------------------------------------------------------
bool DoorLock::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Bool == _value.GetID().GetType() )
	{
		ValueBool const* value = static_cast<ValueBool const*>(&_value);

		Log::Write( LogLevel_Info, GetNodeId(), "DoorLock::Set - Requesting lock to be %s", value->GetValue() ? "Locked" : "Unlocked" );
		Msg* msg = new Msg( "DoorLockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockCmd_Set );
		msg->Append( value->GetValue() ? 0xFF:0x00 );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <DoorLock::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void DoorLock::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Locked", "", false, false, false, 0 );
	}
}


