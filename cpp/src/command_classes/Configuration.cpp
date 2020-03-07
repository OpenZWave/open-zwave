//-----------------------------------------------------------------------------
//
//	Configuration.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CONFIGURATION
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
#include "command_classes/Configuration.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Notification.h"
#include "platform/Log.h"
#include "value_classes/ValueBitSet.h"
#include "value_classes/ValueBool.h"
#include "value_classes/ValueButton.h"
#include "value_classes/ValueByte.h"
#include "value_classes/ValueInt.h"
#include "value_classes/ValueList.h"
#include "value_classes/ValueShort.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum ConfigurationCmd
			{
				ConfigurationCmd_Defaults_Reset = 0x01,
				ConfigurationCmd_Set = 0x04,
				ConfigurationCmd_Get = 0x05,
				ConfigurationCmd_Report = 0x06,
				ConfigurationCmd_Bulk_Set = 0x07,
				ConfigurationCmd_Bulk_Get = 0x08,
				ConfigurationCmd_Bulk_Report = 0x09,
				ConfigurationCmd_Name_Get = 0x0A,
				ConfigurationCmd_Name_Report = 0x0B,
				ConfigurationCmd_Info_Get = 0x0C,
				ConfigurationCmd_Info_Report = 0x0D,
				ConfigurationCmd_Properties_Get = 0x0E,
				ConfigurationCmd_Properties_Report = 0x0F
			};

			uint32 Configuration::getField(const uint8 *data, CC_Param_Size size, uint8 &pos) {
				uint32 value = 0;
				switch (size) {
					case CC_Param_Size::CC_Param_Size_Byte:
						value = data[pos++];
						break;
					case CC_Param_Size::CC_Param_Size_Short:
						value = (data[pos++] << 8) + data[pos++];
						break;
					case CC_Param_Size::CC_Param_Size_Int:
						value = (data[pos++] << 24) + (data[pos++] << 16) + (data[pos++] << 8) + data[pos++];
						break;
					case CC_Param_Size::CC_Param_Size_Unassigned:
						value = 0;
						break;
					default:
						Log::Write(LogLevel_Error, "Unhandled CC_Param_Size %u", size);
						break;
				}
				return value;
			}

//-----------------------------------------------------------------------------
// <Configuration::ReadXML>
// Read the saved association data
//-----------------------------------------------------------------------------
			void Configuration::ReadXML(TiXmlElement const* _ccElement)
			{
				CommandClass::ReadXML(_ccElement);

				TiXmlElement const* configParamsElement = _ccElement->FirstChildElement();
				while (configParamsElement)
				{
					char const* str = configParamsElement->Value();
					if (str && !strcmp(str, "ConfigParams"))
					{
						TiXmlElement const* configParam = configParamsElement->FirstChildElement();
						while (configParam) { 
							ConfigParam param;
							int value;
							if (TIXML_SUCCESS != configParam->QueryIntAttribute("index", &value)) {
								Log::Write(LogLevel_Warning, GetNodeId(), "Missing Index Value on ConfigParam at Row %d", configParam->Row());
								configParam = configParam->NextSiblingElement();		
								continue;
							} else {
								param.paramNo = (int16)(value & 0xFFFF);
							}

							char const *str2 = configParam->Attribute("altering");
							if (!str2) {
								Log::Write(LogLevel_Warning, GetNodeId(), "Missing altering Value on ConfigParam at Row %d", configParam->Row());
								configParam = configParam->NextSiblingElement();		
								continue;
							} else {
								param.altering = !strcmp(str2, "true");
							}
							str2 = configParam->Attribute("advanced");
							if (!str2) {
								Log::Write(LogLevel_Warning, GetNodeId(), "Missing advanced Value on ConfigParam at Row %d", configParam->Row());
								configParam = configParam->NextSiblingElement();		
								continue;
							} else {
								param.advanced = !strcmp(str2, "true");
							}
							str2 = configParam->Attribute("nobulk");
							if (!str2) {
								Log::Write(LogLevel_Warning, GetNodeId(), "Missing nobulk Value on ConfigParam at Row %d", configParam->Row());
								configParam = configParam->NextSiblingElement();		
								continue;
							} else {
								param.nobulk = !strcmp(str2, "true");
							}
							if (TIXML_SUCCESS != configParam->QueryIntAttribute("default", &value)) {
								Log::Write(LogLevel_Warning, GetNodeId(), "Missing default Value on ConfigParam at Row %d", configParam->Row());
								configParam = configParam->NextSiblingElement();		
								continue;
							} else {
								param.defaultval = value;
							}
							m_ConfigParams[param.paramNo] = param;
							configParam = configParam->NextSiblingElement();
						}
					}

					configParamsElement = configParamsElement->NextSiblingElement();
				}
			}

