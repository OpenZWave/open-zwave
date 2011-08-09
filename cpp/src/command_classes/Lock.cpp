//-----------------------------------------------------------------------------
//
//	Lock.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_LOCK
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

#include "CommandClasses.h"
#include "Lock.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueBool.h"

using namespace OpenZWave;

enum LockCmd
{
	LockCmd_Set		= 0x01,
	LockCmd_Get		= 0x02,
	LockCmd_Report	= 0x03
};

//-----------------------------------------------------------------------------
// <Lock::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
bool Lock::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		RequestValue( _requestFlags );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Lock::RequestValue>												   
// Request current value from the device									   
//-----------------------------------------------------------------------------
void Lock::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _dummy2		// = 0 (not used)
)
{
	Msg* msg = new Msg( "LockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( LockCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Lock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Lock::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( LockCmd_Report == (LockCmd)_data[0] )
	{
		Log::Write( "Received Lock report from node %d: Lock is %s", GetNodeId(), _data[1] ? "Locked" : "Unlocked" );

		if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
		{
			value->OnValueChanged( _data[1] != 0 );
		}
		Node* node = GetNodeUnsafe();
		if( node != NULL && node->m_queryPending )
		{
			node->m_queryStageCompleted = true;
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Lock::SetValue>
// Set the lock's state
//-----------------------------------------------------------------------------
bool Lock::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Bool == _value.GetID().GetType() )
	{
		ValueBool const* value = static_cast<ValueBool const*>(&_value);

		Log::Write( "Lock::Set - Requesting the node %d lock to be %s", GetNodeId(), value->GetValue() ? "Locked" : "Unlocked" );
		Msg* msg = new Msg( "LockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( LockCmd_Set );
		msg->Append( value->GetValue() ? 0xff:0x00 );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Lock::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Lock::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueBool( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Locked", "", false, false );
	}
}


