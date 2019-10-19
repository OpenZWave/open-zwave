//-----------------------------------------------------------------------------
//
//	UserCode.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_USER_CODE
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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

#include "tinyxml.h"
#include "command_classes/CommandClasses.h"
#include "command_classes/UserCode.h"
#include "command_classes/NodeNaming.h"
#include "Node.h"
#include "Options.h"
#include "platform/Log.h"

#include "value_classes/ValueShort.h"
#include "value_classes/ValueString.h"
#include "value_classes/ValueRaw.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum UserCodeCmd
			{
				UserCodeCmd_Set = 0x01,
				UserCodeCmd_Get = 0x02,
				UserCodeCmd_Report = 0x03,
				UserNumberCmd_Get = 0x04,
				UserNumberCmd_Report = 0x05
			};

//-----------------------------------------------------------------------------
// <UserCode::UserCode>
// Constructor
//-----------------------------------------------------------------------------
			UserCode::UserCode(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId), m_queryAll(false), m_currentCode(0), m_refreshUserCodes(false)
			{
				m_com.EnableFlag(COMPAT_FLAG_UC_EXPOSERAWVALUE, false);
				m_dom.EnableFlag(STATE_FLAG_USERCODE_COUNT, 0);
				SetStaticRequest(StaticRequest_Values);
				Options::Get()->GetOptionAsBool("RefreshAllUserCodes", &m_refreshUserCodes);

			}

//-----------------------------------------------------------------------------
// <UserCode::RequestState>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
			bool UserCode::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool requests = false;
				if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
				{
					requests |= RequestValue(_requestFlags, ValueID_Index_UserCode::Count, _instance, _queue);
				}

				if (_requestFlags & RequestFlag_Session)
				{
					if (m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT) > 0)
					{
						m_queryAll = true;
						m_currentCode = 1;
						requests |= RequestValue(_requestFlags, m_currentCode, _instance, _queue);
					}
				}

				return requests;
			}

