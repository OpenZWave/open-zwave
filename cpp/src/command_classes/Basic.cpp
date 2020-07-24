//-----------------------------------------------------------------------------
//
//	Basic.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BASIC
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
#include "command_classes/Basic.h"
#include "command_classes/Association.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Notification.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"
#include "value_classes/ValueInt.h"
#include "command_classes/NoOperation.h"

#include "tinyxml.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum BasicCmd
			{
				BasicCmd_Set = 0x01,
				BasicCmd_Get = 0x02,
				BasicCmd_Report = 0x03
			};

//-----------------------------------------------------------------------------
// <Basic::Basic>
// Constructor
//-----------------------------------------------------------------------------
			Basic::Basic(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId)
			{
				m_com.EnableFlag(COMPAT_FLAG_BASIC_IGNOREREMAPPING, false);
				m_com.EnableFlag(COMPAT_FLAG_BASIC_MAPPING, 0);
				m_com.EnableFlag(COMPAT_FLAG_BASIC_SETASREPORT, false);
			}

//-----------------------------------------------------------------------------
// <Basic::ReadXML>
// Read configuration.
//-----------------------------------------------------------------------------
			void Basic::ReadXML(TiXmlElement const* _ccElement)
			{
				CommandClass::ReadXML(_ccElement);
				SetMapping(m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING));
				if (m_com.GetFlagBool(COMPAT_FLAG_BASIC_SETASREPORT))
					SetAfterMark();
			}

//-----------------------------------------------------------------------------
// <Basic::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool Basic::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (IsAfterMark())
				{
					Log::Write(LogLevel_Info, GetNodeId(), "%s is a Controlling Class", GetCommandClassName().c_str());
					return false;
				}
				if (_requestFlags & RequestFlag_Dynamic)
				{
					if ((m_com.GetFlagBool(COMPAT_FLAG_BASIC_IGNOREREMAPPING) || (!m_com.GetFlagBool(COMPAT_FLAG_BASIC_IGNOREREMAPPING) && m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING) == 0)))
						return RequestValue(_requestFlags, ValueID_Index_Basic::Set, _instance, _queue);
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Basic::RequestValue>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool Basic::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (IsAfterMark())
					return false;

				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Msg* msg = new Msg("BasicCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(BasicCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "BasicCmd_Get Not Supported on this node");
				}
				return false;
			}

			bool Basic::HandleIncomingMsg(uint8 const* _data, uint32 const _length, uint32 const _instance
					)
			{
				/* just redirect to the HandleMsg Method */
				return HandleMsg(_data, _length, _instance);
			}

