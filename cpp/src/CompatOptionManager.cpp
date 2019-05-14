//-----------------------------------------------------------------------------
//
//	CompatOptionManager.cpp
//
//	Handles Compatibility Flags in Config Files
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

#include "command_classes/CommandClass.h"
#include "CompatOptionManager.h"
#include "platform/Log.h"

namespace OpenZWave {

CompatOptionFlagDefintions availableCompatFlags[] =
{
		{ "GetSupported",				COMPAT_FLAG_GETSUPPORTED,				COMPAT_FLAG_TYPE_BOOL },
		{ "OverridePrecision",			COMPAT_FLAG_OVERRIDEPRECISION,			COMPAT_FLAG_TYPE_BYTE },
		{ "ForceVersion",				COMPAT_FLAG_FORCEVERSION,				COMPAT_FLAG_TYPE_BYTE },
		{ "CreateVars",					COMPAT_FLAG_CREATEVARS,					COMPAT_FLAG_TYPE_BOOL },
		{ "RefreshOnWakeup",			COMPAT_FLAG_REFRESHONWAKEUP,			COMPAT_FLAG_TYPE_BOOL },
		{ "IgnoreMapping",				COMPAT_FLAG_BASIC_IGNOREREMAPPING,		COMPAT_FLAG_TYPE_BOOL },
		{ "SetAsReport",				COMPAT_FLAG_BASIC_SETASREPORT,			COMPAT_FLAG_TYPE_BOOL },
		{ "Mapping",					COMPAT_FLAG_BASIC_MAPPING,				COMPAT_FLAG_TYPE_BYTE },
		{ "ColorIndexBug",				COMPAT_FLAG_COLOR_IDXBUG,				COMPAT_FLAG_TYPE_BOOL }, // Fibaro RGBW before version 25.25 always reported the coloridx as 3 in the Report Message. Work around it
		{ "ForceInstances",				COMPAT_FLAG_MCA_FORCEINSTANCES,			COMPAT_FLAG_TYPE_BOOL },
		{ "MapRootToEndpoint",			COMPAT_FLAG_MI_MAPROOTTOENDPOINT,		COMPAT_FLAG_TYPE_BOOL }, // was mapping in old version. was 0 or false in old version. when mapping=endpoints, thats = true
		{ "ForceUniqueEndpoints",		COMPAT_FLAG_MI_FORCEUNIQUEENDPOINTS,	COMPAT_FLAG_TYPE_BOOL },
		{ "IgnoreMCCapReports",			COMPAT_FLAG_MI_IGNMCCAPREPORTS,			COMPAT_FLAG_TYPE_BOOL }, // was ignoreUnsolicitedMultiChnCapReport
		{ "EndpointHint",				COMPAT_FLAG_MI_ENDPOINTHINT,			COMPAT_FLAG_TYPE_BYTE },
		{ "Base",						COMPAT_FLAG_TSSP_BASE,					COMPAT_FLAG_TYPE_BYTE },
		{ "AltTypeInterpretation",		COMPAT_FLAG_TSSP_ALTTYPEINTERPRETATION,	COMPAT_FLAG_TYPE_BOOL },
		{ "ExposeRawUserCodes",			COMPAT_FLAG_UC_EXPOSERAWVALUE,			COMPAT_FLAG_TYPE_BOOL },
		{ "ClassGetVersionSupported",	COMPAT_FLAG_VERSION_GETCLASSVERSION,	COMPAT_FLAG_TYPE_BOOL },
		{ "DelayNoMoreInfo",			COMPAT_FLAG_WAKEUP_DELAYNMI,			COMPAT_FLAG_TYPE_INT  },
};

uint16_t availableCompatFlagsCount = sizeof(availableCompatFlags) / sizeof(availableCompatFlags[0]);

CompatOptionFlagDefintions availableDiscoveryFlags [] =
{
		{ "CCVersion",				STATE_FLAG_CCVERSION, 					COMPAT_FLAG_TYPE_BYTE },
		{ "StaticRequests",			STATE_FLAG_STATIC_REQUESTS,				COMPAT_FLAG_TYPE_BYTE },
		{ "AfterMark",				STATE_FLAG_AFTERMARK,					COMPAT_FLAG_TYPE_BOOL },
		{ "Encrypted",				STATE_FLAG_ENCRYPTED,					COMPAT_FLAG_TYPE_BOOL },
		{ "InNif",					STATE_FLAG_INNIF,						COMPAT_FLAG_TYPE_BOOL },
		{ "SceneCount",				STATE_FLAG_CS_SCENECOUNT,				COMPAT_FLAG_TYPE_BYTE },
		{ "ClearTimeout",			STATE_FLAG_CS_CLEARTIMEOUT,				COMPAT_FLAG_TYPE_INT  },
		{ "ChangeCounter",			STATE_FLAG_CCS_CHANGECOUNTER,			COMPAT_FLAG_TYPE_BYTE },
		{ "Channels",				STATE_FLAG_COLOR_CHANNELS,				COMPAT_FLAG_TYPE_SHORT},
		{ "TimeOut",				STATE_FLAG_DOORLOCK_TIMEOUT,			COMPAT_FLAG_TYPE_BYTE },
		{ "InsideMode",				STATE_FLAG_DOORLOCK_INSIDEMODE,			COMPAT_FLAG_TYPE_BYTE },
		{ "OutsideMode",			STATE_FLAG_DOORLOCK_OUTSIDEMODE,		COMPAT_FLAG_TYPE_BYTE },
		{ "TimeOutMins",			STATE_FLAG_DOORLOCK_TIMEOUTMINS,		COMPAT_FLAG_TYPE_BYTE },
		{ "TImeOutSecs",			STATE_FLAG_DOORLOCK_TIMEOUTSECS,		COMPAT_FLAG_TYPE_BYTE },
		{ "MaxRecords",				STATE_FLAG_DOORLOCKLOG_MAXRECORDS,		COMPAT_FLAG_TYPE_BYTE },
		{ "Count",					STATE_FLAG_USERCODE_COUNT,				COMPAT_FLAG_TYPE_BYTE }
};

uint16_t availableDiscoveryFlagsCount = sizeof(availableDiscoveryFlags) / sizeof(availableDiscoveryFlags[0]);

CompatOptionManager::CompatOptionManager
(
		CompatOptionType type,
		CommandClass *owner
):
m_owner(owner),
m_comtype(type)
{
	switch (m_comtype) {
		case CompatOptionType_Compatibility:
			m_availableFlags = availableCompatFlags;
			m_availableFlagsCount = availableCompatFlagsCount;
			break;
		case CompatOptionType_Discovery:
			m_availableFlags = availableDiscoveryFlags;
			m_availableFlagsCount = availableDiscoveryFlagsCount;
			break;
	}
}

CompatOptionManager::~CompatOptionManager
(
)
{
}


void CompatOptionManager::EnableFlag
(
		CompatOptionFlags flag,
		uint32_t defaultval
)
{
	for (uint32_t i = 0; i < m_availableFlagsCount; i++) {
		if (m_availableFlags[i].flag == flag) {
			m_enabledCompatFlags[m_availableFlags[i].name] = flag;
			m_CompatVals[flag].type = m_availableFlags[i].type;
			m_CompatVals[flag].changed = false;
			switch (m_availableFlags[i].type)
			{
				case COMPAT_FLAG_TYPE_BOOL:
					if (defaultval > 2) {
						Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "EnableFlag: Default Value for %s is not a Bool", m_availableFlags[i].name.c_str());
						defaultval = 0;
					}
					m_CompatVals[flag].valBool = defaultval == 0 ? false : true;
					break;
				case COMPAT_FLAG_TYPE_BYTE:
					if (defaultval > UINT8_MAX) {
						Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "EnableFlag: Default Value for %s is larger than a byte", m_availableFlags[i].name.c_str());
						defaultval = 0;
					}
					m_CompatVals[flag].valByte = defaultval;
					break;
				case COMPAT_FLAG_TYPE_SHORT:
					if (defaultval > UINT16_MAX) {
						Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "EnableFlag: Default Value for %s is larger than a short", m_availableFlags[i].name.c_str());
						defaultval = 0;
					}
					m_CompatVals[flag].valShort = defaultval;
					break;
				case COMPAT_FLAG_TYPE_INT:
					if (defaultval > UINT32_MAX) {
						Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "EnableFlag: Default Value for %s is larger than a int", m_availableFlags[i].name.c_str());
						defaultval = 0;
					}
					m_CompatVals[flag].valInt = defaultval;
					break;
			}
		}
	}
}

