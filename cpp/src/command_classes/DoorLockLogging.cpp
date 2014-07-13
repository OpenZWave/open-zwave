//-----------------------------------------------------------------------------
//
//	DoorLockLogging.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_DOOR_LOCK_LOGGING
//
//	Copyright (c) 2014 Justin Hammond <justin@dynam.ac>
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

#include "command_classes/CommandClasses.h"
#include "command_classes/DoorLockLogging.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueBool.h"
#include "value_classes/ValueByte.h"
#include "value_classes/ValueInt.h"

using namespace OpenZWave;

enum DoorLockLoggingCmd
{
	DoorLockLoggingCmd_RecordSupported_Get		= 0x01,
	DoorLockLoggingCmd_RecordSupported_Report	= 0x02,
	DoorLockLoggingCmd_Record_Get				= 0x03,
	DoorLockLoggingCmd_Record_Report			= 0x04
};

enum DoorLockEventType
{
	DoorLockEventType_LockCode						= 0x01,
	DoorLockEventType_UnLockCode					= 0x02,
	DoorLockEventType_LockButton					= 0x03,
	DoorLockEventType_UnLockButton  				= 0x04,
	DoorLockEventType_LockCodeOOSchedule			= 0x05,
	DoorLockEventType_UnLockCodeOOSchedule			= 0x06,
	DoorLockEventType_IllegalCode					= 0x07,
	DoorLockEventType_LockManual					= 0x08,
	DoorLockEventType_UnLockManual					= 0x09,
	DoorLockEventType_LockAuto						= 0x0A,
	DoorLockEventType_UnLockAuto					= 0x0B,
	DoorLockEventType_LockRemoteCode				= 0x0C,
	DoorLockEventType_UnLockRemoteCode				= 0x0D,
	DoorLockEventType_LockRemote					= 0x0E,
	DoorLockEventType_UnLockRemote					= 0x0F,
	DoorLockEventType_LockRemoteCodeOOSchedule		= 0x10,
	DoorLockEventType_UnLockRemoteCodeOOSchedule	= 0x11,
	DoorLockEventType_RemoteIllegalCode				= 0x12,
	DoorLockEventType_LockManual2					= 0x13,
	DoorLockEventType_UnlockManual2					= 0x14,
	DoorLockEventType_LockSecured					= 0x15,
	DoorLockEventType_LockUnsecured					= 0x16,
	DoorLockEventType_UserCodeAdded					= 0x17,
	DoorLockEventType_UserCodeDeleted				= 0x18,
	DoorLockEventType_AllUserCodesDeleted			= 0x19,
	DoorLockEventType_MasterCodeChanged				= 0x1A,
	DoorLockEventType_UserCodeChanged				= 0x1B,
	DoorLockEventType_LockReset						= 0x1C,
	DoorLockEventType_ConfigurationChanged			= 0x1D,
	DoorLockEventType_LowBattery					= 0x1E,
	DoorLockEventType_NewBattery					= 0x1F,
	DoorLockEventType_Max							= 0x20
};

static char const* c_DoorLockEventType[] =
{
	"Locked via Access Code",
	"Unlocked via Access Code",
	"Locked via Lock Button",
	"Unlocked via UnLock Botton",
	"Lock Attempt via Out of Schedule Access Code",
	"Unlock Attemp via Out of Schedule Access Code",
	"Illegal Access Code Entered",
	"Manually Locked",
	"Manually UnLocked",
	"Auto Locked",
	"Auto Unlocked",
	"Locked via Remote Out of Schedule Access Code",
	"Unlocked via Remote Out of Schedule Access Code",
	"Locked via Remote",
	"Unlocked via Remote",
	"Lock Attempt via Remote Out of Schedule Access Code",
	"Unlock Attempt via Remote Out of Schedule Access Code",
	"Illegal Remote Access Code",
	"Manually Locked (2)",
	"Manually Unlocked (2)",
	"Lock Secured",
	"Lock Unsecured",
	"User Code Added",
	"User Code Deleted",
	"All User Codes Deleted",
	"Master Code Changed",
	"User Code Changed",
	"Lock Reset",
	"Configuration Changed",
	"Low Battery",
	"New Battery Installed"
	"Unknown"
};


