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

#include "CommandClasses.h"

using namespace OpenZWave;

#include "Alarm.h"
#include "ApplicationStatus.h"
#include "Association.h"
#include "AssociationCommandConfiguration.h"
#include "Basic.h"
#include "BasicWindowCovering.h"
#include "Battery.h"
#include "ClimateControlSchedule.h"
#include "Clock.h"
#include "Configuration.h"
#include "ControllerReplication.h"
#include "EnergyProduction.h"
#include "Hail.h"
#include "Indicator.h"
#include "Language.h"
#include "Lock.h"
#include "ManufacturerSpecific.h"
#include "Meter.h"
#include "MeterPulse.h"
#include "MultiCmd.h"
#include "MultiInstance.h"
#include "MultiInstanceAssociation.h"
#include "NodeNaming.h"
#include "Powerlevel.h"
#include "Proprietary.h"
#include "Protection.h"
#include "SensorBinary.h"
#include "SensorMultilevel.h"
#include "SwitchAll.h"
#include "SwitchBinary.h"
#include "SwitchMultilevel.h"
#include "SwitchToggleBinary.h"
#include "SwitchToggleMultilevel.h"
#include "ThermostatFanMode.h"
#include "ThermostatFanState.h"
#include "ThermostatMode.h"
#include "ThermostatOperatingState.h"
#include "ThermostatSetpoint.h"
#include "Version.h"
#include "WakeUp.h"

//-----------------------------------------------------------------------------
//	<CommandClasses::CommandClasses>
//	Constructor
//-----------------------------------------------------------------------------
CommandClasses::CommandClasses
(
)
{ 
	memset( m_commandClassCreators, 0, sizeof(pfnCreateCommandClass_t)*256 );
}

//-----------------------------------------------------------------------------
//	<CommandClasses::Register>
//	Static method to register a command class creator method
//-----------------------------------------------------------------------------
void CommandClasses::Register
( 
	uint8 const _commandClassId, 
	pfnCreateCommandClass_t _creator
)
{
	m_commandClassCreators[_commandClassId] = _creator;
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

//-----------------------------------------------------------------------------
//	<CommandClasses::RegisterCommandClasses>
//	Register all our implemented command classes
//-----------------------------------------------------------------------------
void CommandClasses::RegisterCommandClasses
(
)
{
	CommandClasses& cc = Get();
	cc.Register( Alarm::StaticGetCommandClassId(), Alarm::Create );
	cc.Register( ApplicationStatus::StaticGetCommandClassId(), ApplicationStatus::Create );
	cc.Register( Association::StaticGetCommandClassId(), Association::Create );
	cc.Register( AssociationCommandConfiguration::StaticGetCommandClassId(), AssociationCommandConfiguration::Create );
	cc.Register( Basic::StaticGetCommandClassId(), Basic::Create );
	//cc.Register( BasicWindowCovering::StaticGetCommandClassId(), BasicWindowCovering::Create );
	cc.Register( Battery::StaticGetCommandClassId(), Battery::Create );
	cc.Register( ClimateControlSchedule::StaticGetCommandClassId(), ClimateControlSchedule::Create );
	cc.Register( Clock::StaticGetCommandClassId(), Clock::Create );
	cc.Register( Configuration::StaticGetCommandClassId(), Configuration::Create );
	cc.Register( ControllerReplication::StaticGetCommandClassId(), ControllerReplication::Create );
	cc.Register( EnergyProduction::StaticGetCommandClassId(), EnergyProduction::Create );
	cc.Register( Hail::StaticGetCommandClassId(), Hail::Create );
	cc.Register( Indicator::StaticGetCommandClassId(), Indicator::Create );
	cc.Register( Language::StaticGetCommandClassId(), Language::Create );
	cc.Register( Lock::StaticGetCommandClassId(), Lock::Create );
	cc.Register( ManufacturerSpecific::StaticGetCommandClassId(), ManufacturerSpecific::Create );
	cc.Register( Meter::StaticGetCommandClassId(), Meter::Create );
	cc.Register( MeterPulse::StaticGetCommandClassId(), MeterPulse::Create );
	cc.Register( MultiCmd::StaticGetCommandClassId(), MultiCmd::Create );
	cc.Register( MultiInstance::StaticGetCommandClassId(), MultiInstance::Create );
	cc.Register( MultiInstanceAssociation::StaticGetCommandClassId(), MultiInstanceAssociation::Create );
	cc.Register( NodeNaming::StaticGetCommandClassId(), NodeNaming::Create );
	cc.Register( Powerlevel::StaticGetCommandClassId(), Powerlevel::Create );
	cc.Register( Proprietary::StaticGetCommandClassId(), Proprietary::Create );
	cc.Register( Protection::StaticGetCommandClassId(), Protection::Create );
	cc.Register( SensorBinary::StaticGetCommandClassId(), SensorBinary::Create );
	cc.Register( SensorMultilevel::StaticGetCommandClassId(), SensorMultilevel::Create );
	cc.Register( SwitchAll::StaticGetCommandClassId(), SwitchAll::Create );
	cc.Register( SwitchBinary::StaticGetCommandClassId(), SwitchBinary::Create );
	cc.Register( SwitchMultilevel::StaticGetCommandClassId(), SwitchMultilevel::Create );
	cc.Register( SwitchToggleBinary::StaticGetCommandClassId(), SwitchToggleBinary::Create );
	cc.Register( SwitchToggleMultilevel::StaticGetCommandClassId(), SwitchToggleMultilevel::Create );
	cc.Register( ThermostatFanMode::StaticGetCommandClassId(), ThermostatFanMode::Create );
	cc.Register( ThermostatFanState::StaticGetCommandClassId(), ThermostatFanState::Create );
	cc.Register( ThermostatMode::StaticGetCommandClassId(), ThermostatMode::Create );
	cc.Register( ThermostatOperatingState::StaticGetCommandClassId(), ThermostatOperatingState::Create );
	cc.Register( ThermostatSetpoint::StaticGetCommandClassId(), ThermostatSetpoint::Create );
	cc.Register( Version::StaticGetCommandClassId(), Version::Create );
	cc.Register( WakeUp::StaticGetCommandClassId(), WakeUp::Create );
}
