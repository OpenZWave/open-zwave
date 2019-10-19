//-----------------------------------------------------------------------------
//
//	ValueList.cpp
//
//	Represents a list of items
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

#include "tinyxml.h"
#include "value_classes/ValueList.h"
#include "Msg.h"
#include "platform/Log.h"
#include "Manager.h"
#include "Localization.h"
#include <ctime>

namespace OpenZWave
{
	namespace Internal
	{
		namespace VC
		{

//-----------------------------------------------------------------------------
// <ValueList::ValueList>
// Constructor
//-----------------------------------------------------------------------------
			ValueList::ValueList(uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, vector<Item> const& _items, int32 const _valueIdx, uint8 const _pollIntensity, uint8 const _size	// = 4
					) :
					Value(_homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_List, _label, _units, _readOnly, _writeOnly, false, _pollIntensity), m_items(_items), m_valueIdx(_valueIdx), m_valueIdxCheck(0), m_size(_size)
			{
				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
				{
					/* first what is currently in m_label is the default text for a Item, so set it */
					Localization::Get()->SetValueItemLabel(m_id.GetNodeId(), _commandClassId, _index, -1, it->m_value, it->m_label, "");
					/* now set to the Localized Value */
					it->m_label = Localization::Get()->GetValueItemLabel(m_id.GetNodeId(), _commandClassId, _index, -1, it->m_value);
				}
			}

//-----------------------------------------------------------------------------
// <ValueList::ValueList>
// Constructor
//-----------------------------------------------------------------------------
			ValueList::ValueList() :
					Value(), m_items(), m_valueIdx(), m_valueIdxCheck(0), m_size(0)
			{

			}

//-----------------------------------------------------------------------------
// <ValueList::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
			void ValueList::ReadXML(uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement)
			{
				Value::ReadXML(_homeId, _nodeId, _commandClassId, _valueElement);

				// Get size of values
				int intSize;
				if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("size", &intSize))
				{
					if (intSize == 1 || intSize == 2 || intSize == 4)
					{
						m_size = intSize;
					}
					else
					{
						Log::Write(LogLevel_Warning, "Value size is invalid (%d). Only 1, 2 & 4 supported for node %d, class 0x%02x, instance %d, index %d", intSize, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
					}
				}
				else
				{
					Log::Write(LogLevel_Warning, "Value list size is not set, assuming 4 bytes for node %d, class 0x%02x, instance %d, index %d", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
				}

				TiXmlElement const* itemElement = _valueElement->FirstChildElement();

				bool shouldclearlist = true;
				while (itemElement)
				{
					char const* str = itemElement->Value();
					if (str && !strcmp(str, "Item"))
					{
						/* clear the existing list, if we have Item entries. (static list entries are created in the constructor
						 * here, we load up any localized labels
						 */
						if (shouldclearlist)
						{
							m_items.clear();
							shouldclearlist = false;
						}

						bool AddItem = true;
						char const* labelStr = itemElement->Attribute("label");
						char const* lang = "";
						if (itemElement->Attribute("lang"))
						{
							lang = itemElement->Attribute("lang");
							AddItem = false;
						}
						else
						{
							AddItem = true;
						}
						int value = 0;
						if (itemElement->QueryIntAttribute("value", &value) != TIXML_SUCCESS)
						{
							Log::Write(LogLevel_Warning, "Item value %s is wrong type or does not exist in xml configuration for node %d, class 0x%02x, instance %d, index %d", labelStr, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
							continue;
						}
						if ((m_size == 1 && value > 255) || (m_size == 2 && value > 65535))
						{
							Log::Write(LogLevel_Warning, "Item value %s is incorrect size in xml configuration for node %d, class 0x%02x, instance %d, index %d", labelStr, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
						}
						else
						{
							Localization::Get()->SetValueItemLabel(m_id.GetNodeId(), m_id.GetCommandClassId(), m_id.GetIndex(), -1, value, labelStr, lang);
							if (AddItem)
							{
								Item item;
								item.m_label = labelStr;
								item.m_value = value;
								m_items.push_back(item);
							}
						}
					}

					itemElement = itemElement->NextSiblingElement();
				}
				/* setup any Localization now as we should have read all available languages already */
				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
				{
					it->m_label = Localization::Get()->GetValueItemLabel(m_id.GetNodeId(), m_id.GetCommandClassId(), m_id.GetIndex(), -1, it->m_value);
				}

				// Set the value
				bool valSet = false;
				int intVal;
				m_valueIdx = 0;
				if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("value", &intVal))
				{
					valSet = true;
					intVal = GetItemIdxByValue(intVal);
					if (intVal != -1)
					{
						m_valueIdx = (int32) intVal;
					}
					else
					{
						Log::Write(LogLevel_Warning, "Value is not found in xml configuration for node %d, class 0x%02x, instance %d, index %d", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
					}
				}

				// Set the index
				bool indSet = false;
				int intInd = 0;
				if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("vindex", &intInd))
				{
					indSet = true;
					if (intInd >= 0 && intInd < (int32) m_items.size())
					{
						m_valueIdx = (int32) intInd;
					}
					else
					{
						Log::Write(LogLevel_Warning, "Vindex is out of range for index in xml configuration for node %d, class 0x%02x, instance %d, index %d", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
					}
				}
				if (!valSet && !indSet)
				{
					Log::Write(LogLevel_Warning, "Missing default list value or vindex from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
				}

			}

//-----------------------------------------------------------------------------
// <ValueList::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
			void ValueList::WriteXML(TiXmlElement* _valueElement)
			{
				Value::WriteXML(_valueElement);

				char str[16];
				snprintf(str, sizeof(str), "%d", m_valueIdx);
				_valueElement->SetAttribute("vindex", str);

				snprintf(str, sizeof(str), "%d", m_size);
				_valueElement->SetAttribute("size", str);

				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
				{
					TiXmlElement* pItemElement = new TiXmlElement("Item");
					pItemElement->SetAttribute("label", (*it).m_label.c_str());

					snprintf(str, sizeof(str), "%d", (*it).m_value);
					pItemElement->SetAttribute("value", str);

					_valueElement->LinkEndChild(pItemElement);
				}
			}

//-----------------------------------------------------------------------------
// <ValueList::SetByValue>
// Set a new value in the device, selected by item index
//-----------------------------------------------------------------------------
			bool ValueList::SetByValue(int32 const _value)
			{
				// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
				ValueList* tempValue = new ValueList(*this);
				tempValue->m_valueIdx = _value;

				// Set the value in the device.
				bool ret = ((Value*) tempValue)->Set();

				// clean up the temporary value
				delete tempValue;

				return ret;
			}

//-----------------------------------------------------------------------------
// <ValueList::SetByLabel>
// Set a new value in the device, selected by item label
//-----------------------------------------------------------------------------
			bool ValueList::SetByLabel(string const& _label)
			{
				// Ensure the value is one of the options
				int index = GetItemIdxByLabel(_label);
				if (index < 0)
				{
					// Item not found
					return false;
				}

				return SetByValue(index);
			}

//-----------------------------------------------------------------------------
// <ValueList::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
			void ValueList::OnValueRefreshed(int32 const _value)
			{
				// Ensure the value is one of the options
				int32 index = GetItemIdxByValue(_value);
				if (index < 0)
				{
					// Item not found
					return;
				}

				switch (VerifyRefreshedValue((void*) &m_valueIdx, (void*) &m_valueIdxCheck, (void*) &index, ValueID::ValueType_List))
				{
					case 0:		// value hasn't changed, nothing to do
						break;
					case 1:		// value has changed (not confirmed yet), save _value in m_valueCheck
						m_valueIdxCheck = index;
						break;
					case 2:		// value has changed (confirmed), save _value in m_value
						m_valueIdx = index;
						break;
					case 3:		// all three values are different, so wait for next refresh to try again
						break;
				}
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemIdxByLabel>
// Get the index of an item from its label
//-----------------------------------------------------------------------------
			int32 ValueList::GetItemIdxByLabel(string const& _label) const
			{
				for (int32 i = 0; i < (int32) m_items.size(); ++i)
				{
					if (_label == m_items[i].m_label)
					{
						return i;
					}
				}

				return -1;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemIdxByValue>
// Get the index of an item from its value
//-----------------------------------------------------------------------------
			int32 ValueList::GetItemIdxByValue(int32 const _value) const
			{
				for (int32 i = 0; i < (int32) m_items.size(); ++i)
				{
					if (_value == m_items[i].m_value)
					{
						return i;
					}
				}

				return -1;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemLabels>
// Fill a vector with the item labels
//-----------------------------------------------------------------------------
			bool ValueList::GetItemLabels(vector<string>* o_items)
			{
				if (o_items)
				{
					for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
					{
						o_items->push_back((*it).m_label);
					}

					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemValues>
// Fill a vector with the item values
//-----------------------------------------------------------------------------
			bool ValueList::GetItemValues(vector<int32>* o_values)
			{
				if (o_values)
				{
					for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
					{
						o_values->push_back((*it).m_value);
					}

					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItem>
// Get the Item at the Currently selected Index
//-----------------------------------------------------------------------------
			ValueList::Item const *ValueList::GetItem() const
			{
				try
				{
					return &m_items.at(m_valueIdx);
				}
				catch (const std::out_of_range& oor)
				{
					Log::Write(LogLevel_Warning, "Invalid Index Set on ValueList: %s", oor.what());
					return NULL;
				}
			}
		} // namespace VC
	} // namespace Internal
} // namespace OpenZWave
