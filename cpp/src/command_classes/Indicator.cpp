//-----------------------------------------------------------------------------
//
//	Indicator.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_INDICATOR
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
#include "command_classes/Indicator.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"
#include "value_classes/ValueBool.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum IndicatorCmd
			{
				IndicatorCmd_Set = 0x01,
				IndicatorCmd_Get = 0x02,
				IndicatorCmd_Report = 0x03,
				IndicatorCmd_Supported_Get = 0x04,
				IndicatorCmd_Supported_Report = 0x05,
				IndicatorCmd_Description_Get = 0x06,
				IndicatorCmd_Description_Report = 0x07
			};
			std::map<uint32, string> IndicatorTypes = { 
				{ValueID_Index_Indicator::Indicator, "Indicator"},
				{ValueID_Index_Indicator::Armed, "Indicator: Armed"},
				{ValueID_Index_Indicator::Not_Armed, "Indicator: Not Armed"},
				{ValueID_Index_Indicator::Ready, "Indicator: Ready"},
				{ValueID_Index_Indicator::Fault, "Indicator: Fault"},
				{ValueID_Index_Indicator::Busy, "Indicator: Busy"},
				{ValueID_Index_Indicator::Enter_ID, "Indicator: Enter ID"},
				{ValueID_Index_Indicator::Enter_PIN, "Indicator: Enter PIN"},
				{ValueID_Index_Indicator::Code_Accepted, "Indicator: Code Accepted"},
				{ValueID_Index_Indicator::Code_Not_Accepted, "Indicator: Code Not Accepted"},
				{ValueID_Index_Indicator::Armed_Stay, "Indicator: Armed Stay"},
				{ValueID_Index_Indicator::Armed_Away, "Indicator: Armed Away"},
				{ValueID_Index_Indicator::Alarming, "Indicator: Alarming"},
				{ValueID_Index_Indicator::Alarming_Burglar, "Indicator: Alarming: Burglar"},
				{ValueID_Index_Indicator::Alarming_Smoke_Fire, "Indicator: Alarming: Smoke/Fire"},
				{ValueID_Index_Indicator::Alarming_Carbon_Monoxide, "Indicator: Alarming: Carbon Monoxide"},
				{ValueID_Index_Indicator::Bypass_Challenge, "Indicator: Bypass Challenge"},
				{ValueID_Index_Indicator::Entry_Delay, "Indicator: Entry Delay"},
				{ValueID_Index_Indicator::Exit_Delay, "Indicator: Exit Delay"},
				{ValueID_Index_Indicator::Alarming_Medical, "Indicator: Alarming: Medical"},
				{ValueID_Index_Indicator::Alarming_Freeze_Warning, "Indicator: Alarming: Freeze Warning"},
				{ValueID_Index_Indicator::Alarming_Water_Leak, "Indicator: Alarming: Water Leak"},
				{ValueID_Index_Indicator::Alarming_Panic, "Indicator: Alarming: Panic"},
				{ValueID_Index_Indicator::Zone_1_Armed, "Indicator: Zone 1 Armed"},
				{ValueID_Index_Indicator::Zone_2_Armed, "Indicator: Zone 2 Armed"},
				{ValueID_Index_Indicator::Zone_3_Armed, "Indicator: Zone 3 Armed"},
				{ValueID_Index_Indicator::Zone_4_Armed, "Indicator: Zone 4 Armed"},
				{ValueID_Index_Indicator::Zone_5_Armed, "Indicator: Zone 5 Armed"},
				{ValueID_Index_Indicator::Zone_6_Armed, "Indicator: Zone 6 Armed"},
				{ValueID_Index_Indicator::Zone_7_Armed, "Indicator: Zone 7 Armed"},
				{ValueID_Index_Indicator::Zone_8_Armed, "Indicator: Zone 8 Armed"},
				{ValueID_Index_Indicator::LCD_Backlight, "Indicator: LCD Backlight"},
				{ValueID_Index_Indicator::Button_Backlight_Letters, "Indicator: Button: Backlight Letters"},
				{ValueID_Index_Indicator::Button_Backlight_Digits, "Indicator: Button: Backlight Digits"},
				{ValueID_Index_Indicator::Button_Backlight_Command, "Indicator: Button: Backlight Command"},
				{ValueID_Index_Indicator::Button_1_Indication, "Indicator: Button 1"},
				{ValueID_Index_Indicator::Button_2_Indication, "Indicator: Button 2"},
				{ValueID_Index_Indicator::Button_3_Indication, "Indicator: Button 3"},
				{ValueID_Index_Indicator::Button_4_Indication, "Indicator: Button 4"},
				{ValueID_Index_Indicator::Button_5_Indication, "Indicator: Button 5"},
				{ValueID_Index_Indicator::Button_6_Indication, "Indicator: Button 6"},
				{ValueID_Index_Indicator::Button_7_Indication, "Indicator: Button 7"},
				{ValueID_Index_Indicator::Button_8_Indication, "Indicator: Button 8"},
				{ValueID_Index_Indicator::Button_9_Indication, "Indicator: Button 9"},
				{ValueID_Index_Indicator::Button_10_Indication, "Indicator: Button 10"},
				{ValueID_Index_Indicator::Button_11_Indication, "Indicator: Button 11"},
				{ValueID_Index_Indicator::Button_12_Indication, "Indicator: Button 12"},
				{ValueID_Index_Indicator::Node_Identify, "Indicator: Node Identify"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_1, "Indicator: Generic Event Sound Notification 1"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_2, "Indicator: Generic Event Sound Notification 2"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_3, "Indicator: Generic Event Sound Notification 3"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_4, "Indicator: Generic Event Sound Notification 4"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_5, "Indicator: Generic Event Sound Notification 5"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_6, "Indicator: Generic Event Sound Notification 6"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_7, "Indicator: Generic Event Sound Notification 7"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_8, "Indicator: Generic Event Sound Notification 8"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_9, "Indicator: Generic Event Sound Notification 9"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_10, "Indicator: Generic Event Sound Notification 10"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_11, "Indicator: Generic Event Sound Notification 11"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_12, "Indicator: Generic Event Sound Notification 12"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_13, "Indicator: Generic Event Sound Notification 13"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_14, "Indicator: Generic Event Sound Notification 14"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_15, "Indicator: Generic Event Sound Notification 15"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_16, "Indicator: Generic Event Sound Notification 16"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_17, "Indicator: Generic Event Sound Notification 17"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_18, "Indicator: Generic Event Sound Notification 18"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_19, "Indicator: Generic Event Sound Notification 19"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_20, "Indicator: Generic Event Sound Notification 20"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_21, "Indicator: Generic Event Sound Notification 21"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_22, "Indicator: Generic Event Sound Notification 22"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_23, "Indicator: Generic Event Sound Notification 23"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_24, "Indicator: Generic Event Sound Notification 24"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_25, "Indicator: Generic Event Sound Notification 25"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_26, "Indicator: Generic Event Sound Notification 26"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_27, "Indicator: Generic Event Sound Notification 27"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_28, "Indicator: Generic Event Sound Notification 28"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_29, "Indicator: Generic Event Sound Notification 29"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_30, "Indicator: Generic Event Sound Notification 30"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_31, "Indicator: Generic Event Sound Notification 31"},
				{ValueID_Index_Indicator::Generic_Event_Sound_Notification_32, "Indicator: Generic Event Sound Notification 32"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_1, "Indicator: Manufacturer Defined Indicator 1"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_2, "Indicator: Manufacturer Defined Indicator 2"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_3, "Indicator: Manufacturer Defined Indicator 3"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_4, "Indicator: Manufacturer Defined Indicator 4"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_5, "Indicator: Manufacturer Defined Indicator 5"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_6, "Indicator: Manufacturer Defined Indicator 6"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_7, "Indicator: Manufacturer Defined Indicator 7"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_8, "Indicator: Manufacturer Defined Indicator 8"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_9, "Indicator: Manufacturer Defined Indicator 9"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_10, "Indicator: Manufacturer Defined Indicator 10"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_11, "Indicator: Manufacturer Defined Indicator 11"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_12, "Indicator: Manufacturer Defined Indicator 12"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_13, "Indicator: Manufacturer Defined Indicator 13"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_14, "Indicator: Manufacturer Defined Indicator 14"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_15, "Indicator: Manufacturer Defined Indicator 15"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_16, "Indicator: Manufacturer Defined Indicator 16"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_17, "Indicator: Manufacturer Defined Indicator 17"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_18, "Indicator: Manufacturer Defined Indicator 18"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_19, "Indicator: Manufacturer Defined Indicator 19"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_20, "Indicator: Manufacturer Defined Indicator 20"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_21, "Indicator: Manufacturer Defined Indicator 21"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_22, "Indicator: Manufacturer Defined Indicator 22"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_23, "Indicator: Manufacturer Defined Indicator 23"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_24, "Indicator: Manufacturer Defined Indicator 24"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_25, "Indicator: Manufacturer Defined Indicator 25"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_26, "Indicator: Manufacturer Defined Indicator 26"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_27, "Indicator: Manufacturer Defined Indicator 27"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_28, "Indicator: Manufacturer Defined Indicator 28"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_29, "Indicator: Manufacturer Defined Indicator 29"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_30, "Indicator: Manufacturer Defined Indicator 30"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_31, "Indicator: Manufacturer Defined Indicator 31"},
				{ValueID_Index_Indicator::Manufacturer_Defined_Indicator_32, "Indicator: Manufacturer Defined Indicator 32"},
				{ValueID_Index_Indicator::Buzzer, "Indicator: Buzzer"}
			};
