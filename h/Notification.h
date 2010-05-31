//-----------------------------------------------------------------------------
//
//	Notification.h
//
//	Contains details of a Z-Wave event reported to the user
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

#ifndef _Notification_H
#define _Notification_H

#include "Defs.h"
#include "ValueID.h"

namespace OpenZWave
{
	class Notification
	{
		friend class Manager;
		friend class Driver;
		friend class Group;
		friend class Value;
		friend class ValueStore;

	public:
		enum NotificationType 
		{
			Type_ValueAdded = 0,	// Value Added
			Type_ValueRemoved,		// Value Removed
			Type_ValueChanged,		// Value Changed
			Type_Group,				// Group (associations) changed
			Type_NodeAdded,			// Node has been added
			Type_NodeRemoved,		// Node has been removed
			Type_NodeStatus,		// Node status has changed (usually triggered by receiving a basic_set command from the node)
			Type_PollingDisabled,	// Polling of this node has been turned off
			Type_PollingEnabled,	// Polling of this node has been turned on
			Type_DriverReady,		// Driver has been added and is ready to use
			Type_DriverReset		// All nodes and values for this driver have been removed.
		};

		NotificationType GetType()const{ return m_type; }
		uint32 GetHomeId()const{ return m_valueId.GetHomeId(); }
		uint8 GetNodeId()const{ return m_valueId.GetNodeId(); }
		ValueID const& GetValueID()const{ return m_valueId; }
		uint8 GetGroupIdx()const{ assert(Type_Group==m_type); return m_byte; } 
		uint8 GetStatus()const{ assert(Type_NodeStatus==m_type); return m_byte; } 

	private:
		Notification( NotificationType _type ): m_type( _type ), m_byte(0){}
		~Notification(){}

		void SetHomeAndNodeIds( uint32 const _homeId, uint8 const _nodeId ){ m_valueId = ValueID( _homeId, _nodeId ); }
		void SetValueId( ValueID const& _valueId ){ m_valueId = _valueId; }
		void SetGroupIdx( uint8 const _groupIdx ){ assert(Type_Group==m_type); m_byte = _groupIdx; }
		void SetStatus( uint8 const _status ){ assert(Type_NodeStatus==m_type); m_byte = _status; }

		NotificationType	m_type;
		ValueID				m_valueId;
		uint8				m_byte;
	};

} //namespace OpenZWave

#endif //_Notification_H

