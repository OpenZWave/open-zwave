//-----------------------------------------------------------------------------
//
//	BarrierOperator.cpp
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
	BarrierOperatorCmd_SignalReport = 0x08
};

enum BarrierOperatorState
{
	BarrierOperatorState_Closed = 0x00,
	BarrierOperatorState_Closing = 0xFC,
	BarrierOperatorState_Stopped = 0xFD,
	BarrierOperatorState_Opening = 0xFE,
	BarrierOperatorState_Open = 0xFF,
};

enum BarrierOperator_SignalAttributesMask
{
    BarrierOperatorSignalMask_Audible      = 0x02,
    BarrierOperatorSignalMask_Visual       = 0x04,
    BarrierOperatorSignalMask_All          = 0x06,
};

enum BarrierOperatorCmd_Indexes
{
    BarrierOperatorCmd_Index             = 0x00,
    BarrierOperatorLabel_Index           = 0x01,
    BarrierOperatorNumber_Index          = 0x02,
    BarrierOperatorAudible_Index         = 0x03,
    BarrierOperatorVisual_Index          = 0x04,
};

static char const* c_BarrierOperator_States[] =
{
        "Closed",
        "Closing",
        "Stopped",
        "Opening",
        "Opened",
        "Unknown"
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
// <BarrierOperator::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool BarrierOperator::RequestState
(
		uint32 const _requestFlags,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	bool res = false;
	if( _requestFlags & RequestFlag_Dynamic )
	{
		res |= RequestValue( _requestFlags, BarrierOperatorCmd_Index, _instance, _queue );
		res |= RequestSignalSupport( _instance, _queue );
	}

	return res;
}

//-----------------------------------------------------------------------------
// <BarrierOperator::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool BarrierOperator::RequestValue
(
		uint32 const _requestFlags,
		uint8 const _index,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	Log::Write(LogLevel_Info, GetNodeId(), "Requesting BarrierOperator status");
	if (_index == BarrierOperatorCmd_Index) {
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
	else if ( _index == BarrierOperatorAudible_Index ) {
		Msg* msg = new Msg("BarrierOperatorCmd_SignalGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
		msg->SetInstance(this, _instance);
		msg->Append(GetNodeId());
		msg->Append(3);
		msg->Append(GetCommandClassId());
		msg->Append(BarrierOperatorCmd_SignalGet);
		msg->Append(0x01);
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg(msg, _queue);
		return true;
	}
	else if (_index == BarrierOperatorVisual_Index) {
		Msg* msg = new Msg("BarrierOperatorCmd_SignalGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
		msg->SetInstance(this, _instance);
		msg->Append(GetNodeId());
		msg->Append(3);
		msg->Append(GetCommandClassId());
		msg->Append(BarrierOperatorCmd_SignalGet);
		msg->Append(0x02);
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg(msg, _queue);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <BarrierOperator::RequestSignalSupport>
// Request RequestSignalSupport from the device
//-----------------------------------------------------------------------------
bool BarrierOperator::RequestSignalSupport
(
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	Msg* msg = new Msg("BarrierOperatorCmd_SignalSupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
	msg->SetInstance(this, _instance);
	msg->Append(GetNodeId());
	msg->Append(2);
	msg->Append(GetCommandClassId());
	msg->Append(BarrierOperatorCmd_SignalSupportedGet);
	msg->Append(GetDriver()->GetTransmitOptions());
	GetDriver()->SendMsg(msg, _queue);
	return true;
}

//-----------------------------------------------------------------------------
// <BarrierOperator::HandleMsg>
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
		uint8 state_index = 5;
		if (_data[1] == BarrierOperatorState_Closed) {state = c_BarrierOperator_States[0];state_index = 0;}
		else if (_data[1] == BarrierOperatorState_Closing) {state = c_BarrierOperator_States[1];state_index = 1;}
		else if (_data[1] == BarrierOperatorState_Stopped) {state = c_BarrierOperator_States[2];state_index = 2;}
		else if (_data[1] == BarrierOperatorState_Opening) {state = c_BarrierOperator_States[3];state_index = 3;}
		else if (_data[1] == BarrierOperatorState_Open) {state = c_BarrierOperator_States[4];state_index = 4;}

		Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator report: Barrier is %s", state);
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, 1 ) ) )
		{
			value->OnValueRefreshed( state_index );
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for BarrierOperator state");
			return false;
		}
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, 2 ) ) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for BarrierOperator state");
			return false;
		}
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
	if (BarrierOperatorCmd_SignalSupportedReport == (BarrierOperatorCmd)_data[0])
	{
		Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator Signal Support Report");
		if (BarrierOperatorSignalMask_Audible == _data[1])
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received Audible support");
			if (Node* node = GetNodeUnsafe())
			{
				node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 3, "Audible", "", false, false, false, 0);
			}
			RequestValue( 0, BarrierOperatorCmd_SignalGet, _instance, Driver::MsgQueue_Send );
		}
		else if (BarrierOperatorSignalMask_Visual == _data[1])
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received Visual support");
			if (Node* node = GetNodeUnsafe())
			{
				node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 4, "Visual", "", false, false, false, 0);
			}
			RequestValue( 0, BarrierOperatorCmd_SignalGet, _instance, Driver::MsgQueue_Send );
		}
		else if (BarrierOperatorSignalMask_All == _data[1])
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received All (Audible and Visual) support");
			if (Node* node = GetNodeUnsafe())
			{
				node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 3, "Audible", "", false, false, false, 0);
				node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 4, "Visual", "", false, false, false, 0);
			}
			RequestValue( 0, BarrierOperatorCmd_SignalGet, _instance, Driver::MsgQueue_Send );
			RequestValue( 0, BarrierOperatorCmd_SignalGet, _instance, Driver::MsgQueue_Send );
		}
		else
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received No support");
		}
		return true;
	}
	if (BarrierOperatorCmd_SignalReport == (BarrierOperatorCmd)_data[0])
	{
		Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator Signal Report");
		if (_data[1] == 0x01)
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator Signal Report for Audible");
			if (ValueBool* value = static_cast<ValueBool*>(GetValue(_instance, 3)))
			{
				value->OnValueRefreshed(_data[2] != 0x00);
				value->Release();
			}
		}
		if (_data[1] == 0x02)
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator Signal Report for Visual");
			if (ValueBool* value = static_cast<ValueBool*>(GetValue(_instance, 4)))
			{
				value->OnValueRefreshed(_data[2] != 0x00);
				value->Release();
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <BarrierOperator::SetValue>
// Set a value
//-----------------------------------------------------------------------------

bool BarrierOperator::SetValue
(
	Value const& _value
	)
{
	uint8 idx = _value.GetID().GetIndex();
	if (ValueID::ValueType_Bool == _value.GetID().GetType())
	{
		if (idx == 0)
		{
			ValueBool const* value = static_cast<ValueBool const*>(&_value);
			Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperator::Set - Requesting barrier to be %s", value->GetValue() ? "Open" : "Closed");
			Msg* msg = new Msg("BarrierOperatorCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
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
		else if (idx == 3)
		{
			ValueBool const* value = static_cast<ValueBool const*>(&_value);
			Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperatorSignal::Set - Requesting Audible to be %s", value->GetValue() ? "ON" : "OFF");
			Msg* msg = new Msg("BarrierOperatorSignalCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
			msg->SetInstance(this, _value.GetID().GetInstance());
			msg->Append(GetNodeId());
			msg->Append(4);
			msg->Append(GetCommandClassId());
			msg->Append(BarrierOperatorCmd_SignalSet);
			msg->Append(1);
			msg->Append(value->GetValue() ? 0xFF : 0x00);
			msg->Append(GetDriver()->GetTransmitOptions());
			GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
			return true;
		}
		else if (idx == 4)
		{
			ValueBool const* value = static_cast<ValueBool const*>(&_value);
			Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperatorSignal::Set - Requesting Visual to be %s", value->GetValue() ? "ON" : "OFF");
			Msg* msg = new Msg("BarrierOperatorSignalCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
			msg->SetInstance(this, _value.GetID().GetInstance());
			msg->Append(GetNodeId());
			msg->Append(4);
			msg->Append(GetCommandClassId());
			msg->Append(BarrierOperatorCmd_SignalSet);
			msg->Append(2);
			msg->Append(value->GetValue() ? 0xFF : 0x00);
			msg->Append(GetDriver()->GetTransmitOptions());
			GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <BarrierOperator::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void BarrierOperator::CreateVars
(
		uint8 const _instance
)
{
	if (Node* node = GetNodeUnsafe())
	{
		node->CreateValueBool(ValueID::ValueGenre_User, GetCommandClassId(), _instance, BarrierOperatorCmd_Index, "Open", "", false, false, false, 0);
		vector<ValueList::Item> items;
        unsigned int size = (sizeof(c_BarrierOperator_States)/sizeof(c_BarrierOperator_States[0]));
        for( unsigned int i=0; i < size; i++)
        {
            ValueList::Item item;
            item.m_label = c_BarrierOperator_States[i];
            item.m_value = i;
            items.push_back( item );
        }
		node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, BarrierOperatorLabel_Index, "Barrier State Label", "", true, false, size, items, 0, 0 );
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, BarrierOperatorNumber_Index ,"Barrier State Numeric", "", true, false, 0, 0 );
	}
}

