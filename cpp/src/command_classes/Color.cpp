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

#include "value_classes/ValueByte.h"
#include "value_classes/ValueRaw.h"

#include "tinyxml.h"

using namespace OpenZWave;

enum ColorCmd
{
	ColorCmd_Capability_Get = 0x01,
	ColorCmd_Capability_Report = 0x02,
	ColorCmd_Get = 0x03,
	ColorCmd_Report = 0x04,
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


Color::Color
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId )
{
	SetStaticRequest( StaticRequest_Values );
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
	CommandClass::ReadXML( _ccElement );

	int32 intVal;
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "colorchannels", &intVal ) )
	{
		if (intVal <= 0x100) {
			m_capabilities = (uint16)intVal;
		}
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
	CommandClass::WriteXML( _ccElement );

	char str[32];
	if( m_capabilities != 0 )
	{
		snprintf( str, sizeof(str), "%d", m_capabilities );
		_ccElement->SetAttribute( "colorchannels", str );
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
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
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
		for (int i = 0; i <= 8; i++) {
			requests |= RequestValue( _requestFlags, i, _instance, _queue );
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
	if( IsGetSupported() )
	{
		/* make sure our coloridx is valid */
		if ((m_capabilities) & (1<<(coloridx))) {
			Msg* msg = new Msg("ColorCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( ColorCmd_Get );
			msg->Append( coloridx );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
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
		return true;
	}
	if (ColorCmd_Report == (ColorCmd)_data[0])
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
	}
	return false;
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
	if (ValueID::ValueType_Raw == _value.GetID().GetType())
	{
		ValueRaw const* value = static_cast<ValueRaw const*>(&_value);
		uint8* s = value->GetValue();
		uint8 len = value->GetLength();
		if (len > 10) //sanity check
			return false;

		Log::Write( LogLevel_Info, GetNodeId(), "Color::SetValue - Setting Color value");
		Msg* msg = new Msg( "Color Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append(3 + len); //length
		msg->Append( GetCommandClassId() );
		msg->Append(ColorCmd_Set); //cmd
		msg->Append(len); //LEN OF DATA
		for (int ii = 0; ii < len; ii++)
		{
			msg->Append(s[ii]);
		}
		msg->Append(GetDriver()->GetTransmitOptions());
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
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
		RequestValue( 0, i, _instance, Driver::MsgQueue_Send );
	}
}
