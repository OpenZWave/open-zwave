//-----------------------------------------------------------------------------
//
//      zwave.h
//
//      Cli/C++ wrapper for the C++ OpenZWave Manager class
//
//      Copyright (c) 2010 Amer Harb <harb_amer@hotmail.com>
//
//      SOFTWARE NOTICE AND LICENSE
//
//      This file is part of OpenZWave.
//
//      OpenZWave is free software: you can redistribute it and/or modify
//      it under the terms of the GNU Lesser General Public License as published
//      by the Free Software Foundation, either version 3 of the License,
//      or (at your option) any later version.
//
//      OpenZWave is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public License
//      along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------


#pragma once
#include "Windows.h"
#include "Manager.h"
#include "Value.h"
#include "ValueStore.h"
#include "ValueID.h"
#include "ValueBool.h"
#include "ValueInt.h"
#include "ValueByte.h"
#include "ValueString.h"
#include "ValueShort.h"
#include "ValueDecimal.h"
#include "Notification.h"
#include "stdio.h"
#include "stdafx.h"

#include <msclr/auto_gcroot.h>
#include <msclr/lock.h>

using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace OpenZWave;
using namespace Runtime::InteropServices;

namespace OpenZWaveDotNET {

	public ref class ZWValueID{

	public:		
		static uint8 ValueGenre_All = 0;
		static uint8 ValueGenre_User = 1;		// Basic values an ordinary user would be interested in;
		static uint8 ValueGenre_Config = 2;			// Device-specific configuration parameters
		static uint8 ValueGenre_System = 3;			// Values of significance only to users who understand the Z-Wave protocol 
		static uint8 ValueGenre_Count= 4;

		static uint8 ValueType_Bool = 0;
		static uint8 ValueType_Byte=1;
		static uint8 ValueType_Decimal=2;
		static uint8 ValueType_Int=3;
		static uint8 ValueType_List=4;
		static uint8 ValueType_Short=5;
		static uint8 ValueType_String=6;
		static uint8 ValueType_Count=7;

		ZWValueID( uint32 homeId,uint8 uniqueId):m_homeId( homeId ){ m_id = uniqueId; }
		ZWValueID():m_homeId(0),m_id(0){}
		
		void CloneFrom(ZWValueID^ v2){m_id = v2->m_id; m_homeId = v2->m_homeId;}

		ValueID CreateUnmanagedValueID(){return ValueID(GetHomeId(),GetNodeId(),(ValueID::ValueGenre)GetGenre(),GetCommandClassId(),GetInstance(),GetIndex(),(ValueID::ValueType)GetType());}

		uint32		GetHomeId()			{ return m_homeId; }
		uint32		GetUniqueId()		{ return m_id;}
		uint8		GetNodeId()			{ return( (uint8)		( (m_id & 0xff000000) >> 24 ) ); }
		uint8		GetGenre()			{ return( (uint8)	( (m_id & 0x00f00000) >> 20 ) ); }
		uint8		GetCommandClassId()	{ return( (uint8)		( (m_id & 0x000ff000) >> 12 ) ); }
		uint8		GetInstance()		{ return( (uint8)		( (m_id & 0x00000f00) >> 8  ) ); }
		uint8		GetIndex()			{ return( (uint8)		( (m_id & 0x000000f0) >> 4  ) ); }
		uint8		GetType()			{ return( (uint8)	(  m_id & 0x0000000f        ) ); }
		
		// Comparison Operators
		bool operator ==	( ZWValueID^ _other ){ return( (m_homeId == _other->m_homeId) && (m_id == _other->m_id) ); }
		bool operator !=	( ZWValueID^ _other ){ return( (m_homeId != _other->m_homeId) || (m_id != _other->m_id) ); }
		bool operator <		( ZWValueID^ _other ){ return( (m_homeId == _other->m_homeId) ? (m_id < _other->m_id) : (m_homeId < _other->m_homeId) ); }
		bool operator >		( ZWValueID^ _other ){ return( (m_homeId == _other->m_homeId) ? (m_id > _other->m_id) : (m_homeId > _other->m_homeId) ); }

	internal:
		ZWValueID( 	uint32 _homeId,	uint8 _nodeId,	uint8 _genre,	
			uint8 _commandClassId,	uint8 _instance, uint8 _valueIndex,
			uint8 _type	):
			m_homeId( _homeId )
		{
			assert( ((uint32)_genre) < 16 );
			assert( _instance < 16 );
			assert( _valueIndex < 16 );
			assert( ((uint32)_type) < 16 );

			m_id = (((uint32)_nodeId)<<24)
				 | (((uint32)_genre)<<20)
				 | (((uint32)_commandClassId)<<12)
				 | (((uint32)_instance)<<8)
				 | (((uint32)_valueIndex)<<4)
				 | ((uint32)_type);
		}

		uint32	m_id;
		uint32  m_homeId;
	};


