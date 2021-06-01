//-----------------------------------------------------------------------------
//
//	ThermostatMode.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_MODE
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
#include "command_classes/Supervision.h"
#include "command_classes/ThermostatMode.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueList.h"

#include "tinyxml.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum ThermostatModeCmd
			{
				ThermostatModeCmd_Set = 0x01,
				ThermostatModeCmd_Get = 0x02,
				ThermostatModeCmd_Report = 0x03,
				ThermostatModeCmd_SupportedGet = 0x04,
				ThermostatModeCmd_SupportedReport = 0x05
			};

			enum
			{
				ThermostatMode_Off = 0,
				ThermostatMode_Heat,
				ThermostatMode_Cool,
				ThermostatMode_Auto,
				ThermostatMode_Auxiliary,
				ThermostatMode_Resume_On,
				ThermostatMode_Fan,
				ThermostatMode_Furnance,
				ThermostatMode_Dry,
				ThermostatMode_Moist,
				ThermostatMode_AutoChangeover,
				ThermostatMode_HeatingEcon,
				ThermostatMode_CoolingEcon,
				ThermostatMode_Away,
				ThermostatMode_Reserved0E = 0x0E,
				ThermostatMode_FullPower,
				ThermostatMode_Reserved10 = 0x10,
				ThermostatMode_Reserved11,
				ThermostatMode_Reserved12,
				ThermostatMode_Reserved13,
				ThermostatMode_Reserved14,
				ThermostatMode_Reserved15,
				ThermostatMode_Reserved16,
				ThermostatMode_Reserved17,
				ThermostatMode_Reserved18,
				ThermostatMode_Reserved19,
				ThermostatMode_Reserved1A,
				ThermostatMode_Reserved1B,
				ThermostatMode_Reserved1C,
				ThermostatMode_Reserved1D,
				ThermostatMode_Reserved1E,
				ThermostatMode_ManufacturerSpecific = 0x1F,
				ThermostatMode_Count,
			};

			static char const* c_modeName[] =
			{ "Off", "Heat", "Cool", "Auto", "Aux Heat", "Resume", "Fan Only", "Furnace", "Dry Air", "Moist Air", "Auto Changeover", "Heat Econ", "Cool Econ", "Away", "Unknown", "Full Power", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Manufacturer Specific" };


			ThermostatMode::ThermostatMode(uint32 const _homeId, uint8 const _nodeId) :
							CommandClass(_homeId, _nodeId),
							m_currentMode(0)
			{
				SetStaticRequest(StaticRequest_Values);
			}

//-----------------------------------------------------------------------------
// <ThermostatMode::ReadXML>
// Read the supported modes
//-----------------------------------------------------------------------------
			void ThermostatMode::ReadXML(TiXmlElement const* _ccElement)
			{
				CommandClass::ReadXML(_ccElement);

				if (GetNodeUnsafe())
				{
					vector<Internal::VC::ValueList::Item> supportedModes;

					TiXmlElement const* supportedModesElement = _ccElement->FirstChildElement("SupportedModes");
					if (supportedModesElement)
					{
						TiXmlElement const* modeElement = supportedModesElement->FirstChildElement();
						while (modeElement)
						{
							char const* str = modeElement->Value();
							if (str && !strcmp(str, "Mode"))
							{
								int index;
								if (TIXML_SUCCESS == modeElement->QueryIntAttribute("index", &index))
								{
									if (index > ThermostatMode_Count)
									{
										Log::Write(LogLevel_Warning, GetNodeId(), "index Value in XML was greater than range. Setting to Invalid");
										index = ThermostatMode_Count + 1;
									}
									Internal::VC::ValueList::Item item;
									item.m_value = index;
									item.m_label = c_modeName[index];
									supportedModes.push_back(item);
								}
							}

							modeElement = modeElement->NextSiblingElement();
						}
					}

					if (!supportedModes.empty())
					{
						m_supportedModes = supportedModes;
					}
				}
			}

//-----------------------------------------------------------------------------
// <ThermostatMode::WriteXML>
// Save the supported modes
//-----------------------------------------------------------------------------
			void ThermostatMode::WriteXML(TiXmlElement* _ccElement)
			{
				CommandClass::WriteXML(_ccElement);

				if (m_supportedModes.empty())
				{
					return;
				}

				if (GetNodeUnsafe())
				{
					TiXmlElement* supportedModesElement = new TiXmlElement("SupportedModes");
					_ccElement->LinkEndChild(supportedModesElement);

					for (vector<Internal::VC::ValueList::Item>::iterator it = m_supportedModes.begin(); it != m_supportedModes.end(); ++it)
					{
						Internal::VC::ValueList::Item const& item = *it;

						TiXmlElement* modeElement = new TiXmlElement("Mode");
						supportedModesElement->LinkEndChild(modeElement);

						char str[8];
						snprintf(str, 8, "%d", item.m_value);
						modeElement->SetAttribute("index", str);
						modeElement->SetAttribute("label", item.m_label.c_str());
					}
				}
			}

//-----------------------------------------------------------------------------
// <ThermostatMode::RequestState>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
			bool ThermostatMode::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool requests = false;
				if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
				{
					// request supported mode list
					requests |= RequestValue(_requestFlags, 0, _instance, _queue);
				}

				if (_requestFlags & RequestFlag_Dynamic)
				{
					// Request the current mode
					requests |= RequestValue(_requestFlags, ValueID_Index_ThermostatMode::Mode, _instance, _queue);
				}

				return requests;
			}

