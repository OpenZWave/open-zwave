//-----------------------------------------------------------------------------
//
//	ValueIDIndexes.h
//
//	List of all Possible ValueID Indexes in OZW
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

#ifndef _ValueIDIndexes_H
#define _ValueIDIndexes_H

#include "Defs.h"
#include <cstring>

using namespace std;

namespace OpenZWave
{


/* this is good for upto 100 entries per ENUM. I shall predict that 100 entries shall be enough for any CommandClass :) */

#define MAP(macro, ...) \
    IDENTITY( \
        APPLY(CHOOSE_MAP_START, COUNT(__VA_ARGS__)) \
            (macro, __VA_ARGS__))

#define CHOOSE_MAP_START(count) MAP ## count

#define APPLY(macro, ...) IDENTITY(macro(__VA_ARGS__))

// Needed to expand __VA_ARGS__ "eagerly" on the MSVC preprocessor.
#define IDENTITY(x) x

#define MAP1(m, x)      m(x)
#define MAP2(m, x, ...) m(x) IDENTITY(MAP1(m, __VA_ARGS__))
#define MAP3(m, x, ...) m(x) IDENTITY(MAP2(m, __VA_ARGS__))
#define MAP4(m, x, ...) m(x) IDENTITY(MAP3(m, __VA_ARGS__))
#define MAP5(m, x, ...) m(x) IDENTITY(MAP4(m, __VA_ARGS__))
#define MAP6(m, x, ...) m(x) IDENTITY(MAP5(m, __VA_ARGS__))
#define MAP7(m, x, ...) m(x) IDENTITY(MAP6(m, __VA_ARGS__))
#define MAP8(m, x, ...) m(x) IDENTITY(MAP7(m, __VA_ARGS__))
#define MAP9(m, x, ...) m(x) IDENTITY(MAP8(m, __VA_ARGS__))
#define MAP10(m, x, ...) m(x) IDENTITY(MAP9(m, __VA_ARGS__))
#define MAP11(m, x, ...) m(x) IDENTITY(MAP10(m, __VA_ARGS__))
#define MAP12(m, x, ...) m(x) IDENTITY(MAP11(m, __VA_ARGS__))
#define MAP13(m, x, ...) m(x) IDENTITY(MAP12(m, __VA_ARGS__))
#define MAP14(m, x, ...) m(x) IDENTITY(MAP13(m, __VA_ARGS__))
#define MAP15(m, x, ...) m(x) IDENTITY(MAP14(m, __VA_ARGS__))
#define MAP16(m, x, ...) m(x) IDENTITY(MAP15(m, __VA_ARGS__))
#define MAP17(m, x, ...) m(x) IDENTITY(MAP16(m, __VA_ARGS__))
#define MAP18(m, x, ...) m(x) IDENTITY(MAP17(m, __VA_ARGS__))
#define MAP19(m, x, ...) m(x) IDENTITY(MAP18(m, __VA_ARGS__))
#define MAP20(m, x, ...) m(x) IDENTITY(MAP19(m, __VA_ARGS__))
#define MAP21(m, x, ...) m(x) IDENTITY(MAP20(m, __VA_ARGS__))
#define MAP22(m, x, ...) m(x) IDENTITY(MAP21(m, __VA_ARGS__))
#define MAP23(m, x, ...) m(x) IDENTITY(MAP22(m, __VA_ARGS__))
#define MAP24(m, x, ...) m(x) IDENTITY(MAP23(m, __VA_ARGS__))
#define MAP25(m, x, ...) m(x) IDENTITY(MAP24(m, __VA_ARGS__))
#define MAP26(m, x, ...) m(x) IDENTITY(MAP25(m, __VA_ARGS__))
#define MAP27(m, x, ...) m(x) IDENTITY(MAP26(m, __VA_ARGS__))
#define MAP28(m, x, ...) m(x) IDENTITY(MAP27(m, __VA_ARGS__))
#define MAP29(m, x, ...) m(x) IDENTITY(MAP28(m, __VA_ARGS__))
#define MAP30(m, x, ...) m(x) IDENTITY(MAP29(m, __VA_ARGS__))
#define MAP31(m, x, ...) m(x) IDENTITY(MAP30(m, __VA_ARGS__))
#define MAP32(m, x, ...) m(x) IDENTITY(MAP31(m, __VA_ARGS__))
#define MAP33(m, x, ...) m(x) IDENTITY(MAP32(m, __VA_ARGS__))
#define MAP34(m, x, ...) m(x) IDENTITY(MAP33(m, __VA_ARGS__))
#define MAP35(m, x, ...) m(x) IDENTITY(MAP34(m, __VA_ARGS__))
#define MAP36(m, x, ...) m(x) IDENTITY(MAP35(m, __VA_ARGS__))
#define MAP37(m, x, ...) m(x) IDENTITY(MAP36(m, __VA_ARGS__))
#define MAP38(m, x, ...) m(x) IDENTITY(MAP37(m, __VA_ARGS__))
#define MAP39(m, x, ...) m(x) IDENTITY(MAP38(m, __VA_ARGS__))
#define MAP40(m, x, ...) m(x) IDENTITY(MAP39(m, __VA_ARGS__))
#define MAP41(m, x, ...) m(x) IDENTITY(MAP40(m, __VA_ARGS__))
#define MAP42(m, x, ...) m(x) IDENTITY(MAP41(m, __VA_ARGS__))
#define MAP43(m, x, ...) m(x) IDENTITY(MAP42(m, __VA_ARGS__))
#define MAP44(m, x, ...) m(x) IDENTITY(MAP43(m, __VA_ARGS__))
#define MAP45(m, x, ...) m(x) IDENTITY(MAP44(m, __VA_ARGS__))
#define MAP46(m, x, ...) m(x) IDENTITY(MAP45(m, __VA_ARGS__))
#define MAP47(m, x, ...) m(x) IDENTITY(MAP46(m, __VA_ARGS__))
#define MAP48(m, x, ...) m(x) IDENTITY(MAP47(m, __VA_ARGS__))
#define MAP49(m, x, ...) m(x) IDENTITY(MAP48(m, __VA_ARGS__))
#define MAP50(m, x, ...) m(x) IDENTITY(MAP49(m, __VA_ARGS__))
#define MAP51(m, x, ...) m(x) IDENTITY(MAP50(m, __VA_ARGS__))
#define MAP52(m, x, ...) m(x) IDENTITY(MAP51(m, __VA_ARGS__))
#define MAP53(m, x, ...) m(x) IDENTITY(MAP52(m, __VA_ARGS__))
#define MAP54(m, x, ...) m(x) IDENTITY(MAP53(m, __VA_ARGS__))
#define MAP55(m, x, ...) m(x) IDENTITY(MAP54(m, __VA_ARGS__))
#define MAP56(m, x, ...) m(x) IDENTITY(MAP55(m, __VA_ARGS__))
#define MAP57(m, x, ...) m(x) IDENTITY(MAP56(m, __VA_ARGS__))
#define MAP58(m, x, ...) m(x) IDENTITY(MAP57(m, __VA_ARGS__))
#define MAP59(m, x, ...) m(x) IDENTITY(MAP58(m, __VA_ARGS__))
#define MAP60(m, x, ...) m(x) IDENTITY(MAP59(m, __VA_ARGS__))
#define MAP61(m, x, ...) m(x) IDENTITY(MAP60(m, __VA_ARGS__))
#define MAP62(m, x, ...) m(x) IDENTITY(MAP61(m, __VA_ARGS__))
#define MAP63(m, x, ...) m(x) IDENTITY(MAP62(m, __VA_ARGS__))
#define MAP64(m, x, ...) m(x) IDENTITY(MAP63(m, __VA_ARGS__))
#define MAP65(m, x, ...) m(x) IDENTITY(MAP64(m, __VA_ARGS__))
#define MAP66(m, x, ...) m(x) IDENTITY(MAP65(m, __VA_ARGS__))
#define MAP67(m, x, ...) m(x) IDENTITY(MAP66(m, __VA_ARGS__))
#define MAP68(m, x, ...) m(x) IDENTITY(MAP67(m, __VA_ARGS__))
#define MAP69(m, x, ...) m(x) IDENTITY(MAP68(m, __VA_ARGS__))
#define MAP70(m, x, ...) m(x) IDENTITY(MAP69(m, __VA_ARGS__))
#define MAP71(m, x, ...) m(x) IDENTITY(MAP70(m, __VA_ARGS__))
#define MAP72(m, x, ...) m(x) IDENTITY(MAP71(m, __VA_ARGS__))
#define MAP73(m, x, ...) m(x) IDENTITY(MAP72(m, __VA_ARGS__))
#define MAP74(m, x, ...) m(x) IDENTITY(MAP73(m, __VA_ARGS__))
#define MAP75(m, x, ...) m(x) IDENTITY(MAP74(m, __VA_ARGS__))
#define MAP76(m, x, ...) m(x) IDENTITY(MAP75(m, __VA_ARGS__))
#define MAP77(m, x, ...) m(x) IDENTITY(MAP76(m, __VA_ARGS__))
#define MAP78(m, x, ...) m(x) IDENTITY(MAP77(m, __VA_ARGS__))
#define MAP79(m, x, ...) m(x) IDENTITY(MAP78(m, __VA_ARGS__))
#define MAP80(m, x, ...) m(x) IDENTITY(MAP79(m, __VA_ARGS__))
#define MAP81(m, x, ...) m(x) IDENTITY(MAP80(m, __VA_ARGS__))
#define MAP82(m, x, ...) m(x) IDENTITY(MAP81(m, __VA_ARGS__))
#define MAP83(m, x, ...) m(x) IDENTITY(MAP82(m, __VA_ARGS__))
#define MAP84(m, x, ...) m(x) IDENTITY(MAP83(m, __VA_ARGS__))
#define MAP85(m, x, ...) m(x) IDENTITY(MAP84(m, __VA_ARGS__))
#define MAP86(m, x, ...) m(x) IDENTITY(MAP85(m, __VA_ARGS__))
#define MAP87(m, x, ...) m(x) IDENTITY(MAP86(m, __VA_ARGS__))
#define MAP88(m, x, ...) m(x) IDENTITY(MAP87(m, __VA_ARGS__))
#define MAP89(m, x, ...) m(x) IDENTITY(MAP88(m, __VA_ARGS__))
#define MAP90(m, x, ...) m(x) IDENTITY(MAP89(m, __VA_ARGS__))
#define MAP91(m, x, ...) m(x) IDENTITY(MAP90(m, __VA_ARGS__))
#define MAP92(m, x, ...) m(x) IDENTITY(MAP91(m, __VA_ARGS__))
#define MAP93(m, x, ...) m(x) IDENTITY(MAP92(m, __VA_ARGS__))
#define MAP94(m, x, ...) m(x) IDENTITY(MAP93(m, __VA_ARGS__))
#define MAP95(m, x, ...) m(x) IDENTITY(MAP94(m, __VA_ARGS__))
#define MAP96(m, x, ...) m(x) IDENTITY(MAP95(m, __VA_ARGS__))
#define MAP97(m, x, ...) m(x) IDENTITY(MAP96(m, __VA_ARGS__))
#define MAP98(m, x, ...) m(x) IDENTITY(MAP97(m, __VA_ARGS__))
#define MAP99(m, x, ...) m(x) IDENTITY(MAP98(m, __VA_ARGS__))
#define MAP100(m, x, ...) m(x) IDENTITY(MAP99(m, __VA_ARGS__))


#define EVALUATE_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
						_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
						_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
						_31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
						_41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
						_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
						_61, _62, _63, _64, _65, _66, _67, _68, _69, _70, \
						_71, _72, _73, _74, _75, _76, _77, _78, _79, _80, \
						_81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
						_91, _92, _93, _94, _95, _96, _97, _98, _99, _100, \
						count, ...) count

