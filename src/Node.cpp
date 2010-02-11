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
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
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

#include "Node.h"
#include "Defs.h"
#include "Group.h"
#include "Driver.h"
#include "Msg.h"
#include "Log.h"
#include "Mutex.h"

#include "tinyxml.h"

#include "CommandClasses.h"
#include "CommandClass.h"
#include "Association.h"
#include "Basic.h"
#include "Configuration.h"
#include "MultiInstance.h"
#include "WakeUp.h"

#include "ValueID.h"
#include "Value.h"
#include "ValueByte.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <Node::Node>
// Constructor
//-----------------------------------------------------------------------------
Node::Node
( 
	uint8 const _nodeId
):
	m_nodeId( _nodeId ),
	m_bPolled( false ),
	m_bProtocolInfoReceived( false ),
	m_bNodeInfoReceived( false ),
	m_values( new ValueStore() ),
	m_valuesMutex( new Mutex() )
{
	// Request the node protocol info
	Driver::Get()->AddInfoRequest( m_nodeId );
}

//-----------------------------------------------------------------------------
// <Node::Node>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Node::Node
( 
	TiXmlElement* _node	
):
	m_bProtocolInfoReceived( true ),
	m_bNodeInfoReceived( true ),
	m_values( new ValueStore() )
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
		m_bListening = !strcmp( str, "True" );
	}

	str = _node->Attribute( "polled" );
	if( str )
	{
		m_bPolled = !strcmp( str, "True" );
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

	// Get the current values from the node.  Polled devices will do this every few
	// seconds anyway, so we only make the request for non-polled devices.
	if( !IsPolled() )
	{
		RequestState();
	}
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
	if( m_bProtocolInfoReceived )
	{
		// We already have this info
		return;
	}
	m_bProtocolInfoReceived = true;

	// Remove the protocol info request from the queue
	Driver::Get()->RemoveInfoRequest();

	// Capabilities
	m_bListening = (( _data[0] & 0x80 ) != 0 );
	m_bRouting = (( _data[0] & 0x40 ) != 0 );
	
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
	Log::Write( "  Listening	 = %s", m_bListening ? "True" : "False" );
	Log::Write( "  Routing	   = %s", m_bRouting ? "True" : "False" );
	Log::Write( "  Max Baud Rate = %d", m_maxBaudRate );
	Log::Write( "  Version	   = %d", m_version );
	Log::Write( "  Security	  = 0x%.2x", m_security );
	Log::Write( "  Basic Type	= %s", m_basicLabel.c_str() );
	Log::Write( "  Generic Type  = %s", m_genericLabel.c_str() );

	if( !m_bListening )
	{
		// Device does not always listen, so we need the WakeUp handler
		if( CommandClass* pCommandClass = AddCommandClass( WakeUp::StaticGetCommandClassId() ) )
		{
			pCommandClass->SetInstances( 1 );
		}
	}

	// Request the command classes
	Msg* msg = new Msg( "Request Node Info", m_nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_INFO, false, true, FUNC_ID_ZW_APPLICATION_UPDATE );
	msg->Append( m_nodeId );	
	Driver::Get()->SendMsg( msg ); 
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
	if( !m_bNodeInfoReceived )
	{
		m_bNodeInfoReceived = true;

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

		// Get the static configuration from the node
		RequestStatic();
	}

	// Get the current values from the node
	RequestState();
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
	if( CommandClass* pCommandClass = CommandClasses::CreateCommandClass( _commandClassId, m_nodeId ) )
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
// <Node::RequestState>
// Request the current state
//-----------------------------------------------------------------------------
void Node::RequestState
( 
)
{
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		it->second->RequestState();
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
	fprintf( _file, "  <Node id=\"%d\" listening=\"%s\" basic=\"%d\" basic_label=\"%s\" generic=\"%d\" generic_label=\"%s\" specific=\"%d\">\n", m_nodeId, m_bListening ? "True" : "False", m_basic, m_basicLabel.c_str(), m_generic, m_genericLabel.c_str(), m_specific );

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
	// in the device, its response should cause the creation of a value object for future use.
	if( ValueStore* store = GetValueStore() )
	{
		ValueID id( m_nodeId, Configuration::StaticGetCommandClassId(), 0, _param );
		if( Value* value = store->GetValue( id ) )
		{
			uint8 const valueType = value->GetValueTypeId();
			if( valueType == ValueByte::StaticGetValueTypeId() )
			{
				ValueByte* valueByte = static_cast<ValueByte*>( value );
				valueByte->Set( (uint8)_value );
				return true;
			}
			else if( valueType == ValueInt::StaticGetValueTypeId() )
			{
				ValueInt* valueInt = static_cast<ValueInt*>( value );
				valueInt->Set( _value );
				return true;
			}
			else if( valueType == ValueList::StaticGetValueTypeId() )
			{
				ValueList* valueList = static_cast<ValueList*>( value );
				valueList->SetByValue( _value );
				return true;		
			}
		}

		ReleaseValueStore();
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
// <Node::GetValue>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
Value* Node::GetValue
(
	ValueID const& _id
)const
{
	return m_values->GetValue( _id );
}





