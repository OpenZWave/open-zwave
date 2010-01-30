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
#include "AssociationCommandConfiguration.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

#include "ValueByte.h"
#include "ValueStore.h"

using namespace OpenZWave;

static enum AssociationCommandConfigurationCmd
{
    AssociationCommandConfigurationCmd_Get	= 0x04,
    AssociationCommandConfigurationCmd_Report = 0x05
};

static enum
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
)
{
	Log::Write( "Requesting the AssociationCommandConfiguration status from node %d", GetNodeId() );
    Msg* pMsg = new Msg( "AssociationCommandConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 2 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( AssociationCommandConfigurationCmd_Get );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool AssociationCommandConfiguration::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if (AssociationCommandConfigurationCmd_Report == (AssociationCommandConfigurationCmd)_pData[0])
    {
        // We have received a report from the Z-Wave device
		// No known mappings for these values yet
        uint8 AssociationCommandConfigurationType = _pData[1];
        uint8 AssociationCommandConfigurationLevel = _pData[2];
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
	Node* pNode = GetNode();
	if( pNode )
	{
		ValueStore* pStore = pNode->GetValueStore();
		if( pStore )
		{
			Value* pValue;
		
			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Type, "AssociationCommandConfiguration Type", true, 0 );
			pStore->AddValue( pValue );
			pValue->Release();

			pValue = new ValueByte( GetNodeId(), GetCommandClassId(), _instance, ValueIndex_Level, "AssociationCommandConfiguration Level", true, 0 );
			pStore->AddValue( pValue );
			pValue->Release();
		}
	}
}

