//-----------------------------------------------------------------------------
//
//	AssociationGroupInfo.h
//
//	Implementation of the Z-Wave COMMAND_ASSOCIATION_GRP_INFO
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

#ifndef _AssociationGroupInfo_H
#define _AssociationGroupInfo_H

#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_ASSOCIATION_GRP_INFO (0x59), a Z-Wave device command class.
	 */
	class AssociationGroupInfo: public CommandClass
	{
		friend class Group;
		friend class Node;

	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new AssociationGroupInfo( _homeId, _nodeId ); }
		virtual ~AssociationGroupInfo(){}

		static uint8 const StaticGetCommandClassId(){ return 0x59; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_ASSOCIATION_GRP_INFO"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );						
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

	private:
		AssociationGroupInfo( uint32 const _homeId, uint8 const _nodeId );
		void GetGroupName( uint8 const _groupIdx );
		void GetGroupInfo( uint8 const _groupIdx );
		void GetGroupCmdInfo( uint8 const _groupIdx );
	};

} // namespace OpenZWave

#endif

