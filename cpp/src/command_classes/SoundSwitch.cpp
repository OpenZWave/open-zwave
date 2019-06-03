//-----------------------------------------------------------------------------
//
//	SoundSwitch.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SOUND_SWITCH
//
//	Copyright (c) 2014 Justin Hammond <Justin@dynam.ac>
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

#include <time.h>
#include "command_classes/CommandClasses.h"
#include "command_classes/SoundSwitch.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"

using namespace OpenZWave;

enum SoundSwitchCmd
{
	SoundSwitchCmd_Tones_Number_Get		= 0x01,
	SoundSwitchCmd_Tones_Number_Report	= 0x02,
	SoundSwitchCmd_Tones_Info_Get		= 0x03,
	SoundSwitchCmd_Tones_Info_Report	= 0x04,
	SoundSwitchCmd_Tones_Config_Set		= 0x05,
	SoundSwitchCmd_Tones_Config_Get		= 0x06,
	soundSwitchCmd_Tones_Config_Report	= 0x07,
	SoundSwitchCmd_Tones_Play_Set		= 0x08,
	SoundSwitchCmd_Tones_Play_Report	= 0x0A
};

//-----------------------------------------------------------------------------
// <SoundSwitch::SoundSwitch>
// Constructor
//-----------------------------------------------------------------------------
SoundSwitch::SoundSwitch
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId ),
m_toneCount( 0 )
{
	SetStaticRequest( StaticRequest_Values );
}


