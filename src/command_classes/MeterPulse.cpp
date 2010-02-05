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
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
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
#include "ValueStore.h"

using namespace OpenZWave;

static enum MeterPulseCmd
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
)
{
	Log::Write( "Requesting the meter pulse count from node %d", GetNodeId() );
	Msg* msg = new Msg( "MeterPulseCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( MeterPulseCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <MeterPulse::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MeterPulse::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if( MeterPulseCmd_Report == (MeterPulseCmd)_data[0] )
	{
		Node* node = GetNode();
		if( node )
		{
			ValueStore* store = node->GetValueStore();
			if( store )
			{
 				int32 count = 0;
				for( uint8 i=0; i<4; ++i )
				{
					count <<= 8;
					count |= (uint32)_data[i+1];
				}

				if( ValueInt* value = static_cast<ValueInt*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					value->OnValueChanged( count );
				}
				node->ReleaseValueStore();

				Log::Write( "Received a meter pulse count from node %d: Count=%d", GetNodeId(), count );
				return true;
			}
		}
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
	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			Value* value = new ValueInt( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Count", true, 0 );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}



