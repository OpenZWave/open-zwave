//-----------------------------------------------------------------------------
//
//	Value.h
//
//	Base class for all OpenZWave Value Classes
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

#ifndef _Value_H
#define _Value_H

#include <string>
#include "Defs.h"
#include "ValueID.h"

class TiXmlElement;

namespace OpenZWave
{
	class Node;

	/** \brief Base class for values associated with a node.
	 */
	class Value
	{
		friend class Driver;
		friend class ValueStore;

	public:
		Value( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, ValueID::ValueType const _type, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, bool const _isset );
		Value();

		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		ValueID const& GetID()const{ return m_id; }
		bool IsReadOnly()const{ return m_readOnly; }
		bool IsWriteOnly()const{ return m_writeOnly; }
		bool IsSet()const{ return m_isSet; }

		string const& GetLabel()const{ return m_label; }
		void SetLabel( string const& _label ){ m_label = _label; }

		string const& GetUnits()const{ return m_units; }
		void SetUnits( string const& _units ){ m_units = _units; }

		string const& GetHelp()const{ return m_help; }
		void SetHelp( string const& _help ){ m_help = _help; }

		int32 GetMin()const{ return m_min; }
		int32 GetMax()const{ return m_max; }

		virtual string const GetAsString() const { return ""; }
		virtual bool SetFromString( string const& _value ) { return false; }

		uint32 Release(){ uint32 t; if( !(t = --m_refs) ){ delete this; } return t; }

		// Helpers
		static ValueID::ValueGenre GetGenreEnumFromName( char const* _name );
		static char const* GetGenreNameFromEnum( ValueID::ValueGenre _genre );
		static ValueID::ValueType GetTypeEnumFromName( char const* _name );
		static char const* GetTypeNameFromEnum( ValueID::ValueType _type );

	protected:
		virtual ~Value();

		bool Set();				// For the user to change a value in a device
		void SetIsSet() { m_isSet = true; }
		void OnValueChanged();	// A value in a device has been changed.

		int32		m_min;
		int32		m_max;

	private:
		uint32 AddRef(){ ++m_refs; return m_refs; }

		uint32		m_refs;
		ValueID		m_id;
		string		m_label;
		string		m_units;
		string		m_help;
		bool		m_readOnly;
		bool		m_writeOnly;
		bool		m_isSet;
		uint8		m_affectsLength;
		uint8*		m_affects;
		bool		m_affectsAll;
	};

} // namespace OpenZWave

#endif