//-----------------------------------------------------------------------------
// <ThermostatMode::RequestValue>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
			bool ThermostatMode::RequestValue(uint32 const _requestFlags, uint16 const _getTypeEnum, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
				{
					// Request the supported modes
					Msg* msg = new Msg("ThermostatModeCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(ThermostatModeCmd_SupportedGet);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				} 
				else if (_getTypeEnum == ValueID_Index_ThermostatMode::Mode)		// get current mode
				{
					if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
					{
						// Request the current mode
						Msg* msg = new Msg("ThermostatModeCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(ThermostatModeCmd_Get);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						return true;
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "ThermostatModeCmd_Get Not Supported on this node");

					}
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatMode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool ThermostatMode::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (ThermostatModeCmd_Report == (ThermostatModeCmd) _data[0])
				{
					uint8 mode = _data[1] & 0x1f;
					bool validMode = false;
					for (vector<Internal::VC::ValueList::Item>::iterator it = m_supportedModes.begin(); it != m_supportedModes.end(); ++it)
					{
						Internal::VC::ValueList::Item const& item = *it;
						if (item.m_value == mode)
						{
							validMode = true;
							break;
						}
					}

					if (validMode)
					{
						// We have received the thermostat mode from the Z-Wave device
						if (Internal::VC::ValueList* valueList = static_cast<Internal::VC::ValueList*>(GetValue(_instance, ValueID_Index_ThermostatMode::Mode)))
						{
							valueList->OnValueRefreshed(mode);
							if (valueList->GetItem())
								Log::Write(LogLevel_Info, GetNodeId(), "Received thermostat mode: %s", valueList->GetItem()->m_label.c_str());
							else
								Log::Write(LogLevel_Warning, GetNodeId(), "Received thermostat mode: %d (No Item)", mode);
							valueList->Release();
						}
						else
						{
							Log::Write(LogLevel_Info, GetNodeId(), "Received thermostat mode: index %d (No ValueID)", mode);
						}
						m_currentMode = mode;
					}
					else
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Received unknown thermostat mode: index %d", mode);
					}
					return true;
				}

				if (ThermostatModeCmd_SupportedReport == (ThermostatModeCmd) _data[0])
				{
					// We have received the supported thermostat modes from the Z-Wave device
					// these values are used to populate m_supportedModes which, in turn, is used to "seed" the values
					// for each m_modes instance
					Log::Write(LogLevel_Info, GetNodeId(), "Received supported thermostat modes");

					m_supportedModes.clear();
					for (uint32 i = 1; i < _length - 1; ++i)
					{
						for (int32 bit = 0; bit < 8; ++bit)
						{
							if ((_data[i] & (1 << bit)) != 0)
							{
								Internal::VC::ValueList::Item item;
								item.m_value = (int32) ((i - 1) << 3) + bit;
								/* minus 1 in the sizeof calc here, as the Unknown entry is our addition */
								if ((size_t) item.m_value >= (sizeof(c_modeName) / sizeof(*c_modeName) - 1))
								{
									Log::Write(LogLevel_Info, GetNodeId(), "Received unknown thermostat mode: 0x%x", item.m_value);
								}
								else
								{
									item.m_label = c_modeName[item.m_value];
									m_supportedModes.push_back(item);

									Log::Write(LogLevel_Info, GetNodeId(), "    Added mode: %s", c_modeName[item.m_value]);
								}
							}
						}
					}

					/* at this stage, we don't know the Actual Mode of the Fan, so set it to the lowest 
					 * value... If not, set to 0, which possibly could be invalid... 
					 */
					if (!m_supportedModes.empty()) {
						m_currentMode = m_supportedModes[0].m_value;
					} else {
						m_currentMode = 0;
					}
					if (Node* node = GetNodeUnsafe())
					{
						node->CreateValueList(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_ThermostatMode::Mode, "Mode", "", false, false, 1, m_supportedModes, m_currentMode, 0);
					}
					return true;
				}

				return false;
			}

			void ThermostatMode::SupervisionSessionSuccess(uint8 _session_id, uint32 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					uint32 index = node->GetSupervisionIndex(_session_id);

					if (index != Internal::CC::Supervision::StaticNoIndex())
					{
						// We have received the confirmation for the thermostat mode from the Z-Wave device
						if (Internal::VC::ValueList* valueList = static_cast<Internal::VC::ValueList*>(GetValue(_instance, ValueID_Index_ThermostatMode::Mode)))
						{
							valueList->ConfirmNewValue();
							if (valueList->GetItem())
							{
								Log::Write(LogLevel_Info, GetNodeId(), "Confirmed thermostat mode: %s", valueList->GetItem()->m_label.c_str());
								m_currentMode = valueList->GetItem()->m_value;
							}
							else
								Log::Write(LogLevel_Warning, GetNodeId(), "Confirmed thermostat mode (No Item)");
							valueList->Release();
						}
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Ignore unknown supervision session %d", _session_id);
					}
				}
			}

//-----------------------------------------------------------------------------
// <ThermostatMode::SetValue>
// Set the device's thermostat mode
//-----------------------------------------------------------------------------
			bool ThermostatMode::SetValue(Internal::VC::Value const& _value)
			{

				if (Node* node = GetNodeUnsafe())
				{
					if (ValueID::ValueType_List == _value.GetID().GetType())
					{
						Internal::VC::ValueList const* value = static_cast<Internal::VC::ValueList const*>(&_value);
						if (value->GetItem() == NULL)
							return false;
						uint8 state = (uint8)value->GetItem()->m_value;

						uint8 index = value->GetID().GetIndex() & 0xFF;
						uint8 supervision_session_id = node->CreateSupervisionSession(StaticGetCommandClassId(), index);
						if (supervision_session_id == Internal::CC::Supervision::StaticNoSessionId())
						{
							Log::Write(LogLevel_Debug, GetNodeId(), "Supervision not supported, fall back to setpoint set/get");
						}

						Msg* msg = new Msg("ThermostatModeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
						msg->SetInstance(this, _value.GetID().GetInstance());
						msg->SetSupervision(supervision_session_id);
						msg->Append(GetNodeId());
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(ThermostatModeCmd_Set);
						msg->Append(state);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						return true;
					}
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatMode::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void ThermostatMode::CreateVars(uint8 const _instance)
			{
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
