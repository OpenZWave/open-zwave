//-----------------------------------------------------------------------------
//
//	Basic.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BASIC
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
#include "Basic.h"
#include "Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Notification.h"
#include "Log.h"

#include "ValueByte.h"
#include "NoOperation.h"

using namespace OpenZWave;

enum BasicCmd
{
	BasicCmd_Set	= 0x01,
	BasicCmd_Get	= 0x02,
	BasicCmd_Report	= 0x03
};

//-----------------------------------------------------------------------------
// <Basic::Basic>
// Constructor
//-----------------------------------------------------------------------------
Basic::Basic
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_mapping( 0 ),
	m_setAsReport( false )
{
}

//-----------------------------------------------------------------------------
// <Basic::ReadXML>
// Read configuration.
//-----------------------------------------------------------------------------
void Basic::ReadXML
( 
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	int32 intVal;
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "mapping", &intVal ) )
	{
		if( intVal < 256 && intVal != 0 )
		{
			SetMapping( (uint8)intVal, false );
		}
	}

	char const* str = _ccElement->Attribute("setasreport");
	if( str )
	{
		m_setAsReport = !strcmp( str, "true");
	}
}

//-----------------------------------------------------------------------------
// <Basic::WriteXML>
// Save changed configuration
//-----------------------------------------------------------------------------
void Basic::WriteXML
( 
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );

	char str[32];
	if( m_mapping != 0 )
	{
		snprintf( str, sizeof(str), "%d", m_mapping );
		_ccElement->SetAttribute( "mapping", str );
	}

	if( m_setAsReport )
	{
		_ccElement->SetAttribute( "setasreport", "true" );
	}
}

//-----------------------------------------------------------------------------
// <Basic::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Basic::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Basic::RequestValue>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Basic::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	Msg* msg = new Msg( "BasicCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( BasicCmd_Get );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <Basic::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Basic::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( BasicCmd_Report == (BasicCmd)_data[0] )
	{
		// Level
		Log::Write( LogLevel_Info, GetNodeId(), "Received Basic report from node %d: level=%d", GetNodeId(), _data[1] );
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, 0 ) ) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		}
		UpdateMappedClass( _instance, m_mapping, _data[1] );
		return true;
	}

	if( BasicCmd_Set == (BasicCmd)_data[0] )
	{
		if( m_setAsReport )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Basic set from node %d: level=%d. Treating it as a Basic report.", GetNodeId(), _data[1] );
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, 0 ) ) )
			{
				value->OnValueRefreshed( _data[1] );
				value->Release();
			}
		}
		else
		{
			// Commmand received from the node.  Handle as a notification event
			Log::Write( LogLevel_Info, GetNodeId(), "Received Basic set from node %d: level=%d.  Sending event notification.", GetNodeId(), _data[1] );

			Notification* notification = new Notification( Notification::Type_NodeEvent );
			notification->SetHomeNodeIdAndInstance( GetHomeId(), GetNodeId(), _instance );
			notification->SetEvent( _data[1] );
			GetDriver()->QueueNotification( notification );
		}
		UpdateMappedClass( _instance, m_mapping, _data[1] );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Basic::SetValue>
// Set a value on the Z-Wave device
//-----------------------------------------------------------------------------
bool Basic::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Byte == _value.GetID().GetType() )
	{
		ValueByte const* value = static_cast<ValueByte const*>(&_value);
	
		Log::Write( LogLevel_Info, GetNodeId(), "Basic::Set - Setting node %d to level %d", GetNodeId(), value->GetValue() );
		Msg* msg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( BasicCmd_Set );
		msg->Append( value->GetValue() );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Basic::SetValueBasic>
// Set a value from another class (mapping)
//-----------------------------------------------------------------------------
void Basic::SetValueBasic
(
	uint8 const _instance,
	uint8 const _value
)
{
	// Just update local value. Device should reflect this value.
	if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, 0 ) ) )
	{
		value->OnValueRefreshed( _value );
		value->Release();
	}
}

//-----------------------------------------------------------------------------
// <Basic::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Basic::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueByte( ValueID::ValueGenre_Basic, GetCommandClassId(), _instance, 0, "Basic", "", false, false, 0, 0 );
	}
}

//-----------------------------------------------------------------------------
// <Basic::Set>
// Helper method to set the level
//-----------------------------------------------------------------------------
void Basic::Set
(
	uint8 const _level
)
{
	// This may look like a long winded way to do this, but
	// it ensures that all the proper notifications get sent.
	if( ValueByte* value = static_cast<ValueByte*>( GetValue( 1, 0 ) ) )
	{
		value->Set( _level );
		value->Release();
	}
}

//-----------------------------------------------------------------------------
// <Basic::SetMapping>
// Map COMMAND_CLASS_BASIC messages to another command class
//-----------------------------------------------------------------------------
bool Basic::SetMapping
(
	uint8 const _commandClassId,
	bool const _doLog
)
{
	bool res = false;

	if( _commandClassId != NoOperation::StaticGetCommandClassId() )
	{
		if( Node const* node = GetNodeUnsafe() )
		{
			if( CommandClass* cc = node->GetCommandClass( _commandClassId ) )
			{
				if( _doLog )
				{
					Log::Write( LogLevel_Info, GetNodeId(), "    COMMAND_CLASS_BASIC will be mapped to %s", cc->GetCommandClassName().c_str() );
				}
				cc->SetBasicMapped( true );
				m_mapping = _commandClassId;
				res = true;
			}
		}
	}

	if( _doLog )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "    COMMAND_CLASS_BASIC is not mapped" );
	}
	return res;
}
