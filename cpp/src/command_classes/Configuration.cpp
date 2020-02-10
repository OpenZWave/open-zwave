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
				uint32 value;
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
				}
				return value;
			}

//-----------------------------------------------------------------------------
// <Configuration::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Configuration::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (ConfigurationCmd_Report == (ConfigurationCmd) _data[0])
				{
					// Extract the parameter index and value
					uint8 parameter = _data[1];
					uint8 size = _data[2] & 0x07;
					int32 paramValue = 0;
					for (uint8 i = 0; i < size; ++i)
					{
						paramValue <<= 8;
						paramValue |= (int32) _data[i + 3];
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
					param.flags |= ConfigParamFlags_Info_Done;
					m_ConfigParams[paramNo] = param;
					uint16 nextParam = getField(_data, CC_Param_Size::CC_Param_Size_Short, position);
					Log::Write(LogLevel_Warning, GetNodeId(), "Param %d Format: %d Size: %d Min: %d Max: %d Default %d Next %d", param.paramNo, param.format, paramSize, param.min, param.max, param.defaultval, nextParam);
					

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
					} else {
						/* No More Params - Lets process what we have */
						return processConfigParams();
					}
					return true;

				}
				else if (ConfigurationCmd_Name_Report == (ConfigurationCmd) _data[0])
				{
					uint16 paramNo = (_data[1] << 8) + _data[2];
					bool reportsToFollow = (_data[3] > 0);
					if (m_ConfigParams.find(paramNo) != m_ConfigParams.end()) {
						for (int i = 4; i <= (_length -1); i++) {
							m_ConfigParams.at(paramNo).name += _data[i];
						}
						if (reportsToFollow)
							m_ConfigParams.at(paramNo).flags |= ConfigParamFlags_Name_Done;
						uint8 moreInfo = (_data[3]);
						Log::Write(LogLevel_Warning, GetNodeId(), "Param %d Name: %s (moreInfo %d)", paramNo, m_ConfigParams.at(paramNo).name.c_str(), moreInfo);
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
						for (int i = 4; i <= (_length -1); i++) {
							m_ConfigParams.at(paramNo).help += _data[i];
						}
						if (reportsToFollow)
							m_ConfigParams.at(paramNo).flags |= ConfigParamFlags_Info_Done;
						uint8 moreInfo = (_data[3]);
						Log::Write(LogLevel_Warning, GetNodeId(), "Param %d Help: %s (moreInfo: %d)", paramNo, m_ConfigParams.at(paramNo).help.c_str(), moreInfo);
					} else {
						Log::Write(LogLevel_Warning, GetNodeId(), "Cant Find Config Param %d for InfoReport", paramNo);
					}
					return true;
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Configuration::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
			bool Configuration::SetValue(Internal::VC::Value const& _value)
			{
				uint16 param = _value.GetID().GetIndex();
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

				Msg* msg = new Msg("ConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
				msg->Append(GetNodeId());
				msg->Append(4 + _size);
				msg->Append(GetCommandClassId());
				msg->Append(ConfigurationCmd_Set);
				msg->Append((_parameter & 0xFF));
				msg->Append(_size);
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
//-----------------------------------------------------------------------------
// <Configuration::processConfigParams>
// Process the Config Params we got from the device and compare to what
// out config file specifies. Fill in the blanks...
//-----------------------------------------------------------------------------
			bool Configuration::processConfigParams() 
			{
				for (std::map<uint16, ConfigParam>::iterator it = m_ConfigParams.begin(); it != m_ConfigParams.end(); it++) {
					if (it->second.flags != (ConfigParamFlags::ConfigParamFlags_Name_Done + ConfigParamFlags::ConfigParamFlags_Info_Done + ConfigParamFlags_Help_Done))
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Config Param %d is incomplete: %d", it->first, it->second.flags);
						continue;
					}					
					/* see if we have a Value for this Param No */
					if (Internal::VC::Value *var = GetValue(1, it->first))
					{
						/* value Exists.. */
						Log::Write(LogLevel_Info, GetNodeId(), "Config Param %d (%s) already exists as a %s (%s)", it->first, it->second.name.c_str(), var->GetID().GetTypeAsString().c_str(), var->GetLabel().c_str());
						/* what, if any sanity checks should we do? size? */
						continue;
					} else {
						/* value doesn't exist */
						if (Node* node = GetNodeUnsafe())
						{
							switch (it->second.format) {
								case CC_Param_Format::CC_Param_Format_Signed:
								case CC_Param_Format::CC_Param_Format_Unsigned:
								{
									switch (it->second.size) {
										case CC_Param_Size::CC_Param_Size_Byte:
										{
											node->CreateValueByte(ValueID::ValueGenre_Config, GetCommandClassId(), 1, it->first, it->second.name, "", false, false, (uint8) it->second.defaultval, 0);
											if (Internal::VC::ValueByte *vb = static_cast<Internal::VC::ValueByte*>(GetValue(1, it->first))) {
												vb->SetHelp(it->second.help);
												vb->SetMin(it->second.min);
												vb->SetMax(it->second.max);
											}

											break;
										}
										case CC_Param_Size::CC_Param_Size_Short:
										{
											node->CreateValueShort(ValueID::ValueGenre_Config, GetCommandClassId(), 1, it->first, it->second.name, "", false, false, (int16) it->second.defaultval, 0);
											if (Internal::VC::ValueShort *vs = static_cast<Internal::VC::ValueShort*>(GetValue(1, it->first))) {
												vs->SetHelp(it->second.help);
												vs->SetMin(it->second.min);
												vs->SetMax(it->second.max);
											}
											break;
										}
										case CC_Param_Size::CC_Param_Size_Int:
										{
											node->CreateValueInt(ValueID::ValueGenre_Config, GetCommandClassId(), 1, it->first, it->second.name, "", false, false, (int32) it->second.defaultval, 0);
											if (Internal::VC::ValueInt *vi = static_cast<Internal::VC::ValueInt*>(GetValue(1, it->first))) {
												vi->SetHelp(it->second.help);
												vi->SetMin(it->second.min);
												vi->SetMax(it->second.max);
											}
											break;
										}
										case CC_Param_Size_Unassigned: 
										{
											/* do nothing. */
											Log::Write(LogLevel_Warning, GetNodeId(), "Got a Config Param %d with Size 0!", it->first);
											break;
										}
									}
									break;
								}
								case CC_Param_Format::CC_Param_Format_BitSet:
								{
									node->CreateValueBitSet(ValueID::ValueGenre_Config, GetCommandClassId(), 1, it->first, it->second.name, "", false, false, it->second.defaultval, 0);
									if (Internal::VC::ValueBitSet *vbs = static_cast<Internal::VC::ValueBitSet *>(GetValue(1, it->first)))
									{
										vbs->SetHelp(it->second.help);
										vbs->SetBitMask(it->second.max);
										/* I think we need to create the BitFields. */
									}
									break;
								}
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
									Log::Write(LogLevel_Warning, GetNodeId(), "ConfigParam List TODO?");
									break;
								}
							}
							Log::Write(LogLevel_Info, GetNodeId(), "Created a new Config Param %d (%s) as a %d", it->first, it->second.name.c_str(), it->second.format);
						} else {
							Log::Write(LogLevel_Warning, GetNodeId(), "Cant Get Node to Create Config Param");
						}
					}
				}
				return true;
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
