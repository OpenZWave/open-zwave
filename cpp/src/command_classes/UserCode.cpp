//-----------------------------------------------------------------------------
//
//	UserCode.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_USER_CODE
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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

#include "tinyxml.h"
#include "CommandClasses.h"
#include "UserCode.h"
#include "Node.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueString.h"

using namespace OpenZWave;

enum UserCodeCmd
{
	UserCodeCmd_Set			= 0x01,
	UserCodeCmd_Get			= 0x02,
	UserCodeCmd_Report		= 0x03,
	UserNumberCmd_Get		= 0x04,
	UserNumberCmd_Report		= 0x05
};

enum
{
	UserCodeIndex_Unknown		= 0,
	UserCodeIndex_Count		= 255
};

//-----------------------------------------------------------------------------
// <UserCode::UserCode>
// Constructor
//-----------------------------------------------------------------------------
UserCode::UserCode
( 
	uint32 const _homeId,
	uint8 const _nodeId 
):
	CommandClass( _homeId, _nodeId ),
	m_queryAll( false ),
	m_userCodeCount( 0 )
{
	SetStaticRequest( StaticRequest_Values );
	memset( m_userCodesStatus, 0xff, sizeof(m_userCodesStatus) );
}

//-----------------------------------------------------------------------------
// <UserCode::ReadXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void UserCode::ReadXML
( 
	TiXmlElement const* _ccElement
)
{
	int32 intVal;

	CommandClass::ReadXML( _ccElement );
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "codes", &intVal ) )
	{
		m_userCodeCount = intVal;
	}
}

//-----------------------------------------------------------------------------
// <UserCode::WriteXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void UserCode::WriteXML
( 
	TiXmlElement* _ccElement
)
{
	char str[32];

	CommandClass::WriteXML( _ccElement );
	snprintf( str, sizeof(str), "%d", m_userCodeCount );
	_ccElement->SetAttribute( "codes", str);
}

//-----------------------------------------------------------------------------
// <UserCode::RequestState>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
bool UserCode::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests |= RequestValue( _requestFlags, 0xff, _instance, _queue );
	}

	if( _requestFlags & RequestFlag_Session )
	{
		if( m_userCodeCount > 0 )
		{
			m_queryAll = true;
			requests |= RequestValue( _requestFlags, 1, _instance, _queue );
		}
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <UserCode::RequestValue>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
bool UserCode::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _userCodeIdx,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}

	if( _userCodeIdx == 0xff )
	{
		// Get number of supported user codes.
		Msg* msg = new Msg( "UserNumberCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( UserNumberCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	}

	Msg* msg = new Msg( "UserCodeCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( UserCodeCmd_Get );
	msg->Append( _userCodeIdx );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <UserCode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool UserCode::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	char str[16];

	if( UserNumberCmd_Report == (UserCodeCmd)_data[0] )
	{	
		m_userCodeCount = _data[1];
		if( m_userCodeCount > 254 )
		{
			// Make space for code count.
			m_userCodeCount = 254;
		}
		ClearStaticRequest( StaticRequest_Values );
		if( m_userCodeCount == 0 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Not supported", GetNodeId() );
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Supported Codes %d (%d)", GetNodeId(), m_userCodeCount, _data[1] );
		}

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, UserCodeIndex_Count ) ) )
		{
			value->OnValueRefreshed( m_userCodeCount );
			value->Release();
		}

		if( Node* node = GetNodeUnsafe() )
		{
			for( uint8 i = 0; i <= m_userCodeCount; i++ )
			{
				if( i == 0 )
				{
					snprintf( str, sizeof(str), "Unknown Code" );
				}
				else
				{
					snprintf( str, sizeof(str), "Code %d", i );
				}
				node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, i, str, "", false, false, "", 0 );
			}
		}
		return true;
	}
	else if( UserCodeCmd_Report == (UserCodeCmd)_data[0] )
	{	
		int i = _data[1];
		if( ValueString* value = static_cast<ValueString*>( GetValue( _instance, i ) ) )
		{
			uint8 size = _length - 3;
			m_userCodesStatus[i] = _data[2];
			memcpy( str, &_data[3], size );
			value->OnValueRefreshed( str );
			value->Release();
		}
		Log::Write( LogLevel_Info, GetNodeId(), "Received User Code Report from node %d for User Code %d (%s)", GetNodeId(), i, CodeStatus( _data[2] ).c_str() );
		if( m_queryAll )
		{
			if( ++i <= m_userCodeCount )
			{
				RequestValue( 0, i, _instance, Driver::MsgQueue_Query );
			}
			else
			{
				m_queryAll = false;
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <UserCode::SetValue>
// Set a User Code value
//-----------------------------------------------------------------------------
bool UserCode::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_String == _value.GetID().GetType() )
	{
		ValueString const* value = static_cast<ValueString const*>(&_value);
		string s = value->GetValue();
		uint8 len = s.length();

		if( len > 10 )
		{
			len = 10;
		}
		else if (len < 4)
		{
			return false;
		}
		m_userCodesStatus[value->GetID().GetIndex()] = UserCode_Occupied;
		Msg* msg = new Msg( "Set UserCode", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 4 + len );
		msg->Append( GetCommandClassId() );
		msg->Append( UserCodeCmd_Set );
		msg->Append( value->GetID().GetIndex() );
		msg->Append( UserCode_Occupied );
		for( uint8 i = 0; i < len; i++ )
		{
			msg->Append( s[i] );
		}
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <UserCode::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void UserCode::CreateVars
(
	uint8 const _instance

)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, UserCodeIndex_Count, "Code Count", "", true, false, 0, 0 );
	}
}
