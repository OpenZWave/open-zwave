//-----------------------------------------------------------------------------
//
//	Alarm.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_NOTIFICATIOn (formally COMMAND_CLASS_ALARM)
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

// This CommandClass was renamed from ALARM to NOTIFICATION in version 3
// But we cannot rename the class names as we already have a Notification Class used
// for signaling events to the application.



#include "command_classes/CommandClasses.h"
#include "command_classes/Alarm.h"
#include "command_classes/NodeNaming.h"
#include "command_classes/UserCode.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "NotificationCCTypes.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"
#include "value_classes/ValueBool.h"
#include "value_classes/ValueList.h"
#include "value_classes/ValueString.h"
#include "value_classes/ValueInt.h"
using namespace OpenZWave;

enum AlarmCmd
{
	AlarmCmd_Get					= 0x04,
	AlarmCmd_Report					= 0x05,
	AlarmCmd_Set					= 0x06,
	// Version 2
	AlarmCmd_SupportedGet			= 0x07,
	AlarmCmd_SupportedReport		= 0x08,
	// Version 3
	AlarmCmd_Event_Supported_Get 	= 0x01,
	AlarmCmd_Event_Supported_Report = 0x02
};

enum
{
	AlarmIndex_Type_ParamUserCodeid = 256,
	AlarmIndex_Type_ParamUserCodeEntered,
	AlarmIndex_Type_ParamLocation,
	AlarmIndex_Type_ParamList,
	AlarmIndex_Type_ParamByte,
	AlarmIndex_Type_ParamString,
	AlarmIndex_Type_Duration,
	AlarmIndex_Type = 512,
	AlarmIndex_Level
};




//-----------------------------------------------------------------------------
// <WakeUp::WakeUp>
// Constructor
//-----------------------------------------------------------------------------
Alarm::Alarm
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId ),
m_v1Params(false)
{
	SetStaticRequest( StaticRequest_Values );
}


