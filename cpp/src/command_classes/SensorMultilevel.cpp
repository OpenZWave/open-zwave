//-----------------------------------------------------------------------------
//
//	SensorMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_MULTILEVEL
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
#include "command_classes/SensorMultilevel.h"
#include "command_classes/MultiInstance.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "SensorMultiLevelCCTypes.h"
#include "platform/Log.h"


#include "value_classes/ValueDecimal.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum SensorMultilevelCmd
			{
				SensorMultilevelCmd_SupportedGet = 0x01,
				SensorMultilevelCmd_SupportedReport = 0x02,
				SensorMultiLevelCmd_SupportedGetScale = 0x03,
				SensorMultiLevelCmd_SupportedReportScale = 0x06,
				SensorMultilevelCmd_Get = 0x04,
				SensorMultilevelCmd_Report = 0x05
			};


//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool SensorMultilevel::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool res = false;
				if (GetVersion() > 4)
				{
					if (_requestFlags & RequestFlag_Static)
					{
						/* for Versions 5 and Above
						 * Send a Supported Get. When we get the Reply
						 * We will send a SupportedScaleGet Message
						 */
						Msg* msg = new Msg("SensorMultilevelCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(SensorMultilevelCmd_SupportedGet);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						res = true;
					}
				} else {
					if (_requestFlags & RequestFlag_Static)
					{
						/* For Versions 1-4
						** Set a Get Message - The Reply will create our ValueID
						*/
						Msg* msg = new Msg("SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(SensorMultilevelCmd_Get);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						res = true;
					}
				}

				if (_requestFlags & RequestFlag_Dynamic)
				{
					res |= RequestValue(_requestFlags, 0, _instance, _queue);
				}

				return res;
			}

//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool SensorMultilevel::RequestValue(uint32 const _requestFlags, uint16 const _index,		// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool res = false;
				if (!m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Log::Write(LogLevel_Info, GetNodeId(), "SensorMultilevelCmd_Get Not Supported on this node");
					return false;
				}
				/* if Index is 0, then its a being called from RequestState
				 * so we just get all possible Sensor Values
				 */
				if (_index == 0) {
					if (GetVersion() < 5)
					{
						Msg* msg = new Msg("SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(SensorMultilevelCmd_Get);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						res = true;
					}
					else
					{
						for (uint8 i = 1; i < 255; i++)
						{
							Internal::VC::Value* value = GetValue(_instance, i);
							if (value != NULL)
							{
								uint8_t scale = 0;
								/* get the Default Scale they want */
								Internal::VC::ValueList *requestedScale = static_cast<Internal::VC::ValueList *>(GetValue(_instance, i+255));
								if (requestedScale) {
									const Internal::VC::ValueList::Item *item = requestedScale->GetItem();
									if (item)
										scale = item->m_value;
									requestedScale->Release();
								}
								value->Release();
								Msg* msg = new Msg("SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
								msg->SetInstance(this, _instance);
								msg->Append(GetNodeId());
								msg->Append(4);
								msg->Append(GetCommandClassId());
								msg->Append(SensorMultilevelCmd_Get);
								msg->Append(i);
								msg->Append(scale);
								msg->Append(GetDriver()->GetTransmitOptions());
								GetDriver()->SendMsg(msg, _queue);
								res = true;
							}
						}
					}
					return res;
				} else if (_index < 256) {
					Internal::VC::Value* value = GetValue(_instance, _index);
					if (value != NULL) {
						uint8_t scale = 0;
						/* get the Default Scale they want */
						Internal::VC::ValueList *requestedScale = static_cast<Internal::VC::ValueList *>(GetValue(_instance, _index+255));
						if (requestedScale) {
							const Internal::VC::ValueList::Item *item = requestedScale->GetItem();
							if (item)
								scale = item->m_value;
							requestedScale->Release();
						}
						value->Release();
						Msg* msg = new Msg("SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(4);
						msg->Append(GetCommandClassId());
						msg->Append(SensorMultilevelCmd_Get);
						msg->Append((uint8_t)(_index & 0xFF));
						msg->Append(scale);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						return true;
					}
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <SensorMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool SensorMultilevel::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (SensorMultilevelCmd_SupportedReport == (SensorMultilevelCmd) _data[0])
				{
					string msg = "";

					if (GetNodeUnsafe())
					{
						for (uint8 i = 1; i <= (_length - 2); i++)
						{
							for (uint8 j = 0; j < 8; j++)
							{
								if (_data[i] & (1 << j))
								{
									uint32_t sensorType = ((i - 1) * 8) + j + 1;
									Log::Write(LogLevel_Info, GetNodeId(), "Received SensorMultiLevel supported report from node %d: %s (%d)", GetNodeId(), SensorMultiLevelCCTypes::Get()->GetSensorName(sensorType).c_str(), sensorType);
									Msg* msg = new Msg("SensorMultiLevelCmd_SupportedGetScale", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
									msg->SetInstance(this, _instance);
									msg->Append(GetNodeId());
									msg->Append(3);
									msg->Append(GetCommandClassId());
									msg->Append(SensorMultiLevelCmd_SupportedGetScale);
									msg->Append(sensorType);
									msg->Append(GetDriver()->GetTransmitOptions());
									GetDriver()->SendMsg(msg, Driver::MsgQueue::MsgQueue_Send);
								}
							}
						}
					}
					return true;
				}
				else if (SensorMultiLevelCmd_SupportedReportScale == (SensorMultilevelCmd) _data[0])
				{
					uint32_t sensorType = _data[1];
					int8_t defaultScale = -1;
					vector<Internal::VC::ValueList::Item> items;
					for (int i = 0; i <= 3; i++)
					{
						if ((_data[2] & 0x07) & (1 << i)) {

							uint8 sensorScale = i;
							if (defaultScale == -1)
								defaultScale = sensorScale;
							Log::Write(LogLevel_Info, GetNodeId(), "Received SensorMultiLevel supported Scale report from node %d for Sensor %s: %s (%d)", GetNodeId(), SensorMultiLevelCCTypes::Get()->GetSensorName(sensorType).c_str(), SensorMultiLevelCCTypes::Get()->GetSensorUnit(sensorType, sensorScale).c_str(), sensorScale);
							Internal::VC::ValueList::Item item;
							item.m_label = SensorMultiLevelCCTypes::Get()->GetSensorUnitName(sensorType, sensorScale);
							item.m_value = sensorScale;
							items.push_back(item);
						}
					}
					Log::Write(LogLevel_Info, GetNodeId(), "Setting SensorMultiLevel Default Scale to: %s (%d)",  SensorMultiLevelCCTypes::Get()->GetSensorUnit(sensorType, defaultScale).c_str(), defaultScale);

					if (Node* node = GetNodeUnsafe())
					{
						node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, sensorType, SensorMultiLevelCCTypes::Get()->GetSensorName(sensorType), SensorMultiLevelCCTypes::Get()->GetSensorUnit(sensorType, defaultScale), true, false, "0.0", 0);
						node->CreateValueList(ValueID::ValueGenre_System, GetCommandClassId(), _instance, sensorType+255, SensorMultiLevelCCTypes::Get()->GetSensorName(sensorType).append(" Units").c_str(), "",  false, false, 1, items, 0, 0);
						Internal::VC::ValueList *value = static_cast<Internal::VC::ValueList *>(GetValue(_instance, sensorType+255));
						if (value)
							value->SetByLabel(SensorMultiLevelCCTypes::Get()->GetSensorUnit(sensorType, defaultScale));
					}
					return true;
				}
				else if (SensorMultilevelCmd_Report == (SensorMultilevelCmd) _data[0])
				{
					uint8 scale;
					uint8 precision = 0;
					uint8 sensorType = _data[1];
					string valueStr = ExtractValue(&_data[2], &scale, &precision);

					Node* node = GetNodeUnsafe();
					if (node != NULL)
					{
						Internal::VC::ValueDecimal* value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, sensorType));
						if (value == NULL)
						{
							node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, sensorType, SensorMultiLevelCCTypes::Get()->GetSensorName(sensorType), "", true, false, "0.0", 0);
							value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, sensorType));
						}
						value->SetUnits(SensorMultiLevelCCTypes::Get()->GetSensorUnit(sensorType, scale));

						Log::Write(LogLevel_Info, GetNodeId(), "Received SensorMultiLevel report from node %d, instance %d, %s: value=%s%s", GetNodeId(), _instance, SensorMultiLevelCCTypes::Get()->GetSensorName(sensorType).c_str(), valueStr.c_str(), value->GetUnits().c_str());
						if (value->GetPrecision() != precision)
						{
							value->SetPrecision(precision);
						}
						value->OnValueRefreshed(valueStr);
						value->Release();
						return true;
					}
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <SensorMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void SensorMultilevel::CreateVars(uint8 const _instance)
			{
				// Don't create anything here. We do it in the report.
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
