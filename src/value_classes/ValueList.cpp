//-----------------------------------------------------------------------------
//
//	ValueList.cpp
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
#include "ValueList.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueList::ValueList>
// Constructor
//-----------------------------------------------------------------------------
ValueList::ValueList
(
	uint8 const _nodeId,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	string const& _label,
	bool const _bReadOnly,
	vector<string> const& _items,
	string const& _value
):
	Value( _nodeId, _commandClassId, _instance, _index, _label, _bReadOnly ),
	m_items( _items ),
	m_value( _value )
{
}

//-----------------------------------------------------------------------------
// <ValueList::ValueList>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueList::ValueList
(
	TiXmlElement* _pValueElement
):
	Value( _pValueElement )
{
	// Read the items
	TiXmlNode const* pItemNode = _pValueElement->FirstChild();
	while( pItemNode )
	{
		TiXmlElement const* pItemElement = pItemNode->ToElement();
		if( pItemElement )
		{
			char const* str = pItemElement->Value();
			if( str && !strcmp( str, "Item" ) )
			{
				char const* itemStr = pItemElement->Attribute( "value" );
				if( itemStr )
				{
					m_items.push_back( itemStr );
				}
			}
		}

		pItemNode = pItemNode->NextSibling();
	}

	// Set the value
	char const* valueStr = _pValueElement->Attribute( "value" );
	if( valueStr )
	{
		m_value = valueStr;
	}
}

//-----------------------------------------------------------------------------
// <ValueList::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueList::WriteXML
(
	TiXmlElement* _pValueElement
)
{
	Value::WriteXML( _pValueElement );
	_pValueElement->SetAttribute( "value", m_value.c_str() );

	for( vector<string>::iterator it = m_items.begin(); it != m_items.end(); ++it )
	{
		TiXmlElement* pItemElement = new TiXmlElement( "Item" );
		pItemElement->SetAttribute( "value", (*it).c_str() );
		_pValueElement->LinkEndChild( pItemElement );
	}
}

//-----------------------------------------------------------------------------
// <ValueList::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueList::Set
(
	string const& _value
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

	// Ensure the value is one of the options
	bool bFound = false;
	for( vector<string>::iterator it = m_items.begin(); it != m_items.end(); ++it )
	{
		if( _value == (*it) )
		{
			bFound = true;
			break;
		}
	}

	if( bFound )
	{
		// Set the value
		return true;
	}

	return false;
}



