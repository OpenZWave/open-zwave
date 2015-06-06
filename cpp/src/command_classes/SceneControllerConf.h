//-----------------------------------------------------------------------------
//
//	SceneControllerConf.h
//
//	Implementation of the Z-Wave  COMMAND_CLASS_SCENE_CONTROLLER_CONF
//
//	Copyright (c) 2015
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

#ifndef _SceneControllerConf_H
#define _SceneControllerConf_H

#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements  COMMAND_CLASS_SCENE_CONTROLLER_CONF  (0x2D), a Z-Wave device command class.
	 */
	class SceneControllerConf: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new SceneControllerConf( _homeId, _nodeId ); }
		virtual ~SceneControllerConf(){}

		static uint8 const StaticGetCommandClassId(){ return 0x2d; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SCENE_CONTROLLER_CONF"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool SetValue( Value const& _value );
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

	private:
		SceneControllerConf( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){}

		void Set(uint8 const _group, uint8 const _sceneId, uint8 const _duration );
	};

} // namespace OpenZWave

#endif

