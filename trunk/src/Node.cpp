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
#include "Msg.h"
#include "Log.h"
#include "Mutex.h"

#include "tinyxml.h"

#include "CommandClasses.h"
#include "CommandClass.h"
#include "Basic.h"
#include "Configuration.h"
#include "MultiInstance.h"
#include "WakeUp.h"

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
	uint8 const _driverId, 
	uint8 const _nodeId
):
	m_driverId( _driverId ),
	m_nodeId( _nodeId ),
	m_protocolInfoReceived( false ),
	m_nodeInfoReceived( false ),
	m_values( new ValueStore() ),
	m_valuesMutex( new Mutex() ),
	m_numGroups( 0 ),
	m_groups( NULL )
{
	// Request the node protocol info
	GetDriver()->AddInfoRequest( m_nodeId );
}

//-----------------------------------------------------------------------------
// <Node::Node>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Node::Node
( 
	uint8 const _driverId, 
	TiXmlElement* _node	
):
	m_driverId( _driverId ),
	m_protocolInfoReceived( true ),
	m_nodeInfoReceived( true ),
	m_values( new ValueStore() ),
	m_valuesMutex( new Mutex() ),
	m_numGroups( 0 ),
	m_groups( NULL )
{
	char const* str;
	int intVal;

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "id", &intVal ) )
	{
		m_nodeId = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "basic", &intVal ) )
	{
		m_basic = (uint8)intVal;
	}

	str = _node->Attribute( "basic_label" );
	if( str )
	{
		m_basicLabel = str;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "generic", &intVal ) )
	{
		m_generic = (uint8)intVal;
	}

	str = _node->Attribute( "generic_label" );
	if( str )
	{
		m_genericLabel = str;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "specific", &intVal ) )
	{
		m_specific = (uint8)intVal;
	}

	str = _node->Attribute( "listening" );
	if( str )
	{
		m_listening = !strcmp( str, "True" );
	}

	// Create the command classes
	TiXmlNode const* pCommandClassNode = _node->FirstChild();
	while( pCommandClassNode )
	{
		TiXmlElement const* pCommandClassElement = pCommandClassNode->ToElement();
		if( pCommandClassElement )
		{
			str = pCommandClassElement->Value();
			if( str && !strcmp( str, "CommandClass" ) )
			{
				if( TIXML_SUCCESS == pCommandClassElement->QueryIntAttribute( "id", &intVal ) )
				{
					uint8 id = (uint8)intVal;

					if( CommandClass* pCommandClass = AddCommandClass( id ) )
					{
						if( TIXML_SUCCESS == pCommandClassElement->QueryIntAttribute( "version", &intVal ) )
						{
							pCommandClass->SetVersion( (uint8)intVal );
						}

						if( TIXML_SUCCESS == pCommandClassElement->QueryIntAttribute( "instances", &intVal ) )
						{
							pCommandClass->SetInstances( (uint8)intVal );
						}
								
						// Load the static data for this command class, so we don't 
						// need to query the device for it.
						pCommandClass->LoadStatic( pCommandClassElement );
					}
				}
			}
		}

		// Move to the next command class
		pCommandClassNode = pCommandClassNode->NextSibling();
	}

	// Get the current values from the node.
	RequestState();
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
	map<uint8,CommandClass*>::iterator it = m_commandClassMap.begin();
	while( !m_commandClassMap.empty() )
	{
		delete it->second;
		m_commandClassMap.erase( it );
		it = m_commandClassMap.begin();
	}

	delete m_values;
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
	if( m_protocolInfoReceived )
	{
		// We already have this info
		return;
	}
	m_protocolInfoReceived = true;

	// Remove the protocol info request from the queue
	GetDriver()->RemoveInfoRequest();

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
	m_security = _data[1];

	// Device types
	m_basic = _data[3];
	m_generic = _data[4];
	m_specific = _data[5];

	switch( m_basic )
	{
		case BasicType_Controller:			{ m_basicLabel = "Controller";				break; }
		case BasicType_StaticController:	{ m_basicLabel = "Static Controller";		break; }
		case BasicType_Slave:				{ m_basicLabel = "Slave";					break; }
		case BasicType_RoutingSlave:		{ m_basicLabel = "Routing Slave";			break; }
	}

	switch( m_generic )
	{
		case GenericType_Controller:		{ m_genericLabel = "Generic Controller";	break; }
		case GenericType_StaticController:	{ m_genericLabel = "Static Controller";		break; }
		case GenericType_AVControlPoint:	{ m_genericLabel = "AV Control Point";		break; }
		case GenericType_Display:			{ m_genericLabel = "Display";				break; }
		case GenericType_GarageDoor:		{ m_genericLabel = "Garage Door";			break; }
		case GenericType_Thermostat:		{ m_genericLabel = "Thermostat";			break; }
		case GenericType_WindowCovering:	{ m_genericLabel = "Window Covering";		break; }
		case GenericType_RepeaterSlave:		{ m_genericLabel = "Repeater Slave";		break; }
		case GenericType_SwitchBinary:		{ m_genericLabel = "Binary Switch";			break; }
		case GenericType_SwitchMultiLevel:	{ m_genericLabel = "Multi-level Switch";	break; }
		case GenericType_SwitchRemote:		{ m_genericLabel = "Remote Switch";			break; }
		case GenericType_SwitchToggle:		{ m_genericLabel = "Toggle Switch";			break; }
		case GenericType_SensorBinary:		{ m_genericLabel = "Binary Sensor";			break; }
		case GenericType_SensorMultiLevel:	{ m_genericLabel = "Multi-level sensor";	break; }
		case GenericType_WaterControl:		{ m_genericLabel = "Water Control";			break; }
		case GenericType_MeterPulse:		{ m_genericLabel = "Meter Pulse";			break; }
		case GenericType_EntryControl:		{ m_genericLabel = "Entry Control";			break; }
		case GenericType_SemiInteroperable:	{ m_genericLabel = "Semi-Interoperable";	break; }
		case GenericType_NonInteroperable:	{ m_genericLabel = "Non-Interoperable";		break; }
	}

	Log::Write( "Protocol Info for Node %d:", m_nodeId );
	Log::Write( "  Listening	 = %s", m_listening ? "True" : "False" );
	Log::Write( "  Routing	   = %s", m_routing ? "True" : "False" );
	Log::Write( "  Max Baud Rate = %d", m_maxBaudRate );
	Log::Write( "  Version	   = %d", m_version );
	Log::Write( "  Security	  = 0x%.2x", m_security );
	Log::Write( "  Basic Type	= %s", m_basicLabel.c_str() );
	Log::Write( "  Generic Type  = %s", m_genericLabel.c_str() );

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

	// All nodes are assumed to be awake until we fail to get a reply to a message.  
	//
	// Unfortunately, in the case of FUNC_ID_ZW_REQUEST_NODE_INFO, the PC interface responds with a FAILED
	// message rather than allowing the request to time out.  This means that we always get a response, even if
	// the node is actually asleep.  
	//
	// To get around this, if the node is non-listening, and flagged as awake, we add a copy of the same request
	// into its wakeup queue, just in case it is not actually awake.
	if( !IsListeningDevice() )
	{
		if( WakeUp* wakeUp = static_cast<WakeUp*>( GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
		{
			msg = new Msg( "Request Node Info", m_nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_INFO, false, true, FUNC_ID_ZW_APPLICATION_UPDATE );
			msg->Append( m_nodeId );	
			msg->Finalize();

			Log::Write( "" );
			Log::Write( "Queuing Wake-Up Command: %s", msg->GetAsString().c_str() );
			wakeUp->QueueMsg( msg );
		}
	}
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
				// then some command class instance counts will increase once the responses to the RequestStatic
				// call at the end of this method have been processed.
				pCommandClass->SetInstances( 1 );
			}
		}

		Log::Write( "Supported Command Classes for Node %d:", m_nodeId );
		for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
		{
			Log::Write( "  %s", it->second->GetCommandClassName().c_str() );
		}

		// Get the static configuration from the node.
		// The dynamic state data will have been requested during the SetInstances call above.
		RequestStatic();
	}
	else
	{
	RequestState();
	}
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
		Log::Write( "Node(%d)::ApplicationCommandHandler - Unhandled Command Class 0x%.2x", m_nodeId, _data[5] );
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
	if( CommandClass* pCommandClass = CommandClasses::CreateCommandClass( _commandClassId, m_driverId, m_nodeId ) )
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
// <Node::RequiresPolling>
// Returns whether this node should be polled for value state
//-----------------------------------------------------------------------------
bool Node::RequiresPolling
( 
)
{
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		if( it->second->RequiresPolling() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Node::Poll>
// Request the current state of dynamic values marked for polling
//-----------------------------------------------------------------------------
void Node::Poll
( 
)
{
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		it->second->Poll();
	}
}

//-----------------------------------------------------------------------------
// <Node::RequestState>
// Request the current state of all dynamic values in the node
//-----------------------------------------------------------------------------
void Node::RequestState
( 
)
{
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		// We pass zero to RequestState to get the state of all instances
		it->second->RequestState( 0 );
	}
}

