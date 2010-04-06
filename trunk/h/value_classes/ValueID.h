//-----------------------------------------------------------------------------
//
//	ValueID.h
//
//	Unique identifier for a Value object
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

#ifndef _ValueID_H
#define _ValueID_H

#include <string>
#include <assert.h>
#include "Defs.h"

class TiXmlElement;

namespace OpenZWave
{
	/** 
	 * Provides a unique ID for a value reported by a Z-Wave device.
	 * The ValueID is used to uniquely identify a value reported by a 
	 * Z-Wave device.
	 * <p>
	 * The ID is built by packing various identifying characteristics into a single
	 * 32-bit number - the Z-Wave driver index, device node ID, the command class and
	 * command class instance that handles the value, plus an index for the value 
	 * to distinguish it among all the other values managed by that command class 
	 * instance.  The type (bool, byte, string etc) of the value is also stored.
	 * <p>
	 * The packing of the ID is such that a list of Values sorted by ValueID
	 * will be in a sensible order for display to the user.
	 */
	class ValueID
	{
	public:
		enum ValueGenre
		{
			ValueGenre_All = 0,
			ValueGenre_User,			// Basic values an ordinary user would be interested in
			ValueGenre_Config,			// Device-specific configuration parameters
			ValueGenre_System,			// Values of significance only to users who understand the Z-Wave protocol 
			ValueGenre_Count
		};

		enum ValueType
		{
			ValueType_Bool = 0,
			ValueType_Byte,
			ValueType_Decimal,
			ValueType_Int,
			ValueType_List,
			ValueType_Short,
			ValueType_String,
			ValueType_Count
		};

		// Constructor
		ValueID
		( 
			uint32 const _homeId,
			uint8 const	_nodeId,
			ValueGenre const _genre,
			uint8 const _commandClassId,
			uint8 const _instance,
			uint8 const _valueIndex,
			ValueType const _type
		):
			m_homeId( _homeId )
		{
			assert( ((uint32)_genre) < 16 );
			assert( _instance < 16 );
			assert( _valueIndex < 16 );
			assert( ((uint32)_type) < 16 );

			m_id = (((uint32)_nodeId)<<24)
				 | (((uint32)_genre)<<20)
				 | (((uint32)_commandClassId)<<12)
				 | (((uint32)_instance)<<8)
				 | (((uint32)_valueIndex)<<4)
				 | ((uint32)_type);
		}

		// Construct a value id for use in notifications
		ValueID( uint32 const _homeId, uint8 const _nodeId ): m_homeId( _homeId ){ m_id = ((uint32)_nodeId)<<24; }

		// Default constructor
		ValueID():m_homeId(0),m_id(0){}

		// Accessors
		uint32		GetHomeId()const			{ return m_homeId; }
		uint8		GetNodeId()const			{ return( (uint8)		( (m_id & 0xff000000) >> 24 ) ); }
		ValueGenre	GetGenre()const				{ return( (ValueGenre)	( (m_id & 0x00f00000) >> 20 ) ); }
		uint8		GetCommandClassId()const	{ return( (uint8)		( (m_id & 0x000ff000) >> 12 ) ); }
		uint8		GetInstance()const			{ return( (uint8)		( (m_id & 0x00000f00) >> 8  ) ); }
		uint8		GetIndex()const				{ return( (uint8)		( (m_id & 0x000000f0) >> 4  ) ); }
		ValueType	GetType()const				{ return( (ValueType)	(  m_id & 0x0000000f        ) ); }

		// Comparison Operators
		bool operator ==	( ValueID const& _other )const{ return( (m_homeId == _other.m_homeId) && (m_id == _other.m_id) ); }
		bool operator !=	( ValueID const& _other )const{ return( (m_homeId != _other.m_homeId) || (m_id != _other.m_id) ); }
		bool operator <		( ValueID const& _other )const{ return( (m_homeId == _other.m_homeId) ? (m_id < _other.m_id) : (m_homeId < _other.m_homeId) ); }
		bool operator >		( ValueID const& _other )const{ return( (m_homeId == _other.m_homeId) ? (m_id > _other.m_id) : (m_homeId > _other.m_homeId) ); }

	private:
		// ID Packing:
		// Bits
		// 24-31:	8 bits. Node ID of device
		// 20-23:	4 bits. genre of value value (see ValueGenre enum).
		// 12-19:	8 bits. ID of command class that created and manages this value.
		// 08-11:	4 bits. Instance index of the command class.
		// 04-07:	4 bits. Index of value within all the value created by the command class instance.
		// 00-03:	4 bits. Type of value (bool, byte, string etc).
		uint32	m_id;

		// Unique PC interface identifier
		uint32  m_homeId;
	};

} // namespace OpenZWave

#endif



