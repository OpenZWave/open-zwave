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
#include "value_classes/ValueButton.h"
#include "value_classes/ValueByte.h"

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
    CentralSceneIndex_SceneCount                         = 0x00,
    CentralSceneIndex_Scene_KeyAttribute                 = 0x01,
    CentralSceneIndex_SceneID                            = 0x02,
    CentralSceneIndex_Button                             = 0x03,
    CentralSceneIndex_Scenes_Identical                   = 0x04,
    CentralSceneIndex_Supported_KeyAttributes_All_Scenes = 0x05,
    CentralSceneIndex_Supported_KeyAttributes_Scene_1    = 0x06,
    CentralSceneIndex_Supported_KeyAttributes_Scene_2    = 0x07,
    CentralSceneIndex_Supported_KeyAttributes_Scene_3    = 0x08,
    CentralSceneIndex_SceneNumber                        = 0x80,

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

enum CentralScene_KeyAttributes
{
    CentralScene_KeyAttributes_KeyPressed1time        = 0,
    CentralScene_KeyAttributes_KeyReleased            = 1,
    CentralScene_KeyAttributes_KeyHeldDown            = 2,
    CentralScene_KeyAttributes_KeyPressed2times       = 3,
    CentralScene_KeyAttributes_KeyPressed3times       = 4,
    CentralScene_KeyAttributes_KeyPressed4times       = 5,
    CentralScene_KeyAttributes_KeyPressed5times       = 6,
    CentralScene_KeyAttributes_reserved               = 7,
};

static char const* c_CentralScene_KeyAttributes[] =
{
        "Pressed 1 Time",
        "Key Released",
        "Key Held down",
        "Pressed 2 Times",
        "Pressed 3 Times",
        "Pressed 4 Times",
        "Pressed 5 Times"
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
		uint8 sceneNumber = sceneID*10 + keyAttribute;
		Log::Write( LogLevel_Info, GetNodeId(), "Received Central Scene set from node %d: scene id=%d with key Attribute %d. Sending event notification.", GetNodeId(), sceneID, keyAttribute);

		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, CentralSceneIndex_Scene_KeyAttribute ) ) )
		{
			value->OnValueRefreshed( keyAttribute );
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for Scene Keyattribute");
			return false;
		}
        if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, CentralSceneIndex_SceneID ) ) )
        {
            value->OnValueRefreshed( sceneID );
            value->Release();
        } else {
            Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for Scene ID");
            return false;
        }
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, CentralSceneIndex_SceneNumber ) ) )
        {
            value->OnValueRefreshed( sceneNumber );
            value->Release();
        } else {
            Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for Scene Number");
            return false;
        }
        if ( keyAttribute == CentralScene_KeyAttributes_KeyHeldDown )
        {
            if( ValueButton* value = static_cast<ValueButton*>( GetValue( _instance, CentralSceneIndex_Button ) ) )
            {
                value->PressButton();
                value->Release();
            } else {
                Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for Scene Button");
                return false;
            }
        }
        else if ( keyAttribute == CentralScene_KeyAttributes_KeyReleased )
        {
            if( ValueButton* value = static_cast<ValueButton*>( GetValue( _instance, CentralSceneIndex_Button ) ) )
            {
                value->ReleaseButton();
                value->Release();
            } else {
                Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for Scene Button");
                return false;
            }
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
		if (m_scenecount == 0)
		{
			m_scenecount = scenecount;
		}
		bool identical = true; //version 1 does not know this, so set it to true.
		if ( GetVersion() >= 2 )
		{
		    identical = _data[2] & 0b00000001;
		    Log::Write( LogLevel_Detail, GetNodeId(), "this is version 2 or higher, all scenes identical? %i",identical);
		}
		if ( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, CentralSceneIndex_Scenes_Identical)))
		{
		    value->OnValueRefreshed(identical);
		    value->Release();
		} else {
		    Log::Write( LogLevel_Warning, GetNodeId(), "Can't find ValueID for Scenes_Identical");
		}

		if ( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, CentralSceneIndex_SceneCount)))
		{
			value->OnValueRefreshed(m_scenecount);
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "Can't find ValueID for SceneCount");
		}

		for (int i = 1; i <= m_scenecount ; i++) {
		    if ( GetVersion() == 1 )
		    {
		        // version 1 does not tell us which keyAttributes are supported, but only single press, released and held down are supported, so add these 3
		        if( Node* node = GetNodeUnsafe() )
		        {
		            vector<ValueList::Item> items;
		            for( unsigned int i=0; i < 3; i++)
		            {
		                ValueList::Item item;
		                item.m_label = c_CentralScene_KeyAttributes[i];
		                item.m_value = i;
		                items.push_back( item );
		            }
		            node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, CentralSceneIndex_Supported_KeyAttributes_All_Scenes, "Supported Key Attributes All Scenes", "", false, false, 3, items, 0, 0 );
		        }
		    }
		    if ( GetVersion() >= 2 )
		    {
		        if ( identical )
		        {
		            int keyAttributes = _data[3];
		            createSupportedKeyAttributesValues(keyAttributes,0,CentralSceneIndex_Supported_KeyAttributes_All_Scenes,_instance);
		        }
		        else
		        {
		            int numberOfBitMasks = (_data[2] & 0b00000110) >> 1;
		            for ( int i = 1; i <= numberOfBitMasks; i++ )
		            {
		                int keyAttributes = _data[2 +i];
		                createSupportedKeyAttributesValues(keyAttributes,i,CentralSceneIndex_Supported_KeyAttributes_All_Scenes+i,_instance);
		            }
		        }
		    }
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
		node->CreateValueInt( ValueID::ValueGenre_User, GetCommandClassId(), _instance, CentralSceneIndex_SceneCount, "Scene Count", "", true, false, 0, 0 );

        vector<ValueList::Item> items;
        unsigned int size = (sizeof(c_CentralScene_KeyAttributes)/sizeof(c_CentralScene_KeyAttributes[0]));
        for( unsigned int i=0; i < size; i++)
        {
            ValueList::Item item;
            item.m_label = c_CentralScene_KeyAttributes[i];
            item.m_value = i;
            items.push_back( item );
        }
        node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, CentralSceneIndex_Scene_KeyAttribute, "Scene KeyAttribute", "", false, false, size, items, 0, 0 );

        node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, CentralSceneIndex_SceneID, "Scene ID", "", true, false, 0, 0 );
        node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, CentralSceneIndex_SceneNumber, "Scene Number", "", true, false, 0, 0 );
        node->CreateValueButton(ValueID::ValueGenre_User, GetCommandClassId(), _instance, CentralSceneIndex_Button, "Button", 0 );
        node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, CentralSceneIndex_Scenes_Identical, "Scenes Identical", "", true, false, 0, 0 );
	}
}

