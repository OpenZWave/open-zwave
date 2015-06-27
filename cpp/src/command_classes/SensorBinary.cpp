//-----------------------------------------------------------------------------
//
//	SensorBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_BINARY
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

#include "command_classes/CommandClasses.h"
#include "command_classes/SensorBinary.h"
#include "command_classes/WakeUp.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include "value_classes/ValueBool.h"
#include "tinyxml.h"

using namespace OpenZWave;

enum SensorBinaryCmd
{
	SensorBinaryCmd_Get		= 0x02,
	SensorBinaryCmd_Report		= 0x03,

	// Version 2
	SensorBinaryCmd_SupportedSensorGet = 0x01,
	SensorBinaryCmd_SupportedSensorReport = 0x04
};

enum SensorBinaryType {
	SensorBinaryType_Reserved = 0,
	SensorBinaryType_GeneralPurpose,
	SensorBinaryType_Smoke,
	SensorBinaryType_CO,
	SensorBinaryType_CO2,
	SensorBinaryType_Heat,
	SensorBinaryType_Water,
	SensorBinaryType_Freeze,
	SensorBinaryType_Tamper,
	SensorBinaryType_Aux,
	SensorBinaryType_DoorWindow,
	SensorBinaryType_Tilt,
	SensorBinaryType_Motion,
	SensorBinaryType_GlassBreak,
	SensorBinaryType_Count
};

static char const* c_sensorBinaryTypeNames[] = {
	"Reserved",
	"General purpose",
	"Smoke",
	"CO",
	"CO2",
	"Heat",
	"Water",
	"Freeze",
	"Tamper",
	"Aux",
	"Door/Window",
	"Tilt",
	"Motion",
	"Glass Break"
};


