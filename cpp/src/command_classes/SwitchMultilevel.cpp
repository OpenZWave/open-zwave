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

#include "command_classes/CommandClasses.h"
#include "command_classes/SwitchMultilevel.h"
#include "command_classes/WakeUp.h"
#include "command_classes/Supervision.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "platform/Log.h"

#include "value_classes/ValueBool.h"
#include "value_classes/ValueButton.h"
#include "value_classes/ValueByte.h"
#include "value_classes/ValueInt.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum SwitchMultilevelCmd
			{
				SwitchMultilevelCmd_Set = 0x01,
				SwitchMultilevelCmd_Get = 0x02,
				SwitchMultilevelCmd_Report = 0x03,
				SwitchMultilevelCmd_StartLevelChange = 0x04,
				SwitchMultilevelCmd_StopLevelChange = 0x05,
				SwitchMultilevelCmd_SupportedGet = 0x06,
				SwitchMultilevelCmd_SupportedReport = 0x07
			};

			static uint8 c_directionParams[] =
			{ 0x00, 0x40, 0x00, 0x40 };

			static char const* c_directionDebugLabels[] =
			{ "Up", "Down", "Inc", "Dec" };

			static char const* c_switchLabelsPos[] =
			{ "Undefined", "On", "Up", "Open", "Clockwise", "Right", "Forward", "Push" };

			static char const* c_switchLabelsNeg[] =
			{ "Undefined", "Off", "Down", "Close", "Counter-Clockwise", "Left", "Reverse", "Pull" };

