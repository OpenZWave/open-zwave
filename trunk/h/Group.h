//-----------------------------------------------------------------------------
//
//	Group.h
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

#ifndef _Group_H
#define _Group_H

#include <string>
#include <map>
#include "Defs.h"

namespace OpenZWave
{
	class Group
	{
	public:
		Group( string const& _associations );
		Group( uint8 numAssociations, uint8* _pAssociations );

		~Group(){ delete [] m_pAssociations; }

		uint32 GetNumAssociations()const{ return m_numAssociations; }
		uint8 GetAssociation( uint32 _idx )const{ return ( _idx < m_numAssociations ) ? m_pAssociations[_idx] : 0xff; }

	private:
		uint32	m_numAssociations;
		uint8*	m_pAssociations;
	};

} //namespace OpenZWave

#endif //_Group_H

