//-----------------------------------------------------------------------------
//
//	SwitchBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_BINARY
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
//  spec: http://zwavepublic.com/sites/default/files/command_class_specs_2017A/SDS13781-3%20Z-Wave%20Application%20Command%20Class%20Specification.pdf
//        pp. 78ff
//-----------------------------------------------------------------------------

#include "command_classes/CommandClasses.h"
#include "command_classes/SwitchBinary.h"
#include "command_classes/WakeUp.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "platform/Log.h"

#include "value_classes/ValueBool.h"
#include "value_classes/ValueByte.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum SwitchBinaryCmd
			{
				SwitchBinaryCmd_Set = 0x01,
				SwitchBinaryCmd_Get = 0x02,
				SwitchBinaryCmd_Report = 0x03
			};

//-----------------------------------------------------------------------------
// <SwitchBinary::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool SwitchBinary::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_requestFlags & RequestFlag_Dynamic)
				{
					return RequestValue(_requestFlags, 0, _instance, _queue);
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <SwitchBinary::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool SwitchBinary::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Msg* msg = new Msg("SwitchBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(SwitchBinaryCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "SwitchBinaryCmd_Get Not Supported on this node");
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <SwitchBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool SwitchBinary::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (SwitchBinaryCmd_Report == (SwitchBinaryCmd) _data[0])
				{
					Log::Write(LogLevel_Info, GetNodeId(), "Received SwitchBinary report from node %d: level=%s", GetNodeId(), _data[1] ? "On" : "Off");

					// data[1] => Switch state
					if (Internal::VC::ValueBool* value = static_cast<Internal::VC::ValueBool*>(GetValue(_instance, ValueID_Index_SwitchBinary::Level)))
					{
						value->OnValueRefreshed(_data[1] != 0);
						value->Release();
					}

					if (GetVersion() >= 2)
					{

						// data[2] => target state
						if (Internal::VC::ValueBool* value = static_cast<Internal::VC::ValueBool*>(GetValue(_instance, ValueID_Index_SwitchBinary::TargetState)))
						{
							value->OnValueRefreshed(_data[2] != 0);
							value->Release();
						}

						// data[3] might be duration
						if (_length > 3)
						{
							if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchBinary::Duration)))
							{
								value->OnValueRefreshed(_data[3]);
								value->Release();
							}
						}
					}

					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <SwitchBinary::SetValue>
// Set the state of the switch
//-----------------------------------------------------------------------------
			bool SwitchBinary::SetValue(Internal::VC::Value const& _value)
			{
				bool res = false;
				uint8 instance = _value.GetID().GetInstance();

				switch (_value.GetID().GetIndex())
				{
					case ValueID_Index_SwitchBinary::Level:
					{
						if (Internal::VC::ValueBool* value = static_cast<Internal::VC::ValueBool*>(GetValue(instance, ValueID_Index_SwitchBinary::Level)))
						{
							res = SetState(instance, (static_cast<Internal::VC::ValueBool const*>(&_value))->GetValue());
							value->Release();
						}
						break;
					}
					case ValueID_Index_SwitchBinary::Duration:
					{
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(instance, ValueID_Index_SwitchBinary::Duration)))
						{
							value->OnValueRefreshed((static_cast<Internal::VC::ValueByte const*>(&_value))->GetValue());
							value->Release();
						}
						res = true;
						break;
					}

				}

				return res;
			}

//-----------------------------------------------------------------------------
// <SwitchBinary::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
			void SwitchBinary::SetValueBasic(uint8 const _instance, uint8 const _value)
			{
				// Send a request for new value to synchronize it with the BASIC set/report.
				// In case the device is sleeping, we set the value anyway so the BASIC set/report
				// stays in sync with it. We must be careful mapping the uint8 BASIC value
				// into a class specific value.
				// When the device wakes up, the real requested value will be retrieved.
				RequestValue(0, 0, _instance, Driver::MsgQueue_Send);
				if (Node* node = GetNodeUnsafe())
				{
					if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(node->GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
					{
						if (!wakeUp->IsAwake())
						{
							if (Internal::VC::ValueBool* value = static_cast<Internal::VC::ValueBool*>(GetValue(_instance, 0)))
							{
								value->OnValueRefreshed(_value != 0);
								value->Release();
							}
						}
					}
				}
			}

//-----------------------------------------------------------------------------
// <SwitchBinary::SetState>
// Set a new state for the switch
//-----------------------------------------------------------------------------
			bool SwitchBinary::SetState(uint8 const _instance, bool const _state)
			{
				uint8 const nodeId = GetNodeId();
				uint8 const targetValue = _state ? 0xff : 0;

				Log::Write(LogLevel_Info, nodeId, "SwitchBinary::Set - Setting to %s", _state ? "On" : "Off");
				Msg* msg = new Msg("SwitchBinaryCmd_Set", nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true);
				msg->SetInstance(this, _instance);
				msg->Append(nodeId);

				if (GetVersion() >= 2)
				{
					Internal::VC::ValueByte* durationValue = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchBinary::Duration));
					uint8 duration = durationValue->GetValue();
					durationValue->Release();
					if (duration == 0xff)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "  Duration: Default");
					}
					else if (duration >= 0x80)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "  Duration: %d minutes", duration - 0x7f);
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "  Duration: %d seconds", duration);
					}

					msg->Append(4);
					msg->Append(GetCommandClassId());
					msg->Append(SwitchBinaryCmd_Set);
					msg->Append(targetValue);
					msg->Append(duration);
				}
				else
				{
					msg->Append(3);
					msg->Append(GetCommandClassId());
					msg->Append(SwitchBinaryCmd_Set);
					msg->Append(targetValue);
				}

				msg->Append(GetDriver()->GetTransmitOptions());
				GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
				return true;
			}

//-----------------------------------------------------------------------------
// <SwitchBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void SwitchBinary::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					if (GetVersion() >= 2)
					{
						node->CreateValueByte(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_SwitchBinary::Duration, "Transition Duration", "", false, false, 0xff, 0);
						node->CreateValueBool(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_SwitchBinary::TargetState, "Target State", "", true, false, true, 0);
					}
					node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SwitchBinary::Level, "Switch", "", false, false, false, 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
