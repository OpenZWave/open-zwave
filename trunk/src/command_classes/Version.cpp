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
#include "Version.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

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
// <Version::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void Version::RequestState
(
	uint32 const _requestFlags
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		Msg* msg = new Msg( "VersionCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( VersionCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <Version::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Version::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( VersionCmd_Report == (VersionCmd)_data[0] )
	{
		char library[8];
		char protocol[16];
		char application[16];

		snprintf( library, sizeof(library), "%d", _data[1] );
		snprintf( protocol, sizeof(protocol), "%d.%d", _data[2], _data[3] );
		snprintf( application, sizeof(application), "%d.%d", _data[4], _data[5] );

		Log::Write( "Received Version report from node %d: Library=%s, Protocol=%s, Application=%s", GetNodeId(), library, protocol, application );
		ClearStaticRequest( StaticRequest_Values );

		if( ValueString* libraryValue = m_library.GetInstance( _instance ) )
		{
			libraryValue->OnValueChanged( library );
		}
		if( ValueString* protocolValue = m_protocol.GetInstance( _instance ) )
		{
			protocolValue->OnValueChanged( protocol );
		}
		if( ValueString* applicationValue = m_application.GetInstance( _instance ) )
		{
			applicationValue->OnValueChanged( application );
		}

		return true;
	}
	
	if (VersionCmd_CommandClassReport == (VersionCmd)_data[0])
	{
		if( Node* node = GetNode() )
		{
			if( CommandClass* pCommandClass = node->GetCommandClass( _data[1] ) )
			{
				Log::Write( "Received Command Class Version report from node %d: CommandClass=%s, Version=%d", GetNodeId(), pCommandClass->GetCommandClassName().c_str(), _data[2] );
				pCommandClass->ClearStaticRequest( StaticRequest_Version );
				pCommandClass->SetVersion( _data[2] );
			}

			ReleaseNode();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Version::RequestCommandClassVersion>
// Request the version of a command class used by the device 
//-----------------------------------------------------------------------------
void Version::RequestCommandClassVersion
(
	CommandClass const* _commandClass
)
{
	Msg* msg = new Msg( "VersionCmd_CommandClassGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( VersionCmd_CommandClassGet );
	msg->Append( _commandClass->GetCommandClassId() );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
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
	if( Node* node = GetNode() )
	{
		m_library.AddInstance( _instance, node->CreateValueString( ValueID::ValueGenre_System, GetCommandClassId(), _instance, (uint8)ValueIndex_Library, "Library Version", "", true, "Unknown" ) );
		m_protocol.AddInstance( _instance, node->CreateValueString( ValueID::ValueGenre_System, GetCommandClassId(), _instance, (uint8)ValueIndex_Protocol, "Protocol Version", "", true, "Unknown" ) );
		m_application.AddInstance( _instance, node->CreateValueString( ValueID::ValueGenre_System, GetCommandClassId(), _instance, (uint8)ValueIndex_Application, "Application Version", "", true, "Unknown" ) );
		ReleaseNode();
	}
}
