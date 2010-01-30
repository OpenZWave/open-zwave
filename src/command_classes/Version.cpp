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

static enum VersionCmd
{
    VersionCmd_Get					= 0x11,
    VersionCmd_Report				= 0x12,
    VersionCmd_CommandClassGet		= 0x13,
    VersionCmd_CommandClassReport	= 0x14
};

static enum
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
    Msg* pMsg = new Msg( "VersionCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( VersionCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Version::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Version::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			if (VersionCmd_Report == (VersionCmd)_pData[0])
			{
				ValueString* pValue;

				char library[8];
				snprintf( library, 8, "%d", _pData[1] );
				if( pValue = static_cast<ValueString*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Library ) ) ) )
				{
					pValue->OnValueChanged( library );
				}

				char protocol[16];
				snprintf( protocol, 6, "%d.%d", _pData[2], _pData[3] );
				if( pValue = static_cast<ValueString*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Protocol ) ) ) )
				{
					pValue->OnValueChanged( protocol );
				}

				char application[16];
				snprintf( application, 6, "%d.%d", _pData[4], _pData[5] );
				if( pValue = static_cast<ValueString*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Application ) ) ) )
				{
					pValue->OnValueChanged( application );
				}

				Log::Write( "Received Version report from node %d: Library=%s, Protocol=%s, Application=%s", GetNodeId(), library, protocol, application );
				return true;
			}

			if (VersionCmd_CommandClassReport == (VersionCmd)_pData[0])
			{
				if( CommandClass* pCommandClass = GetNode()->GetCommandClass( _pData[1] ) )
				{
					pCommandClass->SetVersion( _pData[2] );				
					Log::Write( "Received Command Class Version report from node %d: CommandClass=%s, Version=%d", GetNodeId(), pCommandClass->GetCommandClassName().c_str(), _pData[2] );
				}

				return true;
			}
		}
	}

	return false;
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
    Msg* pMsg = new Msg( "VersionCmd_CommandClassGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( VersionCmd_CommandClassGet );
    pMsg->Append( _commandClassId );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
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
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			Value* pValue;
			
			pValue = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Library, "Library Version", true, "0.0"  );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Protocol, "Protocol Version", true, "0.0"  );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Application, "Application Version", true, "0.0"  );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}
