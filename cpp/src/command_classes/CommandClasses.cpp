//-----------------------------------------------------------------------------
//
//	CommandClasses.cpp
//
//	Singleton holding methods to create each command class object
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

#include <string.h>

#include "command_classes/CommandClasses.h"

using namespace OpenZWave;

#include "command_classes/Alarm.h"
#include "command_classes/ApplicationStatus.h"
#include "command_classes/Association.h"
#include "command_classes/AssociationCommandConfiguration.h"
#include "command_classes/Basic.h"
#include "command_classes/BasicWindowCovering.h"
#include "command_classes/Battery.h"
#include "command_classes/CentralScene.h"
#include "command_classes/ClimateControlSchedule.h"
#include "command_classes/Clock.h"
#include "command_classes/Color.h"
#include "command_classes/Configuration.h"
#include "command_classes/ControllerReplication.h"
#include "command_classes/CRC16Encap.h"
#include "command_classes/DeviceResetLocally.h"
#include "command_classes/DoorLock.h"
#include "command_classes/DoorLockLogging.h"
#include "command_classes/EnergyProduction.h"
#include "command_classes/Hail.h"
#include "command_classes/Indicator.h"
#include "command_classes/Language.h"
#include "command_classes/Lock.h"
#include "command_classes/ManufacturerSpecific.h"
#include "command_classes/Meter.h"
#include "command_classes/MeterPulse.h"
#include "command_classes/MultiCmd.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/MultiInstanceAssociation.h"
#include "command_classes/NodeNaming.h"
#include "command_classes/NoOperation.h"
#include "command_classes/Powerlevel.h"
#include "command_classes/Proprietary.h"
#include "command_classes/Protection.h"
#include "command_classes/SceneActivation.h"
#include "command_classes/Security.h"
#include "command_classes/SensorAlarm.h"
#include "command_classes/SensorBinary.h"
#include "command_classes/SensorMultilevel.h"
#include "command_classes/SwitchAll.h"
#include "command_classes/SwitchBinary.h"
#include "command_classes/SwitchMultilevel.h"
#include "command_classes/SwitchToggleBinary.h"
#include "command_classes/SwitchToggleMultilevel.h"
#include "command_classes/TimeParameters.h"
#include "command_classes/ThermostatFanMode.h"
#include "command_classes/ThermostatFanState.h"
#include "command_classes/ThermostatMode.h"
#include "command_classes/ThermostatOperatingState.h"
#include "command_classes/ThermostatSetpoint.h"
#include "command_classes/UserCode.h"
#include "command_classes/Version.h"
#include "command_classes/WakeUp.h"
#include "command_classes/ZWavePlusInfo.h"

#include "value_classes/ValueBool.h"
#include "value_classes/ValueButton.h"
#include "value_classes/ValueByte.h"
#include "value_classes/ValueDecimal.h"
#include "value_classes/ValueInt.h"
#include "value_classes/ValueList.h"
#include "value_classes/ValueSchedule.h"
#include "value_classes/ValueShort.h"
#include "value_classes/ValueString.h"

#include "Manager.h"
#include "Options.h"
#include "Utils.h"

//-----------------------------------------------------------------------------
//	<CommandClasses::CommandClasses>
//	Constructor
//-----------------------------------------------------------------------------
CommandClasses::CommandClasses
(
)
{
	memset( m_commandClassCreators, 0, sizeof(pfnCreateCommandClass_t)*256 );
	memset( m_supportedCommandClasses, 0, sizeof(uint32)*8 );
}

//-----------------------------------------------------------------------------
//	<CommandClasses::IsSupported>
//	Static method to determine whether a command class is supported
//-----------------------------------------------------------------------------
bool CommandClasses::IsSupported
(
	uint8 const _commandClassId
)
{
	// Test the bit representing the command class
	return( (Get().m_supportedCommandClasses[_commandClassId>>5] & (1u<<(_commandClassId&0x1f))) != 0 );
}
string CommandClasses::GetName(
	uint8 const _commandClassId
)
{
	for (std::map<string,uint8>::iterator it = Get().m_namesToIDs.begin(); it != Get().m_namesToIDs.end(); it++) {
		if (it->second == _commandClassId)
			return it->first;
	}
	return string("Unknown");
}
//-----------------------------------------------------------------------------
//	<CommandClasses::Register>
//	Static method to register a command class creator method
//-----------------------------------------------------------------------------
void CommandClasses::Register
(
	uint8 const _commandClassId,
	string const& _commandClassName,
	pfnCreateCommandClass_t _creator
)
{
	m_commandClassCreators[_commandClassId] = _creator;

	// Set the bit representing the command class
	Get().m_supportedCommandClasses[_commandClassId>>5] |= (1u<<(_commandClassId&0x1f));

	m_namesToIDs[_commandClassName] = _commandClassId;
}

//-----------------------------------------------------------------------------
//	<CommandClasses::CreateCommandClass>
//	Create a command class object using the registered method
//-----------------------------------------------------------------------------
CommandClass* CommandClasses::CreateCommandClass
(
	uint8 const _commandClassId,
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	// Get a pointer to the required CommandClass's Create method
	pfnCreateCommandClass_t creator = Get().m_commandClassCreators[_commandClassId];
	if( NULL == creator )
	{
		return NULL;
	}

	// Create an instance of the command class
	return creator( _homeId, _nodeId );
}

