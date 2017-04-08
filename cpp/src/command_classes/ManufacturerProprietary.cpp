//-----------------------------------------------------------------------------
//
//	Proprietary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_PROPRIETARY
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
#include "command_classes/ManufacturerProprietary.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"

using namespace OpenZWave;

enum ProprietaryCmd
{
	ProprietaryCmd_Set		= 0x01,
	ProprietaryCmd_Get		= 0x02,
	ProprietaryCmd_Report	= 0x03
};

const uint8 FIBARO[2] = { 0x01, 0x0f };
const uint8 FIBARO_BLINDS_SLATS[3] = {0x26, 0x03, 0x03};

//-----------------------------------------------------------------------------
// <Proprietary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ManufacturerProprietary::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	uint8 const* payload = _data+2;
	if( FIBARO[0] == _data[0] &&
			FIBARO[1] == _data[1] ){

		if( FIBARO_BLINDS_SLATS[0] == payload[0] &&
				FIBARO_BLINDS_SLATS[1] == payload[1] &&
				FIBARO_BLINDS_SLATS[2] == payload[2] ){
			uint8 blinds = payload[3];
			uint8 slats = payload[4];
			ValueByte* blindsValue = static_cast<ValueByte*>( GetValue( _instance, 0 ) );
			ValueByte* slatsValue = static_cast<ValueByte*>( GetValue( _instance, 1 ) );

			Log::Write( LogLevel_Info, GetNodeId(), "Received Fibaro proprietary blind/slat position for node %d: Blinds: %d Slats: %d",
						    GetNodeId(), blinds, slats);

			blindsValue->OnValueRefreshed(blinds);
			blindsValue->Release();
			slatsValue->OnValueRefreshed(slats);
			slatsValue->Release();

			return true;
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "Received unknown Fibaro proprietary message for node %d.",
						    GetNodeId());
			return false;
		}
	}
	Log::Write( LogLevel_Warning, GetNodeId(), "Received unknown manufacturer proprietary message for node %d.",
					GetNodeId());
	return false;
}

