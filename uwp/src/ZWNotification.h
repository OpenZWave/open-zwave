#pragma once

#include "Manager.h"
#include "Value.h"
#include "ValueStore.h"
#include "ValueID.h"
#include "ValueBool.h"
#include "ValueInt.h"
#include "ValueByte.h"
#include "ValueString.h"
#include "ValueShort.h"
#include "ValueDecimal.h"
#include "Notification.h"
#include "ZWValueID.h"

using namespace OpenZWave;

namespace OpenZWaveUWP
{
	public enum class NotificationType
	{
		ValueAdded = Notification::Type_ValueAdded,
		ValueRemoved = Notification::Type_ValueRemoved,
		ValueChanged = Notification::Type_ValueChanged,
		ValueRefreshed = Notification::Type_ValueRefreshed,
		Group = Notification::Type_Group,
		NodeNew = Notification::Type_NodeNew,
		NodeAdded = Notification::Type_NodeAdded,
		NodeRemoved = Notification::Type_NodeRemoved,
		NodeReset = Notification::Type_NodeReset,
		NodeProtocolInfo = Notification::Type_NodeProtocolInfo,
		NodeNaming = Notification::Type_NodeNaming,
		NodeEvent = Notification::Type_NodeEvent,
		PollingDisabled = Notification::Type_PollingDisabled,
		PollingEnabled = Notification::Type_PollingEnabled,
		SceneEvent = Notification::Type_SceneEvent,
		CreateButton = Notification::Type_CreateButton,
		DeleteButton = Notification::Type_DeleteButton,
		ButtonOn = Notification::Type_ButtonOn,
		ButtonOff = Notification::Type_ButtonOff,
		DriverReady = Notification::Type_DriverReady,
		DriverFailed = Notification::Type_DriverFailed,
		DriverReset = Notification::Type_DriverReset,
		EssentialNodeQueriesComplete = Notification::Type_EssentialNodeQueriesComplete,
		NodeQueriesComplete = Notification::Type_NodeQueriesComplete,
		AwakeNodesQueried = Notification::Type_AwakeNodesQueried,
		AllNodesQueriedSomeDead = Notification::Type_AllNodesQueriedSomeDead,
		AllNodesQueried = Notification::Type_AllNodesQueried,
		Notification = Notification::Type_Notification,
		DriverRemoved = Notification::Type_DriverRemoved,
		ControllerCommand = Notification::Type_ControllerCommand
	};

	public enum class NotificationCode
	{
		MsgComplete = Notification::Code_MsgComplete,
		Timeout = Notification::Code_Timeout,
		NoOperation = Notification::Code_NoOperation,
		Awake = Notification::Code_Awake,
		Sleep = Notification::Code_Sleep,
		Dead = Notification::Code_Dead,
		Alive = Notification::Code_Alive
	};

	public ref class ZWNotification sealed
	{
	internal:
		ZWNotification(Notification* notification)
		{
			m_type = (NotificationType)notification->GetType();
			m_byte = notification->GetByte();

			//Check if notification is either NodeEvent or ControllerCommand, otherwise GetEvent() will fail
			if ((m_type == NotificationType::NodeEvent) || (m_type == NotificationType::ControllerCommand))
			{
				m_event = notification->GetEvent();
			}

			m_valueId = ref new ZWValueID(notification->GetValueID());
		}

	public:
		NotificationType GetType() { return m_type; }
		NotificationCode GetCode() { return (NotificationCode)m_byte; }
		uint32 GetHomeId() { return m_valueId->GetHomeId(); }
		uint8 GetNodeId() { return m_valueId->GetNodeId(); }
		ZWValueID^ GetValueID() { return m_valueId; }
		uint8 GetGroupIdx() { assert(NotificationType::Group == m_type); return m_byte; }
		uint8 GetEvent() { return m_event; }
		uint8 GetByte() { return m_byte; }

	internal:
		NotificationType		m_type;
		ZWValueID^	m_valueId;
		uint8		m_byte;
		uint8		m_event;
	};
}