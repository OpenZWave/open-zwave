//-----------------------------------------------------------------------------
//
//	Battery.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BATTERY
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
#include "Battery.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum BatteryCmd
{
    BatteryCmd_Get		= 0x02,
    BatteryCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <Battery::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void Battery::RequestState
(
)
{
	Log::Write( "Requesting the battery level from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "BatteryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( BatteryCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Battery::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Battery::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if (BatteryCmd_Report == (BatteryCmd)_pData[0])
    {
        // We have received a battery level report from the Z-Wave device.
		// Devices send 0xff instead of zero for a low battery warning.
		uint8 batteryLevel = _pData[1];
		if( batteryLevel == 0xff )
		{
			batteryLevel = 0;
		}

		Node* pNode = GetNode();
		if( pNode )
		{
			ValueStore* pStore = pNode->GetValueStore();
			if( pStore )
			{
				if( ValueByte* pValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					pValue->OnValueChanged( batteryLevel );
				}

				Log::Write( "Received Battery report from node %d: level=%d", GetNodeId(), batteryLevel );
				return true;
			}
		}
    }
    return false;
}

//-----------------------------------------------------------------------------
// <Battery::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Battery::CreateVars
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
			Value* pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 0, "Battery Level", true, 100 );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}