void CompatOptionManager::ReadXML
(
		TiXmlElement const* _ccElement
)
{
	TiXmlElement const *compatElement = _ccElement->FirstChildElement(GetXMLTagName().c_str());

	if (compatElement) {
		map<string,CompatOptionFlags>::iterator it;
		string value;
		for ( it = m_enabledCompatFlags.begin(); it != m_enabledCompatFlags.end(); it++)
		{
			TiXmlElement const *valElement = compatElement->FirstChildElement(it->first.c_str());
			if (valElement) {
				value = valElement->GetText();
				char* pStopChar;
				uint32_t val = strtol( value.c_str(), &pStopChar, 10 );;
	std::cout << "Flags: " << it->first.c_str() << value.c_str() << std::endl;;
				switch (m_CompatVals[it->second].type) {
					case COMPAT_FLAG_TYPE_BOOL:
						m_CompatVals[it->second].valBool = !strcmp(value.c_str(), "true");
						m_CompatVals[it->second].changed = true;
						break;
					case COMPAT_FLAG_TYPE_BYTE:
						if (val > UINT8_MAX) {
							Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "ReadXML: (%s) - Value for %s is larger than a byte", m_owner->GetCommandClassName().c_str(), it->first.c_str());
							val = 0;
						}
						m_CompatVals[it->second].valByte = val;
						m_CompatVals[it->second].changed = true;
						break;
					case COMPAT_FLAG_TYPE_SHORT:
						if (val > UINT16_MAX) {
							Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "ReadXML: (%s) - Value for %s is larger than a short", m_owner->GetCommandClassName().c_str(), it->first.c_str());
							val = 0;
						}
						m_CompatVals[it->second].valShort = val;
						m_CompatVals[it->second].changed = true;
						break;
					case COMPAT_FLAG_TYPE_INT:
						if (val > UINT32_MAX) {
							Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "ReadXML: (%s) - Value for %s is larger than a int", m_owner->GetCommandClassName().c_str(), it->first.c_str());
							val = 0;
						}
						m_CompatVals[it->second].valInt = val;
						m_CompatVals[it->second].changed = true;
						break;
				}
			}
		}
	}
	{
		map<string,CompatOptionFlags>::iterator it;
		Log::Write(LogLevel_Info, m_owner->GetNodeId(), "(%d - %s) - %s Flags:", m_owner->GetCommandClassId(), m_owner->GetCommandClassName().c_str(), GetXMLTagName().c_str());
		for ( it = m_enabledCompatFlags.begin(); it != m_enabledCompatFlags.end(); it++)
		{
			if (m_CompatVals[it->second].changed) {
				switch (m_CompatVals[it->second].type) {
					case COMPAT_FLAG_TYPE_BOOL:
						Log::Write(LogLevel_Info, m_owner->GetNodeId(), "\t %s: %s", it->first.c_str(), m_CompatVals[it->second].valBool ? "true": "false");
						break;
					case COMPAT_FLAG_TYPE_BYTE:
						Log::Write(LogLevel_Info, m_owner->GetNodeId(), "\t %s: %d", it->first.c_str(), m_CompatVals[it->second].valByte);
						break;
					case COMPAT_FLAG_TYPE_SHORT:
						Log::Write(LogLevel_Info, m_owner->GetNodeId(), "\t %s: %d", it->first.c_str(), m_CompatVals[it->second].valShort);
						break;
					case COMPAT_FLAG_TYPE_INT:
						Log::Write(LogLevel_Info, m_owner->GetNodeId(), "\t %s: %d", it->first.c_str(), m_CompatVals[it->second].valInt);
						break;
				}
			}
		}
	}

}

