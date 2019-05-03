//-----------------------------------------------------------------------------
//
//	NotificationCCTypes.cpp
//
//	NotificationCCTypes for Notification Command Class
//
//	Copyright (c) 2018 Justin Hammond <justin@dynam.ac>
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
#include "NotificationCCTypes.h"

#include <string.h>

#include "tinyxml.h"
#include "Options.h"
#include "platform/Log.h"

using namespace OpenZWave;

NotificationCCTypes *NotificationCCTypes::m_instance = NULL;
std::map<uint32, NotificationCCTypes::NotificationTypes *> NotificationCCTypes::Notifications;
uint32 NotificationCCTypes::m_revision(0);

NotificationCCTypes::NotificationCCTypes()
{
}

void NotificationCCTypes::ReadXML
(
)
{
	// Parse the Z-Wave manufacturer and product XML file.
	string configPath;
	Options::Get()->GetOptionAsString( "ConfigPath", &configPath );

	string path = configPath + "NotificationCCTypes.xml";
	TiXmlDocument* pDoc = new TiXmlDocument();
	if( !pDoc->LoadFile( path.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		delete pDoc;
		Log::Write( LogLevel_Warning, "Unable to load NotificationCCTypes file %s", path.c_str());
		return;
	}
	pDoc->SetUserData((void*)path.c_str());
	Log::Write( LogLevel_Info, "Loading NotificationCCTypes File %s", path.c_str() );

	TiXmlElement const* root = pDoc->RootElement();
	char const *str = root->Value();
	if( str && !strcmp( str, "NotificationTypes" ) )
	{
		// Read in the revision attributes
		str = root->Attribute( "Revision" );
		if( !str )
		{
			Log::Write( LogLevel_Info, "Error in Product Config file at line %d - missing Revision  attribute", root->Row() );
			delete pDoc;
			return;
		}
		m_revision = atol(str);
	}
	TiXmlElement const* AlarmTypeElement = root->FirstChildElement();
	while( AlarmTypeElement )
	{
		char const* str = AlarmTypeElement->Value();
		char* pStopChar;
		if( str && !strcmp( str, "AlarmType" ) )
		{
			NotificationTypes *nt = new NotificationTypes;

			str = AlarmTypeElement->Attribute( "id" );
			if( !str )
			{
				Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing AlarmType ID attribute", AlarmTypeElement->GetDocument()->GetUserData(), AlarmTypeElement->Row() );
				AlarmTypeElement = AlarmTypeElement->NextSiblingElement();
				delete nt;
				continue;
			}
			nt->id = (uint32)strtol( str, &pStopChar, 10 );
			str = AlarmTypeElement->Attribute( "name" );
			if ( !str )
			{
				Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing AlarmType name attribute", AlarmTypeElement->GetDocument()->GetUserData(), AlarmTypeElement->Row() );
				AlarmTypeElement = AlarmTypeElement->NextSiblingElement();
				delete nt;
				continue;
			}
			nt->name = str;
			TiXmlElement const* AlarmEventElement = AlarmTypeElement->FirstChildElement();
			while (AlarmEventElement) {
				str = AlarmEventElement->Value();
				if (str && !strcmp( str, "AlarmEvent" ) )
				{
					NotificationEvents *ne = new NotificationEvents;
					str = AlarmEventElement->Attribute( "id" );
					if ( !str )
					{
						Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing AlarmEventParam id attribute", AlarmEventElement->GetDocument()->GetUserData(), AlarmEventElement->Row() );
						AlarmEventElement = AlarmEventElement->NextSiblingElement();
						delete ne;
						continue;
					}

					ne->id = (uint32)strtol( str, &pStopChar, 10 );

					str = AlarmEventElement->Attribute( "name" );
					if ( !str )
					{
						Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing AlarmEventParam name attribute", AlarmEventElement->GetDocument()->GetUserData(), AlarmEventElement->Row() );
						AlarmEventElement = AlarmEventElement->NextSiblingElement();
						delete ne;
						continue;
					}
					ne->name = str;

					TiXmlElement const* nextElement = AlarmEventElement->FirstChildElement();
					while (nextElement) {
						str = nextElement->Value();
						if (str && !strcmp( str, "AlarmEventParam" ) )
						{
							NotificationEventParams *aep = new NotificationEventParams;
							str = nextElement->Attribute( "id" );
							if ( !str )
							{
								Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing AlarmEventParam id attribute", nextElement->GetDocument()->GetUserData(), nextElement->Row() );
								nextElement = nextElement->NextSiblingElement();
								delete aep;
								continue;
							}
							aep->id = (uint32)strtol( str, &pStopChar, 10 );

							str = nextElement->Attribute( "type" );
							if ( !str )
							{
								Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing AlarmEventParam id attribute", nextElement->GetDocument()->GetUserData(), nextElement->Row() );
								nextElement = nextElement->NextSiblingElement();
								delete aep;
								continue;
							}

							if (!strcasecmp(str, "location")) {
								aep->type = NotificationCCTypes::NEPT_Location;
							} else if (!strcasecmp(str, "list")) {
								aep->type = NotificationCCTypes::NEPT_List;
								TiXmlElement const* listElement = nextElement->FirstChildElement();
								while (listElement) {
									str = listElement->Value();
									if (str && !strcmp( str, "Item" ) )
									{
										str = listElement->Attribute( "id" );
										if ( !str )
										{
											Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing Item id attribute", listElement->GetDocument()->GetUserData(), nextElement->Row() );
											listElement = listElement->NextSiblingElement();
											continue;
										}
										uint32 listID = (uint32)strtol( str, &pStopChar, 10 );
										str = listElement->Attribute( "label" );
										if ( !str )
										{
											Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing Item name attribute", listElement->GetDocument()->GetUserData(), nextElement->Row() );
											listElement = listElement->NextSiblingElement();
											continue;
										}
										if (aep->ListItems.find(listID) == aep->ListItems.end()) {
											aep->ListItems.insert(std::pair<int32, string>(listID, str));
										} else {
											Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s - A AlarmEventElement with id %d already exists. Skipping ", listElement->GetDocument()->GetUserData(), ne->id);
										}
									}
									listElement = listElement->NextSiblingElement();
								}
							} else if (!strcasecmp(str, "usercodereport")) {
								aep->type = NotificationCCTypes::NEPT_UserCodeReport;
							} else if (!strcasecmp(str, "byte")) {
								aep->type = NotificationCCTypes::NEPT_Byte;
							} else if (!strcasecmp(str, "string")) {
								aep->type = NotificationCCTypes::NEPT_String;
							} else if (!strcasecmp(str, "duration")) {
								aep->type = NotificationCCTypes::NEPT_Time;
							} else {
								Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - unknown AlarmEventParam type attribute (%s)", nextElement->GetDocument()->GetUserData(), nextElement->Row(), str );
								nextElement = nextElement->NextSiblingElement();
								continue;
							}

							str = nextElement->Attribute( "name" );
							if ( !str )
							{
								Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing AlarmEventParam name attribute", nextElement->GetDocument()->GetUserData(), nextElement->Row() );
								nextElement = nextElement->NextSiblingElement();
								continue;
							}
							aep->name = str;

							if (ne->EventParams.find(aep->id) == ne->EventParams.end())
								ne->EventParams[aep->id] = aep;
							else {
								Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s - A AlarmEventParam with id %d already exists. Skipping ", nextElement->GetDocument()->GetUserData(), aep->id);
								delete aep;
							}
						}
						nextElement = nextElement->NextSiblingElement();
					}
					if (nt->Events.find(ne->id) == nt->Events.end())
						nt->Events[ne->id] = ne;
					else {
						Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s - A AlarmEventElement with id %d already exists. Skipping ", AlarmEventElement->GetDocument()->GetUserData(), ne->id);
						delete ne;
					}
				}
				AlarmEventElement = AlarmEventElement->NextSiblingElement();
			}
			if (Notifications.find(nt->id) == Notifications.end())
				Notifications[nt->id] = nt;
			else {
				Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s - A AlarmTypeElement with id %d already exists. Skipping ", AlarmTypeElement->GetDocument()->GetUserData(), nt->id);
				delete nt;
			}
		}
		AlarmTypeElement = AlarmTypeElement->NextSiblingElement();
	}
	Log::Write(LogLevel_Info, "Loaded %s With Revision %d", pDoc->GetUserData(), m_revision);
#if 0
	std::cout << "NotificationCCTypes" << std::endl;
	for (std::map<uint32, NotificationCCTypes::NotificationTypes *>::iterator it = Notifications.begin(); it != Notifications.end(); it++) {
		std::cout << "\tAlarmType:" << it->first << " Name: " << it->second->name << std::endl;
		for (std::map<uint32, NotificationCCTypes::NotificationEvents *>::iterator it2 = it->second->Events.begin(); it2 != it->second->Events.end(); it2++) {
			std::cout << "\t\tAlarmEvents: " << it2->first << " Name: " << it2->second->name << std::endl;
			for (std::map<uint32, NotificationCCTypes::NotificationEventParams* >::iterator it3 = it2->second->EventParams.begin(); it3 != it2->second->EventParams.end(); it3++) {
				std::cout << "\t\t\tEventParams: " << it3->first << " Name: " << it3->second->name << " Type: " << GetEventParamNames(it3->second->type) << std::endl;
				for (std::map<uint32, string>::iterator it4 = it3->second->ListItems.begin(); it4 != it3->second->ListItems.end(); it4++) {
					std::cout << "\t\t\t\tEventParamsList: " << it4->first << " Name: " << it4->second << std::endl;
				}
			}
		}
	}
#endif

}

string NotificationCCTypes::GetEventParamNames
(
		NotificationEventParamTypes type
)
{
	switch (type) {
	case NEPT_Location:
		return "Location";
		break;
	case NEPT_List:
		return "List";
		break;
	case NEPT_UserCodeReport:
		return "UserCodeReport";
		break;
	case NEPT_Byte:
		return "Byte";
		break;
	case NEPT_String:
		return "String";
		break;
	case NEPT_Time:
		return "Duration";
		break;
	};
	return "Unknown";
}

string NotificationCCTypes::GetAlarmType
(
		uint32 type
)
{
	if (Notifications.find(type) != Notifications.end()) {
		return Notifications.at(type)->name;
	}
	Log::Write( LogLevel_Warning, "NotificationCCTypes::GetAlarmType - Unknown AlarmType %d", type);
	return "Unknown";
}

string NotificationCCTypes::GetEventForAlarmType
(
		uint32 type,
		uint32 event
)
{
	if ( const NotificationCCTypes::NotificationEvents *ne = NotificationCCTypes::GetAlarmNotificationEvents(type, event)) {
		return ne->name;
	}
	Log::Write( LogLevel_Warning, "NotificationCCTypes::GetEventForAlarmType - Unknown AlarmType/Event %d/d", type, event);
	return "Unknown";
}



const NotificationCCTypes::NotificationTypes* NotificationCCTypes::GetAlarmNotificationTypes
(
		uint32 type
)
{
	if (Notifications.find(type) != Notifications.end()) {
		return Notifications.at(type);
	}
	else
	{
		Log::Write( LogLevel_Warning, "NotificationCCTypes::GetAlarmNotificationTypes - Unknown Alarm Type %d", type);
	}
	return NULL;
}

const NotificationCCTypes::NotificationEvents* NotificationCCTypes::GetAlarmNotificationEvents
(
		uint32 type,
		uint32 event
)
{
	if (const NotificationCCTypes::NotificationTypes *nt = GetAlarmNotificationTypes(type)) {
		if (nt->Events.find(event) != nt->Events.end()) {
			return nt->Events.at(event);
		}
		Log::Write( LogLevel_Warning, "NotificationCCTypes::GetAlarmNotificationEvents - Unknown Alarm Event %d for Alarm Type %s (%d)", event, GetAlarmType(type).c_str(), type);
	}
	return NULL;
}

const std::map<uint32, NotificationCCTypes::NotificationEventParams* > NotificationCCTypes::GetAlarmNotificationEventParams
(
		uint32 type,
		uint32 event
)
{
	if (const NotificationCCTypes::NotificationTypes *nt = GetAlarmNotificationTypes(type)) {
		if (nt->Events.find(event) != nt->Events.end()) {
			return nt->Events.at(event)->EventParams;
		}
		Log::Write( LogLevel_Warning, "NotificationCCTypes::GetAlarmNotificationEventParams - Unknown Alarm Event %d for Alarm Type %s (%d)", event, GetAlarmType(type).c_str(), type);
	}
	return std::map<uint32, NotificationCCTypes::NotificationEventParams* >();
}

bool NotificationCCTypes::Create
(
)
{
	if (m_instance != NULL)
	{
		return true;
	}
	m_instance = new NotificationCCTypes();
	ReadXML();
	return true;
}



NotificationCCTypes *NotificationCCTypes::Get
(
)
{
	if ( m_instance != NULL )
	{
		return m_instance;
	}
	m_instance = new NotificationCCTypes();
	ReadXML();
	return m_instance;
}
