//-----------------------------------------------------------------------------
//
//	MultiInstance.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_INSTANCE
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

#include "CommandClasses.h"
#include "MultiInstance.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

using namespace OpenZWave;

// Reduced set of Generic Device classes sorted to reduce
// the likely number of calls to MultiChannelCmd_EndPointFind.
uint8 const c_genericClass[] =
{
	0x21,		// Multilevel Sensor
	0x20,		// Binary Sensor
	0x31,		// Meter
	0x08,		// Thermostat
	0x11,		// Multilevel Switch
	0x10,		// Binary Switch
	0x12,		// Remote Switch
	0xa1,		// Alarm Sensor
	0x16,		// Ventilation
	0x30,		// Pulse Meter
	0x40,		// Entry Control
	0x13,		// Toggle Switch
	0x03,		// AV Control Point
	0x04,		// Display
	0x00		// End of list
};

char const* c_genericClassName[] =
{
	"Multilevel Sensor",
	"Binary Sensor",
	"Meter",
	"Thermostat",
	"Multilevel Switch",
	"Binary Switch",
	"Remote Switch",
	"Alarm Sensor",
	"Ventilation",
	"Pulse Meter",
	"Entry Control",
	"Toggle Switch",
	"AV Control Point",
	"Display"
};

//-----------------------------------------------------------------------------
// <MultiInstance::RequestInstances>
// Request number of instances of the specified command class from the device
//-----------------------------------------------------------------------------
bool MultiInstance::RequestInstances
(
)
{
	bool res = false;

	if( GetVersion() == 1 )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			// MULTI_INSTANCE
			char str[128];
			for( map<uint8,CommandClass*>::const_iterator it = node->m_commandClassMap.begin(); it != node->m_commandClassMap.end(); ++it )
			{
				CommandClass* cc = it->second;
 				if( cc->HasStaticRequest( StaticRequest_Instances ) )
				{
					snprintf( str, sizeof( str ), "MultiInstanceCmd_Get for %s", cc->GetCommandClassName().c_str() );

					Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->Append( GetNodeId() );
					msg->Append( 3 );
					msg->Append( GetCommandClassId() );
					msg->Append( MultiInstanceCmd_Get );
					msg->Append( cc->GetCommandClassId() );
					msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
					GetDriver()->SendMsg( msg, Driver::MsgQueue_Query );
					res = true;
				}
			}
		}
	}
	else
	{
		// MULTI_CHANNEL
		char str[128];
		snprintf( str, sizeof( str ), "MultiChannelCmd_EndPointGet for node %d", GetNodeId() );

		Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelCmd_EndPointGet );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Query );
		res = true;
	}

	return res;
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MultiInstance::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	bool handled = false;
	Node* node = GetNodeUnsafe();
	if( node != NULL )
	{
		handled = true;
		switch( (MultiInstanceCmd)_data[0] )
		{
			case MultiInstanceCmd_Report:
			{
				HandleMultiInstanceReport( _data, _length );
				break;
			}
			case MultiInstanceCmd_Encap:
			{
				HandleMultiInstanceEncap( _data, _length );
				break;
			}
			case MultiChannelCmd_EndPointReport:
			{
				HandleMultiChannelEndPointReport( _data, _length );
				break;
			}
			case MultiChannelCmd_CapabilityReport:
			{
				HandleMultiChannelCapabilityReport( _data, _length );
				break;
			}
			case MultiChannelCmd_EndPointFindReport:
			{
				HandleMultiChannelEndPointFindReport( _data, _length );
				break;
			}
			case MultiChannelCmd_Encap:
			{
				HandleMultiChannelEncap( _data, _length );
				break;
			}
			default:
			{
				handled = false;
				break;
			}
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiInstanceReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiInstanceReport
(	
	uint8 const* _data,
	uint32 const _length
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		uint8 commandClassId = _data[1];
		uint8 instances = _data[2];
		if( GetVersion() > 1 )
		{
			instances &= 0x7f;
		}

		if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
		{
			Log::Write( LogLevel_Info, "%s, Received MultiInstanceReport from node %d for %s: Number of instances = %d", GetDriver()->GetNodeString( GetNodeId() ).c_str(), GetNodeId(), pCommandClass->GetCommandClassName().c_str(), instances );
			pCommandClass->SetInstances( instances );
			pCommandClass->ClearStaticRequest( StaticRequest_Instances );
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiInstanceEncap>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiInstanceEncap
(	
	uint8 const* _data,
	uint32 const _length
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		uint8 instance = _data[1];
		if( GetVersion() > 1 )
		{
			instance &= 0x7f;
		}
		uint8 commandClassId = _data[2];

		if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
		{
			Log::Write( LogLevel_Info, "%s, Received a MultiInstanceEncap from node %d, instance %d, for Command Class %s", GetDriver()->GetNodeString( GetNodeId() ).c_str(), GetNodeId(), instance, pCommandClass->GetCommandClassName().c_str() );
			pCommandClass->HandleMsg( &_data[3], _length-3, instance );
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelEndPointReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelEndPointReport
(	
	uint8 const* _data,
	uint32 const _length
)
{
	if( m_numEndpoints != 0 )
	{
		return;
	}

	m_numEndPointsCanChange = (( _data[1] & 0x80 ) != 0 );	// Number of endpoints can change.
	m_endPointsAreSameClass = (( _data[1] & 0x40 ) != 0 );	// All endpoints are the same command class.
	m_numEndpoints = _data[2] & 0x7f;

	if( m_endPointsAreSameClass )
	{
		Log::Write( LogLevel_Info, "%s, Received MultiChannelEndPointReport from node %d.  All %d endpoints are the same.", GetDriver()->GetNodeString( GetNodeId() ).c_str(), GetNodeId(), m_numEndpoints );
	
		// Send a single capability request to endpoint 1 (since all classes are the same)
		char str[128];
		snprintf( str, sizeof( str ), "MultiChannelCmd_CapabilityGet for endpoint 1" );
		Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelCmd_CapabilityGet );
		msg->Append( 1 );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
	else
	{
		Log::Write( LogLevel_Info, "%s, Received MultiChannelEndPointReport from node %d.  Endpoints are not all the same.", GetDriver()->GetNodeString( GetNodeId() ).c_str(), GetNodeId() );
		Log::Write( LogLevel_Info, "%s,    Starting search for endpoints by generic class...", GetDriver()->GetNodeString( GetNodeId() ).c_str() );

		// This is where things get really ugly.  We need to get the capabilities of each
		// endpoint, but we only know how many there are, not which indices they
		// are at.  We will first try to use MultiChannelCmd_EndPointFind to get
		// lists of indices.  We have to specify a generic device class in the find,
		// so we try generic classes in an order likely to find the endpoints the quickest.
		m_endPointFindIndex = 0;
		m_numEndPointsFound = 0;

		char str[128];
		snprintf( str, 128, "MultiChannelCmd_EndPointFind for generic device class 0x%.2x (%s)", c_genericClass[m_endPointFindIndex], c_genericClassName[m_endPointFindIndex] );
		Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelCmd_EndPointFind );
		msg->Append( c_genericClass[m_endPointFindIndex] );		// Generic device class
		msg->Append( 0xff );									// Any specific device class
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelCapabilityReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelCapabilityReport
(	
	uint8 const* _data,
	uint32 const _length
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		uint8 endPoint = _data[1] & 0x7f;
		bool dynamic = ((_data[1] & 0x80)!=0);

		Log::Write( LogLevel_Info, "%s, Received MultiChannelCapabilityReport from node %d for endpoint %d", GetDriver()->GetNodeString( GetNodeId() ).c_str(), GetNodeId(), endPoint );
		Log::Write( LogLevel_Info, "%s,	 Endpoint is%sdynamic, and is a %s", GetDriver()->GetNodeString( GetNodeId() ).c_str(), dynamic ? " " : " not ", node->GetEndPointDeviceClassLabel( _data[2], _data[3] ).c_str() );
		Log::Write( LogLevel_Info, "%s,    Command classes supported by the endpoint are:", GetDriver()->GetNodeString( GetNodeId() ).c_str() );

		// Store the command classes for later use
		bool afterMark = false;
		m_endPointCommandClasses.clear();
		uint8 numCommandClasses = _length - 5;
		for( uint8 i=0; i<numCommandClasses; ++i )
		{
			uint8 commandClassId = _data[i+4];
			if( commandClassId == 0xef )
			{
				afterMark = true;
				continue;
			}

			m_endPointCommandClasses.insert( commandClassId );

			// Ensure the node supports this command class
			CommandClass* cc = node->GetCommandClass( commandClassId );
			if( !cc )
			{
				cc = node->AddCommandClass( commandClassId );
				if( cc && afterMark )
				{
					cc->SetAfterMark();
				}
			}
			if( cc )
			{
				Log::Write( LogLevel_Info, "%s,        %s", GetDriver()->GetNodeString( GetNodeId() ).c_str(), cc->GetCommandClassName().c_str() );
			}
 		}

		if( ( endPoint == 1 ) && m_endPointsAreSameClass )
		{
			Log::Write( LogLevel_Info, "%s, All endpoints in this device are the same as endpoint 1.  Searching for the other endpoints...", GetDriver()->GetNodeString( GetNodeId() ).c_str() );
	
			// All end points have the same command classes.
			// We just need to find them...
			char str[128];
			snprintf( str, sizeof( str ), "MultiChannelCmd_EndPointFind for generic device class 0x%.2x", _data[2] );
			Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->Append( GetNodeId() );
			msg->Append( 4 );
			msg->Append( GetCommandClassId() );
			msg->Append( MultiChannelCmd_EndPointFind );
			msg->Append( _data[2] );	// Generic device class
			msg->Append( 0xff );		// Any specific device class
			msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
		else
		{
			// Create instances (endpoints) for each command class in the list
			for( set<uint8>::iterator it=m_endPointCommandClasses.begin(); it!=m_endPointCommandClasses.end(); ++it )
			{
				uint8 commandClassId = *it;
				CommandClass* cc = node->GetCommandClass( commandClassId );
				if( cc )
				{	
					cc->SetInstance( endPoint );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelEndPointFindReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelEndPointFindReport
(	
	uint8 const* _data,
	uint32 const _length
)
{
	Log::Write( LogLevel_Info, "%s, Received MultiChannelEndPointFindReport from node %d", GetDriver()->GetNodeString( GetNodeId() ).c_str(), GetNodeId() );
	uint8 numEndPoints = _length - 5;
	for( uint8 i=0; i<numEndPoints; ++i )
	{
		uint8 endPoint = _data[i+4] & 0x7f;

		if( m_endPointsAreSameClass )
		{
			// Use the stored command class list to set up the endpoint.
			if( Node* node = GetNodeUnsafe() )
			{
				for( set<uint8>::iterator it=m_endPointCommandClasses.begin(); it!=m_endPointCommandClasses.end(); ++it )
				{
					uint8 commandClassId = *it;
					CommandClass* cc = node->GetCommandClass( commandClassId );
					if( cc )
					{	
						Log::Write( LogLevel_Info, "%s,    Endpoint %d: Adding %s", GetDriver()->GetNodeString( GetNodeId() ).c_str(), endPoint, cc->GetCommandClassName().c_str() );
						cc->SetInstance( endPoint );
					}
				}
			}
		}
		else
		{
			// Endpoints are different, so request the capabilities
			char str[128];
			snprintf( str, 128, "MultiChannelCmd_CapabilityGet for node %d, endpoint %d", GetNodeId(), endPoint );
			Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( MultiChannelCmd_CapabilityGet );
			msg->Append( endPoint );
			msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
	}

	m_numEndPointsFound += numEndPoints;
	if( !m_endPointsAreSameClass )
	{
		if( _data[1] == 0 )
		{
			// No more reports to follow this one, so we can continue the search.
			if( m_numEndPointsFound < numEndPoints )
			{
				// We have not yet found all the endpoints, so move to the next generic class request
				++m_endPointFindIndex;
				if( c_genericClass[m_endPointFindIndex] > 0 )
				{
					char str[128];
					snprintf( str, 128, "MultiChannelCmd_EndPointFind for generic device class 0x%.2x (%s)", c_genericClass[m_endPointFindIndex], c_genericClassName[m_endPointFindIndex] );
					Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->Append( GetNodeId() );
					msg->Append( 4 );
					msg->Append( GetCommandClassId() );
					msg->Append( MultiChannelCmd_EndPointFind );
					msg->Append( c_genericClass[m_endPointFindIndex] );		// Generic device class
					msg->Append( 0xff );									// Any specific device class
					msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
					GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelEncap>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelEncap
(	
	uint8 const* _data,
	uint32 const _length
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		uint8 endPoint = _data[1] & 0x7f;
		uint8 commandClassId = _data[3];
		if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
		{
			Log::Write( LogLevel_Info, "%s, Received a MultiChannelEncap from node %d, endpoint %d for Command Class %s", GetDriver()->GetNodeString( GetNodeId() ).c_str(), GetNodeId(), endPoint, pCommandClass->GetCommandClassName().c_str() );
			pCommandClass->HandleMsg( &_data[4], _length-4, endPoint );
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::SendEncap>
// Send a message encasulated in a MultiInstance/MultiChannel command
//-----------------------------------------------------------------------------
void MultiInstance::SendEncap
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance,
	uint32 const _requestFlags
)
{
	char str[128];
	Msg* msg;
	if( GetVersion() == 1 )
	{
		// MultiInstance
		snprintf( str, sizeof( str ), "MultiInstanceCmd_Encap (Instance=%d)", _instance );
		msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3+_length );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiInstanceCmd_Encap );
	}
	else
	{
		// MultiChannel
		snprintf( str, sizeof( str ), "MultiChannelCmd_Encap (Instance=%d)", _instance );
		msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 4+_length );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelCmd_Encap );
		msg->Append( 0 );
	}

	msg->Append( _instance );
	for( uint8 i=0; i<_length; ++i )
	{
		msg->Append( _data[i] );
	}

	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
}




