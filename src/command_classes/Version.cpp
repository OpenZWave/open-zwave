//-----------------------------------------------------------------------------
//
//	Version.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_VERSION
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

#include "CommandClasses.h"
#include "Version.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueStore.h"
#include "ValueString.h"

using namespace OpenZWave;

enum VersionCmd
{
	VersionCmd_Get					= 0x11,
	VersionCmd_Report				= 0x12,
	VersionCmd_CommandClassGet		= 0x13,
	VersionCmd_CommandClassReport	= 0x14
};

enum
{
	ValueIndex_Library = 0,
	ValueIndex_Protocol,
	ValueIndex_Application
};


//-----------------------------------------------------------------------------
// <Version::RequestStatic>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void Version::RequestStatic
(
)
{
	Msg* msg = new Msg( "VersionCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( VersionCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Version::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Version::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	bool handled = false;

	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			if (VersionCmd_Report == (VersionCmd)_data[0])
			{
				ValueString* value;

				char library[8];
				snprintf( library, 8, "%d", _data[1] );
				if( value = static_cast<ValueString*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Library ) ) ) )
				{
					value->OnValueChanged( library );
				}

				char protocol[16];
				snprintf( protocol, 6, "%d.%d", _data[2], _data[3] );
				if( value = static_cast<ValueString*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Protocol ) ) ) )
				{
					value->OnValueChanged( protocol );
				}

				char application[16];
				snprintf( application, 6, "%d.%d", _data[4], _data[5] );
				if( value = static_cast<ValueString*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Application ) ) ) )
				{
					value->OnValueChanged( application );
				}

				Log::Write( "Received Version report from node %d: Library=%s, Protocol=%s, Application=%s", GetNodeId(), library, protocol, application );
				handled = true;
			}
			else if (VersionCmd_CommandClassReport == (VersionCmd)_data[0])
			{
				if( CommandClass* pCommandClass = GetNode()->GetCommandClass( _data[1] ) )
				{
					pCommandClass->SetVersion( _data[2] );				
					Log::Write( "Received Command Class Version report from node %d: CommandClass=%s, Version=%d", GetNodeId(), pCommandClass->GetCommandClassName().c_str(), _data[2] );
				}

				handled = true;
			}

			node->ReleaseValueStore();
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <Version::RequestCommandClassVersion>
// Request the version of a command class used by the device 
//-----------------------------------------------------------------------------
void Version::RequestCommandClassVersion
(
	uint8 const _commandClassId
)
{
	Msg* msg = new Msg( "VersionCmd_CommandClassGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( VersionCmd_CommandClassGet );
	msg->Append( _commandClassId );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Version::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Version::CreateVars
(
	uint8 const _instance
)
{
	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			Value* value;
			
			value = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Library, Value::Genre_System, "Library Version", true, "0.0"  );
			store->AddValue( value );
			value->Release();

			value = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Protocol, Value::Genre_System, "Protocol Version", true, "0.0"  );
			store->AddValue( value );
			value->Release();

			value = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Application, Value::Genre_System, "Application Version", true, "0.0"  );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}