#define COUNT(...) \
    IDENTITY(EVALUATE_COUNT(__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, \
    									 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, \
    									 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, \
										 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, \
										 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, \
										 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, \
    									 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, \
										 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, \
    									 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
										 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))



struct ignore_assign {
    ignore_assign(int value) : _value(value) { }
    operator int() const { return _value; }

    const ignore_assign& operator =(int dummy) { return *this; }

    int _value;
};

#define IGNORE_ASSIGN_SINGLE(expression) (ignore_assign)expression,
#define IGNORE_ASSIGN(...) IDENTITY(MAP(IGNORE_ASSIGN_SINGLE, __VA_ARGS__))

#define STRINGIZE_SINGLE(expression) #expression,
#define STRINGIZE(...) IDENTITY(MAP(STRINGIZE_SINGLE, __VA_ARGS__))



#define ENUM(EnumName, ...)                                            \
struct EnumName {                                                      \
    enum _enumerated { __VA_ARGS__ };                                  \
                                                                       \
    _enumerated     _value;                                            \
                                                                       \
    EnumName(_enumerated value) : _value(value) { }                    \
    operator _enumerated() const { return _value; }                    \
                                                                       \
    const char* _to_string() const                                     \
    {                                                                  \
        for (size_t index = 0; index < _count; ++index) {              \
            if (_values()[index] == _value)                            \
                return _names()[index];                                \
        }                                                              \
                                                                       \
        return NULL;                                                   \
    }                                                                  \
                                                                       \
    static const size_t _count = IDENTITY(COUNT(__VA_ARGS__));         \
                                                                       \
    static const int* _values()                                        \
    {                                                                  \
        static const int values[] =                                    \
            { IDENTITY(IGNORE_ASSIGN(__VA_ARGS__)) };                  \
        return values;                                                 \
    }                                                                  \
                                                                       \
    static const char* const* _names()                                 \
    {                                                                  \
        static const char* const    raw_names[] =                      \
            { IDENTITY(STRINGIZE(__VA_ARGS__)) };                      \
                                                                       \
        static char*                processed_names[_count];           \
        static bool                 initialized = false;               \
                                                                       \
        if (!initialized) {                                            \
            for (size_t index = 0; index < _count; ++index) {          \
                size_t length =                                        \
                    std::strcspn(raw_names[index], " =\t\n\r");        \
                                                                       \
                processed_names[index] = new char[length + 1];         \
                                                                       \
					strncpy(                                           \
                    processed_names[index], raw_names[index], length); \
                processed_names[index][length] = '\0';                 \
            }                                                          \
        }                                                              \
                                                                       \
        return processed_names;                                        \
    }                                                                  \
};