//-----------------------------------------------------------------------------
// <Node::RequestStatic>
// Request the static node configuration data
//-----------------------------------------------------------------------------
void Node::RequestStatic
( 
)
{
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		it->second->RequestStatic();
	}
}

//-----------------------------------------------------------------------------
// <Node::SaveStatic>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void Node::SaveStatic
( 
	FILE* _file	
)
{
	fprintf( _file, "  <Node id=\"%d\" listening=\"%s\" basic=\"%d\" basic_label=\"%s\" generic=\"%d\" generic_label=\"%s\" specific=\"%d\">\n", m_nodeId, m_listening ? "True" : "False", m_basic, m_basicLabel.c_str(), m_generic, m_genericLabel.c_str(), m_specific );

	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		it->second->SaveStatic( _file );
	}

	fprintf( _file, "</Node>\n" );
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
	// See if there is a value already created for this parameter.  If there is not, we will
	// send the command directly via the configuration command class.  If the parameter exists
	// in the device, the response from the device will cause the creation of a value object for future use.
	if( ValueByte* valueByte = GetValueByte( ValueID::ValueGenre_Config, Configuration::StaticGetCommandClassId(), 0, _param ) )
	{
		valueByte->Set( (uint8)_value );
		return true;
	}

	if( ValueShort* valueShort = GetValueShort( ValueID::ValueGenre_Config, Configuration::StaticGetCommandClassId(), 0, _param ) )
	{
		valueShort->Set( (uint16)_value );
		return true;
	}

	if( ValueInt* valueInt = GetValueInt( ValueID::ValueGenre_Config, Configuration::StaticGetCommandClassId(), 0, _param ) )
	{
		valueInt->Set( (int32)_value );
		return true;
	}

	if( ValueList* valueList = GetValueList( ValueID::ValueGenre_Config, Configuration::StaticGetCommandClassId(), 0, _param ) )
	{
		valueList->SetByValue( (int32)_value );
		return true;
	}

	// Failed to find an existing value object representing this 
	// configuration parameter, so we try to set the value directly 
	// through the Configuration command class.
	if( Configuration* cc = static_cast<Configuration*>( GetCommandClass( Configuration::StaticGetCommandClassId() ) ) )
	{
		cc->Set( _param, _value );
		return true;
	}

	return false;
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
	return ValueID( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _type );
}

