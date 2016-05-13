//-----------------------------------------------------------------------------
//
//	AssociationCommandConfiguration.cpp
//
//	Implementation of the COMMAND_CLASS_BARRIER_OPERATOR
//
//	Copyright (c) 2016 srirams (https://github.com/srirams)
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
#include "command_classes/BarrierOperator.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"
#include "value_classes/ValueBool.h"

using namespace OpenZWave;

enum BarrierOperatorCmd
{
	BarrierOperatorCmd_Set = 0x01,
	BarrierOperatorCmd_Get = 0x02,
	BarrierOperatorCmd_Report = 0x03,
	BarrierOperatorCmd_SignalSupportedGet = 0x04,
	BarrierOperatorCmd_SignalSupportedReport = 0x05,
	BarrierOperatorCmd_SignalSet = 0x06,
	BarrierOperatorCmd_SignalGet = 0x07,
	BarrierOperatorCmd_SignalReport = 0x06
};

enum BarrierOperatorState
{
	BarrierOperatorState_Closed = 0x00,
	BarrierOperatorState_Closing = 0xFC,		
	BarrierOperatorState_Stopped = 0xFD,
	BarrierOperatorState_Opening = 0xFE,
	BarrierOperatorState_Open = 0xFF,
};

BarrierOperator::BarrierOperator
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId )
{
}


//-----------------------------------------------------------------------------
// <Alarm::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool BarrierOperator::RequestState
(
		uint32 const _requestFlags,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool BarrierOperator::RequestValue
(
		uint32 const _requestFlags,
		uint8 const _dummy1,	// = 0 (not used)
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if (IsGetSupported())
	{
		Log::Write(LogLevel_Info, GetNodeId(), "Requesting BarrierOperator status");
		Msg* msg = new Msg("BarrierOperatorCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
		msg->SetInstance(this, _instance);
		msg->Append(GetNodeId());
		msg->Append(2);
		msg->Append(GetCommandClassId());
		msg->Append(BarrierOperatorCmd_Get);
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg(msg, _queue);
		return true;
	}
	else {
		Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperatorCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool BarrierOperator::HandleMsg
(
		uint8 const* _data,
		uint32 const _length,
		uint32 const _instance	// = 1
)
{
	if (BarrierOperatorCmd_Report == (BarrierOperatorCmd)_data[0])
	{
		const char* state = "Unknown";
		if (_data[1] == BarrierOperatorState_Closed) state = "Closed";
		else if (_data[1] == BarrierOperatorState_Closing) state = "Closing";
		else if (_data[1] == BarrierOperatorState_Stopped) state = "Stopped";
		else if (_data[1] == BarrierOperatorState_Opening) state = "Opening";
		else if (_data[1] == BarrierOperatorState_Open) state = "Open";

		Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator report: Barrier is %s", state);

		if (_data[1] == BarrierOperatorState_Open || _data[1] == BarrierOperatorState_Closed)
		{
			if (ValueBool* value = static_cast<ValueBool*>(GetValue(_instance, 0)))
			{
				value->OnValueRefreshed(_data[1] != BarrierOperatorState_Closed);
				value->Release();				
			}
		}
		return true;		
	}

	return false;
}

bool BarrierOperator::SetValue
(
	Value const& _value
	)
{
	if (ValueID::ValueType_Bool == _value.GetID().GetType())
	{
		ValueBool const* value = static_cast<ValueBool const*>(&_value);

		Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperator::Set - Requesting barrier to be %s", value->GetValue() ? "Open" : "Closed");
		Msg* msg = new Msg("LockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
		msg->SetInstance(this, _value.GetID().GetInstance());
		msg->Append(GetNodeId());
		msg->Append(3);
		msg->Append(GetCommandClassId());
		msg->Append(BarrierOperatorCmd_Set);
		msg->Append(value->GetValue() ? 0xFF : 0x00);
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void BarrierOperator::CreateVars
(
		uint8 const _instance
)
{
	if (Node* node = GetNodeUnsafe())
	{
		node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Open", "", false, false, false, 0);
	}
}

