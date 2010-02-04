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
		uint8 day = _pData[1] >> 5;
		uint8 hour = _pData[1] & 0x1f;
		uint8 minute = _pData[2];

		Node* pNode = GetNode();
		if( pNode )
		{
			ValueStore* pStore = pNode->GetValueStore();
			if( pStore )
			{
				// Day
				if( ValueList* pValueList = static_cast<ValueList*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					pValueList->OnValueChanged( day-1 );
				}

				// Hour
				ValueByte* pValue;
				if( pValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 1 ) ) ) )
				{
					pValue->OnValueChanged( hour );
				}

				// Minute
				if( pValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 2 ) ) ) )
				{
					pValue->OnValueChanged( minute );
				}

				pNode->ReleaseValueStore();

				Log::Write( "Received Clock report from node %d: %s %.2d:%.2d", GetNodeId(), c_dayNames[day], hour, minute );
				return true;
			}
		}
    }
    return false;
}

//-----------------------------------------------------------------------------
// <Clock::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
bool Clock::SetValue
(
	Value const& _value
)
{
	bool ret = false;

	uint8 instance = _value.GetID().GetInstance();

	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			ValueList const* pDayValue = static_cast<ValueList*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), instance, 0 ) ) );
			ValueByte const* pHourValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), instance, 1 ) ) );
			ValueByte const* pMinuteValue = static_cast<ValueByte*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), instance, 2 ) ) );

			if( pDayValue && pHourValue && pMinuteValue )
			{
				uint8 day = pDayValue->GetItem().m_value;
				uint8 hour = pHourValue->GetValue();
				uint8 minute = pMinuteValue->GetValue();

				switch( _value.GetID().GetIndex() )
				{
					case 0:
					{
						// Day
						day = pDayValue->GetPending().m_value;
						break;
					}
					case 1:
					{
						// Hour
						hour = pHourValue->GetPending();
						break;
					}
					case 2:
					{
						// Minute
						minute = pMinuteValue->GetPending();
						break;
					}
				}

				Msg* pMsg = new Msg( "ClockCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
				pMsg->Append( GetNodeId() );
				pMsg->Append( 4 );
				pMsg->Append( GetCommandClassId() );
				pMsg->Append( ClockCmd_Set );
				pMsg->Append( ( day << 5 ) | hour );
				pMsg->Append( minute );
				pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
				Driver::Get()->SendMsg( pMsg );
				ret = true;
			}

			pNode->ReleaseValueStore();
		}
	}

	return ret;
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
			
			vector<ValueList::Item> items;
			for( int i=1; i<=7; ++i )
			{	
				ValueList::Item item;
				item.m_label = c_dayNames[i];
				item.m_value = i;
				items.push_back( item ); 
			}

			pValue = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Day", false, items, 0 );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 1, Value::Genre_User, "Hour", false, 0 );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, 2, Value::Genre_User, "Minute", false, 0 );
			pStore->AddValue( pValue );
			pValue->Release();

			pNode->ReleaseValueStore();
		}
	}
}


