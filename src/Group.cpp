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


//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor
//-----------------------------------------------------------------------------
Group::Group
( 
	string const& _associations 
):
	m_numAssociations( 0 ),
	m_pAssociations( NULL )
{
	uint32 i;

	// Convert the string containing a comma separated list of node Ids into an array
	if( _associations.size() == 0 )
	{
		return;
	}
	
	// Count the commas and allocate space for the node Ids
	m_numAssociations = 1;
	for( i=0; i<_associations.size(); ++i )
	{
		if( _associations[i] == ',' )
		{
			++m_numAssociations;
		}
	}

	m_pAssociations = new uint8[m_numAssociations];

	// Extract the node Ids
	int32 start = 0;
	int32 idx = 0;
	for( i=0; i<=_associations.size(); ++i )
	{
		uint8 ch = _associations[i];
		if( ( 0 == ch ) || ( ',' == ch ) )
		{
			string idStr = _associations.substr( start, i-start );
			m_pAssociations[idx++] = (uint8)atoi( idStr.c_str() );
			start = i+1;
		}
	}
}


//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor
//-----------------------------------------------------------------------------
Group::Group
( 
	uint8 _numAssociations, 
	uint8* _pAssociations
):
	m_numAssociations( _numAssociations )
{
	m_pAssociations = new uint8[m_numAssociations];
	memcpy( m_pAssociations, _pAssociations, m_numAssociations );
}
