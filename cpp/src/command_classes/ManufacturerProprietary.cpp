//-----------------------------------------------------------------------------
//
//    ManufacturerProprietary.cpp
//
//    Implementation of the Z-Wave COMMAND_CLASS_PROPRIETARY
//
//    Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//    SOFTWARE NOTICE AND LICENSE
//
//    This file is part of OpenZWave.
//
//    OpenZWave is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as published
//    by the Free Software Foundation, either version 3 of the License,
//    or (at your option) any later version.
//
//    OpenZWave is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public License
//    along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "command_classes/CommandClasses.h"
#include "command_classes/ManufacturerProprietary.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"

using namespace OpenZWave;


const uint8 MANUFACTURER_ID_FIBARO[2] = { 0x01, 0x0f };
//manufacturer proprietary message identifier for venetian blinds reports and get/set
const uint8 FIBARO_VENETIEN_BLINDS_REPORT_ID[3] = { 0x26, 0x03, 0x03 };
const uint8 FIBARO_VENETIAN_BLINDS_GET_POSITION_TILT[5] = { 0x26, 0x02, 0x02, 0x00, 0x00 };
const uint8 FIBARO_VENETIAN_BLINDS_SET_TILT[4] =          { 0x26, 0x01, 0x01, 0x00 };
const uint8 FIBARO_VENETIAN_BLINDS_SET_POSITION[3] =      { 0x26, 0x01, 0x02 };


//-----------------------------------------------------------------------------
// <ManufacturerProprietary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ManufacturerProprietary::HandleMsg
(
    uint8 const* _data,
    uint32 const _length,
    uint32 const _instance    // = 1
)
{
    uint8 const* payload = _data+2;
    if( MANUFACTURER_ID_FIBARO[0] == _data[0] &&
        MANUFACTURER_ID_FIBARO[1] == _data[1] )
    {

        if( FIBARO_VENETIEN_BLINDS_REPORT_ID[0] == payload[0] &&
            FIBARO_VENETIEN_BLINDS_REPORT_ID[1] == payload[1] &&
            FIBARO_VENETIEN_BLINDS_REPORT_ID[2] == payload[2] )
        {
        ValueByte* blindsValue = static_cast<ValueByte*>( GetValue( _instance, ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Blinds ) );
        ValueByte* tiltValue   = static_cast<ValueByte*>( GetValue( _instance, ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Tilt ) );

        if( NULL != blindsValue && NULL != tiltValue)
        {
            Log::Write( LogLevel_Info, GetNodeId(), "Received Fibaro proprietary blind/slat position for node %d: Blinds: %d Slats: %d",
                        GetNodeId(), payload[3], payload[4] );
            blindsValue->OnValueRefreshed( payload[3] );
            tiltValue->OnValueRefreshed( payload[4] );
            blindsValue->Release();
            tiltValue->Release();
        }
        else
        {
            Log::Write( LogLevel_Warning, GetNodeId(), "Error setting Fibaro blind/slat position. Values were not found." );      
        }
            return true;
        }
        else
        {
            Log::Write( LogLevel_Warning, GetNodeId(), "Received unknown Fibaro proprietary message for node %d.",
                        GetNodeId());
            return false;
        }
    }
    Log::Write( LogLevel_Warning, GetNodeId(), "Received unknown manufacturer proprietary message for node %d.",
                GetNodeId());
    return false;
}

//-----------------------------------------------------------------------------
// <ManufacturerProprietary::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool ManufacturerProprietary::RequestValue
(
    uint32 const _requestFlags,
    uint16 const _index,
    uint8 const _instance,
    Driver::MsgQueue const _queue
)
{
    if ( m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED) )
    {
        Msg* msg = new Msg( "ManufacturerProprietary_RequestValue", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
        if (ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Blinds == _index || ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Tilt == _index){
            msg->SetInstance( this, _instance );
            msg->Append( GetNodeId() );
            msg->Append( 1+sizeof(MANUFACTURER_ID_FIBARO)+sizeof(FIBARO_VENETIAN_BLINDS_GET_POSITION_TILT) ); // length of data
            msg->Append( GetCommandClassId() );
            msg->AppendArray( MANUFACTURER_ID_FIBARO, sizeof(MANUFACTURER_ID_FIBARO) );
            msg->AppendArray( FIBARO_VENETIAN_BLINDS_GET_POSITION_TILT, sizeof(FIBARO_VENETIAN_BLINDS_GET_POSITION_TILT) );
            msg->Append( GetDriver()->GetTransmitOptions() );
            GetDriver()->SendMsg( msg, _queue );
            return true;
        } else {
            Log::Write( LogLevel_Info, GetNodeId(), "ManufacturerProprietary_RequestValue Not Supported for value index %d",
                    _index);
        }
    } else {
        Log::Write( LogLevel_Info, GetNodeId(), "ManufacturerProprietary_RequestValue Not Supported on this node");
    }
    return false;
}

//-----------------------------------------------------------------------------
// <ManufacturerProprietary::SetValue>
// Set the lock's state
//-----------------------------------------------------------------------------
bool ManufacturerProprietary::SetValue
(
    Value const& _value
)
{
    uint64 value_id = _value.GetID().GetIndex();
    Msg* msg = new Msg( "ManufacturerProprietary_SetValue", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );

    if (ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Blinds == value_id || ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Tilt == value_id){
        ValueByte const* value = static_cast<ValueByte const*>(&_value);

        msg->SetInstance( this, _value.GetID().GetInstance() );
        msg->Append( GetNodeId() );
        msg->Append( 2 + // length of data
                     sizeof(MANUFACTURER_ID_FIBARO) +
                     sizeof(FIBARO_VENETIAN_BLINDS_GET_POSITION_TILT) );
        msg->Append( GetCommandClassId() );
        msg->AppendArray( MANUFACTURER_ID_FIBARO, sizeof(MANUFACTURER_ID_FIBARO) );
        if (ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Blinds == value_id) {
            msg->AppendArray( FIBARO_VENETIAN_BLINDS_SET_POSITION, sizeof(FIBARO_VENETIAN_BLINDS_SET_POSITION) );
            msg->Append( value->GetValue() );
            msg->Append( 0x00 );
        } else if (ValueID_Index_ManufacturerProprietary::FibaroVenetianBlinds_Tilt == value_id) {
            msg->AppendArray( FIBARO_VENETIAN_BLINDS_SET_TILT, sizeof(FIBARO_VENETIAN_BLINDS_SET_TILT) );
            msg->Append( value->GetValue() );
        }
        msg->Append( GetDriver()->GetTransmitOptions() );
        GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
        return true;
    } else {
        Log::Write( LogLevel_Info, GetNodeId(), "ManufacturerProprietary_SetValue %d not supported on node %d", value_id, GetNodeId());
        return false;
    }
}


