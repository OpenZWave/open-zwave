//-----------------------------------------------------------------------------
//
//	Hail.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_HAIL
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
#include "Hail.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Log.h"

using namespace OpenZWave;

static enum HailCmdEnum
{
	HailCmd_Hail = 1
};


//-----------------------------------------------------------------------------
// <Hail::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Hail::HandleMsg
(
    uint8 const* _pData,
    uint32 const _length,
	uint32 const _instance	// = 0
)
{
    if( HailCmd_Hail == _pData[0] )
    {
        // We have received a hail from the Z-Wave device
		// Request the a basic report from the node
		if( Node* pNode = GetNode() )
		{
//			pNode->RequestBasicReport();
		}
        return true;
	}
    return false;
}

