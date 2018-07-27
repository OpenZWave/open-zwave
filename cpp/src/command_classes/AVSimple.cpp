#include "command_classes/CommandClasses.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/AVSimple.h"
#include "command_classes/AVSimpleCommandItem.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include <vector>

#include "value_classes/ValueShort.h"
#include "value_classes/ValueList.h"

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
	uint16 shortval;
	if (ValueID::ValueType_Short == _value.GetID().GetType())
	{
		ValueShort const* value = static_cast<ValueShort const*>(&_value);
		shortval = value->GetValue();
	}
	else if (ValueID::ValueType_List == _value.GetID().GetType())
	{
		ValueList const* value = static_cast<ValueList const*>(&_value);
		shortval = value->GetItem()->m_value;
	}
	else return false;

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
		// Create list value
		vector<ValueList::Item> items;
		vector<AVSimpleCommandItem> commands = AVSimpleCommandItem::GetCommands();
		vector<AVSimpleCommandItem>::iterator iterator;
		string helpList = "Possible values: \n";
		string helpNumeric = "Possible values: \n";
		for (iterator = commands.begin(); iterator != commands.end(); iterator++)
		{
			AVSimpleCommandItem command = *iterator;
			if (command.GetVersion() <= GetVersion())
			{
				ValueList::Item item;
				item.m_value = command.GetCode();
				item.m_label = command.GetName();
				items.push_back(item);

				// ValueList - create command description for help
				string codeStr = string("ZWave command number: ").append(to_string(command.GetCode()));
				string descriptionFull = "";
				if (command.GetDescription() == "")
				{
					descriptionFull = 
						command
						.GetName()
						.append(" (")
						.append(codeStr)
						.append(")\n");
				}
				else
				{
					descriptionFull = 
						command
						.GetName()
						.append(" (")
						.append(command.GetDescription())
						.append("; ")
						.append(codeStr)
						.append(")\n");
				}
				helpList += descriptionFull;

				// ValueShort - create command description for help 
				if (command.GetDescription() == "")
				{
					descriptionFull = 
						to_string(command.GetCode())
						.append(" (")
						.append(command.GetName())
						.append(")\n");
				}
				else
				{
					descriptionFull =
						to_string(command.GetCode())
						.append(" (")
						.append(command.GetName())
						.append("; ")
						.append(command.GetDescription())
						.append(")\n");
				}
				helpNumeric += descriptionFull;
			}
		}

		node->CreateValueList(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, string("OutputAVCommand_").append(to_string(_instance)), "", false, true, 2, items, 0, 0, helpList);
		
		// Create a similar numeric value
		// node->CreateValueShort(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, string("OutputAVCommandNumber_").append(to_string(_instance)), "", false, true, 0, 0, helpNumeric);
	}
}