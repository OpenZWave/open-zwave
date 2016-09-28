//-----------------------------------------------------------------------------
//
//	CentralScene.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CENTRAL_SCENE
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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
#include "command_classes/CentralScene.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include "value_classes/ValueInt.h"

#include "tinyxml.h"

using namespace OpenZWave;

enum CentralSceneCmd
{
	CentralSceneCmd_Capability_Get    = 0x01,
	CentralSceneCmd_Capability_Report = 0x02,
	CentralSceneCmd_Set				  = 0x03
};


enum CentralScene_ValueID_Index
{
	CentralSceneIndex_SceneCount         = 0x00,
	CentralSceneIndex_SinglePress        = 0x01,
	CentralSceneIndex_Released           = 0x02,
	CentralSceneIndex_HeldDown           = 0x03,
	CentralSceneIndex_DoublePress        = 0x04,
	CentralSceneIndex_TriplePress        = 0x05,
	CentralSceneIndex_Pressed4Times      = 0x06,
	CentralSceneIndex_Pressed5Times      = 0x07

};

enum CentralScene_KeyAttributesMask
{
    CentralSceneMask_KeyPressed1time        = 0x01,
    CentralSceneMask_KeyReleased            = 0x02,
    CentralSceneMask_HeldDown               = 0x04,
    CentralSceneMask_KeyPressed2times       = 0x08,
    CentralSceneMask_KeyPressed3times       = 0x10,
    CentralSceneMask_KeyPressed4times       = 0x20,
    CentralSceneMask_KeyPressed5times       = 0x40,
    CentralSceneMask_reserved               = 0x80,
};
//-----------------------------------------------------------------------------
// <CentralScene::CentralScene>
// Constructor
//-----------------------------------------------------------------------------
CentralScene::CentralScene
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId ),
m_scenecount(0)
{
    SetStaticRequest( StaticRequest_Values );
	Log::Write(LogLevel_Info, GetNodeId(), "CentralScene - Created %d", HasStaticRequest( StaticRequest_Values ));
}


//-----------------------------------------------------------------------------
// <CentralScene::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool CentralScene::RequestState
(
		uint32 const _requestFlags,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	Log::Write(LogLevel_Info, GetNodeId(), "CentralScene RequestState: %d", _requestFlags);
	bool requests = false;
	if( (_requestFlags & RequestFlag_Static) && HasStaticRequest( StaticRequest_Values ) )
	{
			requests = RequestValue( _requestFlags, CentralSceneCmd_Capability_Get, _instance, _queue );
	} else {
		Log::Write(LogLevel_Info, GetNodeId(), "CentralScene: Not a StaticRequest");
	}
	return requests;
}

//-----------------------------------------------------------------------------
// <CentralScene::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool CentralScene::RequestValue
(
		uint32 const _requestFlags,
		uint8 const _what,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if (_what == CentralSceneCmd_Capability_Get) {
		Msg* msg = new Msg( "CentralSceneCmd_Capability_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( _what );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
	}
	return true;
}
//-----------------------------------------------------------------------------
// <CentralScene::ReadXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void CentralScene::ReadXML
(
		TiXmlElement const* _ccElement
)
{
	int32 intVal;

	CommandClass::ReadXML( _ccElement );
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "scenecount", &intVal ) )
	{
		m_scenecount = intVal;
	}
}

//-----------------------------------------------------------------------------
// <CentralScene::WriteXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void CentralScene::WriteXML
(
		TiXmlElement* _ccElement
)
{
	char str[32];

	CommandClass::WriteXML( _ccElement );
	snprintf( str, sizeof(str), "%d", m_scenecount );
	_ccElement->SetAttribute( "scenecount", str);
}


