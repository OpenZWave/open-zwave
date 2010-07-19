//-----------------------------------------------------------------------------
//
//	SwitchMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_MULTILEVEL
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
#include "SwitchMultilevel.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

#include "ValueBool.h"
#include "ValueButton.h"
#include "ValueByte.h"

using namespace OpenZWave;

enum SwitchMultilevelCmd
{
	SwitchMultilevelCmd_Set						= 0x01,
	SwitchMultilevelCmd_Get						= 0x02,
	SwitchMultilevelCmd_Report					= 0x03,
	SwitchMultilevelCmd_StartLevelChange		= 0x04,
	SwitchMultilevelCmd_StopLevelChange			= 0x05,
	SwitchMultilevelCmd_SupportedGet			= 0x06,
	SwitchMultilevelCmd_SupportedReport			= 0x07
};

static uint8 c_directionParams[] = 
{ 
	0x18, 
	0x58, 
	0xc0, 
	0xc8 
};

static char const* c_directionDebugLabels[] = 
{ 
	"Up", 
	"Down", 
	"Inc", 
	"Dec" 
};

static char const* c_switchLabelsPos[] = 
{
	"Undefined",
	"On",
	"Up",
	"Open",
	"Clockwise",
	"Right",
	"Forward",
	"Push"
};

static char const* c_switchLabelsNeg[] = 
{
	"Undefined",
	"Off",
	"Down",
	"Close",
	"Counter-Clockwise",
	"Left",
	"Reverse",
	"Pull"
};


