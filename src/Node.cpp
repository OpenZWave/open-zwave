//-----------------------------------------------------------------------------
//
//	Node.cpp
//
//	A node in the Z-Wave network.
//
//	Copyright (c) 2009 Mal Lansell <xpl@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
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

#include "Node.h"
#include "Defs.h"
#include "Group.h"
#include "Manager.h"
#include "Driver.h"
#include "Notification.h"
#include "Msg.h"
#include "Log.h"
#include "Mutex.h"

#include "tinyxml.h"

#include "CommandClasses.h"
#include "CommandClass.h"
#include "Basic.h"
#include "Configuration.h"
#include "ControllerReplication.h"
#include "MultiInstance.h"
#include "WakeUp.h"
#include "NodeNaming.h"
#include "Version.h"

#include "ValueID.h"
#include "Value.h"
#include "ValueBool.h"
#include "ValueByte.h"
#include "ValueDecimal.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueShort.h"
#include "ValueString.h"
#include "ValueStore.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <Node::Node>
// Constructor
//-----------------------------------------------------------------------------
Node::Node
( 
	uint32 const _homeId, 
	uint8 const _nodeId
):
	m_homeId( _homeId ),
	m_nodeId( _nodeId ),
	m_listening( true ),	// assume we start out listening
	m_protocolInfoReceived( false ),
	m_nodeInfoReceived( false ),
	m_values( new ValueStore() )
{
}

//-----------------------------------------------------------------------------
// <Node::~Node>
// Destructor
//-----------------------------------------------------------------------------
Node::~Node
(
)
{
	// Delete the command classes
	map<uint8,CommandClass*>::iterator cit = m_commandClassMap.begin();
	while( !m_commandClassMap.empty() )
	{
		delete cit->second;
		m_commandClassMap.erase( cit );
		cit = m_commandClassMap.begin();
	}

	// Delete the groups
	map<uint8,Group*>::iterator git = m_groups.begin();
	while( !m_groups.empty() )
	{
		delete git->second;
		m_groups.erase( git );
		git = m_groups.begin();
	}

	// Delete the values
	delete m_values;
}

