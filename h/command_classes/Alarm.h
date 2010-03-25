//-----------------------------------------------------------------------------
//
//	Alarm.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_ALARM
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

#ifndef _Alarm_H
#define _Alarm_H

#include "CommandClass.h"

namespace OpenZWave
{
	class Alarm: public CommandClass
	{
	public:
		static CommandClass* Create( uint8 const _driverId, uint8 const _nodeId ){ return new Alarm( _driverId, _nodeId ); }
		virtual ~Alarm(){}

		static uint8 const StaticGetCommandClassId(){ return 0x71; }		
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_ALARM"; }

		// From CommandClass
		virtual void RequestState( uint8 const _instance );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }		
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		Alarm( uint8 const _driverId, uint8 const _nodeId ): CommandClass( _driverId, _nodeId ){}
	};

} // namespace OpenZWave

#endif
