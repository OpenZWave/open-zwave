//-----------------------------------------------------------------------------
//
//	AssociationCommandConfiguration.cpp
//
//	Implementation of the COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION
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
#include "AssociationCommandConfiguration.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueStore.h"

using namespace OpenZWave;

enum AssociationCommandConfigurationCmd
{
	AssociationCommandConfigurationCmd_Get	= 0x04,
	AssociationCommandConfigurationCmd_Report = 0x05
};

enum
{
	ValueIndex_Type	= 0,
	ValueIndex_Level
};


//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::RequestState
(
	bool const _poll
)
{
	if( !_poll )
	{
		Log::Write( "Requesting the AssociationCommandConfiguration status from node %d", GetNodeId() );
		Msg* msg = new Msg( "AssociationCommandConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( AssociationCommandConfigurationCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		Driver::Get()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool AssociationCommandConfiguration::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if (AssociationCommandConfigurationCmd_Report == (AssociationCommandConfigurationCmd)_data[0])
	{
		// We have received a report from the Z-Wave device
		// No known mappings for these values yet
		uint8 AssociationCommandConfigurationType = _data[1];
		uint8 AssociationCommandConfigurationLevel = _data[2];
		Log::Write( "Received AssociationCommandConfiguration report from node %d: type=%d, level=%d", GetNodeId(), AssociationCommandConfigurationType, AssociationCommandConfigurationLevel );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::CreateVars
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
		
			value = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Type, Value::Genre_System, "AssociationCommandConfiguration Type", true, 0 );
			store->AddValue( value );
			value->Release();

			value = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Level, Value::Genre_System, "AssociationCommandConfiguration Level", true, 0 );
			store->AddValue( value );
			value->Release();

			node->ReleaseValueStore();
		}
	}
}

