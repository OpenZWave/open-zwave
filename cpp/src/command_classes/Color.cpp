//-----------------------------------------------------------------------------
//
//	Color.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_COLOR
//
//	Copyright (c) 2014 GizMoCuz
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

#include <iostream>
#include <iomanip>

#include "command_classes/CommandClasses.h"
#include "command_classes/Color.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueInt.h"
#include "value_classes/ValueString.h"
#include "value_classes/ValueByte.h"

#include "tinyxml.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum ColorCmd
			{
				ColorCmd_Capability_Get = 0x01,
				ColorCmd_Capability_Report = 0x02,
				ColorCmd_Get = 0x03,
				ColorCmd_Report = 0x04,
				ColorCmd_Set = 0x05,
				/* Version 2 */
				ColorCmd_StartCapabilityLevelChange = 0x06,
				ColorCmd_StopStateChange = 0x07
			};

//Capabilities
//0=Warm White (0x00 - 0xFF: 0 100%)
//1=Cold White (0x00 - 0xFF: 0 100%)
//2=Red (0x00 - 0xFF: 0 100%)
//3=Green (0x00 - 0xFF: 0 100%)
//4=Blue (0x00 - 0xFF: 0 100%)
//5=Amber (for 6ch Color mixing) (0x00 - 0xFF: 0 100%)
//6=Cyan (for 6ch Color mixing)  (0x00 - 0xFF: 0 100%)
//7=Purple (for 6ch Color mixing)  (0x00 - 0xFF: 0 100%)
//8=Indexed Color (0x00 0x0FF: Color Index 0 - 255

			enum ColorIDX
			{
				COLORIDX_WARMWHITE,
				COLORIDX_COLDWHITE,
				COLORIDX_RED,
				COLORIDX_GREEN,
				COLORIDX_BLUE,
				COLORIDX_AMBER,
				COLORIDX_CYAN,
				COLORIDX_PURPLE,
				COLORIDX_INDEXCOLOR
			};

			static char const* c_ColorIndex[] =
			{ "Off", "Cool White", "Warm White", "Red", "Lime", "Blue", "Yellow", "Cyan", "Magenta", "Silver", "Gray", "Maroon", "Olive", "Green", "Purple", "Teal", "Navy", "Custom" };

			enum ValueIDSystemIndexes
			{
			};

			Color::Color(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId), m_refreshinprogress(false), m_coloridxcount(0)
			{
				m_com.EnableFlag(COMPAT_FLAG_COLOR_IDXBUG, false);
				m_dom.EnableFlag(STATE_FLAG_COLOR_CHANNELS, 0);

				for (uint8 i = 0; i < 9; i++)
					m_colorvalues[i] = 0;
				SetStaticRequest(StaticRequest_Values);
			}

//-----------------------------------------------------------------------------
// <Color::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool Color::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool requests = false;
				if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
				{
					/* if we havn't got a valid capability from our Config File, then get what the device is capable of. If the user changes the Color Channels Value we use the stored version instead */
					if (m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS) == 0)
					{
						Msg* msg = new Msg("ColorCmd_CapabilityGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(ColorCmd_Capability_Get);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
					}
					return true;
				}
				if (_requestFlags & RequestFlag_Dynamic)
				{
					/* do a request for each channel. the RequestValue will filter out channels that are not supported
					 * by the device
					 */
					if (m_refreshinprogress == true)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Color Refresh in progress");
						return false;
					}

					/* if the device has the ColorIDX bug, then only request the first channel. We will get the rest later */
					for (int i = 0; i <= 9; i++)
					{
						bool tmprequests = RequestColorChannelReport(i, _instance, _queue);
						if (tmprequests)
							m_coloridxcount = i;
						requests |= tmprequests;
						if (m_com.GetFlagBool(COMPAT_FLAG_COLOR_IDXBUG) && tmprequests)
						{
							m_refreshinprogress = true;
							break;
						}
					}
				}

				return requests;
			}

