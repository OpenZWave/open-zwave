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

#include "Group.h"
#include "Driver.h"
#include "Node.h"
#include "Association.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor
//-----------------------------------------------------------------------------
Group::Group
( 
	uint8 const _nodeId,
	uint8 const _groupIdx
):
	m_nodeId( _nodeId ),
	m_groupIdx( _groupIdx )
{
	char str[16];
	snprintf( str, 16, "Group %d", m_groupIdx );
	m_label = str;
}

//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Group::Group
(
	uint8 const _nodeId,
	TiXmlElement* _groupElement
):
	m_nodeId( _nodeId )
{
	int intVal;
	_groupElement->QueryIntAttribute( "index", &intVal );
	m_groupIdx = (uint8)intVal;

	char const* str = _groupElement->Attribute( "label" );
	if( str )
	{
		m_label = str;
	}

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
				pAssociationElement->QueryIntAttribute( "node", &intVal );
				m_associations.insert( (uint8)intVal );
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

	snprintf( str, 16, "%d", m_groupIdx );
	_groupElement->SetAttribute( "index", str );

	_groupElement->SetAttribute( "label", m_label.c_str() );

	for( Iterator it = Begin(); it != End(); ++it )
	{
		TiXmlElement* pAssociationElement = new TiXmlElement( "Association" );
		
		snprintf( str, 16, "%d", *it );
		pAssociationElement->SetAttribute( "node", str );

		_groupElement->LinkEndChild( pAssociationElement );
	}
}

//-----------------------------------------------------------------------------
// <Group::AddNode>
// Associate a node with this group
//-----------------------------------------------------------------------------
void Group::AddNode
(
	uint8 const _nodeId
)
{
	if( Node* node = Driver::Get()->GetNode( m_nodeId ) )
	{
		if( Association* cc = static_cast<Association*>( node->GetCommandClass( Association::StaticGetCommandClassId() ) ) )
		{
			cc->Set( m_groupIdx, _nodeId );
		}
	}
}

//-----------------------------------------------------------------------------
// <Group:RemoveNode>
// Remove a node from this group
//-----------------------------------------------------------------------------
void Group::RemoveNode
(
	uint8 const _nodeId
)
{
	if( Node* node = Driver::Get()->GetNode( m_nodeId ) )
	{
		if( Association* cc = static_cast<Association*>( node->GetCommandClass( Association::StaticGetCommandClassId() ) ) )
		{
			cc->Remove( m_groupIdx, _nodeId );
		}
	}
}

//-----------------------------------------------------------------------------
// <Group::OnGroupChanged>
// Change the group contents and notify the watchers
//-----------------------------------------------------------------------------
void Group::OnGroupChanged
(
	uint8 const _numAssociations,
	uint8 const* _associations
)
{
	bool notify = false;

	// If the number of associations is different, we'll save 
	// ourselves some work and clear the old set now.
	if( _numAssociations != m_associations.size() )
	{
		m_associations.clear();
		notify = true;
	}

	// Add the new associations. 
	uint8 oldSize = (uint8)m_associations.size();

	uint8 i;
	for( i=0; i<_numAssociations; ++i )
	{
		m_associations.insert( _associations[i] );
	}

	if( (!notify) && ( oldSize != m_associations.size() ) )
	{
		// The number of nodes in the original and new groups is the same, but
		// the number of associations has grown. There must be different nodes 
		// in the original and new sets of nodes in the group.  The easiest way
		// to sort this out is to clear the associations and add the new nodes again.
		m_associations.clear();
		for( i=0; i<_numAssociations; ++i )
		{
			m_associations.insert( _associations[i] );
		}
		notify = true;
	}

	if( notify )
	{
		// Send notification that the group contents have changed
		Driver::Notification* notification = new Driver::Notification();
	
		notification->m_type = Driver::NotificationType_Group;
		notification->m_id = ValueID();
		notification->m_nodeId = m_nodeId;
		notification->m_groupIdx = m_groupIdx;

		Driver::Get()->NotifyWatchers( notification ); 
		delete notification;
	}
}




