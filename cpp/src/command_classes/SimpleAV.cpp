//-----------------------------------------------------------------------------
//
//	SimpleAVCommandItem.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SIMPLE_AV_CONTROL
//
//	Copyright (c) 2018
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
#include "command_classes/MultiInstance.h"
#include "command_classes/SimpleAV.h"
#include "command_classes/SimpleAVCommandItem.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include <vector>

#include "value_classes/ValueShort.h"
#include "value_classes/ValueList.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			uint8 m_sequence;

			enum SimpleAVCmd
			{
				SimpleAVCmd_Set = 0x01
			};

//-----------------------------------------------------------------------------
// <SimpleAV::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool SimpleAV::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance)
			{
				return true;
			}

//-----------------------------------------------------------------------------
// <SimpleAV::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
			bool SimpleAV::SetValue(Internal::VC::Value const& _value)
			{
				uint16 shortval;
				if (ValueID::ValueType_Short == _value.GetID().GetType())
				{
					Internal::VC::ValueShort const* value = static_cast<Internal::VC::ValueShort const*>(&_value);
					shortval = value->GetValue();
				}
				else if (ValueID::ValueType_List == _value.GetID().GetType())
				{
					Internal::VC::ValueList const* value = static_cast<Internal::VC::ValueList const*>(&_value);
					shortval = value->GetItem()->m_value;
				}
				else
					return false;

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
// <SimpleAV::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void SimpleAV::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					// Create list value
					vector<Internal::VC::ValueList::Item> items;
					vector<SimpleAVCommandItem> commands = SimpleAVCommandItem::GetCommands();
					vector<SimpleAVCommandItem>::iterator iterator;
					string helpList = "Possible values: \n";
					string helpNumeric = "Possible values: \n";
					for (iterator = commands.begin(); iterator != commands.end(); iterator++)
					{
						SimpleAVCommandItem command = *iterator;
						if (command.GetVersion() <= GetVersion())
						{
							Internal::VC::ValueList::Item item;
							item.m_value = command.GetCode();
							item.m_label = command.GetName();
							items.push_back(item);

#if 0
							// ValueList - create command description for help
							string codeStr = string("ZWave command number: ").append(std::to_string(command.GetCode()));
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
								std::to_string(command.GetCode())
								.append(" (")
								.append(command.GetName())
								.append(")\n");
							}
							else
							{
								descriptionFull =
								std::to_string(command.GetCode())
								.append(" (")
								.append(command.GetName())
								.append("; ")
								.append(command.GetDescription())
								.append(")\n");
							}
							helpNumeric += descriptionFull;
#endif
						}
					}

					node->CreateValueList(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SimpleAV::Command, string("OutputAVCommand_").append(std::to_string(_instance)), "", false, true, 2, items, 0, 0);

					// Create a similar numeric value
					// node->CreateValueShort(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, string("OutputAVCommandNumber_").append(std::to_string(_instance)), "", false, true, 0, 0, helpNumeric);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
