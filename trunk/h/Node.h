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

#ifndef _Node_H
#define _Node_H

#include <string>
#include <vector>
#include <map>
#include "Defs.h"
#include "ValueID.h"
#include "ValueList.h"

class TiXmlElement;

namespace OpenZWave
{
	class CommandClass;
	class Driver;
	class Group;
	class ValueStore;
	class Value;
	class ValueBool;
	class ValueButton;
	class ValueByte;
	class ValueDecimal;
	class ValueInt;
	class ValueShort;
	class ValueString;
	class Mutex;

	class Node
	{
		friend class Driver;
		friend class Association;
		friend class CommandClass;
		friend class ManufacturerSpecific;
		friend class NodeNaming;

	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	public:
		Node( uint32 const _homeId, uint8 const _nodeId );
		virtual ~Node();

	private:
		Driver* GetDriver()const;

	//-----------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------
	public:
		void UpdateProtocolInfo( uint8 const* _data );
		void UpdateNodeInfo( uint8 const* _data, uint8 const _length );
		bool NodeInfoReceived()const{ return m_nodeInfoReceived; }
		void SetStaticRequests();

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
		string const& GetType()const{ return m_type; }	

	private:
		bool		m_listening;
		bool		m_routing;
		uint32		m_maxBaudRate;
		uint8		m_version;
		uint8		m_security;
		uint32		m_homeId;
		uint8		m_nodeId;
		uint8		m_basic;
		uint8		m_generic;
		uint8		m_specific;
		string		m_type;			// Label representing the specific/generic/basic value

	//-----------------------------------------------------------------------------
	// Device Naming
	//-----------------------------------------------------------------------------
	private:
		// Manufacturer, Product and Name are stored here so they can be set by the
		// user even if the device does not support the relevant command classes.
		string GetManufacturerName()const{ return m_manufacturerName; }	
		string GetProductName()const{ return m_productName; }	
		string GetNodeName()const{ return m_nodeName; }	
		string GetLocation()const{ return m_location; }	

		string GetManufacturerId()const{ return m_manufacturerId; }	
		string GetProductType()const{ return m_productType; }	
		string GetProductId()const{ return m_productId; }	

		void SetManufacturerName( string const& _manufacturerName ){ m_manufacturerName = _manufacturerName; }
		void SetProductName( string const& _productName ){ m_productName = _productName; }
		void SetNodeName( string const& _nodeName );
		void SetLocation( string const& _location );

		void SetManufacturerId( string const& _manufacturerId ){ m_manufacturerId = _manufacturerId; }
		void SetProductType( string const& _productType ){ m_productType = _productType; }
		void SetProductId( string const& _productId ){ m_productId = _productId; }
		
		string		m_manufacturerName;
		string		m_productName;
		string		m_nodeName;
		string		m_location;

		string		m_manufacturerId;
		string		m_productType;
		string		m_productId;

	//-----------------------------------------------------------------------------
	// Command Classes
	//-----------------------------------------------------------------------------
	public:
		CommandClass* GetCommandClass( uint8 const _commandClassId )const;
		
		void RequestEntireNodeState();
		void RequestInstances()const;
		void RequestVersions()const;
		void RequestState( uint32 const _requestFlags );
		void ApplicationCommandHandler( uint8 const* _data );

		bool RequiresPolling();
		void Poll();

	private:
		CommandClass* AddCommandClass( uint8 const _commandClassId );
		void ReadXML( TiXmlElement const* _nodeElement );
		void ReadCommandClassesXML( TiXmlElement const* _ccsElement );
		void WriteXML( TiXmlElement* _nodeElement );

		map<uint8,CommandClass*>		m_commandClassMap;

	//-----------------------------------------------------------------------------
	// Basic commands (helpers that go through the basic command class)
	//-----------------------------------------------------------------------------
	public:
		void SetLevel( uint8 const _level );

	//-----------------------------------------------------------------------------
	// Values (handled by the command classes)
	//-----------------------------------------------------------------------------
	public:
		ValueID CreateValueID( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, ValueID::ValueType const _type );

