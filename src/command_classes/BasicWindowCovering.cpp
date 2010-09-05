//-----------------------------------------------------------------------------
//
//	BasicWindowCovering.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BASIC_WINDOW_COVERING
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
#include "BasicWindowCovering.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"
#include "ValueButton.h"

using namespace OpenZWave;

enum BasicWindowCoveringCmd
{
	BasicWindowCoveringCmd_StartLevelChange	= 0x01,
	BasicWindowCoveringCmd_StopLevelChange	= 0x02
};

//-----------------------------------------------------------------------------
// <BasicWindowCovering::SetValue>
// Set a value on the Z-Wave device
//-----------------------------------------------------------------------------
bool BasicWindowCovering::SetValue
(
	Value const& _value
)
{
	bool res = false;
	uint8 instance = _value.GetID().GetInstance();

	uint8 action;
	ValueButton const* button = NULL;

	if( _value.GetID().GetIndex() )
	{
		// Close
		action = 0;
		button = m_close.GetInstance( instance );
	}
	else
	{
		// Open
		action = 0x40;
		button = m_open.GetInstance( instance );
	}

	if( button && button->IsPressed() )
	{
		Log::Write( "BasicWindowCovering - Start Level Change (%s) on node %d", action ? "Open" : "Close", GetNodeId() );
		Msg* msg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( BasicWindowCoveringCmd_StartLevelChange );
		msg->Append( 0x40 );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
		return true;
	}
	else
	{
		Log::Write( "BasicWindowCovering - Stop Level Change on node %d", GetNodeId() );
		Msg* msg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( BasicWindowCoveringCmd_StopLevelChange );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <BasicWindowCovering::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void BasicWindowCovering::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		m_open.AddInstance( _instance, node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Open" ) );
		m_close.AddInstance( _instance, node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 1, "Close" ) );
	}
}

