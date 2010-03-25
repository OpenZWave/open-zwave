//-----------------------------------------------------------------------------
//
//	Alarm.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ALARM
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
#include "Alarm.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"

using namespace OpenZWave;

enum AlarmCmd
{
	AlarmCmd_Get	= 0x04,
	AlarmCmd_Report = 0x05
};

enum
{
	ValueIndex_Type	= 0,
	ValueIndex_Level
};


//-----------------------------------------------------------------------------
// <Alarm::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void Alarm::RequestState
(
	uint8 const _instance
)
{
	Log::Write( "Requesting the alarm status from node %d", GetNodeId() );
	Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( AlarmCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Alarm::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Alarm::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (AlarmCmd_Report == (AlarmCmd)_data[0])
	{
		// We have received a report from the Z-Wave device
		// No known mappings for these values yet
		if( Node* node = GetNode() )
		{
			ValueByte* value;

			// Alarm Type
			if( value = node->GetValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueIndex_Type ) )
			{
				value->OnValueChanged( _data[1] );
			}
		
			// Alarm Level
			if( value = node->GetValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueIndex_Level ) )
			{
				value->OnValueChanged( _data[2] );
			}

			Log::Write( "Received Alarm report from node %d: type=%d, level=%d", GetNodeId(), _data[1], _data[2] );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Alarm::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNode() )
	{
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueIndex_Type, "Alarm Type", "", true, 0 );
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueIndex_Level, "Alarm Level", "", true, 0 );
	}
}