ENUM(ValueID_Index_Alarm,
		Type_Start = 0,
		Type_End = 255,
		Type_ParamStart = 256,
		Type_ParamEnd = 511,
		Type_v1 = 512,
		Level_v1 = 513,
		AutoClearEvents = 514
	);
ENUM(ValueID_Index_AssociationCommandConfiguration,
		MaxCommandLength = 0,
		CommandsAreValues = 1,
		CommandsAreConfigurable = 2,
		NumFreeCommands = 3,
		MaxCommands = 4
	);
ENUM(ValueID_Index_BarrierOperator,
		Command = 0,
		Label = 1,
		SupportedSignals = 2,
		Audible = 3,
		Visual = 4
	);
ENUM(ValueID_Index_Basic,
		Set = 0
	);
ENUM(ValueID_Index_BasicWindowCovering,
		Open = 0,
		Close = 1
	);
ENUM(ValueID_Index_Battery,
		Level = 0
	);
ENUM(ValueID_Index_CentralScene,
		Start = 1,
		End = 255,
		SceneCount = 256,
		ClearSceneTimeout = 257
	);
ENUM(ValueID_Index_ClimateControlSchedule,
		DOW_Monday = 1,
		DOW_Tuesday = 2,
		DOW_Wednsday = 3,
		DOW_Thursday = 4,
		DOW_Friday = 5,
		DOW_Saturday = 6,
		DOW_Sunday = 7,
		OverrideState = 8,
		OverrideSetback = 9
	);
