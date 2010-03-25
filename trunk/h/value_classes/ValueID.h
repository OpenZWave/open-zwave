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
			ValueGenre_Config,		// Device-specific configuration parameters
			ValueGenre_System			// Values of significance only to users who understand the Z-Wave protocol 
			// No more genres can be added without changing the packing of m_id
		};

		enum ValueType
		{
			ValueType_Bool = 0,
			ValueType_Byte,
			ValueType_Decimal,
			ValueType_Int,
			ValueType_List,
			ValueType_Short,
			ValueType_String
			// Type value cannot exceed 7 without changing the packing of m_id
		};

		// Constructor
		ValueID
		( 
			uint8 const _driverId,
			uint8 const	_nodeId,
			ValueGenre const _genre,
			uint8 const _commandClassId,
			uint8 const _instance,
			uint8 const _valueIndex,
			ValueType const _type
		)
		{
			assert( _driverId < 16 );
			assert( ((uint32)_genre) < 4 );
			assert( _instance < 16 );
			assert( _valueIndex < 8 );
			assert( ((uint32)_type) < 8 );

			m_id = (((uint32)_driverId)<<28)
				 | (((uint32)_nodeId)<<20) 
				 | (((uint32)_genre)<<18)
				 | (((uint32)_commandClassId)<<10)
				 | (((uint32)_instance)<<6)
				 | (((uint32)_valueIndex)<<3)
				 | ((uint32)_type);
		}
		
		// Default constructor
		ValueID():m_id(0){}

		// Accessors
		uint8		GetDriverId()const			{ return( (uint8)		( (m_id & 0xf0000000) >> 28 ) ); }
		uint8		GetNodeId()const			{ return( (uint8)		( (m_id & 0x0ff00000) >> 20 ) ); }
		ValueGenre	GetGenre()const				{ return( (ValueGenre)	( (m_id & 0x000c0000) >> 18 ) ); }
		uint8		GetCommandClassId()const	{ return( (uint8)		( (m_id & 0x0003fc00) >> 10 ) ); }
		uint8		GetInstance()const			{ return( (uint8)		( (m_id & 0x000003c0) >> 6  ) ); }
		uint8		GetIndex()const				{ return( (uint8)		( (m_id & 0x00000038) >> 3  ) ); }
		ValueType	GetType()const				{ return( (ValueType)	(  m_id & 0x00000007        ) ); }

		// Comparison Operators
		bool operator ==	( ValueID const& _other )const{ return( m_id == _other.m_id ); }
		bool operator !=	( ValueID const& _other )const{ return( m_id != _other.m_id ); }
		bool operator <		( ValueID const& _other )const{ return( m_id <  _other.m_id ); }
		bool operator >		( ValueID const& _other )const{ return( m_id >  _other.m_id ); }

		static const ValueID	g_nullValueID;
	private:
		// ID Packing:
		// Bits
		// 28-31:	4 bits. Driver index (i.e. which PC interface handles this value)
		// 20-27:	8 bits. Node ID of device
		// 18-19:	2 bits. genre of value value (see ValueGenre enum).
		// 10-17:	8 bits. ID of command class that created and manages this value.
		// 06-09:	4 bits. Instance index of the command class.
		// 03-05:	3 bits. Index of value within all the value created by the command class instance.
		// 00-02:	3 bits. Type of value (bool, byte, string etc).
		uint32	m_id;
	};

} // namespace OpenZWave

#endif



