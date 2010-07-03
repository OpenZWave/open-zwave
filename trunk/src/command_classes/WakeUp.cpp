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
#include "WakeUp.h"
#include "MultiCmd.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueInt.h"

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
	uint32 const _homeId,
	uint8 const _nodeId 
):
	CommandClass( _homeId, _nodeId ), 
	m_awake( true ),
	m_pollRequired( false ),
	m_init( false ),
	m_notification( false )
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
// <WakeUp::Init>
// Starts the process of requesting node state from a sleeping device
//-----------------------------------------------------------------------------
void WakeUp::Init
( 
)
{
	// Request the wake up interval.  When we receive the response, we
	// can send a set interval message with the same interval, but with
	// the target node id set to that of the controller.  This will ensure
	// that the controller will receive the wake-up notifications from
	// the device.  Once this is done, we can request the rest of the node
	// state.
	m_init = true;
	RequestState( CommandClass::RequestFlag_Session );
}

//-----------------------------------------------------------------------------
// <WakeUp::RequestState>
// Nothing to do for wakeup
//-----------------------------------------------------------------------------
void WakeUp::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Session )
	{
		// We won't get a response until the device next wakes up
		Msg* msg = new Msg( "WakeUpCmd_IntervalGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_IntervalGet );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <WakeUp::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool WakeUp::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( WakeUpCmd_IntervalReport == (WakeUpCmd)_data[0] )
	{	
		if( ValueInt* value = m_interval.GetInstance( _instance ) )
		{
			uint32 interval = ((uint32)_data[1]) << 16;
			interval |= (((uint32)_data[2]) << 8);
			interval |= (uint32)_data[3];

			uint8 targetNodeId = _data[4];

			Log::Write( "Received Wakeup Interval report from node %d: Interval=%d, Target Node=%d", GetNodeId(), interval, targetNodeId );

			value->OnValueChanged( (int32)interval );
		
			// Ensure that the target node for wake-up notifications is the controller
			if( GetDriver()->GetNodeId() != targetNodeId )
			{
				SetValue( *value );	
			}

			// If we are in init mode, now is the time to request the rest of the node state
			if( m_init )
			{
				if( Node* node = GetNode() )
				{
					node->RequestEntireNodeState();
					ReleaseNode();
				}
				m_init = false;
			}
		}
		return true;
	}
	else if( WakeUpCmd_Notification == (WakeUpCmd)_data[0] )
	{	
		// The device is awake.
		Log::Write( "Received Wakeup Notification from node %d", GetNodeId() );
		m_notification = true;
		SetAwake( true );				
		return true;
	}

	return false;
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
	if( ValueID::ValueType_Int == _value.GetID().GetType() )
	{
		ValueInt const* value = static_cast<ValueInt const*>(&_value);
	
		Msg* msg = new Msg( "Wakeup Interval Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		
		if( GetNode()->GetCommandClass( MultiCmd::StaticGetCommandClassId() ) )
		{
			ReleaseNode();
			msg->Append( 10 );
			msg->Append( MultiCmd::StaticGetCommandClassId() );
			msg->Append( MultiCmd::MultiCmdCmd_Encap );
			msg->Append( 1 );
		}

		int32 interval = value->GetValue();

		msg->Append( 6 );
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_IntervalSet );
		msg->Append( (uint8)(( interval >> 16 ) & 0xff) ); 
		msg->Append( (uint8)(( interval >> 8 ) & 0xff) );	 
		msg->Append( (uint8)( interval & 0xff ) );		
		msg->Append( GetDriver()->GetNodeId() );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
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
		Log::Write( "Node %d has been marked as %s", GetNodeId(), m_awake ? "awake" : "asleep" );
	}

	if( m_awake )
	{
		// If the device is marked for polling, request the current state
		if( m_pollRequired )
		{
			if( Node* node = GetNode() )
			{
				node->RequestState( RequestFlag_Dynamic );
				ReleaseNode();
			}
			m_pollRequired = false;
		}
			
		// Send all pending messages
		SendPending();
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

	// See if there is already a copy of this message in the queue.  If so, 
	// we delete it.  This is to prevent duplicates building up if the 
	// device does not wake up very often.  Deleting the original and
	// adding the copy to the end avoids problems with the order of
	// commands such as on and off.
	for( list<Msg*>::iterator it = m_pendingQueue.begin(); it != m_pendingQueue.end(); ++it )
	{
		if( *(*it) == *_msg )
		{
			// Duplicate found
			delete *it;
			m_pendingQueue.erase( it );
		}
	}
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
		GetDriver()->SendMsg( msg );
		it = m_pendingQueue.erase( it );
	}

	m_mutex.Release();

	// Send the device back to sleep
	if( m_notification )
	{
		m_notification = false;
		Msg* msg = new Msg( "Wakeup - No More Information", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_NoMoreInformation );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
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
	if( Node* node = GetNode() )
	{
		m_interval.AddInstance( _instance, node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 0, "Wake-up Interval", "Seconds", false, 3600 ) );
		ReleaseNode();
	}
}

