//-----------------------------------------------------------------------------
//
//	MeterPulse.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_METER_PULSE
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
#include "MeterPulse.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueInt.h"

using namespace OpenZWave;

enum MeterPulseCmd
{
	MeterPulseCmd_Get		= 0x04,
	MeterPulseCmd_Report	= 0x05
};


//-----------------------------------------------------------------------------
// <MeterPulse::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void MeterPulse::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		Msg* msg = new Msg( "MeterPulseCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( MeterPulseCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <MeterPulse::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MeterPulse::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( MeterPulseCmd_Report == (MeterPulseCmd)_data[0] )
	{
		int32 count = 0;
		for( uint8 i=0; i<4; ++i )
		{
			count <<= 8;
			count |= (uint32)_data[i+1];
		}

		Log::Write( "Received a meter pulse count from node %d: Count=%d", GetNodeId(), count );

		if( ValueInt* value = m_count.GetInstance( _instance ) )
		{
			value->OnValueChanged( count );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <MeterPulse::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void MeterPulse::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNode() )
	{
		m_count.AddInstance( _instance, node->CreateValueInt( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Count", "", true, 0 ) );
		ReleaseNode();
	}
}