//-----------------------------------------------------------------------------
// <Basic::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Basic::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (BasicCmd_Report == (BasicCmd) _data[0])
				{
					// Level
					Log::Write(LogLevel_Info, GetNodeId(), "Received Basic report from node %d: level=%d", GetNodeId(), _data[1]);
					if (!m_com.GetFlagBool(COMPAT_FLAG_BASIC_IGNOREREMAPPING) && m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING) != 0)
					{
						/* update our Mapped Class with the Target Values */
						if (GetVersion() >= 2 && _length == 4)
							UpdateMappedClass(_instance, m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING), _data[2]);
						else
							UpdateMappedClass(_instance, m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING), _data[1]);
					}
					else 
					{ 
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_Basic::Set)))
						{
							/* Set the Target Value, if it is present */
							if (_length == 4)
								value->SetTargetValue(_data[2], _data[3]);
							value->OnValueRefreshed(_data[1]);
							value->Release();
						}
						else
						{
							Log::Write(LogLevel_Warning, GetNodeId(), "No Valid Mapping for Basic Command Class and No ValueID Exported. Error?");
						}
						if (_length == 4)
						{
							if (Internal::VC::ValueByte* target = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_Basic::Target)))
							{
								target->OnValueRefreshed(_data[2]);
								target->Release();
							}
							if (Internal::VC::ValueInt* duration = static_cast<Internal::VC::ValueInt *>(GetValue(_instance, ValueID_Index_Basic::Duration)))
							{
								duration->OnValueRefreshed(decodeDuration(_data[3]));
								duration->Release();
							}
						}
					}
					return true;
				}

				if (BasicCmd_Set == (BasicCmd) _data[0])
				{
					if (m_com.GetFlagBool(COMPAT_FLAG_BASIC_SETASREPORT))
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Received Basic set from node %d: level=%d. Treating it as a Basic report.", GetNodeId(), _data[1]);
						if (!m_com.GetFlagBool(COMPAT_FLAG_BASIC_IGNOREREMAPPING) && m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING) != 0)
						{
							/* update our Mapped Class with the Target Values */
							if (GetVersion() >= 2 && _length == 4)
								UpdateMappedClass(_instance, m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING), _data[2]);
							else
								UpdateMappedClass(_instance, m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING), _data[1]);
						}
						else 
						{
							if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_Basic::Set)))
							{
								value->OnValueRefreshed(_data[1]);
								value->Release();
							}
							else
							{
								Log::Write(LogLevel_Warning, GetNodeId(), "No Valid Mapping for Basic Command Class and No ValueID Exported. Error?");
							}
							
							if (_length == 4)
							{
								if (Internal::VC::ValueByte* target = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_Basic::Target)))
								{
									target->OnValueRefreshed(_data[2]);
									target->Release();
								}
								if (Internal::VC::ValueInt* duration = static_cast<Internal::VC::ValueInt *>(GetValue(_instance, ValueID_Index_Basic::Duration)))
								{
									duration->OnValueRefreshed(decodeDuration(_data[3]));
									duration->Release();
								}
							}
						}
					}
					else
					{
						// Commmand received from the node.  Handle as a notification event
						Log::Write(LogLevel_Info, GetNodeId(), "Received Basic set from node %d: level=%d.  Sending event notification.", GetNodeId(), _data[1]);

						Notification* notification = new Notification(Notification::Type_NodeEvent);
						notification->SetHomeNodeIdAndInstance(GetHomeId(), GetNodeId(), _instance);
						notification->SetEvent(_data[1]);
						GetDriver()->QueueNotification(notification);
					}
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <Basic::SetValue>
// Set a value on the Z-Wave device
//-----------------------------------------------------------------------------
			bool Basic::SetValue(Internal::VC::Value const& _value)
			{
				if (ValueID_Index_Basic::Set == _value.GetID().GetIndex())
				{
					Internal::VC::ValueByte const* value = static_cast<Internal::VC::ValueByte const*>(&_value);

					Log::Write(LogLevel_Info, GetNodeId(), "Basic::Set - Setting node %d to level %d", GetNodeId(), value->GetValue());
					Msg* msg = new Msg("BasicCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->SetInstance(this, _value.GetID().GetInstance());
					msg->Append(GetNodeId());
					msg->Append(3);
					msg->Append(GetCommandClassId());
					msg->Append(BasicCmd_Set);
					msg->Append(value->GetValue());
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <Basic::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void Basic::CreateVars(uint8 const _instance)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_BASIC_IGNOREREMAPPING) || (m_com.GetFlagByte(COMPAT_FLAG_BASIC_MAPPING) == 0))
				{
					Log::Write(LogLevel_Info, GetNodeId(), "COMMAND_CLASS_BASIC is not mapped to another CC. Exposing ValueID");
					if (Node* node = GetNodeUnsafe())
					{
						node->CreateValueByte(ValueID::ValueGenre_Basic, GetCommandClassId(), _instance, ValueID_Index_Basic::Set, "Basic", "", IsAfterMark(), false, 0, 0);
						if ((GetVersion() >= 2) || (IsAfterMark()))
						{
							node->CreateValueByte(ValueID::ValueGenre_Basic, GetCommandClassId(), _instance, ValueID_Index_Basic::Target, "Basic Target", "", true, false, 0, 0);
							node->CreateValueInt(ValueID::ValueGenre_Basic, GetCommandClassId(), _instance, ValueID_Index_Basic::Duration, "Basic Duration", "", true, false, 0, 0);
						}
					}
				}

			}

//-----------------------------------------------------------------------------
// <Basic::Set>
// Helper method to set the level
//-----------------------------------------------------------------------------
			void Basic::Set(uint8 const _level)
			{
				// This may look like a long winded way to do this, but
				// it ensures that all the proper notifications get sent.
				if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(1, ValueID_Index_Basic::Set)))
				{
					value->Set(_level);
					value->Release();
				}
			}

//-----------------------------------------------------------------------------
// <Basic::SetMapping>
// Map COMMAND_CLASS_BASIC messages to another command class
//-----------------------------------------------------------------------------
			bool Basic::SetMapping(uint8 const _commandClassId)
			{
				bool res = false;

				if (_commandClassId != NoOperation::StaticGetCommandClassId())
				{
					char str[16];
					snprintf(str, sizeof(str), "0x%02x", _commandClassId);
					string ccstr = str;
					if (Node const* node = GetNodeUnsafe())
					{
						if (CommandClass* cc = node->GetCommandClass(_commandClassId))
						{
							ccstr = cc->GetCommandClassName();
						}
					}
					if (m_com.GetFlagBool(COMPAT_FLAG_BASIC_IGNOREREMAPPING))
					{
						Log::Write(LogLevel_Info, GetNodeId(), "    COMMAND_CLASS_BASIC will not be mapped to %s (ignored)", ccstr.c_str());
						m_com.SetFlagByte(COMPAT_FLAG_BASIC_MAPPING, 0);
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "    COMMAND_CLASS_BASIC will be mapped to %s", ccstr.c_str());
						m_com.SetFlagByte(COMPAT_FLAG_BASIC_MAPPING, _commandClassId);
						RemoveValue(1, ValueID_Index_Basic::Set);
						RemoveValue(1, ValueID_Index_Basic::Target);
						RemoveValue(1, ValueID_Index_Basic::Duration);
					}
					res = true;
				}
				return res;
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