//-----------------------------------------------------------------------------
// <Node::CreateValueBool>
// Helper to create a new bool value and add it to the value store
//-----------------------------------------------------------------------------
void Node::CreateValueBool
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
	Value* value = new ValueBool( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	ReleaseValueStore();
}

//-----------------------------------------------------------------------------
// <Node::CreateValueByte>
// Helper to create a new byte value and add it to the value store
//-----------------------------------------------------------------------------
void Node::CreateValueByte
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
	Value* value = new ValueByte( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	ReleaseValueStore();
}

//-----------------------------------------------------------------------------
// <Node::CreateValueDecimal>
// Helper to create a new decimal value and add it to the value store
//-----------------------------------------------------------------------------
void Node::CreateValueDecimal
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
	Value* value = new ValueDecimal( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	ReleaseValueStore();
}

//-----------------------------------------------------------------------------
// <Node::CreateValueInt>
// Helper to create a new int value and add it to the value store
//-----------------------------------------------------------------------------
void Node::CreateValueInt
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
	Value* value = new ValueInt( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	ReleaseValueStore();
}

//-----------------------------------------------------------------------------
// <Node::CreateValueList>
// Helper to create a new list value and add it to the value store
//-----------------------------------------------------------------------------
void Node::CreateValueList
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
	Value* value = new ValueList( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _items, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	ReleaseValueStore();
}

