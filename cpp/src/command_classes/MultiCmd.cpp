//-----------------------------------------------------------------------------
//
//	MultiCmd.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_CMD
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

#include "command_classes/CommandClasses.h"
#include "command_classes/MultiCmd.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

//-----------------------------------------------------------------------------
// <MultiCmd::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool MultiCmd::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (MultiCmdCmd_Encap == (MultiCmdCmd) _data[0])
				{
					int commands;

					if (_length < 3)
					{
						Log::Write(LogLevel_Error, GetNodeId(), "Multi-command frame received is invalid, _length is < 3");
						return false;
					}
					else
					{
						commands = _data[1];
						Log::Write(LogLevel_Info, GetNodeId(), "Multi-command frame received, encapsulates %d command(s)", commands);
					}

					if (Node const *node = GetNodeUnsafe())
					{
						// Iterate over commands
						int base = 2;
						// Highest possible index is _length minus two because
						// array is zero-based and _data starts at second byte of command frame
						int highest_index = _length - 2;
						for (uint8 i = 1; i <= commands; ++i)
						{
							if (base > highest_index)
							{
								Log::Write(LogLevel_Error, GetNodeId(),
										   "Multi-command command part %d is invalid, frame is too short: base > highest_index (%d > %d)",
										   i, base, highest_index);
								return false;
							}

							uint8 length = _data[base];
							int end = base + length;
							if (end > highest_index)
							{
								Log::Write(LogLevel_Error, GetNodeId(),
										   "Multi-command command part %d with base %d is invalid, end > highest_index (%d > %d)",
										   i, base, end, highest_index);
								return false;
							}

							uint8 commandClassId = _data[base + 1];

							if (CommandClass *pCommandClass = node->GetCommandClass(commandClassId))
							{
								if (!pCommandClass->IsAfterMark())
									pCommandClass->HandleMsg(&_data[base + 2], length - 1);
								else
									pCommandClass->HandleIncomingMsg(&_data[base + 2], length - 1);
							}

							base += (length + 1);
						}
					}

					Log::Write(LogLevel_Info, GetNodeId(), "Multi-command, all %d command(s) processed", commands);
					return true;
				}
				return false;
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