enum Indicator_Property_Bit { 
	MultiLevel_Bit = 0x02,
	Binary_Bit = 0x04,
	OnOffPeriod_Bit = 0x08,
	OnOffCycle_Bit = 0x10,
	OnTimeWithPeriod_Bit = 0x20,
	Timeout_Min_Bit = 0x40,
	Timeout_Sec_Bit = 0x80,
	Timeout_MS_Bit = 0x100,
	SoundLevel_Bit = 0x200,
	LowPower_Bit = 0x400
};

enum Indicator_Property_Group {
	MultiLevel_Grp = 0x01,
	Binary_Grp = 0x02,
	Toogle_Grp = 0x03,
	Timeout_Grp = 0x04,
	Sound_Grp = 0x05,
	Advertised_Grp = 0x06
};
enum Indicator_Property_offset {
	Multilevel_Prop = 0x01,
	Binary_Prop = 0x02,
	OnOffPeriod_Prop = 0x03,
	OnOffCycle_Prop = 0x04,
	OnTimeWithPeriod_Prop = 0x05,
	Timeout_Min_Prop = 0x06,
	Timeout_Sec_Prop = 0x07,
	Timeout_Ms_Prop = 0x08,
	Sound_Prop = 0x09,
	Adv_Low_Power_Prop = 0x10
};