//-----------------------------------------------------------------------------
// <Configuration::WriteXML>
// Save the association data
//-----------------------------------------------------------------------------
			void Configuration::WriteXML(TiXmlElement* _ccElement)
			{
				CommandClass::WriteXML(_ccElement);

				TiXmlElement* configParamsElement = new TiXmlElement("ConfigParams");
				for (std::map<uint16, ConfigParam>::iterator it = m_ConfigParams.begin(); it != m_ConfigParams.end(); it++) { 
					TiXmlElement* configElement = new TiXmlElement("ConfigParam");
					char str[32];
					snprintf(str, sizeof(str), "%d", it->first);
					configElement->SetAttribute("index", str);
					configElement->SetAttribute("altering", it->second.altering ? "true" : "false");
					configElement->SetAttribute("advanced", it->second.advanced ? "true" : "false");
					configElement->SetAttribute("nobulk", it->second.nobulk ? "true" : "false");
					snprintf(str, sizeof(str), "%d", it->second.defaultval);
					configElement->SetAttribute("default", str);
					configParamsElement->LinkEndChild(configElement);
				}
				_ccElement->LinkEndChild(configParamsElement);
			}


//-----------------------------------------------------------------------------
// <Configuration::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Configuration::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if ( (ConfigurationCmd_Report == (ConfigurationCmd) _data[0]) || (ConfigurationCmd_Bulk_Report == (ConfigurationCmd) _data[0]) )
				{
					bool bulk = (_data[0] == ConfigurationCmd_Bulk_Report);
					uint16 parameter;
					uint8 size;
					uint32 paramValue = 0;
					// Extract the parameter index and value
					if (bulk)
					{
						parameter = (_data[1] << 8) + (_data[2]);
						size = _data[5] & 0x07;
						for (uint8 i = 0; i < size; ++i)
						{
							paramValue <<= 8;
							paramValue |= (int32) _data[i + 6];
						}
					}
					else 
					{
						parameter = _data[1];
						size = _data[2] & 0x07;
						for (uint8 i = 0; i < size; ++i)
						{
							paramValue <<= 8;
							paramValue |= (int32) _data[i + 3];
						}
					}
					if (Internal::VC::Value* value = GetValue(1, parameter))
					{
						switch (value->GetID().GetType())
						{
							case ValueID::ValueType_BitSet:
							{
								Internal::VC::ValueBitSet* vbs = static_cast<Internal::VC::ValueBitSet*>(value);
								vbs->OnValueRefreshed(paramValue);
								break;
							}
							case ValueID::ValueType_Bool:
							{
								Internal::VC::ValueBool* valueBool = static_cast<Internal::VC::ValueBool*>(value);
								valueBool->OnValueRefreshed(paramValue != 0);
								break;
							}
							case ValueID::ValueType_Byte:
							{
								Internal::VC::ValueByte* valueByte = static_cast<Internal::VC::ValueByte*>(value);
								valueByte->OnValueRefreshed((uint8) paramValue);
								break;
							}
							case ValueID::ValueType_Short:
							{
								Internal::VC::ValueShort* valueShort = static_cast<Internal::VC::ValueShort*>(value);
								valueShort->OnValueRefreshed((int16) paramValue);
								break;
							}
							case ValueID::ValueType_Int:
							{
								Internal::VC::ValueInt* valueInt = static_cast<Internal::VC::ValueInt*>(value);
								valueInt->OnValueRefreshed(paramValue);
								break;
							}
							case ValueID::ValueType_List:
							{
								Internal::VC::ValueList* valueList = static_cast<Internal::VC::ValueList*>(value);
								valueList->OnValueRefreshed(paramValue);
								break;
							}
							default:
							{
								Log::Write(LogLevel_Info, GetNodeId(), "Invalid type (%d) for configuration parameter %d", value->GetID().GetType(), parameter);
							}
						}
						value->Release();
					}
					else
					{
						char label[16];
						snprintf(label, 16, "Parameter #%d", parameter);

						// Create a new value
						if (Node* node = GetNodeUnsafe())
						{
							switch (size)
							{
								case 1:
								{
									node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), _instance, parameter, label, "", false, false, (uint8) paramValue, 0);
									break;
								}
								case 2:
								{
									node->CreateValueShort(ValueID::ValueGenre_Config, GetCommandClassId(), _instance, parameter, label, "", false, false, (int16) paramValue, 0);
									break;
								}
								case 4:
								{
									node->CreateValueInt(ValueID::ValueGenre_Config, GetCommandClassId(), _instance, parameter, label, "", false, false, (int32) paramValue, 0);
									break;
								}
								default:
								{
									Log::Write(LogLevel_Info, GetNodeId(), "Invalid size of %d bytes for configuration parameter %d", size, parameter);
								}
							}
						}
					}

					Log::Write(LogLevel_Info, GetNodeId(), "Received Configuration report: Parameter=%d, Value=%d", parameter, paramValue);
					return true;
				} 
				else if (ConfigurationCmd_Properties_Report == (ConfigurationCmd) _data[0])
				{
					uint16 paramNo = (_data[1] << 8) + _data[2];
					CC_Param_Format paramFormat = (CC_Param_Format)((_data[3] & 0x38) >> 3);
					CC_Param_Size paramSize = (CC_Param_Size)(_data[3] & 0x07);
					if (paramSize == CC_Param_Size::CC_Param_Size_Unassigned) {
						/* this is a non-existant Param, but the "next param" field holds the next Param Number. Go Get it... */
						Log::Write(LogLevel_Info, GetNodeId(), "First Configuration CC Param to Query is %d", ((_data[4] << 8) + _data[5]));
						{
							Msg* msg = new Msg("ConfigurationCmd_Properties_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->Append(GetNodeId());
							msg->Append(4);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Properties_Get);
							msg->Append(_data[4]);
							msg->Append(_data[5]);
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						}
						{
							Msg* msg = new Msg("ConfigurationCmd_Name_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->Append(GetNodeId());
							msg->Append(4);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Name_Get);
							msg->Append(_data[4]);
							msg->Append(_data[5]);
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						}
						{
							Msg* msg = new Msg("ConfigurationCmd_Info_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->Append(GetNodeId());
							msg->Append(4);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Info_Get);
							msg->Append(_data[4]);
							msg->Append(_data[5]);
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						}

						return true;
					}
					/* the length of each of the remaining fields depends upon the CC_Param_Size
					 * so we use a little helper function to extract them out 
					 */
					uint8 position = 4;
					ConfigParam param;
					param.paramNo = paramNo;
					param.min = getField(_data, paramSize, position);
					param.max = getField(_data, paramSize, position);
					param.defaultval = getField(_data, paramSize, position);
					param.format = paramFormat;

					param.flags = ConfigParamFlags_Info_Done;
					m_ConfigParams[paramNo] = param;
					uint16 nextParam = getField(_data, CC_Param_Size::CC_Param_Size_Short, position);
					if (GetVersion() >= 4) {
						param.readonly = (_data[3] & 0x40);
						param.altering = (_data[3] & 0x80);
						param.advanced = (_data[position] & 0x01);
						param.nobulk = (_data[position] & 0x02);
					} else {
						param.readonly = false;
						param.altering = false;
						param.advanced = false;
						param.nobulk = false;
					}
					Log::Write(LogLevel_Info, GetNodeId(), "Param %d Format: %d Size: %d Min: %d Max: %d Default %d ReadOnly %d Altering %d Advanced %d NoBulk %d Next %d", param.paramNo, param.format, paramSize, param.min, param.max, param.defaultval, param.readonly, param.altering, param.advanced, param.nobulk, nextParam);
					

					if (nextParam > 0) {
						{
							Msg* msg = new Msg("ConfigurationCmd_Properties_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->Append(GetNodeId());
							msg->Append(4);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Properties_Get);
							msg->Append(((nextParam & 0xFF00) >> 8));
							msg->Append((nextParam & 0xFF));
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						}
						{
							Msg* msg = new Msg("ConfigurationCmd_Name_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->Append(GetNodeId());
							msg->Append(4);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Name_Get);
							msg->Append(((nextParam & 0xFF00) >> 8));
							msg->Append((nextParam & 0xFF));
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						}
						{
							Msg* msg = new Msg("ConfigurationCmd_Info_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->Append(GetNodeId());
							msg->Append(4);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Info_Get);
							msg->Append(((nextParam & 0xFF00) >> 8));
							msg->Append((nextParam & 0xFF));
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						}
						return true;
					}
					return true;

				}
				else if (ConfigurationCmd_Name_Report == (ConfigurationCmd) _data[0])
				{
					uint16 paramNo = (_data[1] << 8) + _data[2];
					bool reportsToFollow = (_data[3] > 0);
					if (m_ConfigParams.find(paramNo) != m_ConfigParams.end()) {
						for (uint32 i = 4; i <= (_length -1); i++) {
							m_ConfigParams.at(paramNo).name += _data[i];
						}
						if (!reportsToFollow) {
							m_ConfigParams.at(paramNo).flags |= ConfigParamFlags_Name_Done;
						}
						uint8 moreInfo = (_data[3]);
						Log::Write(LogLevel_Info, GetNodeId(), "Param %d Name: %s (moreInfo %d, flags %d)", paramNo, m_ConfigParams.at(paramNo).name.c_str(), moreInfo, m_ConfigParams.at(paramNo).flags);
					} else {
						Log::Write(LogLevel_Warning, GetNodeId(), "Cant Find Config Param %d for NameReport", paramNo);
					}
					return true;
				}
				else if (ConfigurationCmd_Info_Report == (ConfigurationCmd) _data[0])
				{
					uint16 paramNo = (_data[1] << 8) + _data[2];
					bool reportsToFollow = (_data[3] > 0);
					if (m_ConfigParams.find(paramNo) != m_ConfigParams.end()) { 
						for (uint32 i = 4; i <= (_length -1); i++) {
							m_ConfigParams.at(paramNo).help += _data[i];
						}
						if (!reportsToFollow) {
							m_ConfigParams.at(paramNo).flags |= ConfigParamFlags_Help_Done;
						}
						uint8 moreInfo = (_data[3]);
						Log::Write(LogLevel_Info, GetNodeId(), "Param %d Help: %s (moreInfo: %d, flags %d)", paramNo, m_ConfigParams.at(paramNo).help.c_str(), moreInfo, m_ConfigParams.at(paramNo).flags);
						return processConfigParams(paramNo);
					} else {
						Log::Write(LogLevel_Warning, GetNodeId(), "Cant Find Config Param %d for InfoReport", paramNo);
					}
					return true;
				}
				Log::Write(LogLevel_Warning, GetNodeId(), "Unhandled Configuration Command: %d", _data[0]);
				return false;
			}

