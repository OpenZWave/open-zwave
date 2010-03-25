//-----------------------------------------------------------------------------
//
//	Protection.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_PROTECTION
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

#ifndef _Protection_H
#define _Protection_H

#include "CommandClass.h"

namespace OpenZWave
{
	class Protection: public CommandClass
	{
	public:
		enum ProtectionEnum
		{
			Protection_Unprotected = 0,
			Protection_Sequence,
			Protection_NOP
		};

		static CommandClass* Create( uint8 const _driverId, uint8 const _nodeId ){ return new Protection( _driverId, _nodeId ); }
		virtual ~Protection(){}

		static uint8 const StaticGetCommandClassId(){ return 0x75; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_PROTECTION"; }

		// From CommandClass
		virtual void RequestState( uint8 const _instance );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		virtual bool SetValue( Value const& _value );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		Protection( uint8 const _driverId, uint8 const _nodeId ): CommandClass( _driverId, _nodeId ){}
	};

} // namespace OpenZWave

#endif