//-----------------------------------------------------------------------------
// <Node::CreateValueShort>
// Helper to create a new short value and add it to the value store
//-----------------------------------------------------------------------------
void Node::CreateValueShort
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
	Value* value = new ValueShort( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	ReleaseValueStore();
}

//-----------------------------------------------------------------------------
// <Node::CreateValueString>
// Helper to create a new string value and add it to the value store
//-----------------------------------------------------------------------------
void Node::CreateValueString
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
	Value* value = new ValueString( m_driverId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _default );
	ValueStore* store = GetValueStore();
	store->AddValue( value );
	value->Release();
	ReleaseValueStore();
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
	Value* value = NULL;
	ValueStore* store = GetValueStore();
	value = store->GetValue( _id );
	ReleaseValueStore();
	return value;
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
	ReleaseValueStore();
	return value;
}

//-----------------------------------------------------------------------------
// <Node::GetValueBool>
// Get the value object as a ValueBool
//-----------------------------------------------------------------------------
ValueBool* Node::GetValueBool
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	return( static_cast<ValueBool*>( GetValue( _genre, _commandClassId, _instance, _valueIndex, ValueID::ValueType_Bool ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueByte>
// Get the value object as a ValueByte
//-----------------------------------------------------------------------------
ValueByte* Node::GetValueByte
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	return( static_cast<ValueByte*>( GetValue( _genre, _commandClassId, _instance, _valueIndex, ValueID::ValueType_Byte ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueDecimal>
// Get the value object as a ValueDecimal
//-----------------------------------------------------------------------------
ValueDecimal* Node::GetValueDecimal
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	return( static_cast<ValueDecimal*>( GetValue( _genre, _commandClassId, _instance, _valueIndex, ValueID::ValueType_Decimal ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueInt>
// Get the value object as a ValueInt
//-----------------------------------------------------------------------------
ValueInt* Node::GetValueInt
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	return( static_cast<ValueInt*>( GetValue( _genre, _commandClassId, _instance, _valueIndex, ValueID::ValueType_Int ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueList>
// Get the value object as a ValueList
//-----------------------------------------------------------------------------
ValueList* Node::GetValueList
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	return( static_cast<ValueList*>( GetValue( _genre, _commandClassId, _instance, _valueIndex, ValueID::ValueType_List ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueShort>
// Get the value object as a ValueShort
//-----------------------------------------------------------------------------
ValueShort* Node::GetValueShort
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	return( static_cast<ValueShort*>( GetValue( _genre, _commandClassId, _instance, _valueIndex, ValueID::ValueType_Short ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueString>
// Get the value object as a ValueString
//-----------------------------------------------------------------------------
ValueString* Node::GetValueString
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	return( static_cast<ValueString*>( GetValue( _genre, _commandClassId, _instance, _valueIndex, ValueID::ValueType_String ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueBool>
// Get the value object as a ValueBool
//-----------------------------------------------------------------------------
ValueBool* Node::GetValueBool
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_Bool != _id.GetType() )
	{
		// Value is not a bool
		assert(0);
		return NULL;
	}

	return( static_cast<ValueBool*>( GetValue( _id ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueByte>
// Get the value object as a ValueByte
//-----------------------------------------------------------------------------
ValueByte* Node::GetValueByte
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_Byte != _id.GetType() )
	{
		// Value is not a byte
		assert(0);
		return NULL;
	}

	return( static_cast<ValueByte*>( GetValue( _id ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueDecimal>
// Get the value object as a ValueDecimal
//-----------------------------------------------------------------------------
ValueDecimal* Node::GetValueDecimal
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_Decimal != _id.GetType() )
	{
		// Value is not a decimal
		assert(0);
		return NULL;
	}

	return( static_cast<ValueDecimal*>( GetValue( _id ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueInt>
// Get the value object as a ValueInt
//-----------------------------------------------------------------------------
ValueInt* Node::GetValueInt
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_Int != _id.GetType() )
	{
		// Value is not an int
		assert(0);
		return NULL;
	}

	return( static_cast<ValueInt*>( GetValue( _id ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueList>
// Get the value object as a ValueList
//-----------------------------------------------------------------------------
ValueList* Node::GetValueList
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_List != _id.GetType() )
	{
		// Value is not a list
		assert(0);
		return NULL;
	}

	return( static_cast<ValueList*>( GetValue( _id ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueShort>
// Get the value object as a ValueShort
//-----------------------------------------------------------------------------
ValueShort* Node::GetValueShort
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_Short != _id.GetType() )
	{
		// Value is not a string
		assert(0);
		return NULL;
	}

	return( static_cast<ValueShort*>( GetValue( _id ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueString>
// Get the value object as a ValueString
//-----------------------------------------------------------------------------
ValueString* Node::GetValueString
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_String != _id.GetType() )
	{
		// Value is not a string
		assert(0);
		return NULL;
	}

	return( static_cast<ValueString*>( GetValue( _id ) ) );
}

//-----------------------------------------------------------------------------
// <Node::GetValueStore>
// Get serialized access to the value store object
//-----------------------------------------------------------------------------
ValueStore* Node::GetValueStore
(
)
{
	m_valuesMutex->Lock();
	return m_values;
}

//-----------------------------------------------------------------------------
// <Node::ReleaseValueStore>
// Release the lock on the value store
//-----------------------------------------------------------------------------
void Node::ReleaseValueStore
(
)
{
	m_valuesMutex->Release();
}

//-----------------------------------------------------------------------------
// <Node::GetGroup>
// Get a Group using the same one-based indexing as Z-Wave device instructions
//-----------------------------------------------------------------------------
Group* Node::GetGroup
(
	uint8 const _groupIdx
)
{ 
	if( ( _groupIdx < 1 ) || ( _groupIdx > m_numGroups ) )
	{
		Log::Write( "Node(%d)::GetGroup - Invalid Group Index %d", m_nodeId, _groupIdx );
		return NULL;
	}

	return m_groups[_groupIdx-1]; 
}

//-----------------------------------------------------------------------------
// <Node::SetNumGroups>
// Set up the array of group pointers
//-----------------------------------------------------------------------------
void Node::SetNumGroups
(
	uint8 const _numGroups
)
{
	if( _numGroups == m_numGroups )
	{
		// Nothing to do
		return;
	}

	// Remove any old groups (in theory this should never need to do anything
	// since the number of groups a device supports should not change)
	uint8 i;
	for( i=0; i<m_numGroups; ++i )
	{
		delete m_groups[i];
	}

	m_numGroups = _numGroups;
	m_groups = new Group*[_numGroups];

	for( i=0; i<m_numGroups; ++i )
	{
		m_groups[i] = new Group( m_driverId, m_nodeId, i+1 ); 
	}
}

//-----------------------------------------------------------------------------
// <Node::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* Node::GetDriver
(
)const
{
	return( Manager::Get()->GetDriver( m_driverId ) );
}






