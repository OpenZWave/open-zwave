//-----------------------------------------------------------------------------
//
//	Clock.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CLOCK
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
#include "Clock.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum ClockCmd
{
    ClockCmd_Set	= 0x04,
    ClockCmd_Get	= 0x05,
    ClockCmd_Report	= 0x06
};

static char* const c_dayNames[] = 
{
	"Invalid",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
};


//-----------------------------------------------------------------------------
// <Clock::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void Clock::RequestState
(
)
{
	Log::Write( "Requesting the clock settings from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "ClockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( ClockCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Clock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Clock::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0   
)
{
    if (ClockCmd_Report == (ClockCmd)_pData[0])
    {
		m_day = _pData[1] >> 5;
		m_hour = _pData[1] & 0x1f;
		m_minute = _pData[2];

		Log::Write( "Received Clock report from node %d: %s %.2d:%.2d", GetNodeId(), c_dayNames[m_day], m_hour, m_minute );
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// <Clock::Set>
// Set the device's 
//-----------------------------------------------------------------------------
void Clock::Set
(
	uint8 const _day,		// _day value is 1 to 7 (Monday to Sunday)
	uint8 const _hour,
	uint8 const _minute
)
{
    Msg* pMsg = new Msg( "ClockCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 4 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( ClockCmd_Set );
	pMsg->Append( ( m_day << 5 ) | m_hour );
	pMsg->Append( m_minute );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <Clock::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Clock::CreateVars
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
			
			vector<string> items;
			for( int i=1; i<=7; ++i )
			{	
				items.push_back( c_dayNames[i] ); 
			}
			pValue = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, "Day", false, items, c_dayNames[1] );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 1, "Hour", false, 0 );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 2, "Minute", false, 0 );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}


