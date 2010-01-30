//-----------------------------------------------------------------------------
//
//	ThermostatOperatingState.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_OPERATING_STATE
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

#ifndef _ThermostatOperatingState_H
#define _ThermostatOperatingState_H

#include <vector>
#include <string>
#include "CommandClass.h"

namespace OpenZWave
{
	class ThermostatOperatingState: public CommandClass
	{
	public:
		static CommandClass* Create( uint8 const _nodeId ){ return new ThermostatOperatingState( _nodeId ); }
		virtual ~ThermostatOperatingState(){}

		static uint8 const StaticGetCommandClassId(){ return 0x42; }		
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_THERMOSTAT_OPERATING_STATE"; }

		// From CommandClass
		virtual void RequestStatic();
		virtual void RequestState();
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _pData, uint32 const _length, uint32 const _instance = 0 );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		ThermostatOperatingState( uint8 const _nodeId ): CommandClass( _nodeId ){}

		enum ThermostatOperatingStateEnum
		{
			ThermostatOperatingState_Idle = 0,
			ThermostatOperatingState_Heating,
			ThermostatOperatingState_Cooling,
			ThermostatOperatingState_FanOnly,
			ThermostatOperatingState_PendingHeat,
			ThermostatOperatingState_PendingCool,
			ThermostatOperatingState_VentOrEconomizer
		};

		vector<string>					m_supportedStates;
	};

} // namespace OpenZWave

#endif




