//-----------------------------------------------------------------------------
//
//	MeterPulse.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_METER_PULSE
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
#include "MeterPulse.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueInt.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum MeterPulseCmd
{
    MeterPulseCmd_Get		= 0x04,
    MeterPulseCmd_Report	= 0x05
};


//-----------------------------------------------------------------------------
// <MeterPulse::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void MeterPulse::RequestState
(
)
{
	Log::Write( "Requesting the meter pulse count from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "MeterPulseCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( MeterPulseCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <MeterPulse::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MeterPulse::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if( MeterPulseCmd_Report == (MeterPulseCmd)_pData[0] )
    {
 		m_count = 0;
		for( uint8 i=0; i<4; ++i )
		{
			m_count <<= 8;
			m_count |= (uint32)_pData[i+1];
		}

		Log::Write( "Received a meter pulse count from node %d: Count=%d", GetNodeId(), m_count );
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// <MeterPulse::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void MeterPulse::CreateVars
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
			Value* pValue = new ValueInt( GetNodeId(), GetCommandClassId(), _instance, 0, "Count", true, 0 );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}



