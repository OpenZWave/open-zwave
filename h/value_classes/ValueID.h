//-----------------------------------------------------------------------------
//
//	ValueID.h
//
//	Unique identifier for a value object
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

#ifndef _ValueID_H
#define _ValueID_H

#include "Defs.h"

namespace OpenZWave
{
	class ValueID
	{
	public:
		ValueID( uint8 const _nodeId, uint8 const _commandClassId, uint8 const _instance, uint8 const _index )
		{
			m_id = (((uint32)_nodeId)<<24) | (((uint32)_commandClassId)<<16)  | (((uint32)_instance)<<8)  | ((uint32)_index);
		}
		//ValueID( uint32 const _id ): m_id( _id ){}
		ValueID():m_id(0){}
		
		uint8 GetNodeId()const			{ return( (uint8)(m_id>>24) ); }
		uint8 GetCommandClassId()const	{ return( (uint8)(m_id>>16) ); }
		uint8 GetInstance()const		{ return( (uint8)(m_id>>8) ); }
		uint8 GetIndex()const			{ return( (uint8) m_id ); }

		bool operator == ( ValueID const& _other )const{ return( m_id == _other.m_id );	}
		bool operator != ( ValueID const& _other )const{ return( m_id != _other.m_id ); }
		bool operator < ( ValueID const& _other )const{ return( m_id < _other.m_id ); }

	private:
		uint32	m_id;
	};

} // namespace OpenZWave

#endif



