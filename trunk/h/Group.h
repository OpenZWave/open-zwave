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
#include <set>
#include "Defs.h"

class TiXmlElement;

namespace OpenZWave
{
	class Group
	{
	public:
		typedef set<uint8>::const_iterator Iterator;

		Iterator Begin(){ return m_associations.begin(); }
		Iterator End(){ return m_associations.end(); }

		Group( uint8 const _nodeId, uint8 const _groupIdx );
		Group( uint8 const _nodeId, TiXmlElement* _valueElement );
		~Group(){}

		void WriteXML( TiXmlElement* _valueElement );
		string const& GetLabel()const{ return m_label; }

		void AddNode( uint8 const _nodeId );
		void RemoveNode( uint8 const _nodeId );

		void OnGroupChanged( uint8 const _numAssociations, uint8 const* _associations );

	private:
		string		m_label;
		uint8		m_nodeId;
		uint8		m_groupIdx;
		set<uint8>	m_associations;
	};

} //namespace OpenZWave

#endif //_Group_H

