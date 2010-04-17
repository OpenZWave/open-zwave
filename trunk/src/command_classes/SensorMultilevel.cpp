//-----------------------------------------------------------------------------
//
//	SensorMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_MULTILEVEL
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
#include "SensorMultilevel.h"
#include "MultiInstance.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueDecimal.h"

using namespace OpenZWave;

enum SensorMultilevelCmd
{
	SensorMultilevelCmd_Get		= 0x04,
	SensorMultilevelCmd_Report	= 0x05
};


//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void SensorMultilevel::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		uint8 numInstances = GetInstances();
		if( numInstances > 1 )
		{
			// More than one instance - query each one in turn
			for( uint8 i=0; i<numInstances; ++i )
			{
				Msg* msg = new Msg( "SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, MultiInstance::StaticGetCommandClassId() );
				msg->Append( GetNodeId() );
				msg->Append( 5 );
				msg->Append( MultiInstance::StaticGetCommandClassId() );
				msg->Append( MultiInstance::MultiInstanceCmd_CmdEncap );
				msg->Append( i+1 );
				msg->Append( GetCommandClassId() );
				msg->Append( SensorMultilevelCmd_Get );
				msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
				GetDriver()->SendMsg( msg );
			}
		}
		else
		{
			Msg* msg = new Msg( "SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( SensorMultilevelCmd_Get );
			msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			GetDriver()->SendMsg( msg );
		}
	}
}

//-----------------------------------------------------------------------------
// <SensorMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SensorMultilevel::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( Node* node = GetNode() )
	{
		if (SensorMultilevelCmd_Report == (SensorMultilevelCmd)_data[0])
		{
			uint8 scale;
			string valueStr = ExtractValueAsString( &_data[2], &scale );

			if( ValueDecimal* value = node->GetValueDecimal(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0 ) )
			{
				switch( _data[1] )
				{
					case 0x01:
					{
						// Temperature
						value->SetLabel( "Temperature" );
						value->SetUnits( scale ? "F" : "C" );
						break;
					}
					case 0x02:
					{
						// General
						value->SetLabel( "Sensor" );
						value->SetUnits( scale ? "" : "%" );
						break;
					}
					case 0x03:
					{
						// Luminance
						value->SetLabel( "Luminance" );
						value->SetUnits( scale ? "Lux" : "%" );
						break;
					}
					case 0x04:
					{
						// Power
						value->SetLabel( "Power" );
						value->SetUnits( scale ? "W" : "" );
						break;
					}
					case 0x05:
					{
						// Humidity
						value->SetLabel( "Humidity" );
						value->SetUnits( "%" );
						break;
					}
					case 0x11:
					{
						// CO2
						value->SetLabel( "Carbon Monoxide" );
						value->SetUnits( "ppm" );
						break;
					}
				}

				value->OnValueChanged( valueStr );

				Log::Write( "Received SensorMultiLevel report from node %d, instance %d: value=%s%s", GetNodeId(), _instance, valueStr.c_str(), value->GetUnits().c_str() );
				value->Release();
			}

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SensorMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SensorMultilevel::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNode() )
	{
		node->CreateValueDecimal(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Unknown", "", true, "0.0"  );
	}
}



