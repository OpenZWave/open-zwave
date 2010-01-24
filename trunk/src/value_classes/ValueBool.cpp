//-----------------------------------------------------------------------------
//
//	ValueBool.cpp
//
//	Represents a boolean value
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

#include "tinyxml.h"
#include "ValueBool.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueBool::ValueBool>
// Constructor
//-----------------------------------------------------------------------------
ValueBool::ValueBool
(
	uint8 const _nodeId,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	string const& _label,
	bool const _bReadOnly,
	bool const _bValue
):
	Value( _nodeId, _commandClassId, _instance, _index, _label, _bReadOnly ),
	m_bValue( _bValue )
{
}

//-----------------------------------------------------------------------------
// <ValueBool::ValueBool>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueBool::ValueBool
(
	TiXmlElement* _pValueElement
):
	Value( _pValueElement )
{
	char const* str = _pValueElement->Attribute( "value" );
	if( str )
	{
		m_bValue = !strcmp( str, "True" );
	}
}

//-----------------------------------------------------------------------------
// <ValueBool::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueBool::WriteXML
(
	TiXmlElement* _pValueElement
)
{
	Value::WriteXML( _pValueElement );
	_pValueElement->SetAttribute( "value", m_bValue ? "True" : "False" );
}

//-----------------------------------------------------------------------------
// <ValueBool::GetAsString>
// Convert the value to string form
//-----------------------------------------------------------------------------
string ValueBool::GetAsString
(
)const
{
	return( m_bValue ? "True" : "False" );
}

//-----------------------------------------------------------------------------
// <ValueBool::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueBool::Set
(
	bool const _bValue
)
{
	if( IsReadOnly() )
	{
		return false;
	}

	if( _bValue == m_bValue )
	{
		return true;
	}

	return false;
}



