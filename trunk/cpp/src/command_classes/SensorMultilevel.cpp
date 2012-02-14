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
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestValue>												   
// Request current value from the device									   
//-----------------------------------------------------------------------------
bool SensorMultilevel::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy,		// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	Msg* msg = new Msg( "SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SensorMultilevelCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg, _queue );
	return true;
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
		uint8 precision = 0;
		uint8 sensorType = _data[1];
		string valueStr = ExtractValue( &_data[2], &scale, &precision );

		Node* node = GetNodeUnsafe();
		if( node != NULL )
		{
			ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, sensorType ) );
			if( value == NULL)
			{
				char const* units = "";
				switch( sensorType )
				{
					case SensorType_Temperature:			units = scale ? "F" : "C";				break;
					case SensorType_General:				units = scale ? "" : "%";				break;
					case SensorType_Luminance:				units = scale ? "lux" : "%";			break;
					case SensorType_Power:					units = scale ? "BTU/h" : "W";			break;
					case SensorType_RelativeHumidity:		units = "%";							break;
					case SensorType_Velocity:				units = scale ? "mph" : "m/s";			break;
					case SensorType_Direction:				units = "";								break;
					case SensorType_AtmosphericPressure:	units = scale ? "inHg" : "kPa";			break;
					case SensorType_BarometricPressure:		units = scale ? "inHg" : "kPa";			break;
					case SensorType_SolarRadiation:			units = "W/m2";							break;
					case SensorType_DewPoint:				units = scale ? "in/h" : "mm/h";		break;
					case SensorType_RainRate:				units = scale ? "F" : "C";				break;
					case SensorType_TideLevel:				units = scale ? "ft" : "m";				break;
					case SensorType_Weight:					units = scale ? "lb" : "kg";			break;
					case SensorType_Voltage:				units = scale ? "mV" : "V";				break;
					case SensorType_Current:				units = scale ? "mA" : "A";				break;
					case SensorType_CO2:					units = "ppm";							break;
					case SensorType_AirFlow:				units = scale ? "cfm" : "m3/h";			break;
					case SensorType_TankCapacity:			units = c_tankCapcityUnits[scale];		break;
					case SensorType_Distance:				units = c_distanceUnits[scale];			break;
					default:																		break;
				}
				node->CreateValueDecimal(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, sensorType, c_sensorTypeNames[sensorType], units, true, false, "0.0"  );
				value = static_cast<ValueDecimal*>( GetValue( _instance, sensorType ) );
			}

			Log::Write( "Received SensorMultiLevel report from node %d, instance %d: value=%s%s", GetNodeId(), _instance, valueStr.c_str(), value->GetUnits().c_str() );
			if( value->GetPrecision() != precision )
			{
				value->SetPrecision( precision );
			}
			value->OnValueChanged( valueStr );
			value->Release();
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
	// Don't create anything here. We do it in the report.
}