enum ValueIDSystemIndexes
{
	Value_System_Config_MaxRecords		= 0x00,		/* Simple On/Off Mode for Lock */
	Value_GetRecordNo					= 0x01
};



//-----------------------------------------------------------------------------
// <DoorLockLogging::DoorLockLogging>
// Constructor
//-----------------------------------------------------------------------------
DoorLockLogging::DoorLockLogging
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_MaxRecords(0),
	m_CurRecord(0)
{
	SetStaticRequest( StaticRequest_Values );
}

//-----------------------------------------------------------------------------
// <UserCode::ReadXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void DoorLockLogging::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	int32 intVal;

	CommandClass::ReadXML( _ccElement );
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "m_MaxRecords", &intVal ) )
	{
		m_MaxRecords = intVal;
	}
}

//-----------------------------------------------------------------------------
// <UserCode::WriteXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void DoorLockLogging::WriteXML
(
	TiXmlElement* _ccElement
)
{
	char str[32];

	CommandClass::WriteXML( _ccElement );
	snprintf( str, sizeof(str), "%d", m_MaxRecords );
	_ccElement->SetAttribute( "m_MaxRecords", str);
}



//-----------------------------------------------------------------------------
// <DoorLockLogging::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool DoorLockLogging::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests = RequestValue( _requestFlags, DoorLockLoggingCmd_RecordSupported_Get, _instance, _queue );
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		requests |= RequestValue( _requestFlags, DoorLockLoggingCmd_Record_Get, _instance, _queue );
	}

	return requests;
}




//-----------------------------------------------------------------------------
// <DoorLockLogging::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool DoorLockLogging::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _what,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( _what >= DoorLockLoggingCmd_RecordSupported_Get) {
		Msg* msg = new Msg( "DoorLockLoggingCmd_RecordSupported_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockLoggingCmd_RecordSupported_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;

	} else if (_what == DoorLockLoggingCmd_Record_Get) {
		Msg* msg = new Msg( "DoorLockLoggingCmd_Record_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockLoggingCmd_Record_Get );
		msg->Append( m_CurRecord );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// <DoorLockLogging::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool DoorLockLogging::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{

	if( DoorLockLoggingCmd_RecordSupported_Report == (DoorLockLoggingCmd)_data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received DoorLockLoggingCmd_RecordSupported_Report: Max Records is %d ", _data[1]);

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, Value_System_Config_MaxRecords ) ) )
		{
			value->OnValueRefreshed( _data[1]);
			value->Release();
		}
		ClearStaticRequest( StaticRequest_Values );
		return true;
	} else if (DoorLockLoggingCmd_Record_Report == (DoorLockLoggingCmd)_data[0] )
	{
		uint8 EventType = _data[9];
		if (EventType >= DoorLockEventType_Max)
			EventType = DoorLockEventType_Max;

		Log::Write (LogLevel_Info, GetNodeId(), "Recieved a DoorLockLogging Record %d which is \"%s\"", _data[1], c_DoorLockEventType[EventType]);

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, Value_GetRecordNo ) ) )
		{
			value->OnValueRefreshed( _data[1]);
			value->Release();
		}



	}
	return false;
}

//-----------------------------------------------------------------------------
// <DoorLockLogging::SetValue>
// Set the lock's state
//-----------------------------------------------------------------------------
bool DoorLockLogging::SetValue
(
	Value const& _value
)
{
	if( (Value_GetRecordNo == _value.GetID().GetIndex()) && ValueID::ValueType_Byte == _value.GetID().GetType() )
	{
		ValueByte const* value = static_cast<ValueByte const*>(&_value);

		Log::Write( LogLevel_Info, GetNodeId(), "DoorLockLoggingCmd_Record_Get - Requesting Log Record %d", value->GetValue() );
		Msg* msg = new Msg( "DoorLockLoggingCmd_Record_Get",  GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockLoggingCmd_Record_Get );
		msg->Append( value->GetValue() );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		m_CurRecord = value->GetValue();
		return true;
	}
	return false;
}



//-----------------------------------------------------------------------------
// <DoorLockLogging::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void DoorLockLogging::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
  		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, Value_System_Config_MaxRecords, "Max Number of Records", "", true, false, 0x0, 0 );
  		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, Value_GetRecordNo, "Current Record Number", "", false, false, 0x0, 0 );

	}
}