#define COMMAND_CLASSES \
	COMMAND_CLASS( Alarm ) \
	COMMAND_CLASS( ApplicationStatus ) \
	COMMAND_CLASS( Association ) \
	COMMAND_CLASS( AssociationCommandConfiguration ) \
	COMMAND_CLASS( Basic ) \
	COMMAND_CLASS( BasicWindowCovering ) \
	COMMAND_CLASS( Battery ) \
	COMMAND_CLASS( CentralScene ) \
	COMMAND_CLASS( ClimateControlSchedule ) \
	COMMAND_CLASS( Clock ) \
	COMMAND_CLASS( Color ) \
	COMMAND_CLASS( Configuration ) \
	COMMAND_CLASS( ControllerReplication ) \
	COMMAND_CLASS( CRC16Encap ) \
	COMMAND_CLASS( DeviceResetLocally ) \
	COMMAND_CLASS( DoorLock ) \
	COMMAND_CLASS( DoorLockLogging ) \
	COMMAND_CLASS( EnergyProduction ) \
	COMMAND_CLASS( Hail ) \
	COMMAND_CLASS( Indicator ) \
	COMMAND_CLASS( Language ) \
	COMMAND_CLASS( Lock ) \
	COMMAND_CLASS( ManufacturerSpecific ) \
	COMMAND_CLASS( Meter ) \
	COMMAND_CLASS( MeterPulse ) \
	COMMAND_CLASS( MultiCmd ) \
	COMMAND_CLASS( MultiInstance ) \
	COMMAND_CLASS( MultiInstanceAssociation ) \
	COMMAND_CLASS( NodeNaming ) \
	COMMAND_CLASS( NoOperation ) \
	COMMAND_CLASS( Powerlevel ) \
	COMMAND_CLASS( Proprietary ) \
	COMMAND_CLASS( Protection ) \
	COMMAND_CLASS( SceneActivation ) \
	COMMAND_CLASS( Security ) \
	COMMAND_CLASS( SensorAlarm ) \
	COMMAND_CLASS( SensorBinary ) \
	COMMAND_CLASS( SensorMultilevel ) \
	COMMAND_CLASS( SwitchAll ) \
	COMMAND_CLASS( SwitchBinary ) \
	COMMAND_CLASS( SwitchMultilevel ) \
	COMMAND_CLASS( SwitchToggleBinary ) \
	COMMAND_CLASS( SwitchToggleMultilevel ) \
	COMMAND_CLASS( TimeParameters ) \
	COMMAND_CLASS( ThermostatFanMode ) \
	COMMAND_CLASS( ThermostatFanState ) \
	COMMAND_CLASS( ThermostatMode ) \
	COMMAND_CLASS( ThermostatOperatingState ) \
	COMMAND_CLASS( ThermostatSetpoint ) \
	COMMAND_CLASS( UserCode ) \
	COMMAND_CLASS( Version ) \
	COMMAND_CLASS( WakeUp ) \
	COMMAND_CLASS( ZWavePlusInfo )

//-----------------------------------------------------------------------------
//	<CommandClasses::RegisterCommandClasses>
//	Register all our implemented command classes
//-----------------------------------------------------------------------------
void CommandClasses::RegisterCommandClasses
(
)
{
	CommandClasses& cc = Get();
#define COMMAND_CLASS(c) cc.Register( c::StaticGetCommandClassId(), c::StaticGetCommandClassName(), c::Create );
	COMMAND_CLASSES
#undef COMMAND_CLASS

	// Now all the command classes have been registered, we can modify the
	// supported command classes array according to the program options.
	string str;
	Options::Get()->GetOptionAsString( "Include", &str );
	if( str != "" )
	{
		// The include list has entries, so we assume that it is a
		// complete list of what should be supported.
		// Any existing support is cleared first.
		memset( cc.m_supportedCommandClasses, 0, sizeof(uint32)*8 );
		cc.ParseCommandClassOption( str, true );
	}

	// Apply the excluded command class option
	Options::Get()->GetOptionAsString( "Exclude", &str );
	if( str != "" )
	{
		cc.ParseCommandClassOption( str, false );
	}
}

//-----------------------------------------------------------------------------
//	<CommandClasses::ParseCommandClassOption>
//	Parse a comma delimited list of included/excluded command classes
//-----------------------------------------------------------------------------
void CommandClasses::ParseCommandClassOption
(
	string const& _optionStr,
	bool const _include
)
{
	size_t pos = 0;
    size_t start = 0;
	bool parsing = true;
	while( parsing )
	{
		string ccStr;

		pos = _optionStr.find_first_of( ",", start );
		if( string::npos == pos )
		{
			ccStr = _optionStr.substr( start );
			parsing = false;
		}
		else
		{
			ccStr = _optionStr.substr( start, pos-start );
			start = pos + 1;
		}

		if( ccStr != "" )
		{
			uint8 ccIdx = GetCommandClassId( ccStr );
			if( _include )
			{
				m_supportedCommandClasses[ccIdx>>5] |= (1u<<(ccIdx&0x1f));
			}
			else
			{
				m_supportedCommandClasses[ccIdx>>5] &= ~(1u<<(ccIdx&0x1f));
			}
		}
	}
}

//-----------------------------------------------------------------------------
//	<CommandClasses::GetCommandClassId>
//	Convert a command class name (e.g COMMAND_CLASS_BASIC) into its 8-bit ID
//-----------------------------------------------------------------------------
uint8 CommandClasses::GetCommandClassId
(
	string const& _name
)
{
	string upperName = ToUpper( _name );
	map<string,uint8>::iterator it = m_namesToIDs.find( upperName );
	if( it != m_namesToIDs.end() )
	{
		return it->second;
	}

	return 0xff;
}


