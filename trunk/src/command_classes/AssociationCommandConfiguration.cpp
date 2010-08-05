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

using namespace OpenZWave;

enum AssociationCommandConfigurationCmd
{
	AssociationCommandConfigurationCmd_Get	= 0x04,
	AssociationCommandConfigurationCmd_Report = 0x05
};


//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Session )
	{
		Msg* msg = new Msg( "AssociationCommandConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( AssociationCommandConfigurationCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
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
	uint32 const _instance	// = 1
)
{
	if (AssociationCommandConfigurationCmd_Report == (AssociationCommandConfigurationCmd)_data[0])
	{
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
}