//-----------------------------------------------------------------------------
// <SensorBinary::ReadXML>
// Read node configuration data
//-----------------------------------------------------------------------------
void SensorBinary::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	TiXmlElement const* child = _ccElement->FirstChildElement();

	char const* str; int index; int type;

	while( child )
	{
		str = child->Value();

		if( str )
		{
			if( !strcmp( str, "SensorMap" ) )
			{
				if( TIXML_SUCCESS == child->QueryIntAttribute( "index", &index ) &&
					TIXML_SUCCESS == child->QueryIntAttribute( "type", &type ) )
				{
					m_sensorsMap[(uint8)type] = (uint8)index;
				}
			}
		}

		child = child->NextSiblingElement();
	}
}
//-----------------------------------------------------------------------------
// <SensorBinary::WriteXML>
// Write node configuration data
//-----------------------------------------------------------------------------
void SensorBinary::WriteXML
(
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );

	char str[8];

	for( map<uint8,uint8>::iterator it = m_sensorsMap.begin(); it != m_sensorsMap.end(); it++ )
	{
		TiXmlElement* sensorMapElement = new TiXmlElement( "SensorMap" );
		_ccElement->LinkEndChild( sensorMapElement );

		snprintf( str, 8, "%d", it->second );
		sensorMapElement->SetAttribute( "index", str );

		snprintf( str, 8, "%d", it->first );
		sensorMapElement->SetAttribute( "type", str );
	}
}
//-----------------------------------------------------------------------------
// <SensorBinary::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool SensorBinary::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool res = false;
	if (_requestFlags & RequestFlag_Static)
	{
		if( GetVersion() > 1 )
		{
			Msg* msg = new Msg( "SensorBinaryCmd_SupportedSensorGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( SensorBinaryCmd_SupportedSensorGet );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			res = true;
		}
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		res |= RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return res;
}

//-----------------------------------------------------------------------------
// <SensorBinary::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool SensorBinary::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool res = false;
	if ( IsGetSupported() )
	{
		if( GetVersion() > 1 ) {
			for( uint8 i = 0; i < SensorBinaryType_Count; i++ )
			{
		    	uint8 index = i;
				if( !m_sensorsMap.empty() )
				{	// use the (legacy) sensor map
			    	index = m_sensorsMap[i];
				}

				if( Value* value = GetValue( _instance, index ) )
				{
					value->Release();
					Msg* msg = new Msg( "SensorBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->SetInstance( this, _instance );
					msg->Append( GetNodeId() );
					msg->Append( 3 );
					msg->Append( GetCommandClassId() );
					msg->Append( SensorBinaryCmd_Get );
					msg->Append( i );
					msg->Append( GetDriver()->GetTransmitOptions() );
					GetDriver()->SendMsg( msg, _queue );
					res = true;
				}
			}
		}
		else
		{
			Msg* msg = new Msg( "SensorBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( SensorBinaryCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			res = true;
		}
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "SensorBinaryCmd_Get Not Supported on this node");
	}
	return res;
}

//-----------------------------------------------------------------------------
// <SensorBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SensorBinary::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (SensorBinaryCmd_Report == (SensorBinaryCmd)_data[0])
	{
	    if( GetVersion() > 1 )
	    {
	    	uint8 index = _data[2];
			if( !m_sensorsMap.empty() )
			{	// use the (legacy) sensor map
		    	index = m_sensorsMap[_data[2]];
			}
	    	string sensor_type =  ( _data[2] < SensorBinaryType_Count ) ? c_sensorBinaryTypeNames[_data[2]] : "Unknown type";

            Log::Write( LogLevel_Info, GetNodeId(), "Received SensorBinary report: Sensor:%s(%d) State=%s", sensor_type.c_str(), _data[2], _data[1] ? "On" : "Off" );

            if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, index ) ) )
            {
                value->OnValueRefreshed( _data[1] != 0 );
                value->Release();
            }

            return true;
	    }
	    else
	    {
            Log::Write( LogLevel_Info, GetNodeId(), "Received SensorBinary report: State=%s", _data[1] ? "On" : "Off" );

            if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
            {
                value->OnValueRefreshed( _data[1] != 0 );
                value->Release();
            }

            return true;
	    }
	}
	else if( SensorBinaryCmd_SupportedSensorReport == (SensorBinaryCmd)_data[0] ) {
		Log::Write( LogLevel_Info, GetNodeId(), "Received SensorBinaryCmd_SupportedSensorReport from node %d", GetNodeId() );

		// the configuration contains a (legacy)sensor map. Do not use the sensor auto discovery.
		if( !m_sensorsMap.empty() )
		{
			return true;
		}

		if( Node* node = GetNodeUnsafe() )
		{
			// Parse the data for the supported sensor types
			for( uint8 i = 1; i <= ( _length - 2 ); i++ )
			{
				for( uint8 j = 0; j < 8; j++ )
				{
					if( _data[i] & ( 1 << j ) )
					{
						uint8 index = ( ( i - 1 ) * 8 ) + j;
						if( index < SensorBinaryType_Count )
						{
							node->CreateValueBool(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, index, c_sensorBinaryTypeNames[index], "", true, false, false, 0  );
							Log::Write( LogLevel_Info, GetNodeId(), "    Added sensor type: %s", c_sensorBinaryTypeNames[index] );
						}
						else
						{
							Log::Write( LogLevel_Info, GetNodeId(), "    Unknown sensor type: %d", index );
						}
					}
				}
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SensorBinary::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
void SensorBinary::SetValueBasic
(
	uint8 const _instance,
	uint8 const _value
)
{
	// Send a request for new value to synchronize it with the BASIC set/report.
	// In case the device is sleeping, we set the value anyway so the BASIC set/report
	// stays in sync with it. We must be careful mapping the uint8 BASIC value
	// into a class specific value.
	// When the device wakes up, the real requested value will be retrieved.
	RequestValue( 0, 0, _instance, Driver::MsgQueue_Send );
	if( Node* node = GetNodeUnsafe() )
	{
		if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
		{
			if( !wakeUp->IsAwake() )
			{
				if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
				{
					value->OnValueRefreshed( _value != 0 );
					value->Release();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <SensorBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SensorBinary::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Sensor", "", true, false, false, 0 );
	}
}
