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
										str = nextElement->Attribute( "id" );
										if ( !str )
										{
											Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing Item id attribute", nextElement->GetDocument()->GetUserData(), nextElement->Row() );
											listElement = listElement->NextSiblingElement();
											continue;
										}
										//uint32 listID = (uint32)strtol( str, &pStopChar, 10 );
										str = nextElement->Attribute( "name" );
										if ( !str )
										{
											Log::Write( LogLevel_Warning, "NotificationCCTypes::ReadXML: Error in %s at line %d - missing Item name attribute", nextElement->GetDocument()->GetUserData(), nextElement->Row() );
											listElement = listElement->NextSiblingElement();
											continue;
										}
									}
									listElement = listElement->NextSiblingElement();
								}
							} else if (!strcasecmp(str, "usercodereport")) {
								aep->type = NotificationCCTypes::NEPT_UserCodeReport;
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

							ne->EventParams[aep->id] = aep;
						}
						nextElement = nextElement->NextSiblingElement();
					}
					nt->Events[ne->id] = ne;
				}
				AlarmEventElement = AlarmEventElement->NextSiblingElement();
			}
			Notifications[nt->id] = nt;
		}
		AlarmTypeElement = AlarmTypeElement->NextSiblingElement();
	}
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
