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
		"Inactive",
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
m_slowrefresh( false ),
m_sequence( 0 )
{
	m_dom.EnableFlag(STATE_FLAG_CS_SCENECOUNT, 0);
	m_dom.EnableFlag(STATE_FLAG_CS_CLEARTIMEOUT, 1000);
	Timer::SetDriver(GetDriver());
	SetStaticRequest( StaticRequest_Values );
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
	bool requests = false;
	if( (_requestFlags & RequestFlag_Static) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests = RequestValue( _requestFlags, CentralSceneCmd_Capability_Get, _instance, _queue );
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
		uint16 const _what,
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
		msg->Append( CentralSceneCmd_Capability_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
	}
	return true;
}
//-----------------------------------------------------------------------------
// <CentralScene::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------

bool CentralScene::SetValue
(
		Value const& _value
)
{
	if ((ValueID::ValueType_Int == _value.GetID().GetType()) && (_value.GetID().GetIndex() == ValueID_Index_CentralScene::ClearSceneTimeout)) {
		ValueInt const *value = static_cast<ValueInt const *>(&_value);
		m_dom.SetFlagInt(STATE_FLAG_CS_CLEARTIMEOUT, value->GetValue());
		return true;
	}
	return false;
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
		/* if the sequence number is the same as what we have recieved previously this is a retried packet */
		if (m_sequence == _data[1]) {
			Log::Write( LogLevel_Warning, GetNodeId(), "Recieved Duplicated Scene Notification. Dropping...");
			return true;
		}
		m_sequence = _data[1];
		uint8 keyAttribute = (_data[2] & 0x07);
		uint8 sceneID = _data[3];
		Log::Write( LogLevel_Info, GetNodeId(), "Received Central Scene set from node %d: scene id=%d with key Attribute %d. Sending event notification.", GetNodeId(), sceneID, keyAttribute);

		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, sceneID ) ) )
		{
			/* plus one, as we have our own "inactive" entry at index 0 */
			value->OnValueRefreshed( keyAttribute +1  );
			value->Release();
			/* Start up a Timer to set this back to Inactive */
			Log::Write( LogLevel_Info, GetNodeId(), "Automatically Clearing Scene %d in %dms", sceneID, m_dom.GetFlagInt(STATE_FLAG_CS_CLEARTIMEOUT) );
			if (m_TimersSet.find(sceneID) == m_TimersSet.end()) {
				m_TimersSet.insert(std::pair<uint32, uint32>(sceneID, _instance));
			} else {
				/* clear the Old Timer */
				TimerDelEvent(sceneID);
				/* no need to pop it off the list, as we will add it again below */
			}
			TimerThread::TimerCallback callback = bind(&CentralScene::ClearScene, this, sceneID);
			TimerSetEvent(m_dom.GetFlagInt(STATE_FLAG_CS_CLEARTIMEOUT), callback, sceneID);
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "No ValueID created for Scene %d", sceneID);
			return false;
		}
		return true;
	}
	else if (CentralSceneCmd_Capability_Report == (CentralSceneCmd)_data[0])
	{
		/* Create a Number of ValueID's based on the STATE_FLAG_CS_SCENECOUNT variable
		 * We prefer what the Config File specifies rather than what is returned by
		 * the Device...
		 */
		int scenecount = _data[1];
		if (m_dom.GetFlagByte(STATE_FLAG_CS_SCENECOUNT) == 0)
		{
			m_dom.SetFlagByte(STATE_FLAG_CS_SCENECOUNT, scenecount);
		}
		bool identical = true; //version 1 does not know this, so set it to true.
		if ( GetVersion() >= 2 )
		{
			identical = _data[2] & 0x01;
			Log::Write( LogLevel_Detail, GetNodeId(), "CentralScene: all scenes identical? %i",identical);
			if ( GetVersion() >= 3 )
				m_slowrefresh = (_data[2] & 0x80) == 1 ? true : false;
		}

		if ( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, ValueID_Index_CentralScene::SceneCount)))
		{
			value->OnValueRefreshed(m_dom.GetFlagByte(STATE_FLAG_CS_SCENECOUNT));
			value->Release();
		} else {
			Log::Write( LogLevel_Warning, GetNodeId(), "Can't find ValueID for SceneCount");
		}

		for (int sceneID = 1; sceneID <= m_dom.GetFlagByte(STATE_FLAG_CS_SCENECOUNT) ; sceneID++) {
			if ( GetVersion() == 1 )
			{
				// version 1 does not tell us which keyAttributes are supported, but only single press, released and held down are supported, so add these 3
				if( Node* node = GetNodeUnsafe() )
				{
					vector<ValueList::Item> items;
					for( unsigned int i=0; i < 4; i++)
					{
						ValueList::Item item;
						item.m_label = c_CentralScene_KeyAttributes[i];
						item.m_value = i;
						items.push_back( item );
					}
					char lbl[64];
					snprintf(lbl, 64, "Scene %d", sceneID);
					node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, sceneID, lbl, "", true, false, 3, items, 0, 0 );
				}
			}
			if ( GetVersion() >= 2 )
			{
				if ( identical )
				{
					int keyAttributes = _data[3];
					createSupportedKeyAttributesValues(keyAttributes,sceneID,_instance);
				}
				else
				{
					int keyAttributes = _data[2 +sceneID];
					createSupportedKeyAttributesValues(keyAttributes, sceneID,_instance);
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
		node->CreateValueInt( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_CentralScene::SceneCount, "Scene Count", "", true, false, 0, 0 );
		node->CreateValueInt( ValueID::ValueGenre_Config, GetCommandClassId(), _instance, ValueID_Index_CentralScene::ClearSceneTimeout, "Scene Reset Timeout", "", false, false, m_dom.GetFlagInt(STATE_FLAG_CS_CLEARTIMEOUT), 0);
	}
}

