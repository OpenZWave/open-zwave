//-----------------------------------------------------------------------------
//
//	SensorBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_BINARY
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
#include "SensorBinary.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueBool.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum SensorBinaryCmd
{
    SensorBinaryCmd_Get		= 0x02,
    SensorBinaryCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <SensorBinary::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void SensorBinary::RequestState
(
)
{
    Msg* pMsg = new Msg( "SensorBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( SensorBinaryCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <SensorBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SensorBinary::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if (SensorBinaryCmd_Report == (SensorBinaryCmd)_pData[0])
	{
		Node* pNode = GetNode();
		if( pNode )
		{
			ValueStore* pStore = pNode->GetValueStore();
			if( pStore )
			{
				if( ValueBool* pValue = static_cast<ValueBool*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					pValue->OnValueChanged( _pData[1] != 0 );
				}
				pNode->ReleaseValueStore();

				Log::Write( "Received SensorBinary report from node %d: State=%s", GetNodeId(), _pData[1] ? "On" : "Off" );
				return true;
			}
		}
	}

    return false;
}

//-----------------------------------------------------------------------------
// <SensorBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SensorBinary::CreateVars
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
			Value* pValue = new ValueBool( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Sensor", true, false );
			pStore->AddValue( pValue );
			pValue->Release();

			pNode->ReleaseValueStore();
		}
	}
}







