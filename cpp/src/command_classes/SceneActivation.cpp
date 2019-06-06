//-----------------------------------------------------------------------------
//
//	SceneActivation.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SCENE_ACTIVATION
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
#include "command_classes/SceneActivation.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Notification.h"
#include "platform/Log.h"
#include "value_classes/ValueInt.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum SceneActivationCmd
			{
				SceneActivationCmd_Set = 0x01
			};

//-----------------------------------------------------------------------------
// <SceneActivation::SceneActivation>
// Constructor
//-----------------------------------------------------------------------------
			SceneActivation::SceneActivation(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId)
			{
				Timer::SetDriver(GetDriver());
			}

//-----------------------------------------------------------------------------
// <SceneActivation::HandleIncommingMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool SceneActivation::HandleIncomingMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (SceneActivationCmd_Set == (SceneActivationCmd) _data[0])
				{
					// Scene Activation Set received so send notification
					char msg[64];
					uint32 duration;
					if (_data[2] == 0)
					{
						snprintf(msg, sizeof(msg), "now");
						duration = 0;
					}
					else if (_data[2] <= 0x7F)
					{
						snprintf(msg, sizeof(msg), "%d seconds", _data[2]);
						duration = _data[2];
					}
					else if (_data[2] <= 0xFE)
					{
						snprintf(msg, sizeof(msg), "%d minutes", _data[2]);
						duration = _data[2] * 60;
					}
					else
					{
						snprintf(msg, sizeof(msg), "via configuration");
						duration = 0;
					}
					Log::Write(LogLevel_Info, GetNodeId(), "Received SceneActivation set from node %d: scene id=%d %s. Sending event notification.", GetNodeId(), _data[1], msg);
					/* Sending the Scene ID via a notification should be depreciated in 1.8 */
					Notification* notification = new Notification(Notification::Type_SceneEvent);
					notification->SetHomeAndNodeIds(GetHomeId(), GetNodeId());
					notification->SetSceneId(_data[1]);
					GetDriver()->QueueNotification(notification);

					Log::Write(LogLevel_Info, GetNodeId(), "Received SceneActivation report: %d (duration: %d)", _data[1], duration);
					if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_SceneActivation::SceneID)))
					{
						value->OnValueRefreshed(_data[1]);
						value->Release();
					}
					if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_SceneActivation::Duration)))
					{
						value->OnValueRefreshed(duration);
						value->Release();
					}
					if (duration < 1000)
						duration = 1000;
					else
						duration = duration * 1000;

					Log::Write(LogLevel_Info, GetNodeId(), "Automatically Clearing SceneID/Duration in %d ms", duration);
					TimerThread::TimerCallback callback = bind(&SceneActivation::ClearScene, this, _instance);
					TimerSetEvent(duration, callback, _instance);
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <SceneActivation::HandleIncommingMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool SceneActivation::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				Log::Write(LogLevel_Warning, GetNodeId(), "Received a SceneActivation Message for a Controlled CommandClass");
				return false;
			}

//-----------------------------------------------------------------------------
// <SceneActivation::ClearScene>
// Return the Scene ValueID's to defaults, after the duration has expired
//-----------------------------------------------------------------------------
			void SceneActivation::ClearScene(uint32 _instance)
			{
				if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_SceneActivation::SceneID)))
				{
					value->OnValueRefreshed(0);
					value->Release();
				}
				if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(_instance, ValueID_Index_SceneActivation::Duration)))
				{
					value->OnValueRefreshed(0);
					value->Release();
				}
			}

//-----------------------------------------------------------------------------
// <SceneActivation::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void SceneActivation::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SceneActivation::SceneID, "Scene", "", true, false, 0, 0);
					node->CreateValueInt(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_SceneActivation::Duration, "Duration", "", true, false, 0, 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
