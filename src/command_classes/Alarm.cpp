//-----------------------------------------------------------------------------
//
//	Alarm.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ALARM
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
#include "Alarm.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum AlarmCmd
{
    AlarmCmd_Get	= 0x04,
    AlarmCmd_Report = 0x05
};

static enum
{
    ValueIndex_Type	= 0,
    ValueIndex_Level
};


//-----------------------------------------------------------------------------
// <Alarm::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void Alarm::RequestState
(
)
{
	Log::Write( "Requesting the alarm status from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( AlarmCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Alarm::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Alarm::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if (AlarmCmd_Report == (AlarmCmd)_pData[0])
    {
        // We have received a report from the Z-Wave device
		// No known mappings for these values yet
		if( Node* pNode = GetNode() )
		{
			if( ValueStore* pStore = pNode->GetValueStore() )
			{
				ValueByte* pValue;

				// Alarm Type
				if( pValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Type ) ) ) )
				{
					pValue->OnValueChanged( _pData[1] );
				}
		
				// Alarm Level
				if( pValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Level ) ) ) )
				{
					pValue->OnValueChanged( _pData[2] );
				}

				Log::Write( "Received Alarm report from node %d: type=%d, level=%d", GetNodeId(), _pData[1], _pData[2] );
			}
		}

        return true;
	}

    return false;
}

//-----------------------------------------------------------------------------
// <Alarm::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Alarm::CreateVars
(
	uint8 const _instance
)
{
	Node* pNode = GetNode();
	if( pNode )
	{
		if( ValueStore* pStore = pNode->GetValueStore() )
		{
			Value* pValue;
		
			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Type, "Alarm Type", true, 0 );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Level, "Alarm Level", true, 0 );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}

