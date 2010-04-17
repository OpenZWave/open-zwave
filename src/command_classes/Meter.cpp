//-----------------------------------------------------------------------------
//
//	Meter.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_METER
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
#include "Meter.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueDecimal.h"

using namespace OpenZWave;


enum MeterCmd
{
	MeterCmd_Get	= 0x04,
	MeterCmd_Report	= 0x05
};

//-----------------------------------------------------------------------------
// <Meter::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void Meter::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		Msg* msg = new Msg( "MeterCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( MeterCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <Meter::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Meter::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( Node* node = GetNode() )
	{
		if (MeterCmd_Report == (MeterCmd)_data[0])
		{
			uint8 scale;
			string valueStr = ExtractValueAsString( &_data[2], &scale );

			if( ValueDecimal* value = node->GetValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0 ) )
			{
				if( value->GetLabel() == "Unknown" )
				{
					switch( _data[1] )
					{
						case 0x01:
						{
							// Electricity Meter
							value->SetLabel( "Electricity" );
							value->SetUnits( "kWh" );
							break;
						}
						case 0x02:
						{
							// Gas Meter
							value->SetLabel( "Gas" );
							value->SetUnits( "" );
							break;
						}
						case 0x03:
						{
							// Water Meter
							value->SetLabel( "Water" );
							value->SetUnits( "" );
							break;
						}
					}
				}

				value->OnValueChanged( valueStr );
				value->Release();
			}

			Log::Write( "Received Meter report from node %d: value=%s", GetNodeId(), valueStr );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Meter::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Meter::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNode() )
	{
		node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Unknown", "", true, "0.0" );
	}
}



