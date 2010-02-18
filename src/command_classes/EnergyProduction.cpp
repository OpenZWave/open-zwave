//-----------------------------------------------------------------------------
//
//	EnergyProduction.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ENERGY_PRODUCTION
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
#include "EnergyProduction.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueDecimal.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum EnergyProductionCmd
{
	EnergyProductionCmd_Get		= 0x02,
	EnergyProductionCmd_Report	= 0x03
};

static char* const c_energyParameterNames[] = 
{
	"Instant energy production",
	"Total energy production",
	"Energy production today",
	"Total production time"
};


//-----------------------------------------------------------------------------
// <EnergyProduction::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void EnergyProduction::RequestState
(
)
{
	// Request each of the production values
	Get( Production_Instant );
	Get( Production_Total );
	Get( Production_Today );
	Get( Production_Time );
}

//-----------------------------------------------------------------------------
// <EnergyProduction::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool EnergyProduction::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	bool handled = false;

	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			if (EnergyProductionCmd_Report == (EnergyProductionCmd)_data[0])
			{
				uint8 scale;
				float32 value = ExtractValue( &_data[2], &scale );

				char valueStr[16];
				snprintf( valueStr, 16, "%.3f", value );

				if( ValueDecimal* value = static_cast<ValueDecimal*>( store->GetValue( ValueID( GetNodeId(), GetCommandClassId(), _instance, _data[1] ) ) ) )
				{
					value->OnValueChanged( valueStr );
				}

				Log::Write( "Received an Energy production report from node %d: %s = %s", GetNodeId(), c_energyParameterNames[_data[1]], valueStr );
				handled = true;
			}

			node->ReleaseValueStore();
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <EnergyProduction::Get>												   
// Request current production from the device									   
//-----------------------------------------------------------------------------
void EnergyProduction::Get
(
	ProductionEnum _production
)
{
	Log::Write( "Requesting the %s value from node %d", c_energyParameterNames[_production], GetNodeId() );
	Msg* msg = new Msg( "EnergyProductionCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( EnergyProductionCmd_Get );
	msg->Append( _production );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	Driver::Get()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <EnergyProduction::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void EnergyProduction::CreateVars
(
	uint8 const _instance
)
{
	Node* node = GetNode();
	if( node )
	{
		ValueStore* store = node->GetValueStore();
		if( store )
		{
			Value* value;
			
			value = new ValueDecimal( GetNodeId(), GetCommandClassId(), _instance, (uint8)Production_Instant, Value::Genre_User, c_energyParameterNames[Production_Instant], true, "0.0"  );
			store->AddValue( value );
			value->Release();

			value = new ValueDecimal( GetNodeId(), GetCommandClassId(), _instance, (uint8)Production_Total, Value::Genre_User, c_energyParameterNames[Production_Total], true, "0.0"  );
			store->AddValue( value );
			value->Release();

			value = new ValueDecimal( GetNodeId(), GetCommandClassId(), _instance, (uint8)Production_Today, Value::Genre_User, c_energyParameterNames[Production_Today], true, "0.0"  );
			store->AddValue( value );
			value->Release();

			value = new ValueDecimal( GetNodeId(), GetCommandClassId(), _instance, (uint8)Production_Time, Value::Genre_User, c_energyParameterNames[Production_Time], true, "0.0"  );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}



