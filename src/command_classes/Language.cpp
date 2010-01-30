//-----------------------------------------------------------------------------
//
//	Language.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_LANGUAGE
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
#include "Language.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueStore.h"
#include "ValueString.h"

using namespace OpenZWave;

static enum LanguageCmd
{
    LanguageCmd_Set		= 0x01,
    LanguageCmd_Get		= 0x02,
    LanguageCmd_Report	= 0x03
};

static enum
{
    ValueIndex_Language	= 0,
    ValueIndex_Country
};

//-----------------------------------------------------------------------------
// <Language::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void Language::RequestState
(
)
{
	Log::Write( "Requesting the language from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "LanguageCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( LanguageCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Language::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Language::HandleMsg
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
			if( LanguageCmd_Report == (LanguageCmd)_pData[0] )
			{
				char language[4];
				char country[3];

				language[0] = _pData[1];
				language[1] = _pData[2];
				language[2] = _pData[3];
				language[3] = 0;

				country[0] = _pData[4];
				country[1] = _pData[5];
				country[2] = 0;
				
				ValueString* pValue;

				if( pValue = static_cast<ValueString*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Language ) ) ) )
				{
					pValue->OnValueChanged( language );
				}

				if( pValue = static_cast<ValueString*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Country ) ) ) )
				{
					pValue->OnValueChanged( country );
				}

				Log::Write( "Received Language report from node %d: Language=%s, Country=%s", GetNodeId(), language, country );
				return true;
			}
		}
	}

    return false;
}

//-----------------------------------------------------------------------------
// <Language::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Language::CreateVars
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
			
			pValue = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Language, "Language", false, ""  );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueString( GetNodeId(), GetCommandClassId(), _instance, (uint8)ValueIndex_Country, "Country", false, ""  );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}

