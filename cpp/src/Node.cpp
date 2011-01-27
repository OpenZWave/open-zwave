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
#include "Options.h"
#include "Manager.h"
#include "Driver.h"
#include "Notification.h"
#include "Msg.h"
#include "Log.h"
#include "Mutex.h"

#include "tinyxml.h"

#include "CommandClasses.h"
#include "CommandClass.h"
#include "Association.h"
#include "Basic.h"
#include "Configuration.h"
#include "ControllerReplication.h"
#include "ManufacturerSpecific.h"
#include "MultiInstance.h"
#include "WakeUp.h"
#include "NodeNaming.h"
#include "Version.h"
#include "SwitchAll.h"

#include "ValueID.h"
#include "Value.h"
#include "ValueBool.h"
#include "ValueButton.h"
#include "ValueByte.h"
#include "ValueDecimal.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueSchedule.h"
#include "ValueShort.h"
#include "ValueString.h"
#include "ValueStore.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// Statics
//-----------------------------------------------------------------------------
bool Node::s_deviceClassesLoaded = false;
map<uint8,string> Node::s_basicDeviceClasses;
map<uint8,Node::GenericDeviceClass*> Node::s_genericDeviceClasses;

static char const* c_queryStageNames[] = 
{
	"None",
	"ProtocolInfo",
	"Neighbors",
	"WakeUp",
	"NodeInfo",
	"ManufacturerSpecific",
	"Versions",
	"Instances",
	"Static",
	"Associations",
	"Session",
	"Dynamic",
	"Configuration",
	"Complete"
};

//-----------------------------------------------------------------------------
// <Node::Node>
// Constructor
//-----------------------------------------------------------------------------
Node::Node
( 
	uint32 const _homeId, 
	uint8 const _nodeId
):
	m_listening( true ),	// assume we start out listening
	m_homeId( _homeId ),
	m_nodeId( _nodeId ),
	m_values( new ValueStore() ),
	m_queryStage( QueryStage_None ),
	m_queryPending( false ),
	m_queryConfiguration( false ),
	m_queryRetries( 0 ),
	m_protocolInfoReceived( false ),
	m_nodeInfoReceived( false )
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
	while( !m_commandClassMap.empty() )
	{
		map<uint8,CommandClass*>::iterator it = m_commandClassMap.begin();
		delete it->second;
		m_commandClassMap.erase( it );
	}

	// Delete the groups
	while( !m_groups.empty() )
	{
		map<uint8,Group*>::iterator it = m_groups.begin();
		delete it->second;
		m_groups.erase( it );
	}

	// Delete the values
	delete m_values;
}

