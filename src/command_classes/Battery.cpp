//-----------------------------------------------------------------------------
//
//	Battery.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BATTERY
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
#include "Battery.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"

using namespace OpenZWave;

enum BatteryCmd
{
	BatteryCmd_Get		= 0x02,
	BatteryCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <Battery::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void Battery::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		Msg* msg = new Msg( "BatteryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( BatteryCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <Battery::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Battery::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (BatteryCmd_Report == (BatteryCmd)_data[0])
	{
		// We have received a battery level report from the Z-Wave device.
		// Devices send 0xff instead of zero for a low battery warning.
		uint8 batteryLevel = _data[1];
		if( batteryLevel == 0xff )
		{
			batteryLevel = 0;
		}

		Log::Write( "Received Battery report from node %d: level=%d", GetNodeId(), batteryLevel );

		if( ValueByte* value = m_level.GetInstance( _instance ) )
		{
			value->OnValueChanged( batteryLevel );
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Battery::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Battery::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		m_level.AddInstance( _instance, node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Battery Level", "%", true, 100 ) );
	}
}


