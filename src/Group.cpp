//-----------------------------------------------------------------------------
//
//	Group.cpp
//
//	A set of associations in a Z-Wave device.
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

#include "Group.h"

using namespace OpenZWave;


////-----------------------------------------------------------------------------
//// <Group::Group>
//// Constructor
////-----------------------------------------------------------------------------
//Group::Group
//( 
//	string const& _associations 
//):
//	m_numAssociations( 0 ),
//	m_pAssociations( NULL )
//{
//	uint32 i;
//
//	// Convert the string containing a comma separated list of node Ids into an array
//	if( _associations.size() == 0 )
//	{
//		return;
//	}
//	
//	// Count the commas and allocate space for the node Ids
//	m_numAssociations = 1;
//	for( i=0; i<_associations.size(); ++i )
//	{
//		if( _associations[i] == ',' )
//		{
//			++m_numAssociations;
//		}
//	}
//
//	m_pAssociations = new uint8[m_numAssociations];
//
//	// Extract the node Ids
//	int32 start = 0;
//	int32 idx = 0;
//	for( i=0; i<=_associations.size(); ++i )
//	{
//		uint8 ch = _associations[i];
//		if( ( 0 == ch ) || ( ',' == ch ) )
//		{
//			string idStr = _associations.substr( start, i-start );
//			m_pAssociations[idx++] = (uint8)atoi( idStr.c_str() );
//			start = i+1;
//		}
//	}
//}


//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor
//-----------------------------------------------------------------------------
Group::Group
( 
	uint8 _groupId,
	uint8 _numAssociations, 
	uint8* _pAssociations
):
	m_groupId( _groupId ),
	m_numAssociations( _numAssociations )
{
	char str[16];
	snprintf( str, 16, "Group %d", m_groupId );
	m_label = str;

	m_pAssociations = new uint8[m_numAssociations];
	memcpy( m_pAssociations, _pAssociations, m_numAssociations );
}

//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Group::Group
(
	TiXmlElement* _groupElement
):
	m_pAssociations( NULL )
{
	int intVal;
	_groupElement->QueryIntAttribute( "id", &intVal );
	m_groupId = (uint8)intVal;

	char const* str = _groupElement->Attribute( "label" );
	if( str )
	{
		m_label = str;
	}

	_groupElement->QueryIntAttribute( "num_associations", &intVal );
	m_numAssociations = (uint8)intVal;
	
	if( m_numAssociations )
	{
		m_pAssociations = new uint8[m_numAssociations];

		// Read the associations for this group
		TiXmlNode const* pAssociationNode = _groupElement->FirstChild();
		while( pAssociationNode )
		{
			TiXmlElement const* pAssociationElement = pAssociationNode->ToElement();
			if( pAssociationElement )
			{
				char const* elementName = pAssociationElement->Value();
				if( elementName && !strcmp( elementName, "Association" ) )
				{
					int32 idx = 0;
					pAssociationElement->QueryIntAttribute( "index", &idx );
					pAssociationElement->QueryIntAttribute( "node", &intVal );
					m_pAssociations[idx] = (uint8)intVal;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Group::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Group::WriteXML
(
	TiXmlElement* _groupElement
)
{
	char str[16];

	snprintf( str, 16, "%d", m_groupId );
	_groupElement->SetAttribute( "id", str );

	_groupElement->SetAttribute( "label", m_label.c_str() );

	snprintf( str, 16, "%d", m_numAssociations );
	_groupElement->SetAttribute( "num_associations", str );

	for( uint8 i=0; i<m_numAssociations; ++i )
	{
		TiXmlElement* pAssociationElement = new TiXmlElement( "Association" );
		
		snprintf( str, 16, "%d", i );
		pAssociationElement->SetAttribute( "index", str );

		snprintf( str, 16, "%d", m_pAssociations[i] );
		pAssociationElement->SetAttribute( "node", str );

		_groupElement->LinkEndChild( pAssociationElement );
	}
}




