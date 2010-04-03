//-----------------------------------------------------------------------------
//
//	Basic.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BASIC
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
#include "Basic.h"
#include "Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"

using namespace OpenZWave;

enum BasicCmd
{
	BasicCmd_Set	= 0x01,
	BasicCmd_Get	= 0x02,
	BasicCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <Basic::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void Basic::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		Msg* msg = new Msg( "BasicCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( BasicCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <Basic::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Basic::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	bool handled = false;

	if( Node* node = GetNode() )
	{
		if( BasicCmd_Report == (BasicCmd)_data[0] )
		{
			// Level
			if( ValueByte* value = node->GetValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0 ) )
			{
				value->OnValueChanged( _data[1] );
			}

			Log::Write( "Received Basic report from node %d: level=%d", GetNodeId(), _data[1] );
			handled = true;
		}

		if( BasicCmd_Set == (BasicCmd)_data[0] )
		{
			// Level
			if( ValueByte* value = node->GetValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0 ) )
			{
				value->OnValueChanged( _data[1] );
			}

			Log::Write( "Received Basic set from node %d: level=%d", GetNodeId(), _data[1] );
			handled = true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Basic::SetValue>
// Set a value on the Z-Wave device
//-----------------------------------------------------------------------------
bool Basic::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Byte == _value.GetID().GetType() )
	{
		ValueByte const* value = static_cast<ValueByte const*>(&_value);
	
		Log::Write( "Basic::Set - Setting node %d to level %d", GetNodeId(), value->GetPending() );
		Msg* msg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );		
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( BasicCmd_Set );
		msg->Append( value->GetPending() );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Basic::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Basic::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNode() )
	{
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Level", "", false, 0 );
	}
}