//-----------------------------------------------------------------------------
// <SwitchMultilevel::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void SwitchMultilevel::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		Msg* msg = new Msg( "SwitchMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchMultilevelCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchMultilevel::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( SwitchMultilevelCmd_Report == (SwitchMultilevelCmd)_data[0] )
	{
		Log::Write( "Received SwitchMultiLevel report from node %d: level=%d", GetNodeId(), _data[1] );

		if( ValueByte* value = m_level.GetInstance( _instance ) )
		{
			value->OnValueChanged( _data[1] );
		}
		return true;
	}

	if( SwitchMultilevelCmd_SupportedReport == (SwitchMultilevelCmd)_data[0] )
	{
		uint8 switchType1 = _data[1] & 0x1f;
		uint8 switchType2 = _data[2] & 0x1f;
		
		Log::Write( "Received SwitchMultiLevel supported report from node %d: Switch1=%s/%s, Switch2=%s/%s", GetNodeId(), c_switchLabelsPos[switchType1], c_switchLabelsNeg[switchType1], c_switchLabelsPos[switchType2], c_switchLabelsNeg[switchType2] );

		// Set the labels on the values
		ValueButton* button;

		if( switchType1 )
		{
			if( button = m_bright.GetInstance( _instance ) )
			{
				button->SetLabel( c_switchLabelsPos[switchType1] );
			}
			if( button = m_dim.GetInstance( _instance ) )
			{
				button->SetLabel( c_switchLabelsNeg[switchType1] );
			}
		}
		
		if( switchType2 )
		{
			if( button = m_inc.GetInstance( _instance ) )
			{
				button->SetLabel( c_switchLabelsPos[switchType2] );
			}
			if( button = m_dec.GetInstance( _instance ) )
			{
				button->SetLabel( c_switchLabelsNeg[switchType2] );
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetVersion>
// Set the command class version
//-----------------------------------------------------------------------------
void SwitchMultilevel::SetVersion
(
	uint8 const _version
)
{
	CommandClass::SetVersion( _version );

	if( _version == 3 )
	{
		// Request the supported switch types
		Msg* msg = new Msg( "SwitchMultilevelCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchMultilevelCmd_SupportedGet );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetValue>
// Set the level on a device
//-----------------------------------------------------------------------------
bool SwitchMultilevel::SetValue
(
	Value const& _value
)
{
	bool res = false;
	uint8 instance = _value.GetID().GetInstance();

	switch( _value.GetID().GetIndex() )
	{
		case 0:
		{
			// Level
			if( ValueByte const* level = m_level.GetInstance( instance ) )
			{
				res = SetLevel( instance, level->GetValue() );
			}
			break;
		}
		case 1:
		{
			// Bright
			if( ValueButton const* button = m_bright.GetInstance( instance ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Up );
				}
				else
				{
					res = StopLevelChange();
				}
			}
			break;
		}
		case 2:
		{
			// Dim
			if( ValueButton const* button = m_dim.GetInstance( instance ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Down );
				}
				else
				{
					res = StopLevelChange();
				}
			}
			break;
		}
		case 3:
		{
			// Ignore Start Level
			// Nothing to do.  State is set within the value, and
			// is not separately transmitted to the device.
			break;
		}
		case 4:
		{
			// Start level
			// Nothing to do.  State is set within the value, and
			// is not separately transmitted to the device.
			break;
		}
		case 5:
		{
			// Dimming Duration
			// Nothing to do.  State is set within the value, and
			// is not separately transmitted to the device.
			break;
		}
		case 6:
		{
			// Step Size
			// Nothing to do.  State is set within the value, and
			// is not separately transmitted to the device.
			break;
		}
		case 7:
		{
			// Inc
			if( ValueButton const* button = m_inc.GetInstance( instance ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Inc );
				}
				else
				{
					res = StopLevelChange();
				}
			}
			break;
		}
		case 8:
		{
			// Dec
			if( ValueButton const* button = m_dec.GetInstance( instance ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Dec );
				}
				else
				{
					res = StopLevelChange();
				}
			}
			break;
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetLevel>
// Set a new level for the switch
//-----------------------------------------------------------------------------
bool SwitchMultilevel::SetLevel
(
	uint8 const _instance,
	uint8 const _level
)
{
	Log::Write( "SwitchMultilevel::Set - Setting node %d to level %d", GetNodeId(), _level );
	Msg* msg = new Msg( "SwitchMultiLevel Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	
	if( ValueByte const* durationValue = m_duration.GetInstance( _instance ) )
	{
		uint8 duration = durationValue->GetValue();
		if( duration == 0xff )
		{
			Log::Write( "  Duration: Default" );
		}
		else if( duration >= 0x80 )
		{
			Log::Write( "  Duration: %d minutes", duration - 0x7f );
		}
		else
		{
			Log::Write( "  Duration: %d seconds", duration );
		}

		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchMultilevelCmd_Set );
		msg->Append( _level );
		msg->Append( duration );
	}
	else
	{
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchMultilevelCmd_Set );
		msg->Append( _level );
	}

	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SwitchMultilevelCmd_StartLevelChange>
// Start the level changing
//-----------------------------------------------------------------------------
bool SwitchMultilevel::StartLevelChange
(
	uint8 const _instance,
	SwitchMultilevelDirection const _direction
)
{
	Log::Write( "SwitchMultilevel::StartLevelChange - Starting a level change on node %d", GetNodeId() );

	uint8 length = 4;
	uint8 direction = c_directionParams[_direction];
	Log::Write( "  Direction:          %s", c_directionDebugLabels[_direction] );

	if( ValueBool const* ignoreStartLevel = m_ignoreStartLevel.GetInstance( _instance ) )
	{
		if( ignoreStartLevel->GetValue() )
		{
			// Set the ignore start level flag
			direction |= 0x20;
		}
	}
	Log::Write( "  Ignore Start Level: %s", (direction & 0x20) ? "True" : "False" );

	uint8 startLevel = 0;
	if( ValueByte const* startLevelValue = m_startLevel.GetInstance( _instance ) )
	{
		startLevel = startLevelValue->GetValue();
	}
	Log::Write( "  Start Level:        %d", startLevel );

	uint8 duration = 0;
	if( ValueByte const* durationValue = m_duration.GetInstance( _instance ) )
	{
		length = 5;
		duration = durationValue->GetValue();
		Log::Write( "  Duration:           %d", duration );
	}

	uint8 step = 0;
	if( ( SwitchMultilevelDirection_Inc == _direction ) || ( SwitchMultilevelDirection_Dec == _direction ) )
	{
		if( ValueByte const* stepValue = m_step.GetInstance( _instance ) )
		{
			length = 6;
			step = stepValue->GetValue();
			Log::Write( "  Step Size:          %d", step );
		}
	}
	
	Msg* msg = new Msg( "SwitchMultilevel StartLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( length );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchMultilevelCmd_StartLevelChange );
	msg->Append( direction );
	msg->Append( startLevel );

	if( length >= 5 )
	{
		msg->Append( duration );
	}

	if( length == 6 )
	{
		msg->Append( step );
	}

	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::StopLevelChange>
// Stop the level changing
//-----------------------------------------------------------------------------
bool SwitchMultilevel::StopLevelChange
(
)
{
	Log::Write( "SwitchMultilevel::StopLevelChange - Stopping the level change on node %d", GetNodeId() );
	Msg* msg = new Msg( "SwitchMultilevel StopLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchMultilevelCmd_StopLevelChange );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchMultilevel::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNode() )
	{
		switch( GetVersion() )
		{
			case 3:
			{
				m_step.AddInstance( _instance, node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 6, "Step Size", "", false, 0 ) );
				m_inc.AddInstance( _instance, node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 7, "Inc" ) );
				m_dec.AddInstance( _instance, node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 8, "Dec" ) );
				// Fall through to version 2
			}
			case 2:
			{
				m_duration.AddInstance( _instance, node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 5, "Dimming Duration", "", false, 0xff ) );
				// Fall through to version 1
			}
			case 1:
			{
				m_level.AddInstance( _instance, node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Level", "", false, 0 ) );
				m_bright.AddInstance( _instance, node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 1, "Bright" ) );
				m_dim.AddInstance( _instance, node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 2, "Dim" ) );
				m_ignoreStartLevel.AddInstance( _instance, node->CreateValueBool( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 3, "Ignore Start Level", "", false, true ) );
				m_startLevel.AddInstance( _instance, node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 4, "Start Level", "", false, 0 ) );
				break;
			}
		}

		ReleaseNode();
	}
}



