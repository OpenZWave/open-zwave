//-----------------------------------------------------------------------------
//
//      Api_OPENZWAVELL.h
//
//      The header file of the API for the public class members of
//      OpenZwave that this dll exposes.
//
//      Copyright (c) 2010 Doug Brown <djbrown2001@gmail.com>
//
//
//      SOFTWARE NOTICE AND LICENSE
//
//      This file is part of OpenZWave.
//
//      OpenZWave is free software: you can redistribute it and/or modify
//      it under the terms of the GNU Lesser General Public License as published
//      by the Free Software Foundation, either version 3 of the License,
//      or (at your option) any later version.
//
//      OpenZWave is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public License
//      along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#pragma once

#if defined OPENZWAVEDLL_EXPORTS
#define OPENZWAVEDLL_API __declspec(dllexport)
#pragma message( "==============> compiling the OpenZwaveDLL DLL (for export)" )
#else
#define OPENZWAVEDLL_API __declspec(dllimport)
#endif

#include "Manager.h"
#include "Driver.h"
#include "Value.h"
#include "Notification.h"

// OpenZWave Manager methods
//
extern "C" OPENZWAVEDLL_API OpenZWave::Manager* WINAPI OPENZWAVEDLL_Create(LPTSTR strConfigPath, LPTSTR strUserPath);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_DisposeManager(OpenZWave::Manager* pManager);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_AddDriver(OpenZWave::Manager* pManager, LPTSTR strPort);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_RemoveDriver(OpenZWave::Manager* pManager, LPTSTR strPort);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_SetPollInterval(OpenZWave::Manager* pManager, int32 seconds);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_EnablePoll(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_DisablePoll(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_IsSlave(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_HasTimerSupport(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_IsPrimaryController(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_IsStaticUpdateController(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_AddWatcher(OpenZWave::Manager* pManager, OpenZWave::Manager::pfnOnNotification_t notifyCB, LPVOID context);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_RemoveWatcher(OpenZWave::Manager* pManager, OpenZWave::Manager::pfnOnNotification_t notifyCB, LPVOID context);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_NotifyWatchers(OpenZWave::Manager* pManager, OpenZWave::Notification* pNotification);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_ResetController(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_SoftReset(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_RequestNodeNeighborUpdate(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_AssignReturnRoute(OpenZWave::Manager* pManager, uint32 homeId, uint8 srcNodeId, uint8 dstNodeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginAddNode(OpenZWave::Manager* pManager, uint32 homeId, bool bHighPower);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginAddController(OpenZWave::Manager* pManager, uint32 homeId, bool bHighPower);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_EndAddNode(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginRemoveNode(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_EndRemoveNode(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginReplicateController(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_EndReplicateController(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_RequestNetworkUpdate(OpenZWave::Manager* pManager, uint32 homeId);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_ControllerChange(OpenZWave::Manager* pManager, uint32 homeId);
//extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_ReadMemory(OpenZWave::Manager* pManager, uint32 homeId, uint16 offset);
//extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_SetConfiguration(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId, uint8 parameter, uint32 value);
//typedef struct ValIdStruct
//{
//	uint32 homeId;
//	uint8 nodeId;
//	OpenZWave::ValueID::ValueGenre genre;
//	uint8 commandClassId;
//	uint8 instance;
//	uint8 valueIndex;
//	OpenZWave::ValueID::ValueType type;
//} VALID;

extern "C" OPENZWAVEDLL_API OpenZWave::ValueBool* WINAPI OPENZWAVEDLL_GetValueBoolPtr(OpenZWave::Manager* pManager,
			uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type);
extern "C" OPENZWAVEDLL_API OpenZWave::ValueByte* WINAPI OPENZWAVEDLL_GetValueBytePtr(OpenZWave::Manager* pManager,
			uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type);
extern "C" OPENZWAVEDLL_API OpenZWave::ValueDecimal* WINAPI OPENZWAVEDLL_GetValueDecimalPtr(OpenZWave::Manager* pManager,
			uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type);
extern "C" OPENZWAVEDLL_API OpenZWave::ValueInt* WINAPI OPENZWAVEDLL_GetValueIntPtr(OpenZWave::Manager* pManager,
			uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type);
extern "C" OPENZWAVEDLL_API OpenZWave::ValueList* WINAPI OPENZWAVEDLL_GetValueListPtr(OpenZWave::Manager* pManager,
			uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type);
extern "C" OPENZWAVEDLL_API OpenZWave::ValueShort* WINAPI OPENZWAVEDLL_GetValueShortPtr(OpenZWave::Manager* pManager,
			uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type);
extern "C" OPENZWAVEDLL_API OpenZWave::ValueString* WINAPI OPENZWAVEDLL_GetValueStringPtr(OpenZWave::Manager* pManager,
			uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type);

// OpenZWave Notification methods
//
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetNotifyType(OpenZWave::Notification* pNotify);
extern "C" OPENZWAVEDLL_API uint32 WINAPI OPENZWAVEDLL_GetHomeIdFromNotify(OpenZWave::Notification* pNotify);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetNodeIdFromNotify(OpenZWave::Notification* pNotify);
extern "C" OPENZWAVEDLL_API OpenZWave::ValueID const& WINAPI OPENZWAVEDLL_GetValueID(OpenZWave::Notification* pNotify);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetGroupIdx(OpenZWave::Notification* pNotify);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetStatus(OpenZWave::Notification* pNotify);

// OpenZWave ValueID Accessor methods
//
extern "C" OPENZWAVEDLL_API uint32 WINAPI OPENZWAVEDLL_GetHomeId(OpenZWave::ValueID* pValueId);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetNodeId(OpenZWave::ValueID* pValueId);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetGenre(OpenZWave::ValueID* pValueId);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetCommandClassId(OpenZWave::ValueID* pValueId);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetInstance(OpenZWave::ValueID* pValueId);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetIndex(OpenZWave::ValueID* pValueId);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetType(OpenZWave::ValueID* pValueId);

// OpenZWave ValueBool methods
//
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetBool(OpenZWave::ValueBool* pValue, bool setState);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedBool(OpenZWave::ValueBool* pValue, bool setState);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringBool(OpenZWave::ValueBool* pValue, char* strBool);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_GetValueBool(OpenZWave::ValueBool* pValue);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_GetPendingBool(OpenZWave::ValueBool* pValue);

// OpenZWave ValueDecimal methods
//
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetDecimal(OpenZWave::ValueDecimal* pValue, char* strValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedDecimal(OpenZWave::ValueDecimal* pValue, char* strValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringDecimal(OpenZWave::ValueDecimal* pValue, char* strValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetValueDecimal(OpenZWave::ValueDecimal* pValue, char* strValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetPendingDecimal(OpenZWave::ValueDecimal* pValue, char* strValue);

// OpenZWave ValueByte methods
//
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetByte(OpenZWave::ValueByte* pValue, uint8 setValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedByte(OpenZWave::ValueByte* pValue, uint8 setValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringByte(OpenZWave::ValueByte* pValue, char* strByte);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetValueByte(OpenZWave::ValueByte* pValue);
extern "C" OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetPendingByte(OpenZWave::ValueByte* pValue);

// OpenZWave ValueInt methods
//
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetInt(OpenZWave::ValueInt* pValue, int32 setValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedInt(OpenZWave::ValueInt* pValue, int32 setValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringInt(OpenZWave::ValueInt* pValue, char* strValue);
extern "C" OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetValueInt(OpenZWave::ValueInt* pValue);
extern "C" OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetPendingInt(OpenZWave::ValueInt* pValue);

// OpenZWave ValueShort methods
//
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetShort(OpenZWave::ValueShort* pValue, uint16 setValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedShort(OpenZWave::ValueShort* pValue, uint16 setValue);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringShort(OpenZWave::ValueShort* pValue, char* strValue);
extern "C" OPENZWAVEDLL_API uint16 WINAPI OPENZWAVEDLL_GetValueShort(OpenZWave::ValueShort* pValue);
extern "C" OPENZWAVEDLL_API uint16 WINAPI OPENZWAVEDLL_GetPendingShort(OpenZWave::ValueShort* pValue);

// OpenZWave ValueString methods
//
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetString(OpenZWave::ValueString* pValue, char* str);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedString(OpenZWave::ValueString* pValue, char* str);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringString(OpenZWave::ValueString* pValue, char* str);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetValueString(OpenZWave::ValueString* pValue, char* str);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetPendingString(OpenZWave::ValueString* pValue, char* str);

// OpenZWave ValueList methods
//
typedef struct ItemStruct
{
	char	    m_label[128];
	int32		m_value;
} ITEM;

extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetListByLabel(OpenZWave::ValueList* pValue, char* str);
extern "C" OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetListByValue(OpenZWave::ValueList* pValue, int32 val);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedList(OpenZWave::ValueList* pValue, int32 valueIdx);
extern "C" OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringList(OpenZWave::ValueList* pValue, char* str);
extern "C" OPENZWAVEDLL_API ITEM WINAPI OPENZWAVEDLL_GetValueListItem(OpenZWave::ValueList* pValue);
extern "C" OPENZWAVEDLL_API ITEM WINAPI OPENZWAVEDLL_GetPendingListItem(OpenZWave::ValueList* pValue);
extern "C" OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetItemIdxByLabel(OpenZWave::ValueList* pValue, char* str);
extern "C" OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetItemIdxByValue(OpenZWave::ValueList* pValue, int32 val);

