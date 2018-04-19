//-----------------------------------------------------------------------------
//
//	Localization.cpp
//
//	Localization for CC and Value Classes
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
#include <bitset>

#include "Localization.h"
#include "tinyxml.h"
#include "Options.h"
#include "platform/Log.h"
#include "value_classes/ValueBitSet.h"
#include "command_classes/Configuration.h"

using namespace OpenZWave;

Localization *Localization::m_instance = NULL;
map<int64,ValueLocalizationEntry*> Localization::m_valueLocalizationMap;
map<uint8,LabelLocalizationEntry*> Localization::m_commandClassLocalizationMap;
string Localization::m_selectedLang = "";

LabelLocalizationEntry::LabelLocalizationEntry
(
        uint16 _index,
        uint32 _pos
):
    m_index( _index ),
    m_pos( _pos )
{
    //std::cout << "Index: " << unsigned(m_index) << " pos: " << unsigned(m_pos) << std::endl;
    //std::cout << "Key: " << GetIdx() << " " << std::bitset<64>(GetIdx()) << std::endl;
}



void LabelLocalizationEntry::AddLabel
(
        string label,
        string lang
        )
{
    if (lang.empty())
        m_defaultLabel = label;
    else
        m_Label[lang] = label;
}
uint64 LabelLocalizationEntry::GetIdx
(
        )
{
    uint64 key =  ((uint64)m_index << 32) | ((uint64)m_pos);
    return key;

}
string LabelLocalizationEntry::GetLabel
(
        string lang
        )
{
    if (lang.empty() || (m_Label.find(lang) == m_Label.end()))
        return m_defaultLabel;
    else
        return m_Label[lang];

}

ValueLocalizationEntry::ValueLocalizationEntry
(
        uint8 _commandClass,
        uint16 _index,
        uint32 _pos
):
    m_commandClass( _commandClass ),
    m_index( _index ),
    m_pos( _pos )
{
    //std::cout << "CommandClass: " << unsigned(m_commandClass) << " Index: " << unsigned(m_index) << " pos: " << unsigned(m_pos) << std::endl;
    //std::cout << "Key: " << GetIdx() << " " << std::bitset<64>(GetIdx()) << std::endl;


}


uint64 ValueLocalizationEntry::GetIdx
(
        )
{
    uint64 key = ((uint64)m_commandClass << 48) | ((uint64)m_index << 32) | ((uint64)m_pos);
    return key;
}
string ValueLocalizationEntry::GetHelpText
(
        string lang
        )

{
    if (lang.empty() || (m_HelpText.find(lang) == m_HelpText.end()))
        return m_DefaultHelpText;
    else
        return m_HelpText[lang];
}

void ValueLocalizationEntry::AddHelp
(
        string HelpText,
        string lang
        )
{
    if (lang.empty())
        m_DefaultHelpText = HelpText;
    else
        m_HelpText[lang] = HelpText;

}
string ValueLocalizationEntry::GetLabelText
(
        string lang
        )
{
    if (lang.empty() || (m_LabelText.find(lang) == m_LabelText.end()))
        return m_DefaultLabelText;
    else
        return m_LabelText[lang];
}
void ValueLocalizationEntry::AddLabel
(
        string Label,
        string lang
        )
{
    if (lang.empty())
        m_DefaultLabelText = Label;
    else
        m_LabelText[lang] = Label;
}



Localization::Localization()
{
}

