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
	AlarmIndex_Type = 256,
	AlarmIndex_Level,
	AlarmIndex_SourceNodeId,
	AlarmIndex_Count,
	AlarmIndex_Params
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
#if 0
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
		else
		{
			bool res = false;
			for( uint8 i = 0; i < Alarm_Count; i++ )
			{
				if( Value* value = GetValue( _instance, i + AlarmIndex_Count ) ) {
					value->Release();
					Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->SetInstance( this, _instance );
					msg->Append( GetNodeId() );
					msg->Append( GetVersion() == 2 ? 4 : 5);
					msg->Append( GetCommandClassId() );
					msg->Append( AlarmCmd_Get );
					msg->Append( 0x00); // ? proprietary alarm ?
					msg->Append( i );
					if( GetVersion() > 2 )
						msg->Append(0x01); //get first event of type.
					msg->Append( GetDriver()->GetTransmitOptions() );
					GetDriver()->SendMsg( msg, _queue );
					res = true;
				}
			}
			return res;
		}
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "AlarmCmd_Get Not Supported on this node");
	}
#endif
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
#if 0
	if (AlarmCmd_Report == (AlarmCmd)_data[0])
	{
		// We have received a report from the Z-Wave device
		if( GetVersion() == 1 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d", _data[1], _data[2] );

			ValueByte* value;
			if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Type ) )) )
			{
				value->OnValueRefreshed( _data[1] );
				value->Release();
			}
			// For device on version 1 the level could have different value. This level value correspond to a list of alarm type.
			if ( Value* value = GetValue( _instance, AlarmIndex_Level ) )
			{
				switch ( value->GetID().GetType() )
				{
				case ValueID::ValueType_Byte:
				{
					ValueByte* valueByte = static_cast<ValueByte*>( value );
					valueByte->OnValueRefreshed( _data[2] );
					break;
				}
				case ValueID::ValueType_List:
				{
					ValueList* valueList = static_cast<ValueList*>( value );
					valueList->OnValueRefreshed( _data[2] );
					break;
				}
				default:
				{
					Log::Write( LogLevel_Info, GetNodeId(), "Invalid type (%d) for Alarm Level %d", value->GetID().GetType(), _data[2] );
				}
				}
				value->Release();
			}
		}
		/* version 2 */
		else if(( GetVersion() > 1 ) && ( _length >= 7  ))
		{
			// With Version=2, the data has more detailed information about the alarm

			string alarm_type =  ( _data[5] < Alarm_Count ) ? c_alarmTypeName[_data[5]] : "Unknown type";
			string alarm_event = ( c_Alarm_Events[_data[5]][_data[6]].empty() != true ) ? c_Alarm_Events[_data[5]][_data[6]] : "Unknown Event";

			Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: sensorSrcID=%d, type:%s event:%s, status=%s",
					_data[3], alarm_type.c_str(), alarm_event.c_str(), _data[4] == 0x00 ? "false" : "true" );

			{
				ValueByte* value;

				if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_SourceNodeId ) )) )
				{
					value->OnValueRefreshed( _data[3] );
					value->Release();
				}
			}
			{
				ValueList *valuel;

				if( (valuel = static_cast<ValueList*>( GetValue( _instance, _data[5]+AlarmIndex_Count ) )) )
				{
					/* if status is 0, then the alarm is inactive */
					if (_data[4] == 0x00)
						valuel->OnValueRefreshed( 0 );
					else
						valuel->OnValueRefreshed( _data[6] );
					valuel->Release();
				}
			}
			/* if the length byte is greater than 1, then there are params */
			if (_data[7] > 0) {
				/* first, delete our old Temp ValueID's */
				for (multimap<uint8, uint8>::iterator it = m_TempValueIDs.begin(); it != m_TempValueIDs.end(); it++) {
					/* first is index, second is instance*/
					if (it->second == _instance) {
						if( Node* node = GetNodeUnsafe() )
						{
							node->RemoveValue(GetCommandClassId(), _instance, it->first);
							m_TempValueIDs.erase(it);
						}
					}
				}

				/* figure out what type of extra data we have */
				switch (_data[5]) {
				/* these alarm types have params */
				case Alarm_Smoke:
					switch(_data[6]) {
					case Alarm_Smoke_Detected_Location:
						string location = ExtractString(&_data[8], _data[7]);
						if( Node* node = GetNodeUnsafe() )
						{
							node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, (AlarmIndex_Params+m_TempValueIDs.size()), "Location", "", true, false, location, 0 );
							m_TempValueIDs.insert(std::pair<uint8, uint8>(AlarmIndex_Params+m_TempValueIDs.size(), _instance));
						}
						break;
					};
					break;
					case Alarm_CarbonMonoxide:
						switch(_data[6]) {
						case Alarm_CO_Detected_Location:
							string location = ExtractString(&_data[8], _data[7]);
							if( Node* node = GetNodeUnsafe() )
							{
								node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, (AlarmIndex_Params+m_TempValueIDs.size()), "Location", "", true, false, location, 0 );
								m_TempValueIDs.insert(std::pair<uint8, uint8>(AlarmIndex_Params+m_TempValueIDs.size(), _instance));
							}
							break;
						};
						break;
						case Alarm_CarbonDioxide:
							switch(_data[6]) {
							case Alarm_CO2_Detected_Location:
								string location = ExtractString(&_data[8], _data[7]);
								if( Node* node = GetNodeUnsafe() )
								{
									node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, (AlarmIndex_Params+m_TempValueIDs.size()), "Location", "", true, false, location, 0 );
									m_TempValueIDs.insert(std::pair<uint8, uint8>(AlarmIndex_Params+m_TempValueIDs.size(), _instance));
								}
								break;
							};
							break;
							case Alarm_Heat:
								switch(_data[6]) {
								case Alarm_Heat_OverHeat_Location:
								case Alarm_Heat_RapidTempRise_Location:
								case Alarm_Heat_UnderHeat_Location:
									string location = ExtractString(&_data[8], _data[7]);
									if( Node* node = GetNodeUnsafe() )
									{
										node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, (AlarmIndex_Params+m_TempValueIDs.size()), "Location", "", true, false, location, 0 );
										m_TempValueIDs.insert(std::pair<uint8, uint8>(AlarmIndex_Params+m_TempValueIDs.size(), _instance));
									}
									break;
								};
								break;
								case Alarm_Flood:
									switch(_data[6]) {
									case Alarm_Flood_Leak_Location:
									case Alarm_Flood_Leak_UnknownLocation:
									case Alarm_Flood_Drop_Location:
										string location = ExtractString(&_data[8], _data[7]);
										if( Node* node = GetNodeUnsafe() )
										{
											node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, (AlarmIndex_Params+m_TempValueIDs.size()), "Location", "", true, false, location, 0 );
											m_TempValueIDs.insert(std::pair<uint8, uint8>(AlarmIndex_Params+m_TempValueIDs.size(), _instance));
										}
										break;
									};
									break;
									case Alarm_Access_Control:
										switch(_data[6]) {
										case Alarm_Access_Control_Keypad_Lock:
										case Alarm_Access_Control_Keypad_Unlock:
											/* create version 1 ValueID's */
											if( Node* node = GetNodeUnsafe() )
											{
												node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, (AlarmIndex_Params+m_TempValueIDs.size()), "UserCode", "", true, false, _data[8], 0 );
												m_TempValueIDs.insert(std::pair<uint8, uint8>(AlarmIndex_Params+m_TempValueIDs.size(), _instance));
											}
											break;
										};
										break;
										case Alarm_HomeSecurity:
											switch(_data[6]) {
											case Alarm_HomeSecurity_Intrusion_Location:
											case Alarm_HomeSecurity_GlassBreakage_Location:
												string location = ExtractString(&_data[8], _data[7]);
												if( Node* node = GetNodeUnsafe() )
												{
													node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, (AlarmIndex_Params+m_TempValueIDs.size()), "Location", "", true, false, location, 0 );
													m_TempValueIDs.insert(std::pair<uint8, uint8>(AlarmIndex_Params+m_TempValueIDs.size(), _instance));
												}
												break;
											};
											break;
				}


			}


		}

		return true;
	}
