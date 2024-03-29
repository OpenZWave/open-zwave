//-----------------------------------------------------------------------------
//
//	ThermostatSetpoint.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_SETPOINT
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
#include "command_classes/ThermostatSetpoint.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueDecimal.h"

#include "tinyxml.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum ThermostatSetpointCmd
			{
				ThermostatSetpointCmd_Set = 0x01,
				ThermostatSetpointCmd_Get = 0x02,
				ThermostatSetpointCmd_Report = 0x03,
				ThermostatSetpointCmd_SupportedGet = 0x04,
				ThermostatSetpointCmd_SupportedReport = 0x05,
				ThermostatSetpointCmd_CapabilitiesGet = 0x09,
				ThermostatSetpointCmd_CapabilitiesReport = 0x0A
			};

#define ThermostatSetpoint_Count (ValueID_Index_ThermostatSetpoint::CoolingHeating + 1)

			static char const* c_setpointName[] =
			{ "Unused 0", "Heating 1", "Cooling 1", "Unused 3", "Unused 4", "Unused 5", "Unused 6", "Furnace", "Dry Air", "Moist Air", "Auto Changeover", "Heating Econ", "Cooling Econ", "Away Heating", "Away Cooling", "Full Power" };

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::ThermostatSetpoint>
// Constructor
//-----------------------------------------------------------------------------
			ThermostatSetpoint::ThermostatSetpoint(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId)
			{
				m_com.EnableFlag(COMPAT_FLAG_TSSP_BASE, 1);
				m_com.EnableFlag(COMPAT_FLAG_TSSP_ALTTYPEINTERPRETATION, true);
				SetStaticRequest(StaticRequest_Values);
			}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::RequestState>
// Get the static thermostat setpoint details from the device
//-----------------------------------------------------------------------------
			bool ThermostatSetpoint::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool requests = false;
				if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
				{
					requests |= RequestValue(_requestFlags, 0xff, _instance, _queue);
				}

				if (_requestFlags & RequestFlag_Session)
				{
					for (uint8 i = 0; i < ThermostatSetpoint_Count; ++i)
					{
						requests |= RequestValue(_requestFlags, i, _instance, _queue);
					}
				}

				return requests;
			}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::RequestValue>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool ThermostatSetpoint::RequestValue(uint32 const _requestFlags, uint16 const _setPointIndex, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_setPointIndex == 0xff)		// check for supportedget
				{
					// Request the supported setpoints
					Msg* msg = new Msg("ThermostatSetpointCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(ThermostatSetpointCmd_SupportedGet);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				if (!m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Log::Write(LogLevel_Info, GetNodeId(), "ThermostatSetpointCmd_Get Not Supported on this node");
					return false;
				}
				Internal::VC::Value* value = GetValue(1, _setPointIndex);
				if (value != NULL)
				{
					value->Release();
					// Request the setpoint value
					Msg* msg = new Msg("ThermostatSetpointCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(3);
					msg->Append(GetCommandClassId());
					msg->Append(ThermostatSetpointCmd_Get);
					msg->Append((_setPointIndex & 0xFF));
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool ThermostatSetpoint::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance // = 1
					)
			{
				if (ThermostatSetpointCmd_Report == (ThermostatSetpointCmd) _data[0])
				{
					// We have received a thermostat setpoint value from the Z-Wave device
					if (Internal::VC::ValueDecimal* value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, _data[1])))
					{
						uint8 scale;
						uint8 precision = 0;
						string temperature = ExtractValue(&_data[2], &scale, &precision);

						value->SetUnits(scale ? "F" : "C");
						value->OnValueRefreshed(temperature);
						if (value->GetPrecision() != precision)
						{
							value->SetPrecision(precision);
						}
						value->Release();

						Log::Write(LogLevel_Info, GetNodeId(), "Received thermostat setpoint report: Setpoint %s = %s%s", value->GetLabel().c_str(), value->GetValue().c_str(), value->GetUnits().c_str());
					}
					return true;
				}

				else if (ThermostatSetpointCmd_SupportedReport == (ThermostatSetpointCmd) _data[0])
				{
					if (Node* node = GetNodeUnsafe())
					{
						// We have received the supported thermostat setpoints from the Z-Wave device
						Log::Write(LogLevel_Info, GetNodeId(), "Received supported thermostat setpoints");

						// Parse the data for the supported setpoints
						for (uint32 i = 1; i < _length - 1; ++i)
						{
							for (int32 bit = 0; bit < 8; ++bit)
							{
								if ((_data[i] & (1 << bit)) != 0)
								{
									if (GetVersion() >= 3)
									{
										// Request the supported setpoints
										Msg* msg = new Msg("ThermostatSetpointCmd_CapabilitesGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
										msg->SetInstance(this, _instance);
										msg->Append(GetNodeId());
										msg->Append(3);
										msg->Append(GetCommandClassId());
										msg->Append(ThermostatSetpointCmd_CapabilitiesGet);
										uint8 type = ((i - 1) << 3) + bit;
										if (m_com.GetFlagBool(COMPAT_FLAG_TSSP_ALTTYPEINTERPRETATION) == false)
										{
											// for interpretation A the setpoint identifier makes a jump of 4 after the 2nd bit ... wtf @ zensys
											if (type > 2)
											{
												type += 4;
											}
										}
										msg->Append(type);
										msg->Append(GetDriver()->GetTransmitOptions());
										GetDriver()->SendMsg(msg, OpenZWave::Driver::MsgQueue_Query);
									}

									uint8 type = ((i - 1) << 3) + bit;
									if (m_com.GetFlagBool(COMPAT_FLAG_TSSP_ALTTYPEINTERPRETATION) == false)
									{
										// for interpretation A the setpoint identifier makes a jump of 4 after the 2nd bit ... wtf @ zensys
										if (type > 2)
										{
											type += 4;
										}
									}
									int32 index = (int32) type + m_com.GetFlagByte(COMPAT_FLAG_TSSP_BASE);
									// Add supported setpoint
									if (index < ThermostatSetpoint_Count)
									{
										string setpointName = c_setpointName[index];

										node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, index, setpointName, "C", false, false, "0.0", 0);
										Log::Write(LogLevel_Info, GetNodeId(), "    Added setpoint: %s", setpointName.c_str());
									}
								}
							}
						}
					}

					ClearStaticRequest(StaticRequest_Values);
					return true;
				}
				else if (ThermostatSetpointCmd_CapabilitiesReport == (ThermostatSetpointCmd) _data[0])
				{
					if (Node* node = GetNodeUnsafe())
					{
						// We have received the capabilities for supported setpoint Type
						uint8 scale;
						uint8 precision = 0;
						uint8 size = _data[2] & 0x07;
						string minValue = ExtractValue(&_data[2], &scale, &precision);
						string maxValue = ExtractValue(&_data[2 + size + 1], &scale, &precision);

						Log::Write(LogLevel_Info, GetNodeId(), "Received capabilities of thermostat setpoint type %d, min %s max %s", (int) _data[1], minValue.c_str(), maxValue.c_str());

						uint8 index = _data[1];
						// Add supported setpoint
						if (index < ThermostatSetpoint_Count)
						{
							string setpointName = c_setpointName[index];

							node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_ThermostatSetpoint::Unused_0_Minimum + index, setpointName + "_minimum", "C", false, false, minValue, 0);
							node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_ThermostatSetpoint::Unused_0_Maximum + index, setpointName + "_maximum", "C", false, false, maxValue, 0);
							Log::Write(LogLevel_Info, GetNodeId(), "    Added setpoint: %s", setpointName.c_str());
						}

					}
				}

				return false;
			}

			void ThermostatSetpoint::SupervisionSessionSuccess(uint8 _session_id, uint32 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{			
					uint32 index = node->GetSupervisionIndex(_session_id);

					if (index != Internal::CC::Supervision::StaticNoIndex())
					{
						if (Internal::VC::ValueDecimal* value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, index)))
						{
							value->ConfirmNewValue();

							Log::Write(LogLevel_Info, GetNodeId(), "Confirmed thermostat setpoint index %d to %s%s",
								index, value->GetValue().c_str(), value->GetUnits().c_str());
						}
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Ignore unknown supervision session %d", _session_id);
					}
				}
			}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::SetValue>