//-----------------------------------------------------------------------------
// <Indicator::Indicator>
// Constructor
//-----------------------------------------------------------------------------
			Indicator::Indicator(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId)
			{ 
				Timer::SetDriver(GetDriver());
			}

//-----------------------------------------------------------------------------
// <Indicator::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool Indicator::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool ret = false;
				if (_requestFlags & RequestFlag_Dynamic)
				{

					if (GetVersion() == 1)
						return RequestValue(_requestFlags, 0, _instance, _queue);
					for (int i = 1; i <= ValueID_Index_Indicator::Buzzer; i++) {
						if (Internal::VC::Value *value = GetValue(_instance, i))
						{
							ret |= RequestValue(_requestFlags, i, _instance, _queue);
							value->Release();
						}
					}
				}
				if (GetVersion() > 1) 
				{
					if (_requestFlags & RequestFlag_Static)
					{
						Msg* msg = new Msg("IndicatorCmd_Supported_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(IndicatorCmd_Supported_Get);
						/* get the first Supported Indicator */
						msg->Append(0);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						ret = true;
					}
				}
				return ret;
			}

//-----------------------------------------------------------------------------
// <Indicator::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool Indicator::RequestValue(uint32 const _requestFlags, uint16 const index,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					if (index == ValueID_Index_Indicator::Indicator) 
					{
						Msg* msg = new Msg("IndicatorCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(IndicatorCmd_Get);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						return true;
					} 
					else if (index > ValueID_Index_Indicator::Indicator && index <= ValueID_Index_Indicator::Buzzer)
					{
						Msg* msg = new Msg("IndicatorCmd_Get_v2", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(IndicatorCmd_Get);
						msg->Append(index);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						return true;
					}
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "IndicatorCmd_Get Not Supported on this node");
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Indicator::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Indicator::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (IndicatorCmd_Supported_Report == (IndicatorCmd) _data[0]) 
				{
					uint8 id = _data[1];
					uint8 nextid = _data[2];
					uint8 propertiessupportedlength = (_data[3] & 0x1F);
					uint32 propertiessupported = 0;
					for (int i = 0; i < propertiessupportedlength; i++) 
					{
						propertiessupported = _data[4+i] << (8*i);
					}
					Log::Write(LogLevel_Info, GetNodeId(), "Indicator Supported Report for %d - Support %d - Next Indicator %d", id, propertiessupported, nextid);
					std::shared_ptr<Properties> idprops = std::make_shared<Properties>();
					idprops->id = id;
					idprops->instance = _instance;
					idprops->properties = propertiessupported;
					this->m_indicatorLists.insert(std::pair<uint8, std::shared_ptr<Properties> >(id, idprops));

					if ((GetVersion() >= 4) && (id >= 0x80) && (id <= 0x9f))
					{
						Msg* msg = new Msg("IndicatorCmd_Description_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(IndicatorCmd_Description_Get);
						msg->Append(id);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					}
					else 
					{
						this->createIndicatorConfigValues(id);
					}

					/* Get the Next Indicator */
					if (nextid > 0)
					{
						Msg* msg = new Msg("IndicatorCmd_Supported_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(IndicatorCmd_Supported_Get);
						msg->Append(nextid);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Query);
						return true;
					}
					return true;
				}
				if (IndicatorCmd_Report == (IndicatorCmd) _data[0])
				{
					if (GetVersion() == 1) { 
						Log::Write(LogLevel_Info, GetNodeId(), "Received an Indicator report: Indicator=%d", _data[1]);
						if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_Indicator::Indicator)))
						{
							value->OnValueRefreshed(_data[1]);
							value->Release();
						}
						return true;
					} else {
						uint8 size = (_data[2] & 0x1F);
						uint8 setid = 0;
						uint8 setparam = 0;
						for (int i = 0; i < size; i++) 
						{
							uint8 id = _data[3 + (i*3)];
							uint8 property = _data[4 + (i*3)];
							uint8 value = _data[5 + (i*3)];
							Log::Write(LogLevel_Info, GetNodeId(), "Indicator Report for %d - Property %d - Value %d", id, property, value);
							this->setIndicatorValue(id, _instance, property, value);
							/* 4.24.5 - ID Field:
							 * All indicator objects MUST carry the same Indicator ID.
							 */
							if (setid == 0)
								setid = id;
							if (value > 0)
								setparam = property;
						}
						if (Internal::VC::ValueList* value = static_cast<Internal::VC::ValueList *>(GetValue(_instance, setid)))
						{
							bool setTimer = true;
							if (setparam == Indicator_Property_offset::Multilevel_Prop)
							{
								value->OnValueRefreshed(Indicator_Property_Group::MultiLevel_Grp);
								setTimer = false;
							}
							else if (setparam == Indicator_Property_offset::Binary_Prop)
							{
								value->OnValueRefreshed(Indicator_Property_Group::Binary_Grp);
								setTimer = false;
							}
							else if (setparam >= Indicator_Property_offset::OnOffPeriod_Prop && setparam <= Indicator_Property_offset::OnTimeWithPeriod_Prop)
								value->OnValueRefreshed(Indicator_Property_Group::Toogle_Grp);
							else if (setparam >= Indicator_Property_offset::Timeout_Min_Prop && setparam <= Indicator_Property_offset::Timeout_Ms_Prop)
								value->OnValueRefreshed(Indicator_Property_Group::Timeout_Grp);
							else if (setparam == Indicator_Property_offset::Sound_Prop)
								value->OnValueRefreshed(Indicator_Property_Group::Sound_Grp);
							else 
							{
								value->OnValueRefreshed(0);
								setTimer = false;
							}
							value->Release();
							if (setTimer) 
							{
								int32 id = setid + (_instance << 16);
								TimerThread::TimerCallback callback = bind(&Indicator::refreshIndicator, this, id);
								TimerSetEvent(1000, callback, id);
							}
						}
						else
						{
							Log::Write(LogLevel_Warning, GetNodeId(), "Can't Find ValueID for Indicator %d", setid);
						}

					}
					return true;
				}
				if (IndicatorCmd_Description_Report == (IndicatorCmd) _data[0])
				{
					uint8 id = _data[1];
					uint8 length = _data[2];
					string label("Unknown Indicator");
					if (m_indicatorLists.find(id) != m_indicatorLists.end())
					{
						if (length == 0) 
						{
							/* use the default label */
							if (IndicatorTypes.find(id) != IndicatorTypes.end()) {
								m_indicatorLists[id]->label = IndicatorTypes.at(id);
							}
						}
						else 
						{
							string description((const char *) &_data[3], length);
							if (!description.empty())
								m_indicatorLists[id]->label = description;
						}
						this->createIndicatorConfigValues(id);
					}
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Indicator::SetValue>
// Set the device's indicator value
//-----------------------------------------------------------------------------
			bool Indicator::SetValue(Internal::VC::Value const& _value)
			{
				
				if (ValueID_Index_Indicator::Indicator == _value.GetID().GetIndex())
				{
					Internal::VC::ValueByte const* value = static_cast<Internal::VC::ValueByte const*>(&_value);

					Log::Write(LogLevel_Info, GetNodeId(), "Indicator::SetValue - Setting indicator to %d", value->GetValue());
					Msg* msg = new Msg("IndicatorCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->SetInstance(this, _value.GetID().GetInstance());
					msg->Append(GetNodeId());
					msg->Append(3);
					msg->Append(GetCommandClassId());
					msg->Append(IndicatorCmd_Set);
					msg->Append(value->GetValue());
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					return true;
				}
				if (_value.GetID().GetIndex() > 1 && _value.GetID().GetIndex() <= ValueID_Index_Indicator::Buzzer) {
					Internal::VC::ValueList const* value = static_cast<Internal::VC::ValueList const*>(&_value);
					uint32 selected = value->GetItem()->m_value;
					uint16 index = _value.GetID().GetIndex();
					/* depending upon what is selected, we can get a Property Config ValueID's */
					uint32 property = 1 << selected;
					uint32 propertyindex = (256 + (32 * index));
					Log::Write(LogLevel_Info, GetNodeId(), "Setting Index: %d Selected %d Property %d PropertyIndex %d", index, selected, property, propertyindex);
					vector<uint8> payload;
					switch (selected) {
						case 0:
							{ 
								/* turn off any indicator */
								Msg* msg = new Msg("IndicatorCmd_Set_v2", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
								msg->SetInstance(this, _value.GetID().GetInstance());
								msg->Append(GetNodeId());
								msg->Append(3);
								msg->Append(GetCommandClassId());
								msg->Append(IndicatorCmd_Set);
								msg->Append(0);
								msg->Append(GetDriver()->GetTransmitOptions());
								GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
								return true;
								break;
							}
						case Indicator_Property_Group::Binary_Grp:
						{
							if (Internal::VC::ValueBool* propertyValue = static_cast<Internal::VC::ValueBool *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::Binary_Prop)))
							{
								payload.push_back(1);
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::Binary_Prop);
								payload.push_back(propertyValue->GetValue() == true ? 1 : 0);
							}
							break;
						}
						case Indicator_Property_Group::MultiLevel_Grp:
						{
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::Multilevel_Prop)))
							{
								payload.push_back(1);
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::Multilevel_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							break;
						}
						case Indicator_Property_Group::Toogle_Grp:
						{
							int params = 0;
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::OnOffPeriod_Prop)))
							{
								params++;
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::OnOffPeriod_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::OnOffCycle_Prop)))
							{
								params++;
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::OnOffCycle_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::OnTimeWithPeriod_Prop)))
							{
								params++;
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::OnTimeWithPeriod_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							payload.insert(payload.begin(), params);
							break;
						}
						case Indicator_Property_Group::Timeout_Grp:
						{
							int params = 0;
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::Timeout_Min_Prop)))
							{
								params++;
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::Timeout_Min_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::Timeout_Sec_Prop)))
							{
								params++;
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::Timeout_Sec_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::Timeout_Ms_Prop)))
							{
								params++;
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::Timeout_Ms_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							payload.insert(payload.begin(), params);
							break;
						}
						case Indicator_Property_Group::Sound_Grp:
						{
							if (Internal::VC::ValueByte* propertyValue = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), propertyindex + Indicator_Property_offset::Sound_Prop)))
							{
								payload.push_back(1);
								payload.push_back(index);
								payload.push_back(Indicator_Property_offset::Sound_Prop);
								payload.push_back(propertyValue->GetValue());
							}
							break;

						}
						case Indicator_Property_Group::Advertised_Grp:
						{
								/* shouldn't ever be present */
								break;
						}
					}
					Msg* msg = new Msg("IndicatorCmd_Set_v2", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->SetInstance(this, _value.GetID().GetInstance());
					msg->Append(GetNodeId());
					msg->Append(3 + payload.size());
					msg->Append(GetCommandClassId());
					msg->Append(IndicatorCmd_Set);
					msg->Append(0);
					for (unsigned int i = 0; i < payload.size(); i++) {
						msg->Append(payload.at(i));
					}
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					return true;
				}
				if (_value.GetID().GetIndex() > ValueID_Index_Indicator::Buzzer)
				{
					if (_value.GetID().GetType() == ValueID::ValueType_Byte) 
					{
						if (Internal::VC::ValueByte * valueByte = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), _value.GetID().GetIndex())))
						{
							valueByte->OnValueRefreshed(static_cast<Internal::VC::ValueByte const *>(&_value)->GetValue());
							valueByte->Release();
						}
					}
					if (_value.GetID().GetType() == ValueID::ValueType_Bool)
					{
						if (Internal::VC::ValueBool * valueBool = static_cast<Internal::VC::ValueBool *>(GetValue(_value.GetID().GetInstance(), _value.GetID().GetIndex())))
						{
							valueBool->OnValueRefreshed(static_cast<Internal::VC::ValueBool const *>(&_value)->GetValue());
							valueBool->Release();
						}
					}
					return true;
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Indicator::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void Indicator::CreateVars(uint8 const _instance)
			{
				if (GetVersion() == 1) { 
					if (Node* node = GetNodeUnsafe())
					{
						node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_Indicator::Indicator, "Indicator", "", false, false, false, 0);
					}
				}
			}