void CompatOptionManager::WriteXML
(
		TiXmlElement * _ccElement
)
{
	TiXmlElement* compatElement = new TiXmlElement( GetXMLTagName().c_str() );

	map<string,CompatOptionFlags>::iterator it;
	string value;
	for ( it = m_enabledCompatFlags.begin(); it != m_enabledCompatFlags.end(); it++)
	{
		TiXmlElement* valElement = new TiXmlElement( it->first.c_str() );
		char str[32];
		TiXmlText * text = NULL;
//		std::cout << "Name " << it->first << " Type: " << m_CompatVals[it->second].type << std::endl;
		switch (m_CompatVals[it->second].type) {
			case COMPAT_FLAG_TYPE_BOOL:
				text = new TiXmlText(m_CompatVals[it->second].valBool == true ? "true" : "false");
				break;
			case COMPAT_FLAG_TYPE_BYTE:
				snprintf(str, sizeof(str), "%d", m_CompatVals[it->second].valByte);
				text = new TiXmlText(str);
				break;
			case COMPAT_FLAG_TYPE_SHORT:
				snprintf(str, sizeof(str), "%d", m_CompatVals[it->second].valShort);
				text = new TiXmlText(str);
				break;
			case COMPAT_FLAG_TYPE_INT:
				snprintf(str, sizeof(str), "%d", m_CompatVals[it->second].valInt);
				text = new TiXmlText(str);
				break;
		}
		valElement->LinkEndChild(text);
		compatElement->LinkEndChild( valElement );
	}
	_ccElement->LinkEndChild(compatElement);
}