//-----------------------------------------------------------------------------
// <SwitchMultilevel::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool SwitchMultilevel::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_requestFlags & RequestFlag_Static) {
					if (GetVersion() >= 3)
					{
						// Request the supported switch types
						Msg* msg = new Msg("SwitchMultilevelCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(SwitchMultilevelCmd_SupportedGet);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					}
					return true;
				}
				if (_requestFlags & RequestFlag_Dynamic)
				{
					return RequestValue(_requestFlags, ValueID_Index_SwitchMultiLevel::Level, _instance, _queue);
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool SwitchMultilevel::RequestValue(uint32 const _requestFlags, uint16 const _index, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_index == ValueID_Index_SwitchMultiLevel::Level)
				{
					if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
					{
						Msg* msg = new Msg("SwitchMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(SwitchMultilevelCmd_Get);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						
						return true;
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "SwitchMultilevelCmd_Get Not Supported on this node");
					}
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool SwitchMultilevel::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (SwitchMultilevelCmd_Report == (SwitchMultilevelCmd) _data[0])
				{
					Log::Write(LogLevel_Info, GetNodeId(), "Received SwitchMultiLevel report: level=%d", _data[1]);

					if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Level)))
					{
						/* Target Value - 0 to 100 is valid values, 0xFF is also valid */
						if ((GetVersion() >= 4) && ((_data[2] <= 100) || (_data[2] == 0xFF)))
							value->SetTargetValue(_data[2], _data[3]);
						value->OnValueRefreshed(_data[1]);
						value->Release();
					}

					if (GetVersion() >= 4)
					{

						// data[2] => target value
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::TargetValue)))
						{
							value->OnValueRefreshed(_data[2]);
							value->Release();
						}

						// data[3] might be duration
						if (_length > 3)
						{
							if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Duration)))
							{
								value->OnValueRefreshed(decodeDuration(_data[3]));
								value->Release();
							}
						}
					}

					return true;
				}

				if (SwitchMultilevelCmd_SupportedReport == (SwitchMultilevelCmd) _data[0])
				{
					uint8 switchType1 = _data[1] & 0x1f;
					uint8 switchType2 = _data[2] & 0x1f;
					uint8 switchtype1label = switchType1;
					uint8 switchtype2label = switchType2;
					if (switchtype1label > 7) /* size of c_switchLabelsPos, c_switchLabelsNeg */
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "switchtype1label Value was greater than range. Setting to Invalid");
						switchtype1label = 0;
					}
					if (switchtype2label > 7) /* sizeof c_switchLabelsPos, c_switchLabelsNeg */
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "switchtype2label Value was greater than range. Setting to Invalid");
						switchtype2label = 0;
					}

					Log::Write(LogLevel_Info, GetNodeId(), "Received SwitchMultiLevel supported report: Switch1=%s/%s, Switch2=%s/%s", c_switchLabelsPos[switchtype1label], c_switchLabelsNeg[switchtype1label], c_switchLabelsPos[switchtype2label], c_switchLabelsNeg[switchtype2label]);
					ClearStaticRequest(StaticRequest_Version);

					// Set the labels on the values
					Internal::VC::ValueButton* button;

					if (switchType1)
					{
						if ( NULL != (button = static_cast<Internal::VC::ValueButton*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Bright))))
						{
							button->SetLabel(c_switchLabelsPos[switchtype1label]);
							button->Release();
						}
						if ( NULL != (button = static_cast<Internal::VC::ValueButton*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Dim))))
						{
							button->SetLabel(c_switchLabelsNeg[switchtype1label]);
							button->Release();
						}
					}

					if (switchType2)
					{
						if ( NULL != (button = static_cast<Internal::VC::ValueButton*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Inc))))
						{
							button->SetLabel(c_switchLabelsPos[switchtype2label]);
							button->Release();
						}
						if ( NULL != (button = static_cast<Internal::VC::ValueButton*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Dec))))
						{
							button->SetLabel(c_switchLabelsNeg[switchtype2label]);
							button->Release();
						}
					}
					return true;
				}
				Log::Write(LogLevel_Warning, GetNodeId(), "Recieved a Unhandled SwitchMultiLevel Command: %d", _data[0]);
				return false;
			}

			void SwitchMultilevel::SupervisionSessionSuccess(uint8 _session_id, uint32 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					uint32 index = node->GetSupervisionIndex(_session_id);

					if (index != Internal::CC::Supervision::StaticNoIndex())
					{
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Level)))
						{
							value->ConfirmNewValue();

							Log::Write(LogLevel_Info, GetNodeId(), "Confirmed switch multi level index %d to value=%d",
								index, value->GetValue());
						}
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Ignore unknown supervision session %d", _session_id);
					}
				}
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetValue>
// Set the level on a device
//-----------------------------------------------------------------------------
			bool SwitchMultilevel::SetValue(Internal::VC::Value const& _value)
			{
				bool res = false;
				uint8 instance = _value.GetID().GetInstance();

				switch (_value.GetID().GetIndex())
				{
					case ValueID_Index_SwitchMultiLevel::Level:
					{
						// Level
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::Level)))
						{
							res = SetLevel(instance, (static_cast<Internal::VC::ValueByte const*>(&_value))->GetValue());
							value->Release();
						}
						break;
					}
					case ValueID_Index_SwitchMultiLevel::Bright:
					{
						// Bright
						if (Internal::VC::ValueButton* button = static_cast<Internal::VC::ValueButton*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::Bright)))
						{
							if (button->IsPressed())
							{
								res = StartLevelChange(instance, SwitchMultilevelDirection_Up);
							}
							else
							{
								res = StopLevelChange(instance);
							}
							button->Release();
						}
						break;
					}
					case ValueID_Index_SwitchMultiLevel::Dim:
					{
						// Dim
						if (Internal::VC::ValueButton* button = static_cast<Internal::VC::ValueButton*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::Dim)))
						{
							if (button->IsPressed())
							{
								res = StartLevelChange(instance, SwitchMultilevelDirection_Down);
							}
							else
							{
								res = StopLevelChange(instance);
							}
							button->Release();
						}
						break;
					}
					case ValueID_Index_SwitchMultiLevel::IgnoreStartLevel:
					{
						if (Internal::VC::ValueBool* value = static_cast<Internal::VC::ValueBool*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::IgnoreStartLevel)))
						{
							value->OnValueRefreshed((static_cast<Internal::VC::ValueBool const*>(&_value))->GetValue());
							value->Release();
						}
						res = true;
						break;
					}
					case ValueID_Index_SwitchMultiLevel::StartLevel:
					{
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::StartLevel)))
						{
							value->OnValueRefreshed((static_cast<Internal::VC::ValueByte const*>(&_value))->GetValue());
							value->Release();
						}
						res = true;
						break;
					}
					case ValueID_Index_SwitchMultiLevel::Duration:
					{
						if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::Duration)))
						{
							value->OnValueRefreshed((static_cast<Internal::VC::ValueInt const*>(&_value))->GetValue());
							value->Release();
						}
						res = true;
						break;
					}
					case ValueID_Index_SwitchMultiLevel::Step:
					{
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::Step)))
						{
							value->OnValueRefreshed((static_cast<Internal::VC::ValueByte const*>(&_value))->GetValue());
							value->Release();
						}
						res = true;
						break;
					}
					case ValueID_Index_SwitchMultiLevel::Inc:
					{
						// Inc
						if (Internal::VC::ValueButton* button = static_cast<Internal::VC::ValueButton*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::Inc)))
						{
							if (button->IsPressed())
							{
								res = StartLevelChange(instance, SwitchMultilevelDirection_Inc);
							}
							else
							{
								res = StopLevelChange(instance);
							}
							button->Release();
						}
						break;
					}
					case ValueID_Index_SwitchMultiLevel::Dec:
					{
						// Dec
						if (Internal::VC::ValueButton* button = static_cast<Internal::VC::ValueButton*>(GetValue(instance, ValueID_Index_SwitchMultiLevel::Dec)))
						{
							if (button->IsPressed())
							{
								res = StartLevelChange(instance, SwitchMultilevelDirection_Dec);
							}
							else
							{
								res = StopLevelChange(instance);
							}
							button->Release();
						}
						break;
					}
				}

				return res;
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
			void SwitchMultilevel::SetValueBasic(uint8 const _instance, uint8 const _value)
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
							if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Level)))
							{
								value->OnValueRefreshed(_value != 0);
								value->Release();
							}
						}
					}
				}
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetLevel>
// Set a new level for the switch
//-----------------------------------------------------------------------------
			bool SwitchMultilevel::SetLevel(uint8 const _instance, uint8 const _level)
			{

				if (Node* node = GetNodeUnsafe())
				{
					//add supervision encapsulation if supported
					uint8 index = ValueID_Index_SwitchMultiLevel::Level;
					uint8 supervision_session_id = node->CreateSupervisionSession(StaticGetCommandClassId(), index);
					if (supervision_session_id == Internal::CC::Supervision::StaticNoSessionId())
					{
						Log::Write(LogLevel_Debug, GetNodeId(), "Supervision not supported, fall back to setpoint set/get");
					}

					Log::Write(LogLevel_Info, GetNodeId(), "SwitchMultilevel::Set - Setting to level %d", _level);
					Msg* msg = new Msg("SwitchMultilevelCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->SetInstance(this, _instance);
					msg->SetSupervision(supervision_session_id);
					msg->Append(GetNodeId());

					if (GetVersion() >= 2)
					{
						Internal::VC::ValueInt* durationValue = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Duration));
						uint32 duration = durationValue->GetValue();
						durationValue->Release();
						if (duration > 7620)
							Log::Write(LogLevel_Info, GetNodeId(), "  Duration: Device Default");
						else if (duration > 0x7F)
							Log::Write(LogLevel_Info, GetNodeId(), "  Rouding to %d Minutes (over 127 seconds)", encodeDuration(duration)-0x79);
						else 
							Log::Write(LogLevel_Info, GetNodeId(), "  Duration: %d seconds", duration);

						msg->Append(4);
						msg->Append(GetCommandClassId());
						msg->Append(SwitchMultilevelCmd_Set);
						msg->Append(_level);
						msg->Append(encodeDuration(duration));
					}
					else
					{
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(SwitchMultilevelCmd_Set);
						msg->Append(_level);
					}

					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					return true;
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SwitchMultilevelCmd_StartLevelChange>
// Start the level changing
//-----------------------------------------------------------------------------
			bool SwitchMultilevel::StartLevelChange(uint8 const _instance, SwitchMultilevelDirection const _direction)
			{
				Log::Write(LogLevel_Info, GetNodeId(), "SwitchMultilevel::StartLevelChange - Starting a level change");

				uint8 length = 4;
				if (_direction > 3) /* size of  c_directionParams, c_directionDebugLabels */
				{
					Log::Write(LogLevel_Warning, GetNodeId(), "_direction Value was greater than range. Dropping");
					return false;
				}
				uint8 direction = c_directionParams[_direction];
				Log::Write(LogLevel_Info, GetNodeId(), "  Direction:          %s", c_directionDebugLabels[_direction]);

				if (Internal::VC::ValueBool* ignoreStartLevel = static_cast<Internal::VC::ValueBool*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::IgnoreStartLevel)))
				{
					if (ignoreStartLevel->GetValue())
					{
						// Set the ignore start level flag
						direction |= 0x20;
					}
					ignoreStartLevel->Release();
				}
				Log::Write(LogLevel_Info, GetNodeId(), "  Ignore Start Level: %s", (direction & 0x20) ? "True" : "False");

				uint8 startLevel = 0;
				if (Internal::VC::ValueByte* startLevelValue = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::StartLevel)))
				{
					startLevel = startLevelValue->GetValue();
					startLevelValue->Release();
				}
				Log::Write(LogLevel_Info, GetNodeId(), "  Start Level:        %d", startLevel);

				uint32 duration = -1;
				if (Internal::VC::ValueInt* durationValue = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Duration)))
				{
					length = 5;
					duration = durationValue->GetValue();
					durationValue->Release();
					Log::Write(LogLevel_Info, GetNodeId(), "  Duration:           %d", duration);
				}

				uint8 step = 0;
				if ((SwitchMultilevelDirection_Inc == _direction) || (SwitchMultilevelDirection_Dec == _direction))
				{
					if (Internal::VC::ValueByte* stepValue = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_SwitchMultiLevel::Step)))
					{
						length = 6;
						step = stepValue->GetValue();
						stepValue->Release();
						Log::Write(LogLevel_Info, GetNodeId(), "  Step Size:          %d", step);
					}
				}

				Msg* msg = new Msg("SwitchMultilevelCmd_StartLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
				msg->SetInstance(this, _instance);
				msg->Append(GetNodeId());
				msg->Append(length);
				msg->Append(GetCommandClassId());
				msg->Append(SwitchMultilevelCmd_StartLevelChange);
				if (GetVersion() == 2)
				{
					direction &= 0x60;
				}
				else if (GetVersion() >= 3)
				{
					/* we don't support secondary switch, so we mask that out as well */
					direction &= 0xE0;
				}

				msg->Append(direction);
				msg->Append(startLevel);

				if (length >= 5)
				{
					msg->Append(encodeDuration(duration));
				}

				if (length == 6)
				{
					msg->Append(step);
				}

				msg->Append(GetDriver()->GetTransmitOptions());
				GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
				/* get a updated Level Value from the Device */
				RequestValue(0, ValueID_Index_SwitchMultiLevel::Level, _instance, Driver::MsgQueue_Send);
				return true;
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::StopLevelChange>
// Stop the level changing
//-----------------------------------------------------------------------------
			bool SwitchMultilevel::StopLevelChange(uint8 const _instance)
			{
				Log::Write(LogLevel_Info, GetNodeId(), "SwitchMultilevel::StopLevelChange - Stopping the level change");
				Msg* msg = new Msg("SwitchMultilevelCmd_StopLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
				msg->SetInstance(this, _instance);
				msg->Append(GetNodeId());
				msg->Append(2);
				msg->Append(GetCommandClassId());
				msg->Append(SwitchMultilevelCmd_StopLevelChange);
				msg->Append(GetDriver()->GetTransmitOptions());
				GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);

				/* get a updated Level Value from the Device */
				RequestValue(0, ValueID_Index_SwitchMultiLevel::Level, _instance, Driver::MsgQueue_Send);
				return true;
			}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void SwitchMultilevel::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					if (GetVersion() >= 4)
					{
						node->CreateValueByte(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::TargetValue, "Target Value", "", true, false, 0, 0);
					}
					if (GetVersion() >= 3)
					{
						node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::Step, "Step Size", "", false, false, 0, 0);
						node->CreateValueButton(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::Inc, "Inc", 0);
						node->CreateValueButton(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::Dec, "Dec", 0);
					}
					if (GetVersion() >= 2)
					{
						node->CreateValueInt(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::Duration, "Dimming Duration", "", false, false, -1, 0);
					}
					node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::Level, "Level", "", false, false, 0, 0);
					node->CreateValueButton(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::Bright, "Bright", 0);
					node->CreateValueButton(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::Dim, "Dim", 0);
					node->CreateValueBool(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::IgnoreStartLevel, "Ignore Start Level", "", false, false, true, 0);
					node->CreateValueByte(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_SwitchMultiLevel::StartLevel, "Start Level", "", false, false, 0, 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