#endif
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
			if (const NotificationCCTypes::NotificationEvents *ne = NotificationCCTypes::Get()->GetAlarmNotificationEvents(type, 0)) {
				ValueList::Item item;
				item.m_value = ne->id;
				item.m_label = ne->name;
				_items.push_back( item );
			} else {
				/* it doesn't exist. This is a Config Error in the XML File */
				Log::Write( LogLevel_Warning, GetNodeId(), "Clear Event is Missing from NotificationCCTypes.xml for Alarm Type %s (%d)",NotificationCCTypes::Get()->GetAlarmType(type).c_str(), type);
				ValueList::Item item;
				item.m_value = 0;
				item.m_label = string("Clear");
				_items.push_back( item );
			}

			for( uint32 i=0; i<numBytes; ++i )
			{
				for( int32 bit=0; bit<8; ++bit )
				{
					if( ( _data[i+3] & (1<<bit) ) != 0 )
					{
						uint32 index = (int32)(i<<3) + bit;
						if (const NotificationCCTypes::NotificationEvents *ne = NotificationCCTypes::Get()->GetAlarmNotificationEvents(type, index)) {
							Log::Write( LogLevel_Info, GetNodeId(), "\tEvent Type %d: %s ", ne->id, ne->name.c_str());
							ValueList::Item item;
							item.m_value = ne->id;
							item.m_label = ne->name;
							_items.push_back( item );
						} else {
							Log::Write (LogLevel_Info, GetNodeId(), "\tEvent Type %d: Unknown", index);
							ValueList::Item item;
							item.m_value = index;
							item.m_label = string("Unknown");
							_items.push_back( item );
						}
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
