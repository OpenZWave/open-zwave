//-----------------------------------------------------------------------------
//
//	Manager.h
//
//	Handles Z-Wave interfaces
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

#ifndef _Manager_H
#define _Manager_H

#include <string>
#include <map>
#include <list>
#include <deque>

#include "Defs.h"
#include "ValueID.h"

namespace OpenZWave
{
	class Driver;
	class Node;
	class Msg;
	class Value;
	class Event;
	class Mutex;
	class SerialPort;
	class Thread;
	class ValueBool;
	class ValueByte;
	class ValueDecimal;
	class ValueInt;
	class ValueList;
	class ValueShort;
	class ValueString;

	class Manager
	{
		friend class CommandClass;
		friend class Group;
		friend class Node;
		friend class Value;

	public:
		struct Notification;
		typedef void (*pfnOnNotification_t)( Notification const* _pNotification, void* _context );

	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	public:
		static Manager* Create( string const& _configPath );
		static Manager* Get(){ return s_instance; }
		static void Destroy();

		string const& GetConfigPath()const{ return m_configPath; }

	private:
		Manager( string const& _configPath );
		virtual ~Manager();

		Event*					m_exitEvent;		// Event that will be signalled when the threads should exit
		bool					m_exit;
		string					m_configPath;
		static Manager*			s_instance;

	//-----------------------------------------------------------------------------
	//	Drivers
	//-----------------------------------------------------------------------------
	public:
		bool AddDriver( string const& _serialPortName, uint8* o_driverId );
		bool RemoveDriver( uint8 const _driverId );

		bool IsSlave( uint8 const _driverId );
		bool HasTimerSupport( uint8 const _driverId );
		bool IsPrimaryController( uint8 const _driverId );
		bool IsStaticUpdateController( uint8 const _driverId );

	private:
		enum
		{
			// The maximum number of drivers is limited by the packing of the
			// driver index into the ValueIDs.  Four bits are currently allocated.
			MaxDrivers = 16	
		};

		Driver* GetDriver( uint8 const _driverId );

		Driver*	m_drivers[MaxDrivers];

	//-----------------------------------------------------------------------------
	//	Polling Z-Wave devices
	//-----------------------------------------------------------------------------
	public:
		void SetPollInterval( int32 _seconds );
		bool EnablePoll( ValueID const& _id );
		bool DisablePoll( ValueID const& _id );

	//-----------------------------------------------------------------------------
	//	Retrieving Node information
	//-----------------------------------------------------------------------------
	public:
		ValueBool* GetValueBool( ValueID const& _id );
		ValueByte* GetValueByte( ValueID const& _id );
		ValueDecimal* GetValueDecimal( ValueID const& _id );
		ValueInt* GetValueInt( ValueID const& _id );
		ValueList* GetValueList( ValueID const& _id );
		ValueShort* GetValueShort( ValueID const& _id );
		ValueString* GetValueString( ValueID const& _id );

	//-----------------------------------------------------------------------------
	//	Notifications
	//-----------------------------------------------------------------------------
	public:
		enum NotificationType 
		{
			NotificationType_Value = 0,			// Value changed
			NotificationType_Group,				// Group (associations) changed
			NotificationType_NodeAdded,			// Node has been added
			NotificationType_NodeRemoved,		// Node has been removed
			NotificationType_PollingDisabled,	// Polling of this value has been turned off
			NotificationType_PollingEnabled		// Polling of this value has been turned on
		};

		struct Notification
		{
		public:
			NotificationType	m_type;
			ValueID				m_id;
			uint8				m_groupIdx;
		};

		bool AddWatcher( pfnOnNotification_t watcher, void* _context );
		bool RemoveWatcher( pfnOnNotification_t watcher, void* _context );
		void NotifyWatchers( Notification const* _pNotification );

	private:
		struct Watcher
		{
			pfnOnNotification_t	m_callback;
			void*				m_context;

			Watcher
			(
				pfnOnNotification_t _callback,
				void* _context
			):
				m_callback( _callback ),
				m_context( _context )
			{
			}
		};

		list<Watcher*>	m_watchers;

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	public:	
		void ResetController( uint8 const _driverId );
		void SoftReset( uint8 const _driverId );

		void RequestNodeNeighborUpdate( uint8 const _driverId, uint8 const _nodeId );
		void AssignReturnRoute( uint8 const _driverId, uint8 const _srcNodeId, uint8 const _dstNodeId );
		
		void BeginAddNode( uint8 const _driverId, bool const _bHighpower = false );
		void BeginAddController( uint8 const _driverId, bool const _bHighpower = false );
		void EndAddNode( uint8 const _driverId );
		
		void BeginRemoveNode( uint8 const _driverId );
		void EndRemoveNode( uint8 const _driverId );

		void BeginReplicateController( uint8 const _driverId );
		void EndReplicateController( uint8 const _driverId );

		void ReadMemory( uint8 const _driverId,  uint16 const offset );

		void SetConfiguration( uint8 const _driverId, uint8 const _nodeId, uint8 const _parameter, uint32 const _value );

	};

} // namespace OpenZWave

#endif // _Manager_H

