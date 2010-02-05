//-----------------------------------------------------------------------------
//
//	SwitchMultilevel.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_MULTILEVEL
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

#ifndef _SwitchMultilevel_H
#define _SwitchMultilevel_H

#include "CommandClass.h"

namespace OpenZWave
{
	class SwitchMultilevel: public CommandClass
	{
	public:
		enum SwitchMultilevelDirection
		{
			SwitchMultilevelDirection_Up	= 0x00,
			SwitchMultilevelDirection_Down	= 0x40
		};

		static CommandClass* Create( uint8 const _nodeId ){ return new SwitchMultilevel( _nodeId ); }
		virtual ~SwitchMultilevel(){}

		static uint8 const StaticGetCommandClassId(){ return 0x26; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SWITCH_MULTILEVEL"; }

		void StartLevelChange( SwitchMultilevelDirection const _direction, bool const _bIgnoreStartLevel, bool const _bRollover );
		void StopLevelChange();
		void EnableLevelChange( bool const _bState );

		// From CommandClass
		virtual void RequestState();
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 0 );
		virtual bool SetValue( Value const& _value );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		SwitchMultilevel( uint8 const _nodeId ): CommandClass( _nodeId ){}
	};

} // namespace OpenZWave

#endif

