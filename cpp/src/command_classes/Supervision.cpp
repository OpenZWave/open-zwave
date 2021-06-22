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
			uint8 Supervision::CreateSupervisionSession(uint8 _command_class_id, uint8 _index)
			{
				m_last_session_id++;
				m_last_session_id &= 0x3f;

				if (m_sessions.size() >= 6)
				{
					// Clean up oldest session, we support max 6 simultaneous sessions per node 
					m_sessions.pop_front();
				}

				m_sessions.push_back({
					.session_id = m_last_session_id, 
					.command_class_id = _command_class_id,
					.index = _index
				});

				return m_last_session_id;
			}

			uint32 Supervision::GetSupervisionIndex(uint8 _session_id)
			{
				for (auto it = m_sessions.cbegin(); it != m_sessions.cend(); ++it) 
				{
					if (it->session_id == _session_id) 
					{
						return it->index;
					}
				}
			
				return StaticNoIndex();
			}

//-----------------------------------------------------------------------------
// <Supervision::HandleSupervisionReport>
// Handle a supervision report message from the Z-Wave network
//-----------------------------------------------------------------------------
			void Supervision::HandleSupervisionReport(uint8 const* _data, uint32 const _length, uint32 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					if (_length >= 4) 
					{
						uint8 more_status_updates = _data[1] >> 7;
						uint8 session_id = _data[1] & 0x3f;
						uint8 status = _data[2];
						int duration = _data[3];

						const char *status_identifier;
						switch (status) {
						case 0x00: status_identifier = "NO_SUPPORT"; break;
						case 0x01: status_identifier = "WORKING"; break;
						case 0x02: status_identifier = "FAIL"; break;
						case 0xff: status_identifier = "SUCCESS"; break;
						default: status_identifier = "UNKNOWN"; break;
						}

						for (auto it = m_sessions.cbegin(); it != m_sessions.cend(); ++it) 
						{
							if (it->session_id == session_id) 
							{
								if (CommandClass* pCommandClass = node->GetCommandClass(it->command_class_id))
								{
									Log::Write(LogLevel_Info, GetNodeId(), "Received SupervisionReport: session %d, %s index %d, status %s, duration %d sec, more status updates %d",
										session_id, 
										pCommandClass->GetCommandClassName().c_str(), it->index, 
										status_identifier, decodeDuration(duration), more_status_updates);

									if (status == SupervisionStatus::SupervisionStatus_Success)
									{
										pCommandClass->SupervisionSessionSuccess(session_id, _instance);
									}
								}
								else
								{
									Log::Write(LogLevel_Warning, GetNodeId(), "Received SupervisionReport for unknown CC %d", it->command_class_id);
								}

								if (more_status_updates == 0)
								{
									m_sessions.erase(it);
								}
							
								return;
							}
						}

						Log::Write(LogLevel_Warning, GetNodeId(), "Received SupervisionReport: unknown session %d, status %s, duration %d sec, more status updates %d",
							session_id, 
							status_identifier, decodeDuration(duration), more_status_updates);
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
			bool Supervision::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance)	// = 1
			{
				bool handled = false;
				Node* node = GetNodeUnsafe();
				if (node != NULL)
				{
					handled = true;
					switch ((SupervisionCmd) _data[0])
					{
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

