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

static enum WakeUpCmd
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
	m_bAwake( false )
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
    Msg* pMsg = new Msg( "WakeUpCmd_IntervalGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( WakeUpCmd_IntervalGet );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <WakeUp::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool WakeUp::HandleMsg
(
	uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if( WakeUpCmd_IntervalReport == (WakeUpCmd)_pData[0] )
	{	
		m_interval = ((uint32)_pData[1]) << 16;
		m_interval |= (((uint32)_pData[2]) << 8);
		m_interval |= (uint32)_pData[3];

		// Report the new interval to xPL
		Log::Write( "Received Wakeup Interval report from node %d: Interval=%d", GetNodeId(), m_interval );

		return true;
	}

	if( WakeUpCmd_Notification == (WakeUpCmd)_pData[0] )
	{	
		// The device is awake.
		Log::Write( "Received Wakeup Notification from node %d", GetNodeId() );
		
		// If the device is marked for polling, request the current state
		if( Node* pNode = GetNode() )
		{
			if( pNode->IsPolled() )
			{
				pNode->RequestState();
			}
		}
		
		// Send all pending messages
		SendPending();
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <WakeUp::Set>
// Set the device's wakeup interval
//-----------------------------------------------------------------------------
void WakeUp::Set
(
	uint32 const _interval
)
{
	Msg* pMsg = new Msg( "Wakeup Interval Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );

	pMsg->Append( GetNodeId() );
	
	if( GetNode()->GetCommandClass( MultiCmd::StaticGetCommandClassId() ) )
	{
		pMsg->Append( 10 );
		pMsg->Append( MultiCmd::StaticGetCommandClassId() );
		pMsg->Append( MultiCmd::MultiCmdCmd_Encap );
		pMsg->Append( 1 );
	}

	pMsg->Append( 6 );
	pMsg->Append( GetCommandClassId() );
	pMsg->Append( WakeUpCmd_IntervalSet );
	pMsg->Append( (uint8)(( _interval >> 16 ) & 0xff) ); 
	pMsg->Append( (uint8)(( _interval >> 8 ) & 0xff) );	 
	pMsg->Append( (uint8)( _interval & 0xff ) );		
	pMsg->Append( GetNodeId() );
	pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <WakeUp::QueueMsg>
// Add a Z-Wave message to the queue
//-----------------------------------------------------------------------------
void WakeUp::QueueMsg
(
	Msg* _pMsg
)
{
	m_mutex.Lock();
	m_pendingQueue.push_back( _pMsg );
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
	m_bAwake = true;

	m_mutex.Lock();

	list<Msg*>::iterator it = m_pendingQueue.begin();
	while( it != m_pendingQueue.end() )
	{	
		Msg* pMsg = *it;
		Driver::Get()->SendMsg( pMsg );
		it = m_pendingQueue.erase( it );
	}

	m_mutex.Release();

	//// Send the device back to sleep
	//Msg* pMsg = new Msg( "Wakeup - No More Information", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	//pMsg->Append( GetNodeId() );
	//pMsg->Append( 2 );
	//pMsg->Append( GetCommandClassId() );
	//pMsg->Append( WakeUpCmd_NoMoreInformation );
	//pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	//Driver::Get()->SendMsg( pMsg ); 
	
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
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			Value* pValue = new ValueInt( GetNodeId(), GetCommandClassId(), _instance, 0, "Wake-up Interval", false, 0 );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}