//-----------------------------------------------------------------------------
// <Configuration::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
			bool Configuration::SetValue(Internal::VC::Value const& _value)
			{
				uint16 param = _value.GetID().GetIndex();
				if ((GetVersion() >= 3) && (param == ValueID_Index_Configuration::ResetToDefault))
				{
					/* Reset all our Params */
					for (std::map<uint16, ConfigParam>::iterator it = m_ConfigParams.begin(); it != m_ConfigParams.end(); it++) 
					{
						if (it->first < 255) { 
							Msg* msg = new Msg("ConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
							msg->Append(GetNodeId());
							msg->Append(4 + it->second.size);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Set);
							msg->Append((it->first & 0xFF));
							msg->Append((it->second.size & 0x07) + 0x80);
							if (it->second.size > 2)
							{
								msg->Append((uint8) ((it->second.defaultval >> 24) & 0xff));
								msg->Append((uint8) ((it->second.defaultval >> 16) & 0xff));
							}
							if (it->second.size > 1)
							{
								msg->Append((uint8) ((it->second.defaultval >> 8) & 0xff));
							}
							msg->Append((uint8) (it->second.defaultval & 0xff));
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						} 
						else 
						{
							Msg* msg = new Msg("ConfigurationCmd_Bulk_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
							msg->Append(GetNodeId());
							msg->Append(6 + it->second.size);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Bulk_Set);
							msg->Append(((it->first & 0xFF00) >> 8));
							msg->Append((it->first & 0xFF));
							msg->Append(1);
							msg->Append((it->second.size & 0x07) + 0x80);
							if (it->second.size > 2)
							{
								msg->Append((uint8) ((it->second.defaultval >> 24) & 0xff));
								msg->Append((uint8) ((it->second.defaultval >> 16) & 0xff));
							}
							if (it->second.size > 1)
							{
								msg->Append((uint8) ((it->second.defaultval >> 8) & 0xff));
							}
							msg->Append((uint8) (it->second.defaultval & 0xff));
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);

						}
					}
				}
				else
				{
					switch (_value.GetID().GetType())
					{
						case ValueID::ValueType_BitSet:
						{
							Internal::VC::ValueBitSet const& vbs = static_cast<Internal::VC::ValueBitSet const&>(_value);
							Set(param, (int32) vbs.GetValue(), vbs.GetSize());
							return true;
						}
						case ValueID::ValueType_Bool:
						{
							Internal::VC::ValueBool const& valueBool = static_cast<Internal::VC::ValueBool const&>(_value);
							Set(param, (int32) valueBool.GetValue(), 1);
							return true;
						}
						case ValueID::ValueType_Byte:
						{
							Internal::VC::ValueByte const& valueByte = static_cast<Internal::VC::ValueByte const&>(_value);
							Set(param, (int32) valueByte.GetValue(), 1);
							return true;
						}
						case ValueID::ValueType_Short:
						{
							Internal::VC::ValueShort const& valueShort = static_cast<Internal::VC::ValueShort const&>(_value);
							Set(param, (int32) valueShort.GetValue(), 2);
							return true;
						}
						case ValueID::ValueType_Int:
						{
							Internal::VC::ValueInt const& valueInt = static_cast<Internal::VC::ValueInt const&>(_value);
							Set(param, valueInt.GetValue(), 4);
							return true;
						}
						case ValueID::ValueType_List:
						{
							Internal::VC::ValueList const& valueList = static_cast<Internal::VC::ValueList const&>(_value);
							if (valueList.GetItem() != NULL)
								Set(param, valueList.GetItem()->m_value, valueList.GetSize());
							return true;
						}
						case ValueID::ValueType_Button:
						{
							Internal::VC::ValueButton const& valueButton = static_cast<Internal::VC::ValueButton const&>(_value);
							Set(param, valueButton.IsPressed(), 1);
							return true;
						}
						default:
						{
						}
					}
				}
				Log::Write(LogLevel_Info, GetNodeId(), "Configuration::Set failed (bad value or value type) - Parameter=%d", param);
				return false;
			}
