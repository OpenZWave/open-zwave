//-----------------------------------------------------------------------------
//
//	Node.h
//
//	A node in the Z-Wave network
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

#ifndef _Node_H
#define _Node_H

#include <string>
#include <map>
#include "Defs.h"

class TiXmlElement;

namespace OpenZWave
{
	class CommandClass;
	class Group;
	class ValueStore;
	class ValueID;
	class Value;
	class Mutex;

	class Node
	{
	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	public:
		Node( uint8 const _nodeId );
		Node( TiXmlElement* _node );
		virtual ~Node();

	//-----------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------
	public:
		void UpdateProtocolInfo( uint8 const* _data );
		void UpdateNodeInfo( uint8 const* _data, uint8 const _length );

	private:
		bool	m_protocolInfoReceived;
		bool	m_nodeInfoReceived;


	//-----------------------------------------------------------------------------
	// Capabilities
	//-----------------------------------------------------------------------------
	public:
		// Security flags
		enum
		{
			SecurityFlag_Security				= 0x01,
			SecurityFlag_Controller				= 0x02,
			SecurityFlag_SpecificDevice			= 0x04,
			SecurityFlag_RoutingSlave			= 0x08,
			SecurityFlag_BeamCapability			= 0x10,
			SecurityFlag_Sensor250ms			= 0x20,
			SecurityFlag_Sensor1000ms			= 0x40,
			SecurityFlag_OptionalFunctionality	= 0x80
		};

		// Basic Types
		enum
		{
			BasicType_Unknown					= 0x00,
			BasicType_Controller				= 0x01,
			BasicType_StaticController			= 0x02,
			BasicType_Slave						= 0x03,
			BasicType_RoutingSlave				= 0x04
		};

		// Generic Types
		enum
		{
			GenericType_Unknown					= 0x00,
			GenericType_Controller				= 0x01,
			GenericType_StaticController		= 0x02,
			GenericType_AVControlPoint			= 0x03,
			GenericType_Display					= 0x06,
			GenericType_GarageDoor				= 0x07,
			GenericType_Thermostat				= 0x08,
			GenericType_WindowCovering			= 0x09,
			GenericType_RepeaterSlave			= 0x0f,
			GenericType_SwitchBinary			= 0x10,
			GenericType_SwitchMultiLevel		= 0x11,
			GenericType_SwitchRemote			= 0x12,
			GenericType_SwitchToggle			= 0x13,
			GenericType_SensorBinary			= 0x20,
			GenericType_SensorMultiLevel		= 0x21,
			GenericType_WaterControl			= 0x22,
			GenericType_MeterPulse				= 0x30,
			GenericType_EntryControl			= 0x40,
			GenericType_SemiInteroperable		= 0x50,
			GenericType_NonInteroperable		= 0xff
		};

		// Node Ids
		enum
		{
			NodeBroadcast = 0xff
		};

		bool IsListeningDevice()const{ return m_listening; }
		bool IsRoutingDevice()const{ return m_routing; }
		uint32 GetMaxBaudRate()const{ return m_maxBaudRate; }
		uint8 GetVersion()const{ return m_version; }
		uint8 GetSecurity()const{ return m_security; }
		
		uint8 GetNodeId()const{ return m_nodeId; }
		
		uint8 GetBasic()const{ return m_basic; }
		uint8 GetGeneric()const{ return m_generic; }
		uint8 GetSpecific()const{ return m_specific; }
		string const& GetBasicLabel()const{ return m_basicLabel; }	
		string const& GetGenericLabel()const{ return m_genericLabel; }	
		
		bool IsPolled()const{ return m_polled; }
		void SetPolled( bool _state ){ m_polled = _state; }

	private:
		bool		m_listening;
		bool		m_routing;
		uint32		m_maxBaudRate;
		uint8		m_version;
		uint8		m_security;
		uint8		m_nodeId;
		uint8		m_basic;
		uint8		m_generic;
		uint8		m_specific;
		string		m_basicLabel;
		string		m_genericLabel;
		bool		m_polled;


	//-----------------------------------------------------------------------------
	// Command Classes
	//-----------------------------------------------------------------------------
	public:
		CommandClass* GetCommandClass( uint8 const _commandClassId )const;
		void RequestInstances()const;
		void RequestState();
		void RequestStatic();
		void ApplicationCommandHandler( uint8 const* _data );

	private:
		CommandClass* AddCommandClass( uint8 const _commandClassId );
		void SaveStatic( FILE* _file );

		map<uint8,CommandClass*>		m_commandClassMap;


	//-----------------------------------------------------------------------------
	// Values (handled by the command classes)
	//-----------------------------------------------------------------------------
	public:
		ValueStore* GetValueStore();
		void ReleaseValueStore();
		Value* GetValue( ValueID const& _id )const;

	private:
		ValueStore*	m_values;			// Values reported via command classes
		Mutex*		m_valuesMutex;		// Serialize access to the store


	//-----------------------------------------------------------------------------
	// Configuration Parameters (handled by the Configuration command class)
	//-----------------------------------------------------------------------------
	public:
		bool SetConfigParam( uint8 const _param, int32 _value );
		void RequestConfigParam( uint8 const _param );


	//-----------------------------------------------------------------------------
	// Groups (handled by the Association command class)
	//-----------------------------------------------------------------------------
	public:
		Group* GetGroup( uint8 const _groupIdx );
		uint8 GetNumGroups()const{ return m_numGroups; }
		void SetNumGroups( uint8 const _numGroups );

	private:
		Group** m_groups;
		uint8   m_numGroups;
	};

} //namespace OpenZWave

#endif //_Node_H

