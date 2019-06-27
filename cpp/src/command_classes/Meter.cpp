//-----------------------------------------------------------------------------
//
//	Meter.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_METER
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
#include "command_classes/Meter.h"
#include "command_classes/MultiInstance.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include "Utils.h"

#include "value_classes/ValueDecimal.h"
#include "value_classes/ValueList.h"
#include "value_classes/ValueButton.h"
#include "value_classes/ValueInt.h"
#include "value_classes/ValueBool.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum MeterCmd
			{
				MeterCmd_Get = 0x01,
				MeterCmd_Report = 0x02,
				// Version 2
				MeterCmd_SupportedGet = 0x03,
				MeterCmd_SupportedReport = 0x04,
				MeterCmd_Reset = 0x05
			};

			enum MeterType
			{
				MeterType_Electric = 0,
				MeterType_Gas,
				MeterType_Water,
				MeterType_Heating,
				MeterType_Cooling
			};

			struct s_MeterTypes {
				string Label;
				string Unit;
			};
			std::map<uint32_t, s_MeterTypes> MeterTypes = {
					{ValueID_Index_Meter::Electric_kWh, {"Electric - kWh", "kWh"}},
					{ValueID_Index_Meter::Electric_kVah, {"Electric - kVah", "kVah"}},
					{ValueID_Index_Meter::Electric_W, {"Electric - W", "W"}},
					{ValueID_Index_Meter::Electric_Pulse, {"Electric - Pulses", "Pulses"}},
					{ValueID_Index_Meter::Electric_V, {"Electric - V", "V"}},
					{ValueID_Index_Meter::Electric_A, {"Electric - A", "A"}},
					{ValueID_Index_Meter::Electric_PowerFactor, {"Electric - PF", "PF"}},
					{ValueID_Index_Meter::Electric_Unknown_1, {"Electric (Unknown)", ""}},
					{ValueID_Index_Meter::Electric_kVar, {"Electric - kVar", "kVar"}},
					{ValueID_Index_Meter::Electric_kVarh, {"Electric - kVarh", "kVarh"}},
					{ValueID_Index_Meter::Electric_Unknown_2, {"Electric (Unknown)", ""}},
					{ValueID_Index_Meter::Electric_Unknown_3, {"Electric (Unknown)", ""}},
					{ValueID_Index_Meter::Electric_Unknown_4, {"Electric (Unknown)", ""}},
					{ValueID_Index_Meter::Electric_Unknown_5, {"Electric (Unknown)", ""}},
					{ValueID_Index_Meter::Electric_Unknown_6, {"Electric (Unknown)", ""}},
					{ValueID_Index_Meter::Electric_Unknown_7, {"Electric (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Cubic_Meters, {"Gas - m3", "m3"}},
					{ValueID_Index_Meter::Gas_Cubic_Feet, {"Gas - ft3", "ft3"}},
					{ValueID_Index_Meter::Gas_Unknown_1, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Pulse, {"Gas - Pulses", "Pulses"}},
					{ValueID_Index_Meter::Gas_Unknown_2, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_3, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_4, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_5, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_6, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_7, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_8, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_9, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_10, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_11, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_12, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Gas_Unknown_13, {"Gas (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Cubic_Meters, {"Water - m3", "m3"}},
					{ValueID_Index_Meter::Water_Cubic_Feet, {"Water - ft3", "ft3"}},
					{ValueID_Index_Meter::Water_Cubic_US_Gallons, {"Water - gal", "gal"}},
					{ValueID_Index_Meter::Water_Cubic_Pulse, {"Water - Pulses", "Pulses"}},
					{ValueID_Index_Meter::Water_Unknown_1, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_2, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_3, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_4, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_5, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_6, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_7, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_8, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_9, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_10, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_11, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Water_Unknown_12, {"Water (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_kWh, {"Heating - kWh", "kWh"}},
					{ValueID_Index_Meter::Heating_Unknown_1, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_2, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_3, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_4, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_5, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_6, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_7, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_8, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_9, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_10, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_11, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_12, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_13, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_14, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Heating_Unknown_15, {"Heating (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_kWh, {"Cooling - kWh", "kWh"}},
					{ValueID_Index_Meter::Cooling_Unknown_1, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_2, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_3, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_4, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_5, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_6, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_7, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_8, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_9, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_10, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_11, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_12, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_13, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_14, {"Cooling (Unknown)", ""}},
					{ValueID_Index_Meter::Cooling_Unknown_15, {"Cooling (Unknown)", ""}}
			};
//-----------------------------------------------------------------------------
// <Meter::Meter>
// Constructor
//-----------------------------------------------------------------------------
			Meter::Meter(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId)
			{
#if 0
				std::map<uint32_t, s_MeterTypes>::iterator it;
				for (it = MeterTypes.begin(); it != MeterTypes.end(); it++) {
					std::cout << "Type: " << it->first << std::endl;
					std::cout << "\tLabel: " << it->second.Label << std::endl;
					std::cout << "\tUnit: " << it->second.Unit << std::endl;
				}
#endif
				SetStaticRequest(StaticRequest_Values);
			}

//-----------------------------------------------------------------------------
// <Meter::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool Meter::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool res = false;
				if (GetVersion() > 1)
				{
					if (_requestFlags & RequestFlag_Static)
					{
						Msg* msg = new Msg("MeterCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(MeterCmd_SupportedGet);
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
// <Meter::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool Meter::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool res = false;
				if (!m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Log::Write(LogLevel_Info, GetNodeId(), "MeterCmd_Get Not Supported on this node");
					return false;
				}
				for (uint8 i = 0; i < MeterTypes.size(); ++i)
				{
					Internal::VC::Value* value = GetValue(_instance, i);
					if (value != NULL)
					{
						value->Release();
						Msg* msg = new Msg("MeterCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						if (GetVersion() == 1)
							msg->Append(2);
						else if (GetVersion() <= 3)
							msg->Append(3);
						else if (GetVersion() >= 4) {
							uint8 scale = (i % 16);
							if (scale > ValueID_Index_Meter::Electric_Unknown_1)
								msg->Append(4);
							else
								msg->Append(3);
						}
						msg->Append(GetCommandClassId());
						msg->Append(MeterCmd_Get);
						if (GetVersion() == 2)
							msg->Append((((i % 16) & 0x03) << 3));
						else if (GetVersion() == 3)
							msg->Append((((i % 16) & 0x07) << 3));
						else if (GetVersion() >= 4) {
							uint8 scale = (i % 16);
							if (scale > ValueID_Index_Meter::Electric_Unknown_1) {
								/* 4.55.3 - 0x38 is scale value 7 unshifted */
								msg->Append(0x38);
								msg->Append(scale-8);
							} else {
								/* our Scale can fit in Scale 1 */
								msg->Append((((i % 16) & 0x07) << 3));
							}
						}
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						res |= true;
					}
				}
//				exit(-1);
				return res;
			}

//-----------------------------------------------------------------------------
// <Meter::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Meter::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				bool handled = false;
				if (MeterCmd_SupportedReport == (MeterCmd) _data[0])
				{
					handled = HandleSupportedReport(_data, _length, _instance);
				}
				else if (MeterCmd_Report == (MeterCmd) _data[0])
				{
					handled = HandleReport(_data, _length, _instance);
				}

				return handled;
			}

//-----------------------------------------------------------------------------
// <Meter::HandleSupportedReport>
// Create the values for this command class based on the reported parameters
//-----------------------------------------------------------------------------
			bool Meter::HandleSupportedReport(uint8 const* _data, uint32 const _length, uint32 const _instance)
			{
				bool canReset = ((_data[1] & 0x80) != 0);
				int8 meterType = (MeterType) (_data[1] & 0x1f);
				if (meterType > MeterType_Cooling) /* size of c_meterTypes */
				{
					Log::Write(LogLevel_Warning, GetNodeId(), "meterType Value was greater than range. Dropping Message");
					return false;
				}
				uint32 scale = 0;
				uint8 scalesize = 1;
				/* decode the Scale */
				/* Version 1 Doesn't have a Supported Report Message
				 * The Scale is encoded in the Report Message instead
				 */
				if (GetVersion() == 2) {
					scale = (_data[2] & 0x0F);
				}
				if (GetVersion() == 3) {
					scale = (_data[2] & 0xFF);
				}
				/* Version 4 has the Scale All Over the Place */
				if (GetVersion() >= 4) {
					/* if the MSB is set - Then the Scales Supported are specified as optional
					 * bytes following this byte
					 */
					scale = (_data[2] & 0x7F);
					if (_data[2] & 0x80) {
						uint8_t size = _data[3];
						for (int i = 1; i <= size; i++) {
							uint8 scale2 = _data[3+1];
							scale |= (scale2 << 8*i);
						}
						size += scalesize;
					}
				}
				if (Node* node = GetNodeUnsafe())
				{
					for (uint8 i = 0; i <= (scalesize*8)+1; ++i)
					{
						if (scale & (1 << i))
						{
							uint32 type = ((meterType-1) * 16) + i;
							if (type > MeterTypes.size()) {
								/* error */
								Log::Write(LogLevel_Warning, GetNodeId(), "MeterType %d and Unit %d is unknown", meterType, i);
								continue;
							}
							Log::Write(LogLevel_Info, GetNodeId(), "Creating MeterType %s (%d) with Unit %s (%d) at Index %d", MeterTypes.at(type).Label.c_str(), meterType, MeterTypes.at(type).Unit.c_str(), i, type);
							node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, type, MeterTypes.at(type).Label, MeterTypes.at(type).Unit, true, false, "0.0", 0);
						}
					}
					// Create the export flag
					node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_Meter::Exporting, "Exporting", "", true, false, false, 0);

					// Create the reset button
					if (canReset)
					{
						node->CreateValueButton(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_Meter::Reset, "Reset", 0);
					}
					return true;
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Meter::HandleReport>
// Read the reported meter value
//-----------------------------------------------------------------------------
			bool Meter::HandleReport(uint8 const* _data, uint32 const _length, uint32 const _instance)
			{

				// Get the value and scale
				uint8 scale;
				uint8 precision = 0;
				string valueStr = ExtractValue(&_data[2], &scale, &precision);
				scale = GetScale(_data, _length);
				int8 meterType = (MeterType) (_data[1] & 0x1f);

				uint16_t index = (((meterType -1) * 16) + scale);

				if (MeterTypes.count(index) == 0) {
					Log::Write(LogLevel_Warning, GetNodeId(), "MeterTypes Index is out of range/not valid - %d", index);
					return false;
				}

				Log::Write(LogLevel_Info, GetNodeId(), "Received Meter Report for %s (%d) with Units %s (%d) on Index %d: %s",MeterTypes.at(index).Label.c_str(), meterType, MeterTypes.at(index).Unit.c_str(), scale, index, valueStr.c_str());

				Internal::VC::ValueDecimal* value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, index));
				if (!value && (GetVersion() == 1))
				{
					if (Node* node = GetNodeUnsafe())
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Creating Version 1 MeterType %s (%d) with Unit %s (%d) at Index %d", MeterTypes.at(index).Label.c_str(), meterType, MeterTypes.at(index).Unit.c_str(), scale, index);
						node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, index, MeterTypes.at(index).Label, MeterTypes.at(index).Unit, true, false, "0.0", 0);
						value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, index));
					}
				} else if (!value) {
					Log::Write(LogLevel_Warning, GetNodeId(), "Can't Find a ValueID Index for %s (%d) with Unit %s (%d) - Index %d", MeterTypes.at(index).Label.c_str(), meterType, MeterTypes.at(index).Unit.c_str(), scale, index);
					return false;
				}
				value->OnValueRefreshed(valueStr);
				if (value->GetPrecision() != precision)
				{
					value->SetPrecision(precision);
				}
				value->Release();
				bool exporting = false;
				if (GetVersion() > 1)
				{
					exporting = ((_data[1] & 0x60) == 0x40);
					if (Internal::VC::ValueBool* value = static_cast<Internal::VC::ValueBool*>(GetValue(_instance, ValueID_Index_Meter::Exporting)))
					{
						value->OnValueRefreshed(exporting);
						value->Release();
					}
				}

#if 0
				/* Are Previous Values Important? */
				// Read any previous value and time delta
				uint8 size = _data[2] & 0x07;
				uint16 delta = (uint16) ((_data[3 + size] << 8) | _data[4 + size]);

				if (delta)
				{
					// There is only a previous value if the time delta is non-zero
					Internal::VC::ValueDecimal* previous = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, baseIndex + 1));
					if ( NULL == previous)
					{
						// We need to create a value to hold the previous
						if (Node* node = GetNodeUnsafe())
						{
							node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, baseIndex + 1, "Previous Reading", value->GetUnits().c_str(), true, false, "0.0", 0);
							previous = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, baseIndex + 1));
						}
					}
					if (previous)
					{
						precision = 0;
						valueStr = ExtractValue(&_data[2], &scale, &precision, 3 + size);
						Log::Write(LogLevel_Info, GetNodeId(), "    Previous value was %s%s, received %d seconds ago.", valueStr.c_str(), previous->GetUnits().c_str(), delta);
						previous->OnValueRefreshed(valueStr);
						if (previous->GetPrecision() != precision)
						{
							previous->SetPrecision(precision);
						}
						previous->Release();
					}

					// Time delta
					Internal::VC::ValueInt* interval = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, baseIndex + 2));
					if ( NULL == interval)
					{
						// We need to create a value to hold the time delta
						if (Node* node = GetNodeUnsafe())
						{
							node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), _instance, baseIndex + 2, "Interval", "seconds", true, false, 0, 0);
							interval = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, baseIndex + 2));
						}
					}
					if (interval)
					{
						interval->OnValueRefreshed((int32) delta);
						interval->Release();
					}
				}