ENUM(ValueID_Index_Clock,
		Day = 0,
		Hour = 1,
		Minute = 2
	);
ENUM(ValueID_Index_Color,
		Color = 0,
		Index = 1,
		Channels_Capabilities = 2,
		Duration = 4
	);
ENUM(ValueID_Index_Configuration,
		Param_Start = 0,
		Param_End = 255
	);
ENUM(ValueID_Index_ControllerReplication,
		NodeId = 0,
		Function = 1,
		Replicate = 2
	);
ENUM(ValueID_Index_DoorLock,
		Lock = 0,
		Lock_Mode = 1,
		System_Config_Mode = 2,
		System_Config_Minutes = 3,
		System_Config_Seconds = 4,
		System_Config_OutsideHandles = 5,
		System_Config_InsideHandles = 6
	);
ENUM(ValueID_Index_DoorLockLogging,
		System_Config_MaxRecords = 0,
		GetRecordNo = 1,
		LogRecord = 2
	);
ENUM(ValueID_Index_EnergyProduction,
		Instant = 0,
		Total = 1,
		Today = 2,
		Time = 3
	);
ENUM(ValueID_Index_Indicator,
		Indicator = 0
	);
ENUM(ValueID_Index_Language,
		Language = 0,
		Country = 1
	);
ENUM(ValueID_Index_Lock,
		Locked = 0
	);
