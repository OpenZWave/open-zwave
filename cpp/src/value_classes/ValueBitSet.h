//-----------------------------------------------------------------------------
//
//	ValueBitSet.h
//
//	Represents a Range of Bits
//
//	Copyright (c) 2017 Justin Hammond <justin@dynam.ac>
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

#ifndef _ValueBitSet_H
#define _ValueBitSet_H

#include <string>
#include "Defs.h"
#include "value_classes/Value.h"
#include "Bitfield.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;
	class CommandClass;

	/** \brief BitSet value sent to/received from a node.
	 * \ingroup ValueID
	 */
	class ValueBitSet: public Value
	{
	public:
		ValueBitSet( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint32 const _value, uint8 const _pollIntensity );
		ValueBitSet(){}
		virtual ~ValueBitSet(){}

		bool Set( uint32 const _value );
		bool SetBit( uint8 const _idx);
		bool ClearBit(uint8 const _idx);
		void OnValueRefreshed( uint32 const _value );

		// From Value
		virtual string const GetAsString() const;
		virtual bool SetFromString( string const& _value );
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		uint32 GetValue() const;
		bool GetBit(uint8 _idx) const;

	private:
		Bitfield	m_value;				// the current index in the m_items vector
		Bitfield	m_valueCheck;			// the previous value (used for double-checking spurious value reads)
		Bitfield	m_newValue;				// a new value to be set on the appropriate device
	};

} // namespace OpenZWave

#endif



