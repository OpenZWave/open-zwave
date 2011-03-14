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
	MeterCmd_Get				= 0x01,
	MeterCmd_Report				= 0x02,
	MeterCmd_GetSupported		= 0x03,		// Version 2 Only
	MeterCmd_ReportSupported	= 0x04,		// Version 2 Only
	MeterCmd_Reset				= 0x05		// Version 2 Only
};

static char const* c_electricityUnits[] = 
{
	"kWh",
	"kVAh",
	"W",
	""
};

static char const* c_gasUnits[] = 
{
	"cubic meters",
	"cubic feet",
	"",
	""
};

static char const* c_waterUnits[] = 
{
	"cubic meters",
	"cubic feet",
	"US gallons",
	""
};

//-----------------------------------------------------------------------------
// <Meter::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
bool Meter::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		RequestValue();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Meter::RequestValue>												   
// Request current value from the device									   
//-----------------------------------------------------------------------------
void Meter::RequestValue
(
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _dummy2		// = 0 (not used)
)
{
	Msg* msg = new Msg( "MeterCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( MeterCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
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
	if (MeterCmd_Report == (MeterCmd)_data[0])
	{
		uint8 scale;
		string valueStr = ExtractValue( &_data[2], &scale );

		if( ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, 0 ) ) )
		{
			if( value->GetLabel() == "Unknown" )
			{
				switch( _data[1] )
				{
					case 0x01:
					{
						// Electricity Meter
						value->SetLabel( "Electricity" );
						value->SetUnits( c_electricityUnits[scale] );
						break;
					}
					case 0x02:
					{
						// Gas Meter
						value->SetLabel( "Gas" );
						value->SetUnits( c_gasUnits[scale] );
						break;
					}
					case 0x03:
					{
						// Water Meter
						value->SetLabel( "Water" );
						value->SetUnits( c_waterUnits[scale] );
						break;
					}
				}
			}

			value->OnValueChanged( valueStr );
		}

		Log::Write( "Received Meter report from node %d: value=%s", GetNodeId(), valueStr.c_str() );
		return true;
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
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Unknown", "", true, "0.0" );
	}
}