//-----------------------------------------------------------------------------
// <Configuration::RequestValue>
// Request current parameter value from the device
//-----------------------------------------------------------------------------


				bool Configuration::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
				{
					/* discovery of Params is only available in CC Version 3 and above */
					if (GetVersion() > 2) { 
						if (_requestFlags & RequestFlag_Session) {
							/* Make a request for Param 0 - That will tell us the first available Param */
							Msg* msg = new Msg("ConfigurationCmd_Properties_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->Append(GetNodeId());
							msg->Append(4);
							msg->Append(GetCommandClassId());
							msg->Append(ConfigurationCmd_Properties_Get);
							msg->Append(0x00);
							msg->Append(0x00);
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, _queue);
							return true;
						}
					}
					return false;
				}



//-----------------------------------------------------------------------------
// <Configuration::RequestValue>
// Request current parameter value from the device
//-----------------------------------------------------------------------------
			bool Configuration::RequestValue(uint32 const _requestFlags, uint16 const _parameter,			// parameter number is encoded as the Index portion of ValueID
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_instance != 1)
				{
					// This command class doesn't work with multiple instances
					return false;
				}
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					if ((GetVersion() < 3 ) || (_parameter < 255) ) 
					{
						Msg* msg = new Msg("ConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->Append(GetNodeId());
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(ConfigurationCmd_Get);
						msg->Append((_parameter & 0xFF));
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						return true;
					} 
					else 
					{
						Msg* msg = new Msg("ConfigurationCmd_Bulk_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->Append(GetNodeId());
						msg->Append(5);
						msg->Append(GetCommandClassId());
						msg->Append(ConfigurationCmd_Bulk_Get);
						msg->Append(((_parameter & 0xFF00) >> 8));
						msg->Append((_parameter & 0xFF));
						msg->Append(1);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						return true;
					}
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "ConfigurationCmd_Get Not Supported on this node");
				}
				return false;
			}