void Localization::ReadXML
(
        )
{
    // Parse the Z-Wave manufacturer and product XML file.
    string configPath;
    Options::Get()->GetOptionAsString( "ConfigPath", &configPath );

    string path = configPath + "Localization.xml";
    TiXmlDocument* pDoc = new TiXmlDocument();
    if( !pDoc->LoadFile( path.c_str(), TIXML_ENCODING_UTF8 ) )
    {
        delete pDoc;
        Log::Write( LogLevel_Warning, "Unable to load ValueHelp file %s", path.c_str() );
        return;
    }
    Log::Write( LogLevel_Info, "Loading Localization File %s", path.c_str() );

    TiXmlElement const* root = pDoc->RootElement();
    TiXmlElement const* CCElement = root->FirstChildElement();
    while( CCElement )
    {
        char const* str = CCElement->Value();
        char* pStopChar;
        if( str && !strcmp( str, "CommandClass" ) )
        {
            str = CCElement->Attribute( "id" );
            if( !str )
            {
                Log::Write( LogLevel_Warning, "Localization::ReadXML: Error in Localization.xml at line %d - missing commandclass ID attribute", CCElement->Row() );
                CCElement = CCElement->NextSiblingElement();
                continue;
            }
            uint8 ccID = (uint8)strtol( str, &pStopChar, 10 );
            TiXmlElement const* LangElement = CCElement->FirstChildElement();
            while (LangElement) {
            	str = LangElement->Value();
            	if (str && !strcmp( str, "Lang" ) ) {
            		TiXmlElement const* LabelElement = LangElement->FirstChildElement();
					while (LabelElement)
					{
						str = LabelElement->Value();
						if (str && !strcmp( str, "Label" ) )
						{
							ReadXMLLabel(ccID, LabelElement, LangElement->Attribute("id"));
						}
						if (str && !strcmp( str, "Value" ) )
						{
							ReadXMLValue(ccID, LabelElement, LangElement->Attribute("id"));
						}
						LabelElement = LabelElement->NextSiblingElement();
					}
            	}
            	LangElement = LangElement->NextSiblingElement();
            }
        }


        CCElement = CCElement->NextSiblingElement();
    }
}
void Localization::ReadXMLLabel(uint8 ccID, const TiXmlElement *labelElement, const string Language) {


    if (m_commandClassLocalizationMap.find(ccID) == m_commandClassLocalizationMap.end()) {
        m_commandClassLocalizationMap[ccID] = new LabelLocalizationEntry(0);
        //std::cout << "Adding " << unsigned(ccID) << " Label: " << labelElement->GetText() << std::endl;
    } else {
        Log::Write( LogLevel_Warning, "Localization::ReadXMLLabel: Error in Localization.xml at line %d - Duplicate Entry for CommandClass %d: %s", labelElement->Row(), ccID, labelElement->GetText() );
    }

    if( Language.empty() )
    {
        m_commandClassLocalizationMap[ccID]->AddLabel(labelElement->GetText());
    }
    else
    {
        m_commandClassLocalizationMap[ccID]->AddLabel(labelElement->GetText(), Language);
    }
}

void Localization::ReadXMLValue(uint8 ccID, const TiXmlElement *valueElement, const string Language) {

    char const* str = valueElement->Attribute( "index");
    if ( !str )
    {
        Log::Write( LogLevel_Info, "Localization::ReadXMLValue: Error in Localization.xml at line %d - missing Index  attribute", valueElement->Row() );
        return;
    }
    char* pStopChar;
    uint16 indexId = (uint16)strtol( str, &pStopChar, 16 );

    uint32 pos = -1;
    str = valueElement->Attribute( "pos");
    if (str )
    {
        pos = (uint32)strtol( str, &pStopChar, 16 );
    }

    TiXmlElement const* valueIDElement = valueElement->FirstChildElement();
    while (valueIDElement)
    {
        str = valueIDElement->Value();
        if (str && !strcmp( str, "Label" ) )
        {
            ReadXMLVIDLabel(ccID, indexId, pos, valueIDElement, Language);
        }
        if (str && !strcmp( str, "Help" ) )
        {
            ReadXMLVIDHelp(ccID, indexId, pos, valueIDElement, Language);
        }
        valueIDElement = valueIDElement->NextSiblingElement();
    }
}

void Localization::ReadXMLVIDLabel(uint8 ccID, uint16 indexId, uint32 pos, const TiXmlElement *labelElement, const string Language) {

    uint64 key = GetValueKey(ccID, indexId, pos);
    if (m_valueLocalizationMap.find(key) == m_valueLocalizationMap.end()) {
        m_valueLocalizationMap[key] = new ValueLocalizationEntry(ccID, indexId, pos);
    }

    if( Language.empty() )
    {
        m_valueLocalizationMap[key]->AddLabel(labelElement->GetText());
    }
    else
    {
        m_valueLocalizationMap[key]->AddLabel(labelElement->GetText(), Language);
    }
}

