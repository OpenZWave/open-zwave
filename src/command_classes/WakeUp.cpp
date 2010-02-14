//-----------------------------------------------------------------------------
//
//	WakeUp.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_WAKE_UP
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
#include "WakeUp.h"
#include "MultiCmd.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueInt.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum WakeUpCmd
{
	WakeUpCmd_IntervalSet		= 0x04,
	WakeUpCmd_IntervalGet		= 0x05,
	WakeUpCmd_IntervalReport	= 0x06,
	WakeUpCmd_Notification		= 0x07,
	WakeUpCmd_NoMoreInformation	= 0x08
};


//-----------------------------------------------------------------------------
// <WakeUp::WakeUp>
// Constructor
//-----------------------------------------------------------------------------
WakeUp::WakeUp
( 
	uint8 const _nodeId 
):
	CommandClass( _nodeId ), 
	m_awake( true ),
	m_pollRequired( false )
{
}

//-----------------------------------------------------------------------------
// <WakeUp::~WakeUp>
// Destructor
//-----------------------------------------------------------------------------
WakeUp::~WakeUp
( 
)
{
}

//-----------------------------------------------------------------------------
// <WakeUp::RequestState>
// Nothing to do for wakeup
//-----------------------------------------------------------------------------
void WakeUp::RequestState
(
)
{
	// We won't get a response until the device next wakes up
	Msg* msg = new Msg( "WakeUpCmd_IntervalGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( WakeUpCmd_IntervalGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <WakeUp::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool WakeUp::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	bool handled = false;

	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			if( WakeUpCmd_IntervalReport == (WakeUpCmd)_data[0] )
			{	
				uint32 interval = ((uint32)_data[1]) << 16;
				interval |= (((uint32)_data[2]) << 8);
				interval |= (uint32)_data[3];

				if( ValueInt* value = static_cast<ValueInt*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					value->OnValueChanged( (int32)interval );
				}

				Log::Write( "Received Wakeup Interval report from node %d: Interval=%d", GetNodeId(), interval );
				handled = true;
			}
			else if( WakeUpCmd_Notification == (WakeUpCmd)_data[0] )
			{	
				// The device is awake.
				Log::Write( "Received Wakeup Notification from node %d", GetNodeId() );
				SetAwake( true );				
				handled = true;
			}

			node->ReleaseValueStore();
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <WakeUp::SetValue>
// Set the device's wakeup interval
//-----------------------------------------------------------------------------
bool WakeUp::SetValue
(
	Value const& _value
)
{
	if( ValueInt const* value = static_cast<ValueInt const*>(&_value) )
	{
		Msg* msg = new Msg( "Wakeup Interval Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );

		msg->Append( GetNodeId() );
		
		if( GetNode()->GetCommandClass( MultiCmd::StaticGetCommandClassId() ) )
		{
			msg->Append( 10 );
			msg->Append( MultiCmd::StaticGetCommandClassId() );
			msg->Append( MultiCmd::MultiCmdCmd_Encap );
			msg->Append( 1 );
		}

		int32 interval = value->GetPending();

		msg->Append( 6 );
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_IntervalSet );
		msg->Append( (uint8)(( interval >> 16 ) & 0xff) ); 
		msg->Append( (uint8)(( interval >> 8 ) & 0xff) );	 
		msg->Append( (uint8)( interval & 0xff ) );		
		msg->Append( GetNodeId() );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( msg );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <WakeUp::SetAwake>
// Set whether the device is likely to be awake
//-----------------------------------------------------------------------------
void WakeUp::SetAwake
(
	bool _state
)
{
	if( m_awake != _state )
	{
		m_awake = _state;
		if( m_awake )
		{
			// Device has woken up

			// If the device is marked for polling, request the current state
			if( m_pollRequired )
			{
				if( Node* node = GetNode() )
				{
					node->RequestState();
				}
				m_pollRequired = false;
			}
				
			// Send all pending messages
			SendPending();
		}
	}
}

//-----------------------------------------------------------------------------
// <WakeUp::QueueMsg>
// Add a Z-Wave message to the queue
//-----------------------------------------------------------------------------
void WakeUp::QueueMsg
(
	Msg* _msg
)
{
	m_mutex.Lock();
	m_pendingQueue.push_back( _msg );
	m_mutex.Release();
}

//-----------------------------------------------------------------------------
// <WakeUp::SendPending>
// The device is awake, so send all the pending messages
//-----------------------------------------------------------------------------
void WakeUp::SendPending
(
)
{
	m_awake = true;

	m_mutex.Lock();

	list<Msg*>::iterator it = m_pendingQueue.begin();
	while( it != m_pendingQueue.end() )
	{	
		Msg* msg = *it;
		Driver::Get()->SendMsg( msg );
		it = m_pendingQueue.erase( it );
	}

	m_mutex.Release();

	//// Send the device back to sleep
	//Msg* msg = new Msg( "Wakeup - No More Information", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	//msg->Append( GetNodeId() );
	//msg->Append( 2 );
	//msg->Append( GetCommandClassId() );
	//msg->Append( WakeUpCmd_NoMoreInformation );
	//msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	//Driver::Get()->SendMsg( msg ); 
	
	//m_bAwake = false;
}

//-----------------------------------------------------------------------------
// <WakeUp::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void WakeUp::CreateVars
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
			Value* value = new ValueInt( GetNodeId(), GetCommandClassId(), _instance, 0, Value::Genre_System, "Wake-up Interval", false, 0 );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}