#endif
				return true;
			}

//-----------------------------------------------------------------------------
// <Meter::SetValue>
// Set the device's scale, or reset its accumulated values.
//-----------------------------------------------------------------------------
			bool Meter::SetValue(Internal::VC::Value const& _value)
			{
				if (ValueID_Index_Meter::Reset == _value.GetID().GetIndex())
				{
					Internal::VC::ValueButton const* button = static_cast<Internal::VC::ValueButton const*>(&_value);
					if (button->IsPressed())
					{
						Msg* msg = new Msg("MeterCmd_Reset", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
						msg->SetInstance(this, _value.GetID().GetInstance());
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(MeterCmd_Reset);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						return true;
					}
				}

				return false;
			}

			//-----------------------------------------------------------------------------
			// <Meter::GetScale>
			// Decode The Scale as its all over the place in different versions
			//-----------------------------------------------------------------------------

			int32_t Meter::GetScale(uint8_t const *_data, uint32_t const _length) {
				uint8 scale = 0;
				//uint8 size = (_data[2] & 0x07);
				if (GetVersion() >= 1) {
					scale = ((_data[2] & 0x18) >> 3);
				}
				if (GetVersion() >= 3) {
					scale |= ((_data[1] & 0x80) >> 5);
				}
				if (GetVersion() >= 4) {
					/* 4.55.4 - Bit 7 indicates to use Scale 2 Field */
					if (scale == 7) {
						scale = (_data[_length-2] + 8);
					}
				}
				return scale;
			}


		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
