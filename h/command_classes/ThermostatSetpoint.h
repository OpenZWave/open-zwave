//-----------------------------------------------------------------------------
//
//	ThermostatSetpoint.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_SETPOINT
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

#ifndef _ThermostatSetpoint_H
#define _ThermostatSetpoint_H

#include <vector>
#include <string>
#include "CommandClass.h"

namespace OpenZWave
{
	class ThermostatSetpoint: public CommandClass
	{
	public:
		static CommandClass* Create( uint8 const _nodeId ){ return new ThermostatSetpoint( _nodeId ); }
		virtual ~ThermostatSetpoint(){ memset( m_supportedSetpoints, 0, sizeof(m_supportedSetpoints) ); } 

		static uint8 const StaticGetCommandClassId(){ return 0x43; }		
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_THERMOSTAT_SETPOINT"; }

		// From CommandClass
		virtual void RequestStatic();
		virtual void RequestState();
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _pData, uint32 const _length, uint32 const _instance = 0 );
		virtual bool SetValue( Value const& _value );

	private:
		ThermostatSetpoint( uint8 const _nodeId ): CommandClass( _nodeId ){}

		enum ThermostatSetpointEnum
		{
			ThermostatSetpoint_Heating1,
			ThermostatSetpoint_Cooling1,
			ThermostatSetpoint_Furnace,
			ThermostatSetpoint_DryAir,
			ThermostatSetpoint_MoistAir,
			ThermostatSetpoint_AutoChangeover,
			ThermostatSetpoint_Count
		};

		bool m_supportedSetpoints[ThermostatSetpoint_Count];
	};

} // namespace OpenZWave

#endif