		Value* GetValue( ValueID const& _id );
		Value* GetValue( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, ValueID::ValueType const _type );

		// Helpers for creating values
		ValueBool* CreateValueBool( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _default );
		ValueButton* CreateValueButton( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label );
		ValueByte* CreateValueByte( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, uint8 const _default );
		ValueDecimal* CreateValueDecimal( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, string const& _default );
		ValueInt* CreateValueInt( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, int32 const _default );
		ValueList* CreateValueList( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, vector<ValueList::Item> const& _items, int32 const _default );
		ValueShort* CreateValueShort( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, int16 const _default );
		ValueString* CreateValueString( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, string const& _default );

		void ReadValueFromXML( uint8 const _commandClassId, TiXmlElement const* _valueElement );
		Value* CreateValueFromXML( uint8 const _commandClassId, TiXmlElement const* _valueElement );

	private:
		ValueStore* GetValueStore()const{ return m_values; }

		ValueStore*	m_values;			// Values reported via command classes

	//-----------------------------------------------------------------------------
	// Configuration Parameters (handled by the Configuration command class)
	//-----------------------------------------------------------------------------
	private:
		bool SetConfigParam( uint8 const _param, int32 _value );
		void RequestConfigParam( uint8 const _param );

	//-----------------------------------------------------------------------------
	// Groups
	//-----------------------------------------------------------------------------
	private:		
		// The public interface is provided via the wrappers in the Manager class
		uint8 GetNumGroups();
		uint32 GetAssociations( uint8 const _groupIdx, uint8** o_associations );
		void AddAssociation( uint8 const _groupIdx, uint8 const _targetNodeId );
		void RemoveAssociation( uint8 const _groupIdx, uint8 const _targetNodeId );

		// The following methods are not exposed
		Group* GetGroup( uint8 const _groupIdx );							// Get a pointer to a Group object.  This must only be called while holding the node Lock.
		void AddGroup( Group* _group );										// The groups are fixed properties of a device, so there is no need for a matching RemoveGroup.
		void WriteGroups( TiXmlElement* _associationsElement );				// Write the group data out to XNL

		map<uint8,Group*> m_groups;											// Maps group indices to Group objects.

	//-----------------------------------------------------------------------------
	// Device Classes (static data read from the device_classes.xml file)
	//-----------------------------------------------------------------------------
	private:
		// Container for device class info
		class DeviceClass
		{
		public:
			DeviceClass( TiXmlElement const* _el );
			~DeviceClass(){ delete [] m_mandatoryCommandClasses; }

			uint8 const*	GetMandatoryCommandClasses(){ return m_mandatoryCommandClasses; }
			uint8			GetBasicMapping(){ return m_basicMapping; }
			string const&	GetLabel(){ return m_label; }

		private:
			uint8*			m_mandatoryCommandClasses;						// Zero terminated array of mandatory command classes for this device type.
			uint8			m_basicMapping;									// Command class that COMMAND_CLASS_BASIC maps on to, or zero if there is no mapping.
			string			m_label;										// Descriptive label for the device.
		};

		// Container for generic device class info
		class GenericDeviceClass : public DeviceClass
		{
		public:
			GenericDeviceClass( TiXmlElement const* _el );
			~GenericDeviceClass();

			DeviceClass* GetSpecificDeviceClass( uint8 const& _specific );

		private:
			map<uint8,DeviceClass*>	m_specificDeviceClasses;
		};


		bool SetDeviceClasses( uint8 const _basic, uint8 const _generic, uint8 const _specific );	// Set the device class data for the node
		bool AddMandatoryCommandClasses( uint8 const* _commandClasses );							// Add mandatory command classes as specified in the device_classes.xml to the node.
		void ReadDeviceClasses();																	// Read the static device class data from the device_classes.xml file

		static bool								s_deviceClassesLoaded;		// True if the xml file has alreayd been loaded
		static map<uint8,string>				s_basicDeviceClasses;		// Map of basic device classes.
		static map<uint8,GenericDeviceClass*>	s_genericDeviceClasses;		// Map of generic device classes.
	};

} //namespace OpenZWave

#endif //_Node_H

