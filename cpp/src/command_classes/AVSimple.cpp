#include "command_classes/CommandClasses.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/AVSimple.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include <chrono>

#include "value_classes/ValueShort.h"

using namespace OpenZWave;

uint8 m_sequence;

enum SimpleAVCmd
{
	SimpleAVCmd_Set = 0x01
};

//-----------------------------------------------------------------------------
// <AVSimple::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool AVSimple::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance
)
{
	return true;
}

//-----------------------------------------------------------------------------
// <AVSimple::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
bool AVSimple::SetValue
(
	Value const& _value
)
{
	if (ValueID::ValueType_Short == _value.GetID().GetType())
	{
		ValueShort const* value = static_cast<ValueShort const*>(&_value);
		uint16 shortval = value->GetValue();
		uint8 instance = _value.GetID().GetInstance();
		Msg* msg = new Msg("SimpleAVCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
		msg->SetInstance(this, instance);
		msg->Append(GetNodeId());
		msg->Append(8); // Length
		msg->Append(GetCommandClassId());
		msg->Append(SimpleAVCmd_Set);
		msg->Append(m_sequence++); // Crutch
		msg->Append(0);
		msg->Append(0);
		msg->Append(0);
		msg->Append(shortval >> 8);
		msg->Append(shortval & 0xff);
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);

		if (m_sequence == 255)
			m_sequence = 0;

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <AVSimple::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void AVSimple::CreateVars
(
	uint8 const _instance
)
{
	if (Node* node = GetNodeUnsafe())
	{
		node->CreateValueShort(ValueID::ValueGenre_User, GetCommandClassId(), 1, 0, "OutputMain", "", false, true, 0, 0);
	}
}