//-----------------------------------------------------------------------------
// <Color::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool Color::RequestValue(uint32 const _requestFlags, uint16 const _what, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED) && (_what == ValueID_Index_Color::Color || _what == ValueID_Index_Color::Index))
				{
					if (m_com.GetFlagBool(COMPAT_FLAG_COLOR_IDXBUG) && m_refreshinprogress == true)
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "ColorRefresh is already in progress. Ignoring Get Request");
						return false;
					}
					for (int i = 0; i <= 9; i++)
					{
						if (RequestColorChannelReport(i, _instance, _queue))
						{
							if (m_com.GetFlagBool(COMPAT_FLAG_COLOR_IDXBUG))
							{
								m_refreshinprogress = true;
								m_coloridxcount = 0;
								return true;
							}
						}
					}
				}
				return false;
			}

			bool Color::RequestColorChannelReport(uint8 const coloridx, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				/* make sure our coloridx is valid */
				if ((m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS)) & (1 << (coloridx)))
				{
					Msg* msg = new Msg("ColorCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(3);
					msg->Append(GetCommandClassId());
					msg->Append(ColorCmd_Get);
					msg->Append(coloridx);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				return false;

			}

//-----------------------------------------------------------------------------
// <GetColor>
// Get a Specific Color Value from a position in a RGB String
// its assumed the string is formated such as #RRGGBB[WWCWAMCYPR]
// where the color values in [] are optional.
// throws a exception when position is out of bounds
//-----------------------------------------------------------------------------
			uint16 GetColor(std::string rgbstring, uint8 const position)
			{

				/* check the length of the string based on position value we passed in including the #*/
				if (rgbstring.length() < (size_t) (position * 2) + 1)
				{
					OpenZWave::Log::Write(OpenZWave::LogLevel_Warning, "Request for Color Position %d exceeds String Length: %s", position, rgbstring.c_str());
					throw;
				}
				std::string result = rgbstring.substr(((position - 1) * 2) + 1, 2);
				std::stringstream ss(result);
				uint16 rawresult;
				ss >> std::hex >> rawresult;
				return rawresult;
			}

//-----------------------------------------------------------------------------
// <Color::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Color::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (ColorCmd_Capability_Report == (ColorCmd) _data[0])
				{
					m_dom.SetFlagShort(STATE_FLAG_COLOR_CHANNELS, (_data[1] + (_data[2] << 8)));
					uint16_t f_capabilities = m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS);
					string helpstr = "#RRGGBB";
					Log::Write(LogLevel_Info, GetNodeId(), "Received an Color Capability Report: Capability=%x", f_capabilities);
					if (f_capabilities & 0x04)
						Log::Write(LogLevel_Info, GetNodeId(), "Red (0x02)");
					if (f_capabilities & 0x08)
						Log::Write(LogLevel_Info, GetNodeId(), "Green (0x03)");
					if (f_capabilities & 0x10)
						Log::Write(LogLevel_Info, GetNodeId(), "Blue (0x04)");
					if (f_capabilities & 0x01)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Warm White (0x00)");
						helpstr += "WW";
					}
					if (f_capabilities & 0x02)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Cold White (0x01)");
						helpstr += "CW";
					}
					if (f_capabilities & 0x20)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Amber (0x05)");
						helpstr += "AM";
					}
					if (f_capabilities & 0x40)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Cyan (0x06)");
						helpstr += "CY";
					}
					if (f_capabilities & 0x80)
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Purple (0x07)");
						helpstr += "PR";
					}
					if (f_capabilities & 0x100)
						Log::Write(LogLevel_Info, GetNodeId(), "Indexed Color (0x08)");
					if (Internal::VC::ValueInt* colorchannels = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_Color::Channels_Capabilities)))
					{
						colorchannels->OnValueRefreshed(f_capabilities);
						colorchannels->Release();
					}
					if (Node* node = GetNodeUnsafe())
					{
						if (Internal::VC::ValueString *color = static_cast<Internal::VC::ValueString *>(GetValue(_instance, ValueID_Index_Color::Color)))
						{
							color->SetUnits(helpstr);
						}
						else
						{
							node->CreateValueString(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_Color::Color, "Color", helpstr, false, false, "#000000", 0);
						}

						/* always create the ColorIndex Value - If the device doesn't support Indexed Colors, we fake it up when sending */
						{
							vector<Internal::VC::ValueList::Item> items;
							unsigned int size = (sizeof(c_ColorIndex) / sizeof(c_ColorIndex[0]));
							for (unsigned int i = 0; i < size; i++)
							{
								Internal::VC::ValueList::Item item;
								item.m_label = c_ColorIndex[i];
								item.m_value = i;
								items.push_back(item);
							}
							node->CreateValueList(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_Color::Index, "Color Index", "", false, false, (sizeof(c_ColorIndex) / sizeof(c_ColorIndex[0])), items, 0, 0);

						}

						if (GetVersion() > 1)
							node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_Color::Duration, "Duration", "Sec", false, false, 255, 0);

					}
					return true;
				}
				if (ColorCmd_Report == (ColorCmd) _data[0])
				{
					/* Fibaro is messed up. It always returns 0x03 as the Coloridx in a report message, but the values are correct for what the request was.
					 * I read a forum post about it being a bug. If this is a case, its going to make this class messy as hell :(
					 *
					 * http://forum.z-wave.me/viewtopic.php?f=3422&t=20042
					 *
					 * Received: 0x01, 0x0a, 0x00, 0x04, 0x00, 0x34, 0x04, 0x33, 0x04, 0x03, 0xff, 0x0a
					 *                                                                 ^^^^
					 *                                                                 Always 0x03
					 *                                                                       ^^^^
					 *                                                                       But this appears to be the right value for the channel we requested
					 *
					 */
					/* request the next color index if we are bugged */
					uint8 coloridx = _data[1];
					if (m_com.GetFlagBool(COMPAT_FLAG_COLOR_IDXBUG))
					{
						coloridx = m_coloridxcount;
						for (uint8 idx = m_coloridxcount + 1; idx < 9; idx++)
						{
							if (RequestColorChannelReport(idx, _instance, Driver::MsgQueue_Send))
							{
								m_coloridxcount = idx;
								break;
							}
						}
					}
					m_colorvalues[coloridx] = _data[2];

					/* test if there are any more valid coloridx */
					for (uint8 idx = coloridx + 1; idx < 9; idx++)
					{
						if ((m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS)) & (1 << (idx)))
						{
							return true;
						}
					}
					if (m_com.GetFlagBool(COMPAT_FLAG_COLOR_IDXBUG))
						m_refreshinprogress = false;

					/* if we get here, then we can update our ValueID */
					if (Internal::VC::ValueString* color = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_Color::Color)))
					{
						/* create a RGB[W] String */
						std::stringstream ss;
						std::stringstream ssbuf;
						bool usingbuf = false;
						uint16_t f_capabilities = m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS);
						ss << "#";
						/* do R */
						if ((f_capabilities) & (1 << (COLORIDX_RED)))
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_RED];
						else
							ss << "00";
						/* do G */
						if ((f_capabilities) & (1 << (COLORIDX_GREEN)))
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_GREEN];
						else
							ss << "00";
						/* do B */
						if ((f_capabilities) & (1 << (COLORIDX_BLUE)))
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_BLUE];
						else
							ss << "00";

						/* if both whites are present.... */
						if (((f_capabilities) & (1 << (COLORIDX_WARMWHITE))) && ((f_capabilities) & (1 << (COLORIDX_COLDWHITE))))
						{
							/* append them both */
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_WARMWHITE];
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_COLDWHITE];
						}
						else if ((f_capabilities) & (1 << (COLORIDX_WARMWHITE)))
						{
							/* else, if the warm white is present, append that */
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_WARMWHITE];
						}
						else if ((f_capabilities) & (1 << (COLORIDX_COLDWHITE)))
						{
							/* else, if the cold white is present, append that */
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_COLDWHITE];
						}
						else
						{
							/* we put 0000 into our buffer to represent both Warm and Cold white */
							ssbuf << "0000";
							usingbuf = true;
						}
						if ((f_capabilities) & (1 << (COLORIDX_AMBER)))
						{
							/* if AMBER is present, append our buffer if needed */
							if (usingbuf)
							{
								ss << ssbuf.str();
								ssbuf.str("");
								ssbuf.clear();
								usingbuf = false;
							}
							/* and then our Color */
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_AMBER];
						}
						else
						{
							/* put 00 into our buffer */
							ssbuf << "00";
							usingbuf = true;
						}
						if ((f_capabilities) & (1 << (COLORIDX_CYAN)))
						{
							/* if CYAN is present, append our buffer if needed */
							if (usingbuf)
							{
								ss << ssbuf.str();
								ssbuf.str("");
								ssbuf.clear();
								usingbuf = false;
							}
							/* and then our Color */
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_CYAN];
						}
						else
						{
							/* put 00 into our buffer */
							ssbuf << "00";
							usingbuf = true;
						}
						if ((f_capabilities) & (1 << (COLORIDX_PURPLE)))
						{
							/* if PURPLE is present, append our buffer if needed */
							if (usingbuf)
							{
								ss << ssbuf.str();
								ssbuf.str("");
								ssbuf.clear();
								usingbuf = false;
							}
							/* and then our Color */
							ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int) m_colorvalues[COLORIDX_PURPLE];
						}
						/* No need for a else case here as COLORIDX_PURPLE is the last color. If its not supported, we
						 * don't put anything in our Color String
						 */

						Log::Write(LogLevel_Info, GetNodeId(), "Received a updated Color from Device: %s", ss.str().c_str());
						color->OnValueRefreshed(string(ss.str()));
						color->Release();

						/* if we don't support the Color Index then fake it */
						if (!(f_capabilities & (1 << (COLORIDX_INDEXCOLOR))))
						{
							if (Internal::VC::ValueList* coloridx = static_cast<Internal::VC::ValueList*>(GetValue(_instance, ValueID_Index_Color::Index)))
							{
								/* it supports the AMBER/CYAN/PURPLE Channels */
								if (f_capabilities > 31)
								{
									/* Custom */
									coloridx->OnValueRefreshed(17);
									coloridx->Release();
									return true;
								}
								if ((f_capabilities) & (1 << (COLORIDX_WARMWHITE)))
								{
									if (ss.str().substr(0, 9) == "#000000FF")
									{
										/* Warm White */
										coloridx->OnValueRefreshed(2);
										coloridx->Release();
										return true;
									}
								}
								if ((f_capabilities) & (1 << (COLORIDX_COLDWHITE)))
								{
									if (ss.str().substr(0, 11) == "#00000000FF")
									{
										/* Cool White */
										coloridx->OnValueRefreshed(1);
										coloridx->Release();
										return true;
									}
								}
								if (ss.str().substr(0, 7) == "#000000")
								{
									/* off */
									coloridx->OnValueRefreshed(0);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#FFFFFF")
								{
									/* White */
									coloridx->OnValueRefreshed(1);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#FF9329")
								{
									/* warm white */
									coloridx->OnValueRefreshed(2);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#FF0000")
								{
									/* red */
									coloridx->OnValueRefreshed(3);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#00FF00")
								{
									/* lime */
									coloridx->OnValueRefreshed(4);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#0000FF")
								{
									/* blue */
									coloridx->OnValueRefreshed(5);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#FFFF00")
								{
									/* yellow */
									coloridx->OnValueRefreshed(6);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#00FFFF")
								{
									/* Cyan */
									coloridx->OnValueRefreshed(7);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#FF00FF")
								{
									/* Magenta */
									coloridx->OnValueRefreshed(8);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#C0C0C0")
								{
									/* Silver */
									coloridx->OnValueRefreshed(9);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#808080")
								{
									/* gray */
									coloridx->OnValueRefreshed(10);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#800000")
								{
									/* maroon */
									coloridx->OnValueRefreshed(11);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#808000")
								{
									/* Olive */
									coloridx->OnValueRefreshed(12);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#008000")
								{
									/* green */
									coloridx->OnValueRefreshed(13);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#800080")
								{
									/* purple */
									coloridx->OnValueRefreshed(14);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#008080")
								{
									/* teal */
									coloridx->OnValueRefreshed(15);
									coloridx->Release();
									return true;
								}
								else if (ss.str().substr(0, 7) == "#000080")
								{
									/* navy */
									coloridx->OnValueRefreshed(16);
									coloridx->Release();
									return true;
								}
								else
								{
									/* custom */
									coloridx->OnValueRefreshed(17);
									coloridx->Release();
									return true;
								}

							}
						}
					}
					/* if we got a updated Color Index Value - Update our ValueID */
					if ((m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS)) & (1 << (COLORIDX_INDEXCOLOR)))
					{
						if (Internal::VC::ValueList* coloridx = static_cast<Internal::VC::ValueList*>(GetValue(_instance, ValueID_Index_Color::Index)))
						{
							coloridx->OnValueRefreshed(m_colorvalues[COLORIDX_INDEXCOLOR]);
							coloridx->Release();
						}
					}
					return true;
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Color::SetValue>
// Set the device's Color value
//-----------------------------------------------------------------------------
			bool Color::SetValue(Internal::VC::Value const& _value)
			{
				if (ValueID_Index_Color::Color == _value.GetID().GetIndex())
				{
					Internal::VC::ValueString const* value = static_cast<Internal::VC::ValueString const*>(&_value);
					string s = value->GetValue();
					/* make sure the first char is # */
					if (s.at(0) != '#')
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Color::SetValue - String is Malformed. Missing #: %s", s.c_str());
						return false;
					}
					/* length minus the # has to be multiple of 2's */
					if ((s.length() - 1) % 2 != 0)
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Color::SetValue - Uneven Length string. Each Color should be 2 chars: %s", s.c_str());
						return false;
					}

					/* the size of the string tells us how many colors are set */
					uint8_t nocols = (uint8_t) ((s.length() - 1) / 2) & 0xFF;
					uint8_t colvals[8] =
					{ 0, 0, 0, 0, 0, 0, 0, 0 };
					bool colvalset[8] =
					{ false, false, false, false, false, false, false, false };

					try
					{
						/* we always will have at least RGB */
						colvals[COLORIDX_RED] = GetColor(s, 1);
						colvals[COLORIDX_GREEN] = GetColor(s, 2);
						colvals[COLORIDX_BLUE] = GetColor(s, 3);

						/* if nocols = 4 then allwhite is set */
						if (nocols == 4)
						{
							/* use ww as the var for all white */
							colvals[COLORIDX_WARMWHITE] = GetColor(s, 4);
							colvalset[COLORIDX_WARMWHITE] = true;
						}
						if (nocols >= 5)
						{
							/* warm white and cold white are set */
							colvals[COLORIDX_WARMWHITE] = GetColor(s, 4);
							colvals[COLORIDX_COLDWHITE] = GetColor(s, 5);
							colvalset[COLORIDX_WARMWHITE] = true;
							colvalset[COLORIDX_COLDWHITE] = true;
						}
						if (nocols >= 6)
						{
							/* amber is also set */
							colvals[COLORIDX_AMBER] = GetColor(s, 6);
							colvalset[COLORIDX_AMBER] = true;
						}
						if (nocols >= 7)
						{
							/* cyan is also set */
							colvals[COLORIDX_CYAN] = GetColor(s, 7);
							colvalset[COLORIDX_CYAN] = true;
						}
						if (nocols == 8)
						{
							/* purple is also set */
							colvals[COLORIDX_PURPLE] = GetColor(s, 8);
							colvalset[COLORIDX_PURPLE] = true;
						}
					}
					catch (...)
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "Color::SetValue - Color String Decoding Failed: %s", s.c_str());
						return false;
					}
					Log::Write(LogLevel_Info, GetNodeId(), "Color::SetValue - Setting Color value");

					Msg* msg = new Msg("ColorCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, false);
					msg->SetInstance(this, _value.GetID().GetInstance());
					msg->Append(GetNodeId());
					if (GetVersion() > 1)
						msg->Append(3 + (nocols * 2) + 1); // each color 2 bytes - and 1 byte for duration
					else
						msg->Append(3 + (nocols * 2)); // each color 2 bytes
					msg->Append(GetCommandClassId());
					msg->Append(ColorCmd_Set); //cmd
					msg->Append(nocols);
					msg->Append(COLORIDX_RED);
					msg->Append(colvals[COLORIDX_RED]);
					msg->Append(COLORIDX_GREEN);
					msg->Append(colvals[COLORIDX_GREEN]);
					msg->Append(COLORIDX_BLUE);
					msg->Append(colvals[COLORIDX_BLUE]);
					if (colvalset[COLORIDX_WARMWHITE] & !colvalset[COLORIDX_COLDWHITE])
					{
						msg->Append(COLORIDX_WARMWHITE);
						msg->Append(colvals[COLORIDX_WARMWHITE]);
					}
					if (colvalset[COLORIDX_WARMWHITE] && colvalset[COLORIDX_COLDWHITE])
					{
						msg->Append(COLORIDX_WARMWHITE);
						msg->Append(colvals[COLORIDX_WARMWHITE]);
						msg->Append(COLORIDX_COLDWHITE);
						msg->Append(colvals[COLORIDX_COLDWHITE]);
					}
					if (colvalset[COLORIDX_AMBER])
					{
						msg->Append(COLORIDX_AMBER);
						msg->Append(colvals[COLORIDX_AMBER]);
					}
					if (colvalset[COLORIDX_CYAN])
					{
						msg->Append(COLORIDX_CYAN);
						msg->Append(colvals[COLORIDX_CYAN]);
					}
					if (colvalset[COLORIDX_PURPLE])
					{
						msg->Append(COLORIDX_PURPLE);
						msg->Append(colvals[COLORIDX_PURPLE]);
					}
					if (GetVersion() > 1)
					{
						uint8 duration = 0;
						if (Internal::VC::ValueByte *valduration = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), ValueID_Index_Color::Duration)))
						{
							duration = valduration->GetValue();
						}
						msg->Append(duration);
					}
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);

					return true;
				}
				else if (ValueID_Index_Color::Index == _value.GetID().GetIndex())
				{

					Internal::VC::ValueList const* value = static_cast<Internal::VC::ValueList const*>(&_value);
					if (value->GetItem() == NULL)
					{
						return false;
					}
					uint8 index = value->GetItem()->m_value;
					if ((m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS)) & (1 << (COLORIDX_INDEXCOLOR)))
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Color::SetValue - Setting Color Index Value (Real)");

						Msg* msg = new Msg("Value_Color_Index", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, false);
						msg->SetInstance(this, _value.GetID().GetInstance());
						msg->Append(GetNodeId());
						if (GetVersion() > 1)
							msg->Append(3 + 2 + 1); // each color 2 bytes - and 1 byte for duration
						else
							msg->Append(3 + 2); // each color 2 bytes
						msg->Append(GetCommandClassId());
						msg->Append(ColorCmd_Set); //cmd
						msg->Append(1);
						msg->Append(COLORIDX_INDEXCOLOR);
						msg->Append(index);
						if (GetVersion() > 1)
						{
							uint8 duration = 0;
							if (Internal::VC::ValueByte *valduration = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), ValueID_Index_Color::Duration)))
							{
								duration = valduration->GetValue();
							}
							msg->Append(duration);
						}
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						return true;
					}
					else
					{
						Log::Write(LogLevel_Info, GetNodeId(), "Color::SetValue - Setting Color Index Value (Fake)");

						/* figure out the size */
						uint8 nocols = 3;
						uint16_t f_capabilities = m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS);
						if ((f_capabilities) & (1 << (COLORIDX_WARMWHITE)))
						{
							nocols++;
						}
						if ((f_capabilities) & (1 << (COLORIDX_COLDWHITE)))
						{
							nocols++;
						}
						if ((f_capabilities) & (1 << (COLORIDX_AMBER)))
						{
							nocols++;
						}
						if ((f_capabilities) & (1 << (COLORIDX_CYAN)))
						{
							nocols++;
						}
						if ((f_capabilities) & (1 << (COLORIDX_PURPLE)))
						{
							nocols++;
						}
						Msg* msg = new Msg("ColorCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, false);
						msg->SetInstance(this, _value.GetID().GetInstance());
						msg->Append(GetNodeId());
						if (GetVersion() > 1)
							msg->Append(3 + (nocols * 2) + 1); // each color 2 bytes - and 1 byte for duration
						else
							msg->Append(3 + (nocols * 2)); // each color 2 bytes
						msg->Append(GetCommandClassId());
						msg->Append(ColorCmd_Set); //cmd
						msg->Append(nocols);
						bool cwset = false;
						bool wwset = false;
						/* fake it */
						switch (index)
						{
							case 0: /* "Off" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
							case 1: /* Cool White" */
								if ((f_capabilities) & (1 << (COLORIDX_COLDWHITE)))
								{
									if ((f_capabilities) & (1 << (COLORIDX_WARMWHITE)))
									{
										msg->Append(COLORIDX_WARMWHITE);
										msg->Append(0x00);
										wwset = true;
									}
									if ((f_capabilities) & (1 << (COLORIDX_COLDWHITE)))
									{
										msg->Append(COLORIDX_COLDWHITE);
										msg->Append(0xFF);
										cwset = true;
									}
									msg->Append(COLORIDX_RED);
									msg->Append(0x00);
									msg->Append(COLORIDX_GREEN);
									msg->Append(0x00);
									msg->Append(COLORIDX_BLUE);
									msg->Append(0x00);
								}
								else
								{
									msg->Append(COLORIDX_RED);
									msg->Append(0xFF);
									msg->Append(COLORIDX_GREEN);
									msg->Append(0xFF);
									msg->Append(COLORIDX_BLUE);
									msg->Append(0xFF);
								}
								break;
							case 2: /* Warm White */
								if ((f_capabilities) & (1 << (COLORIDX_WARMWHITE)))
								{
									if ((f_capabilities) & (1 << (COLORIDX_WARMWHITE)))
									{
										msg->Append(COLORIDX_WARMWHITE);
										msg->Append(0xFF);
										wwset = true;
									}
									if ((f_capabilities) & (1 << (COLORIDX_COLDWHITE)))
									{
										msg->Append(COLORIDX_COLDWHITE);
										msg->Append(0x00);
										cwset = true;
									}
									msg->Append(COLORIDX_RED);
									msg->Append(0x00);
									msg->Append(COLORIDX_GREEN);
									msg->Append(0x00);
									msg->Append(COLORIDX_BLUE);
									msg->Append(0x00);
								}
								else
								{
									msg->Append(COLORIDX_RED);
									msg->Append(0xFF);
									msg->Append(COLORIDX_GREEN);
									msg->Append(0x93);
									msg->Append(COLORIDX_BLUE);
									msg->Append(0x29);
								}
								break;
							case 3: /* "Red" */
								msg->Append(COLORIDX_RED);
								msg->Append(0xFF);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
							case 4: /* "Lime" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0xFF);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
							case 5: /* 	"Blue" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0xFF);
								break;
							case 6: /* 	"Yellow" */
								msg->Append(COLORIDX_RED);
								msg->Append(0xFF);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0xFF);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
							case 7: /* 	"Cyan" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0xFF);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0xFF);
								break;
							case 8: /*	"Magenta" */
								msg->Append(COLORIDX_RED);
								msg->Append(0xFF);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0xFF);
								break;
							case 9: /* 	"Silver" */
								msg->Append(COLORIDX_RED);
								msg->Append(0xC0);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0xC0);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0xC0);
								break;
							case 10: /* "Gray" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x80);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x80);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x80);
								break;
							case 11: /*	"Maroon" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x80);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
							case 12: /*	"Olive" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x80);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x80);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
							case 13: /*	"Green" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x80);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
							case 14: /* "Purple" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x80);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x80);
								break;
							case 15: /* "Teal" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x80);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x80);
								break;
							case 16: /* "Navy" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x80);
								break;
							case 17: /*	"Custom" */
								msg->Append(COLORIDX_RED);
								msg->Append(0x00);
								msg->Append(COLORIDX_GREEN);
								msg->Append(0x00);
								msg->Append(COLORIDX_BLUE);
								msg->Append(0x00);
								break;
						}
						if (((f_capabilities) & (1 << (COLORIDX_WARMWHITE))) && !wwset)
						{
							msg->Append(COLORIDX_WARMWHITE);
							msg->Append(0x00);
						}
						if (((f_capabilities) & (1 << (COLORIDX_COLDWHITE))) && !cwset)
						{
							msg->Append(COLORIDX_COLDWHITE);
							msg->Append(0x00);
						}
						if ((f_capabilities) & (1 << (COLORIDX_AMBER)))
						{
							msg->Append(COLORIDX_AMBER);
							msg->Append(0x00);
						}
						if ((f_capabilities) & (1 << (COLORIDX_CYAN)))
						{
							msg->Append(COLORIDX_CYAN);
							msg->Append(0x00);
						}
						if ((f_capabilities) & (1 << (COLORIDX_PURPLE)))
						{
							msg->Append(COLORIDX_PURPLE);
							msg->Append(0x00);
						}
						if (GetVersion() > 1)
						{
							uint8 duration = 0;
							if (Internal::VC::ValueByte *valduration = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), ValueID_Index_Color::Duration)))
							{
								duration = valduration->GetValue();
							}
							msg->Append(duration);
						}
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						return true;
					}
				}
				else if (ValueID_Index_Color::Duration == _value.GetID().GetIndex())
				{
					Log::Write(LogLevel_Info, GetNodeId(), "Color::SetValue - Setting Color Fade Duration");
					Internal::VC::ValueByte const* value = static_cast<Internal::VC::ValueByte const*>(&_value);
					uint8 _duration = value->GetValue();
					if (Internal::VC::ValueByte * m_value = static_cast<Internal::VC::ValueByte *>(GetValue(_value.GetID().GetInstance(), ValueID_Index_Color::Duration)))
					{
						m_value->OnValueRefreshed(_duration);
						m_value->Release();
					}
					return true;
				}
				else if (ValueID_Index_Color::Channels_Capabilities == _value.GetID().GetIndex())
				{
					Log::Write(LogLevel_Info, GetNodeId(), "Color::SetValue - Setting Color Channels");
					Internal::VC::ValueInt const* value = static_cast<Internal::VC::ValueInt const*>(&_value);
					m_dom.SetFlagShort(STATE_FLAG_COLOR_CHANNELS, value->GetValue());
					/* if the Capabilities is set to 0 by the user, then refresh the defaults from the device */
					if (m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS) == 0)
					{
						Msg* msg = new Msg("ColorCmd_CapabilityGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(ColorCmd_Capability_Get);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
					}
					if (Internal::VC::ValueInt *m_value = static_cast<Internal::VC::ValueInt *>(GetValue(_value.GetID().GetInstance(), ValueID_Index_Color::Channels_Capabilities)))
					{
						m_value->OnValueRefreshed(m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS));
						m_value->Release();
					}
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Color::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void Color::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					/* XXX TODO convert this to a bitset when we implement */
					node->CreateValueInt(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_Color::Channels_Capabilities, "Color Channels", "", false, false, m_dom.GetFlagShort(STATE_FLAG_COLOR_CHANNELS), 0);
				}

			}

//-----------------------------------------------------------------------------
// <Color::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
			void Color::SetValueBasic(uint8 const _instance, uint8 const _value)
			{
				RequestValue(0, ValueID_Index_Color::Color, _instance, Driver::MsgQueue_Send);
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