void Localization::ReadXMLVIDHelp(uint8 ccID, uint16 indexId, uint32 pos, const TiXmlElement *labelElement, const string Language) {

    uint64 key = GetValueKey(ccID, indexId, pos);
    if (m_valueLocalizationMap.find(key) == m_valueLocalizationMap.end()) {
    	m_valueLocalizationMap[key] = new ValueLocalizationEntry(indexId, pos);
    }

    if( Language.empty() )
    {
        m_valueLocalizationMap[key]->AddHelp(labelElement->GetText());
    }
    else
    {
        m_valueLocalizationMap[key]->AddHelp(labelElement->GetText(), Language);
    }
}

uint64 Localization::GetValueKey
(
        uint8 _commandClass,
        uint16 _index,
        uint32 _pos
        )
{
    return ((uint64)_commandClass << 48) | ((uint64)_index << 32) | ((uint64)_pos);
}

void Localization::SetupValue
(
        Value *value
        )
{
    uint64 key = GetValueKey(value->GetID().GetCommandClassId(), value->GetID().GetIndex());
    if (m_valueLocalizationMap.find(key) != m_valueLocalizationMap.end()) {
        value->SetHelp(m_valueLocalizationMap[key]->GetHelpText(m_selectedLang));
        value->SetLabel(m_valueLocalizationMap[key]->GetLabelText(m_selectedLang));
    } else {
    	/* dont warn on Configuration CC */
    	if (value->GetID().GetCommandClassId() != Configuration::StaticGetCommandClassId()) Log::Write( LogLevel_Warning, "Localization::SetupValue: Localization Warning: No Entry for ValueID - CC: %d, Index: %d", value->GetID().GetCommandClassId(), value->GetID().GetIndex());
    }
    /* if its a bitset we need to set the help/label for each bit entry */
    if (value->GetID().GetType() == ValueID::ValueType_BitSet) {
        ValueBitSet *vbs = static_cast<ValueBitSet *>(value);
        uint8 size = vbs->GetSize();
        for (int i = 0; i < size; ++i) {
            key = GetValueKey(value->GetID().GetCommandClassId(), value->GetID().GetIndex(), i);
            if (m_valueLocalizationMap.find(key) != m_valueLocalizationMap.end()) {
                vbs->SetBitHelp(i, m_valueLocalizationMap[key]->GetHelpText(m_selectedLang));
                vbs->SetBitLabel(i, m_valueLocalizationMap[key]->GetLabelText(m_selectedLang));
            } else {
            	if (value->GetID().GetCommandClassId() != Configuration::StaticGetCommandClassId()) Log::Write( LogLevel_Warning, "Localization Warning: No Entry for ValueID - CC: %d, Index: %d, Pos %d", value->GetID().GetCommandClassId(), value->GetID().GetIndex(), i);
            }
        }
    }
}
void Localization::SetupCommandClass
(
        CommandClass *cc
        )
{
    uint8 ccID = cc->GetCommandClassId();
    if (m_commandClassLocalizationMap.find(ccID) != m_commandClassLocalizationMap.end()) {
        cc->SetCommandClassLabel(m_commandClassLocalizationMap[ccID]->GetLabel(m_selectedLang));
    } else {
        Log::Write( LogLevel_Warning, "Localization::SetupCommandClass: Localization Warning: No Entry for CommandClass - CC: %d (%s)", ccID, cc->GetCommandClassName().c_str());
        cc->SetCommandClassLabel(cc->GetCommandClassName());
    }
}



Localization *Localization::Get
(
        )
{
    if ( m_instance != NULL )
    {
        return m_instance;
    }
    m_instance = new Localization();
    ReadXML();
    Options::Get()->GetOptionAsString( "Language", &m_selectedLang );
    return m_instance;
}
