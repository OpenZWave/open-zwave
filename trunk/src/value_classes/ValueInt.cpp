//-----------------------------------------------------------------------------
//
//	ValueInt.cpp
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
#include "ValueInt.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueInt::ValueInt>
// Constructor
//-----------------------------------------------------------------------------
ValueInt::ValueInt
(
	uint8 const _nodeId,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	string const& _label,
	bool const _bReadOnly,
	int32 const _value
):
	Value( _nodeId, _commandClassId, _instance, _index, _label, _bReadOnly ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueInt::ValueInt>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueInt::ValueInt
(
	TiXmlElement* _pValueElement
):
	Value( _pValueElement )
{
	int intVal;
	if( TIXML_SUCCESS == _pValueElement->QueryIntAttribute( "value", &intVal ) )
	{
		m_value = (int32)intVal;
	}
}

//-----------------------------------------------------------------------------
// <ValueInt::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueInt::WriteXML
(
	TiXmlElement* _pValueElement
)
{
	Value::WriteXML( _pValueElement );

	char str[16];
	snprintf( str, sizeof(str), "%d", m_value );
	_pValueElement->SetAttribute( "value", str );
}

//-----------------------------------------------------------------------------
// <ValueInt::GetAsString>
// Convert the value to string form
//-----------------------------------------------------------------------------
string ValueInt::GetAsString
(
)const
{
	char str[16];
	snprintf( str, 16, "%d", m_value );
	return( string( str ) );
}

//-----------------------------------------------------------------------------
// <ValueInt::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueInt::Set
(
	int32 const _value
)
{
	if( IsReadOnly() )
	{
		return false;
	}

	if( _value == m_value )
	{
		return true;
	}

	return false;
}