void CentralScene::createSupportedKeyAttributesValues(uint8 keyAttributes, uint8 sceneNumber, uint8 index, uint8 instance)
{
    if( Node* node = GetNodeUnsafe() )
    {
        vector<ValueList::Item> items;

        for(int i = 0; i < 8; i++)
        {
            if ( keyAttributes & ( 1 << i) )
            {
                ValueList::Item item;
                item.m_label = c_CentralScene_KeyAttributes[i];
                item.m_value = i;
                items.push_back( item );
            }
        }

//        if ( keyAttributes & CentralSceneMask_KeyPressed1time)
//        {
//            ValueList::Item item;
//            item.m_label = c_CentralScene_KeyAttributes[0];
//            item.m_value = 0;
//            items.push_back( item );
//        }
//        if ( keyAttributes & CentralSceneMask_KeyReleased)
//        {
//            ValueList::Item item;
//            item.m_label = c_CentralScene_KeyAttributes[1];
//            item.m_value = 1;
//            items.push_back( item );
//        }
//        if ( keyAttributes & CentralSceneMask_HeldDown)
//        {
//            ValueList::Item item;
//            item.m_label = c_CentralScene_KeyAttributes[2];
//            item.m_value = 2;
//            items.push_back( item );
//        }
//        if ( keyAttributes & CentralSceneMask_KeyPressed2times)
//        {
//            ValueList::Item item;
//            item.m_label = c_CentralScene_KeyAttributes[3];
//            item.m_value = 3;
//            items.push_back( item );
//        }
//        if ( keyAttributes & CentralSceneMask_KeyPressed3times)
//        {
//            ValueList::Item item;
//            item.m_label = c_CentralScene_KeyAttributes[4];
//            item.m_value = 4;
//            items.push_back( item );
//        }
//        if ( keyAttributes & CentralSceneMask_KeyPressed4times)
//        {
//            ValueList::Item item;
//            item.m_label = c_CentralScene_KeyAttributes[5];
//            item.m_value = 5;
//            items.push_back( item );
//        }
//        if ( keyAttributes & CentralSceneMask_KeyPressed5times)
//        {
//            ValueList::Item item;
//            item.m_label = c_CentralScene_KeyAttributes[6];
//            item.m_value = 6;
//            items.push_back( item );
//        }
        if ( index == CentralSceneIndex_Supported_KeyAttributes_All_Scenes )
        {
            node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), instance, index, "Supported Key Attributes All Scenes", "", false, false, items.size(), items, 0, 0 );
        }
        else
        {
            char lbl[64];
            snprintf(lbl, 64, "Supported Key Attributes Scene %d", sceneNumber);
            node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), instance, index, lbl, "", false, false, items.size(), items, 0, 0 );
        }
    }
}