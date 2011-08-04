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
#include "Bitfield.h"
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

enum SensorType
{
	SensorType_Temperature = 1,
	SensorType_General,
	SensorType_Luminance,
	SensorType_Power,
	SensorType_RelativeHumidity,
	SensorType_Velocity,
	SensorType_Direction,
	SensorType_AtmosphericPressure,
	SensorType_BarometricPressure,
	SensorType_SolarRadiation,
	SensorType_DewPoint,
	SensorType_RainRate,
	SensorType_TideLevel,
	SensorType_Weight,
	SensorType_Voltage,
	SensorType_Current,
	SensorType_CO2,
	SensorType_AirFlow,
	SensorType_TankCapacity,
	SensorType_Distance
};

static char const* c_sensorTypeNames[] = 
{
	"Undefined",
	"Temperature",
	"General",
	"Luminance",
	"Power",
	"Relative Humidity",
	"Velocity",
	"Direction",
	"Atmospheric Pressure",
	"Barometric Pressure",
	"Solar Radiation",
	"Dew Point",
	"Rain Rate",
	"Tide Level",
	"Weight",
	"Voltage",
	"Current",
	"CO2 Level",
	"Air Flow",
	"Tank Capacity",
	"Distance"
};

static char const* c_tankCapcityUnits[] = 
{
	"l",
	"cbm",
	"gal"
};

static char const* c_distanceUnits[] = 
{
	"m",
	"cm",
	"ft"
};

//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
bool SensorMultilevel::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			if( MultiInstance* multiInstance = static_cast<MultiInstance*>( node->GetCommandClass( MultiInstance::StaticGetCommandClassId() ) ) )
			{
				Bitfield const* instances = GetInstances();
				for( Bitfield::Iterator it = instances->Begin(); it != instances->End(); ++it )
				{
					RequestValue( 0, (uint8)*it );
				}
			}
			return true;
		}
		else
		{
			RequestValue();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestValue>												   
// Request current value from the device									   
//-----------------------------------------------------------------------------
void SensorMultilevel::RequestValue
(
	uint8 const _dummy,		// = 0 (not used)
	uint8 const _instance
)
{
	if( _instance > 1 )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			if( MultiInstance* multiInstance = static_cast<MultiInstance*>( node->GetCommandClass( MultiInstance::StaticGetCommandClassId() ) ) )
			{
				Log::Write( "Sending SensorMultilevelCmd_Get to node %d for instance/endpoint %d", GetNodeId(), _instance );
				uint8 data[2];
				data[0] = GetCommandClassId();
				data[1] = SensorMultilevelCmd_Get;
				multiInstance->SendEncap( data, 2, _instance );
			}
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
	if (SensorMultilevelCmd_Report == (SensorMultilevelCmd)_data[0])
	{
		uint8 scale;
		string valueStr = ExtractValue( &_data[2], &scale );

		if( ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, 0 ) ) )
		{
			value->SetLabel( c_sensorTypeNames[_data[1]] );
			switch( _data[1] )
			{
				case SensorType_Temperature:			value->SetUnits( scale ? "F" : "C" );			break;
				case SensorType_General:				value->SetUnits( scale ? "" : "%" );			break;
				case SensorType_Luminance:				value->SetUnits( scale ? "lux" : "%" );			break;
				case SensorType_Power:					value->SetUnits( scale ? "BTU/h" : "W" );		break;
				case SensorType_RelativeHumidity:		value->SetUnits( "%" );							break;
				case SensorType_Velocity:				value->SetUnits( scale ? "mph" : "m/s" );		break;
				case SensorType_Direction:				value->SetUnits( "" );							break;
				case SensorType_AtmosphericPressure:	value->SetUnits( scale ? "inHg" : "kPa" );		break;
				case SensorType_BarometricPressure:		value->SetUnits( scale ? "inHg" : "kPa" );		break;
				case SensorType_SolarRadiation:			value->SetUnits( "W/m2" );						break;
				case SensorType_DewPoint:				value->SetUnits( scale ? "in/h" : "mm/h" );		break;
				case SensorType_RainRate:				value->SetUnits( scale ? "F" : "C" );			break;
				case SensorType_TideLevel:				value->SetUnits( scale ? "ft" : "m" );			break;
				case SensorType_Weight:					value->SetUnits( scale ? "lb" : "kg" );			break;
				case SensorType_Voltage:				value->SetUnits( scale ? "mV" : "V" );			break;
				case SensorType_Current:				value->SetUnits( scale ? "mA" : "A" );			break;
				case SensorType_CO2:					value->SetUnits( "ppm" );						break;
				case SensorType_AirFlow:				value->SetUnits( scale ? "cfm" : "m3/h" );		break;
				case SensorType_TankCapacity:			value->SetUnits( c_tankCapcityUnits[scale] );	break;
				case SensorType_Distance:				value->SetUnits( c_distanceUnits[scale] );		break;
				default:																				break;
			}

			Log::Write( "Received SensorMultiLevel report from node %d, instance %d: value=%s%s", GetNodeId(), _instance, valueStr.c_str(), value->GetUnits().c_str() );
			value->OnValueChanged( valueStr );
		}

		return true;
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
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueDecimal(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Unknown", "", true, "0.0"  );
	}
}