//-----------------------------------------------------------------------------
// <UserCode::RequestValue>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
			bool UserCode::RequestValue(uint32 const _requestFlags, uint16 const _userCodeIdx, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_instance != 1)
				{
					// This command class doesn't work with multiple instances
					return false;
				}
				if (!m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Log::Write(LogLevel_Info, GetNodeId(), "UserNumberCmd_Get Not Supported on this node");
					return false;
				}
				if (_userCodeIdx == ValueID_Index_UserCode::Count)
				{
					// Get number of supported user codes.
					Msg* msg = new Msg("UserNumberCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(UserNumberCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				if (_userCodeIdx == 0)
				{
					Log::Write(LogLevel_Warning, GetNodeId(), "UserCodeCmd_Get with Index 0 not Supported");
					return false;
				}
				if (_userCodeIdx > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT))
				{
					Log::Write(LogLevel_Warning, GetNodeId(), "UserCodeCmd_Get with index %d is greater than max UserCodes", _userCodeIdx);
					return false;
				}
				Msg* msg = new Msg("UserCodeCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
				msg->Append(GetNodeId());
				msg->Append(3);
				msg->Append(GetCommandClassId());
				msg->Append(UserCodeCmd_Get);
				msg->Append((_userCodeIdx & 0xFF));
				msg->Append(GetDriver()->GetTransmitOptions());
				GetDriver()->SendMsg(msg, _queue);
				return true;
			}

//-----------------------------------------------------------------------------
// <UserCode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool UserCode::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (UserNumberCmd_Report == (UserCodeCmd) _data[0])
				{
					m_dom.SetFlagByte(STATE_FLAG_USERCODE_COUNT, _data[1]);
					ClearStaticRequest(StaticRequest_Values);
					if (_data[1] == 0)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Not supported", GetNodeId());
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Supported Codes %d (%d)", GetNodeId(), _data[1], _data[1]);
					}

					if (Internal::VC::ValueShort* value = static_cast<Internal::VC::ValueShort*>(GetValue(_instance, ValueID_Index_UserCode::Count)))
					{
						value->OnValueRefreshed(_data[1]);
						value->Release();
					}

					if (Node* node = GetNodeUnsafe())
					{
						string data;

						for (uint16 i = 0; i <= m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT); i++)
						{
							char str[16];
							if (i == 0)
							{
								snprintf(str, sizeof(str), "Enrollment Code");
								node->CreateValueString(ValueID::ValueGenre_User, GetCommandClassId(), _instance, i, str, "", true, false, data, 0);
							}
							else
							{
								snprintf(str, sizeof(str), "Code %d:", i);
								node->CreateValueString(ValueID::ValueGenre_User, GetCommandClassId(), _instance, i, str, "", false, false, data, 0);
							}
							m_userCode[i].status = UserCode_Available;
							/* silly compilers */
							for (int j = 0; j < 10; j++)
								m_userCode[i].usercode[j] = 0;
						}
						if (m_com.GetFlagBool(COMPAT_FLAG_UC_EXPOSERAWVALUE))
						{
							node->CreateValueRaw(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_UserCode::RawValue, "Raw UserCode", "", false, false, 0, 0, 0);
							node->CreateValueShort(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_UserCode::RawValueIndex, "Raw UserCode Index", "", false, false, 0, 0);
						}
					}
					return true;
				}
				else if (UserCodeCmd_Report == (UserCodeCmd) _data[0])
				{
					int i = _data[1];
					Log::Write(LogLevel_Info, GetNodeId(), "Received User Code Report from node %d for User Code %d (%s)", GetNodeId(), i, CodeStatus(_data[2]).c_str());

					int8 size = _length - 4;
					if (size > 10)
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "User Code length %d is larger then maximum 10", size);
						size = 10;
					}
					m_userCode[i].status = (UserCodeStatus) _data[2];
					memcpy(&m_userCode[i].usercode, &_data[3], size);
					if (Internal::VC::ValueString* value = static_cast<Internal::VC::ValueString*>(GetValue(_instance, i)))
					{
						string data;
						/* Max UserCode Length is 10 */
						Log::Write(LogLevel_Info, GetNodeId(), "User Code Packet is %d", size);
						data.assign((const char*) &_data[3], size);
						value->OnValueRefreshed(data);
						value->Release();
					}
					if (m_com.GetFlagBool(COMPAT_FLAG_UC_EXPOSERAWVALUE))
					{
						if (Internal::VC::ValueShort* value = static_cast<Internal::VC::ValueShort*>(GetValue(_instance, ValueID_Index_UserCode::RawValueIndex)))
						{
							value->OnValueRefreshed(i);
							value->Release();
						}
						if (Internal::VC::ValueRaw* value = static_cast<Internal::VC::ValueRaw*>(GetValue(_instance, ValueID_Index_UserCode::RawValue)))
						{
							value->OnValueRefreshed(&_data[3], (_length - 4));
							value->Release();
						}
					}

					if (m_queryAll && i == m_currentCode)
					{

						if (m_refreshUserCodes || (_data[2] != UserCode_Available))
						{
							if (++i <= m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT))
							{
								m_currentCode = i;
								RequestValue(0, m_currentCode, _instance, Driver::MsgQueue_Query);
							}
							else
							{
								m_queryAll = false;
								/* we might have reset this as part of the RefreshValues Button Value */
								Options::Get()->GetOptionAsBool("RefreshAllUserCodes", &m_refreshUserCodes);
							}
						}
						else
						{
							Log::Write(LogLevel_Info, GetNodeId(), "Not Requesting additional UserCode Slots as RefreshAllUserCodes is false, and slot %d is available", i);
							m_queryAll = false;
						}
					}
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <UserCode::SetValue>
// Set a User Code value
//-----------------------------------------------------------------------------
			bool UserCode::SetValue(Internal::VC::Value const& _value)
			{
				if ((ValueID::ValueType_String == _value.GetID().GetType()) && (_value.GetID().GetIndex() < ValueID_Index_UserCode::Refresh))
				{
					Internal::VC::ValueString const* value = static_cast<Internal::VC::ValueString const*>(&_value);
					string s = value->GetValue();
					if (s.length() < 4)
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "UserCode is smaller than 4 digits", value->GetID().GetIndex());
						return false;
					}
					if (s.length() > 10)
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "UserCode is larger than 10 digits", value->GetID().GetIndex());
						return false;
					}
					uint8 len = (uint8_t) (s.length() & 0xFF);
					if (value->GetID().GetIndex() == 0 || value->GetID().GetIndex() > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT))
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", value->GetID().GetIndex());
						return false;
					}

					Msg* msg = new Msg("UserCodeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->SetInstance(this, _value.GetID().GetInstance());
					msg->Append(GetNodeId());
					msg->Append(4 + len);
					msg->Append(GetCommandClassId());
					msg->Append(UserCodeCmd_Set);
					msg->Append((uint8_t) (value->GetID().GetIndex() & 0xFF));
					msg->Append(UserCode_Occupied);
					for (uint8 i = 0; i < len; i++)
					{
						msg->Append(s[i]);
					}
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);

					return true;
				}
				if ((ValueID::ValueType_Button == _value.GetID().GetType()) && (_value.GetID().GetIndex() == ValueID_Index_UserCode::Refresh))
				{
					m_refreshUserCodes = true;
					m_currentCode = 1;
					m_queryAll = true;
					RequestValue(0, m_currentCode, _value.GetID().GetInstance(), Driver::MsgQueue_Query);
					return true;
				}
				if ((ValueID::ValueType_Short == _value.GetID().GetType()) && (_value.GetID().GetIndex() == ValueID_Index_UserCode::RemoveCode))
				{
					Internal::VC::ValueShort const* value = static_cast<Internal::VC::ValueShort const*>(&_value);
					uint8_t index = (uint8_t) (value->GetValue() & 0xFF);
					if (index == 0 || index > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT))
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", index);
						return false;
					}
					Msg* msg = new Msg("UserCodeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->SetInstance(this, _value.GetID().GetInstance());
					msg->Append(GetNodeId());
					msg->Append(8);
					msg->Append(GetCommandClassId());
					msg->Append(UserCodeCmd_Set);
					msg->Append(index);
					msg->Append(UserCode_Available);
					for (uint8 i = 0; i < 4; i++)
					{
						msg->Append(0);
					}
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);

					RequestValue(0, index, _value.GetID().GetInstance(), Driver::MsgQueue_Send);

