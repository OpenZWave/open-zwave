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

#include "command_classes/CommandClasses.h"
#include "command_classes/Color.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueInt.h"
#include "value_classes/ValueString.h"

#include "tinyxml.h"
#include <iomanip>

using namespace OpenZWave;

enum ColorCmd
{
	ColorCmd_Capability_Get = 0x01,
	ColorCmd_Capability_Report = 0x02,
	ColorCmd_Get = 0x03,
	ColorCmd_Get_Report = 0x04,
	ColorCmd_Set = 0x05,
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

enum ColorIDX {
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

Color::Color
(
uint32 const _homeId,
uint8 const _nodeId
) :
CommandClass(_homeId, _nodeId),
m_capabilities(0),
m_coloridxbug(false),
m_coloridxcount(0)
{
	for (uint8 i = 0; i < 9; i++)
		m_colorvalues[i] = 0;
	SetStaticRequest(StaticRequest_Values);
}

//-----------------------------------------------------------------------------
// <Color::ReadXML>
// Read configuration.
//-----------------------------------------------------------------------------
void Color::ReadXML
(
TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML(_ccElement);

	int32 intVal;
	if (TIXML_SUCCESS == _ccElement->QueryIntAttribute("colorchannels", &intVal))
	{
		if (intVal <= 0x100) {
			m_capabilities = (uint16)intVal;
		}
	}
	char const* str = _ccElement->Attribute("coloridxbug");
	if (str)
	{
		m_coloridxbug = !strcmp(str, "true");
	}
}

//-----------------------------------------------------------------------------
// <Color::WriteXML>
// Save changed configuration
//-----------------------------------------------------------------------------
void Color::WriteXML
(
TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML(_ccElement);

	char str[32];
	if (m_capabilities != 0)
	{
		snprintf(str, sizeof(str), "%d", m_capabilities);
		_ccElement->SetAttribute("colorchannels", str);
	}
	if (m_coloridxbug)
	{
		_ccElement->SetAttribute("coloridxbug", "true");
	}
}

//-----------------------------------------------------------------------------
// <Color::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Color::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
	{
		Msg* msg = new Msg("ColorCmd_CapabilityGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
		msg->Append(GetNodeId());
		msg->Append(2);
		msg->Append(GetCommandClassId());
		msg->Append(ColorCmd_Capability_Get);
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg(msg, _queue);
		return true;
	}
	if (_requestFlags & RequestFlag_Dynamic)
	{
		/* do a request for each channel. the RequestValue will filter out channels that are not supported
		* by the device
		*/
		/* if the device has the ColorIDX bug, then only request the first channel. We will get the rest later */
		for (int i = 0; i <= 9; i++) {
			bool tmprequests = RequestValue(_requestFlags, i, _instance, _queue);
			requests |= tmprequests;
			if (m_coloridxbug && tmprequests)
				break;
		}
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <Color::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Color::RequestValue
(
	uint32 const _requestFlags,
	uint8 const coloridx,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	/* rework this so we split out the coloridx from value index. This will then enable us to either start a
	* refresh for valueID's based on index, or a call from other methods to get a particular color.
	*/


	if (IsGetSupported())
	{
		/* make sure our coloridx is valid */
		if ((m_capabilities)& (1 << (coloridx))) {
			Msg* msg = new Msg("ColorCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
			msg->SetInstance(this, _instance);
			msg->Append(GetNodeId());
			msg->Append(3);
			msg->Append(GetCommandClassId());
			msg->Append(ColorCmd_Get);
			msg->Append(coloridx);
			msg->Append(GetDriver()->GetTransmitOptions());
			GetDriver()->SendMsg(msg, _queue);
			m_coloridxcount = coloridx;
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// <Color::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Color::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (ColorCmd_Capability_Report == (ColorCmd)_data[0])
	{
		m_capabilities = (_data[1] + (_data[2] << 8));
		Log::Write(LogLevel_Info, GetNodeId(), "Received an Color Capability Report: Capability=%xd", m_capabilities);
		if (m_capabilities & 0x01)
			Log::Write(LogLevel_Info, GetNodeId(), "Warm White (0x00)");
		if (m_capabilities & 0x02)
			Log::Write(LogLevel_Info, GetNodeId(), "Cold White (0x01)");
		if (m_capabilities & 0x04)
			Log::Write(LogLevel_Info, GetNodeId(), "Red (0x02)");
		if (m_capabilities & 0x08)
			Log::Write(LogLevel_Info, GetNodeId(), "Green (0x03)");
		if (m_capabilities & 0x10)
			Log::Write(LogLevel_Info, GetNodeId(), "Blue (0x04)");
		if (m_capabilities & 0x20)
			Log::Write(LogLevel_Info, GetNodeId(), "Amber (0x05)");
		if (m_capabilities & 0x40)
			Log::Write(LogLevel_Info, GetNodeId(), "Cyan (0x06)");
		if (m_capabilities & 0x80)
			Log::Write(LogLevel_Info, GetNodeId(), "Purple (0x07)");
		if (m_capabilities & 0x100)
			Log::Write(LogLevel_Info, GetNodeId(), "Indexed Color (0x08)");

		if (ValueInt* colorchannels = static_cast<ValueInt*>(GetValue(_instance, 254)))
		{
			colorchannels->OnValueRefreshed(m_capabilities);
			colorchannels->Release();
		}
		if (m_capabilities & 0x100) {
			if (Node* node = GetNodeUnsafe())
			{
				node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 2, "Color Index", "", false, false, 0, 0);
			}
		}
		return true;
	}
	if (ColorCmd_Get_Report == (ColorCmd)_data[0])
	{
		/* Fibaro is messed up. It always returns 0x03 as the Coloridx in a report message, but the values are correct for what the request was.
		* I read a forum post about it being a bug. If this is the case, its going to make this class messy as hell :(
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
		if (m_coloridxbug) {
			coloridx = m_coloridxcount;
			for (uint8 idx = m_coloridxcount + 1; idx < 9; idx++) {
				if (RequestValue(0, idx, _instance, Driver::MsgQueue_Send)) {
					m_coloridxcount = idx;
					break;
				}
			}
		}
		m_colorvalues[coloridx] = _data[2];

		/* test if there are any more valid coloridx */
		for (uint8 idx = coloridx + 1; idx < 9; idx++) {
			if ((m_capabilities)& (1 << (idx))) {
				return true;
			}
		}

		/* if we get here, then we can update our ValueID */
		if (ValueString* color = static_cast<ValueString*>(GetValue(_instance, 1)))
		{
			/* create a RGB[W] String */
			std::stringstream ss;
			std::stringstream ssbuf;
			bool usingbuf = false;
			ss << "#";
			/* do R */
			if ((m_capabilities)& (1 << (COLORIDX_RED)))
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_RED];
			else
				ss << "00";
			/* do G */
			if ((m_capabilities)& (1 << (COLORIDX_GREEN)))
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_GREEN];
			else
				ss << "00";
			/* do B */
			if ((m_capabilities)& (1 << (COLORIDX_BLUE)))
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_BLUE];
			else
				ss << "00";

			/* if both whites are present.... */
			if (((m_capabilities)& (1 << (COLORIDX_WARMWHITE)))
				& ((m_capabilities)& (1 << (COLORIDX_COLDWHITE)))) {
				/* append them both */
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_WARMWHITE];
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_COLDWHITE];
			}
			else if ((m_capabilities)& (1 << (COLORIDX_WARMWHITE))) {
				/* else, if the warm white is present, append that */
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_WARMWHITE];
			}
			else if ((m_capabilities)& (1 << (COLORIDX_COLDWHITE))) {
				/* else, if the cold white is present, append that */
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_COLDWHITE];
			}
			else {
				/* we put 0000 into our buffer to represent both Warm and Cold white */
				ssbuf << "0000";
				usingbuf = true;
			}
			if ((m_capabilities)& (1 << (COLORIDX_AMBER))) {
				/* if AMBER is present, append our buffer if needed */
				if (usingbuf) {
					ss << ssbuf.str();
					ssbuf.str("");
					ssbuf.clear();
					usingbuf = false;
				}
				/* and then our Color */
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_AMBER];
			}
			else {
				/* put 00 into our buffer */
				ssbuf << "00";
				usingbuf = true;
			}
			if ((m_capabilities)& (1 << (COLORIDX_CYAN))) {
				/* if CYAN is present, append our buffer if needed */
				if (usingbuf) {
					ss << ssbuf.str();
					ssbuf.str("");
					ssbuf.clear();
					usingbuf = false;
				}
				/* and then our Color */
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_CYAN];
			}
			else {
				/* put 00 into our buffer */
				ssbuf << "00";
				usingbuf = true;
			}
			if ((m_capabilities)& (1 << (COLORIDX_PURPLE))) {
				/* if PURPLE is present, append our buffer if needed */
				if (usingbuf) {
					ss << ssbuf.str();
					ssbuf.str("");
					ssbuf.clear();
					usingbuf = false;
				}
				/* and then our Color */
				ss << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << (int)m_colorvalues[COLORIDX_PURPLE];
			}
			else {
				/* put 00 into our buffer */
				ssbuf << "00";
				usingbuf = true;
			}

			Log::Write(LogLevel_Info, GetNodeId(), "Received a updated Color from Device: %s", ss.str().c_str());
			color->OnValueRefreshed(ss.str());
			color->Release();

		}
		/* if we got a updated Color Index Value - Update our ValueID */
		if ((m_capabilities)& (1 << (COLORIDX_INDEXCOLOR))) {
			if (ValueInt* coloridx = static_cast<ValueInt*>(GetValue(_instance, 2)))
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
// <GetColor>
// Get a Specific Color Value from a position in a RGB String
// its assumed the string is formated such as #RRGGBB[WWCWAMCYPR]
// where the color values in [] are optional.
// throws a exception when position is out of bounds
//-----------------------------------------------------------------------------
uint8 GetColor(string rgbstring, uint8 const position) {

	/* check the length of the string based on position value we passed in including the #*/
	if (rgbstring.length() < (size_t)(position * 2) + 1) {
		Log::Write(LogLevel_Warning, "Request for Color Position %d exceeds String Length: %s", position, rgbstring.c_str());
		throw;
	}
	string result = rgbstring.substr(((position - 1) * 2) + 1, 2);

	std::stringstream ss(result);
	int rawresult;
	ss >> std::hex >> rawresult;
	return (uint8)rawresult;
}

//-----------------------------------------------------------------------------
// <Color::SetValue>
// Set the device's Color value
//-----------------------------------------------------------------------------
bool Color::SetValue
(
	Value const& _value
)
{
	if (ValueID::ValueType_String == _value.GetID().GetType())
	{
		ValueString const* value = static_cast<ValueString const*>(&_value);
		string s = value->GetValue();
		/* make sure the first char is # */
		if (s.at(0) != '#') {
			Log::Write(LogLevel_Warning, GetNodeId(), "Color::SetValue - String is Malformed. Missing #: %s", s.c_str());
			return false;
		}
		/* length minus the # has to be multiple of 2's */
		if ((s.length() - 1) % 2 != 0) {
			Log::Write(LogLevel_Warning, GetNodeId(), "Color::SetValue - Uneven Length string. Each Color should be 2 chars: %s", s.c_str());
			return false;
		}

		/* the size of the string tells us how many colors are set */
		uint8 nocols = (s.length() - 1) / 2;
		uint8 colvals[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		bool colvalset[8] = { false, false, false, false, false, false, false, false };


		try {
			/* we always will have at least RGB */
			colvals[COLORIDX_RED] = GetColor(s, 1);
			colvals[COLORIDX_GREEN] = GetColor(s, 2);
			colvals[COLORIDX_BLUE] = GetColor(s, 3);

			if (nocols == 3) {
				//RGB value, we have to turn off the supported white colors

				if (((m_capabilities)& (1 << (COLORIDX_WARMWHITE)))	& ((m_capabilities)& (1 << (COLORIDX_COLDWHITE)))) {
					colvals[COLORIDX_WARMWHITE] = 0;
					colvals[COLORIDX_COLDWHITE] = 0;
					colvalset[COLORIDX_WARMWHITE] = true;
					colvalset[COLORIDX_COLDWHITE] = true;
					nocols += 2;
				}
				else if ((m_capabilities)& (1 << (COLORIDX_WARMWHITE))) {
					colvals[COLORIDX_WARMWHITE] = 0;
					colvalset[COLORIDX_WARMWHITE] = true;
					nocols += 1;
				}
				else if ((m_capabilities)& (1 << (COLORIDX_COLDWHITE))) {
					colvals[COLORIDX_COLDWHITE] = 0;
					colvalset[COLORIDX_COLDWHITE] = true;
					nocols += 1;
				}
			}
			else
			{
				if (nocols == 4) {
					/* warm white is set */
					/* set cold_white to 0 */
					colvals[COLORIDX_WARMWHITE] = GetColor(s, 4);
					colvalset[COLORIDX_WARMWHITE] = true;
					if ((m_capabilities)& (1 << (COLORIDX_COLDWHITE))) {
						colvals[COLORIDX_COLDWHITE] = 0;
						colvalset[COLORIDX_COLDWHITE] = true;
						nocols += 1;
					}
				}
				if (nocols >= 5) {
					/* warm white and cold white are set */
					colvals[COLORIDX_WARMWHITE] = GetColor(s, 4);
					colvals[COLORIDX_COLDWHITE] = GetColor(s, 5);
					colvalset[COLORIDX_WARMWHITE] = true;
					colvalset[COLORIDX_COLDWHITE] = true;
				}
				if (nocols >= 6) {
					/* amber is also set */
					colvals[COLORIDX_AMBER] = GetColor(s, 6);
					colvalset[COLORIDX_AMBER] = true;
				}
				if (nocols >= 7) {
					/* cyan is also set */
					colvals[COLORIDX_CYAN] = GetColor(s, 7);
					colvalset[COLORIDX_CYAN] = true;
				}
				if (nocols == 8) {
					/* purple is also set */
					colvals[COLORIDX_PURPLE] = GetColor(s, 8);
					colvalset[COLORIDX_PURPLE] = true;
				}
			}
		}
		catch (...) {
			Log::Write(LogLevel_Warning, GetNodeId(), "Color::SetValue - Color String Decoding Failed: %s", s.c_str());
			return false;
		}
		Log::Write(LogLevel_Info, GetNodeId(), "Color::SetValue - Setting Color value");

		Msg* msg = new Msg("Color Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
		msg->SetInstance(this, _value.GetID().GetInstance());
		msg->Append(GetNodeId());
		msg->Append(3 + (nocols * 2)); //length
		msg->Append(GetCommandClassId());
		msg->Append(ColorCmd_Set); //cmd
		msg->Append(nocols); //number of color pairs
		msg->Append(COLORIDX_RED);
		msg->Append(colvals[COLORIDX_RED]);
		msg->Append(COLORIDX_GREEN);
		msg->Append(colvals[COLORIDX_GREEN]);
		msg->Append(COLORIDX_BLUE);
		msg->Append(colvals[COLORIDX_BLUE]);
		if (colvalset[COLORIDX_WARMWHITE] & !colvalset[COLORIDX_COLDWHITE]) {
			msg->Append(COLORIDX_WARMWHITE);
			msg->Append(colvals[COLORIDX_WARMWHITE]);
		}
		if (colvalset[COLORIDX_WARMWHITE] && colvalset[COLORIDX_COLDWHITE]) {
			msg->Append(COLORIDX_WARMWHITE);
			msg->Append(colvals[COLORIDX_WARMWHITE]);
			msg->Append(COLORIDX_COLDWHITE);
			msg->Append(colvals[COLORIDX_COLDWHITE]);
		}
		if (colvalset[COLORIDX_AMBER]) {
			msg->Append(COLORIDX_AMBER);
			msg->Append(colvals[COLORIDX_AMBER]);
		}
		if (colvalset[COLORIDX_CYAN]) {
			msg->Append(COLORIDX_CYAN);
			msg->Append(colvals[COLORIDX_CYAN]);
		}
		if (colvalset[COLORIDX_PURPLE]) {
			msg->Append(COLORIDX_PURPLE);
			msg->Append(colvals[COLORIDX_PURPLE]);
		}
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
		return true;
	}


	return false;
}

//-----------------------------------------------------------------------------
// <Color::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Color::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		//Write only for now
		node->CreateValueInt(ValueID::ValueGenre_System, GetCommandClassId(), _instance, 254, "Color Channels", "", true, false, 0, 0);
		node->CreateValueString(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 1, "Color", "#RRGGBB[WWCWAMCYPR]", false, false, "#000000", 0);
		node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 2, "Color Index", "", false, false, 0, 0);
	}
}

//-----------------------------------------------------------------------------
// <Color::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
void Color::SetValueBasic
(
uint8 const _instance,
uint8 const _value
)
{
	// Send a request for new value to synchronize it with the BASIC set/report.
	for (int i = 0; i <= 8; i++) {
		RequestValue(0, i, _instance, Driver::MsgQueue_Send);
	}
}