//-----------------------------------------------------------------------------
// <Node::AdvanceQueries>
// Proceed through the initialisation process
//-----------------------------------------------------------------------------
void Node::AdvanceQueries
(
)
{
	// For OpenZWave to discover everything about a node, we have to follow a certain
	// order of queries, because the results of one stage may affect what is requested
	// in the next stage.  The stage is saved with the node data, so that any incomplete
	// queries can be restarted the next time the application runs.
	// The individual command classes also store some state as to whether they have
	// had a response to certain queries.  This state is initilized by the SetStaticRequests
	// call in QueryStage_None.  It is also saved, so we do not need to request state 
	// from every commaned class if some have previously responded. 
	while( !m_queryPending )
	{
		switch( m_queryStage )
		{
			case QueryStage_None:
			{
				// Init the node query process
				m_queryStage = QueryStage_ProtocolInfo;
				m_queryRetries = 0;
				break;
			}
			case QueryStage_ProtocolInfo:
			{
				// determines, among other things, whether this node is a listener, its maximum baud rate and its device classes
				if( !ProtocolInfoReceived() )
				{
					Log::Write( "Node %d: QueryStage_ProtocolInfo", m_nodeId );
					Msg* msg = new Msg( "Get Node Protocol Info", m_nodeId, REQUEST, FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO, false );
					msg->Append( m_nodeId );	
					GetDriver()->SendMsg( msg ); 
					m_queryPending = true;
				}
				else
				{
					// This stage has been done already, so move to the Neighbours stage
					m_queryStage = QueryStage_Neighbors;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Neighbors:
			{
				// retrieves this node's neighbors and stores the neighbor bitmap in the node object
				Log::Write( "Node %d: QueryStage_Neighbors", m_nodeId );
				GetDriver()->RequestNodeNeighbors( m_nodeId );
				m_queryPending = true;
				break;
			}
			case QueryStage_WakeUp:
			{
				// For sleeping devices other than controllers, we need to defer the usual requests until
				// we have told the device to send it's wake-up notifications to the PC controller.
				Log::Write( "Node %d: QueryStage_WakeUp", m_nodeId );

				WakeUp* wakeUp = static_cast<WakeUp*>( GetCommandClass( WakeUp::StaticGetCommandClassId() ) );

				// if this device is a "sleeping device" and not a controller
				if( wakeUp && ( GetBasic() >= 0x03 ) )
				{
					// start the process of requesting node state from this sleeping device
					wakeUp->Init();
					m_queryPending = true;
				}
				else
				{
					// this is not a sleeping device, so move to the NodeInfo stage
					m_queryStage = QueryStage_NodeInfo;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_NodeInfo:
			{
				if( !NodeInfoReceived() )
				{
					// obtain from the node a list of command classes that it 1) supports and 2) controls (separated by a mark in the buffer)
					Log::Write( "Node %d: QueryStage_NodeInfo", m_nodeId );
					Msg* msg = new Msg( "Request Node Info", m_nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_INFO, false, true, FUNC_ID_ZW_APPLICATION_UPDATE );
					msg->Append( m_nodeId );	
					GetDriver()->SendMsg( msg ); 
					m_queryPending = true;
				}
				else
				{
					// This stage has been done already, so move to the Manufacturer Specific stage
					m_queryStage = QueryStage_ManufacturerSpecific;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_ManufacturerSpecific:
			{
				// Obtain manufacturer, product type and product ID code from the node device
				// Manufacturer Specific data is requested before the other command class data so 
				// that we can modify the supported command classes list through the product XML files.
				Log::Write( "Node %d: QueryStage_ManufacturerSpecific", m_nodeId );
				ManufacturerSpecific* cc = static_cast<ManufacturerSpecific*>( GetCommandClass( ManufacturerSpecific::StaticGetCommandClassId() ) );
				if( cc  )
				{
					m_queryPending = cc->RequestState( CommandClass::RequestFlag_Static );
				}
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Versions;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Versions:
			{
				// Get the version information (if the device supports COMMAND_CLASS_VERSION
				Log::Write( "Node %d: QueryStage_Versions", m_nodeId );
				Version* vcc = static_cast<Version*>( GetCommandClass( Version::StaticGetCommandClassId() ) );
				// if this node supports VERSION
				if( vcc )
				{

					for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
					{
						// Get the version for each supported command class
						if( vcc->RequestCommandClassVersion( it->second ) )
						{
							m_queryPending = true;
							break;
						}
					}
				}
				// advance to Instances stage when finished
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Instances;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Instances:
			{
				// if the device at this node supports multiple instances, obtain a list of these instances
				Log::Write( "Node %d: QueryStage_Instances", m_nodeId );
				MultiInstance* micc = static_cast<MultiInstance*>( GetCommandClass( MultiInstance::StaticGetCommandClassId() ) );
				if( micc )
				{
					for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
					{
						if( micc->RequestInstances( it->second ) )
						{
							m_queryPending = true;
							break;
						}
					}
				}
				// when done, advance to the Static stage
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Static;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Static:
			{
				// Request any other static values associated with each command class supported by this node
				// examples are supported thermostat operating modes, setpoints and fan modes
				Log::Write( "Node %d: QueryStage_Static", m_nodeId );
				for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
				{
					if( !it->second->IsAfterMark() )
					{
						m_queryPending |= it->second->RequestState( CommandClass::RequestFlag_Static );
					}
				}

				if( !m_queryPending )
				{
					// when all (if any) static information has been retrieved, advance to the Associations stage
					m_queryStage = QueryStage_Associations;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Associations:
			{
				// if this device supports COMMAND_CLASS_ASSOCIATION, determine to which groups this node belong
				Log::Write( "Node %d: QueryStage_Associations", m_nodeId );
				Association* acc = static_cast<Association*>( GetCommandClass( Association::StaticGetCommandClassId() ) );
				if( acc )
				{
					acc->RequestAllGroups();
					m_queryPending = true;
				}
				else
				{
					// if this device doesn't support Associations, move to retrieve Session information
					m_queryStage = QueryStage_Session;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Session:
			{
				// Request the session values from the command classes in turn
				// examples of Session information are: current thermostat setpoints, node names and climate control schedules
				Log::Write( "Node %d: QueryStage_Session", m_nodeId );
				for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
				{
					if( !it->second->IsAfterMark() )
					{
						m_queryPending |= it->second->RequestState( CommandClass::RequestFlag_Session );
					}
				}
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Dynamic;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Dynamic:
			{
				// Request the dynamic values from the node, that can change at any time
				// Examples include on/off state, heating mode, temperature, etc.
				Log::Write( "Node %d: QueryStage_Dynamic", m_nodeId );
				for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
				{
					if( !it->second->IsAfterMark() )
					{
						m_queryPending |= it->second->RequestState( CommandClass::RequestFlag_Dynamic );
					}
				}
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Configuration; 
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Configuration:
			{
				// Request the configurable parameter values from the node.
				Log::Write( "Node %d: QueryStage_Configuration", m_nodeId );
				if( m_queryConfiguration )
				{
					if( RequestAllConfigParams() )
					{
						m_queryPending = true;
					}
					m_queryConfiguration = false;
				}
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Complete;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Complete:
			{
				// Notify the watchers that the queries are complete for this node
				Log::Write( "Node %d: QueryStage_Complete", m_nodeId );
				Notification* notification = new Notification( Notification::Type_NodeQueriesComplete );
				notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
				GetDriver()->QueueNotification( notification ); 
				return;
			}
			default:
			{
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::QueryStageComplete>
// We are done with a stage in the query process
//-----------------------------------------------------------------------------
void Node::QueryStageComplete
(
	QueryStage const _stage
)
{
	// Check that we are actually on the specified stage
	if( _stage != m_queryStage )
	{
		return;
	}

	if( m_queryStage != QueryStage_Complete )
	{
		// Move to the next stage
		m_queryPending = false;
		m_queryStage = (QueryStage)((uint32)m_queryStage + 1);
		m_queryRetries = 0;
	}
}

//-----------------------------------------------------------------------------
// <Node::QueryStageRetry>
// Retry a stage up to the specified maximum
//-----------------------------------------------------------------------------
void Node::QueryStageRetry
(
	QueryStage const _stage,
	uint8 const _maxAttempts // = 0
)
{
	// Check that we are actually on the specified stage
	if( _stage != m_queryStage )
	{
		return;
	}

	++m_queryRetries;
	if( _maxAttempts && ( m_queryRetries >= _maxAttempts ) )
	{
		// We've retried too many times.  Move to the next stage.
		// Setting m_queryRetries to 0 will make it look like the stage succeeded.
		m_queryRetries = 0;
	}
	m_queryPending = false;
}

//-----------------------------------------------------------------------------
// <Node::GoBackToQueryStage>
// Set the query stage (but only to an earlier stage)
//-----------------------------------------------------------------------------
void Node::GoBackToQueryStage
(
	QueryStage const _stage
)
{
	if( (int)_stage < (int)m_queryStage )
	{
		m_queryStage = _stage;
		m_queryPending = false;
	}

	if( QueryStage_Configuration == _stage )
	{
		m_queryConfiguration = true;
	}
}

//-----------------------------------------------------------------------------
// <Node::GetQueryStageName>
// Gets the query stage name
//-----------------------------------------------------------------------------
string Node::GetQueryStageName
(
	QueryStage const _stage
)
{
	return c_queryStageNames[_stage];
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

	str = _node->Attribute( "query_stage" );
	if( str )
	{
		// After restoring state from a file, we need to at least refresh the association, session and dynamic values.
		m_queryStage = QueryStage_Associations;			
		for( uint32 i=0; i<(uint32)QueryStage_Associations; ++i )
		{
			if( !strcmp( str, c_queryStageNames[i] ) )
			{
				m_queryStage = (QueryStage)i;
				break;
			}
		}
	}

	if( m_queryStage > QueryStage_ProtocolInfo )
	{
		// Notify the watchers of the protocol info.
		// We do the notification here so that it gets into the queue ahead of
		// any other notifications generated by adding command classes etc.
		m_protocolInfoReceived = true;
		Notification* notification = new Notification( Notification::Type_NodeProtocolInfo );
		notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
		GetDriver()->QueueNotification( notification );
	}

	if( m_queryStage > QueryStage_NodeInfo )
	{
		m_nodeInfoReceived = true;
	}

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
				Notification* notification = new Notification( Notification::Type_NodeNaming );
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

				// Check whether this command class is to be removed (product XMLs might
				// request this if a class is not implemented properly by the device)
				bool remove = false;
				char const* action = ccElement->Attribute( "action" );
				if( action && !strcasecmp( action, "remove" ) )
				{
					remove = true;
				}		

				CommandClass* cc = GetCommandClass( id );
				if( remove )
				{	
					// Remove support for the command class
					RemoveCommandClass( id );
				}
				else
				{
					if( NULL == cc )
					{
						// Command class support does not exist yet, so we create it
						cc = AddCommandClass( id );
					}

					if( NULL != cc )
					{
						cc->ReadXML( ccElement );
					}
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

	nodeElement->SetAttribute( "query_stage", c_queryStageNames[m_queryStage] );

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
	if( ProtocolInfoReceived() )
	{
		// We already have this info
		return;
	}

	// Notify the watchers of the protocol info.
	// We do the notification here so that it gets into the queue ahead of
	// any other notifications generated by adding command classes etc.
	Notification* notification = new Notification( Notification::Type_NodeProtocolInfo );
	notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
	GetDriver()->QueueNotification( notification );

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

    // Optional flag is true if the device reports optional command classes.
    // NOTE: We stopped using this because not all devices report it properly,
    // and now just request the optional classes regardless.
	// bool optional = (( _data[1] & 0x80 ) != 0 );	

	Log::Write( "Protocol Info for Node %d:", m_nodeId );
	Log::Write( "  Listening     = %s", m_listening ? "true" : "false" );
	Log::Write( "  Routing       = %s", m_routing ? "true" : "false" );
	Log::Write( "  Max Baud Rate = %d", m_maxBaudRate );
	Log::Write( "  Version       = %d", m_version );
	Log::Write( "  Security      = 0x%.2x", m_security );

	// Set up the device class based data for the node, including mandatory command classes
	SetDeviceClasses( _data[3], _data[4], _data[5] );
	m_protocolInfoReceived = true;
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
	if( !NodeInfoReceived() )
	{
		// Add the command classes specified by the device
		Log::Write( "Optional command classes for node %d:", m_nodeId );
		
		bool newCommandClasses = false;
		uint32 i;

		bool afterMark = false;
		for( i=0; i<_length; ++i )
		{
			if( _data[i] == 0xef )
			{
				// COMMAND_CLASS_MARK.  
				// Marks the end of the list of supported command classes.  The remaining classes 
				// are those that can be controlled by the device.  These classes are created 
				// without values.  Messages received cause notification events instead.
				afterMark = true;

				if( !newCommandClasses )
				{
					Log::Write( "  None" );
				}
				Log::Write( "Optional command classes controlled by node %d:", m_nodeId );
				newCommandClasses = false;
				continue;
			}

			if( CommandClasses::IsSupported( _data[i] ) )
            {
                if( CommandClass* pCommandClass = AddCommandClass( _data[i] ) )
				{
					// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
					if( afterMark )
					{
						pCommandClass->SetAfterMark();
					}

					// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
					// then some command class instance counts will increase once the responses to the RequestState
					// call at the end of this method have been processed.
                    pCommandClass->SetInstances( 1 );
					newCommandClasses = true;
					Log::Write( "  %s", pCommandClass->GetCommandClassName().c_str() );
                }
            }
            else
            {
                Log::Write( "Node(%d)::CommandClass 0x%.2x - NOT REQUIRED", m_nodeId, _data[i] );
			}
		}

		if( !newCommandClasses )
		{
			// No additional command classes over the mandatory ones.
			Log::Write( "  None" );
		}

		SetStaticRequests();
		m_nodeInfoReceived = true;
	}
	else
	{
		// We probably only need to do the dynamic stuff
		GetDriver()->AddNodeQuery( m_nodeId, QueryStage_Dynamic );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetStaticRequests>
// The first time we hear from a node, we set flags to indicate the
// need to request certain static data from the device.  This is so that
// we can track which data has been received, and which has not.
//-----------------------------------------------------------------------------
void Node::SetStaticRequests
(
)
{
	uint8 request = 0;

	if( GetCommandClass( MultiInstance::StaticGetCommandClassId() ) )
	{
		// Request instances
		request |= (uint8)CommandClass::StaticRequest_Instances;
	}

	if( GetCommandClass( Version::StaticGetCommandClassId() ) )
	{
		// Request versions
		request |= (uint8)CommandClass::StaticRequest_Version;
	}

	if( request )
	{
		for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
		{
			it->second->SetStaticRequest( request );
		}
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
		cc->SetName( _nodeName );
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
	if( NodeNaming* cc = static_cast<NodeNaming*>( GetCommandClass( NodeNaming::StaticGetCommandClassId() ) ) )
	{
		// The node supports naming, so we try to write the location into the device
		cc->SetLocation( _location );
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
// Get the specified command class object if supported, otherwise NULL
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
// <Node::RemoveCommandClass>
// Remove a command class from the node
//-----------------------------------------------------------------------------
void Node::RemoveCommandClass
( 
	uint8 const _commandClassId
)
{
	map<uint8,CommandClass*>::iterator it = m_commandClassMap.find( _commandClassId );
	if( it == m_commandClassMap.end() )
	{
		// Class is not found
		return;
	}

	// Remove all the values associated with this class
	if( ValueStore* store = GetValueStore() )
	{
		store->RemoveCommandClassValues( _commandClassId );
	}

	// Destroy the command class object and remove it from our map
	Log::Write( "Node(%d)::RemoveCommandClass - Removed support for %s", m_nodeId, it->second->GetCommandClassName().c_str() );

	delete it->second;
	m_commandClassMap.erase( it );
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
		if( Value* value = cc->GetValue( 1, _param ) )
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
				default:
				{
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
		cc->RequestValue( _param );
	}
}

//-----------------------------------------------------------------------------
// <Node::RequestAllConfigParams>
// Request the values of all known configuration parameters from the device
//-----------------------------------------------------------------------------
bool Node::RequestAllConfigParams
(	
)
{
	bool res = false;
	if( Configuration* cc = static_cast<Configuration*>( GetCommandClass( Configuration::StaticGetCommandClassId() ) ) )
	{
		// Go through all the values in the value store, and request all those which are in the Configuration command class
		for( ValueStore::Iterator it = m_values->Begin(); it != m_values->End(); ++it )
		{
			Value* value = it->second;
			if( value->GetID().GetCommandClassId() == Configuration::StaticGetCommandClassId() )
			{
				cc->RequestValue( value->GetID().GetIndex() );
				res = true;
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Node::SetLevel>
// Helper method to set a device's basic level
//-----------------------------------------------------------------------------
void Node::SetLevel
(
	uint8 const _level
)
{
	// Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
	uint8 adjustedLevel = _level;
	if( ( _level > 99 ) && ( _level < 255 ) )
	{
		adjustedLevel = 99;
	}

	if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
	{
		cc->Set( adjustedLevel );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeOn>
// Helper method to set a device to be on
//-----------------------------------------------------------------------------
void Node::SetNodeOn
(
)
{
    // Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
    if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
    {
        cc->Set( 255 );
    }
}

//-----------------------------------------------------------------------------
// <Node::SetNodeOff>
// Helper method to set a device to be off
//-----------------------------------------------------------------------------
void Node::SetNodeOff
(
)
{
    // Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
    if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
    {
        cc->Set( 0 );
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
// <Node::CreateValueButton>
// Helper to create a new trigger value and add it to the value store
//-----------------------------------------------------------------------------
ValueButton* Node::CreateValueButton
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label
)
{
	ValueButton* value = new ValueButton( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label );
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
// <Node::CreateValueSchedule>
// Helper to create a new schedule value and add it to the value store
//-----------------------------------------------------------------------------
ValueSchedule* Node::CreateValueSchedule
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly
)
{
	ValueSchedule* value = new ValueSchedule( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly );
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
	int16 const _default
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
	return value;
}

//-----------------------------------------------------------------------------
// <Node::RemoveValueList>
// Helper to remove an existing list value from the value store
//-----------------------------------------------------------------------------
void Node::RemoveValueList
(
	ValueList* _value
)
{
	ValueStore* store = GetValueStore();
	store->RemoveValue( _value->GetID().GetValueStoreKey() );
	delete _value;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueFromXML>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
Value* Node::CreateValueFromXML
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
		case ValueID::ValueType_Bool:		{	value = new ValueBool();		break;	}
		case ValueID::ValueType_Byte:		{	value = new ValueByte();		break;	}
		case ValueID::ValueType_Decimal:	{	value = new ValueDecimal();		break;	}
		case ValueID::ValueType_Int:		{	value = new ValueInt();			break;	}
		case ValueID::ValueType_List:		{	value = new ValueList();		break;	}
		case ValueID::ValueType_Schedule:	{	value = new ValueSchedule();	break;	}
		case ValueID::ValueType_Short:		{	value = new ValueShort();		break;	}
		case ValueID::ValueType_String:		{	value = new ValueString();		break;	}
		case ValueID::ValueType_Button:		{	value = new ValueButton();		break;	}
	}

	if( value )
	{
		value->ReadXML( m_homeId, m_nodeId, _commandClassId, _valueElement );

		ValueStore* store = GetValueStore();
		store->AddValue( value );
	}

	return value;
}

//-----------------------------------------------------------------------------
// <Node::ReadValueFromXML>
// Apply XML differences to a value
//-----------------------------------------------------------------------------
void Node::ReadValueFromXML
( 
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	int32 intVal;

	ValueID::ValueGenre genre = Value::GetGenreEnumFromName( _valueElement->Attribute( "genre" ) );
	ValueID::ValueType type = Value::GetTypeEnumFromName( _valueElement->Attribute( "type" ) );

	uint8 instance = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "instance", &intVal ) )
	{
		instance = (uint8)intVal;
	}

	uint8 index = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "index", &intVal ) )
	{
		index = (uint8)intVal;
	}

	ValueID id = ValueID( m_homeId, m_nodeId, genre, _commandClassId, instance, index, type );

	// Try to get the value from the ValueStore (everything except configuration parameters
	// should already have been created when the command class instance count was read in).
	// Create it if it doesn't already exist.
	if( ValueStore* store = GetValueStore() )
	{
		if( Value* value = store->GetValue( id.GetValueStoreKey() ) )
		{
			value->ReadXML( m_homeId, m_nodeId, _commandClassId, _valueElement );
		}
		else
		{
			CreateValueFromXML( _commandClassId, _valueElement );
		}
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
	return GetValueStore()->GetValue( _id.GetValueStoreKey() );
}

//-----------------------------------------------------------------------------
// <Node::GetValue>
// Get the value object with the specified settings
//-----------------------------------------------------------------------------
Value* Node::GetValue
(
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	Value* value = NULL;
	ValueStore* store = GetValueStore();
	value = store->GetValue( ValueID::GetValueStoreKey( _commandClassId, _instance, _valueIndex ) );
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
// <Node::GetAssociations>
// Gets the maximum number of associations for a group
//-----------------------------------------------------------------------------
uint8 Node::GetMaxAssociations
(
	uint8 const _groupIdx
)
{
	uint8 maxAssociations = 0;
	if( Group* group = GetGroup( _groupIdx ) )
	{
		maxAssociations = group->GetMaxAssociations();	
	}

	return maxAssociations;
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
    if( Group* group = GetGroup( _groupIdx ) )
	{
		group->AddAssociation( _targetNodeId );
	}
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
    if( Group* group = GetGroup( _groupIdx ) )
	{
		group->RemoveAssociation( _targetNodeId );
	}
}

//-----------------------------------------------------------------------------
// <Node::AutoAssociate>
// Automatically associate the controller with certain groups
//-----------------------------------------------------------------------------
void Node::AutoAssociate
(
)
{
	bool autoAssociate = false;
	Options::Get()->GetOptionAsBool( "Associate", &autoAssociate );
	if( autoAssociate )
	{
		// Try to automatically associate with any groups that have been flagged.
		uint8 controllerNodeId = GetDriver()->GetNodeId();

		for( map<uint8,Group*>::iterator it = m_groups.begin(); it != m_groups.end(); ++it )
		{
			Group* group = it->second;
			if( group->IsAuto() && !group->Contains( controllerNodeId ) )
			{
				// Associate the controller into the group
				Log::Write( "Adding the controller to group %d (%s) of node %d", group->GetIdx(), group->GetLabel().c_str(), GetNodeId() );
				group->AddAssociation( controllerNodeId );
			}
		}
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
	return( Manager::Get()->GetDriver( m_homeId ) );
}

//-----------------------------------------------------------------------------
// Device Classes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Node::SetDeviceClasses>
// Set the device class data for the node
//-----------------------------------------------------------------------------
bool Node::SetDeviceClasses
( 
	uint8 const _basic,
	uint8 const _generic,
	uint8 const _specific
)
{
	m_basic = _basic;
	m_generic = _generic;
	m_specific = _specific;

	// Read in the device class data if it has not been read already. 
	if( !s_deviceClassesLoaded )
	{
		ReadDeviceClasses();
	}

	// Get the basic device class label
	map<uint8,string>::iterator bit = s_basicDeviceClasses.find( _basic );
	if( bit != s_basicDeviceClasses.end() )
	{
		m_type = bit->second;
		Log::Write( "Node(%d) Basic device class    (0x%.2x) - %s", m_nodeId, m_basic, m_type.c_str() );
	}
	else
	{
		Log::Write( "Node(%d) Basic device class unknown", m_nodeId );
	}

	// Apply any Generic device class data
	uint8 basicMapping = 0;
	map<uint8,GenericDeviceClass*>::iterator git = s_genericDeviceClasses.find( _generic );
	if( git != s_genericDeviceClasses.end() )
	{
		GenericDeviceClass* genericDeviceClass = git->second;
		m_type = genericDeviceClass->GetLabel();

		Log::Write( "Node(%d) Generic device Class  (0x%.2x) - %s", m_nodeId, m_generic, m_type.c_str() );

		// Add the mandatory command classes for this generic class type
		AddMandatoryCommandClasses( genericDeviceClass->GetMandatoryCommandClasses() );
		
		// Get the command class that COMMAND_CLASS_BASIC maps to.
		basicMapping = genericDeviceClass->GetBasicMapping();

		// Apply any Specific device class data
		if( DeviceClass* specificDeviceClass = genericDeviceClass->GetSpecificDeviceClass( _specific ) )
		{
			m_type = specificDeviceClass->GetLabel();

			Log::Write( "Node(%d) Specific device class (0x%.2x) - %s", m_nodeId, m_specific, m_type.c_str() );

			// Add the mandatory command classes for this specific class type
			AddMandatoryCommandClasses( specificDeviceClass->GetMandatoryCommandClasses() );
			
			if( specificDeviceClass->GetBasicMapping() )
			{
				// Override the generic device class basic mapping with the specific device class one.
				basicMapping = specificDeviceClass->GetBasicMapping();
			}
		}
		else
		{
			Log::Write( "Node(%d) No specific device class defined", m_nodeId );
		}
	}
	else
	{
		Log::Write( "Node(%d) No generic or specific device classes defined", m_nodeId );
	}

	// Deal with sleeping devices
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

	// Apply any COMMAND_CLASS_BASIC remapping
	if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
	{
		cc->SetMapping( basicMapping );
	}

	// Write the mandatory command classes to the log
	if( !m_commandClassMap.empty() )
	{
		map<uint8,CommandClass*>::const_iterator cit;

		Log::Write( "Mandatory Command Classes for Node %d:", m_nodeId );
		bool reportedClasses = false;
		for( cit = m_commandClassMap.begin(); cit != m_commandClassMap.end(); ++cit )
		{
			if( !cit->second->IsAfterMark() )
			{
				Log::Write( "  %s", cit->second->GetCommandClassName().c_str() );
				reportedClasses = true;
			}
		}
		if( !reportedClasses )
		{
			Log::Write( "  None" );
		}

		Log::Write( "Mandatory Command Classes controlled by Node %d:", m_nodeId );
		reportedClasses = false;
		for( cit = m_commandClassMap.begin(); cit != m_commandClassMap.end(); ++cit )
		{
			if( cit->second->IsAfterMark() )
			{
				Log::Write( "  %s", cit->second->GetCommandClassName().c_str() );
				reportedClasses = true;
			}
		}
		if( !reportedClasses )
		{
			Log::Write( "  None" );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Node::AddMandatoryCommandClasses>
// Add mandatory command classes to the node
//-----------------------------------------------------------------------------
bool Node::AddMandatoryCommandClasses
(
	uint8 const* _commandClasses
)
{
	if( NULL == _commandClasses )
	{
		// No command classes to add
		return false;
	}

	int i=0;
	bool afterMark = false;
	while( uint8 cc = _commandClasses[i++] )
	{
		if( cc == 0xef )
		{
			// COMMAND_CLASS_MARK.  
			// Marks the end of the list of supported command classes.  The remaining classes 
			// are those that can be controlled by this device, which we can ignore.
			afterMark = true;
			continue;
		}

		if( CommandClasses::IsSupported( cc ) )
        {
			if( CommandClass* commandClass = AddCommandClass( cc ) )
			{
				// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
				if( afterMark )
				{
					commandClass->SetAfterMark();
				}

				// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
				// then some command class instance counts will increase.
				commandClass->SetInstances( 1 );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Node::ReadDeviceClasses>
// Read the static device class data from the device_classes.xml file
//-----------------------------------------------------------------------------
void Node::ReadDeviceClasses
(
)
{
	// Load the XML document that contains the device class information
	string configPath;
	Options::Get()->GetOptionAsString( "ConfigPath", &configPath );

	string filename =  configPath + string("device_classes.xml");

	TiXmlDocument doc;
	if( !doc.LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		Log::Write( "Failed to load device_classes.xml" );
		Log::Write( "Check that the config path provided when creating the Manager points to the correct location." );
		return;
	}

	TiXmlElement const* deviceClassesElement = doc.RootElement();

	// Read the basic and generic device classes
	TiXmlElement const* child = deviceClassesElement->FirstChildElement();
	while( child )
	{
		char const* str = child->Value();
		if( str )
		{
			char const* keyStr = child->Attribute( "key" );
			if( keyStr )
			{
				char* pStop;
				uint8 key = (uint8)strtol( keyStr, &pStop, 16 );

				if( !strcmp( str, "Generic" ) )
				{
					s_genericDeviceClasses[key] = new GenericDeviceClass( child ); 
				}
				else if( !strcmp( str, "Basic" ) )
				{
					char const* label = child->Attribute( "label" );
					if( label )
					{
						s_basicDeviceClasses[key] = label;
					}
				}
			}
		}

		child = child->NextSiblingElement();
	}

	s_deviceClassesLoaded = true;
}

//-----------------------------------------------------------------------------
// <DeviceClass::DeviceClass>
// Constructor
//-----------------------------------------------------------------------------
Node::DeviceClass::DeviceClass
(
	TiXmlElement const* _el
):
	m_mandatoryCommandClasses(NULL),
	m_basicMapping(0)
{
	char const* str = _el->Attribute( "label" );
	if( str )
	{
		m_label = str;
	}

	str = _el->Attribute( "command_classes" );
	if( str )
	{
		// Parse the comma delimted command class 
		// list into a temporary vector.
		vector<uint8> ccs;
		char* pos = const_cast<char*>(str);
		while( *pos )
		{
			ccs.push_back( (uint8)strtol( pos, &pos, 16 ) );
			if( (*pos) == ',' )
			{
				++pos;
			}
		}

		// Copy the vector contents into an array.
		uint32 numCCs = ccs.size(); 
		m_mandatoryCommandClasses = new uint8[numCCs+1];
		m_mandatoryCommandClasses[numCCs] = 0;	// Zero terminator

		for( uint32 i=0; i<numCCs; ++i )
		{
			m_mandatoryCommandClasses[i] = ccs[i];
		}
	}

	str = _el->Attribute( "basic" );
	if( str )
	{
		char* pStop;
		m_basicMapping = (uint8)strtol( str, &pStop, 16 );
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::GenericDeviceClass>
// Constructor
//-----------------------------------------------------------------------------
Node::GenericDeviceClass::GenericDeviceClass
(
	TiXmlElement const* _el
):
	DeviceClass( _el )
{
	// Add any specific device classes
	TiXmlElement const* child = _el->FirstChildElement();
	while( child )
	{
		char const* str = child->Value();
		if( str && !strcmp( str, "Specific" ) )
		{
			char const* keyStr = child->Attribute( "key" );
			if( keyStr )
			{
				char* pStop;
				uint8 key = (uint8)strtol( keyStr, &pStop, 16 );
				
				m_specificDeviceClasses[key] = new DeviceClass( child ); 
			}
		}

		child = child->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::~GenericDeviceClass>
// Destructor
//-----------------------------------------------------------------------------
Node::GenericDeviceClass::~GenericDeviceClass
(
)
{
	while( !m_specificDeviceClasses.empty() )
	{
		map<uint8,DeviceClass*>::iterator it = m_specificDeviceClasses.begin();
		delete it->second;
		m_specificDeviceClasses.erase( it );
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::GetSpecificDeviceClass>
// Get a specific device class object
//-----------------------------------------------------------------------------
Node::DeviceClass* Node::GenericDeviceClass::GetSpecificDeviceClass
(
	uint8 const& _specific
)
{
	map<uint8,DeviceClass*>::iterator it = m_specificDeviceClasses.find( _specific );
	if( it != m_specificDeviceClasses.end() )
	{
		return it->second;
	}

	return NULL;
}










