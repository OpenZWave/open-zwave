//-----------------------------------------------------------------------------
//
//	Supervision.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SUPERVISION
//
//	Copyright (c) 2020 Mark Ruys <mark@paracas.nl>
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
#include "command_classes/Supervision.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/Value.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{
            uint Supervision::m_session_id = 0;

            uint8 Supervision::GetSession(uint8 _command_class_id)
            {
                Supervision::m_session_id++;
                Supervision::m_session_id &= 0x3f;

                Supervision::m_command_class_id = _command_class_id;

                return Supervision::m_session_id;
            }

//-----------------------------------------------------------------------------
// <Supervision::HandleSupervisionReport>
// Handle a supervision report message from the Z-Wave network
//-----------------------------------------------------------------------------
			void Supervision::HandleSupervisionReport(uint8 const* _data, uint32 const _length, uint32 const _instance)
			{
                if (Node* node = GetNodeUnsafe())
				{
				    if ( _length >= 4 ) {
				        uint8 more_status_updates = _data[1] >> 7;
                        uint8 session_id = _data[1] & 0x3f;
                        uint8 status = _data[2];
                        int duration = _data[3];

                        Log::Write(LogLevel_Info, GetNodeId(), "Received SupervisionReport: session id %d, status 0x%02x, duration %d sec, more status updates %d",
                            session_id, status, decodeDuration(duration), more_status_updates);

                        if ( status == Supervision::SupervisionStatus::SupervisionStatus_Success )
                        {
                            if ( Supervision::m_session_id == session_id )
                            {
                                if (CommandClass* pCommandClass = node->GetCommandClass(Supervision::m_command_class_id))
                                {
                                    pCommandClass->SupervisionSessionSuccess(session_id, _instance);
                                }
                            }
                        }

                        if ( more_status_updates == 0 )
                        {
                            // Clean up session
                        }
					}
				}
			}

			bool Supervision::HandleIncomingMsg(uint8 const* _data, uint32 const _length, uint32 const _instance)
			{
				return HandleMsg(_data, _length, _instance);
			}

//-----------------------------------------------------------------------------
// <Supervision::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Supervision::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				bool handled = false;
				Node* node = GetNodeUnsafe();
				if (node != NULL)
				{
					handled = true;
					switch ((SupervisionCmd) _data[0])
					{
// 						case SupervisionCmd_Get:
// 						{
// 							HandleSupervisionEncap(_data, _length);
// 							break;
// 						}
						case SupervisionCmd_Report:
						{
							HandleSupervisionReport(_data, _length, _instance);
							break;
						}
						default:
						{
							handled = false;
							break;
						}
					}
				}

				return handled;


			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