bool CompatOptionManager::GetFlagBool
(
		CompatOptionFlags const flag
) const
{
	if (m_CompatVals.count(flag) == 0)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagBool: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_BOOL)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagBool: (%s) - Flag %s Not a Boolean Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	return m_CompatVals.at(flag).valBool;
}

uint8_t CompatOptionManager::GetFlagByte
(
		CompatOptionFlags flag
) const
{
	if (m_CompatVals.count(flag) == 0) {
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagByte: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return 0;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_BYTE)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagByte: (%s) - Flag %s Not a Byte Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return 0;
	}
	return m_CompatVals.at(flag).valByte;
}

uint16_t CompatOptionManager::GetFlagShort
(
		CompatOptionFlags flag
) const
{
	if (m_CompatVals.count(flag) == 0) {
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagShort: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return 0;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_SHORT)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagShort: (%s) - Flag %s Not a Short Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return 0;
	}
	return m_CompatVals.at(flag).valShort;
}

uint32_t CompatOptionManager::GetFlagInt
(
		CompatOptionFlags flag
) const
{
	if (m_CompatVals.count(flag) == 0) {
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagInt: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return 0;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_INT)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "GetFlagInt: (%s) - Flag %s Not a Int Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return 0;
	}
	return m_CompatVals.at(flag).valInt;
}

bool CompatOptionManager::SetFlagBool
(
		CompatOptionFlags flag,
		bool value
)
{
	if (m_CompatVals.count(flag) == 0) {
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagBool: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_BOOL)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagBool: (%s) - Flag %s Not a Bool Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	m_CompatVals.at(flag).valBool = value;
	m_CompatVals.at(flag).changed = true;
	return true;
}

bool CompatOptionManager::SetFlagByte
(
		CompatOptionFlags flag,
		uint8_t value
)
{
	if (m_CompatVals.count(flag) == 0) {
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagByte: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_BYTE)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagByte: (%s) - Flag %s Not a Byte Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	m_CompatVals.at(flag).valByte = value;
	m_CompatVals.at(flag).changed = true;
	return true;
}

bool CompatOptionManager::SetFlagShort
(
		CompatOptionFlags flag,
		uint16_t value
)
{
	if (m_CompatVals.count(flag) == 0) {
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagShort: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_SHORT)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagShort: (%s) - Flag %s Not a Short Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	m_CompatVals.at(flag).valShort = value;
	m_CompatVals.at(flag).changed = true;
	return true;
}

bool CompatOptionManager::SetFlagInt
(
		CompatOptionFlags flag,
		uint32_t value
)
{
	if (m_CompatVals.count(flag) == 0) {
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagInt: (%s) - Flag %s Not Enabled!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	if (m_CompatVals.at(flag).type != COMPAT_FLAG_TYPE_INT)
	{
		Log::Write(LogLevel_Warning, m_owner->GetNodeId(), "SetFlagInt: (%s) - Flag %s Not a Int Value!", m_owner->GetCommandClassName().c_str(), GetFlagName(flag).c_str());
		return false;
	}
	m_CompatVals.at(flag).valInt = value;
	m_CompatVals.at(flag).changed = true;
	return true;
}

string CompatOptionManager::GetFlagName
(
		CompatOptionFlags flag
) const
{
	for (uint32_t i = 0; i < m_availableFlagsCount; i++) {
		if (m_availableFlags[i].flag == flag) {
			return m_availableFlags[i].name;
		}
	}
	return "Unknown";
}
string CompatOptionManager::GetXMLTagName
(
)
{
	switch (m_comtype) {
		case CompatOptionType_Compatibility:
			return "Compatibility";
		case CompatOptionType_Discovery:
			return "State";
	}
	assert(0);	
	return "Unknown";
}
} /* namespace OpenZWave */