	public ref class ZWNotification {

	public:

		static uint8 Type_ValueAdded = 0;	// Value Added
		static uint8 Type_ValueRemoved = 1;		// Value Removed
		static uint8 Type_ValueChanged=2;		// Value Changed
		static uint8 Type_Group=3;				// Group (associations) changed
		static uint8 Type_NodeAdded=4;			// Node has been added
		static uint8 Type_NodeRemoved=5;		// Node has been removed
		static uint8 Type_NodeStatus=6;		// Node status has changed (usually triggered by receiving a basic_set command from the node)
		static uint8 Type_PollingDisabled=7;	// Polling of this node has been turned off
		static uint8 Type_PollingEnabled=8;	// Polling of this node has been turned on
		static uint8 Type_DriverReady=9;

		uint8 GetType()	{ return m_type; }
		uint32 GetHomeId()	{ return m_valueId->GetHomeId(); }
		uint8 GetNodeId()	{ return m_valueId->GetNodeId(); }
		ZWValueID^ GetValueID()	{ return m_valueId; }
		uint8 GetGroupIdx()	{ assert(Type_Group==m_type); return m_byte; } 
		uint8 GetStatus()	{ assert(Type_NodeStatus==m_type); return m_byte; } 

	internal:
		void SetHomeAndNodeIds( uint32 _homeId, uint8 _nodeId )	{ m_valueId = gcnew ZWValueID( _homeId, _nodeId ); }
		void SetValueId( ZWValueID^ _valueId )	{ m_valueId = _valueId; }
		void SetGroupIdx( uint8 _groupIdx )	{ assert(Type_Group==m_type); m_byte = _groupIdx; }
		void SetStatus( uint8 _status )	{ assert(Type_NodeStatus==m_type); m_byte = _status; }

		uint8	m_type;
		ZWValueID^ m_valueId;
		uint8	m_byte;
	};

	
	public ref class ZWValue{
	public:
		ZWValue(ZWValueID^ vid){m_valueId = vid;}
		String^ GetAsString(){return m_valueString;}
		bool IsReadOnly(){return m_readOnly;}
		String^ GetLabel(){return m_label;}
		String^ GetUnits(){return m_units;}
		ZWValueID^ GetValueId(){return m_valueId;}
	internal:
		ZWValueID^ m_valueId;
		String^ m_label;
		String^ m_units;
		String^ m_valueString;
		bool m_readOnly;
	};
	

	public delegate void ManagedWatchersHandler(ZWNotification^ notification);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	private delegate  void OnNotificationFromUnmanagedDelegate(Notification* _notification,void* _context);

