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
				IndicatorCmd_Report = 0x03
			};

//-----------------------------------------------------------------------------
// <Indicator::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool Indicator::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_requestFlags & RequestFlag_Dynamic)
				{
					return RequestValue(_requestFlags, 0, _instance, _queue);
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <Indicator::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool Indicator::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
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
				if (IndicatorCmd_Report == (IndicatorCmd) _data[0])
				{
					Log::Write(LogLevel_Info, GetNodeId(), "Received an Indicator report: Indicator=%d", _data[1]);

					if (Internal::VC::ValueByte* value = static_cast<Internal::VC::ValueByte*>(GetValue(_instance, ValueID_Index_Indicator::Indicator)))
					{
						value->OnValueRefreshed(_data[1]);
						value->Release();
					}
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <Indicator::SetValue>
// Set the device's indicator value
//-----------------------------------------------------------------------------
			bool Indicator::SetValue(Internal::VC::Value const& _value)
			{
				if (ValueID::ValueType_Byte == _value.GetID().GetType())
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

				return false;
			}

//-----------------------------------------------------------------------------
// <Indicator::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void Indicator::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_Indicator::Indicator, "Indicator", "", false, false, false, 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
