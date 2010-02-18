//-----------------------------------------------------------------------------
//
//	Indicator.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_INDICATOR
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
#include "Indicator.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueBool.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum IndicatorCmd
{
	IndicatorCmd_Set	= 0x01,
	IndicatorCmd_Get	= 0x02,
	IndicatorCmd_Report	= 0x03
};


//-----------------------------------------------------------------------------
// <Indicator::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void Indicator::RequestState
(
)
{
	Log::Write( "Requesting the indicator from node %d", GetNodeId() );
	Msg* msg = new Msg( "IndicatorCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( IndicatorCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Indicator::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Indicator::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if( IndicatorCmd_Report == (IndicatorCmd)_data[0] )
	{
		Node* node = GetNode();
		if( node )
		{
			ValueStore* store = node->GetValueStore();
			if( store )
			{
				if( ValueBool* value = static_cast<ValueBool*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					value->OnValueChanged( _data[1] != 0 );
				}
				node->ReleaseValueStore();

				Log::Write( "Received an Indicator report from node %d: Indicator=%d", GetNodeId(), _data[1] );
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Indicator::SetValue>
// Set the device's indicator value 
//-----------------------------------------------------------------------------
bool Indicator::SetValue
(
	Value const& _value
)
{
	if( ValueBool const* value = static_cast<ValueBool const*>(&_value) )
	{
		Log::Write( "Indicator::SetValue - Setting indicator on node %d to %s", GetNodeId(), value->GetPending() ? "On" : "Off" );
		Msg* msg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( IndicatorCmd_Set );
		msg->Append( value->GetPending() ? 0xff: 0x00 );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( msg );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Indicator::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Indicator::CreateVars
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
			Value* value = new ValueBool( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_User, "Indicator", false, false );
			store->AddValue( value );
			value->Release();
		}
	}
}


