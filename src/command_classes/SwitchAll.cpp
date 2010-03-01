//-----------------------------------------------------------------------------
//
//	SwitchAll.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_ALL
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
#include "SwitchAll.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueList.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum SwitchAllCmd
{
	SwitchAllCmd_Set	= 0x01,
	SwitchAllCmd_Get	= 0x02,
	SwitchAllCmd_Report	= 0x03,
	SwitchAllCmd_On		= 0x04,
	SwitchAllCmd_Off	= 0x05
};

static char* const c_switchAllStateName[] = 
{
	"Disabled",
	"Off Enabled",
	"On Enabled",
	"On and Off Enabled"
};


//-----------------------------------------------------------------------------
// <SwitchAll::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void SwitchAll::RequestState
(
	bool const _poll
)
{
	if( !_poll )
	{
		Msg* msg = new Msg( "SwitchAllCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchAllCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <SwitchAll::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchAll::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if (SwitchAllCmd_Report == (SwitchAllCmd)_data[0])
	{
		Node* node = GetNode();
		if( node )
		{
			ValueStore* store = node->GetValueStore();
			if( store )
			{
				if( ValueList* value = static_cast<ValueList*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					int32 idx = value->GetItemIdxByValue( (int32)_data[1] );
					if( idx >= 0 )
					{
						value->OnValueChanged( idx );
						Log::Write( "Received SwitchAll report from node %d: %s", GetNodeId(), value->GetItem().m_label.c_str() );
					}
				}
				node->ReleaseValueStore();
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchAll::SetValue>
// Set the device's response to SWITCH_ALL commands 
//-----------------------------------------------------------------------------
bool SwitchAll::SetValue
(
	Value const& _value
)
{
	if( ValueList const* value = static_cast<ValueList const*>(&_value) )
	{
		ValueList::Item const& item = value->GetPending();

		Log::Write( "SwitchAll::Set - %s on node %d", value->GetPending().m_label.c_str(), GetNodeId() );
		Msg* msg = new Msg( "SwitchAllCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchAllCmd_Set );
		msg->Append( (uint8)item.m_value );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( msg );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchAll::Off>
// Send a command to switch all devices off 
//-----------------------------------------------------------------------------
void SwitchAll::Off
(
)
{
	Log::Write( "SwitchAll::Off (Node=%d)", GetNodeId() );
	Msg* msg = new Msg( "SwitchAllCmd_Off", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchAllCmd_Off );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchAll::On>
// Set the device's response to SWITCH_ALL commands 
//-----------------------------------------------------------------------------
void SwitchAll::On
(
)
{
	Log::Write( "SwitchAll::On (Node=%d)", GetNodeId() );
	Msg* msg = new Msg( "SwitchAllCmd_On", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchAllCmd_On );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
}

//-----------------------------------------------------------------------------
// <SwitchAll::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchAll::CreateVars
(
	uint8 const _instance
)
{
	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			vector<ValueList::Item> items;
			for( int i=0; i<4; ++i )
			{	
				ValueList::Item item;
				item.m_label = c_switchAllStateName[i];
				item.m_value = (i==3) ? 0x000000ff : i;
				items.push_back( item ); 
			}
			Value* value = new ValueList( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_System, "Switch All", false, items, 0 );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}



