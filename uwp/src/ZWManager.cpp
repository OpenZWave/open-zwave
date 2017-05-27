#include "pch.h"
#include "ZWManager.h"

using namespace OpenZWaveUWP;
using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<ZWManager::Create>
//	Create the unmanaged Manager singleton object, and add a watcher
//-----------------------------------------------------------------------------
void ZWManager::Create
(
)
{
	// Create the Manager singleton
	Manager::Create();

	Manager::Get()->AddWatcher(OnNotificationFromUnmanaged, reinterpret_cast<void*>(this));
}

//-----------------------------------------------------------------------------
//	<ZWManager::OnNotificationFromUnmanaged>
//	Trigger an event from the unmanaged notification callback
//-----------------------------------------------------------------------------
void ZWManager::OnNotificationFromUnmanaged(Notification const* _notification, void* _context)
{
	ZWManager^ manager = reinterpret_cast<ZWManager^>(_context);
	ZWNotification^ notification = ref new ZWNotification((Notification *)_notification);
	manager->OnNotification(notification);
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueAsBool>
// Gets a value as a Bool
//-----------------------------------------------------------------------------
bool ZWManager::GetValueAsBool
(
	ZWValueID^ id,
	bool *o_value
)
{
	bool value;
	if (Manager::Get()->GetValueAsBool(id->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueAsByte>
// Gets a value as a Byte
//-----------------------------------------------------------------------------
bool ZWManager::GetValueAsByte
(
	ZWValueID^ id,
	byte *o_value
)
{
	uint8 value;
	if (Manager::Get()->GetValueAsByte(id->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueAsDecimal>
// Gets a value as a Decimal
//-----------------------------------------------------------------------------
/*
bool ZWManager::GetValueAsDecimal
(
	ZWValueID^ id,
	[Out] System::Decimal %o_value
)
{
	string value;
	if (Manager::Get()->GetValueAsString(id->CreateUnmanagedValueID(), &value))
	{
		String^ decimal = gcnew String(value.c_str());
		o_value = Decimal::Parse(decimal);
		return true;
	}
	return false;
}
*/

//-----------------------------------------------------------------------------
// <ZWManager::GetValueAsInt>
// Gets a value as an Int32
//-----------------------------------------------------------------------------
bool ZWManager::GetValueAsInt
(
	ZWValueID^ id,
	int32 *o_value
)
{
	int32 value;
	if (Manager::Get()->GetValueAsInt(id->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueAsShort>
// Gets a value as an Int16
//-----------------------------------------------------------------------------
bool ZWManager::GetValueAsShort
(
	ZWValueID^ id,
	int16 *o_value
)
{
	int16 value;
	if (Manager::Get()->GetValueAsShort(id->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueAsString>
// Gets a value as a String
//-----------------------------------------------------------------------------
bool ZWManager::GetValueAsString
(
	ZWValueID^ id,
	String^ *o_value
)
{
	string value;
	if (Manager::Get()->GetValueAsString(id->CreateUnmanagedValueID(), &value))
	{
		*o_value = ConvertString(value);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueListSelection>
// Gets the selected item from a list value (returning a string)
//-----------------------------------------------------------------------------
bool ZWManager::GetValueListSelection
(
	ZWValueID^ id,
	String^ *o_value
)
{
	string value;
	if (Manager::Get()->GetValueListSelection(id->CreateUnmanagedValueID(), &value))
	{
		*o_value = ConvertString(value);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueListSelection>
// Gets the selected item from a list value (returning the value)
//-----------------------------------------------------------------------------
bool ZWManager::GetValueListSelection
(
	ZWValueID^ id,
	int32 *o_value
)
{
	int32 value;
	if (Manager::Get()->GetValueListSelection(id->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueListItems>
// Gets the list of items from a list value
//-----------------------------------------------------------------------------
bool ZWManager::GetValueListItems
(
	ZWValueID^ id,
	Platform::Array<String^>^ *o_value
)
{
	vector<string> items;
	if (Manager::Get()->GetValueListItems(id->CreateUnmanagedValueID(), &items))
	{
		Platform::Array<String^>^ arr = ref new Platform::Array<String^>(items.size());
		for (uint32 i = 0; i<items.size(); ++i)
		{
			arr[i] = ConvertString(items[i]);
		}
		*o_value = arr;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetValueListValues>
// Gets the list of values from a list value
//-----------------------------------------------------------------------------
bool ZWManager::GetValueListValues
(
	ZWValueID^ id,
	Platform::Array<int>^ *o_value
)
{
	vector<int32> items;
	if (Manager::Get()->GetValueListValues(id->CreateUnmanagedValueID(), &items))
	{
		Platform::Array<int>^ arr = ref new Platform::Array<int>(items.size());
		for (uint32 i = 0; i<items.size(); ++i)
		{
			arr[i] = items[i];
		}
		*o_value = arr;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetNeighbors>
// Gets the neighbors for a node
//-----------------------------------------------------------------------------
uint32 ZWManager::GetNodeNeighbors
(
	uint32 homeId,
	uint8 nodeId,
	Platform::Array<byte>^ *o_neighbors
)
{
	uint8* neighbors;
	uint32 numNeighbors = Manager::Get()->GetNodeNeighbors(homeId, nodeId, &neighbors);
	if (numNeighbors)
	{
		
		Platform::Array<byte>^ arr = ref new Platform::Array<byte>(numNeighbors);;
		for (uint32 i = 0; i<numNeighbors; ++i)
		{
			arr[i] = neighbors[i];
		}
		*o_neighbors = arr;
		delete[] neighbors;
	}

	return numNeighbors;
}


bool ZWManager::GetNodeClassInformation
(
	uint32 homeId,
	uint8 nodeId,
	uint8 commandClassId,
	String^ *o_name,
	byte *o_version
)
{
	string value;
	uint8 version;
	if (Manager::Get()->GetNodeClassInformation(homeId, nodeId, commandClassId, &value, &version))
	{
		*o_name = ConvertString(value);
		*o_version = version;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetSwitchPoint>
// Get switchpoint data from the schedule
//-----------------------------------------------------------------------------
bool ZWManager::GetSwitchPoint
(
	ZWValueID^ id,
	uint8 idx,
	byte *o_hours,
	byte *o_minutes,
	byte *o_setback
)
{
	uint8 hours;
	uint8 minutes;
	int8 setback;
	if (Manager::Get()->GetSwitchPoint(id->CreateUnmanagedValueID(), idx, &hours, &minutes, &setback))
	{
		*o_hours = hours;
		*o_minutes = minutes;
		*o_setback = setback;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 ZWManager::GetAssociations
(
	uint32 homeId,
	uint8 nodeId,
	uint8 groupIdx,
	Platform::Array<byte>^ *o_associations
)
{
	uint8* associations;
	uint32 numAssociations = Manager::Get()->GetAssociations(homeId, nodeId, groupIdx, &associations);
	if (numAssociations)
	{
		Platform::Array<byte>^ arr = ref new Platform::Array<byte>(numAssociations);
		for (uint32 i = 0; i<numAssociations; ++i)
		{
			arr[i] = associations[i];
		}
		*o_associations = arr;
		delete[] associations;
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <ZWManager::GetAllScenes>
// Gets a list of all the SceneIds
//-----------------------------------------------------------------------------
uint8 ZWManager::GetAllScenes
(
	Platform::WriteOnlyArray<byte>^ o_sceneIds
)
{
	uint8* sceneIds;
	uint32 numScenes = Manager::Get()->GetAllScenes(&sceneIds);
	if (numScenes)
	{
		o_sceneIds = ref new Platform::Array<byte>(numScenes);
		for (uint32 i = 0; i<numScenes; ++i)
		{
			o_sceneIds[i] = sceneIds[i];
		}
		delete[] sceneIds;
	}

	return numScenes;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValues>
// Retrieves the scene's list of values
//-----------------------------------------------------------------------------
int ZWManager::SceneGetValues
(
	uint8 sceneId,
	Platform::WriteOnlyArray<ZWValueID^>^ o_values
)
{
	vector<ValueID> values;
	uint32 numValues = Manager::Get()->SceneGetValues(sceneId, &values);
	if (numValues)
	{
		o_values = ref new Platform::Array<ZWValueID^>(numValues);
		for (uint32 i = 0; i<numValues; ++i)
		{
			o_values[i] = ref new ZWValueID(values[i]);
		}
	}

	return numValues;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueAsBool>
// Retrieves a scene's value as a bool
//-----------------------------------------------------------------------------
bool ZWManager::SceneGetValueAsBool
(
	uint8 sceneId,
	ZWValueID^ valueId,
	bool *o_value
)
{
	bool value;
	if (Manager::Get()->SceneGetValueAsBool(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueAsByte>
// Retrieves a scene's value as an 8-bit unsigned integer
//-----------------------------------------------------------------------------
bool ZWManager::SceneGetValueAsByte
(
	uint8 sceneId,
	ZWValueID^ valueId,
	byte *o_value
)
{
	uint8 value;
	if (Manager::Get()->SceneGetValueAsByte(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueAsDecimal>
// Retrieves a scene's value as a decimal
//-----------------------------------------------------------------------------
/*
bool ZWManager::SceneGetValueAsDecimal
(
	uint8 sceneId,
	ZWValueID^ valueId,
	[Out] System::Decimal %o_value
)
{
	string value;
	if (Manager::Get()->SceneGetValueAsString(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		String^ decimal = gcnew String(value.c_str());
		o_value = Decimal::Parse(decimal);
		return true;
	}
	return false;
}
*/

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueAsInt>
// Retrieves a scene's value as a 32-bit signed integer
//-----------------------------------------------------------------------------
bool ZWManager::SceneGetValueAsInt
(
	uint8 sceneId,
	ZWValueID^ valueId,
	int *o_value
)
{
	int32 value;
	if (Manager::Get()->SceneGetValueAsInt(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueAsShort>
// Retrieves a scene's value as a 16-bit signed integer
//-----------------------------------------------------------------------------
bool ZWManager::SceneGetValueAsShort
(
	uint8 sceneId,
	ZWValueID^ valueId,
	int16 *o_value
)
{
	int16 value;
	if (Manager::Get()->SceneGetValueAsShort(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueAsString>
// Retrieves a scene's value as a string
//-----------------------------------------------------------------------------
bool ZWManager::SceneGetValueAsString
(
	uint8 sceneId,
	ZWValueID^ valueId,
	String^ *o_value
)
{
	string value;
	if (Manager::Get()->SceneGetValueAsString(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		*o_value = ConvertString(value);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueListSelection>
// Retrieves a scene's value in a list (as a string)
//-----------------------------------------------------------------------------
bool ZWManager::SceneGetValueListSelection
(
	uint8 sceneId,
	ZWValueID^ valueId,
	String^ *o_value
)
{
	string value;
	if (Manager::Get()->SceneGetValueListSelection(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		*o_value = ConvertString(value);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWManager::SceneGetValueListSelection>
// Retrieves a scene's value in a list (as a integer)
//-----------------------------------------------------------------------------
bool ZWManager::SceneGetValueListSelection
(
	uint8 sceneId,
	ZWValueID^ valueId,
	int *o_value
)
{
	int32 value;
	if (Manager::Get()->SceneGetValueListSelection(sceneId, valueId->CreateUnmanagedValueID(), &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}