//-----------------------------------------------------------------------------
// <SoundSwitch::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool SoundSwitch::RequestState
(
		uint32 const _requestFlags,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests |=  RequestValue( _requestFlags, ValueID_Index_SoundSwitch::Tone_Count, _instance, _queue );
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <SoundSwitch::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool SoundSwitch::RequestValue
(
		uint32 const _requestFlags,
		uint16 const _index,	// = 0 (not used)
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if (_index == ValueID_Index_SoundSwitch::Tone_Count ) {
		if ( m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED) )
		{
			Msg* msg = new Msg( "SoundSwitchCmd_Tones_Number_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( SoundSwitchCmd_Tones_Number_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		} else {
			Log::Write(  LogLevel_Info, GetNodeId(), "SoundSwitchCmd_Tones_Number_Get Not Supported on this node");
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SoundSwitch::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SoundSwitch::HandleMsg
(
		uint8 const* _data,
		uint32 const _length,
		uint32 const _instance	// = 1
)
{
	if (SoundSwitchCmd_Tones_Number_Report == (SoundSwitchCmd)_data[0])
	{
		m_toneCount = _data[1];
		Log::Write( LogLevel_Info, GetNodeId(), "Received SoundSwitch Tone Count report: %d", m_toneCount);
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, ValueID_Index_SoundSwitch::Tone_Count ) ) )
		{
			value->OnValueRefreshed( m_toneCount );
			value->Release();
		}
		for (int i = 1; i <= m_toneCount; i++) {
			Msg* msg = new Msg( "SoundSwitchCmd_Tones_Info_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( SoundSwitchCmd_Tones_Info_Get );
			msg->Append( i );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
		return true;
	}
	if (SoundSwitchCmd_Tones_Info_Report == (SoundSwitchCmd)_data[0])
	{
		uint8 index = _data[1];
		uint16 duration = (_data[2] << 8) + _data[3];
		string name((const char *)&_data[5], _data[4]);
		m_toneInfo[index].duration = duration;
		m_toneInfo[index].name = name;
		Log::Write( LogLevel_Info, GetNodeId(), "Received SoundSwitch Tone Info Report: %d - %s - %d sec", index, name.c_str(), duration);
		if (index == m_toneCount) {
			vector<ValueList::Item> items;
			{
				ValueList::Item item;
				item.m_label = "Inactive";
				item.m_value = 0;
				items.push_back( item );
			}
			for( unsigned int i=1; i <= m_toneCount; i++)
			{
				ValueList::Item item;
				char str[32];
				snprintf( str, sizeof(str), "%s (%d sec)", m_toneInfo[i].name.c_str(), m_toneInfo[i].duration );
				item.m_label = str;
				item.m_value = i;
				items.push_back( item );
			}
			{
				ValueList::Item item;
				item.m_label = "Default Tone";
				item.m_value = 0xff;
				items.push_back( item );
			}
			if (Node* node = GetNodeUnsafe()) {
				node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SoundSwitch::Tones, "Tones", "", false, false, m_toneCount, items, 0, 0 );
				node->CreateValueList( ValueID::ValueGenre_Config, GetCommandClassId(), _instance, ValueID_Index_SoundSwitch::Default_Tone, "Default Tone", "", false, false, m_toneCount, items, 0, 0 );
			}
			/* after we got the list of Tones, Get the Configuration */
			Msg* msg = new Msg( "SoundSwitchCmd_Tones_Config_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( SoundSwitchCmd_Tones_Config_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
		return true;
	}
	if (soundSwitchCmd_Tones_Config_Report == (SoundSwitchCmd)_data[0]) {
		uint8 volume = _data[1];
		uint8 defaulttone = _data[2];
		if (volume > 100)
			volume = 100;
		Log::Write( LogLevel_Info, GetNodeId(), "Received SoundSwitch Tone Config report - Volume: %d, defaulttone: %d", volume, defaulttone);
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, ValueID_Index_SoundSwitch::Volume ) ) )
		{
			value->OnValueRefreshed( volume );
			value->Release();
		}
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, ValueID_Index_SoundSwitch::Default_Tone ) ) )
		{
			value->OnValueRefreshed( defaulttone );
			value->Release();
		}
		ClearStaticRequest( StaticRequest_Values );
		return true;
	}
	if (SoundSwitchCmd_Tones_Play_Report == (SoundSwitchCmd)_data[0]) {
		Log::Write( LogLevel_Info, GetNodeId(), "Received SoundSwitch Tone Play report: %d", _data[1]);
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, ValueID_Index_SoundSwitch::Tones ) ) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SoundSwitch::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
bool SoundSwitch::SetValue
(
		Value const& _value
)
{
	bool ret = false;
	uint8 instance = _value.GetID().GetInstance();
	uint16 index = _value.GetID().GetIndex();
	if (index == ValueID_Index_SoundSwitch::Tones) {
		if (ValueList const* value = static_cast<ValueList const*>(&_value)) {
			ValueList::Item const *item = value->GetItem();
			if (item == NULL)
				return false;
			Msg* msg = new Msg( "SoundSwitchCmd_Tones_Play_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, instance );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( SoundSwitchCmd_Tones_Play_Set );
			msg->Append( item->m_value );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
		ret = true;
	}
	if (index == ValueID_Index_SoundSwitch::Volume || index == ValueID_Index_SoundSwitch::Default_Tone) {
		uint8 volume = 0xff;
		uint8 defaulttone = 0x01;
		if( ValueByte const* value = static_cast<ValueByte const*>( GetValue( instance, ValueID_Index_SoundSwitch::Volume ) ) )
		{
			volume = value->GetValue();
		}
		if (ValueList const* value = static_cast<ValueList const*>( GetValue( instance, ValueID_Index_SoundSwitch::Default_Tone ) ) )
		{
			ValueList::Item const *item = value->GetItem();
			if (item == NULL)
				return false;
			defaulttone = item->m_value;
			/* 0 means dont update the Default Tone 0xFF is the Default tone! */
			if (defaulttone == 0xFF)
				defaulttone = 1;
		}
		Msg* msg = new Msg( "SoundSwitchCmd_Tones_Config_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, instance );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( SoundSwitchCmd_Tones_Config_Set );
		msg->Append( volume );
		msg->Append( defaulttone );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		ret = true;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// <SoundSwitch::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SoundSwitch::CreateVars
(
		uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_SoundSwitch::Tone_Count, "Number of Tones", "", true, false, 0, 0 );
		node->CreateValueByte( ValueID::ValueGenre_Config, GetCommandClassId(), _instance, ValueID_Index_SoundSwitch::Volume, "Volume", "", false, false, 100, 0);
	}
}
