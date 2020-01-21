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
				ConfigurationCmd_Set = 0x04,
				ConfigurationCmd_Get = 0x05,
				ConfigurationCmd_Report = 0x06
			};

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
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
