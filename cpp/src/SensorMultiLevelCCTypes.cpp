//-----------------------------------------------------------------------------
//
//	SensorMultiLevelCCTypes.cpp
//
//	SensorMultiLevelCCTypes for SensorMultiLevel Command Class
//
//	Copyright (c) 2019 Justin Hammond <justin@dynam.ac>
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
#include "SensorMultiLevelCCTypes.h"

#include <string.h>

#include "tinyxml.h"
#include "Options.h"
#include "Utils.h"
#include "platform/Log.h"

namespace OpenZWave
{
	namespace Internal
	{

		SensorMultiLevelCCTypes *SensorMultiLevelCCTypes::m_instance = NULL;
		std::map<uint32, std::shared_ptr<SensorMultiLevelCCTypes::SensorMultiLevelTypes> > SensorMultiLevelCCTypes::SensorTypes;
		uint32 SensorMultiLevelCCTypes::m_revision(0);

		SensorMultiLevelCCTypes::SensorMultiLevelCCTypes()
		{
		}

		void SensorMultiLevelCCTypes::ReadXML()
		{
			// Parse the Z-Wave manufacturer and product XML file.
			string configPath;
			Options::Get()->GetOptionAsString("ConfigPath", &configPath);

			string path = configPath + "SensorMultiLevelCCTypes.xml";
			TiXmlDocument* pDoc = new TiXmlDocument();
			if (!pDoc->LoadFile(path.c_str(), TIXML_ENCODING_UTF8))
			{
				delete pDoc;
				Log::Write(LogLevel_Warning, "Unable to load SensorMultiLevelCCTypes file %s", path.c_str());
				return;
			}
			pDoc->SetUserData((void*) path.c_str());
			Log::Write(LogLevel_Info, "Loading SensorMultiLevelCCTypes File %s", path.c_str());

			TiXmlElement const* root = pDoc->RootElement();
			char const *str = root->Value();
			if (str && !strcmp(str, "SensorTypes"))
			{
				// Read in the revision attributes
				str = root->Attribute("Revision");
				if (!str)
				{
					Log::Write(LogLevel_Info, "Error in SensorMultiLevel Config file at line %d - missing Revision  attribute", root->Row());
					delete pDoc;
					return;
				}
				m_revision = atol(str);
			}
			TiXmlElement const* SensorTypeElement = root->FirstChildElement();
			while (SensorTypeElement)
			{
				char const* str = SensorTypeElement->Value();
				char* pStopChar;
				if (str && !strcmp(str, "SensorType"))
				{
					SensorMultiLevelTypes *st = new SensorMultiLevelTypes;

					str = SensorTypeElement->Attribute("id");
					if (!str)
					{
						Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::ReadXML: Error in %s at line %d - missing SensorType ID attribute", SensorTypeElement->GetDocument()->GetUserData(), SensorTypeElement->Row());
						SensorTypeElement = SensorTypeElement->NextSiblingElement();
						delete st;
						continue;
					}
					st->id = (uint32) strtol(str, &pStopChar, 10);
					str = SensorTypeElement->Attribute("name");
					if (!str)
					{
						Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::ReadXML: Error in %s at line %d - missing SensorType name attribute", SensorTypeElement->GetDocument()->GetUserData(), SensorTypeElement->Row());
						SensorTypeElement = SensorTypeElement->NextSiblingElement();
						delete st;
						continue;
					}
					st->name = str;
					trim(st->name);
					TiXmlElement const* SensorScaleElement = SensorTypeElement->FirstChildElement();
					while (SensorScaleElement)
					{
						str = SensorScaleElement->Value();
						if (str && !strcmp(str, "SensorScale"))
						{
							SensorMultiLevelScales *ss = new SensorMultiLevelScales;
							str = SensorScaleElement->Attribute("id");
							if (!str)
							{
								Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::ReadXML: Error in %s at line %d - missing SensorScale id attribute", SensorScaleElement->GetDocument()->GetUserData(), SensorScaleElement->Row());
								SensorScaleElement = SensorScaleElement->NextSiblingElement();
								delete ss;
								continue;
							}

							ss->id = (uint32) strtol(str, &pStopChar, 10);

							str = SensorScaleElement->Attribute("name");
							if (!str)
							{
								Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::ReadXML: Error in %s at line %d - missing SensorScale name attribute", SensorScaleElement->GetDocument()->GetUserData(), SensorScaleElement->Row());
								SensorScaleElement = SensorScaleElement->NextSiblingElement();
								delete ss;
								continue;
							}
							ss->name = str;
							trim(ss->name);

							str = SensorScaleElement->GetText();
							if (str) {
								ss->unit = str;
								trim(ss->unit);
							}

							if (st->allSensorScales.find(ss->id) == st->allSensorScales.end())
								st->allSensorScales[ss->id] = std::shared_ptr<SensorMultiLevelCCTypes::SensorMultiLevelScales>(ss);
							else
							{
								Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::ReadXML: Error in %s at line %d - A SensorScale with id %d already exists. Skipping ", SensorScaleElement->GetDocument()->GetUserData(), SensorScaleElement->Row(), ss->id);
								delete ss;
							}
						}
						SensorScaleElement = SensorScaleElement->NextSiblingElement();
					}
					if (SensorTypes.find(st->id) == SensorTypes.end())
						SensorTypes[st->id] = std::shared_ptr<SensorMultiLevelCCTypes::SensorMultiLevelTypes>(st);
					else
					{
						Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::ReadXML: Error in %s at line %d - A SensorTypeElement with id %d already exists. Skipping ", SensorTypeElement->GetDocument()->GetUserData(), SensorTypeElement->Row(), st->id);
						delete st;
					}
				}
				SensorTypeElement = SensorTypeElement->NextSiblingElement();
			}
			Log::Write(LogLevel_Info, "Loaded %s With Revision %d", pDoc->GetUserData(), m_revision);
#if 0
			std::cout << "SensorMultiLevelCCTypes" << std::endl;
			for (std::map<uint32, SensorMultiLevelCCTypes::SensorMultiLevelTypes *>::iterator it = SensorTypes.begin(); it != SensorTypes.end(); it++)
			{
				std::cout << "\tSensorTypes:" << (uint32)it->first << " Name: " << it->second->name << std::endl;
				for (std::map<uint8, SensorMultiLevelCCTypes::SensorMultiLevelScales *>::iterator it2 = it->second->allSensorScales.begin(); it2 != it->second->allSensorScales.end(); it2++)
				{
					std::cout << "\t\tSensorScales: " << (uint32)it2->first << " Name: " << it2->second->name << std::endl;
				}
			}
			exit(0);
#endif
			delete pDoc;
		}