void CentralScene::createSupportedKeyAttributesValues
(
		uint8 keyAttributes,
		uint8 sceneNumber,
		uint8 instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		vector<ValueList::Item> items;
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[0];
			item.m_value = 0;
			items.push_back( item );
		}

		if ( keyAttributes & CentralSceneMask_KeyPressed1time)
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[1];
			item.m_value = 1;
			items.push_back( item );
		}
		if ( keyAttributes & CentralSceneMask_KeyReleased)
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[2];
			item.m_value = 2;
			items.push_back( item );
		}
		if ( keyAttributes & CentralSceneMask_HeldDown)
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[3];
			item.m_value = 3;
			items.push_back( item );
		}
		if ( keyAttributes & CentralSceneMask_KeyPressed2times)
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[4];
			item.m_value = 4;
			items.push_back( item );
		}
		if ( keyAttributes & CentralSceneMask_KeyPressed3times)
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[5];
			item.m_value = 5;
			items.push_back( item );
		}
		if ( keyAttributes & CentralSceneMask_KeyPressed4times)
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[6];
			item.m_value = 6;
			items.push_back( item );
		}
		if ( keyAttributes & CentralSceneMask_KeyPressed5times)
		{
			ValueList::Item item;
			item.m_label = c_CentralScene_KeyAttributes[7];
			item.m_value = 7;
			items.push_back( item );
		}
		char lbl[64];
		snprintf(lbl, 64, "Scene %d", sceneNumber);
		node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), instance, sceneNumber, lbl, "", true, false, (uint8_t)(items.size() & 0xFF), items, 0, 0 );
	}
}

void CentralScene::ClearScene
(
		uint32 sceneID
)
{

	uint8 _instance;
	if (m_TimersSet.find(sceneID) != m_TimersSet.end()) {
		_instance = m_TimersSet.at(sceneID);
	} else {
		Log::Write( LogLevel_Warning, "Can't find Timer in TimerSet List");
		return;
	}

	if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, sceneID ) ) )
	{
		/* plus one, as we have our own "inactive" entry at index 0 */
		value->OnValueRefreshed( 0 );
		value->Release();
	}
	m_TimersSet.erase(sceneID);

}
