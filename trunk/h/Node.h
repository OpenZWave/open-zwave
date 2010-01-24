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
	class ValueStore;
	class ValueID;
	class Value;

	class Node
	{
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

		// Construction
		Node( uint8 const _nodeId );
		Node( TiXmlElement* _pNode );
		virtual ~Node();

		void UpdateProtocolInfo( uint8 const* _pData );
		void UpdateNodeInfo( uint8 const* _pData, uint8 const _length );
		void ApplicationCommandHandler( uint8 const* _pData );

		// Command Classes
		CommandClass* GetCommandClass( uint8 const _commandClassId )const;
		void RequestInstances()const;

		void RequestState();
		void RequestStatic();

		// Accessors
		bool IsListeningDevice()const{ return m_bListening; }
		bool IsRoutingDevice()const{ return m_bRouting; }
		uint32 GetMaxBaudRate()const{ return m_maxBaudRate; }
		uint8 GetVersion()const{ return m_version; }
		uint8 GetSecurity()const{ return m_security; }
		uint8 GetNodeId()const{ return m_nodeId; }
		uint8 GetBasic()const{ return m_basic; }
		uint8 GetGeneric()const{ return m_generic; }
		uint8 GetSpecific()const{ return m_specific; }
		string const& GetBasicLabel()const{ return m_basicLabel; }	
		string const& GetGenericLabel()const{ return m_genericLabel; }	
		uint8 GetLevel()const{ return m_level; }
		void SetLevel( uint8 const _level );
		bool IsPolled()const{ return m_bPolled; }
		void SetPolled( bool _bState ){ m_bPolled = _bState; }
		Value* GetValue( ValueID const& _id )const;
		ValueStore* GetValueStore()const{ return m_pValues; }

	private:
		CommandClass* AddCommandClass( uint8 const _commandClassId );
		void SaveStatic( FILE* _pFile );

		map<uint8,CommandClass*>		m_commandClassMap;

		bool							m_bListening;
		bool							m_bRouting;
		uint32							m_maxBaudRate;
		uint8							m_version;
		uint8							m_security;
		uint8							m_nodeId;
		uint8							m_basic;
		uint8							m_generic;
		uint8							m_specific;
		string							m_basicLabel;
		string							m_genericLabel;

		bool							m_bPolled;

		uint8							m_level;

		bool							m_bProtocolInfoReceived;
		bool							m_bNodeInfoReceived;

		ValueStore*						m_pValues;	// Values reported via command classes
	};

} //namespace OpenZWave

#endif //_Node_H