#if 0
					/* Reset Our Local Copy here */

					if( ValueString* oldvalue = static_cast<ValueString*>( GetValue( _value.GetID().GetInstance(), index ) ) )
					{
						string data;
						oldvalue->OnValueRefreshed( data );
						oldvalue->Release();
					}
#endif
					return false;
				}
				if ((ValueID::ValueType_Short == _value.GetID().GetType()) && (_value.GetID().GetIndex() == ValueID_Index_UserCode::RawValueIndex))
				{
					Internal::VC::ValueShort const* value = static_cast<Internal::VC::ValueShort const*>(&_value);
					uint16 index = value->GetValue();
					if (index == 0 || index > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT))
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", index);
						return false;
					}
					if (Internal::VC::ValueRaw* oldvalue = static_cast<Internal::VC::ValueRaw*>(GetValue(_value.GetID().GetInstance(), ValueID_Index_UserCode::RawValue)))
					{
						oldvalue->OnValueRefreshed((const uint8*) &m_userCode[index].usercode, 10);
						oldvalue->Release();
					}
					return false;
				}
				if ((ValueID::ValueType_Raw == _value.GetID().GetType()) && (_value.GetID().GetIndex() == ValueID_Index_UserCode::RawValue))
				{
					Internal::VC::ValueRaw const* value = static_cast<Internal::VC::ValueRaw const*>(&_value);
					uint16 index = 0;
					if (Internal::VC::ValueShort* valueindex = static_cast<Internal::VC::ValueShort*>(GetValue(_value.GetID().GetInstance(), ValueID_Index_UserCode::RawValueIndex)))
					{
						index = valueindex->GetValue();
					}
					if (index == 0 || index > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT))
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", index);
						return false;
					}
					uint8 *s = value->GetValue();
					uint8 len = value->GetLength();

					Msg* msg = new Msg("UserCodeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
					msg->SetInstance(this, _value.GetID().GetInstance());
					msg->Append(GetNodeId());
					msg->Append(4 + len);
					msg->Append(GetCommandClassId());
					msg->Append(UserCodeCmd_Set);
					msg->Append((uint8_t) (index & 0xFF));
					msg->Append(UserCode_Occupied);
					for (uint8 i = 0; i < len; i++)
					{
						msg->Append(s[i]);
					}
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					RequestValue(0, index, _value.GetID().GetInstance(), Driver::MsgQueue_Send);

					return false;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <UserCode::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void UserCode::CreateVars(uint8 const _instance

			)
			{
				if (Node* node = GetNodeUnsafe())
				{
					node->CreateValueShort(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_UserCode::Count, "Code Count", "", true, false, 0, 0);
					node->CreateValueButton(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_UserCode::Refresh, "Refresh All UserCodes", 0);
					node->CreateValueShort(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_UserCode::RemoveCode, "Remove User Code", "", false, true, 0, 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
