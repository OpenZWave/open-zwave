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
    BarrierOperatorSignalMask_Audible      = 0x01,
    BarrierOperatorSignalMask_Visual       = 0x02,
	BarrierOperatorSignalMask_All		   = 0x03
};

enum BarrierOperatorCmd_Indexes
{
    BarrierOperatorCmd_Index             	= 0x00,
    BarrierOperatorLabel_Index           	= 0x01,
    BarrierOperatorSupportedSignals_Index   = 0x02,
    BarrierOperatorAudible_Index        	= 0x03,
    BarrierOperatorVisual_Index          	= 0x04,
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

static char const *c_BarrierOperator_Signals[] =
{
		"None",
		"Audible",
		"Visual",
		"Both"
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
	}
	if ( _requestFlags & RequestFlag_Static )
	{
		res |= RequestValue( _requestFlags, BarrierOperatorSupportedSignals_Index, _instance, _queue );
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
		uint16 const _index,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if (_index == BarrierOperatorCmd_Index)
	{
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
	else if ( _index == BarrierOperatorAudible_Index )
	{
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
	else if (_index == BarrierOperatorVisual_Index)
	{
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
	else if (_index == BarrierOperatorSupportedSignals_Index)
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
	return false;
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
		uint8 state_index = 5;
		switch (_data[1])
		{
			case BarrierOperatorState_Closed:
			{
				state_index = 0;
				break;
			}
			case BarrierOperatorState_Closing:
			{
				state_index = 1;
				break;
			}
			case BarrierOperatorState_Stopped:
			{
				state_index = 2;
				break;
			}
			case BarrierOperatorState_Opening:
			{
				state_index = 3;
				break;
			}
			case BarrierOperatorState_Open:
			{
				state_index = 4;
				break;
			}
			default:
			{
				Log::Write(LogLevel_Warning, GetNodeId(), "Received Invalid BarrierOperatorState %d", _data[1]);
				break;
			}
		}

		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, BarrierOperatorLabel_Index ) ) )
		{
			value->OnValueRefreshed( state_index );
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for BarrierOperator state");
			return false;
		}
		return true;
	}
	if (BarrierOperatorCmd_SignalSupportedReport == (BarrierOperatorCmd)_data[0])
	{

		Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator Signal Support Report");
		uint8 state_index = 0;
		uint8 data = _data[1];

		/* Aeotec GDC shifts the SupportedReport by one, so we have to shift back */
		if (data > 3)
		{
			Log::Write(LogLevel_Warning, GetNodeId(), "SignalSupportedReport is out of Range. Shifting Right");
			data = data >> 1;
		}
		switch (data)
		{
			case BarrierOperatorSignalMask_Audible:
			{
				state_index = 1;
				RequestValue( 0, BarrierOperatorAudible_Index , _instance, Driver::MsgQueue_Send );
				break;
			}
			case BarrierOperatorSignalMask_Visual:
			{
				state_index = 2;
				RequestValue( 0, BarrierOperatorVisual_Index , _instance, Driver::MsgQueue_Send );
				break;
			}

			case BarrierOperatorSignalMask_All:
			{
				state_index = 3;
				RequestValue( 0, BarrierOperatorAudible_Index , _instance, Driver::MsgQueue_Send );
				RequestValue( 0, BarrierOperatorVisual_Index , _instance, Driver::MsgQueue_Send );
				break;
			}
			default:
			{
				Log::Write(LogLevel_Warning, GetNodeId(), "Received Invalid SignalSupported Report: %d", _data[1]);
				break;
			}
		}
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, BarrierOperatorSupportedSignals_Index ) ) )
		{
			value->OnValueRefreshed( state_index );
			value->Release();

		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for BarrierOperator SupportedSignals");
			return false;
		}
		return true;
	}
	if (BarrierOperatorCmd_SignalReport == (BarrierOperatorCmd)_data[0])
	{
		if (_data[1] & 0x01)
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator Signal Report for Audible");
			if (ValueBool* value = static_cast<ValueBool*>(GetValue(_instance, 3)))
			{
				value->OnValueRefreshed(_data[2] == 0xFF ? true : false);
				value->Release();
			}
		}
		if (_data[1] & 0x02)
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received BarrierOperator Signal Report for Visual");
			if (ValueBool* value = static_cast<ValueBool*>(GetValue(_instance, 4)))
			{
				value->OnValueRefreshed(_data[2] == 0xFF ? true : false	);
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
	uint8 idx = (uint8_t)(_value.GetID().GetIndex() & 0xFF);
	if (ValueID::ValueType_List == _value.GetID().GetType())
	{
		if (idx == BarrierOperatorLabel_Index)
		{
			ValueList const* value = static_cast<ValueList const*>(&_value);
			const ValueList::Item  *item = value->GetItem();
			uint8 position = BarrierOperatorState_Closed;
			if (item->m_value > 0)
				position = BarrierOperatorState_Open;
			Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperator::Set - Requesting barrier to be %s", position > 0 ? "Open" : "Closed");
			Msg* msg = new Msg("BarrierOperatorCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
			msg->SetInstance(this, _value.GetID().GetInstance());
			msg->Append(GetNodeId());
			msg->Append(3);
			msg->Append(GetCommandClassId());
			msg->Append(BarrierOperatorCmd_Set);
			msg->Append(position);
			msg->Append(GetDriver()->GetTransmitOptions());
			GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
			return true;
		}
	}
	if (ValueID::ValueType_Bool == _value.GetID().GetType())
	{
		if (idx == BarrierOperatorAudible_Index)
		{
			ValueBool const* value = static_cast<ValueBool const*>(&_value);
			Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperatorSignal::Set - Requesting Audible to be %s", value->GetValue() ? "ON" : "OFF");
			Msg* msg = new Msg("BarrierOperatorSignalCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
			msg->SetInstance(this, _value.GetID().GetInstance());
			msg->Append(GetNodeId());
			msg->Append(4);
			msg->Append(GetCommandClassId());
			msg->Append(BarrierOperatorCmd_SignalSet);
			msg->Append(BarrierOperatorSignalMask_Audible);
			msg->Append(value->GetValue() ? 0xFF : 0x00);
			msg->Append(GetDriver()->GetTransmitOptions());
			GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
			return true;
		}
		else if (idx == BarrierOperatorVisual_Index)
		{
			ValueBool const* value = static_cast<ValueBool const*>(&_value);
			Log::Write(LogLevel_Info, GetNodeId(), "BarrierOperatorSignal::Set - Requesting Visual to be %s", value->GetValue() ? "ON" : "OFF");
			Msg* msg = new Msg("BarrierOperatorSignalCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
			msg->SetInstance(this, _value.GetID().GetInstance());
			msg->Append(GetNodeId());
			msg->Append(4);
			msg->Append(GetCommandClassId());
			msg->Append(BarrierOperatorCmd_SignalSet);
			msg->Append(BarrierOperatorSignalMask_Visual);
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
		{
			vector<ValueList::Item> items;
			unsigned int size = (sizeof(c_BarrierOperator_States)/sizeof(c_BarrierOperator_States[0]));
			for( unsigned int i=0; i < size; i++)
			{
				ValueList::Item item;
				item.m_label = c_BarrierOperator_States[i];
				item.m_value = i;
				items.push_back( item );
			}
			node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, BarrierOperatorLabel_Index, "Barrier State", "", false, false, size, items, 0, 0 );
		}
		{
			vector<ValueList::Item> items;
			unsigned int size = (sizeof(c_BarrierOperator_Signals)/sizeof(c_BarrierOperator_Signals[0]));
			for( unsigned int i=0; i < size; i++)
			{
				ValueList::Item item;
				item.m_label = c_BarrierOperator_Signals[i];
				item.m_value = i;
				items.push_back( item );
			}
			node->CreateValueList( ValueID::ValueGenre_Config, GetCommandClassId(), _instance, BarrierOperatorSupportedSignals_Index, "Supported Signals", "", true, false, size, items, 0, 0 );
		}
		node->CreateValueBool(ValueID::ValueGenre_Config, GetCommandClassId(), _instance, BarrierOperatorAudible_Index, "Audible Notification", "", false, false, false, 0);
		node->CreateValueBool(ValueID::ValueGenre_Config, GetCommandClassId(), _instance, BarrierOperatorVisual_Index, "Visual Notification", "", false, false, false, 0);


	}
}