// Set a thermostat setpoint temperature
//-----------------------------------------------------------------------------
			bool ThermostatSetpoint::SetValue(Internal::VC::Value const& _value)
			{
				if (Node* node = GetNodeUnsafe())
				{
					if (ValueID::ValueType_Decimal == _value.GetID().GetType())
					{
						Internal::VC::ValueDecimal const* value = static_cast<Internal::VC::ValueDecimal const*>(&_value);
						
						uint8 index = value->GetID().GetIndex() & 0xFF;
						uint8 supervision_session_id = node->CreateSupervisionSession(StaticGetCommandClassId(), index);
						if (supervision_session_id == Internal::CC::Supervision::StaticNoSessionId())
						{
							Log::Write(LogLevel_Debug, GetNodeId(), "Supervision not supported, fall back to setpoint set/get");
						}

						uint8 scale = strcmp("C", value->GetUnits().c_str()) ? 1 : 0;

						Msg* msg = new Msg("ThermostatSetpointCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
						msg->SetInstance(this, _value.GetID().GetInstance());
						msg->SetSupervision(supervision_session_id);
						msg->Append(GetNodeId());
						msg->Append(4 + GetAppendValueSize(value->GetValue()));
						msg->Append(GetCommandClassId());
						msg->Append(ThermostatSetpointCmd_Set);
						msg->Append(index);
						AppendValue(msg, value->GetValue(), scale);

						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						return true;
					}
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatSetpoint::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void ThermostatSetpoint::CreateVars(uint8 const _instance)
			{
#if 0
/* I don't think this was ever orignally called when we had a index param, so lets not create this strange SetPoint */
#issue #1981
				if (Node* node = GetNodeUnsafe())
				{
					node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, _index, "Setpoint", "C", false, false, "0.0", 0);
				}
#endif
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
