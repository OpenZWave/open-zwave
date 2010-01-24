//-----------------------------------------------------------------------------
//
//	MultiInstance.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_INSTANCE
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
#include "MultiInstance.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "Log.h"

using namespace OpenZWave;

static enum MultiInstanceCmd
{
    MultiInstanceCmd_Get		= 0x04,
    MultiInstanceCmd_Report		= 0x05,
    MultiInstanceCmd_CmdEncap	= 0x06
};


//-----------------------------------------------------------------------------
// <MultiInstance::RequestStatic>                                                   
// Request the static instance data                                      
//-----------------------------------------------------------------------------
void MultiInstance::RequestStatic
(
)
{
	if( Node const* pNode = GetNode() )
	{
		pNode->RequestInstances();
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::RequestInstances>                                                   
// Request number of instances of the specified command class from the device                                       
//-----------------------------------------------------------------------------
void MultiInstance::RequestInstances
(
	CommandClass const* _pCommandClass
)
{
	Log::Write( "Requesting the instance-count from node %d for %s", GetNodeId(), _pCommandClass->GetCommandClassName().c_str() );
    Msg* pMsg = new Msg( "MultiInstanceCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
    pMsg->Append( GetNodeId() );
    pMsg->Append( 3 );
    pMsg->Append( GetCommandClassId() );
    pMsg->Append( MultiInstanceCmd_Get );
	pMsg->Append( _pCommandClass->GetCommandClassId() );
    pMsg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
    Driver::Get()->SendMsg( pMsg );
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MultiInstance::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
	if( Node const* pNode = GetNode() )
	{
		if( MultiInstanceCmd_Report == (MultiInstanceCmd)_pData[0] )
		{
			uint8 commandClassId = _pData[1];
			uint8 instances = _pData[2];

			if( CommandClass* pCommandClass = pNode->GetCommandClass( commandClassId ) )
			{
				Log::Write( "Received instance-count from node %d for %s: %d", GetNodeId(), pCommandClass->GetCommandClassName().c_str(), instances );
				pCommandClass->SetInstances( instances );
			}
			return true;
		}
	
		if( MultiInstanceCmd_CmdEncap == (MultiInstanceCmd)_pData[0] )
		{
			uint8 instance = _pData[1];
			uint8 commandClassId = _pData[2];

			if( CommandClass* pCommandClass = pNode->GetCommandClass( commandClassId ) )
			{
				Log::Write( "Received a multi-instance encapsulated command from node %d: Command Class %s, Instance=%d", GetNodeId(), pCommandClass->GetCommandClassName().c_str(), instance );
				pCommandClass->HandleMsg( &_pData[3], _length-3, instance );
			}

			return true;
		}
    }
    return false;
}