//-----------------------------------------------------------------------------
// <CentralScene::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool CentralScene::HandleMsg
(
		uint8 const* _data,
		uint32 const _length,
		uint32 const _instance	// = 1
)
{
	if( CentralSceneCmd_Set == (CentralSceneCmd)_data[0] )
	{
		// Central Scene Set received so send notification
		uint8 keyAttribute = _data[2];
		uint8 sceneID = _data[3];
		Log::Write( LogLevel_Info, GetNodeId(), "Received Central Scene set from node %d: scene id=%d with key Attribute %d. Sending event notification.", GetNodeId(), sceneID, keyAttribute);

		int index = 1;
		if      ( keyAttribute == 0) { index = CentralSceneIndex_SinglePress; }
		else if ( keyAttribute == 1) { index = CentralSceneIndex_Released; }
		else if ( keyAttribute == 2) { index = CentralSceneIndex_HeldDown; }
		else if ( keyAttribute == 3) { index = CentralSceneIndex_DoublePress; }
		else if ( keyAttribute == 4) { index = CentralSceneIndex_TriplePress; }
		else if ( keyAttribute == 5) { index = CentralSceneIndex_Pressed4Times; }
		else if ( keyAttribute == 6) { index = CentralSceneIndex_Pressed5Times;}

		if( ValueInt* value = static_cast<ValueInt*>( GetValue( sceneID, index ) ) )
		{
			value->OnValueRefreshed( 1 );
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for Scene %d", _data[3]);
			return false;
		}
		return true;
	}
	else if (CentralSceneCmd_Capability_Report == (CentralSceneCmd)_data[0])
	{
		/* Create a Number of ValueID's based on the m_scenecount variable
		 * We prefer what the Config File specifies rather than what is returned by
		 * the Device...
		 */
		int scenecount = _data[1];
		if (scenecount != 0)
		{
			m_scenecount = scenecount;
		}
		bool identical = false;
		bool version2orHigher = false;
		if ( _length >= 2 )
		{
		    version2orHigher = true;
		    identical = _data[2] & 0b00000001;
		    Log::Write( LogLevel_Warning, GetNodeId(), "this is version 2 or higher, all scenes identical? %i",identical);
		}

		if ( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, CentralSceneIndex_SceneCount)))
		{
			value->OnValueRefreshed(m_scenecount);
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "Can't find ValueID for SceneCount");
		}

		if( Node* node = GetNodeUnsafe() )
		{
				for (int i = 1; i <= m_scenecount ; i++) {
					if ( version2orHigher )
					{
					    if ( identical )
					    {
					        // instance for scene number normaly isn't right, but in this case i think this is the best solution.
					        // Because i higly doubt that someone will ever use the multichannel class together with the centralScene class.
                            int keyAttributes = _data[3];
                            if ( keyAttributes & CentralSceneMask_KeyPressed1time)
                            {
                                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_SinglePress, "Scene", "", true, false, 0, 0 );
                            }
                            if ( keyAttributes & CentralSceneMask_KeyPressed2times)
                            {
                                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_DoublePress, "Scene double press", "", true, false, 0, 0 );
                            }
                            if ( keyAttributes & CentralSceneMask_HeldDown)
                            {
                                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_HeldDown, "Scene held down", "", true, false, 0, 0 );
                            }
                            if ( keyAttributes & CentralSceneMask_KeyReleased)
                            {
                                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_Released, "Scene released", "", true, false, 0, 0 );
                            }
                            if ( keyAttributes & CentralSceneMask_KeyPressed3times)
                            {
                                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_TriplePress, "Scene triple press", "", true, false, 0, 0 );
                            }
                            if ( keyAttributes & CentralSceneMask_KeyPressed4times)
                            {
                                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_Pressed4Times, "Scene pressed 4 times", "", true, false, 0, 0 );
                            }
                            if ( keyAttributes & CentralSceneMask_KeyPressed5times)
                            {
                                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_Pressed5Times, "Scene pressed 5 times", "", true, false, 0, 0 );
                            }
                        }
					    else
					    {
					        int numberOfBitMasks = _data[2] & 0b00000110;
					        for ( int i = 1; i <= numberOfBitMasks; i++ )
					        {
					            int keyAttributes = _data[2 +i];
					            if ( keyAttributes & CentralSceneMask_KeyPressed1time)
					            {
					                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_SinglePress, "Scene", "", true, false, 0, 0 );
					            }
					            if ( keyAttributes & CentralSceneMask_KeyPressed2times)
					            {
					                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_DoublePress, "Scene double press", "", true, false, 0, 0 );
					            }
					            if ( keyAttributes & CentralSceneMask_HeldDown)
					            {
					                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_HeldDown, "Scene held down", "", true, false, 0, 0 );
					            }
					            if ( keyAttributes & CentralSceneMask_KeyReleased)
					            {
					                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_Released, "Scene released", "", true, false, 0, 0 );
					            }
					            if ( keyAttributes & CentralSceneMask_KeyPressed3times)
					            {
					                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_TriplePress, "Scene triple press", "", true, false, 0, 0 );
					            }
					            if ( keyAttributes & CentralSceneMask_KeyPressed4times)
					            {
					                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_Pressed4Times, "Scene pressed 4 times", "", true, false, 0, 0 );
					            }
					            if ( keyAttributes & CentralSceneMask_KeyPressed5times)
					            {
					                node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_Pressed5Times, "Scene pressed 5 times", "", true, false, 0, 0 );
					            }
					        }
					        //in case not all scenes are identical, we need to loop through all keyAttribute Bytes and assign them according to the right instance.
					    }
                    }
                    else
                    {
                        node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), i, CentralSceneIndex_SinglePress, "Scene", "", true, false, 0, 0 );
					}
				}

		} else {
			Log::Write(LogLevel_Info, GetNodeId(), "CentralScene: Can't find Node!");
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <CentralScene::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void CentralScene::CreateVars
(
		uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, CentralSceneIndex_SceneCount, "Scene Count", "", true, false, 0, 0 );
	}
}