		std::string SensorMultiLevelCCTypes::GetSensorName(uint32 type)
		{
			if (SensorTypes.find(type) != SensorTypes.end())
			{
				return SensorTypes.at(type)->name;
			}
			Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::GetSensorName - Unknown SensorType %d", type);
			return "Unknown";
		}

		std::string SensorMultiLevelCCTypes::GetSensorUnit(uint32 type, uint8 scale)
		{
			if (SensorTypes.find(type) == SensorTypes.end())
			{
				Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::GetSensorUnit - Unknown SensorType %d", type);
				return "";
			}
			SensorScales ss = SensorTypes.at(type)->allSensorScales;
			if (ss.find(scale) == ss.end())
			{
				Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::GetSensorUnit - Unknown SensorScale %d", scale);
				return "";

			}
			return ss.at(scale)->unit;
		}

		std::string SensorMultiLevelCCTypes::GetSensorUnitName(uint32 type, uint8 scale) {
			if (SensorTypes.find(type) == SensorTypes.end())
			{
				Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::GetSensorUnit - Unknown SensorType %d", type);
				return "";
			}
			SensorScales ss = SensorTypes.at(type)->allSensorScales;
			if (ss.find(scale) == ss.end())
			{
				Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::GetSensorUnit - Unknown SensorScale %d", scale);
				return "";

			}
			return ss.at(scale)->name;

		}


		const SensorMultiLevelCCTypes::SensorScales SensorMultiLevelCCTypes::GetSensorScales(uint32 type)
		{
			if (SensorTypes.find(type) == SensorTypes.end())
			{
				Log::Write(LogLevel_Warning, "SensorMultiLevelCCTypes::GetSensorUnit - Unknown SensorType %d", type);
				return SensorScales();
			}
			return SensorTypes.at(type)->allSensorScales;
		}


		bool SensorMultiLevelCCTypes::Create()
		{
			if (m_instance != NULL)
			{
				return true;
			}
			m_instance = new SensorMultiLevelCCTypes();
			ReadXML();
			return true;
		}

		SensorMultiLevelCCTypes *SensorMultiLevelCCTypes::Get()
		{
			if (m_instance != NULL)
			{
				return m_instance;
			}
			m_instance = new SensorMultiLevelCCTypes();
			ReadXML();
			return m_instance;
		}
	} // namespace Internal
} // namespace OpenZWave