//-----------------------------------------------------------------------------
// <Configuration::Set>
// Set the device's
//-----------------------------------------------------------------------------
			void Configuration::Set(uint16 const _parameter, int32 const _value, uint8 const _size)
			{
				Log::Write(LogLevel_Info, GetNodeId(), "Configuration::Set - Parameter=%d, Value=%d Size=%d", _parameter, _value, _size);

				if ((GetVersion() < 3) || (_parameter < 255))
				{
					Msg* msg = new Msg("ConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->Append(GetNodeId());
					msg->Append(4 + _size);
					msg->Append(GetCommandClassId());
					msg->Append(ConfigurationCmd_Set);
					msg->Append((_parameter & 0xFF));
					msg->Append((_size & 0x07));
					if (_size > 2)
					{
						msg->Append((uint8) ((_value >> 24) & 0xff));
						msg->Append((uint8) ((_value >> 16) & 0xff));
					}
					if (_size > 1)
					{
						msg->Append((uint8) ((_value >> 8) & 0xff));
					}
					msg->Append((uint8) (_value & 0xff));
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
				} 
				else 
				{
					Msg* msg = new Msg("ConfigurationCmd_Bulk_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->Append(GetNodeId());
					msg->Append(6 + _size);
					msg->Append(GetCommandClassId());
					msg->Append(ConfigurationCmd_Bulk_Set);
					msg->Append(((_parameter & 0xFF00) >> 8));
					msg->Append((_parameter & 0xFF));
					msg->Append(1);
					msg->Append((_size & 0x07));
					if (_size > 2)
					{
						msg->Append((uint8) ((_value >> 24) & 0xff));
						msg->Append((uint8) ((_value >> 16) & 0xff));
					}
					if (_size > 1)
					{
						msg->Append((uint8) ((_value >> 8) & 0xff));
					}
					msg->Append((uint8) (_value & 0xff));
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
				}

				if (m_ConfigParams.find(_parameter) != m_ConfigParams.end()) {
					if (m_ConfigParams.at(_parameter).altering) {
						Notification* notification = new Notification(Notification::Type_UserAlerts);
	                	notification->SetUserAlertNotification(Notification::Alert_NodeReloadRequired);
    	            	GetDriver()->QueueNotification(notification);
					}
				}
			}
//-----------------------------------------------------------------------------
// <Configuration::processConfigParams>
// Process the Config Params we got from the device and compare to what
// out config file specifies. Fill in the blanks...
//-----------------------------------------------------------------------------
			bool Configuration::processConfigParams(uint16 param) 
			{
				if (m_ConfigParams.count(param)) {
					if (m_ConfigParams[param].flags != (ConfigParamFlags::ConfigParamFlags_Name_Done + ConfigParamFlags::ConfigParamFlags_Info_Done + ConfigParamFlags_Help_Done))
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Config Param %d is incomplete: %d", param, m_ConfigParams[param].flags);
						return false;
					}					
					/* see if we have a Value for this Param No */
					if (Internal::VC::Value *var = GetValue(1, param))
					{
						/* value Exists.. */
						Log::Write(LogLevel_Info, GetNodeId(), "Config Param %d (%s) already exists as a %s (%s)", param, m_ConfigParams[param].name.c_str(), var->GetID().GetTypeAsString().c_str(), var->GetLabel().c_str());
						/* what, if any sanity checks should we do? size? */
						var->Release();
						RequestValue(0, param, 1, Driver::MsgQueue_Query);

						return true;
					} else {
						/* value doesn't exist */
						if (Node* node = GetNodeUnsafe())
						{
							switch (m_ConfigParams[param].format) {
								case CC_Param_Format::CC_Param_Format_List:
								{
									/* I have no idea?!?!?!?!?:
									 * CC:0070.03.0F.11.00A:
									 * If the parameter format is “Enumerated”, the parameter MUST be treated as an unsigned integer. A graphical configuration tool SHOULD present this parameter as a series of radio buttons [11].
									 * 
									 * ehhhh... how? 
									 * 
									 * Maybe its Bit Positions... 1, 2, 4, 8, 16?
									 */
									Log::Write(LogLevel_Warning, GetNodeId(), "ConfigParam ValueList - TODO");
									/* fall Through to next block and treat it as a Integer till we figure this out */
								}

								case CC_Param_Format::CC_Param_Format_Signed:
								case CC_Param_Format::CC_Param_Format_Unsigned:
								{
									switch (m_ConfigParams[param].size) {
										case CC_Param_Size::CC_Param_Size_Byte:
										{
											node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), 1, param, m_ConfigParams[param].name, "", m_ConfigParams[param].readonly, false, (uint8) m_ConfigParams[param].defaultval, 0);
											if (Internal::VC::ValueByte *vb = static_cast<Internal::VC::ValueByte*>(GetValue(1, param))) {
												vb->SetHelp(m_ConfigParams[param].help);
												vb->SetMin(m_ConfigParams[param].min);
												vb->SetMax(m_ConfigParams[param].max);
												vb->Release();
											}
											break;
										}
										case CC_Param_Size::CC_Param_Size_Short:
										{
											node->CreateValueShort(ValueID::ValueGenre_Config, GetCommandClassId(), 1, param, m_ConfigParams[param].name, "", m_ConfigParams[param].readonly, false, (int16) m_ConfigParams[param].defaultval, 0);
											if (Internal::VC::ValueShort *vs = static_cast<Internal::VC::ValueShort*>(GetValue(1, param))) {
												vs->SetHelp(m_ConfigParams[param].help);
												vs->SetMin(m_ConfigParams[param].min);
												vs->SetMax(m_ConfigParams[param].max);
												vs->Release();
											}
											break;
										}
										case CC_Param_Size::CC_Param_Size_Int:
										{
											node->CreateValueInt(ValueID::ValueGenre_Config, GetCommandClassId(), 1, param, m_ConfigParams[param].name, "", m_ConfigParams[param].readonly, false, (int32) m_ConfigParams[param].defaultval, 0);
											if (Internal::VC::ValueInt *vi = static_cast<Internal::VC::ValueInt*>(GetValue(1, param))) {
												vi->SetHelp(m_ConfigParams[param].help);
												vi->SetMin(m_ConfigParams[param].min);
												vi->SetMax(m_ConfigParams[param].max);
												vi->Release();
											}
											break;
										}
										case CC_Param_Size_Unassigned: 
										{
											/* do nothing. */
											Log::Write(LogLevel_Warning, GetNodeId(), "Got a Config Param %d with Size 0!", param);
											break;
										}
									}
									break;
								}
								case CC_Param_Format::CC_Param_Format_BitSet:
								{
									node->CreateValueBitSet(ValueID::ValueGenre_Config, GetCommandClassId(), 1, param, m_ConfigParams[param].name, "", m_ConfigParams[param].readonly, false, m_ConfigParams[param].defaultval, 0);
									if (Internal::VC::ValueBitSet *vbs = static_cast<Internal::VC::ValueBitSet *>(GetValue(1, param)))
									{
										vbs->SetHelp(m_ConfigParams[param].help);
										vbs->SetBitMask(m_ConfigParams[param].max);
										/* I think we need to create the BitFields. */
										vbs->Release();

									}
									break;
								}
							}
							Log::Write(LogLevel_Info, GetNodeId(), "Created a new Config Param %d (%s) as a %d", param, m_ConfigParams[param].name.c_str(), m_ConfigParams[param].format);
							RequestValue(0, param, 1, Driver::MsgQueue_Query);
							return true;
						} else {
							Log::Write(LogLevel_Warning, GetNodeId(), "Cant Get Node to Create Config Param");
							return false;
						}
					}
				}
				Log::Write(LogLevel_Warning, GetNodeId(), "Cant find ConfigParam %d", param);
				return false;
			}
//-----------------------------------------------------------------------------
// <Configuration::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void Configuration::CreateVars(uint8 const _instance)
			{
				if (GetVersion() >= 3) 
				{
					if (Node* node = GetNodeUnsafe())
					{
						node->CreateValueButton(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_Configuration::ResetToDefault, "Reset Config Params To Default", 0);
					}
				}
			}

		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