//-----------------------------------------------------------------------------
// <Node::ReadXML>
// Read the node config from XML
//-----------------------------------------------------------------------------
void Node::ReadXML
( 
	TiXmlElement const* _node	
)
{
	char const* str;
	int intVal;

	m_protocolInfoReceived = true;
	m_nodeInfoReceived = true;

	str = _node->Attribute( "name" );
	if( str )
	{
		m_nodeName = str;
	}

	str = _node->Attribute( "location" );
	if( str )
	{
		m_location = str;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "basic", &intVal ) )
	{
		m_basic = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "generic", &intVal ) )
	{
		m_generic = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "specific", &intVal ) )
	{
		m_specific = (uint8)intVal;
	}

	str = _node->Attribute( "type" );
	if( str )
	{
		m_type = str;
	}

	m_listening = true;
	str = _node->Attribute( "listening" );
	if( str )
	{
		m_listening = !strcmp( str, "true" );
	}

	m_routing = true;
	str = _node->Attribute( "routing" );
	if( str )
	{
		m_routing = !strcmp( str, "true" );
	}

	m_maxBaudRate = 0;
	if( TIXML_SUCCESS == _node->QueryIntAttribute( "max_baud_rate", &intVal ) )
	{
		m_maxBaudRate = (uint32)intVal;
	}
	
	m_version = 0;
	if( TIXML_SUCCESS == _node->QueryIntAttribute( "version", &intVal ) )
	{
		m_version = (uint8)intVal;
	}

	m_security = 0;
	str = _node->Attribute( "security" );
	if( str )
	{
		char* p;
		m_security = (uint8)strtol( str, &p, 0 );
	}

	// Notify the watchers of the protocol info
	Notification* notification = new Notification( Notification::Type_NodeProtocolInfo );
	notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
	GetDriver()->QueueNotification( notification );

	// Read the manufacturer info and create the command classes
	TiXmlElement const* child = _node->FirstChildElement();
	while( child )
	{
		str = child->Value();
		if( str )
		{
			if( !strcmp( str, "CommandClasses" ) )
			{
				ReadCommandClassesXML( child );
			}
			else if( !strcmp( str, "Manufacturer" ) )
			{
				str = child->Attribute( "id" );
				if( str )
				{
					m_manufacturerId = str;
				}

				str = child->Attribute( "name" );
				if( str )
				{
					m_manufacturerName = str;
				}

				TiXmlElement const* product = child->FirstChildElement();
				if( !strcmp( product->Value(), "Product" ) )
				{
					str = product->Attribute( "type" );
					if( str )
					{
						m_productType = str;
					}

					str = product->Attribute( "id" );
					if( str )
					{
						m_productId = str;
					}

					str = product->Attribute( "name" );
					if( str )
					{
						m_productName = str;
					}
				}

				// Notify the watchers of the name changes
				notification = new Notification( Notification::Type_NodeNaming );
				notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
				GetDriver()->QueueNotification( notification );
			}
		}

		// Move to the next child node
		child = child->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::ReadCommandClassesXML>
// Read the command classes from XML
//-----------------------------------------------------------------------------
void Node::ReadCommandClassesXML
( 
	TiXmlElement const* _ccsElement
)
{
	char const* str;
	int32 intVal;

	TiXmlElement const* ccElement = _ccsElement->FirstChildElement();
	while( ccElement )
	{
		str = ccElement->Value();
		if( str && !strcmp( ccElement->Value(), "CommandClass" ) )
		{
			if( TIXML_SUCCESS == ccElement->QueryIntAttribute( "id", &intVal ) )
			{
				uint8 id = (uint8)intVal;

				// See if it already exists
				CommandClass* cc = GetCommandClass( id );
				if( NULL == cc )
				{
					// It does not, so we create it
					cc = AddCommandClass( id );
				}

				if( NULL != cc )
				{
					cc->ReadXML( ccElement );
				}
			}
		}

		ccElement = ccElement->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::WriteXML>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void Node::WriteXML
( 
	TiXmlElement* _driverElement
)
{
	if( !(IsListeningDevice() || m_nodeInfoReceived) )
	{
		// This is a sleeping node from which we have never had a list of command classes.
		// We should write nothing out otherwise the list will not be requested in future.
		return;
	}
	
	char str[32];

	TiXmlElement* nodeElement = new TiXmlElement( "Node" );
	_driverElement->LinkEndChild( nodeElement );

	snprintf( str, 32, "%d", m_nodeId );
	nodeElement->SetAttribute( "id", str );

	nodeElement->SetAttribute( "name", m_nodeName.c_str() );
	nodeElement->SetAttribute( "location", m_location.c_str() );

	snprintf( str, 32, "%d", m_basic );
	nodeElement->SetAttribute( "basic", str );

	snprintf( str, 32, "%d", m_generic );
	nodeElement->SetAttribute( "generic", str );

	snprintf( str, 32, "%d", m_specific );
	nodeElement->SetAttribute( "specific", str );

	nodeElement->SetAttribute( "type", m_type.c_str() );

	nodeElement->SetAttribute( "listening", m_listening ? "true" : "false" );
	nodeElement->SetAttribute( "routing", m_routing ? "true" : "false" );
	
	snprintf( str, 32, "%d", m_maxBaudRate );
	nodeElement->SetAttribute( "max_baud_rate", str );

	snprintf( str, 32, "%d", m_version );
	nodeElement->SetAttribute( "version", str );

	snprintf( str, 32, "0x%.2x", m_security );
	nodeElement->SetAttribute( "security", str );

	// Write the manufacturer and product data in the same format
	// as used in the ManyfacturerSpecfic.xml file.  This will 
	// allow new devices to be added via a simple cut and paste.
	TiXmlElement* manufacturerElement = new TiXmlElement( "Manufacturer" );
	nodeElement->LinkEndChild( manufacturerElement );

	manufacturerElement->SetAttribute( "id", m_manufacturerId.c_str() );
	manufacturerElement->SetAttribute( "name", m_manufacturerName.c_str() );

	TiXmlElement* productElement = new TiXmlElement( "Product" );
	manufacturerElement->LinkEndChild( productElement );

	productElement->SetAttribute( "type", m_productType.c_str() );
	productElement->SetAttribute( "id", m_productId.c_str() );
	productElement->SetAttribute( "name", m_productName.c_str() );
	
	// Write the command classes
	TiXmlElement* ccsElement = new TiXmlElement( "CommandClasses" );
	nodeElement->LinkEndChild( ccsElement );

	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		TiXmlElement* ccElement = new TiXmlElement( "CommandClass" );
		ccsElement->LinkEndChild( ccElement );
		it->second->WriteXML( ccElement );
	}
}

//-----------------------------------------------------------------------------
// <Node::UpdateProtocolInfo>
// Handle the FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO response
//-----------------------------------------------------------------------------
void Node::UpdateProtocolInfo
( 
	uint8 const* _data
)
{
	// Remove the protocol info request from the queue
	GetDriver()->RemoveNodeInfoRequest();

	if( m_protocolInfoReceived )
	{
		// We already have this info
		return;
	}
	m_protocolInfoReceived = true;

	// Capabilities
	m_listening = (( _data[0] & 0x80 ) != 0 );
	m_routing = (( _data[0] & 0x40 ) != 0 );
	
	m_maxBaudRate = 9600;
	if( ( _data[0] & 0x38 ) == 0x10 )
	{
		m_maxBaudRate = 40000;
	}

	m_version = ( _data[0] & 0x07 ) + 1;
	
	// Security  
	m_security = _data[1] & 0x7f;
//	m_optional = (( _data[1] & 0x80 ) != 0 );

	// Device types
	m_basic = _data[3];
	m_generic = _data[4];
	m_specific = _data[5];

	switch( m_basic )
	{
		case BasicType_Controller:			{ m_type = "Controller";				break; }
		case BasicType_StaticController:	{ m_type = "Static Controller";			break; }
		case BasicType_Slave:				{ m_type = "Slave";						break; }
		case BasicType_RoutingSlave:		{ m_type = "Routing Slave";				break; }
	}

	switch( m_generic )
	{
		case GenericType_Controller:		{ m_type = "Generic Controller";		break; }
		case GenericType_StaticController:	{ m_type = "Static Controller";			break; }
		case GenericType_AVControlPoint:	{ m_type = "AV Control Point";			break; }
		case GenericType_Display:			{ m_type = "Display";					break; }
		case GenericType_GarageDoor:		{ m_type = "Garage Door";				break; }
		case GenericType_Thermostat:		{ m_type = "Thermostat";				break; }
		case GenericType_WindowCovering:	{ m_type = "Window Covering";			break; }
		case GenericType_RepeaterSlave:		{ m_type = "Repeater Slave";			break; }
		case GenericType_SwitchBinary:		{ m_type = "Binary Switch";				break; }
		case GenericType_SwitchMultiLevel:	{ m_type = "Multi-level Switch";		break; }
		case GenericType_SwitchRemote:		{ m_type = "Remote Switch";				break; }
		case GenericType_SwitchToggle:		{ m_type = "Toggle Switch";				break; }
		case GenericType_SensorBinary:		{ m_type = "Binary Sensor";				break; }
		case GenericType_SensorMultiLevel:	{ m_type = "Multi-level sensor";		break; }
		case GenericType_WaterControl:		{ m_type = "Water Control";				break; }
		case GenericType_MeterPulse:		{ m_type = "Meter Pulse";				break; }
		case GenericType_EntryControl:		{ m_type = "Entry Control";				break; }
		case GenericType_SemiInteroperable:	{ m_type = "Semi-Interoperable";		break; }
		case GenericType_NonInteroperable:	{ m_type = "Non-Interoperable";			break; }
	}

	Log::Write( "Protocol Info for Node %d:", m_nodeId );
	Log::Write( "  Type          = %s", m_type.c_str() );
	Log::Write( "  Listening     = %s", m_listening ? "true" : "false" );
	Log::Write( "  Routing       = %s", m_routing ? "true" : "false" );
	Log::Write( "  Max Baud Rate = %d", m_maxBaudRate );
	Log::Write( "  Version       = %d", m_version );
	Log::Write( "  Security      = 0x%.2x", m_security );

	if( !m_listening )
	{
		// Device does not always listen, so we need the WakeUp handler.  We can't 
		// wait for the command class list because the request for the command
		// classes may need to go in the wakeup queue itself!
		if( CommandClass* pCommandClass = AddCommandClass( WakeUp::StaticGetCommandClassId() ) )
		{
			pCommandClass->SetInstances( 1 );
		}
	}

	// Request the command classes
	Msg* msg = new Msg( "Request Node Info", m_nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_INFO, false, true, FUNC_ID_ZW_APPLICATION_UPDATE );
	msg->Append( m_nodeId );	
	GetDriver()->SendMsg( msg ); 

	// Notify the watchers of the protocol info
	Notification* notification = new Notification( Notification::Type_NodeProtocolInfo );
	notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
	GetDriver()->QueueNotification( notification );

	//// All nodes are assumed to be awake until we fail to get a reply to a message.  
	////
	//// Unfortunately, in the case of FUNC_ID_ZW_REQUEST_NODE_INFO, the PC interface responds with a FAILED
	//// message rather than allowing the request to time out.  This means that we always get a response, even if
	//// the node is actually asleep.  
	////
	//// To get around this, if the node is non-listening, and flagged as awake, we add a copy of the same request
	//// into its wakeup queue, just in case it is not actually awake.
	//if( !IsListeningDevice() )
	//{
	//	if( WakeUp* wakeUp = static_cast<WakeUp*>( GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
	//	{
	//		msg = new Msg( "Request Node Info", m_nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_INFO, false, true, FUNC_ID_ZW_APPLICATION_UPDATE );
	//		msg->Append( m_nodeId );	
	//		msg->Finalize();

	//		Log::Write( "" );
	//		Log::Write( "Queuing Wake-Up Command: %s", msg->GetAsString().c_str() );
	//		wakeUp->QueueMsg( msg );
	//	}
	//}
}

//-----------------------------------------------------------------------------
// <Node::UpdateNodeInfo>
// Set up the command classes from the node info frame
//-----------------------------------------------------------------------------
void Node::UpdateNodeInfo
(
	uint8 const* _data,
	uint8 const _length
)
{
	if( !m_nodeInfoReceived )
	{
		m_nodeInfoReceived = true;

		// Add the command classes specified by the device
		int32 i;
		for( i=0; i<_length; ++i )
		{
			if( _data[i] == 0xef )
			{
				// COMMAND_CLASS_MARK.  
				// Marks the end of the list of supported command classes.  The remaining classes 
				// are those that can be controlled by this device, which we can ignore.
				break;
			}

			if( CommandClass* pCommandClass = AddCommandClass( _data[i] ) )
			{
				// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
				// then some command class instance counts will increase once the responses to the RequestState
				// call at the end of this method have been processed.
				pCommandClass->SetInstances( 1 );
			}
		}

		Log::Write( "Supported Command Classes for Node %d:", m_nodeId );
		for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
		{
			Log::Write( "  %s", it->second->GetCommandClassName().c_str() );
		}

		// Now request the node state.  We do this in separate calls so that
		// the static values are in place when the dynamic stuff comes in.  This is
		// essential for things like thermostat modes, where we need to get the list
		// of supported modes before receiving the current state.  If the node
		// supports the multi-instance command class, we hold off requesting the 
		// non-static state until we receive the instance count for each command class.
		RequestVersions();
		RequestInstances();
		RequestState( CommandClass::RequestFlag_Static );
		if( NULL == GetCommandClass( MultiInstance::StaticGetCommandClassId() ) )
		{
			RequestState( CommandClass::RequestFlag_Session | CommandClass::RequestFlag_Dynamic );
		}
	}
	else
	{
		RequestState( CommandClass::RequestFlag_Dynamic );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeName>
// Set the name of the node
//-----------------------------------------------------------------------------
void Node::SetNodeName
(
	string const& _nodeName
)
{
	m_nodeName = _nodeName;
	if( NodeNaming* cc = static_cast<NodeNaming*>( GetCommandClass( NodeNaming::StaticGetCommandClassId() ) ) )
	{
		// The node supports naming, so we try to write the name into the device
		cc->Set( _nodeName );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetLocation>
// Set the location of the node
//-----------------------------------------------------------------------------
void Node::SetLocation
(
	string const& _location
)
{
	m_location = _location;
}

//-----------------------------------------------------------------------------
// <Node::ApplicationCommandHandler>
// Handle a command class message
//-----------------------------------------------------------------------------
void Node::ApplicationCommandHandler
(
	uint8 const* _data
)
{
	if( CommandClass* pCommandClass = GetCommandClass( _data[5] ) )
	{
		pCommandClass->HandleMsg( &_data[6], _data[4] );
	}
	else
	{
		if( _data[5] == ControllerReplication::StaticGetCommandClassId() )
		{
			// This is a controller replication message, and we do not support it.
			// We have to at least acknowledge the message to avoid locking the sending device.
			Log::Write( "Node(%d)::ApplicationCommandHandler - Default acknowledgement of controller replication data", m_nodeId );

			Msg* msg = new Msg( "Replication Command Complete", m_nodeId, REQUEST, FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE, false );
			GetDriver()->SendMsg( msg );
		}
		else
		{
			Log::Write( "Node(%d)::ApplicationCommandHandler - Unhandled Command Class 0x%.2x", m_nodeId, _data[5] );
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::GetCommandClass>
// Get the specified command class object
//-----------------------------------------------------------------------------
CommandClass* Node::GetCommandClass
(
	uint8 const _commandClassId
)const
{
	map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.find( _commandClassId );
	if( it != m_commandClassMap.end() )
	{
		return it->second;
	}

	// Not found
	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::AddCommandClass>
// Add a command class to the node
//-----------------------------------------------------------------------------
CommandClass* Node::AddCommandClass
( 
	uint8 const _commandClassId
)
{
	if( GetCommandClass( _commandClassId ) )
	{
		// Class and instance have already been added
		return NULL;
	}

	// Create the command class object and add it to our map
	if( CommandClass* pCommandClass = CommandClasses::CreateCommandClass( _commandClassId, m_homeId, m_nodeId ) )
	{
		m_commandClassMap[_commandClassId] = pCommandClass;
		return pCommandClass;
	}
	else
	{
		Log::Write( "Node(%d)::AddCommandClass - Unsupported Command Class 0x%.2x", m_nodeId, _commandClassId );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::RequestInstances>
// Request the instance count for each command class
//-----------------------------------------------------------------------------
void Node::RequestInstances
( 
)const
{
	if( MultiInstance* pMultiInstance = static_cast<MultiInstance*>( GetCommandClass( MultiInstance::StaticGetCommandClassId() ) ) )
	{
		for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
		{
			if( it->second != pMultiInstance )
			{
				pMultiInstance->RequestInstances( it->second );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::RequestVersions>
// Request the version number of each command class
//-----------------------------------------------------------------------------
void Node::RequestVersions
( 
)const
{
	if( Version* version = static_cast<Version*>( GetCommandClass( Version::StaticGetCommandClassId() ) ) )
	{
		for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
		{
			if( it->second != version )
			{
				version->RequestCommandClassVersion( it->second );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::RequestState>
// Request the current state of all dynamic values in the node
//-----------------------------------------------------------------------------
void Node::RequestState
( 
	uint32 const _requestFlags
)
{
	// Request state from all the command classes
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		it->second->RequestState( _requestFlags );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetConfigParam>
// Set a configuration parameter in a device
//-----------------------------------------------------------------------------
bool Node::SetConfigParam
(
	uint8 const _param,
	int32 _value
)
{
	if( Configuration* cc = static_cast<Configuration*>( GetCommandClass( Configuration::StaticGetCommandClassId() ) ) )
	{
		// First try to find an existing value representing the parameter, and set that.
		if( Value* value = cc->GetParam( _param ) )
		{
			switch( value->GetID().GetType() )
			{
				case ValueID::ValueType_Byte:
				{
					ValueByte* valueByte = static_cast<ValueByte*>( value );
					valueByte->Set( (uint8)_value );
					break;
				}
				case ValueID::ValueType_Short:
				{
					ValueShort* valueShort = static_cast<ValueShort*>( value );
					valueShort->Set( (uint16)_value );
					break;
				}
				case ValueID::ValueType_Int:
				{
					ValueInt* valueInt = static_cast<ValueInt*>( value );
					valueInt->Set( _value );
					break;
				}
				case ValueID::ValueType_List:
				{
					ValueList* valueList = static_cast<ValueList*>( value );
					valueList->SetByValue( (int32)_value );
					break;
				}
			}

			return true;
		}

		// Failed to find an existing value object representing this 
		// configuration parameter, so we try to set the value directly 
		// through the Configuration command class.
		cc->Set( _param, _value );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Node::RequestConfigParam>
// Request the value of a configuration parameter from the device
//-----------------------------------------------------------------------------
void Node::RequestConfigParam
(	
	uint8 const _param
)
{
	if( Configuration* cc = static_cast<Configuration*>( GetCommandClass( Configuration::StaticGetCommandClassId() ) ) )
	{
		cc->Get( _param );
	}
}

//-----------------------------------------------------------------------------
// <Node::CreateValueID>
// Helper to create a ValueID
//-----------------------------------------------------------------------------
ValueID Node::CreateValueID
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	ValueID::ValueType const _type
)
{
	return ValueID( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _type );
}

//-----------------------------------------------------------------------------
// <Node::CreateValueBool>
// Helper to create a new bool value and add it to the value store
//-----------------------------------------------------------------------------
ValueBool* Node::CreateValueBool
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _default
)
{
	ValueBool* value = new ValueBool( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueByte>
// Helper to create a new byte value and add it to the value store
//-----------------------------------------------------------------------------
ValueByte* Node::CreateValueByte
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	uint8 const _default
)
{
	ValueByte* value = new ValueByte( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueDecimal>
// Helper to create a new decimal value and add it to the value store
//-----------------------------------------------------------------------------
ValueDecimal* Node::CreateValueDecimal
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	string const& _default
)
{
	ValueDecimal* value = new ValueDecimal( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueInt>
// Helper to create a new int value and add it to the value store
//-----------------------------------------------------------------------------
ValueInt* Node::CreateValueInt
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	int32 const _default
)
{
	ValueInt* value = new ValueInt( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueList>
// Helper to create a new list value and add it to the value store
//-----------------------------------------------------------------------------
ValueList* Node::CreateValueList
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	vector<ValueList::Item> const& _items,
	int32 const _default
)
{
	ValueList* value = new ValueList( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _items, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueShort>
// Helper to create a new short value and add it to the value store
//-----------------------------------------------------------------------------
ValueShort* Node::CreateValueShort
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	uint16 const _default
)
{
	ValueShort* value = new ValueShort( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueString>
// Helper to create a new string value and add it to the value store
//-----------------------------------------------------------------------------
ValueString* Node::CreateValueString
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	string const& _default
)
{
	ValueString* value = new ValueString( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	return value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueFromXML>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
void Node::CreateValueFromXML
( 
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value* value = NULL;

	// Create the value
	ValueID::ValueType type = Value::GetTypeEnumFromName( _valueElement->Attribute( "type" ) );
	switch( type )
	{
		case ValueID::ValueType_Bool:
		{
			value = new ValueBool( m_homeId, m_nodeId, _commandClassId, _valueElement );
			break;
		}
		case ValueID::ValueType_Byte:
		{
			value = new ValueByte( m_homeId, m_nodeId, _commandClassId, _valueElement );
			break;
		}
		case ValueID::ValueType_Decimal:
		{
			value = new ValueDecimal( m_homeId, m_nodeId, _commandClassId, _valueElement );
			break;
		}
		case ValueID::ValueType_Int:
		{
			value = new ValueInt( m_homeId, m_nodeId, _commandClassId, _valueElement );
			break;
		}
		case ValueID::ValueType_List:
		{
			value = new ValueList( m_homeId, m_nodeId, _commandClassId, _valueElement );
			break;
		}
		case ValueID::ValueType_Short:
		{
			value = new ValueShort( m_homeId, m_nodeId, _commandClassId, _valueElement );
			break;
		}
		case ValueID::ValueType_String:
		{
			value = new ValueString( m_homeId, m_nodeId, _commandClassId, _valueElement );
			break;
		}
	}

	if( value )
	{
		ValueStore* store = GetValueStore();
		store->AddValue( value );
		value->Release();
	}
}

//-----------------------------------------------------------------------------
// <Node::GetValue>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
Value* Node::GetValue
(
	ValueID const& _id
)
{
	return GetValueStore()->GetValue( _id );
}

//-----------------------------------------------------------------------------
// <Node::GetValue>
// Get the value object with the specified settings
//-----------------------------------------------------------------------------
Value* Node::GetValue
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	ValueID::ValueType const _type
)
{
	Value* value = NULL;
	ValueStore* store = GetValueStore();
	value = store->GetValue( CreateValueID( _genre, _commandClassId, _instance, _valueIndex, _type ) );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::GetGroup>
// Get a Group from the node's map
//-----------------------------------------------------------------------------
Group* Node::GetGroup
(
	uint8 const _groupIdx
)
{ 
	map<uint8,Group*>::iterator it = m_groups.find( _groupIdx );
	if( it == m_groups.end() )
	{
		Log::Write( "Node(%d)::GetGroup - Invalid Group Index %d", m_nodeId, _groupIdx );
		return NULL;
	}

	return it->second; 
}

//-----------------------------------------------------------------------------
// <Node::AddGroup>
// Add a group into the node's map
//-----------------------------------------------------------------------------
void Node::AddGroup
( 
	Group* _group
)
{ 
	map<uint8,Group*>::iterator it = m_groups.find( _group->GetIdx() );
	if( it != m_groups.end() )
	{
		// There is already a group with this id.  We will replace it.
		delete it->second;
		m_groups.erase( it );
	}

	m_groups[_group->GetIdx()] = _group; 
}

//-----------------------------------------------------------------------------
// <Node::WriteGroups>
// Save the group data
//-----------------------------------------------------------------------------
void Node::WriteGroups
( 
	TiXmlElement* _associationsElement
)
{
	for( map<uint8,Group*>::iterator it = m_groups.begin(); it != m_groups.end(); ++it )
	{
		Group* group = it->second;

		TiXmlElement* groupElement = new TiXmlElement( "Group" );
		_associationsElement->LinkEndChild( groupElement );
		group->WriteXML( groupElement );
	}
}

//-----------------------------------------------------------------------------
// <Node::GetNumGroups>
// Gets the number of association groups reported by this node
//-----------------------------------------------------------------------------
uint8 Node::GetNumGroups
(
)
{
	return m_groups.size();
}

//-----------------------------------------------------------------------------
// <Node::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Node::GetAssociations
(
	uint8 const _groupIdx,
	uint8** o_associations
)
{
	uint32 numAssociations = 0;
	if( Group* group = GetGroup( _groupIdx ) )
	{
		numAssociations = group->GetAssociations( o_associations );	
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <Node::AddAssociation>
// Adds a node to an association group
//-----------------------------------------------------------------------------
void Node::AddAssociation
(
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
}

//-----------------------------------------------------------------------------
// <Node::RemoveAssociation>
// Removes a node from an association group
//-----------------------------------------------------------------------------
void Node::RemoveAssociation
(
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
}


//-----------------------------------------------------------------------------
// <Node::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* Node::GetDriver
(
)const
{
	return( Manager::Get()->GetDriver( m_homeId ) );
}








