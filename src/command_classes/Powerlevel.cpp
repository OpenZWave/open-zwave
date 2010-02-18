//-----------------------------------------------------------------------------
//
//	Powerlevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_POWERLEVEL
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
#include "Powerlevel.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Log.h"

using namespace OpenZWave;

enum PowerlevelCmd
{
	PowerlevelCmd_Set				= 0x01,
	PowerlevelCmd_Get				= 0x02,
	PowerlevelCmd_Report			= 0x03,
	PowerlevelCmd_TestNodeSet		= 0x04,
	PowerlevelCmd_TestNodeGet		= 0x05,
	PowerlevelCmd_TestNodeReport	= 0x06
};

static char* const c_powerLevelNames[] = 
{
	"Normal",
	"-1dB",
	"-2dB",
	"-3dB",
	"-4dB",
	"-5dB",
	"-6dB",
	"-7dB",
	"-8dB",
	"-9dB"
};

static char* const c_powerLevelStatusNames[] = 
{
	"Failed",
	"Success",
	"In Progress"
};


//-----------------------------------------------------------------------------
// <Powerlevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Powerlevel::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if( PowerlevelCmd_Report == (PowerlevelCmd)_data[0] )
	{
		PowerLevelEnum powerLevel = (PowerLevelEnum)_data[1];
		uint8 timeout = _data[2];

		Log::Write( "Received a PowerLevel report from node %d: PowerLevel=%s, Timeout=%d", GetNodeId(), c_powerLevelNames[powerLevel], timeout );
		return true;
	}

	if( PowerlevelCmd_TestNodeReport == (PowerlevelCmd)_data[0] )
	{
		uint8 testNode = _data[1];
		PowerLevelStatusEnum status = (PowerLevelStatusEnum)_data[2];
		uint16 ackCount = (((uint16)_data[3])<<8) | (uint16)_data[4];

		Log::Write( "Received a PowerLevel Test Node report on node %d: Test Node=%d, Status=%s, Test Frame ACK Count=%d", GetNodeId(), testNode, c_powerLevelStatusNames[status], ackCount );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Powerlevel::Set>
// Set the transmit power of a node for a specified time
//-----------------------------------------------------------------------------
void Powerlevel::Set
(
	PowerLevelEnum _powerLevel,
	uint8 _timeout
)
{
	if( _powerLevel > PowerLevel_Minus9dB )
	{
		return;
	}

	Log::Write( "Setting the power level of node %d to %s for %d seconds", GetNodeId(), c_powerLevelNames[_powerLevel], _timeout );
	Msg* msg = new Msg( "PowerlevelCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( PowerlevelCmd_Set );
	msg->Append( (uint8)_powerLevel );
	msg->Append( _timeout );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <Powerlevel::Test>
// Test node to node communications
//-----------------------------------------------------------------------------
void Powerlevel::Test
(
	uint8 _testNodeId,
	PowerLevelEnum _powerLevel,
	uint16 _numFrames
)
{
	if( _powerLevel > PowerLevel_Minus9dB )
	{
		return;
	}

	Log::Write( "Running a Power Level Test from node %d: Target Node = %d, Power Level = %s, Number of Frames = %d", GetNodeId(), _testNodeId, c_powerLevelNames[_powerLevel], _numFrames );
	Msg* msg = new Msg( "PowerlevelCmd_TestNodeSet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 6 );
	msg->Append( GetCommandClassId() );
	msg->Append( PowerlevelCmd_TestNodeSet );
	msg->Append( _testNodeId );
	msg->Append( (uint8)_powerLevel );
	msg->Append( (uint8)(_numFrames >> 8) );
	msg->Append( (uint8)(_numFrames & 0x00ff) );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

