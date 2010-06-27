//-----------------------------------------------------------------------------
//
//      zwave.h
//
//      Cli/C++ wrapper for the C++ OpenZWave Manager class
//
//      Copyright (c) 2010 Amer Harb <harb_amer@hotmail.com>
//
//      SOFTWARE NOTICE AND LICENSE
//
//      This file is part of OpenZWave.
//
//      OpenZWave is free software: you can redistribute it and/or modify
//      it under the terms of the GNU Lesser General Public License as published
//      by the Free Software Foundation, either version 3 of the License,
//      or (at your option) any later version.
//
//      OpenZWave is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public License
//      along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------


#pragma once
#include "Windows.h"
#include "ValueID.h"
#include "stdio.h"

#include <msclr/auto_gcroot.h>
#include <msclr/lock.h>

using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace OpenZWave;
using namespace Runtime::InteropServices;

namespace OpenZWaveDotNet 
{
	public ref class ZWValueID
	{
	public:
		enum class ValueGenre
		{
			All		= ValueID::ValueGenre_All,
			User	= ValueID::ValueGenre_User,	
			Config	= ValueID::ValueGenre_Config,	
			System	= ValueID::ValueGenre_System
		};

		enum class ValueType
		{
			Bool	= ValueID::ValueType_Bool,
			Byte	= ValueID::ValueType_Byte,
			Decimal	= ValueID::ValueType_Decimal,
			Int		= ValueID::ValueType_Int,
			List	= ValueID::ValueType_List,
			Short	= ValueID::ValueType_Short,
			String	= ValueID::ValueType_String
		};

		ZWValueID( ValueID const& valueId ){ m_valueId = new ValueID( valueId ); }

		ValueID CreateUnmanagedValueID(){ return ValueID( *m_valueId ); }

		uint32		GetHomeId()			{ return m_valueId->GetHomeId(); }
		uint32		GetNodeId()			{ return m_valueId->GetNodeId(); }
		ValueGenre	GetGenre()			{ return (ValueGenre)Enum::ToObject( ValueGenre::typeid, m_valueId->GetGenre() ); }
		uint32		GetCommandClassId()	{ return m_valueId->GetCommandClassId(); }
		uint32		GetInstance()		{ return m_valueId->GetInstance(); }
		uint32		GetIndex()			{ return m_valueId->GetIndex(); }
		ValueType	GetType()			{ return (ValueType)Enum::ToObject( ValueType::typeid, m_valueId->GetType() ); }
		
		// Comparison Operators
		bool operator ==	( ZWValueID^ _other ){ return( (*m_valueId) == (*_other->m_valueId) ); }
		bool operator !=	( ZWValueID^ _other ){ return( (*m_valueId) != (*_other->m_valueId) ); }
		bool operator <		( ZWValueID^ _other ){ return( (*m_valueId) < (*_other->m_valueId) ); }
		bool operator >		( ZWValueID^ _other ){ return( (*m_valueId) > (*_other->m_valueId) ); }

	internal:
		ValueID* m_valueId;
	};
}