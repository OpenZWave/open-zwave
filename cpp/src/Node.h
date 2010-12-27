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
	class ValueSchedule;
	class ValueShort;
	class ValueString;
	class Mutex;

	/** \brief The Node class describes a Z-Wave node object...typically a device on the 
	 *  Z-Wave network.
	 */
	class Node
	{
		friend class Driver;
		friend class Group;
		friend class Association;
		friend class AssociationCommandConfiguration;
		friend class CommandClass;
		friend class ManufacturerSpecific;
		friend class NodeNaming;

	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	public:
		/** Constructor initializes the node object, associating it with a specific
		 *  network (_homeId) and network node (_nodeId).
		 *  @param _homeId The homeId of the network to which this node is connected.
		 *  @param _nodeId The nodeId of this node.
		 */
		Node( uint32 const _homeId, uint8 const _nodeId );
		/** Destructor cleans up memory allocated to node and its child objects. 
		*/
		virtual ~Node();

	private:
		/** Returns a pointer to the driver (interface with a Z-Wave controller) 
		 *  associated with this node.
		*/
		Driver* GetDriver()const;

	//-----------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------
	public:
		enum QueryStage														
		{
			QueryStage_None,					/**< Query process hasn't started for this node */
			QueryStage_ProtocolInfo,			/**< Retrieve protocol information */
			QueryStage_Neighbors,				/**< Retrieve node neighbor list */
			QueryStage_WakeUp,					/**< Wake up */
			QueryStage_NodeInfo,				/**< Retrieve node info ??? */
			QueryStage_ManufacturerSpecific,	/**< Retrieve manufacturer-specific information */
			QueryStage_Versions,				/**< Retrieve software/hardware version information */
			QueryStage_Instances,				/**< Retrieve information about running instances ??? */
			QueryStage_Static,					/**< Retrieve static information (doesn't change) */
			QueryStage_Associations,			/**< Retrieve information about associations */
			QueryStage_Session,					/**< Retrieve session information (changes infrequently) */
			QueryStage_Dynamic,					/**< Retrieve dynamic information (changes frequently) */
			QueryStage_Complete					/**< Query process is completed for this node */
		};

		/**
		 * This function advances the query process (see Remarks below for more detail on the 
		 * process).  It iterates through the various query stages enumerated in Node::QueryStage.
		 * 
		 * @remark
		 * For OpenZWave to discover everything about a node, we have to follow a certain
		 * order of queries, because the results of one stage may affect what is requested
		 * in the next stage.  The stage is saved with the node data, so that any incomplete
		 * queries can be restarted the next time the application runs.
		 * <p>
		 * The individual command classes also store some state information as to whether 
		 * they have had a response to certain queries.  This state information is 
		 * initilized by the SetStaticRequests 
		 * call in QueryStage_None.  It is also saved, so we do not need to request state 
		 * from every command class if some have previously responded. 
		 */
		void AdvanceQueries();
		/** 
		 *  Signal that a specific query stage has been completed for this node.  This will
		 *  only work if the query process for this node is indeed at the specified stage.  
		 *  Otherwise, the function returns with no action.
		 *  @param _stage The current stage of the query process.
		 */
		void QueryStageComplete( QueryStage const _stage );
		/** 
		 *  Retry the specified query stage (up to _maxAttempts retries).  This will
		 *  only work if the query process for this node is indeed at the specified stage.  
		 *  Otherwise, the function returns with no action.
		 *  @param _stage The query stage to retry.
		 *  @param _maxAttempts 
		 */
		void QueryStageRetry( QueryStage const _stage, uint8 const _maxAttempts = 0 );	    // maxAttempts of zero means no limit
		/**
		 * This function sets the query stage for the node (but only to an earlier stage).  If
		 * a later stage is specified, it is ignored.
		 * @param _stage The desired query stage.
		 * @see m_queryStage, m_queryPending
		 */
		void GoBackToQueryStage( QueryStage const _stage );									// Used to move back to repeat from an earlier stage. 

		/**
		 *  This function handles a response to the FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO
		 *  command for this node.  If protocol information has already been retrieved
		 *  for the node, the function simply returns.  Otherwise, it populates several
		 *  member variables about the device at this node:
		 *  - m_listening (whether it is always listening or whether it "sleeps"
		 *  - m_routing (whether it is a routing node (capable of passing commands along to other nodes in the network) or not
		 *  - m_maxBaudRate (the maximum baud rate at which this device can communicate)
		 *  - m_version (TODO)
		 *  - m_security (
		 */
		void UpdateProtocolInfo( uint8 const* _data );
		void UpdateNodeInfo( uint8 const* _data, uint8 const _length );

		bool ProtocolInfoReceived()const{ return (uint32)m_queryStage > (uint32)QueryStage_ProtocolInfo; }
		bool NodeInfoReceived()const{ return( (uint32)m_queryStage > (uint32)QueryStage_NodeInfo ); }
		bool AllQueriesCompleted()const{ return( QueryStage_Complete == m_queryStage ); }

	private:
		void SetStaticRequests();

		QueryStage	m_queryStage;
		bool		m_queryPending;
		uint8		m_queryRetries;

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
		uint8		m_basic;		//*< Basic device class (0x01-Controller, 0x02-Static Controller, 0x03-Slave, 0x04-Routing Slave
		uint8		m_generic;
		uint8		m_specific;
		string		m_type;				// Label representing the specific/generic/basic value
		uint8		m_neighbors[29];	// Bitmask containing the neighbouring nodes

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
		/**
		 * This function retrieves a pointer to the requested command class object.
		 * @param _commandClassId Class ID (a single byte value) identifying the command class requested.
		 * @return Pointer to the requested CommandClass object.
		 * @see CommandClass, m_commandClassMap
		 */
		CommandClass* GetCommandClass( uint8 const _commandClassId )const;
		void ApplicationCommandHandler( uint8 const* _data );

		bool RequiresPolling();
		void Poll();

	private:
		/**
		 * Creates the specified command class object and adds it to the node (via the 
		 * m_commandClassMap) if it doesn't exist.
		 * No new object is created if it already exists for this node.
		 * @param _commandClassId Class ID (a single byte value) identifying the command class requested.
		 * @return Pointer to the CommandClass object just added to the map (NULL if the object 
		 * was already there or if the CommandClass object creation failed).
		 * @see CommandClass, CommandClasses::CreateCommandClass, m_commandClassMap
		 */
		CommandClass* AddCommandClass( uint8 const _commandClassId );
		/**
		 * Removes a command class object from the node (via the m_commandClassMap).  Before removing the 
		 * object, this function also removes any values stored in the object's ValueStore.
		 * @param _commandClassId Class ID (a single byte value) identifying the command class to be removed.
		 * @see m_commandClassMap, ValueStore, GetValueStore, ValueStore::RemoveCommandClassValues
		 */
		void RemoveCommandClass( uint8 const _commandClassId );
		void ReadXML( TiXmlElement const* _nodeElement );
		void ReadCommandClassesXML( TiXmlElement const* _ccsElement );
		void WriteXML( TiXmlElement* _nodeElement );

		map<uint8,CommandClass*>		m_commandClassMap;	/**< Map of command class ids and pointers to associated command class objects */

	//-----------------------------------------------------------------------------
	// Basic commands (helpers that go through the basic command class)
	//-----------------------------------------------------------------------------
	public:
		void SetLevel( uint8 const _level );

	//-----------------------------------------------------------------------------
    // On/Off commands (helpers that go through the basic or switchall command class)
    //-----------------------------------------------------------------------------
    public:
        void SetNodeOn();
        void SetNodeOff();

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
		ValueSchedule* CreateValueSchedule( ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _valueIndex, string const& _label, string const& _units, bool const _readOnly );
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
		void RequestAllConfigParams();

	//-----------------------------------------------------------------------------
	// Groups
	//-----------------------------------------------------------------------------
	private:		
		// The public interface is provided via the wrappers in the Manager class
		uint8 GetNumGroups();
		uint32 GetAssociations( uint8 const _groupIdx, uint8** o_associations );
		uint8 GetMaxAssociations( uint8 const _groupIdx );
		void AddAssociation( uint8 const _groupIdx, uint8 const _targetNodeId );
		void RemoveAssociation( uint8 const _groupIdx, uint8 const _targetNodeId );
		void AutoAssociate();

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