ENUM(ValueID_Index_ManufacturerProprietary,
		FibaroVenetianBlinds_Blinds = 0,
		FibaroVenetianBlinds_Tilt = 1
	);
ENUM(ValueID_Index_ManufacturerSpecific,
		LoadedConfig = 0,
		LocalConfig = 1,
		LatestConfig = 2,
		DeviceID = 3,
		SerialNumber = 4
	);
ENUM(ValueID_Index_Meter,
		Start = 0,
		End = 31,
		Exporting = 32,
		Reset = 33
	);
ENUM(ValueID_Index_MeterPulse,
		Count = 0
	);
ENUM(ValueID_Index_PowerLevel,
		Powerlevel = 0,
		Timeout = 1,
		Set = 2,
		TestNode = 3,
		TestPowerlevel = 4,
		TestFrames = 5,
		Test = 6,
		Report = 7,
		TestStatus = 8,
		TestAckFrames = 9
	);
ENUM(ValueID_Index_Protection,
		Protection = 0
	);
ENUM(ValueID_Index_SceneActivation,
		SceneID  = 0,
		Duration = 1
	);
ENUM(ValueID_Index_Security,
		Secured = 0
	);
ENUM(ValueID_Index_SensorAlarm,
		Start = 0,
		End = 255
	);
ENUM(ValueID_Index_SensorBinary,
		Sensor = 0,
		Start = 1,
		End = 255
	);
ENUM(ValueID_Index_SensorMultiLevel,
		Start = 0,
		End = 255
	);
ENUM(ValueID_Index_SimpleAV,
		Command = 0
	);
ENUM(ValueID_Index_SoundSwitch,
		Tone_Count = 0,
		Tones = 1,
		Volume = 2,
		Default_Tone = 3
	);
ENUM(ValueID_Index_SwitchAll,
		SwitchAll = 0
	);
ENUM(ValueID_Index_SwitchBinary,
		Level = 0,
		TargetState = 1,
		Duration = 2
	);
ENUM(ValueID_Index_SwitchMultiLevel,
		Level = 0,
		Bright = 1,
		Dim = 2,
		IgnoreStartLevel = 3,
		StartLevel = 4,
		Duration = 5,
		Step = 6,
		Inc = 7,
		Dec = 8,
		TargetValue = 9
	);
ENUM(ValueID_Index_SwitchToggleBinary,
		ToggleSwitch = 0
	);
ENUM(ValueID_Index_SwitchToggleMultilevel,
		Level = 0
	);
ENUM(ValueID_Index_ThermostatFanMode,
		FanMode = 0
	);
ENUM(ValueID_Index_ThermostatFanState,
		FanState = 0
	);
ENUM(ValueID_Index_ThermostatMode,
		Mode = 0
	);
ENUM(ValueID_Index_ThermostatOperatingState,
		OperatingState = 0
	);
ENUM(ValueID_Index_ThermostatSetpoint,
		Start = 0,
		End = 255
	);
ENUM(ValueID_Index_TimeParameters,
		Date = 0,
		Time = 1,
		Set = 2,
		Refresh = 3
	);
ENUM(ValueID_Index_UserCode,
		Start = 1,
		End = 254,
		Refresh = 255,
		RemoveCode = 256,
		Count = 257,
		RawValue = 258,
		RawValueIndex = 259
	);
ENUM(ValueID_Index_Version,
		Library = 0,
		Protocol = 1,
		Application = 2
	);
ENUM(ValueID_Index_WakeUp,
		Interval = 0,
		Min_Interval = 1,
		Max_Interval = 2,
		Default_Interval = 3,
		Interval_Step = 4
	);
ENUM(ValueID_Index_ZWavePlusInfo,
		Version = 0,
		InstallerIcon = 1,
		UserIcon = 2
	);
}
#endif