//-----------------------------------------------------------------------------
// <Alarm::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Alarm::RequestState
(
		uint32 const _requestFlags,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		if( GetVersion() > 1 )
		{
			// Request the supported alarm types
			Msg* msg = new Msg( "AlarmCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( AlarmCmd_SupportedGet );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		}
		else
		{
			/* create version 1 ValueID's */
			if( Node* node = GetNodeUnsafe() )
			{
				m_v1Params = true;
				node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type, "Alarm Type", "", true, false, 0, 0 );
				node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Level, "Alarm Level", "", true, false, 0, 0 );
			}
		}

	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Alarm::RequestValue
(
		uint32 const _requestFlags,
		uint16 const _dummy1,	// = 0 (not used)
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if( IsGetSupported() )
	{
		if( GetVersion() == 1 )
		{
			Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( AlarmCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		}
		else if (GetVersion() >= 3)
		{
			bool res = false;
			Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( GetVersion() == 2 ? 4 : 5);
			msg->Append( GetCommandClassId() );
			msg->Append( AlarmCmd_Get );
			msg->Append( 0x00 ); /* we don't get Version 1/2 Alarm Types  */
			msg->Append( 0xFF );
			if( GetVersion() > 2 )
				msg->Append(0x00); //get first event of type.
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			res = true;
			return res;
		}
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "AlarmCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Alarm::HandleMsg
(
		uint8 const* _data,
		uint32 const _length,
		uint32 const _instance	// = 1
)
{
	if (AlarmCmd_Report == (AlarmCmd)_data[0])
	{
		// We have received a report from the Z-Wave device
		if( GetVersion() == 1)
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d", _data[1], _data[2] );

			if( ValueByte *value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Type ) ) )
			{
				value->OnValueRefreshed( _data[1] );
				value->Release();
			}
			// For device on version 1 the level could have different value. This level value correspond to a list of alarm type.
			if ( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Level ) ) )
			{
				value->OnValueRefreshed( _data[2] );
				value->Release();
			}
		}
		/* version 2 */
		else if(( GetVersion() > 1 ) && ( _length >= 7  ))
		{
			// With Version=2, the data has more detailed information about the alarm
			if (m_v1Params) {

				Log::Write( LogLevel_Info, GetNodeId(), "Received Notification report (v1): type:%d event:%d",
						_data[1], _data[2] );

				if( ValueByte *value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Type ) ) )
				{
					value->OnValueRefreshed( _data[1] );
					value->Release();
				}
				// For device on version 1 the level could have different value. This level value correspond to a list of alarm type.
				if ( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Level ) ) )
				{
					value->OnValueRefreshed( _data[2] );
					value->Release();
				}
			}
			/* ok. Its a AlarmCC/NotificationCC Version 2 or above. */
			bool NotificationStatus = false;
			if (_data[4] == 0xFF)
				NotificationStatus = true;

			uint8 NotificationType = _data[5];
			uint8 NotificationEvent = _data[6];
			bool NotificationSequencePresent = (_data[7] & 0x80);
			uint8 EventParamLength = (_data[7] & 0x1F);
			uint8 NotificationSequence = 0;
			if (NotificationSequencePresent) {
				NotificationSequence = _data[7+EventParamLength];
			}
			Log::Write( LogLevel_Info, GetNodeId(), "Received Notification report (>v1): Type: %s (%d) Event: %s (%d) Status: %s, Param Length: %d", NotificationCCTypes::Get()->GetAlarmType(NotificationType).c_str(), NotificationType, NotificationCCTypes::Get()->GetEventForAlarmType(NotificationType, NotificationEvent).c_str(), NotificationEvent, NotificationStatus ? "true" : "false", EventParamLength);
			if (NotificationSequencePresent)
				Log::Write ( LogLevel_Info, GetNodeId(), "\t Sequence Number: %d", NotificationSequence);

			/* update the actual value */
			if ( ValueList *value = static_cast<ValueList *>( GetValue( _instance, NotificationType ) ) )
			{
				value->OnValueRefreshed(NotificationEvent);
				value->Release();
			} else {
				Log::Write ( LogLevel_Warning, GetNodeId(), "Couldn't Find a ValueList for Notification Type %d (%d)", NotificationType, _instance);
			}
			/* Reset Any of the Params that may have been previously set with another event */
			for (std::vector<uint32>::iterator it = m_ParamsSet.begin(); it != m_ParamsSet.end(); it++)
			{
				switch (*it) {
				case AlarmIndex_Type_ParamUserCodeid: {
					if (ValueByte *value = static_cast<ValueByte *>(GetValue(_instance, AlarmIndex_Type_ParamUserCodeid)))
					{
						value->OnValueRefreshed(0);
						value->Release();
					}
				}
				break;
				case AlarmIndex_Type_ParamUserCodeEntered: {
					if (ValueString *value = static_cast<ValueString *>(GetValue(_instance, AlarmIndex_Type_ParamUserCodeEntered)))
					{
						value->OnValueRefreshed("");
						value->Release();
					}
				}
				break;
				case AlarmIndex_Type_ParamLocation: {
					if (ValueString *value = static_cast<ValueString *>(GetValue(_instance, AlarmIndex_Type_ParamLocation)))
					{
						value->OnValueRefreshed("");
						value->Release();
					}
				}
				break;
				case AlarmIndex_Type_ParamList: {
					if (ValueList *value = static_cast<ValueList *>(GetValue(_instance, AlarmIndex_Type_ParamList)))
					{
						value->OnValueRefreshed(0);
						value->Release();
					}
				}
				break;
				case AlarmIndex_Type_ParamByte: {
					if (ValueByte *value = static_cast<ValueByte *>(GetValue(_instance, AlarmIndex_Type_ParamByte)))
					{
						value->OnValueRefreshed(0);
						value->Release();
					}
				}
				break;
				case AlarmIndex_Type_ParamString: {
					if (ValueString *value = static_cast<ValueString *>(GetValue(_instance, AlarmIndex_Type_ParamString)))
					{
						value->OnValueRefreshed("");
						value->Release();
					}
				}
				break;
				case AlarmIndex_Type_Duration: {
					if (ValueInt *value = static_cast<ValueInt *>(GetValue(_instance, AlarmIndex_Type_Duration)))
					{
						value->OnValueRefreshed(0);
						value->Release();
					}
				}
				break;

				}
			}

			m_ParamsSet.clear();
			/* do any Event Params that are sent over */
			if (EventParamLength > 0) {
				const std::map<uint32, NotificationCCTypes::NotificationEventParams* > nep = NotificationCCTypes::Get()->GetAlarmNotificationEventParams(NotificationType, NotificationEvent);
				if (nep.size() > 0) {
					if( Node* node = GetNodeUnsafe() ) {
						for (std::map<uint32, NotificationCCTypes::NotificationEventParams* >::const_iterator it = nep.begin(); it != nep.end(); it++) {
							switch (it->second->type) {
							case NotificationCCTypes::NEPT_Location: {
								/* _data[8] should be COMMAND_CLASS_NODE_NAMING
								 * _data[9] should be NodeNamingCmd_Report (0x03)
								 */
								if ((_data[8] == NodeNaming::StaticGetCommandClassId()) && (_data[9] == 0x03) && EventParamLength > 2) {
									if (ValueString *value = static_cast<ValueString *>(GetValue(_instance, AlarmIndex_Type_ParamLocation)))
									{
										value->OnValueRefreshed(ExtractString(&_data[10], EventParamLength-2));
										value->Release();
										m_ParamsSet.push_back(AlarmIndex_Type_ParamLocation);
									} else {
										Log::Write( LogLevel_Warning, GetNodeId(), "Couldn't Find AlarmIndex_Type_ParamLocation");
									}
								} else {
									Log::Write( LogLevel_Warning, GetNodeId(), "Location Param didn't have correct Header, or was too small");
								}
								break;
							}
							case NotificationCCTypes::NEPT_List: {
								if (EventParamLength == 1) {
									if (ValueList *value = static_cast<ValueList *>(GetValue(_instance, AlarmIndex_Type_ParamList)))
									{
										value->OnValueRefreshed(_data[8]);
										value->Release();
										m_ParamsSet.push_back(AlarmIndex_Type_ParamList);
									} else {
										Log::Write( LogLevel_Warning, GetNodeId(), "Couldn't Find AlarmIndex_Type_ParamList");
									}
								} else {
									Log::Write( LogLevel_Warning, GetNodeId(), "List Param size was not equal to 1");
								}
								break;
							}
							case NotificationCCTypes::NEPT_UserCodeReport: {
								/* _data[8] should be COMMAND_CLASS_USER_CODE
								 * _data[9] should be UserCodeCmd_Report (0x03)
								 * _data[10] is the UserID
								 * _data[11] is the UserID Status (Ignored)
								 * _data[12] onwards is the UserCode Entered (minimum 4 Bytes)
								 */
								if ((EventParamLength >= 8 ) && (_data[8] == UserCode::StaticGetCommandClassId()) && (_data[9] == 0x03)) {
									if (ValueByte *value = static_cast<ValueByte *>(GetValue(_instance, AlarmIndex_Type_ParamUserCodeid)))
									{
										value->OnValueRefreshed(_data[11]);
										value->Release();
										m_ParamsSet.push_back(AlarmIndex_Type_ParamUserCodeid);
									} else {
										Log::Write( LogLevel_Warning, GetNodeId(), "Couldn't Find AlarmIndex_Type_ParamUserCodeid");
									}
									if (ValueString *value = static_cast<ValueString *>(GetValue(_instance, AlarmIndex_Type_ParamUserCodeEntered)))
									{
										value->OnValueRefreshed(ExtractString(&_data[12], EventParamLength-4));
										value->Release();
										m_ParamsSet.push_back(AlarmIndex_Type_ParamUserCodeEntered);
									} else {
										Log::Write( LogLevel_Warning, GetNodeId(), "Couldn't Find AlarmIndex_Type_ParamUserCodeEntered");
									}
								} else {
									Log::Write( LogLevel_Warning, GetNodeId(), "UserCode Param didn't have correct Header, or was too small");
								}
								break;
							}
							case NotificationCCTypes::NEPT_Byte: {
								if (EventParamLength == 1) {
									if (ValueByte *value = static_cast<ValueByte *>(GetValue(_instance, AlarmIndex_Type_ParamByte)))
									{
										value->OnValueRefreshed(_data[8]);
										value->Release();
										m_ParamsSet.push_back(AlarmIndex_Type_ParamByte);
									} else {
										Log::Write( LogLevel_Warning, GetNodeId(), "Couldn't Find AlarmIndex_Type_ParamByte");
									}
								} else {
									Log::Write( LogLevel_Warning, GetNodeId(), "Byte Param size was not equal to 1");
								}
								break;
							}
							case NotificationCCTypes::NEPT_String: {
								if (ValueString *value = static_cast<ValueString *>(GetValue(_instance, AlarmIndex_Type_ParamString)))
								{
									value->OnValueRefreshed(ExtractString(&_data[10], EventParamLength-2));
									value->Release();
									m_ParamsSet.push_back(AlarmIndex_Type_ParamString);
								} else {
									Log::Write( LogLevel_Warning, GetNodeId(), "Couldn't Find AlarmIndex_Type_ParamString");
								}
								break;
							}
							case NotificationCCTypes::NEPT_Time: {
								/* This is a Duration Entry, we will expose as seconds. Its 3 Bytes from the Event */
								if (EventParamLength == 3) {
									uint32 duration = (_data[10] * 3600) + (_data[11] * 60) + (_data[12]);
									if (ValueInt *value = static_cast<ValueInt *>(GetValue(_instance, AlarmIndex_Type_Duration)))
									{
										value->OnValueRefreshed(duration);
										value->Release();
										m_ParamsSet.push_back(AlarmIndex_Type_Duration);
									} else {
										Log::Write( LogLevel_Warning, GetNodeId(), "Couldn't Find AlarmIndex_Type_Duration");
									}
								} else {
									Log::Write( LogLevel_Warning, GetNodeId(), "Duration Param size was not equal to 3");
								}
								node->CreateValueInt( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_Duration, it->second->name, "", true, false, 0, 0);
								break;
							}

							}
						}
					}
				}
			}
		}
		return true;
	}

	if( AlarmCmd_SupportedReport == (AlarmCmd)_data[0] )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			// We have received the supported alarm types from the Z-Wave device
			Log::Write( LogLevel_Info, GetNodeId(), "Received supported alarm types" );
			/* Device Only supports Version 1 of the Alarm CC */
			if ((GetVersion() > 2) && (_data[1] & 0x80)) {
				m_v1Params = true;
				Log::Write( LogLevel_Info, GetNodeId(), "Notification::SupportedReport - Device Supports Alarm Version 1 Parameters");
				node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type, "Alarm Type", "", true, false, 0, 0 );
				node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Level, "Alarm Level", "", true, false, 0, 0 );
			}
			// Parse the data for the supported alarm types
			uint8 numBytes = (_data[1] & 0x1F);
			for( uint32 i=0; i<numBytes; ++i )
			{
				for( int32 bit=0; bit<8; ++bit )
				{
					if( ( _data[i+2] & (1<<bit) ) != 0 )
					{
						int32 index = (int32)(i<<3) + bit;
						Log::Write( LogLevel_Info, GetNodeId(), "\tAlarmType: %s", NotificationCCTypes::Get()->GetAlarmType(index).c_str());
						if (GetVersion() == 2) {
							/* EventSupported is only compatible in Version 3 and above */
							vector<ValueList::Item> _items;
							if (const NotificationCCTypes::NotificationTypes *nt = NotificationCCTypes::Get()->GetAlarmNotificationTypes(index)) {
								for (std::map<uint32, NotificationCCTypes::NotificationEvents *>::const_iterator it = nt->Events.begin(); it != nt->Events.end(); it++) {
									/* Create it */
									Log::Write ( LogLevel_Info, GetNodeId(), "\t\tAll Events - Alarm CC Version 2");
									ValueList::Item item;
									item.m_value = it->first;
									item.m_label = it->second->name;
									_items.push_back( item );
								}
								node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, index, NotificationCCTypes::Get()->GetAlarmType(index), "", false, false, _items.size(), _items, 0, 0 );
							}
							ClearStaticRequest( StaticRequest_Values );
						}
						else if (GetVersion() > 2)
						{
							/* These Devices have EVENT_SUPPORTED command */
							Msg* msg = new Msg( "AlarmCmd_Event_Supported_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
							msg->SetInstance( this, _instance );
							msg->Append( GetNodeId() );
							msg->Append( 3 );
							msg->Append( GetCommandClassId() );
							msg->Append( AlarmCmd_Event_Supported_Get );
							msg->Append( index);
							msg->Append( GetDriver()->GetTransmitOptions() );
							GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
						}
					}
				}
			}
		}
		return true;
	}
	if( AlarmCmd_Event_Supported_Report == (AlarmCmd)_data[0] )
	{
		//			if( Node* node = GetNodeUnsafe() )
		{
			uint32 type = _data[1];
			// We have received the supported alarm Event types from the Z-Wave device
			Log::Write( LogLevel_Info, GetNodeId(), "Received supported alarm Event types for AlarmType %s (%d)", NotificationCCTypes::Get()->GetAlarmType(type).c_str(), type);
			// Parse the data for the supported Alarm Event types
			uint8 numBytes = (_data[2] & 0x1F);
			vector<ValueList::Item> _items;
			/* always Add the Clear Event Type */
			SetupEvents(type, 0, &_items, _instance);

			for( uint32 i=0; i<numBytes; ++i )
			{
				for( int32 bit=0; bit<8; ++bit )
				{
					if( ( _data[i+3] & (1<<bit) ) != 0 )
					{
						uint32 index = (int32)(i<<3) + bit;
						SetupEvents(type, index, &_items, _instance);
					}
				}
			}
			if( Node* node = GetNodeUnsafe() )
			{
				node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, type, NotificationCCTypes::Get()->GetAlarmType(type), "", false, false, _items.size(), _items, 0, 0 );
			}
		}
		ClearStaticRequest( StaticRequest_Values );
		return true;
	}

	return false;
}
void Alarm::SetupEvents
(
		uint32 type,
		uint32 index,
		vector<ValueList::Item> *_items,
		uint32 const _instance
)
{
	if (const NotificationCCTypes::NotificationEvents *ne = NotificationCCTypes::Get()->GetAlarmNotificationEvents(type, index)) {
		Log::Write( LogLevel_Info, GetNodeId(), "\tEvent Type %d: %s ", ne->id, ne->name.c_str());
		ValueList::Item item;
		item.m_value = ne->id;
		item.m_label = ne->name;
		_items->push_back( item );
		/* If there are Params - Lets create the correct types now */
		if ( Node* node = GetNodeUnsafe() ) {
			for (std::map<uint32, NotificationCCTypes::NotificationEventParams* >::const_iterator it = ne->EventParams.begin(); it != ne->EventParams.end(); it++ ) {
				switch (it->second->type) {
				case NotificationCCTypes::NEPT_Location: {
					node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_ParamLocation, it->second->name, "", true, false, "", 0);
					break;
				}
				case NotificationCCTypes::NEPT_List: {
					vector<ValueList::Item> _Paramitems;
					for (std::map<uint32, string>::iterator it2 = it->second->ListItems.begin(); it2 != it->second->ListItems.end(); it++) {
						ValueList::Item Paramitem;
						Paramitem.m_value = ne->id;
						Paramitem.m_label = ne->name;
						_Paramitems.push_back( Paramitem );
					}
					node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_ParamList, it->second->name, "", false, false, _Paramitems.size(), _Paramitems, 0, 0 );
					break;
				}
				case NotificationCCTypes::NEPT_UserCodeReport: {
					node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_ParamUserCodeid, it->second->name, "", true, false, 0, 0);
					node->CreateValueString(ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_ParamUserCodeEntered, it->second->name, "", true, false, "", 0);
					break;
				}
				case NotificationCCTypes::NEPT_Byte: {
					node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_ParamByte, it->second->name, "", true, false, 0, 0);
					break;
				}
				case NotificationCCTypes::NEPT_String: {
					node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_ParamString, it->second->name, "", true, false, "", 0);
					break;
				}
				case NotificationCCTypes::NEPT_Time: {
					node->CreateValueInt( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type_Duration, it->second->name, "", true, false, 0, 0);
					break;
				}
				}
			}
		}
	} else {
		Log::Write (LogLevel_Info, GetNodeId(), "\tEvent Type %d: Unknown", index);
		ValueList::Item item;
		item.m_value = index;
		item.m_label = string("Unknown");
		_items->push_back( item );
	}
}
