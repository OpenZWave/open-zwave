//-----------------------------------------------------------------------------
//
//	SwitchAll.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_ALL
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

#ifndef _SwitchAll_H
#define _SwitchAll_H

#include "CommandClass.h"

namespace OpenZWave
{
	class SwitchAll: public CommandClass
	{
	public:
		enum SwitchAllEnabled
		{
			SwitchAllEnabled_None	= 0x00,
			SwitchAllEnabled_Off	= 0x01,
			SwitchAllEnabled_On		= 0x02,
			SwitchAllEnabled_Both	= 0xff
		};

		static CommandClass* Create( uint8 const _nodeId ){ return new SwitchAll( _nodeId ); }
		virtual ~SwitchAll(){}

		static uint8 const StaticGetCommandClassId(){ return 0x27; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SWITCH_ALL"; }

		void On();
		void Off();

		// From CommandClass
		virtual void RequestState();
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _pData, uint32 const _length, uint32 const _instance = 0 );
		virtual bool SetValue( Value const& _value );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		SwitchAll( uint8 const _nodeId ): CommandClass( _nodeId ){}

		SwitchAllEnabled	m_state;
	};

} // namespace OpenZWave

#endif

