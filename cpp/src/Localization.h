//-----------------------------------------------------------------------------
//
//	Localization.h
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

#ifndef VALUEHELP_H
#define VALUEHELP_H

#include <cstdio>
#include <string>
#include <map>
#include "Defs.h"
#include "Driver.h"
#include "command_classes/CommandClass.h"

namespace OpenZWave
{

class LabelLocalizationEntry : public Ref
{
public:
    LabelLocalizationEntry (uint16 _index, uint32 _pos = -1);
    ~LabelLocalizationEntry() {}
    void AddLabel(string label, string lang = "");
    uint64 GetIdx();
    string GetLabel(string lang);

private:
    uint8 m_index;
    uint32 m_pos;
    map<string, string> m_Label;
    string m_defaultLabel;
};


class ValueLocalizationEntry : public Ref
{
public:
    ValueLocalizationEntry ( uint8 _commandClass, uint16 _index, uint32 _pos = -1 );
    ~ValueLocalizationEntry() {

    }
    uint64 GetIdx();
    string GetHelpText(string lang);
    void AddHelp(string HelpText, string lang = "");
    string GetLabelText(string lang);
    void AddLabel(string Label, string lang = "");

private:
    uint8 m_commandClass;
    uint16 m_index;
    uint32 m_pos;
    map<string, string> m_HelpText;
    map<string, string> m_LabelText;
    string m_DefaultHelpText;
    string m_DefaultLabelText;
};



class Localization
{
    //-----------------------------------------------------------------------------
    // Construction
    //-----------------------------------------------------------------------------
private:
    Localization();
    ~Localization();

    static void ReadXML();
    static void ReadXMLLabel(uint8 ccID, const TiXmlElement *labelElement, const string Language);
    static void ReadXMLValue(uint8 ccID, const TiXmlElement *valueElement, const string Language);
    static void ReadXMLVIDLabel(uint8 ccID, uint16 indexId, uint32 pos, const TiXmlElement *labelElement, const string Language);
    static void ReadXMLVIDHelp(uint8 ccID, uint16 indexId, uint32 pos, const TiXmlElement *helpElement, const string Language);
    static uint64 GetValueKey (uint8 _commandClass, uint16 _index, uint32 _pos = -1);
public:
    static Localization* Get();
    void SetupValue(Value *value);
    void SetupCommandClass(CommandClass *cc);
    string GetSelectedLang() { return Localization::m_selectedLang;};
    //-----------------------------------------------------------------------------
    // Instance Functions
    //-----------------------------------------------------------------------------
private:
    static Localization* m_instance;
    static map<int64,ValueLocalizationEntry*> m_valueLocalizationMap;
    static map<uint8,LabelLocalizationEntry*> m_commandClassLocalizationMap;
    static string m_selectedLang;


};

};
#endif // VALUEHELP_H
