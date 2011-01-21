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
#include "Group.h"
#include "Driver.h"
#include "Log.h"
#include "ValueBool.h"
#include "ValueByte.h"
#include "ValueShort.h"

using namespace OpenZWave;

enum AssociationCommandConfigurationCmd
{
	AssociationCommandConfigurationCmd_SupportedRecordsGet		= 0x01,
	AssociationCommandConfigurationCmd_SupportedRecordsReport	= 0x02,
	AssociationCommandConfigurationCmd_Set						= 0x03,
	AssociationCommandConfigurationCmd_Get						= 0x04,
	AssociationCommandConfigurationCmd_Report					= 0x05
};

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
bool AssociationCommandConfiguration::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Session )
	{
		RequestValue();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestValue>												   
// Request current value from the device									   
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::RequestValue
(
	uint8 const _index		// = 0
)
{
	Msg* msg = new Msg( "AssociationCommandConfigurationCmd_SupportedRecordsGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCommandConfigurationCmd_SupportedRecordsGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestCommands>												   
// Request the command data									   
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::RequestCommands
(
	uint8 const _groupIdx,
	uint8 const _nodeId
)
{
	Msg* msg = new Msg( "AssociationCommandConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCommandConfigurationCmd_Get );
	msg->Append( _groupIdx );
	msg->Append( _nodeId );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool AssociationCommandConfiguration::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (AssociationCommandConfigurationCmd_SupportedRecordsReport == (AssociationCommandConfigurationCmd)_data[0])
	{
		uint8 maxCommandLength			=	 _data[1] >> 2;
		bool commandsAreValues			= ( (_data[1] & 0x02) != 0 );
		bool commandsAreConfigurable	= ( (_data[1] & 0x01) != 0 );
		int16 numFreeCommands			= (((int16)_data[2])<<16) | (int16)_data[3];
		int16 maxCommands				= (((int16)_data[4])<<16) | (int16)_data[5];

		Log::Write( "Received AssociationCommandConfiguration Supported Records Report from node %d:", GetNodeId() );
		Log::Write( "    Maximum command length = %d bytes", maxCommandLength );
		Log::Write( "    Maximum number of commands = %d", maxCommands );
		Log::Write( "    Number of free commands = %d", numFreeCommands );
		Log::Write( "    Commands are %s and are %s", commandsAreValues ? "values" : "not values", commandsAreConfigurable ? "configurable" : "not configurable" );

		ValueBool* valueBool;
		ValueByte* valueByte;
		ValueShort* valueShort;

		if( ( valueByte = m_maxCommandLength.GetInstance( _instance ) ) != NULL)
		{
			valueByte->OnValueChanged( maxCommandLength );
		}

		if( ( valueBool = m_commandsAreValues.GetInstance( _instance ) ) != NULL)
		{
			valueBool->OnValueChanged( commandsAreValues );
		}

		if( ( valueBool = m_commandsAreConfigurable.GetInstance( _instance ) ) != NULL)
		{
			valueBool->OnValueChanged( commandsAreConfigurable );
		}

		if( ( valueShort = m_numFreeCommands.GetInstance( _instance ) ) != NULL)
		{
			valueShort->OnValueChanged( numFreeCommands );
		}

		if( ( valueShort = m_maxCommands.GetInstance( _instance ) ) != NULL)
		{
			valueShort->OnValueChanged( maxCommands );
		}
		return true;
	}
	
	if (AssociationCommandConfigurationCmd_Report == (AssociationCommandConfigurationCmd)_data[0])
	{
		uint8 groupIdx		= _data[1];
		uint8 nodeIdx		= _data[2];
		bool  firstReports	= ( ( _data[3] & 0x80 ) != 0 );		// True if this is the first message containing commands for this group and node.
		uint8 numReports	= _data[3] & 0x0f;

		Log::Write( "Received AssociationCommandConfiguration Report from node %d:", GetNodeId() );
		Log::Write( "    Commands for node %d in group %d,", nodeIdx, groupIdx );

		if( Node* node = GetNodeUnsafe() )
		{
			Group* group = node->GetGroup( groupIdx );
			if( NULL == group )
			{
				if( firstReports )
				{
					// This is the first report message, so we should clear any existing command data
					group->ClearCommands( nodeIdx );
				}

				uint8 const* start = &_data[4];

				for( uint8 i=0; i<numReports; ++i )
				{
					uint8 length = start[0];
					group->AddCommand( nodeIdx, length, start+1 );
					start += length;
				}
			}
		}

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
	if( Node* node = GetNodeUnsafe() )
	{
		m_maxCommandLength.AddInstance			( _instance, node->CreateValueByte(	 ValueID::ValueGenre_System, GetCommandClassId(), _instance, 0, "Max Command Length", "", true, 0  ) );
		m_commandsAreValues.AddInstance			( _instance, node->CreateValueBool(  ValueID::ValueGenre_System, GetCommandClassId(), _instance, 1, "Commands are Values", "", true, false  ) );
		m_commandsAreConfigurable.AddInstance	( _instance, node->CreateValueBool(  ValueID::ValueGenre_System, GetCommandClassId(), _instance, 2, "Commands are Configurable", "", true, false  ) );
		m_numFreeCommands.AddInstance			( _instance, node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 3, "Free Commands", "", true, 0  ) );
		m_maxCommands.AddInstance				( _instance, node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 4, "Max Commands", "", true, 0  ) );
	}
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::SetCommand>
// Set a command for the association									   
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::SetCommand
(
	uint8 const _groupIdx,
	uint8 const _nodeId,
	uint8 const _length,
	uint8 const* _data
)
{
	Msg* msg = new Msg( "AssociationCommandConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( _length + 5 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCommandConfigurationCmd_Set );
	msg->Append( _groupIdx );
	msg->Append( _nodeId );
	msg->Append( _length );

	for( uint8 i=0; i<_length; ++i )
	{
		msg->Append( _data[i] );
	}

	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
}



