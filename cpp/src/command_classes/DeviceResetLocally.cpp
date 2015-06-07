//-----------------------------------------------------------------------------
//
//	DeviceResetLocally.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_DEVICE_RESET_LOCALLY
//
//	Copyright (c) 2015
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

#include "command_classes/CommandClasses.h"
#include "command_classes/DeviceResetLocally.h"
#include "Defs.h"
#include "platform/Log.h"

using namespace OpenZWave;

enum DeviceResetLocallyCmd
{
	DeviceResetLocallyCmd_Notification = 1
};


//-----------------------------------------------------------------------------
// <DeviceResetLocally::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool DeviceResetLocally::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( DeviceResetLocallyCmd_Notification == _data[0] )
	{
		// device has been reset
		Log::Write( LogLevel_Info, GetNodeId(), "Received Device Reset Locally from node %d", GetNodeId() );

		// TODO tell the controller the node has gone away.
		// Manager::Get()->RemoveFailedNode(GetHomeId(), GetNodeId());
		return true;
	}
	return false;
}

