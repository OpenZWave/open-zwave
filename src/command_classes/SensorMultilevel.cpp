//-----------------------------------------------------------------------------
//
//	SensorMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_MULTILEVEL
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "CommandClasses.h"
#include "SensorMultilevel.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueDecimal.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum SensorMultilevelCmd
{
	SensorMultilevelCmd_Get		= 0x04,
	SensorMultilevelCmd_Report	= 0x05
};


//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestState>                                                   
// Request current state from the device                                       
//-----------------------------------------------------------------------------
void SensorMultilevel::RequestState
(
)
{
    Msg* pMsg = new Msg( "SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( SensorMultilevelCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <SensorMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SensorMultilevel::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			if (SensorMultilevelCmd_Report == (SensorMultilevelCmd)_pData[0])
			{
				uint8 scale;
				float value = ExtractValue( &_pData[2], &scale );

				char valueStr[16];
				snprintf( valueStr, 16, "%.3f", value );

				if( ValueDecimal* pValue = static_cast<ValueDecimal*>( pStore->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, 0 ) ) ) )
				{
					if( pValue->GetLabel() == "Unknown" )
					{
						switch( _pData[1] )
						{
							case 0x01:
							{
								// Temperature
								pValue->SetLabel( "Temperature" );
								break;
							}
							case 0x02:
							{
								// General
								pValue->SetLabel( "Sensor" );
								pValue->SetUnits( "" );
								break;
							}
							case 0x03:
							{
								// Luminance
								pValue->SetLabel( "Luminance" );
								pValue->SetUnits( "%" );
								break;
							}
							case 0x04:
							{
								// Power
								pValue->SetLabel( "Power" );
								pValue->SetUnits( "kW" );
								break;
							}
							case 0x05:
							{
								// Humidity
								pValue->SetLabel( "Humidity" );
								pValue->SetUnits( "%" );
								break;
							}
							case 0x11:
							{
								// CO2
								pValue->SetLabel( "Carbon Monoxide" );
								pValue->SetUnits( "ppm" );
								break;
							}
						}
					}

					if( _pData[1] == 0x01 )
					{
						// Temperature units can usually be changed on 
						// the device, so we have to check each time.
						pValue->SetUnits( scale ? "F" : "C" );
					}

					pValue->OnValueChanged( valueStr );
				}

				Log::Write( "Received SensorMultiLevel report from node %d: value=%s", GetNodeId(), valueStr );
				return true;
			}
		}
	}

    return false;
}

//-----------------------------------------------------------------------------
// <SensorMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SensorMultilevel::CreateVars
(
	uint8 const _instance
)
{
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			Value* pValue = new ValueDecimal( GetNodeId(), GetCommandClassId(), _instance, 0, "Unknown", true, "0.0"  );
			pStore->AddValue( pValue );
		}
	}
}