	public ref class ZWManager{

	
	private:
		GCHandle gch;
		OnNotificationFromUnmanagedDelegate^ OnNot;
		Object^ criticalSection;

	public:

		ZWManager(){criticalSection = gcnew Object();}
		//forward notification to managed delegates hooked via Event addhandler 
		void  OnNotificationFromUnmanaged(Notification* _notification,void* _context);

		void Create(String^ configPath,String^userPath);

		void Destroy(){	Manager::Get()->Destroy();}

		String^ GetConfigPath()	{return gcnew String(Manager::Get()->GetConfigPath().c_str()); }
		String^ GetUserPath()	{return gcnew String(Manager::Get()->GetUserPath().c_str()); }
		void WriteConfig(uint32 homeId)	{Manager::Get()->WriteConfig(homeId);}
		
		bool AddDriver( String^ serialPortName )	{return Manager::Get()->AddDriver((const char*)(Marshal::StringToHGlobalAnsi(serialPortName)).ToPointer());}
		bool RemoveDriver( String^ serialPortName )	{return Manager::Get()->RemoveDriver((const char*)(Marshal::StringToHGlobalAnsi(serialPortName)).ToPointer());}
		
		bool IsSlave( uint32 homeId )	{return Manager::Get()->IsSlave(homeId);}
		bool HasTimerSupport( uint32 homeId )	{return Manager::Get()->HasTimerSupport(homeId);}
		bool IsPrimaryController( uint32 homeId )	{return Manager::Get()->IsPrimaryController(homeId);}
		bool IsStaticUpdateController( uint32 homeId )	{return Manager::Get()->IsStaticUpdateController(homeId);}

		void SetPollInterval( int32 seconds )	{Manager::Get()->SetPollInterval(seconds);}
		bool EnablePoll( uint32 homeId, uint8 nodeId )	{return Manager::Get()->EnablePoll(homeId,nodeId);}
		bool DisablePoll( uint32 homeId, uint8 nodeId ) {return Manager::Get()->DisablePoll(homeId,nodeId);}

		bool RefreshNodeInfo( uint32 homeId, uint8 nodeId )	{return Manager::Get()->RefreshNodeInfo(homeId,nodeId);}
		void RequestState( uint32 homeId, uint8 nodeId )	{Manager::Get()->RequestState(homeId,nodeId);}
		String^ GetBasicLabel( uint32 homeId, uint8 nodeId )	{return gcnew String(Manager::Get()->GetBasicLabel(homeId,nodeId).c_str()) ;}
		String^ GetGenericLabel( uint32 homeId, uint8 nodeId )	{return gcnew String(Manager::Get()->GetGenericLabel(homeId,nodeId).c_str()) ;}

		ZWValue^ GetValue( ZWValueID^ vid );

		bool SetValue(ZWValueID^ vid, String^ newvalue);
	//	bool SetValue(int32 newvalue);
	//	bool SetValue(uint8 newvalue);
	//	bool SetValue(String^ newvalue);
	//	bool SetValue(uint16 newvalue);

		void ResetController( uint32 homeId )	{Manager::Get()->ResetController(homeId);}
		void SoftReset( uint32 homeId )		{Manager::Get()->SoftReset(homeId);}

		void RequestNodeNeighborUpdate( uint32 homeId, uint8 nodeId )	{Manager::Get()->RequestNodeNeighborUpdate(homeId,nodeId);}
		void AssignReturnRoute( uint32 homeId, uint8 srcNodeId, uint8 dstNodeId )	{Manager::Get()->AssignReturnRoute(homeId,srcNodeId,dstNodeId);}
		
		void BeginAddNode( uint32 homeId, bool bHighpower)	{Manager::Get()->BeginAddNode(homeId,bHighpower);}
		void BeginAddController( uint32 homeId, bool bHighpower)	{Manager::Get()->BeginAddController(homeId,bHighpower);}
		void BeginAddNode( uint32 homeId)	{Manager::Get()->BeginAddNode(homeId);}
		void BeginAddController( uint32 homeId)	{Manager::Get()->BeginAddController(homeId);}

		void EndAddNode( uint32 homeId )	{Manager::Get()->EndAddNode(homeId);}
		
		void BeginRemoveNode( uint32 homeId)	{Manager::Get()->BeginRemoveNode(homeId);}
		void EndRemoveNode( uint32 homeId)	{Manager::Get()->EndRemoveNode(homeId);}

		void BeginReplicateController( uint32 homeId )	{Manager::Get()->BeginReplicateController(homeId);}
		void EndReplicateController( uint32 homeId )	{Manager::Get()->EndReplicateController(homeId);}

		void RequestNetworkUpdate( uint32 homeId )	{Manager::Get()->RequestNetworkUpdate(homeId);}
		void ControllerChange( uint32 homeId )	{Manager::Get()->ControllerChange(homeId);}

		//
		//void ReadMemory( uint32 homeId,  uint16 offset)	{Manager::Get()->ReadMemory(homeId,offset);}
		//void SetConfiguration( uint32 homeId, uint8 nodeId, uint8 parameter, uint32 value )	{Manager::Get()->SetConfiguration(homeId,nodeId,parameter,value);}
	public: //events

		ManagedWatchersHandler^  _Event;
		event ManagedWatchersHandler^ OnZWNotification{
			void add(ManagedWatchersHandler ^ d) {_Event += d;} 
			void remove(ManagedWatchersHandler ^ d) {	_Event -= d; } 
			void raise(ZWNotification^ notification) { 
				ManagedWatchersHandler^ tmp = _Event; 
				if (tmp) { tmp->Invoke(notification); } 
			} 
		} 

	};


	
	
}