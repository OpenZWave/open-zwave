//-----------------------------------------------------------------------------
//
//	ControllerReplication.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CONTROLLER_REPLICATION
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
#include "ControllerReplication.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Log.h"

using namespace OpenZWave;

//static enum ControllerReplicationCmd
//{
//	CtrlReplicationTransferGroup = 0x31
//		Sequence number: type=BYTE
//		Group id: type=BYTE
//		Node id: type=BYTE
//	CtrlReplicationTransferGroupName = 0x32
//		Sequence number: type=BYTE
//		Group id: type=BYTE
//		Group name: type=ARRAY
//	CtrlReplicationTransferScene = 0x33
//		Sequence number: type=BYTE
//		Scene id: type=BYTE
//		Node id: type=BYTE
//		Level: type=BYTE
//	CtrlReplicationTransferSceneName = 0x34
//		Sequence number: type=BYTE
//		Scene id: type=BYTE
//		Scene name: type=ARRAY
//};


//-----------------------------------------------------------------------------
// <ControllerReplication::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ControllerReplication::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0	
)
{
	return false;
}