//-----------------------------------------------------------------------------
// <Indicator::createIndicatorConfigValues>
// Create the Config Values for each Indicator ID
//-----------------------------------------------------------------------------

			void Indicator::createIndicatorConfigValues(uint8 id) {
				/* Value Indexes are as follows:
				 * Assume that we can have 32 Properties per Index 
				 * 0 - 256 - Actual Indicator ValueID
				 * 257 - 288 - Indicator 1 Properties 
				 * 289 - 320 - Indicator 2 Properties
				 * 321 - 352 - Indicator 3 Properties 
				 * and so on. */
				if (m_indicatorLists.find(id) == m_indicatorLists.end()) 
				{
					Log::Write(LogLevel_Warning, GetNodeId(), "Cant find Indicator %d in List", id);
					return;
				}
				Log::Write(LogLevel_Info, GetNodeId(), "Indicator Support:");
				if (Node* node = GetNodeUnsafe())
				{
					string label("Unknown Indicator");
					if (m_indicatorLists[id]->label.empty())
					{
						if (IndicatorTypes.find(id) != IndicatorTypes.end()) {
							label = IndicatorTypes.at(id);
						}
					}
					else
					{
						label = m_indicatorLists[id]->label;
					}
					
					uint32 propertiesSet = 0;

					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::MultiLevel_Bit) {
						/* MultiLevel Property */
						Log::Write(LogLevel_Info, GetNodeId(), "\tMultiLevel Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::Multilevel_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::Multilevel_Prop), label + ": Brightness/Level", "", false, false, 0, 0);
						propertiesSet |= (1 << Indicator_Property_Group::MultiLevel_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::Binary_Bit) 
					{
						/* Binary Property */
						Log::Write(LogLevel_Info, GetNodeId(), "\tBinary  Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::Binary_Prop));
						node->CreateValueBool(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::Binary_Prop), label + ": On/Off", "", false, false, false, 0);
						propertiesSet |= (1 << Indicator_Property_Group::Binary_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::OnOffPeriod_Bit)
					{
						/* On/Off Period Property */
						Log::Write(LogLevel_Info, GetNodeId(), "\tOn/Off Period Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::OnOffPeriod_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::OnOffPeriod_Prop), label + ": On/Off Period", "Seconds", false, false, 0, 0);
						propertiesSet |= (1 << Indicator_Property_Group::Toogle_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::OnOffCycle_Bit)
					{
						/* On/Off Cycles */
						Log::Write(LogLevel_Info, GetNodeId(), "\tOn/Off Cycles Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::OnOffCycle_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::OnOffCycle_Prop), label + ": On/Off Cycles", "", false, false, 0, 0);
						propertiesSet |= (1 << Indicator_Property_Group::Toogle_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::OnTimeWithPeriod_Bit)
					{
						/* On Time with On/Off Period combination */
						Log::Write(LogLevel_Info, GetNodeId(), "\tOn Time with On/Off Period Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::OnTimeWithPeriod_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::OnTimeWithPeriod_Prop), label + ": On time within an On/Off period", "Seconds", false, false, 0, 0); 
						propertiesSet |= (1 << Indicator_Property_Group::Toogle_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::Timeout_Min_Bit)
					{
						/* Timeout (Minutes) */
						Log::Write(LogLevel_Info, GetNodeId(), "\tTimeout (Minutes) Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::Timeout_Min_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::Timeout_Min_Prop), label + ": Timeout (Minutes)", "Minutes", false, false, 0, 0);
						propertiesSet |= (1 << Indicator_Property_Group::Timeout_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::Timeout_Sec_Bit)
					{
						/* Timeout (Seconds) */
						Log::Write(LogLevel_Info, GetNodeId(), "\tTimeout (Seconds) Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::Timeout_Sec_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::Timeout_Sec_Prop), label + ": Timeout (Seconds)", "Seconds", false, false, 0, 0);
						propertiesSet |= (1 << Indicator_Property_Group::Timeout_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::Timeout_MS_Bit)
					{
						/* Timeout (1/100 of seconds) */
						Log::Write(LogLevel_Info, GetNodeId(), "\tTimeout (1/100 of seconds) Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::Timeout_Ms_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::Timeout_Ms_Prop), label + ": Timeout (1/100 of seconds)", "Miliseconds", false, false, 0, 0); 
						propertiesSet |= (1 << Indicator_Property_Group::Timeout_Grp);
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::SoundLevel_Bit)
					{
						/* Multilevel Sound level */
						Log::Write(LogLevel_Info, GetNodeId(), "\tMultilevel Sound Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::Sound_Prop));
						node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::Sound_Prop), label + ": Multilevel Sound level", "", false, false, 0, 0);
						propertiesSet |= Indicator_Property_Group::Sound_Grp;
					}
					if (m_indicatorLists[id]->properties & Indicator_Property_Bit::LowPower_Bit)
					{
						/* ADVERTISE: Low power */
						Log::Write(LogLevel_Info, GetNodeId(), "\tADVERTISE: Low power Property - Index %d", (256 + (32 * id) + Indicator_Property_offset::Adv_Low_Power_Prop));
						node->CreateValueBool(ValueID::ValueGenre_Config, GetCommandClassId(), m_indicatorLists[id]->instance, (256 + (32 * id) + Indicator_Property_offset::Adv_Low_Power_Prop), label + ": Low Power Capable", "", true, false, false, 0);
						propertiesSet |= Indicator_Property_Group::Advertised_Grp;
					}
					/* Create the Actual Indicator */
					vector<Internal::VC::ValueList::Item> items;
					Internal::VC::ValueList::Item item;
					item.m_label = "Off";
					item.m_value = 0x00;
					items.push_back(item);
					if (propertiesSet & (1 << Indicator_Property_Group::Binary_Grp)) 
					{
						Internal::VC::ValueList::Item item;
						item.m_label = "On";
						item.m_value = Indicator_Property_Group::Binary_Grp;
						items.push_back(item);
					}					
					if (propertiesSet & (1 << Indicator_Property_Group::MultiLevel_Grp)) 
					{
						Internal::VC::ValueList::Item item;
						item.m_label = "Level";
						item.m_value = Indicator_Property_Group::MultiLevel_Grp;
						items.push_back(item);
					}					
					if (propertiesSet & (1 << Indicator_Property_Group::Toogle_Grp)) 
					{
						Internal::VC::ValueList::Item item;
						item.m_label = "Toogle";
						item.m_value = Indicator_Property_Group::Toogle_Grp;
						items.push_back(item);
					}					
					if (propertiesSet & (1 << Indicator_Property_Group::Timeout_Grp)) 
					{
						Internal::VC::ValueList::Item item;
						item.m_label = "Timeout";
						item.m_value = Indicator_Property_Group::Timeout_Grp;
						items.push_back(item);
					}					
					if (propertiesSet & (1 << Indicator_Property_Group::Sound_Grp)) 
					{
						Internal::VC::ValueList::Item item;
						item.m_label = "Sound Level";
						item.m_value = Indicator_Property_Group::Sound_Grp;
						items.push_back(item);
					}	
					node->CreateValueList(ValueID::ValueGenre_User, GetCommandClassId(), m_indicatorLists[id]->instance, id, label, "", false, false, 0, items, 0, 0);					
				}
			}

//-----------------------------------------------------------------------------
// <Indicator::setIndicatorValue>
// Sets the Properties Values for each Indicator ID
//-----------------------------------------------------------------------------

			void Indicator::setIndicatorValue(uint8 id, uint8 _instance, uint8 property, uint8 value) 
			{
				uint16 index = (256 + (32 * id) + property);
				Internal::VC::Value* propertyValue = GetValue(_instance, index);
				if (!propertyValue) {
					Log::Write(LogLevel_Warning, GetNodeId(), "Got Indicator Property for a Unregistered Property - Id: %d Property: %d Index: %d", id, property, index);
					return;
				}
				switch (propertyValue->GetID().GetType()) {
					case ValueID::ValueType_Bool:
					{
						if (Internal::VC::ValueBool *propertyBool = static_cast<Internal::VC::ValueBool *>(propertyValue))
						{ 
							propertyBool->OnValueRefreshed(value > 0 ? true : false);
						}
						propertyValue->Release();
						break;
					}
					case ValueID::ValueType_Byte:
					{
						if (Internal::VC::ValueByte *propertyByte = static_cast<Internal::VC::ValueByte *>(propertyValue))
						{ 
							propertyByte->OnValueRefreshed(value);
						}
						propertyValue->Release();
						break;
					}
					default:
					{
						/* Not Handled */
						Log::Write(LogLevel_Warning, GetNodeId(), "Got Indicator Property for a Unhandled Property Type %d", propertyValue->GetID().GetType());
						break;
					}
				}
			}

//-----------------------------------------------------------------------------
// <Indicator::Indicator>
// Refresh The Indicator ID's Values. this is called from the timer when a indicator is active. 
//-----------------------------------------------------------------------------
			void Indicator::refreshIndicator(uint32 id)
			{
				uint16 index = (id & 0xFFFF);
				uint8 instance = (id & 0xF0000) >> 16;
				RequestValue(0, index, instance, Driver::MsgQueue_Query);		
			}

//-----------------------------------------------------------------------------
// <SwitchBinary::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
			void Indicator::SetValueBasic(uint8 const _instance, uint8 const _value)
			{
				// Send a request for new value to synchronize it with the BASIC set/report.
				// In case the device is sleeping, we set the value anyway so the BASIC set/report
				// stays in sync with it. We must be careful mapping the uint8 BASIC value
				// into a class specific value.
				// When the device wakes up, the real requested value will be retrieved.
				if (GetVersion() == 1) 
				{
					RequestValue(0, 0, _instance, Driver::MsgQueue_Query);
					return;
				}
				for (int i = 1; i <= ValueID_Index_Indicator::Buzzer; i++) {
					if (Internal::VC::Value *value = GetValue(_instance, i))
					{
						RequestValue(0, i, _instance, Driver::MsgQueue_Query);
						value->Release();
					}
				}
				return;
			}


		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
