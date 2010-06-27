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
#include "Clock.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueList.h"

using namespace OpenZWave;

enum ClockCmd
{
	ClockCmd_Set	= 0x04,
	ClockCmd_Get	= 0x05,
	ClockCmd_Report	= 0x06
};

static char const* c_dayNames[] = 
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
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		Msg* msg = new Msg( "ClockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ClockCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <Clock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Clock::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1   
)
{
	if (ClockCmd_Report == (ClockCmd)_data[0])
	{
		uint8 day = _data[1] >> 5;
		uint8 hour = _data[1] & 0x1f;
		uint8 minute = _data[2];

		Log::Write( "Received Clock report from node %d: %s %.2d:%.2d", GetNodeId(), c_dayNames[day], hour, minute );

		m_day.GetInstance( _instance )->OnValueChanged( day );
		m_hour.GetInstance( _instance )->OnValueChanged( hour );
		m_minute.GetInstance( _instance )->OnValueChanged( minute );
		return true;
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

	ValueList const* pDayValue = m_day.GetInstance( instance );
	ValueByte const* pHourValue = m_hour.GetInstance( instance );
	ValueByte const* pMinuteValue = m_minute.GetInstance( instance );

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

		Msg* msg = new Msg( "ClockCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( ClockCmd_Set );
		msg->Append( ( day << 5 ) | hour );
		msg->Append( minute );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
		ret = true;
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
	if( Node* node = GetNode() )
	{
		vector<ValueList::Item> items;
		for( int i=1; i<=7; ++i )
		{	
			ValueList::Item item;
			item.m_label = c_dayNames[i];
			item.m_value = i;
			items.push_back( item ); 
		}

		m_day.AddInstance( _instance, node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Day", "", false, items, 0 ) );
		m_hour.AddInstance( _instance, node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 1, "Hour", "", false, 12 ) );
		m_minute.AddInstance( _instance, node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 2, "Minute", "", false, 0 ) );
		ReleaseNode();
	}
}


