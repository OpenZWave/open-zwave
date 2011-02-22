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

using namespace OpenZWave;

enum SwitchAllCmd
{
	SwitchAllCmd_Set	= 0x01,
	SwitchAllCmd_Get	= 0x02,
	SwitchAllCmd_Report	= 0x03,
	SwitchAllCmd_On		= 0x04,
	SwitchAllCmd_Off	= 0x05
};

static char const* c_switchAllStateName[] = 
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
bool SwitchAll::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Session )
	{
		RequestValue();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchAll::RequestValue>												   
// Request current value from the device									   
//-----------------------------------------------------------------------------
void SwitchAll::RequestValue
(
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _dummy2		// = 0 (not used)
)
{
	Msg* msg = new Msg( "SwitchAllCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchAllCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <SwitchAll::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchAll::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (SwitchAllCmd_Report == (SwitchAllCmd)_data[0])
	{
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, 0 ) ) )
		{
			Log::Write( "Received SwitchAll report from node %d: %s", GetNodeId(), value->GetItem().m_label.c_str() );
			value->OnValueChanged( (int32)_data[1] );
		}

		return true;
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
	if( ValueID::ValueType_List == _value.GetID().GetType() )
	{
		ValueList const* value = static_cast<ValueList const*>(&_value);
		ValueList::Item const& item = value->GetItem();

		Log::Write( "SwitchAll::Set - %s on node %d", item.m_label.c_str(), GetNodeId() );
		Msg* msg = new Msg( "SwitchAllCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchAllCmd_Set );
		msg->Append( (uint8)item.m_value );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
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
	Driver* _driver,
	uint8 const _nodeId 
)
{
	Log::Write( "SwitchAll::Off (Node=%d)", _nodeId );
	Msg* msg = new Msg( "SwitchAllCmd_Off", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( _nodeId );
	msg->Append( 2 );
	msg->Append( StaticGetCommandClassId() );
	msg->Append( SwitchAllCmd_Off );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    _driver->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <SwitchAll::On>
// Send a command to switch all devices on 
//-----------------------------------------------------------------------------
void SwitchAll::On
(
	Driver* _driver,
	uint8 const _nodeId 
)
{
	Log::Write( "SwitchAll::On (Node=%d)", _nodeId );
	Msg* msg = new Msg( "SwitchAllCmd_On", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( _nodeId );
	msg->Append( 2 );
	msg->Append( StaticGetCommandClassId() );
	msg->Append( SwitchAllCmd_On );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    _driver->SendMsg( msg );
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
	if( Node* node = GetNodeUnsafe() )
	{
		vector<ValueList::Item> items;
		for( int i=0; i<4; ++i )
		{	
			ValueList::Item item;
			item.m_label = c_switchAllStateName[i];
			item.m_value = (i==3) ? 0x000000ff : i;
			items.push_back( item ); 
		}

		node->CreateValueList(  ValueID::ValueGenre_System, GetCommandClassId(), _instance, 0, "Switch All", "", false, items, 0 );
	}
}



