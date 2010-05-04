//-----------------------------------------------------------------------------
//
//      OpenZwaveDLL.cpp
//
//      Contains the API for the public class members of
//      OpenZwave that this dll exposes.  These thin wrappers
//      are meant to expose the methods of the classes in a way
//      that .NET can access using p/invoke.
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
#include "OpenZwaveDll.h"
#include "Api_OpenZwaveDll.h"

#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "Value.h"
#include "ValueBool.h"
#include "ValueByte.h"
#include "ValueID.h"
#include "ValueDecimal.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueShort.h"
#include "ValueString.h"

#include "Log.h"

using namespace std;

using namespace OpenZWave;


//-------------- function needed for all DLLs.
//-------------- We do nothing special, just return TRUE

BOOL APIENTRY DllMain( HMODULE hModule,DWORD nReason,LPVOID lpReserved)
{
	return TRUE;
}
//-------------- exposed API functions

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave Manager Class
//-----------------------------------------------------------------------------
OPENZWAVEDLL_API OpenZWave::Manager* WINAPI OPENZWAVEDLL_Create(LPTSTR strConfigPath, LPTSTR strUserPath)
{
	return OpenZWave::Manager::Create( strConfigPath, strUserPath );
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_DisposeManager(OpenZWave::Manager* pManager)
{
	if(pManager != NULL)
	{
		pManager->Destroy();
		pManager = NULL;
	}
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_AddDriver(OpenZWave::Manager* pManager, LPTSTR strPort)
{
	return pManager->AddDriver(strPort);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_RemoveDriver(OpenZWave::Manager* pManager, LPTSTR strPort)
{
	return pManager->RemoveDriver(strPort);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_SetPollInterval(OpenZWave::Manager* pManager, int32 seconds)
{
	pManager->SetPollInterval(seconds);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_EnablePoll(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId)
{
	return pManager->EnablePoll(homeId, nodeId);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_DisablePoll(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId)
{
	return pManager->DisablePoll(homeId, nodeId);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_IsSlave(OpenZWave::Manager* pManager, uint32 homeId)
{
	return pManager->IsSlave(homeId);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_HasTimerSupport(OpenZWave::Manager* pManager, uint32 homeId)
{
	return pManager->HasTimerSupport(homeId);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_IsPrimaryController(OpenZWave::Manager* pManager, uint32 homeId)
{
	return pManager->IsPrimaryController(homeId);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_IsStaticUpdateController(OpenZWave::Manager* pManager, uint32 homeId)
{
	return pManager->IsStaticUpdateController(homeId);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_AddWatcher(OpenZWave::Manager* pManager, OpenZWave::Manager::pfnOnNotification_t notifyCB, LPVOID context)
{
	return pManager->AddWatcher(notifyCB, context);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_RemoveWatcher(OpenZWave::Manager* pManager, OpenZWave::Manager::pfnOnNotification_t notifyCB, LPVOID context)
{
	return pManager->RemoveWatcher(notifyCB, context);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_NotifyWatchers(OpenZWave::Manager* pManager, OpenZWave::Notification* pNotification)
{
	pManager->NotifyWatchers(pNotification);
}

OPENZWAVEDLL_API OpenZWave::ValueBool* WINAPI OPENZWAVEDLL_GetValueBoolPtr(OpenZWave::Manager* pManager,
				uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type)
{
	OpenZWave::ValueID vid(homeId, nodeId, genre, ccId, instance, valIdx, type);
	return pManager->GetValueBool(vid);
}

OPENZWAVEDLL_API OpenZWave::ValueByte* WINAPI OPENZWAVEDLL_GetValueBytePtr(OpenZWave::Manager* pManager,
				uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type)
{
	OpenZWave::ValueID vid(homeId, nodeId, genre, ccId, instance, valIdx, type);
	return pManager->GetValueByte(vid);
}

OPENZWAVEDLL_API OpenZWave::ValueDecimal* WINAPI OPENZWAVEDLL_GetValueDecimalPtr(OpenZWave::Manager* pManager,
				uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type)
{
	OpenZWave::ValueID vid(homeId, nodeId, genre, ccId, instance, valIdx, type);
	return pManager->GetValueDecimal(vid);
}

OPENZWAVEDLL_API OpenZWave::ValueInt* WINAPI OPENZWAVEDLL_GetValueIntPtr(OpenZWave::Manager* pManager,
				uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type)
{
	OpenZWave::ValueID vid(homeId, nodeId, genre, ccId, instance, valIdx, type);
	return pManager->GetValueInt(vid);
}

OPENZWAVEDLL_API OpenZWave::ValueList* WINAPI OPENZWAVEDLL_GetValueListPtr(OpenZWave::Manager* pManager,
				uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type)
{
	OpenZWave::ValueID vid(homeId, nodeId, genre, ccId, instance, valIdx, type);
	return pManager->GetValueList(vid);
}

OPENZWAVEDLL_API OpenZWave::ValueShort* WINAPI OPENZWAVEDLL_GetValueShortPtr(OpenZWave::Manager* pManager,
				uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type)
{
	OpenZWave::ValueID vid(homeId, nodeId, genre, ccId, instance, valIdx, type);
	return pManager->GetValueShort(vid);
}

OPENZWAVEDLL_API OpenZWave::ValueString* WINAPI OPENZWAVEDLL_GetValueStringPtr(OpenZWave::Manager* pManager, 
				uint32 homeId, uint8 nodeId, OpenZWave::ValueID::ValueGenre genre, uint8 ccId, uint8 instance, uint8 valIdx, OpenZWave::ValueID::ValueType type)
{
	OpenZWave::ValueID vid(homeId, nodeId, genre, ccId, instance, valIdx, type);
	return pManager->GetValueString(vid);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_ResetController(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->ResetController(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_SoftReset(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->SoftReset(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_RequestNodeNeighborUpdate(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId)
{
	pManager->RequestNodeNeighborUpdate(homeId, nodeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_AssignReturnRoute(OpenZWave::Manager* pManager, uint32 homeId, uint8 srcNodeId, uint8 dstNodeId)
{
	pManager->AssignReturnRoute(homeId, srcNodeId, dstNodeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginAddNode(OpenZWave::Manager* pManager, uint32 homeId, bool bHighPower)
{
	pManager->BeginAddNode(homeId, bHighPower);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginAddController(OpenZWave::Manager* pManager, uint32 homeId, bool bHighPower)
{
	pManager->BeginAddController(homeId, bHighPower);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_EndAddNode(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->EndAddNode(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginRemoveNode(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->BeginRemoveNode(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_EndRemoveNode(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->EndRemoveNode(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_BeginReplicateController(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->BeginReplicateController(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_EndReplicateController(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->EndReplicateController(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_RequestNetworkUpdate(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->RequestNetworkUpdate(homeId);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_ControllerChange(OpenZWave::Manager* pManager, uint32 homeId)
{
	pManager->ControllerChange(homeId);
}
OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_RequestState(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId)
{
	pManager->RequestState(homeId, nodeId);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_RefreshNodeInfo(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId)
{
	return pManager->RefreshNodeInfo(homeId, nodeId);
}

//OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_ReadMemory(OpenZWave::Manager* pManager, uint32 homeId, uint16 offset)
//{
//	pManager->ReadMemory(homeId, offset);
//}
//
//OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_SetConfiguration(OpenZWave::Manager* pManager, uint32 homeId, uint8 nodeId, uint8 parameter, uint32 value)
//{
//	pManager->SetConfiguration(homeId, nodeId, parameter, value);
//}


//-----------------------------------------------------------------------------
// Public methods from the OpenZWave Notification Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetNotifyType(OpenZWave::Notification* pNotify)
{
	return (uint8) pNotify->GetType();
}
OPENZWAVEDLL_API uint32 WINAPI OPENZWAVEDLL_GetHomeIdFromNotify(OpenZWave::Notification* pNotify)
{
	return pNotify->GetHomeId();
}
OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetNodeIdFromNotify(OpenZWave::Notification* pNotify)
{
	return pNotify->GetNodeId();
}
OPENZWAVEDLL_API OpenZWave::ValueID const& WINAPI OPENZWAVEDLL_GetValueID(OpenZWave::Notification* pNotify)
{
	return pNotify->GetValueID();
}
OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetGroupIdx(OpenZWave::Notification* pNotify)
{
	return pNotify->GetGroupIdx();
}
OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetStatus(OpenZWave::Notification* pNotify)
{
	return pNotify->GetStatus();
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueID Class
//-----------------------------------------------------------------------------
OPENZWAVEDLL_API uint32 WINAPI OPENZWAVEDLL_GetHomeId(OpenZWave::ValueID* pValueId)
{
	return pValueId->GetHomeId();
}

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetNodeId(OpenZWave::ValueID* pValueId)
{
	return pValueId->GetNodeId();
}

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetGenre(OpenZWave::ValueID* pValueId)
{
	return (uint8) pValueId->GetGenre();
}
OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetCommandClassId(OpenZWave::ValueID* pValueId)
{
	return pValueId->GetCommandClassId();
}

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetInstance(OpenZWave::ValueID* pValueId)
{
	return pValueId->GetInstance();
}

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetIndex(OpenZWave::ValueID* pValueId)
{
	return pValueId->GetIndex();
}

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetType(OpenZWave::ValueID* pValueId)
{
	return (uint8) pValueId->GetType();
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueBool Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetBool(OpenZWave::ValueBool* pValue, bool setState)
{
	return pValue->Set(setState);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedBool(OpenZWave::ValueBool* pValue, bool setState)
{
	pValue->OnValueChanged(setState);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringBool(OpenZWave::ValueBool* pValue, /*out*/ char* strBool)
{
	string strTmp = pValue->GetAsString();
	strncpy(strBool, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_GetValueBool(OpenZWave::ValueBool* pValue)
{
    return pValue->GetValue();
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_GetPendingBool(OpenZWave::ValueBool* pValue)
{
    return pValue->GetPending();
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueByte Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetByte(OpenZWave::ValueByte* pValue, uint8 val)
{
	return pValue->Set(val);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedByte(OpenZWave::ValueByte* pValue, uint8 val)
{
	pValue->OnValueChanged(val);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringByte(OpenZWave::ValueByte* pValue, /*out*/ char* strName)
{
	string strTmp = pValue->GetAsString();
	strncpy(strName, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetValueByte(OpenZWave::ValueByte* pValue)
{
    return pValue->GetValue();
}

OPENZWAVEDLL_API uint8 WINAPI OPENZWAVEDLL_GetPendingByte(OpenZWave::ValueByte* pValue)
{
    return pValue->GetPending();
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueShort Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetShort(OpenZWave::ValueShort* pValue, uint16 val)
{
	return pValue->Set(val);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedShort(OpenZWave::ValueShort* pValue, uint16 val)
{
	pValue->OnValueChanged(val);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringShort(OpenZWave::ValueShort* pValue, /*out*/ char* strValue)
{
	string strTmp = pValue->GetAsString();
	strncpy(strValue, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API uint16 WINAPI OPENZWAVEDLL_GetValueShort(OpenZWave::ValueShort* pValue)
{
    return pValue->GetValue();
}

OPENZWAVEDLL_API uint16 WINAPI OPENZWAVEDLL_GetPendingShort(OpenZWave::ValueShort* pValue)
{
    return pValue->GetPending();
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueInt Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetInt(OpenZWave::ValueInt* pValue, int32 val)
{
	return pValue->Set(val);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedInt(OpenZWave::ValueInt* pValue, int32 val)
{
	pValue->OnValueChanged(val);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringInt(OpenZWave::ValueInt* pValue, /*out*/ char* strName)
{
	string strTmp = pValue->GetAsString();
	strncpy(strName, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetValueInt(OpenZWave::ValueInt* pValue)
{
    return pValue->GetValue();
}

OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetPendingInt(OpenZWave::ValueInt* pValue)
{
    return pValue->GetPending();
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueDecimal Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetDecimal(OpenZWave::ValueDecimal* pValue, char* val)
{
	return pValue->Set(string(val));
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedDecimal(OpenZWave::ValueDecimal* pValue, char* val)
{
	pValue->OnValueChanged(string(val));
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringDecimal(OpenZWave::ValueDecimal* pValue, /*out*/ char* strName)
{
	string strTmp = pValue->GetAsString();
	strncpy(strName, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetValueDecimal(OpenZWave::ValueDecimal* pValue, /*out*/ char* strVal)
{
	string strTmp = pValue->GetValue();
	strncpy(strVal, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetPendingDecimal(OpenZWave::ValueDecimal* pValue, /*out*/ char* strVal)
{
	string strTmp = pValue->GetPending();
	strncpy(strVal, strTmp.c_str(), strTmp.length()+1);
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueString Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetString(OpenZWave::ValueString* pValue, char* val)
{
	return pValue->Set(string(val));
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedString(OpenZWave::ValueString* pValue, char* val)
{
	pValue->OnValueChanged(string(val));
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringString(OpenZWave::ValueString* pValue, /*out*/ char* strName)
{
	string strTmp = pValue->GetAsString();
	strncpy(strName, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetValueString(OpenZWave::ValueString* pValue, /*out*/ char* strVal)
{
	string strTmp = pValue->GetValue();
	strncpy(strVal, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetPendingString(OpenZWave::ValueString* pValue, /*out*/ char* strVal)
{
	string strTmp = pValue->GetPending();
	strncpy(strVal, strTmp.c_str(), strTmp.length()+1);
}

//-----------------------------------------------------------------------------
// Public methods from the OpenZWave ValueList Class
//-----------------------------------------------------------------------------

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetListByLabel(OpenZWave::ValueList* pValue, char* val)
{
	return pValue->SetByLabel(string(val));
}

OPENZWAVEDLL_API BOOL WINAPI OPENZWAVEDLL_SetListByValue(OpenZWave::ValueList* pValue, int32 val)
{
	return pValue->SetByValue(val);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_OnValueChangedList(OpenZWave::ValueList* pValue, int32 valueIdx)
{
	pValue->OnValueChanged(valueIdx);
}

OPENZWAVEDLL_API void WINAPI OPENZWAVEDLL_GetAsStringList(OpenZWave::ValueList* pValue, /*out*/ char* strName)
{
	string strTmp = pValue->GetAsString();
	strncpy(strName, strTmp.c_str(), strTmp.length()+1);
}

OPENZWAVEDLL_API ITEM WINAPI OPENZWAVEDLL_GetValueListItem(OpenZWave::ValueList* pValue)
{
	ITEM retItem;
	ValueList::Item item = pValue->GetItem();
	retItem.m_value = item.m_value;
	strncpy(retItem.m_label, item.m_label.c_str(), item.m_label.length()+1);
	return retItem;
}

OPENZWAVEDLL_API ITEM WINAPI OPENZWAVEDLL_GetPendingListItem(OpenZWave::ValueList* pValue)
{
	ITEM retItem;
	ValueList::Item item = pValue->GetPending();
	retItem.m_value = item.m_value;
	strncpy(retItem.m_label, item.m_label.c_str(), item.m_label.length()+1);
	return retItem;
}

OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetItemIdxByLabel(OpenZWave::ValueList* pValue, char* strVal)
{
	return pValue->GetItemIdxByLabel(string(strVal));
}

OPENZWAVEDLL_API int32 WINAPI OPENZWAVEDLL_GetItemIdxByValue(OpenZWave::ValueList* pValue, int32 val)
{
	return pValue->GetItemIdxByValue(val);
}
