//-----------------------------------------------------------------------------
//
//	ClimateControlSchedule.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE
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

#include "CommandClasses.h"
#include "ClimateControlSchedule.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Log.h"

using namespace OpenZWave;

//static enum ClimateControlScheduleCmd
//{
//	ScheduleSet = 0x01
//	ScheduleGet = 0x02
//		Weekday: type=BYTE
//	ScheduleReport = 0x03
//		Weekday: type=STRUCT_BYTE
//		Switchpoint 0 byte 1: type=BYTE
//		Switchpoint 0 byte 2: type=BYTE
//		Switchpoint 0 byte 3: type=BYTE
//		Switchpoint 1 byte 1: type=BYTE
//		Switchpoint 1 byte 2: type=BYTE
//		Switchpoint 1 byte 3: type=BYTE
//		Switchpoint 2 byte 1: type=BYTE
//		Switchpoint 2 byte 2: type=BYTE
//		Switchpoint 2 byte 3: type=BYTE
//		Switchpoint 3 byte 1: type=BYTE
//		Switchpoint 3 byte 2: type=BYTE
//		Switchpoint 3 byte 3: type=BYTE
//		Switchpoint 4 byte 1: type=BYTE
//		Switchpoint 4 byte 2: type=BYTE
//		Switchpoint 4 byte 3: type=BYTE
//		Switchpoint 5 byte 1: type=BYTE
//		Switchpoint 5 byte 2: type=BYTE
//		Switchpoint 5 byte 3: type=BYTE
//		Switchpoint 6 byte 1: type=BYTE
//		Switchpoint 6 byte 2: type=BYTE
//		Switchpoint 6 byte 3: type=BYTE
//		Switchpoint 7 byte 1: type=BYTE
//		Switchpoint 7 byte 2: type=BYTE
//		Switchpoint 7 byte 3: type=BYTE
//		Switchpoint 8 byte 1: type=BYTE
//		Switchpoint 8 byte 2: type=BYTE
//		Switchpoint 8 byte 3: type=BYTE
//	ScheduleChangedGet = 0x04
//		Weekday: type=STRUCT_BYTE
//	ScheduleChangedReport = 0x05
//		Changecounter: type=BYTE
//	ScheduleOverrideSet = 0x06
//		Override type: type=STRUCT_BYTE
//		Override state: type=BYTE
//	ScheduleOverrideGet = 0x07
//	ScheduleOverrideReport = 0x08
//		Override type: type=STRUCT_BYTE
//		Override state: type=BYTE
//};
//
//
////-----------------------------------------------------------------------------
//// <ClimateControlSchedule::RequestState>												   
//// Request current state from the device									   
////-----------------------------------------------------------------------------
//void ClimateControlSchedule::RequestState
//(
//	  bool const _poll
//)
//{
//	Msg* msg = new Msg( "ScheduleChangedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
//	msg->Append( GetNodeId() );
//	msg->Append( 2 );
//	msg->Append( GetCommandClassId() );
//	msg->Append( ScheduleChangedGet );
//	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
//	GetDriver()->SendMsg( msg );
//}
//
////-----------------------------------------------------------------------------
//// <ClimateControlSchedule::HandleMsg>
//// Handle a message from the Z-Wave network
////-----------------------------------------------------------------------------
//bool ClimateControlSchedule::HandleMsg
//(
//	uint8 const* _data,
//	uint32 const _length,
//	  uint32 const _instance	// = 1
//	
//)
//{
//	if (ClimateControlScheduleCmd_Report == (ClimateControlScheduleCmd)_data[0])
//	{
//		// We have received a report from the Z-Wave device
//		// ThermostatModeEnum mode = (ThermostatModeEnum)_data[1];
//		// m_pZone->HvacModeChanged(c_modeName[mode]);
//		return true;
//	}
//	return false;
//}
//
////-----------------------------------------------------------------------------
//// <ClimateControlSchedule::Set>
//// Set the device's 
////-----------------------------------------------------------------------------
//void ClimateControlSchedule::Set
//(
//)
//{
//}
//
