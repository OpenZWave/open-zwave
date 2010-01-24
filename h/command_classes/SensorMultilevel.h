//-----------------------------------------------------------------------------
//
//	SensorMultilevel.h
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

#ifndef _SensorMultilevel_H
#define _SensorMultilevel_H

#include "CommandClass.h"

namespace OpenZWave
{
	class SensorMultilevel: public CommandClass
	{
	public:
		static CommandClass* Create( uint8 const _nodeId ){ return new SensorMultilevel( _nodeId ); }
		virtual ~SensorMultilevel(){}

		static uint8 const StaticGetCommandClassId(){ return 0x31; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SENSOR_MULTILEVEL"; }

		// From CommandClass
		virtual void RequestState();
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _pData, uint32 const _length, uint32 const _instance = 0 );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		SensorMultilevel( uint8 const _nodeId ): CommandClass( _nodeId ){}

		enum SensorMultilevelCmd
		{
			SensorMultilevelCmd_Get		= 0x04,
			SensorMultilevelCmd_Report	= 0x05
		};

		enum SensorType
		{
			SensorType_Invalid		= 0x00,
			SensorType_Temperature	= 0x01,
			SensorType_General		= 0x02,
			SensorType_Luminance	= 0x03,
			SensorType_Power		= 0x04,
			SensorType_Humidity		= 0x05,
			SensorType_CO2			= 0x11
		};

		SensorType	m_sensorType;
		float32		m_value;
	};

} // namespace OpenZWave


#endif

