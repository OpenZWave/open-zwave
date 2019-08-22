//-----------------------------------------------------------------------------
//
//	Driver.cpp
//
//	Communicates with a Z-Wave network
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
#include "Defs.h"
#include "Driver.h"
#include "Options.h"
#include "Manager.h"
#include "Node.h"
#include "Msg.h"
#include "Notification.h"
#include "Scene.h"
#include "ZWSecurity.h"
#include "DNSThread.h"
#include "TimerThread.h"
#include "Http.h"
#include "ManufacturerSpecificDB.h"

#include "platform/Event.h"
#include "platform/Mutex.h"
#include "platform/SerialController.h"
#ifdef USE_HID
#ifdef WINRT
#include "platform/winRT/HidControllerWinRT.h"
#else
#include "platform/HidController.h"
#endif
#endif
#include "platform/Thread.h"
#include "platform/Log.h"
#include "platform/TimeStamp.h"

#include "command_classes/CommandClasses.h"
#include "command_classes/ApplicationStatus.h"
#include "command_classes/ControllerReplication.h"
#include "command_classes/Security.h"
#include "command_classes/WakeUp.h"
#include "command_classes/SwitchAll.h"
#include "command_classes/ManufacturerSpecific.h"
#include "command_classes/NoOperation.h"

#include "value_classes/ValueID.h"
#include "value_classes/Value.h"
#include "value_classes/ValueStore.h"

#include "tinyxml.h"

#include "Utils.h"
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
# include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * x)
#endif
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace OpenZWave;

// Version numbering for saved configurations. Any change that will invalidate
// previously saved configurations must be accompanied by an increment to the
// version number, and a comment explaining the date of, and reason for, the change.
//
// 01: 12-31-2010 - Introduced config version numbering due to ValueID format change.
// 02: 01-12-2011 - Command class m_afterMark sense corrected, and attribute named to match.
// 03: 08-04-2011 - Changed command class instance handling for non-sequential MultiChannel endpoints.
// 04: 12-07-2019 - Changed Interview Order
uint32 const c_configVersion = 4;

static char const* c_libraryTypeNames[] =
{ "Unknown",			// library type 0
		"Static Controller",		// library type 1
		"Controller",       		// library type 2
		"Enhanced Slave",   		// library type 3
		"Slave",            		// library type 4
		"Installer",			// library type 5
		"Routing Slave",		// library type 6
		"Bridge Controller",    	// library type 7
		"Device Under Test"		// library type 8
		};

static char const* c_controllerCommandNames[] =
{ "None", "Add Device", "Create New Primary", "Receive Configuration", "Remove Device", "Remove Failed Node", "Has Node Failed", "Replace Failed Node", "Transfer Primary Role", "Request Network Update", "Request Node Neighbor Update", "Assign Return Route", "Delete All Return Routes", "Send Node Information", "Replication Send", "Create Button", "Delete Button" };

static char const* c_sendQueueNames[] =
{ "Command", "NoOp", "Controller", "WakeUp", "Send", "Query", "Poll" };

//-----------------------------------------------------------------------------
// <Driver::Driver>
// Constructor
//-----------------------------------------------------------------------------
Driver::Driver(string const& _controllerPath, ControllerInterface const& _interface) :
		m_driverThread(new Internal::Platform::Thread("driver")), m_dns(new Internal::DNSThread(this)), m_dnsThread(new Internal::Platform::Thread("dns")), m_initMutex(new Internal::Platform::Mutex()), m_exit(false), m_init(false), m_awakeNodesQueried(false), m_allNodesQueried(false), m_notifytransactions(false), m_timer(new Internal::TimerThread(this)), m_timerThread(new Internal::Platform::Thread("timer")), m_controllerInterfaceType(_interface), m_controllerPath(_controllerPath), m_controller(
				NULL), m_homeId(0), m_libraryVersion(""), m_libraryTypeName(""), m_libraryType(0), m_manufacturerId(0), m_productType(0), m_productId(0), m_initVersion(0), m_initCaps(0), m_controllerCaps(0), m_Controller_nodeId(0), m_nodeMutex(new Internal::Platform::Mutex()), m_controllerReplication( NULL), m_transmitOptions( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE), m_waitingForAck(false), m_expectedCallbackId(0), m_expectedReply(0), m_expectedCommandClassId(
				0), m_expectedNodeId(0), m_pollThread(new Internal::Platform::Thread("poll")), m_pollMutex(new Internal::Platform::Mutex()), m_pollInterval(0), m_bIntervalBetweenPolls(false),				// if set to true (via SetPollInterval), the pollInterval will be interspersed between each poll (so a much smaller m_pollInterval like 100, 500, or 1,000 may be appropriate)
		m_currentControllerCommand( NULL), m_SUCNodeId(0), m_controllerResetEvent( NULL), m_sendMutex(new Internal::Platform::Mutex()), m_currentMsg( NULL), m_virtualNeighborsReceived(false), m_notificationsEvent(new Internal::Platform::Event()), m_SOFCnt(0), m_ACKWaiting(0), m_readAborts(0), m_badChecksum(0), m_readCnt(0), m_writeCnt(0), m_CANCnt(0), m_NAKCnt(0), m_ACKCnt(0), m_OOFCnt(0), m_dropped(0), m_retries(0), m_callbacks(0), m_badroutes(0), m_noack(0), m_netbusy(0), m_notidle(0), m_txverified(
				0), m_nondelivery(0), m_routedbusy(0), m_broadcastReadCnt(0), m_broadcastWriteCnt(0), AuthKey(0), EncryptKey(0), m_nonceReportSent(0), m_nonceReportSentAttempt(0), m_queueMsgEvent(new Internal::Platform::Event()), m_eventMutex(new Internal::Platform::Mutex())
{
	// set a timestamp to indicate when this driver started
	Internal::Platform::TimeStamp m_startTime;

	// Create the message queue events
	for (int32 i = 0; i < MsgQueue_Count; ++i)
	{
		m_queueEvent[i] = new Internal::Platform::Event();
	}

	// Clear the nodes array
	memset(m_nodes, 0, sizeof(Node*) * 256);

	// Clear the virtual neighbors array
	memset(m_virtualNeighbors, 0, NUM_NODE_BITFIELD_BYTES);

	// Initialize the Network Keys

	initNetworkKeys(false);

#ifdef USE_HID
	if( ControllerInterface_Hid == _interface )
	{
		m_controller = new Internal::Platform::HidController();
	}
	else
#endif
	{
		m_controller = new Internal::Platform::SerialController();
	}
	m_controller->SetSignalThreshold(1);

	Options::Get()->GetOptionAsBool("NotifyTransactions", &m_notifytransactions);
	Options::Get()->GetOptionAsInt("PollInterval", &m_pollInterval);
	Options::Get()->GetOptionAsBool("IntervalBetweenPolls", &m_bIntervalBetweenPolls);

	m_httpClient = new Internal::HttpClient(this);

	m_mfs = Internal::ManufacturerSpecificDB::Create();

	CheckMFSConfigRevision();

}

//-----------------------------------------------------------------------------
// <Driver::Driver>
// Destructor
//-----------------------------------------------------------------------------
Driver::~Driver()
{

	/* Signal that we are going away... so at least Apps know... */
	Notification* notification = new Notification(Notification::Type_DriverRemoved);
	notification->SetHomeAndNodeIds(m_homeId, 0);
	QueueNotification(notification);
	NotifyWatchers();

	// append final driver stats output to the log file
	LogDriverStatistics();

	// Save the driver config before deleting anything else
	bool save;
	if (Options::Get()->GetOptionAsBool("SaveConfiguration", &save))
	{
		if (save)
		{
			WriteCache();
			Internal::Scene::WriteXML("zwscene.xml");
		}
	}

	// The order of the statements below has been achieved by mitigating freed memory
	//references using a memory allocator checker. Do not rearrange unless you are
	//certain memory won't be referenced out of order. --Greg Satz, April 2010
	m_initMutex->Lock();
	m_exit = true;
	m_initMutex->Unlock();

	m_pollThread->Stop();
	m_pollThread->Release();

	m_dnsThread->Stop();
	m_dnsThread->Release();
	delete m_dns;

	m_driverThread->Stop();
	m_driverThread->Release();

	m_timerThread->Stop();
	m_timerThread->Release();
	delete m_timer;

	m_sendMutex->Release();

	m_controller->Close();
	m_controller->Release();

	m_initMutex->Release();

	if (m_currentMsg != NULL)
	{
		RemoveCurrentMsg();
	}

	// Clear the node data
	{
		Internal::LockGuard LG(m_nodeMutex);
		for (int i = 0; i < 256; ++i)
		{
			if (GetNodeUnsafe(i))
			{
				delete m_nodes[i];
				m_nodes[i] = NULL;
				Notification* notification = new Notification(Notification::Type_NodeRemoved);
				notification->SetHomeAndNodeIds(m_homeId, i);
				QueueNotification(notification);
			}
		}
	}
	// Don't release until all nodes have removed their poll values
	m_pollMutex->Release();

	// Clear the send Queue
	for (int32 i = 0; i < MsgQueue_Count; ++i)
	{
		while (!m_msgQueue[i].empty())
		{
			MsgQueueItem const& item = m_msgQueue[i].front();
			if (MsgQueueCmd_SendMsg == item.m_command)
			{
				delete item.m_msg;
			}
			else if (MsgQueueCmd_Controller == item.m_command)
			{
				delete item.m_cci;
			}
			m_msgQueue[i].pop_front();
		}

		m_queueEvent[i]->Release();
	}
	/* Doing our Notification Call back here in the destructor is just asking for trouble
	 * as there is a good chance that the application will do some sort of GetDriver() supported
	 * method on the Manager Class, which by this time, most of the OZW Classes associated with the
	 * Driver class is 99% destructed. (mainly nodes, which cascade to CC, which cascade to ValueID
	 * classes etc). We might need a flag around the Manager::GetDriver() class that stops applications
	 * from getting a half destructed Driver Reference, but still retain a Internal GetDriver() method
	 * that can return half destructed Driver references for internal classes (as per Greg's note above)
	 */
	bool notify;
	if (Options::Get()->GetOptionAsBool("NotifyOnDriverUnload", &notify))
	{
		if (notify)
		{
			NotifyWatchers();
		}
	}

	list<Notification*>::iterator nit = m_notifications.begin();
	while (nit != m_notifications.end())
	{
		Notification* notification = m_notifications.front();
		m_notifications.pop_front();
		delete notification;
		nit = m_notifications.begin();
	}

	if (m_controllerReplication)
		delete m_controllerReplication;

	m_notificationsEvent->Release();
	m_nodeMutex->Release();
	m_queueMsgEvent->Release();
	m_eventMutex->Release();
	delete this->AuthKey;
	delete this->EncryptKey;
	delete this->m_httpClient;
}

//-----------------------------------------------------------------------------
// <Driver::Start>
// Start the driver thread
//-----------------------------------------------------------------------------
void Driver::Start()
{
	// Start the thread that will handle communications with the Z-Wave network
	m_driverThread->Start(Driver::DriverThreadEntryPoint, this);
	m_dnsThread->Start(Internal::DNSThread::DNSThreadEntryPoint, m_dns);
	m_timerThread->Start(Internal::TimerThread::TimerThreadEntryPoint, m_timer);
}

//-----------------------------------------------------------------------------
// <Driver::DriverThreadEntryPoint>
// Entry point of the thread for creating and managing the worker threads
//-----------------------------------------------------------------------------
void Driver::DriverThreadEntryPoint(Internal::Platform::Event* _exitEvent, void* _context)
{
	Driver* driver = (Driver*) _context;
	if (driver)
	{
		driver->DriverThreadProc(_exitEvent);
	}
}

//-----------------------------------------------------------------------------
// <Driver::DriverThreadProc>
// Create and manage the worker threads
//-----------------------------------------------------------------------------
void Driver::DriverThreadProc(Internal::Platform::Event* _exitEvent)
{
#define WAITOBJECTCOUNT 11

	uint32 attempts = 0;
	bool mfsisReady = false;
	while (true)
	{
		if (Init(attempts))
		{
			// Driver has been initialised
			Internal::Platform::Wait* waitObjects[WAITOBJECTCOUNT];
			waitObjects[0] = _exitEvent;						// Thread must exit.
			waitObjects[1] = m_notificationsEvent;				// Notifications waiting to be sent.
			waitObjects[2] = m_queueMsgEvent;
			;					// a DNS and HTTP Event
			waitObjects[3] = m_controller;					    // Controller has received data.
			waitObjects[4] = m_queueEvent[MsgQueue_Command];	// A controller command is in progress.
			waitObjects[5] = m_queueEvent[MsgQueue_NoOp];		// Send device probes and diagnostics messages
			waitObjects[6] = m_queueEvent[MsgQueue_Controller];	// A multi-part controller command is in progress
			waitObjects[7] = m_queueEvent[MsgQueue_WakeUp];		// A node has woken. Pending messages should be sent.
			waitObjects[8] = m_queueEvent[MsgQueue_Send];		// Ordinary requests to be sent.
			waitObjects[9] = m_queueEvent[MsgQueue_Query];		// Node queries are pending.
			waitObjects[10] = m_queueEvent[MsgQueue_Poll];		// Poll request is waiting.

			Internal::Platform::TimeStamp retryTimeStamp;
			int retryTimeout = RETRY_TIMEOUT;
			Options::Get()->GetOptionAsInt("RetryTimeout", &retryTimeout);
			//retryTimeout = RETRY_TIMEOUT * 10;
			while (true)
			{
				Log::Write(LogLevel_StreamDetail, "      Top of DriverThreadProc loop.");
				uint32 count = WAITOBJECTCOUNT;
				int32 timeout = Internal::Platform::Wait::Timeout_Infinite;

				// if the ManufacturerDB class is setting up, we can't do anything yet
				if (mfsisReady == false)
				{
					count = 3;

					// If we're waiting for a message to complete, we can only
					// handle incoming data, notifications, DNS/HTTP  and exit events.
				}
				else if (m_waitingForAck || m_expectedCallbackId || m_expectedReply)
				{
					count = 4;
					timeout = m_waitingForAck ? ACK_TIMEOUT : retryTimeStamp.TimeRemaining();
					if (timeout < 0)
					{
						timeout = 0;
					}
				}
				else if (m_currentControllerCommand != NULL)
				{
					count = 7;
				}
				else
				{
					Log::QueueClear();							// clear the log queue when starting a new message
				}

				// Wait for something to do
				int32 res = Internal::Platform::Wait::Multiple(waitObjects, count, timeout);

				switch (res)
				{
					case -1:
					{
						// Wait has timed out - time to resend
						if (m_currentMsg != NULL)
						{
							Notification* notification = new Notification(Notification::Type_Notification);
							notification->SetHomeAndNodeIds(m_homeId, m_currentMsg->GetTargetNodeId());
							notification->SetNotification(Notification::Code_Timeout);
							QueueNotification(notification);
						}
						if (WriteMsg("Wait Timeout"))
						{
							retryTimeStamp.SetTime(retryTimeout);
						}
						break;
					}
					case 0:
					{
						// Exit has been signalled
						return;
					}
					case 1:
					{
						// Notifications are waiting to be sent
						NotifyWatchers();
						break;
					}
					case 2:
					{
						// a DNS or HTTP Event has occurred
						ProcessEventMsg();
						if (mfsisReady == false && m_mfs->isReady())
						{
							Notification* notification = new Notification(Notification::Type_ManufacturerSpecificDBReady);
							QueueNotification(notification);
							mfsisReady = true;
						}
						break;
					}
					case 3:
					{
						// Data has been received
						ReadMsg();
						break;
					}
					default:
					{
						// All the other events are sending message queue items
						if (WriteNextMsg((MsgQueue) (res - 4)))
						{
							retryTimeStamp.SetTime(retryTimeout);
						}
						break;
					}
				}
			}
		}

		++attempts;

		uint32 maxAttempts = 0;
		Options::Get()->GetOptionAsInt("DriverMaxAttempts", (int32 *) &maxAttempts);
		if (maxAttempts && (attempts >= maxAttempts))
		{
			Manager::Get()->Manager::SetDriverReady(this, false);
			NotifyWatchers();
			break;
		}

		if (attempts < 25)
		{
			// Retry every 5 seconds for the first two minutes
			if (Internal::Platform::Wait::Single(_exitEvent, 5000) == 0)
			{
				// Exit signalled.
				return;
			}
		}
		else
		{
			// Retry every 30 seconds after that
			if (Internal::Platform::Wait::Single(_exitEvent, 30000) == 0)
			{
				// Exit signalled.
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::Init>
// Initialize the controller
//-----------------------------------------------------------------------------
bool Driver::Init(uint32 _attempts)
{
	m_initMutex->Lock();

	if (m_exit)
	{
		m_initMutex->Unlock();
		return false;
	}

	m_Controller_nodeId = -1;
	m_waitingForAck = false;

	// Open the controller
	Log::Write(LogLevel_Info, "  Opening controller %s", m_controllerPath.c_str());

	if (!m_controller->Open(m_controllerPath))
	{
		Log::Write(LogLevel_Warning, "WARNING: Failed to init the controller (attempt %d)", _attempts);
		m_initMutex->Unlock();
		return false;
	}

	// Controller opened successfully, so we need to start all the worker threads
	m_pollThread->Start(Driver::PollThreadEntryPoint, this);

	// Send a NAK to the ZWave device
	uint8 nak = NAK;
	m_controller->Write(&nak, 1);

	// Get/set ZWave controller information in its preferred initialization order
	m_controller->PlayInitSequence(this);

	//If we ever want promiscuous mode uncomment this code.
	//Msg* msg = new Msg( "FUNC_ID_ZW_SET_PROMISCUOUS_MODE", 0xff, REQUEST, FUNC_ID_ZW_SET_PROMISCUOUS_MODE, false, false );
	//msg->Append( 0xff );
	//SendMsg( msg );

	m_initMutex->Unlock();

	// Init successful
	return true;
}

//-----------------------------------------------------------------------------
// <Driver::RemoveQueues>
// Clean up any messages to a node
//-----------------------------------------------------------------------------
void Driver::RemoveQueues(uint8 const _nodeId)
{
	if (m_currentMsg != NULL && m_currentMsg->GetTargetNodeId() == _nodeId)
	{
		RemoveCurrentMsg();
	}

	// Clear the send Queue
	for (int32 i = 0; i < MsgQueue_Count; ++i)
	{
		list<MsgQueueItem>::iterator it = m_msgQueue[i].begin();
		while (it != m_msgQueue[i].end())
		{
			bool remove = false;
			MsgQueueItem const& item = *it;
			if (MsgQueueCmd_SendMsg == item.m_command && _nodeId == item.m_msg->GetTargetNodeId())
			{
				delete item.m_msg;
				remove = true;
			}
			else if (MsgQueueCmd_QueryStageComplete == item.m_command && _nodeId == item.m_nodeId)
			{
				remove = true;
			}
			else if (MsgQueueCmd_Controller == item.m_command && _nodeId == item.m_cci->m_controllerCommandNode && m_currentControllerCommand != item.m_cci)
			{
				delete item.m_cci;
				remove = true;
			}
			else if (MsgQueueCmd_ReloadNode == item.m_command && _nodeId == item.m_nodeId)
			{
				remove = true;
			}
			if (remove)
			{
				it = m_msgQueue[i].erase(it);
			}
			else
			{
				++it;
			}
		}
		if (m_msgQueue[i].empty())
		{
			m_queueEvent[i]->Reset();
		}
	}
}

//-----------------------------------------------------------------------------
//	Configuration
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::ReadCache>
// Read our configuration from an XML document
//-----------------------------------------------------------------------------
bool Driver::ReadCache()
{
	char str[32];
	int32 intVal;

	// Load the XML document that contains the driver configuration
	string userPath;
	Options::Get()->GetOptionAsString("UserPath", &userPath);

	snprintf(str, sizeof(str), "ozwcache_0x%08x.xml", m_homeId);
	string filename = userPath + string(str);

	TiXmlDocument doc;
	if (!doc.LoadFile(filename.c_str(), TIXML_ENCODING_UTF8))
	{
		return false;
	}
	doc.SetUserData((void *) filename.c_str());
	TiXmlElement const* driverElement = doc.RootElement();

	char const *xmlns = driverElement->Attribute("xmlns");
	if (strcmp(xmlns, "https://github.com/OpenZWave/open-zwave"))
	{
		Log::Write(LogLevel_Warning, "Invalid XML Namespace. Ignoring %s", filename.c_str());
		return false;
	}

	// Version
	if (TIXML_SUCCESS != driverElement->QueryIntAttribute("version", &intVal) || (uint32) intVal != c_configVersion)
	{
		Log::Write(LogLevel_Warning, "WARNING: Driver::ReadCache - %s is from an older version of OpenZWave and cannot be loaded.", filename.c_str());
		return false;
	}

	// Capabilities
	if (TIXML_SUCCESS == driverElement->QueryIntAttribute("revision", &intVal))
	{
		m_mfs->setLatestRevision(intVal);
	}

	// Home ID
	char const* homeIdStr = driverElement->Attribute("home_id");
	if (homeIdStr)
	{
		char* p;
		uint32 homeId = (uint32) strtoul(homeIdStr, &p, 0);

		if (homeId != m_homeId)
		{
			Log::Write(LogLevel_Warning, "WARNING: Driver::ReadCache - Home ID in file %s is incorrect", filename.c_str());
			return false;
		}
	}
	else
	{
		Log::Write(LogLevel_Warning, "WARNING: Driver::ReadCache - Home ID is missing from file %s", filename.c_str());
		return false;
	}

	// Node ID
	if (TIXML_SUCCESS == driverElement->QueryIntAttribute("node_id", &intVal))
	{
		if ((uint8) intVal != m_Controller_nodeId)
		{
			Log::Write(LogLevel_Warning, "WARNING: Driver::ReadCache - Controller Node ID in file %s is incorrect", filename.c_str());
			return false;
		}
	}
	else
	{
		Log::Write(LogLevel_Warning, "WARNING: Driver::ReadCache - Node ID is missing from file %s", filename.c_str());
		return false;
	}

	// Capabilities
	if (TIXML_SUCCESS == driverElement->QueryIntAttribute("api_capabilities", &intVal))
	{
		m_initCaps = (uint8) intVal;
	}

	if (TIXML_SUCCESS == driverElement->QueryIntAttribute("controller_capabilities", &intVal))
	{
		m_controllerCaps = (uint8) intVal;
	}

	// Poll Interval
	if (TIXML_SUCCESS == driverElement->QueryIntAttribute("poll_interval", &intVal))
	{
		m_pollInterval = intVal;
	}

	// Poll Interval--between polls or period for polling the entire pollList?
	char const* cstr = driverElement->Attribute("poll_interval_between");
	if (cstr)
	{
		m_bIntervalBetweenPolls = !strcmp(cstr, "true");
	}

	// Read the nodes
	Internal::LockGuard LG(m_nodeMutex);
	TiXmlElement const* nodeElement = driverElement->FirstChildElement();
	while (nodeElement)
	{
		char const* str = nodeElement->Value();
		if (str && !strcmp(str, "Node"))
		{
			// Get the node Id from the XML
			if (TIXML_SUCCESS == nodeElement->QueryIntAttribute("id", &intVal))
			{
				uint8 nodeId = (uint8) intVal;
				Node* node = new Node(m_homeId, nodeId);
				m_nodes[nodeId] = node;

				Notification* notification = new Notification(Notification::Type_NodeAdded);
				notification->SetHomeAndNodeIds(m_homeId, nodeId);
				QueueNotification(notification);

				// Read the rest of the node configuration from the XML
				node->ReadXML(nodeElement);
			}
		}

		nodeElement = nodeElement->NextSiblingElement();
	}

	LG.Unlock();

	// restore the previous state (for now, polling) for the nodes/values just retrieved
	for (int i = 0; i < 256; i++)
	{
		if (m_nodes[i] != NULL)
		{
			Internal::VC::ValueStore* vs = m_nodes[i]->m_values;
			for (Internal::VC::ValueStore::Iterator it = vs->Begin(); it != vs->End(); ++it)
			{
				Internal::VC::Value* value = it->second;
				if (value->m_pollIntensity != 0)
					EnablePoll(value->GetID(), value->m_pollIntensity);
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Driver::WriteCache>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Driver::WriteCache()
{
	char str[32];

	if (!m_homeId)
	{
		Log::Write(LogLevel_Warning, "WARNING: Tried to write driver config with no home ID set");
		return;
	}
	Log::Write(LogLevel_Info, "Saving Cache");
	// Create a new XML document to contain the driver configuration
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	TiXmlElement* driverElement = new TiXmlElement("Driver");
	doc.LinkEndChild(decl);
	doc.LinkEndChild(driverElement);

	driverElement->SetAttribute("xmlns", "https://github.com/OpenZWave/open-zwave");

	snprintf(str, sizeof(str), "%d", c_configVersion);
	driverElement->SetAttribute("version", str);

	snprintf(str, sizeof(str), "%d", GetManufacturerSpecificDB()->getRevision());
	driverElement->SetAttribute("revision", str);

	snprintf(str, sizeof(str), "0x%.8x", m_homeId);
	driverElement->SetAttribute("home_id", str);

	snprintf(str, sizeof(str), "%d", m_Controller_nodeId);
	driverElement->SetAttribute("node_id", str);

	snprintf(str, sizeof(str), "%d", m_initCaps);
	driverElement->SetAttribute("api_capabilities", str);

	snprintf(str, sizeof(str), "%d", m_controllerCaps);
	driverElement->SetAttribute("controller_capabilities", str);

	snprintf(str, sizeof(str), "%d", m_pollInterval);
	driverElement->SetAttribute("poll_interval", str);

	snprintf(str, sizeof(str), "%s", m_bIntervalBetweenPolls ? "true" : "false");
	driverElement->SetAttribute("poll_interval_between", str);

	{
		Internal::LockGuard LG(m_nodeMutex);

		for (int i = 0; i < 256; ++i)
		{
			if (m_nodes[i])
			{
				if (m_nodes[i]->GetCurrentQueryStage() >= Node::QueryStage_CacheLoad)
				{
					m_nodes[i]->WriteXML(driverElement);
					Log::Write(LogLevel_Info, i, "Cache Save for Node %d as its QueryStage_CacheLoad", i);
				}
				else
				{
					Log::Write(LogLevel_Info, i, "Skipping Cache Save for Node %d as its not past QueryStage_CacheLoad", i);
				}
			}
		}
	}
	string userPath;
	Options::Get()->GetOptionAsString("UserPath", &userPath);

	snprintf(str, sizeof(str), "ozwcache_0x%08x.xml", m_homeId);
	string filename = userPath + string(str);

	doc.SaveFile(filename.c_str());
}

//-----------------------------------------------------------------------------
//	Controller
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::GetNodeUnsafe>
// Returns a pointer to the requested node without locking.
// Only to be used by main thread code.
//-----------------------------------------------------------------------------
Node* Driver::GetNodeUnsafe(uint8 _nodeId)
{
	if (Node* node = m_nodes[_nodeId])
	{
		return node;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// <Driver::GetNode>
// Locks the nodes and returns a pointer to the requested one
//-----------------------------------------------------------------------------
Node* Driver::GetNode(uint8 _nodeId)
{
	if (m_nodeMutex->IsSignalled())
	{
		Log::Write(LogLevel_Error, _nodeId, "Driver Thread is Not Locked during Call to GetNode");
		return NULL;
	}
	if (Node* node = m_nodes[_nodeId])
	{
		return node;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//	Sending Z-Wave messages
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::SendQueryStageComplete>
// Queue an item on the query queue that indicates a stage is complete
//-----------------------------------------------------------------------------
void Driver::SendQueryStageComplete(uint8 const _nodeId, Node::QueryStage const _stage)
{
	MsgQueueItem item;
	item.m_command = MsgQueueCmd_QueryStageComplete;
	item.m_nodeId = _nodeId;
	item.m_queryStage = _stage;
	item.m_retry = false;

	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		if (!node->IsListeningDevice())
		{
			if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(node->GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
			{
				if (!wakeUp->IsAwake())
				{
					// If the message is for a sleeping node, we queue it in the node itself.
					Log::Write(LogLevel_Info, "");
					Log::Write(LogLevel_Detail, node->GetNodeId(), "Queuing (%s) Query Stage Complete (%s)", c_sendQueueNames[MsgQueue_WakeUp], node->GetQueryStageName(_stage).c_str());
					wakeUp->QueueMsg(item);
					return;
				}
			}
		}

		// Non-sleeping node
		Log::Write(LogLevel_Detail, node->GetNodeId(), "Queuing (%s) Query Stage Complete (%s)", c_sendQueueNames[MsgQueue_Query], node->GetQueryStageName(_stage).c_str());
		m_sendMutex->Lock();
		m_msgQueue[MsgQueue_Query].push_back(item);
		m_queueEvent[MsgQueue_Query]->Set();
		m_sendMutex->Unlock();

	}
}

//-----------------------------------------------------------------------------
// <Driver::RetryQueryStageComplete>
// Request the current stage will be repeated
//-----------------------------------------------------------------------------
void Driver::RetryQueryStageComplete(uint8 const _nodeId, Node::QueryStage const _stage)
{
	MsgQueueItem item;
	item.m_command = MsgQueueCmd_QueryStageComplete;
	item.m_nodeId = _nodeId;
	item.m_queryStage = _stage;

	m_sendMutex->Lock();

	for (list<MsgQueueItem>::iterator it = m_msgQueue[MsgQueue_Query].begin(); it != m_msgQueue[MsgQueue_Query].end(); ++it)
	{
		if (*it == item)
		{
			(*it).m_retry = true;
			break;
		}
	}
	m_sendMutex->Unlock();
}

//-----------------------------------------------------------------------------
// <Driver::SendMsg>
// Queue a message to be sent to the Z-Wave PC Interface
//-----------------------------------------------------------------------------
void Driver::SendMsg(Internal::Msg* _msg, MsgQueue const _queue)
{
	MsgQueueItem item;

	item.m_command = MsgQueueCmd_SendMsg;
	item.m_msg = _msg;
	/* make sure the HomeId is Set on this message */
	_msg->SetHomeId(m_homeId);
	_msg->Finalize();
	{
		Internal::LockGuard LG(m_nodeMutex);
		if (Node* node = GetNode(_msg->GetTargetNodeId()))
		{
			/* if the node Supports the Security Class - check if this message is meant to be encapsulated */
			if (node->GetCommandClass(Internal::CC::Security::StaticGetCommandClassId()))
			{
				Internal::CC::CommandClass *cc = node->GetCommandClass(_msg->GetSendingCommandClass());
				if ((cc) && (cc->IsSecured()))
				{
					Log::Write(LogLevel_Detail, GetNodeNumber(_msg), "Setting Encryption Flag on Message For Command Class %s", cc->GetCommandClassName().c_str());
					item.m_msg->setEncrypted();
				}
			}

			// If the message is for a sleeping node, we queue it in the node itself.
			if (!node->IsListeningDevice())
			{
				if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(node->GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
				{
					if (!wakeUp->IsAwake())
					{
						Log::Write(LogLevel_Detail, "");
						// Handle saving multi-step controller commands
						if (m_currentControllerCommand != NULL)
						{
							Log::Write(LogLevel_Detail, GetNodeNumber(_msg), "Queuing (%s) %s", c_sendQueueNames[MsgQueue_Controller], c_controllerCommandNames[m_currentControllerCommand->m_controllerCommand]);
							delete _msg;
							item.m_command = MsgQueueCmd_Controller;
							item.m_cci = new ControllerCommandItem(*m_currentControllerCommand);
							item.m_msg = NULL;
							UpdateControllerState(ControllerState_Sleeping);
						}
						else
						{
							Log::Write(LogLevel_Detail, GetNodeNumber(_msg), "Queuing (%s) %s", c_sendQueueNames[MsgQueue_WakeUp], _msg->GetAsString().c_str());
						}
						wakeUp->QueueMsg(item);
						return;
					}
				}
			}
		}
	}
	Log::Write(LogLevel_Detail, GetNodeNumber(_msg), "Queuing (%s) %s", c_sendQueueNames[_queue], _msg->GetAsString().c_str());
	m_sendMutex->Lock();
	m_msgQueue[_queue].push_back(item);
	m_queueEvent[_queue]->Set();
	m_sendMutex->Unlock();
}

//-----------------------------------------------------------------------------
// <Driver::WriteNextMsg>
// Transmit a queued message to the Z-Wave controller
//-----------------------------------------------------------------------------
bool Driver::WriteNextMsg(MsgQueue const _queue)
{

	// There are messages to send, so get the one at the front of the queue
	m_sendMutex->Lock();
	MsgQueueItem item = m_msgQueue[_queue].front();

	if (MsgQueueCmd_SendMsg == item.m_command)
	{
		// Send a message
		m_currentMsg = item.m_msg;
		m_currentMsgQueueSource = _queue;
		m_msgQueue[_queue].pop_front();
		if (m_msgQueue[_queue].empty())
		{
			m_queueEvent[_queue]->Reset();
		}
		if (m_nonceReportSent > 0)
		{
			MsgQueueItem item_new;
			item_new.m_command = MsgQueueCmd_SendMsg;
			item_new.m_nodeId = item.m_msg->GetTargetNodeId();
			item_new.m_retry = item.m_retry;
			item_new.m_msg = new Internal::Msg(*item.m_msg);
			m_msgQueue[_queue].push_front(item_new);
			m_queueEvent[_queue]->Set();
		}
		m_sendMutex->Unlock();
		return WriteMsg("WriteNextMsg");
	}
	else if (MsgQueueCmd_QueryStageComplete == item.m_command)
	{
		// Move to the next query stage
		m_currentMsg = NULL;
		Node::QueryStage stage = item.m_queryStage;
		m_msgQueue[_queue].pop_front();
		if (m_msgQueue[_queue].empty())
		{
			m_queueEvent[_queue]->Reset();
		}
		m_sendMutex->Unlock();

		Node* node = GetNodeUnsafe(item.m_nodeId);
		if (node != NULL)
		{
			Log::Write(LogLevel_Detail, node->GetNodeId(), "Query Stage Complete (%s)", node->GetQueryStageName(stage).c_str());
			if (!item.m_retry)
			{
				node->QueryStageComplete(stage);
			}
			node->AdvanceQueries();
			return true;
		}
	}
	else if (MsgQueueCmd_Controller == item.m_command)
	{
		// Run a multi-step controller command
		m_currentControllerCommand = item.m_cci;
		m_sendMutex->Unlock();
		// Figure out if done with command
		if (m_currentControllerCommand->m_controllerCommandDone)
		{
			m_sendMutex->Lock();
			m_msgQueue[_queue].pop_front();
			if (m_msgQueue[_queue].empty())
			{
				m_queueEvent[_queue]->Reset();
			}
			m_sendMutex->Unlock();
			if (m_currentControllerCommand->m_controllerCallback)
			{
				m_currentControllerCommand->m_controllerCallback(m_currentControllerCommand->m_controllerState, m_currentControllerCommand->m_controllerReturnError, m_currentControllerCommand->m_controllerCallbackContext);
			}
			m_sendMutex->Lock();
			delete m_currentControllerCommand;
			m_currentControllerCommand = NULL;
			m_sendMutex->Unlock();
		}
		else if (m_currentControllerCommand->m_controllerState == ControllerState_Normal)
		{
			DoControllerCommand();
		}
		else if (m_currentControllerCommand->m_controllerStateChanged)
		{
			if (m_currentControllerCommand->m_controllerCallback)
			{
				m_currentControllerCommand->m_controllerCallback(m_currentControllerCommand->m_controllerState, m_currentControllerCommand->m_controllerReturnError, m_currentControllerCommand->m_controllerCallbackContext);
			}
			m_currentControllerCommand->m_controllerStateChanged = false;
		}
		else
		{
			Log::Write(LogLevel_Info, "WriteNextMsg Controller nothing to do");
			m_sendMutex->Lock();
			m_queueEvent[_queue]->Reset();
			m_sendMutex->Unlock();
		}
		return true;
	}
	else if (MsgQueueCmd_ReloadNode == item.m_command)
	{
		m_msgQueue[_queue].pop_front();
		if (m_msgQueue[_queue].empty())
		{
			m_queueEvent[_queue]->Reset();
		}
		m_sendMutex->Unlock();

		Log::Write(LogLevel_Info, item.m_nodeId, "Reloading Sleeping Node");
		/* this will reload the Node, ignoring any cache that exists etc */
		ReloadNode(item.m_nodeId);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Driver::WriteMsg>
// Transmit the current message to the Z-Wave controller
//-----------------------------------------------------------------------------
bool Driver::WriteMsg(string const &msg)
{
	if (!m_currentMsg)
	{
		Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "WriteMsg %s m_currentMsg=%08x", msg.c_str(), m_currentMsg);
		// We try not to hang when this happenes
		m_expectedCallbackId = 0;
		m_expectedCommandClassId = 0;
		m_expectedNodeId = 0;
		m_expectedReply = 0;
		m_waitingForAck = false;
		return false;
	}
	/* if this is called with m_nonceReportSent > 0 it means that we have
	 * tried to send a NONCE report and it timed out or was NAK'd
	 *
	 */
	uint8 attempts;
	uint8 nodeId;
	if (m_nonceReportSent > 0)
	{
		attempts = m_nonceReportSentAttempt++;
		nodeId = m_nonceReportSent;
	}
	else
	{
		attempts = m_currentMsg->GetSendAttempts();
		nodeId = m_currentMsg->GetTargetNodeId();
	}
	Internal::LockGuard LG(m_nodeMutex);
	Node* node = GetNode(nodeId);
	if (attempts >= m_currentMsg->GetMaxSendAttempts() || (node != NULL && !node->IsNodeAlive() && !m_currentMsg->IsNoOperation()))
	{
		if (node != NULL && !node->IsNodeAlive())
		{
			Log::Write(LogLevel_Error, nodeId, "ERROR: Dropping command because node is presumed dead");
		}
		else
		{
			// That's it - already tried to send GetMaxSendAttempt() times.
			Log::Write(LogLevel_Error, nodeId, "ERROR: Dropping command, expected response not received after %d attempt(s)", m_currentMsg->GetMaxSendAttempts());
		}
		if (m_currentControllerCommand != NULL)
		{
			/* its a ControllerCommand that is failed */
			UpdateControllerState(ControllerState_Error, ControllerError_Failed);

		}

		RemoveCurrentMsg();
		m_dropped++;
		return false;
	}

	if ((attempts != 0) && (m_nonceReportSent == 0))
	{
		// this is not the first attempt, so increment the callback id before sending
		m_currentMsg->UpdateCallbackId();
	}

	/* XXX TODO: Minor Bug - Due to the post increament of the SendAttempts, it means our final NONCE_GET will go though
	 * but the subsequent MSG send will fail (as the counter is incremented only upon a successful NONCE_GET, and Not a Send
	 *
	 */

	if (m_nonceReportSent == 0)
	{
		if (m_currentMsg->isEncrypted() && !m_currentMsg->isNonceRecieved())
		{
			m_currentMsg->SetSendAttempts(++attempts);
		}
		else if (!m_currentMsg->isEncrypted())
		{
			m_currentMsg->SetSendAttempts(++attempts);
		}
		m_expectedCallbackId = m_currentMsg->GetCallbackId();
		m_expectedCommandClassId = m_currentMsg->GetExpectedCommandClassId();
		m_expectedNodeId = m_currentMsg->GetTargetNodeId();
		m_expectedReply = m_currentMsg->GetExpectedReply();
		m_waitingForAck = true;
	}
	string attemptsstr = "";
	if (attempts > 1)
	{
		char buf[15];
		snprintf(buf, sizeof(buf), "Attempt %d, ", attempts);
		attemptsstr = buf;
		m_retries++;
		if (node != NULL)
		{
			node->m_retries++;
		}
	}

	Log::Write(LogLevel_Detail, "");

	if (m_nonceReportSent > 0 && node != NULL)
	{
		/* send a new NONCE report */
		SendNonceKey(m_nonceReportSent, node->GenerateNonceKey());
	}
	else if (m_currentMsg->isEncrypted())
	{
		if (m_currentMsg->isNonceRecieved())
		{
			Log::Write(LogLevel_Info, nodeId, "Processing (%s) Encrypted message (%sCallback ID=0x%.2x, Expected Reply=0x%.2x) - %s", c_sendQueueNames[m_currentMsgQueueSource], attemptsstr.c_str(), m_expectedCallbackId, m_expectedReply, m_currentMsg->GetAsString().c_str());
			SendEncryptedMessage();
		}
		else
		{
			Log::Write(LogLevel_Info, nodeId, "Processing (%s) Nonce Request message (%sCallback ID=0x%.2x, Expected Reply=0x%.2x)", c_sendQueueNames[m_currentMsgQueueSource], attemptsstr.c_str(), m_expectedCallbackId, m_expectedReply);
			SendNonceRequest(m_currentMsg->GetLogText());
		}
	}
	else
	{
		Log::Write(LogLevel_Info, nodeId, "Sending (%s) message (%sCallback ID=0x%.2x, Expected Reply=0x%.2x) - %s", c_sendQueueNames[m_currentMsgQueueSource], attemptsstr.c_str(), m_expectedCallbackId, m_expectedReply, m_currentMsg->GetAsString().c_str());
		uint32 bytesWritten = m_controller->Write(m_currentMsg->GetBuffer(), m_currentMsg->GetLength());

		if (bytesWritten == 0)
		{
			//0 will be returned when the port is closed or something bad happened
			//so send notification
			Notification* notification = new Notification(Notification::Type_DriverFailed);
			notification->SetHomeAndNodeIds(m_homeId, m_currentMsg->GetTargetNodeId());
			notification->SetComPort(m_controllerPath);
			QueueNotification(notification);
			NotifyWatchers();

			m_driverThread->Stop();
			return false;
		}
	}
	m_writeCnt++;

	if (nodeId == 0xff)
	{
		m_broadcastWriteCnt++; // not accurate since library uses 0xff for the controller too
	}
	else
	{
		if (node != NULL)
		{
			node->m_sentCnt++;
			node->m_sentTS.SetTime();
			if (m_expectedReply == FUNC_ID_APPLICATION_COMMAND_HANDLER)
			{
				Internal::CC::CommandClass *cc = node->GetCommandClass(m_expectedCommandClassId);
				if (cc != NULL)
				{
					cc->SentCntIncr();
				}
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// <Driver::RemoveCurrentMsg>
// Delete the current message
//-----------------------------------------------------------------------------
void Driver::RemoveCurrentMsg()
{
	Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "Removing current message");
	if (m_currentMsg != NULL)
	{
		delete m_currentMsg;
		m_currentMsg = NULL;
	}

	m_expectedCallbackId = 0;
	m_expectedCommandClassId = 0;
	m_expectedNodeId = 0;
	m_expectedReply = 0;
	m_waitingForAck = false;
	m_nonceReportSent = 0;
	m_nonceReportSentAttempt = 0;
}

//-----------------------------------------------------------------------------
// <Driver::MoveMessagesToWakeUpQueue>
// Move messages for a sleeping device to its wake-up queue
//-----------------------------------------------------------------------------
bool Driver::MoveMessagesToWakeUpQueue(uint8 const _targetNodeId, bool const _move)
{
	// If the target node is one that goes to sleep, transfer
	// all messages for it to its Wake-Up queue.
	if (Node* node = GetNodeUnsafe(_targetNodeId))
	{
		if (!node->IsListeningDevice() && !node->IsFrequentListeningDevice() && _targetNodeId != m_Controller_nodeId)
		{
			if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(node->GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
			{
				// Mark the node as asleep
				wakeUp->SetAwake(false);

				// If we need to save the messages
				if (_move)
				{
					// Move all messages for this node to the wake-up queue
					m_sendMutex->Lock();

					// See if we are working on a controller command
					if (m_currentControllerCommand)
					{
						// Don't save controller message as it will be recreated
						RemoveCurrentMsg();
					}

					// Then try the current message first
					if (m_currentMsg)
					{
						if (_targetNodeId == m_currentMsg->GetTargetNodeId())
						{
							// This message is for the unresponsive node
							// We do not move any "Wake Up No More Information"
							// commands or NoOperations to the pending queue.
							if (!m_currentMsg->IsWakeUpNoMoreInformationCommand() && !m_currentMsg->IsNoOperation())
							{
								Log::Write(LogLevel_Info, _targetNodeId, "Node not responding - moving message to Wake-Up queue: %s", m_currentMsg->GetAsString().c_str());
								/* reset the sendAttempts */
								m_currentMsg->SetSendAttempts(0);

								MsgQueueItem item;
								item.m_command = MsgQueueCmd_SendMsg;
								item.m_msg = m_currentMsg;
								wakeUp->QueueMsg(item);
							}
							else
							{
								delete m_currentMsg;
							}

							m_currentMsg = NULL;
							m_expectedCallbackId = 0;
							m_expectedCommandClassId = 0;
							m_expectedNodeId = 0;
							m_expectedReply = 0;
							m_waitingForAck = false;
						}
					}

					// Now the message queues
					for (int i = 0; i < MsgQueue_Count; ++i)
					{
						list<MsgQueueItem>::iterator it = m_msgQueue[i].begin();
						while (it != m_msgQueue[i].end())
						{
							bool remove = false;
							MsgQueueItem const& item = *it;
							if (MsgQueueCmd_SendMsg == item.m_command)
							{
								if (_targetNodeId == item.m_msg->GetTargetNodeId())
								{
									// This message is for the unresponsive node
									// We do not move any "Wake Up No More Information"
									// commands or NoOperations to the pending queue.
									if (!item.m_msg->IsWakeUpNoMoreInformationCommand() && !item.m_msg->IsNoOperation())
									{
										Log::Write(LogLevel_Info, item.m_msg->GetTargetNodeId(), "Node not responding - moving message to Wake-Up queue: %s", item.m_msg->GetAsString().c_str());
										/* reset any SendAttempts */
										item.m_msg->SetSendAttempts(0);
										wakeUp->QueueMsg(item);
									}
									else
									{
										delete item.m_msg;
									}
									remove = true;
								}
							}
							if (MsgQueueCmd_QueryStageComplete == item.m_command)
							{
								if (_targetNodeId == item.m_nodeId)
								{
									Log::Write(LogLevel_Info, _targetNodeId, "Node not responding - moving QueryStageComplete command to Wake-Up queue");

									wakeUp->QueueMsg(item);
									remove = true;
								}
							}
							if (MsgQueueCmd_Controller == item.m_command)
							{
								if (_targetNodeId == item.m_cci->m_controllerCommandNode)
								{
									Log::Write(LogLevel_Info, _targetNodeId, "Node not responding - moving controller command to Wake-Up queue: %s", c_controllerCommandNames[item.m_cci->m_controllerCommand]);

									wakeUp->QueueMsg(item);
									remove = true;
								}
							}

							if (remove)
							{
								it = m_msgQueue[i].erase(it);
							}
							else
							{
								++it;
							}
						}

						// If the queue is now empty, we need to clear its event
						if (m_msgQueue[i].empty())
						{
							m_queueEvent[i]->Reset();
						}
					}

					if (m_currentControllerCommand)
					{
						// Put command back on queue so it will be cleaned up
						UpdateControllerState(ControllerState_Sleeping);
						MsgQueueItem item;
						item.m_command = MsgQueueCmd_Controller;
						item.m_cci = new ControllerCommandItem(*m_currentControllerCommand);
						m_currentControllerCommand = item.m_cci;
						m_msgQueue[MsgQueue_Controller].push_back(item);
						m_queueEvent[MsgQueue_Controller]->Set();
					}

					m_sendMutex->Unlock();

					// Move completed successfully
					return true;
				}
			}
		}
	}

	// Failed to move messages
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::HandleErrorResponse>
// For messages that return a ZW_SEND_DATA response, process the results here
// If it is a non-listeing (sleeping) node return true.
//-----------------------------------------------------------------------------
bool Driver::HandleErrorResponse(uint8 const _error, uint8 const _nodeId, char const* _funcStr, bool _sleepCheck	// = 0
		)
{
	// Only called with a ZW_SEND_DATA error response. We count and output the message here.
	if (_error == TRANSMIT_COMPLETE_NOROUTE)
	{
		m_badroutes++;
		Log::Write(LogLevel_Info, _nodeId, "ERROR: %s failed. No route available.", _funcStr);
	}
	else if (_error == TRANSMIT_COMPLETE_NO_ACK)
	{
		m_noack++;
		Log::Write(LogLevel_Info, _nodeId, "WARNING: %s failed. No ACK received - device may be asleep.", _funcStr);
		if (m_currentMsg)
		{
			// In case the failure is due to the target being a sleeping node, we
			// first try to move its pending messages to its wake-up queue.
			if (MoveMessagesToWakeUpQueue(m_currentMsg->GetTargetNodeId(), _sleepCheck))
			{
				return true;
			}
			Log::Write(LogLevel_Warning, _nodeId, "WARNING: Device is not a sleeping node.");
		}
	}
	else if (_error == TRANSMIT_COMPLETE_FAIL)
	{
		m_netbusy++;
		Log::Write(LogLevel_Info, _nodeId, "ERROR: %s failed. Network is busy.", _funcStr);
	}
	else if (_error == TRANSMIT_COMPLETE_NOT_IDLE)
	{
		m_notidle++;
		Log::Write(LogLevel_Info, _nodeId, "ERROR: %s failed. Network is busy.", _funcStr);
	}
	else if (_error == TRANSMIT_COMPLETE_VERIFIED)
	{
		m_txverified++;
		Log::Write(LogLevel_Info, _nodeId, "ERROR: %s failed. Transmit Verified.", _funcStr);
	}
	if (Node* node = GetNodeUnsafe(_nodeId))
	{
		if (++node->m_errors >= 3)
		{
			node->SetNodeAlive(false);
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::CheckCompletedNodeQueries>
// Identify controller (as opposed to node) commands...especially blocking ones
//-----------------------------------------------------------------------------
void Driver::CheckCompletedNodeQueries()
{
	Log::Write(LogLevel_Warning, "CheckCompletedNodeQueries m_allNodesQueried=%d m_awakeNodesQueried=%d", m_allNodesQueried, m_awakeNodesQueried);
	if (!m_allNodesQueried)
	{
		bool all = true;
		bool sleepingOnly = true;
		bool deadFound = false;

		{
			Internal::LockGuard LG(m_nodeMutex);
			for (int i = 0; i < 256; ++i)
			{
				if (m_nodes[i])
				{
					if (m_nodes[i]->GetCurrentQueryStage() != Node::QueryStage_Complete)
					{
						if (!m_nodes[i]->IsNodeAlive())
						{
							deadFound = true;
							continue;
						}
						all = false;
						if (m_nodes[i]->IsListeningDevice())
						{
							sleepingOnly = false;
						}
					}
				}
			}
		}

		Log::Write(LogLevel_Warning, "CheckCompletedNodeQueries all=%d, deadFound=%d sleepingOnly=%d", all, deadFound, sleepingOnly);
		if (all)
		{
			if (deadFound)
			{
				// only dead nodes left to query
				Log::Write(LogLevel_Info, "         Node query processing complete except for dead nodes.");
				Notification* notification = new Notification(Notification::Type_AllNodesQueriedSomeDead);
				notification->SetHomeAndNodeIds(m_homeId, 0xff);
				QueueNotification(notification);
			}
			else
			{
				// no sleeping nodes, no dead nodes and no more nodes in the queue, so...All done
				Log::Write(LogLevel_Info, "         Node query processing complete.");
				Notification* notification = new Notification(Notification::Type_AllNodesQueried);
				notification->SetHomeAndNodeIds(m_homeId, 0xff);
				QueueNotification(notification);
			}
			m_awakeNodesQueried = true;
			m_allNodesQueried = true;
		}
		else if (sleepingOnly)
		{
			if (!m_awakeNodesQueried)
			{
				// only sleeping nodes remain, so signal awake nodes queried complete
				Log::Write(LogLevel_Info, "         Node query processing complete except for sleeping nodes.");
				Notification* notification = new Notification(Notification::Type_AwakeNodesQueried);
				notification->SetHomeAndNodeIds(m_homeId, 0xff);
				QueueNotification(notification);
				m_awakeNodesQueried = true;
			}
		}
	}
	WriteCache();
}

//-----------------------------------------------------------------------------
// <Driver::IsExpectedReply>
// Determine if the reply is from the node we are expecting.
//-----------------------------------------------------------------------------
bool Driver::IsExpectedReply(const uint8 _nodeId)
{
	// Accept all controller commands or where the protocol doesn't identify the actual node
	if (m_expectedNodeId == 255 || _nodeId == 0)
	{
		return true;
	}
	// Accept all messages that do not convey source node identification.
	if (m_expectedReply == FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO || m_expectedReply == FUNC_ID_ZW_REQUEST_NODE_INFO || m_expectedReply == FUNC_ID_ZW_GET_ROUTING_INFO || m_expectedReply == FUNC_ID_ZW_ASSIGN_RETURN_ROUTE || m_expectedReply == FUNC_ID_ZW_DELETE_RETURN_ROUTE || m_expectedReply == FUNC_ID_ZW_SEND_DATA || m_expectedReply == FUNC_ID_ZW_SEND_NODE_INFORMATION || m_expectedReply == FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE || m_expectedReply == FUNC_ID_ZW_ENABLE_SUC
			|| m_expectedReply == FUNC_ID_ZW_SET_SUC_NODE_ID || m_expectedReply == FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE_OPTIONS)
	{
		return true;
	}
	// Accept if source message contains node info and it is from the one we are expecting
	if (m_expectedNodeId == _nodeId)
	{
		return true;
	}
	Log::Write(LogLevel_Detail, "IsExpectedReply: m_expectedNodeId = %d m_expectedReply = %02x", m_expectedNodeId, m_expectedReply);
	return false;
}
//-----------------------------------------------------------------------------
//	Receiving Z-Wave messages
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::ReadMsg>
// Read data from the serial port
//-----------------------------------------------------------------------------
bool Driver::ReadMsg()
{
	uint8 buffer[1024];

	memset(buffer, 0, sizeof(uint8) * 1024);

	if (!m_controller->Read(buffer, 1))
	{
		// Nothing to read
		return false;
	}

	switch (buffer[0])
	{
		case SOF:
		{
			m_SOFCnt++;
			if (m_waitingForAck)
			{
				// This can happen on any normal network when a transmission overlaps an unexpected
				// reception and the data in the buffer doesn't contain the ACK. The controller will
				// notice and send us a CAN to retransmit.
				Log::Write(LogLevel_Detail, "Unsolicited message received while waiting for ACK.");
				m_ACKWaiting++;
			}

			// Read the length byte.  Keep trying until we get it.
			m_controller->SetSignalThreshold(1);
			int32 response = Internal::Platform::Wait::Single(m_controller, 50);
			if (response < 0)
			{
				Log::Write(LogLevel_Warning, "WARNING: 50ms passed without finding the length byte...aborting frame read");
				m_readAborts++;
				break;
			}

			m_controller->Read(&buffer[1], 1);
			m_controller->SetSignalThreshold(buffer[1]);
			if (Internal::Platform::Wait::Single(m_controller, 500) < 0)
			{
				Log::Write(LogLevel_Warning, "WARNING: 500ms passed without reading the rest of the frame...aborting frame read");
				m_readAborts++;
				m_controller->SetSignalThreshold(1);
				break;
			}

			m_controller->Read(&buffer[2], buffer[1]);
			m_controller->SetSignalThreshold(1);

			uint32 length = buffer[1] + 2;

			// Log the data
			string str = "";
			for (uint32 i = 0; i < length; ++i)
			{
				if (i)
				{
					str += ", ";
				}

				char byteStr[8];
				snprintf(byteStr, sizeof(byteStr), "0x%.2x", buffer[i]);
				str += byteStr;
			}
			uint8 nodeId = NodeFromMessage(buffer);
			if (nodeId == 0)
			{
				nodeId = GetNodeNumber(m_currentMsg);
			}
			Log::Write(LogLevel_Detail, nodeId, "  Received: %s", str.c_str());

			// Verify checksum
			uint8 checksum = 0xff;
			for (uint32 i = 1; i < (length - 1); ++i)
			{
				checksum ^= buffer[i];
			}

			if (buffer[length - 1] == checksum)
			{
				// Checksum correct - send ACK
				uint8 ack = ACK;
				m_controller->Write(&ack, 1);
				m_readCnt++;

				// Process the received message
				ProcessMsg(&buffer[2], length - 2);
			}
			else
			{
				Log::Write(LogLevel_Warning, nodeId, "WARNING: Checksum incorrect - sending NAK");
				m_badChecksum++;
				uint8 nak = NAK;
				m_controller->Write(&nak, 1);
				m_controller->Purge();
			}
			break;
		}

		case CAN:
		{
			// This is the other side of an unsolicited ACK. As mentioned there if we receive a message
			// just after we transmitted one, the controller will notice and tell us to retransmit here.
			// Don't increment the transmission counter as it is possible the message will never get out
			// on very busy networks with lots of unsolicited messages being received. Increase the amount
			// of retries but only up to a limit so we don't stay here forever.
			Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "CAN received...triggering resend");
			m_CANCnt++;
			if (m_currentMsg != NULL)
			{
				m_currentMsg->SetMaxSendAttempts(m_currentMsg->GetMaxSendAttempts() + 1);
			}
			else
			{
				Log::Write(LogLevel_Warning, "m_currentMsg was NULL when trying to set MaxSendAttempts");
				Log::QueueDump();
			}
			WriteMsg("CAN");
			break;
		}

		case NAK:
		{
			Log::Write(LogLevel_Warning, GetNodeNumber(m_currentMsg), "WARNING: NAK received...triggering resend");
			m_NAKCnt++;
			WriteMsg("NAK");
			break;
		}

		case ACK:
		{
			m_ACKCnt++;
			m_waitingForAck = false;
			if (m_currentMsg == NULL)
			{
				Log::Write(LogLevel_StreamDetail, 255, "  ACK received");
			}
			else
			{
				Log::Write(LogLevel_StreamDetail, GetNodeNumber(m_currentMsg), "  ACK received CallbackId 0x%.2x Reply 0x%.2x", m_expectedCallbackId, m_expectedReply);
				if ((0 == m_expectedCallbackId) && (0 == m_expectedReply))
				{
					// Remove the message from the queue, now that it has been acknowledged.
					RemoveCurrentMsg();
				}
			}
			break;
		}

		default:
		{
			Log::Write(LogLevel_Warning, "WARNING: Out of frame flow! (0x%.2x).  Sending NAK.", buffer[0]);
			m_OOFCnt++;
			uint8 nak = NAK;
			m_controller->Write(&nak, 1);
			m_controller->Purge();
			break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Driver::ProcessMsg>
// Process data received from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::ProcessMsg(uint8* _data, uint8 _length)
{
	bool handleCallback = true;
	bool wasencrypted = false;
	//uint8 nodeId = GetNodeNumber( m_currentMsg );

	if ((REQUEST == _data[0]) && (Internal::CC::Security::StaticGetCommandClassId() == _data[5]))
	{
		/* if this message is a NONCE Report - Then just Trigger the Encrypted Send */
		if (Internal::CC::SecurityCmd_NonceReport == _data[6])
		{
			Log::Write(LogLevel_Info, _data[3], "Received SecurityCmd_NonceReport from node %d", _data[3]);

			/* handle possible resends of NONCE_REPORT messages.... See Issue #931 */
			if (!m_currentMsg)
			{
				Log::Write(LogLevel_Warning, _data[3], "Received a NonceReport from node, but no pending messages. Dropping..");
				return;
			}

			// No Need to triger a WriteMsg here - It should be handled automatically
			m_currentMsg->setNonce(&_data[7]);
			this->SendEncryptedMessage();
			return;

			/* if this is a NONCE Get - Then call to the CC directly, process it, and then bail out. */
		}
		else if (Internal::CC::SecurityCmd_NonceGet == _data[6])
		{
			Log::Write(LogLevel_Info, _data[3], "Received SecurityCmd_NonceGet from node %d", _data[3]);
			{
				uint8 *nonce = NULL;
				Internal::LockGuard LG(m_nodeMutex);
				Node* node = GetNode(_data[3]);
				if (node)
				{
					nonce = node->GenerateNonceKey();
				}
				else
				{
					Log::Write(LogLevel_Warning, _data[3], "Couldn't Generate Nonce Key for Node %d", _data[3]);
					return;
				}

				SendNonceKey(_data[3], nonce);

			}
			/* don't continue processing */
			return;

			/* if this message is encrypted, decrypt it first */
		}
		else if ((Internal::CC::SecurityCmd_MessageEncap == _data[6]) || (Internal::CC::SecurityCmd_MessageEncapNonceGet == _data[6]))
		{
			uint8 _newdata[256];
			uint8 SecurityCmd = _data[6];
			uint8 *_nonce;

			/* clear out NONCE Report tracking */
			m_nonceReportSent = 0;
			m_nonceReportSentAttempt = 0;

			/* make sure the Node Exists, and it has the Security CC */
			{
				Internal::LockGuard LG(m_nodeMutex);
				Node* node = GetNode(_data[3]);
				if (node)
				{
					_nonce = node->GetNonceKey(_data[_data[4] - 4]);
					if (!_nonce)
					{
						Log::Write(LogLevel_Warning, _data[3], "Could Not Retrieve Nonce for Node %d", _data[3]);
						return;
					}
				}
				else
				{
					Log::Write(LogLevel_Warning, _data[3], "Can't Find Node %d for Encrypted Message", _data[3]);
					return;
				}
			}
			if (Internal::DecryptBuffer(&_data[5], _data[4] + 1, this, _data[3], this->GetControllerNodeId(), _nonce, &_newdata[0]))
			{
				/* Ok - _newdata now contains the decrypted packet */
				/* copy it back to the _data packet for processing */
				/* New Length - See Decrypt Packet for why these numbers*/
				_data[4] = _data[4] - 8 - 8 - 2 - 2;

				/* now copy the decrypted packet */
				memcpy(&_data[5], &_newdata[1], _data[4]);
				//PrintHex("Decrypted Packet", _data, _data[4]+5);

				/* if the Node has something else to send, it will encrypt a message and send it as a MessageEncapNonceGet */
				if (Internal::CC::SecurityCmd_MessageEncapNonceGet == SecurityCmd)
				{
					Log::Write(LogLevel_Info, _data[3], "Received SecurityCmd_MessageEncapNonceGet from node %d - Sending New Nonce", _data[3]);
					Internal::LockGuard LG(m_nodeMutex);
					Node* node = GetNode(_data[3]);
					if (node)
					{
						_nonce = node->GenerateNonceKey();
					}
					else
					{
						Log::Write(LogLevel_Warning, _data[3], "Couldn't Generate Nonce Key for Node %d", _data[3]);
						return;
					}
					SendNonceKey(_data[3], _nonce);
				}

				wasencrypted = true;

			}
			else
			{
				/* if the Node has something else to send, it will encrypt a message and send it as a MessageEncapNonceGet */
				if (Internal::CC::SecurityCmd_MessageEncapNonceGet == SecurityCmd)
				{
					Log::Write(LogLevel_Info, _data[3], "Received SecurityCmd_MessageEncapNonceGet from node %d - Sending New Nonce", _data[3]);
					Internal::LockGuard LG(m_nodeMutex);
					Node* node = GetNode(_data[3]);
					if (node)
					{
						_nonce = node->GenerateNonceKey();
					}
					else
					{
						Log::Write(LogLevel_Warning, _data[3], "Couldn't Generate Nonce Key for Node %d", _data[3]);
						return;
					}
					SendNonceKey(_data[3], _nonce);
				}
				/* it failed for some reason, lets just move on */
				m_expectedReply = 0;
				m_expectedNodeId = 0;
				RemoveCurrentMsg();
				return;
			}
		}
	}

	if ( RESPONSE == _data[0])
	{
		switch (_data[1])
		{
			case FUNC_ID_SERIAL_API_GET_INIT_DATA:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSerialAPIGetInitDataResponse(_data);
				break;
			}
			case FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetControllerCapabilitiesResponse(_data);
				break;
			}
			case FUNC_ID_SERIAL_API_GET_CAPABILITIES:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetSerialAPICapabilitiesResponse(_data);
				break;
			}
			case FUNC_ID_SERIAL_API_SOFT_RESET:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSerialAPISoftResetResponse(_data);
				break;
			}
			case FUNC_ID_ZW_SEND_DATA:
			{
				HandleSendDataResponse(_data, false);
				handleCallback = false;			// Skip the callback handling - a subsequent FUNC_ID_ZW_SEND_DATA request will deal with that
				break;
			}
			case FUNC_ID_ZW_GET_VERSION:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetVersionResponse(_data);
				break;
			}
			case FUNC_ID_ZW_GET_RANDOM:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetRandomResponse(_data);
				break;
			}
			case FUNC_ID_SERIAL_API_SETUP:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSerialAPISetupResponse(_data);
				break;
			}
			case FUNC_ID_ZW_MEMORY_GET_ID:
			{
				Log::Write(LogLevel_Detail, "");
				HandleMemoryGetIdResponse(_data);
				break;
			}
			case FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetNodeProtocolInfoResponse(_data);
				break;
			}
			case FUNC_ID_ZW_REPLICATION_SEND_DATA:
			{
				HandleSendDataResponse(_data, true);
				handleCallback = false;			// Skip the callback handling - a subsequent FUNC_ID_ZW_REPLICATION_SEND_DATA request will deal with that
				break;
			}
			case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
			{
				Log::Write(LogLevel_Detail, "");
				if (!HandleAssignReturnRouteResponse(_data))
				{
					m_expectedCallbackId = _data[2];		// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
			{
				Log::Write(LogLevel_Detail, "");
				if (!HandleDeleteReturnRouteResponse(_data))
				{
					m_expectedCallbackId = _data[2];		// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_ENABLE_SUC:
			{
				Log::Write(LogLevel_Detail, "");
				HandleEnableSUCResponse(_data);
				break;
			}
			case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
			{
				Log::Write(LogLevel_Detail, "");
				if (!HandleNetworkUpdateResponse(_data))
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_SET_SUC_NODE_ID:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSetSUCNodeIdResponse(_data);
				break;
			}
			case FUNC_ID_ZW_GET_SUC_NODE_ID:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetSUCNodeIdResponse(_data);
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_INFO:
			{
				// This code used _data[3] to log Node ID
				// but FUNC_ID_ZW_REQUEST_NODE_INFO reply does not report back node number.
				Log::Write(LogLevel_Detail, "");
				if (_data[2])
				{

					Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "FUNC_ID_ZW_REQUEST_NODE_INFO Request successful.");
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "FUNC_ID_ZW_REQUEST_NODE_INFO Request failed.");
				}
				break;
			}
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				Log::Write(LogLevel_Detail, "");
				if (!HandleRemoveFailedNodeResponse(_data))
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_IS_FAILED_NODE_ID:
			{
				Log::Write(LogLevel_Detail, "");
				HandleIsFailedNodeResponse(_data);
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				Log::Write(LogLevel_Detail, "");
				if (!HandleReplaceFailedNodeResponse(_data))
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_GET_ROUTING_INFO:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetRoutingInfoResponse(_data);
				break;
			}
			case FUNC_ID_ZW_R_F_POWER_LEVEL_SET:
			{
				Log::Write(LogLevel_Detail, "");
				HandleRfPowerLevelSetResponse(_data);
				break;
			}
			case FUNC_ID_ZW_READ_MEMORY:
			{
				Log::Write(LogLevel_Detail, "");
				HandleReadMemoryResponse(_data);
				break;
			}
			case FUNC_ID_SERIAL_API_SET_TIMEOUTS:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSerialApiSetTimeoutsResponse(_data);
				break;
			}
			case FUNC_ID_MEMORY_GET_BYTE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleMemoryGetByteResponse(_data);
				break;
			}
			case FUNC_ID_ZW_GET_VIRTUAL_NODES:
			{
				Log::Write(LogLevel_Detail, "");
				HandleGetVirtualNodesResponse(_data);
				break;
			}
			case FUNC_ID_ZW_SET_SLAVE_LEARN_MODE:
			{
				Log::Write(LogLevel_Detail, "");
				if (!HandleSetSlaveLearnModeResponse(_data))
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
			case FUNC_ID_ZW_SEND_SLAVE_NODE_INFO:
			{
				Log::Write(LogLevel_Detail, "");
				if (!HandleSendSlaveNodeInfoResponse(_data))
				{
					m_expectedCallbackId = _data[2];	// The callback message won't be coming, so we force the transaction to complete
					m_expectedReply = 0;
					m_expectedCommandClassId = 0;
					m_expectedNodeId = 0;
				}
				break;
			}
				/* Ignore these. They are manufacturer proprietary */
			case FUNC_ID_PROPRIETARY_0:
			case FUNC_ID_PROPRIETARY_1:
			case FUNC_ID_PROPRIETARY_2:
			case FUNC_ID_PROPRIETARY_3:
			case FUNC_ID_PROPRIETARY_4:
			case FUNC_ID_PROPRIETARY_5:
			case FUNC_ID_PROPRIETARY_6:
			case FUNC_ID_PROPRIETARY_7:
			case FUNC_ID_PROPRIETARY_8:
			case FUNC_ID_PROPRIETARY_9:
			case FUNC_ID_PROPRIETARY_A:
			case FUNC_ID_PROPRIETARY_B:
			case FUNC_ID_PROPRIETARY_C:
			case FUNC_ID_PROPRIETARY_D:
			case FUNC_ID_PROPRIETARY_E:
			{
				break;
			}
			default:
			{
				Log::Write(LogLevel_Detail, "");
				Log::Write(LogLevel_Info, "**TODO: handle response for 0x%.2x** Please report this message.", _data[1]);
				break;
			}
		}
	}
	else if ( REQUEST == _data[0])
	{
		switch (_data[1])
		{
			case FUNC_ID_APPLICATION_COMMAND_HANDLER:
			{
				Log::Write(LogLevel_Detail, "");
				HandleApplicationCommandHandlerRequest(_data, wasencrypted);
				break;
			}
			case FUNC_ID_ZW_SEND_DATA:
			{
				HandleSendDataRequest(_data, _length, false);
				break;
			}
			case FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE:
			{
				if (m_controllerReplication)
				{
					Log::Write(LogLevel_Detail, "");
					m_controllerReplication->SendNextData();
				}
				break;
			}
			case FUNC_ID_ZW_REPLICATION_SEND_DATA:
			{
				HandleSendDataRequest(_data, _length, true);
				break;
			}
			case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleAssignReturnRouteRequest(_data);
				break;
			}
			case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleDeleteReturnRouteRequest(_data);
				break;
			}
			case FUNC_ID_ZW_SEND_NODE_INFORMATION:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSendNodeInformationRequest(_data);
				break;
			}
			case FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE:
			case FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE_OPTIONS:
			{
				Log::Write(LogLevel_Detail, "");
				HandleNodeNeighborUpdateRequest(_data);
				break;
			}
			case FUNC_ID_ZW_APPLICATION_UPDATE:
			{
				Log::Write(LogLevel_Detail, "");
				handleCallback = !HandleApplicationUpdateRequest(_data);
				break;
			}
			case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
			{
				Log::Write(LogLevel_Detail, "");
				HandleAddNodeToNetworkRequest(_data);
				break;
			}
			case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
			{
				Log::Write(LogLevel_Detail, "");
				HandleRemoveNodeFromNetworkRequest(_data);
				break;
			}
			case FUNC_ID_ZW_CREATE_NEW_PRIMARY:
			{
				Log::Write(LogLevel_Detail, "");
				HandleCreateNewPrimaryRequest(_data);
				break;
			}
			case FUNC_ID_ZW_CONTROLLER_CHANGE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleControllerChangeRequest(_data);
				break;
			}
			case FUNC_ID_ZW_SET_LEARN_MODE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSetLearnModeRequest(_data);
				break;
			}
			case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleNetworkUpdateRequest(_data);
				break;
			}
			case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
			{
				Log::Write(LogLevel_Detail, "");
				HandleRemoveFailedNodeRequest(_data);
				break;
			}
			case FUNC_ID_ZW_REPLACE_FAILED_NODE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleReplaceFailedNodeRequest(_data);
				break;
			}
			case FUNC_ID_ZW_SET_SLAVE_LEARN_MODE:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSetSlaveLearnModeRequest(_data);
				break;
			}
			case FUNC_ID_ZW_SEND_SLAVE_NODE_INFO:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSendSlaveNodeInfoRequest(_data);
				break;
			}
			case FUNC_ID_APPLICATION_SLAVE_COMMAND_HANDLER:
			{
				Log::Write(LogLevel_Detail, "");
				HandleApplicationSlaveCommandRequest(_data);
				break;
			}
			case FUNC_ID_PROMISCUOUS_APPLICATION_COMMAND_HANDLER:
			{
				Log::Write(LogLevel_Detail, "");
				HandlePromiscuousApplicationCommandHandlerRequest(_data);
				break;
			}
			case FUNC_ID_ZW_SET_DEFAULT:
			{
				Log::Write(LogLevel_Detail, "");
				HandleSerialAPIResetRequest(_data);
				break;
			}
				/* Ignore these. They are manufacturer proprietary */
			case FUNC_ID_PROPRIETARY_0:
			case FUNC_ID_PROPRIETARY_1:
			case FUNC_ID_PROPRIETARY_2:
			case FUNC_ID_PROPRIETARY_3:
			case FUNC_ID_PROPRIETARY_4:
			case FUNC_ID_PROPRIETARY_5:
			case FUNC_ID_PROPRIETARY_6:
			case FUNC_ID_PROPRIETARY_7:
			case FUNC_ID_PROPRIETARY_8:
			case FUNC_ID_PROPRIETARY_9:
			case FUNC_ID_PROPRIETARY_A:
			case FUNC_ID_PROPRIETARY_B:
			case FUNC_ID_PROPRIETARY_C:
			case FUNC_ID_PROPRIETARY_D:
			case FUNC_ID_PROPRIETARY_E:
			{
				break;
			}
			default:
			{
				Log::Write(LogLevel_Detail, "");
				Log::Write(LogLevel_Info, "**TODO: handle request for 0x%.2x** Please report this message.", _data[1]);
				break;
			}
		}
	}

	// Generic callback handling
	// This code used _data[3] in some parts to log Node ID,
	// but not all serial data frames report back node number.
	if (handleCallback)
	{
		if ((m_expectedCallbackId || m_expectedReply))
		{
			if (m_expectedCallbackId)
			{
				if (m_expectedCallbackId == _data[2])
				{
					Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "  Expected callbackId was received");
					m_expectedCallbackId = 0;
				}
				else if (_data[2] == 0x02 || _data[2] == 0x01)
				{
					/* it was a NONCE request/reply. Drop it */
					return;
				}
			}
			if (m_expectedReply)
			{
				if (m_expectedReply == _data[1])
				{
					if (m_expectedCommandClassId && (m_expectedReply == FUNC_ID_APPLICATION_COMMAND_HANDLER))
					{
						if (m_expectedCallbackId == 0 && m_expectedCommandClassId == _data[5] && m_expectedNodeId == _data[3])
						{
							Log::Write(LogLevel_Detail, _data[3], "  Expected reply and command class was received");
							m_waitingForAck = false;
							m_expectedReply = 0;
							m_expectedCommandClassId = 0;
							m_expectedNodeId = 0;
						}
					}
					else
					{
						if (IsExpectedReply(_data[3]))

						{
							Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "  Expected reply was received");
							m_expectedReply = 0;
							m_expectedNodeId = 0;
						}
					}
				}
			}
			if (!(m_expectedCallbackId || m_expectedReply))
			{
				Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "  Message transaction complete");
				Log::Write(LogLevel_Detail, "");

				if (m_notifytransactions)
				{
					Notification* notification = new Notification(Notification::Type_Notification);
					notification->SetHomeAndNodeIds(m_homeId, GetNodeNumber(m_currentMsg));
					notification->SetNotification(Notification::Code_MsgComplete);
					QueueNotification(notification);
				}
				RemoveCurrentMsg();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetVersionResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetVersionResponse(uint8* _data)
{
	m_libraryVersion = (char*) &_data[2];

	m_libraryType = _data[m_libraryVersion.size() + 3];
	if (m_libraryType < 9)
	{
		m_libraryTypeName = c_libraryTypeNames[m_libraryType];
	}
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_GET_VERSION:");
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    %s library, version %s", m_libraryTypeName.c_str(), m_libraryVersion.c_str());
	if (!((m_libraryType == ZW_LIB_CONTROLLER_STATIC) || (m_libraryType == ZW_LIB_CONTROLLER)))
	{
		Log::Write(LogLevel_Fatal, GetNodeNumber(m_currentMsg), "Z-Wave Interface is not a Supported Library Type: %s", m_libraryTypeName.c_str());
		Log::Write(LogLevel_Fatal, GetNodeNumber(m_currentMsg), "Z-Wave Interface should be a Static Controller Library Type");

		{
			Notification* notification = new Notification(Notification::Type_UserAlerts);
			notification->SetUserAlertNotification(Notification::Alert_UnsupportedController);
			QueueNotification(notification);
		}
		{
			Notification* notification = new Notification(Notification::Type_DriverFailed);
			notification->SetHomeAndNodeIds(m_homeId, m_currentMsg->GetTargetNodeId());
			notification->SetComPort(m_controllerPath);
			QueueNotification(notification);
		}
		NotifyWatchers();
		m_driverThread->Stop();
	}
	return;
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetRandomResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------

void Driver::HandleGetRandomResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, "Received reply to FUNC_ID_ZW_GET_RANDOM: %s", _data[2] ? "true" : "false");
}

void Driver::HandleSerialAPISetupResponse(uint8* _data)
{
	// See INS13954 for description of FUNC_ID_SERIAL_API_SETUP with command 
	// SERIAL_API_SETUP_CMD_TX_STATUS_REPORT
	// Note: SERIAL_API_SETUP can do more things than enable this report...

	Log::Write(LogLevel_Info, "Received reply to FUNC_ID_SERIAL_API_SETUP");

	switch (_data[0])
	{
		case 1:
			Log::Write(LogLevel_Info, "Successfully enabled extended txStatusReport.");
			m_hasExtendedTxStatus = true;
			break;

		case 0:
			Log::Write(LogLevel_Info, "Failed to enable extended txStatusReport. Controller might not support it.");
			m_hasExtendedTxStatus = false;
			break;

		default:
			Log::Write(LogLevel_Info, "FUNC_ID_SERIAL_API_SETUP returned unknown status: %u", _data[0]);
			m_hasExtendedTxStatus = false;
			break;
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetControllerCapabilitiesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetControllerCapabilitiesResponse(uint8* _data)
{
	m_controllerCaps = _data[2];

	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:");

	char str[256];
	if (m_controllerCaps & ControllerCaps_SIS)
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    There is a SUC ID Server (SIS) in this network.");
		snprintf(str, sizeof(str), "    The PC controller is an inclusion %s%s%s", (m_controllerCaps & ControllerCaps_SUC) ? "static update controller (SUC)" : "controller", (m_controllerCaps & ControllerCaps_OnOtherNetwork) ? " which is using a Home ID from another network" : "", (m_controllerCaps & ControllerCaps_RealPrimary) ? " and was the original primary before the SIS was added." : ".");
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), str);

	}
	else
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    There is no SUC ID Server (SIS) in this network.");
		snprintf(str, sizeof(str), "    The PC controller is a %s%s%s", (m_controllerCaps & ControllerCaps_Secondary) ? "secondary" : "primary", (m_controllerCaps & ControllerCaps_SUC) ? " static update controller (SUC)" : " controller", (m_controllerCaps & ControllerCaps_OnOtherNetwork) ? " which is using a Home ID from another network." : ".");
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), str);
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetSerialAPICapabilitiesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetSerialAPICapabilitiesResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), " Received reply to FUNC_ID_SERIAL_API_GET_CAPABILITIES");
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Serial API Version:   %d.%d", _data[2], _data[3]);
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Manufacturer ID:      0x%.2x%.2x", _data[4], _data[5]);
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Product Type:         0x%.2x%.2x", _data[6], _data[7]);
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Product ID:           0x%.2x%.2x", _data[8], _data[9]);

	// _data[10] to _data[41] are a 256-bit bitmask with one bit set for
	// each FUNC_ID_ method supported by the controller.
	// Bit 0 is FUNC_ID_ 1.  So FUNC_ID_SERIAL_API_GET_CAPABILITIES (0x07) will be bit 6 of the first byte.
	m_serialAPIVersion[0] = _data[2];
	m_serialAPIVersion[1] = _data[3];
	m_manufacturerId = (((uint16) _data[4]) << 8) | (uint16) _data[5];
	m_productType = (((uint16) _data[6]) << 8) | (uint16) _data[7];
	m_productId = (((uint16) _data[8]) << 8) | (uint16) _data[9];
	memcpy(m_apiMask, &_data[10], sizeof(m_apiMask));

	if (IsBridgeController())
	{
		SendMsg(new Internal::Msg("FUNC_ID_ZW_GET_VIRTUAL_NODES", 0xff, REQUEST, FUNC_ID_ZW_GET_VIRTUAL_NODES, false), MsgQueue_Command);
	}
	if (IsAPICallSupported( FUNC_ID_ZW_GET_RANDOM))

	{
		Internal::Msg *msg = new Internal::Msg("FUNC_ID_ZW_GET_RANDOM", 0xff, REQUEST, FUNC_ID_ZW_GET_RANDOM, false);
		msg->Append(32);      // 32 bytes
		SendMsg(msg, MsgQueue_Command);
	}

	if (IsAPICallSupported( FUNC_ID_SERIAL_API_SETUP))

	{
		Internal::Msg *msg = new Internal::Msg("FUNC_ID_SERIAL_API_SETUP", 0xff, REQUEST, FUNC_ID_SERIAL_API_SETUP, false);
		msg->Append( SERIAL_API_SETUP_CMD_TX_STATUS_REPORT);
		msg->Append(1);
		SendMsg(msg, MsgQueue_Command);
	}

	SendMsg(new Internal::Msg("FUNC_ID_SERIAL_API_GET_INIT_DATA", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false), MsgQueue_Command);
	if (!IsBridgeController())
	{
		Internal::Msg* msg = new Internal::Msg("FUNC_ID_SERIAL_API_SET_TIMEOUTS", 0xff, REQUEST, FUNC_ID_SERIAL_API_SET_TIMEOUTS, false);
		msg->Append( ACK_TIMEOUT / 10);
		msg->Append( BYTE_TIMEOUT / 10);
		SendMsg(msg, MsgQueue_Command);
	}
	Internal::Msg* msg = new Internal::Msg("FUNC_ID_SERIAL_API_APPL_NODE_INFORMATION", 0xff, REQUEST, FUNC_ID_SERIAL_API_APPL_NODE_INFORMATION, false, false);
	msg->Append( APPLICATION_NODEINFO_LISTENING);
	msg->Append(0x02);			// Generic Static Controller
	msg->Append(0x01);			// Specific Static PC Controller

	/* get a list of Advertised Command Classes */
	list<uint8> advertisedCommandClasses = Internal::CC::CommandClasses::GetAdvertisedCommandClasses();
	msg->Append((uint8) advertisedCommandClasses.size());			// Length
	for (list<uint8>::iterator it = advertisedCommandClasses.begin(); it != advertisedCommandClasses.end(); ++it)
		msg->Append(*it);

	SendMsg(msg, MsgQueue_Command);
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialAPISoftResetResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSerialAPISoftResetResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to Soft Reset.");
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialAPIResetRequest>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSerialAPIResetRequest(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to complete Controller Reset.");
	if (m_controllerResetEvent != NULL)
	{
		m_controllerResetEvent->Set();
		m_controllerResetEvent = NULL;
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleEnableSUCResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleEnableSUCResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to Enable SUC.");
}

//-----------------------------------------------------------------------------
// <Driver::HandleNetworkUpdateResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleNetworkUpdateResponse(uint8* _data)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if (_data[2])
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE - command in progress");
	}
	else
	{
		// Failed
		Log::Write(LogLevel_Warning, GetNodeNumber(m_currentMsg), "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE - command failed");
		state = ControllerState_Failed;
		res = false;
	}

	UpdateControllerState(state);
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleSetSUCNodeIdResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSetSUCNodeIdResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to SET_SUC_NODE_ID.");
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetSUCNodeIdResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetSUCNodeIdResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to GET_SUC_NODE_ID.  Node ID = %d", _data[2]);
	m_SUCNodeId = _data[2];
	if (_data[2] == 0)
	{
		bool enableSIS = true;
		Options::Get()->GetOptionAsBool("EnableSIS", &enableSIS);
		if (enableSIS)
		{
			if (IsAPICallSupported(FUNC_ID_ZW_ENABLE_SUC) && IsAPICallSupported(FUNC_ID_ZW_SET_SUC_NODE_ID))
			{
				Log::Write(LogLevel_Info, "  No SUC, so we become SIS");

				Internal::Msg* msg;
				msg = new Internal::Msg("Enable SUC", m_Controller_nodeId, REQUEST, FUNC_ID_ZW_ENABLE_SUC, false);
				msg->Append(1);
				msg->Append( SUC_FUNC_NODEID_SERVER);		// SIS; SUC would be ZW_SUC_FUNC_BASIC_SUC
				SendMsg(msg, MsgQueue_Send);

				msg = new Internal::Msg("Set SUC node ID", m_Controller_nodeId, REQUEST, FUNC_ID_ZW_SET_SUC_NODE_ID, false);
				msg->Append(m_Controller_nodeId);
				msg->Append(1);								// TRUE, we want to be SUC/SIS
				msg->Append(0);								// no low power
				msg->Append( SUC_FUNC_NODEID_SERVER);
				SendMsg(msg, MsgQueue_Send);
			}
			else
			{
				Log::Write(LogLevel_Info, "Controller Does not Support SUC - Cannot Setup Controller as SUC Node");
			}
		}
		else
		{
			Log::Write(LogLevel_Info, "  No SUC, not becoming SUC as option is disabled");
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleMemoryGetIdResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleMemoryGetIdResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_MEMORY_GET_ID. Home ID = 0x%02x%02x%02x%02x.  Our node ID = %d", _data[2], _data[3], _data[4], _data[5], _data[6]);
	m_homeId = (((uint32) _data[2]) << 24) | (((uint32) _data[3]) << 16) | (((uint32) _data[4]) << 8) | ((uint32) _data[5]);
	m_Controller_nodeId = _data[6];
	m_controllerReplication = static_cast<Internal::CC::ControllerReplication*>(Internal::CC::ControllerReplication::Create(m_homeId, m_Controller_nodeId));
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialAPIGetInitDataResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSerialAPIGetInitDataResponse(uint8* _data)
{
	int32 i;

	if (!m_init)
	{
		// Mark the driver as ready (we have to do this first or
		// all the code handling notifications will go awry).
		Manager::Get()->SetDriverReady(this, true);

		// Read the config file first, to get the last known state
		ReadCache();
	}
	else
	{
		// Notify the user that all node and value information has been deleted
		// We need to wait to do this here so we have new information to report.
		Notification* notification = new Notification(Notification::Type_DriverReset);
		notification->SetHomeAndNodeIds(m_homeId, 0);
		QueueNotification(notification);
	}

	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_SERIAL_API_GET_INIT_DATA:");
	m_initVersion = _data[2];
	m_initCaps = _data[3];

	if (_data[4] == NUM_NODE_BITFIELD_BYTES)
	{
		for (i = 0; i < NUM_NODE_BITFIELD_BYTES; ++i)
		{
			for (int32 j = 0; j < 8; ++j)
			{
				uint8 nodeId = (i * 8) + j + 1;
				if (_data[i + 5] & (0x01 << j))
				{
					if (IsVirtualNode(nodeId))
					{
						Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Node %.3d - Virtual (ignored)", nodeId);
					}
					else
					{
						Internal::LockGuard LG(m_nodeMutex);
						Node* node = GetNode(nodeId);
						if (node)
						{
							Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Node %.3d - Known", nodeId);
							if (!m_init)
							{
								// The node was read in from the config, so we
								// only need to get its current state
								node->SetQueryStage(Node::QueryStage_CacheLoad);
							}

						}
						else
						{
							// This node is new
							Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Node %.3d - New", nodeId);
							Notification* notification = new Notification(Notification::Type_NodeNew);
							notification->SetHomeAndNodeIds(m_homeId, nodeId);
							QueueNotification(notification);

							// Create the node and request its info
							InitNode(nodeId);
						}
					}
				}
				else
				{
					Internal::LockGuard LG(m_nodeMutex);
					if (GetNode(nodeId))
					{
						// This node no longer exists in the Z-Wave network
						Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Node %.3d - Removed", nodeId);
						delete m_nodes[nodeId];
						m_nodes[nodeId] = NULL;
						Notification* notification = new Notification(Notification::Type_NodeRemoved);
						notification->SetHomeAndNodeIds(m_homeId, nodeId);
						QueueNotification(notification);

					}
				}
			}
		}
	}

	m_init = true;
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetNodeProtocolInfoResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetNodeProtocolInfoResponse(uint8* _data)
{
	// The node that the protocol info response is for is not included in the message.
	// We have to assume that the node is the same one as in the most recent request.
	if (!m_currentMsg)
	{
		Log::Write(LogLevel_Warning, "WARNING: Received unexpected FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO message - ignoring.");
		return;
	}

	uint8 nodeId = m_currentMsg->GetTargetNodeId();
	Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO");

	// Update the node with the protocol info
	if (Node* node = GetNodeUnsafe(nodeId))
	{
		node->UpdateProtocolInfo(&_data[2]);
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleAssignReturnRouteResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleAssignReturnRouteResponse(uint8* _data)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if (_data[2])
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE - command in progress");
	}
	else
	{
		// Failed
		Log::Write(LogLevel_Warning, GetNodeNumber(m_currentMsg), "WARNING: Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE - command failed");
		state = ControllerState_Failed;
		res = false;
	}

	UpdateControllerState(state);
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleDeleteReturnRouteResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleDeleteReturnRouteResponse(uint8* _data)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if (_data[2])
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE - command in progress");
	}
	else
	{
		// Failed
		Log::Write(LogLevel_Warning, GetNodeNumber(m_currentMsg), "WARNING: Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE - command failed");
		state = ControllerState_Failed;
		res = false;
	}

	UpdateControllerState(state);
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleRemoveFailedNodeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleRemoveFailedNodeResponse(uint8* _data)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	ControllerError error = ControllerError_None;
	if (_data[2])
	{
		string reason;
		switch (_data[2])
		{
			case FAILED_NODE_NOT_FOUND:
			{
				reason = "Node not found";
				error = ControllerError_NotFound;
				break;
			}
			case FAILED_NODE_REMOVE_PROCESS_BUSY:
			{
				reason = "Remove process busy";
				error = ControllerError_Busy;
				break;
			}
			case FAILED_NODE_REMOVE_FAIL:
			{
				reason = "Remove failed";
				error = ControllerError_Failed;
				break;
			}
			case FAILED_NODE_NOT_PRIMARY_CONTROLLER:
			{
				reason = "Not Primary Controller";
				error = ControllerError_NotPrimary;
				break;
			}
			default:
			{
				reason = "Command failed";
				break;
			}
		}

		Log::Write(LogLevel_Warning, GetNodeNumber(m_currentMsg), "WARNING: Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - %s", reason.c_str());
		state = ControllerState_Failed;
		res = false;
	}
	else
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - Command in progress");
	}

	UpdateControllerState(state, error);
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleIsFailedNodeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleIsFailedNodeResponse(uint8* _data)
{
	ControllerState state;
	uint8 nodeId = m_currentControllerCommand ? m_currentControllerCommand->m_controllerCommandNode : GetNodeNumber(m_currentMsg);
	if (_data[2])
	{
		Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_IS_FAILED_NODE_ID - node %d failed", nodeId);
		state = ControllerState_NodeFailed;
		if (Node* node = GetNodeUnsafe(nodeId))
		{
			if (node->IsNodeReset())
			{
				/* a DeviceReset has Occured. Remove the Node */
				if (!BeginControllerCommand(Driver::ControllerCommand_RemoveFailedNode, NULL, NULL, true, nodeId, 0))
					Log::Write(LogLevel_Warning, nodeId, "RemoveFailedNode for DeviceResetLocally Command Failed");

				Notification* notification = new Notification(Notification::Type_NodeReset);
				notification->SetHomeAndNodeIds(m_homeId, nodeId);
				QueueNotification(notification);
				state = ControllerState_Completed;
			}
			else
			{
				node->SetNodeAlive(false);
			}
		}
	}
	else
	{
		Log::Write(LogLevel_Warning, nodeId, "Received reply to FUNC_ID_ZW_IS_FAILED_NODE_ID - node %d has not failed", nodeId);
		if (Node* node = GetNodeUnsafe(nodeId))
		{
			node->SetNodeAlive(true);
		}
		state = ControllerState_NodeOK;
	}
	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleReplaceFailedNodeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleReplaceFailedNodeResponse(uint8* _data)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	if (_data[2])
	{
		// Command failed
		Log::Write(LogLevel_Warning, GetNodeNumber(m_currentMsg), "WARNING: Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - command failed");
		state = ControllerState_Failed;
		res = false;
	}
	else
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - command in progress");
	}

	UpdateControllerState(state);
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendDataResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendDataResponse(uint8* _data, bool _replication)
{
	if (_data[2])
	{
		Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "  %s delivered to Z-Wave stack", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA");
	}
	else
	{
		Log::Write(LogLevel_Error, GetNodeNumber(m_currentMsg), "ERROR: %s could not be delivered to Z-Wave stack", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA");
		m_nondelivery++;
		if (Node* node = GetNodeUnsafe(GetNodeNumber(m_currentMsg)))
		{
			node->m_sentFailed++;
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetRoutingInfoResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetRoutingInfoResponse(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_GET_ROUTING_INFO");

	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(GetNodeNumber(m_currentMsg)))
	{
		// copy the 29-byte bitmap received (29*8=232 possible nodes) into this node's neighbors member variable
		memcpy(node->m_neighbors, &_data[2], 29);
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Neighbors of this node are:");
		bool bNeighbors = false;
		for (int by = 0; by < 29; by++)
		{
			for (int bi = 0; bi < 8; bi++)
			{
				if ((_data[2 + by] & (0x01 << bi)))
				{
					Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "    Node %d", (by << 3) + bi + 1);
					bNeighbors = true;
				}
			}
		}

		if (!bNeighbors)
		{
			Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), " (none reported)");
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendDataRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendDataRequest(uint8* _data, uint8 _length, bool _replication)
{
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	Log::Write(LogLevel_Detail, nodeId, "  %s Request with callback ID 0x%.2x received (expected 0x%.2x)", _replication ? "ZW_REPLICATION_SEND_DATA" : "ZW_SEND_DATA", _data[2], _data[2] < 10 ? _data[2] : m_expectedCallbackId);
	/* Callback ID's below 10 are reserved for NONCE messages */
	if ((_data[2] > 10) && (_data[2] != m_expectedCallbackId))
	{
		// Wrong callback ID
		m_callbacks++;
		Log::Write(LogLevel_Warning, nodeId, "WARNING: Unexpected Callback ID received");
	}
	else
	{
		Node* node = GetNodeUnsafe(nodeId);
		if (node != NULL)
		{
			if (_data[3] != TRANSMIT_COMPLETE_OK)
			{
				node->m_sentFailed++;
			}
			else
			{
				node->m_lastRequestRTT = -node->m_sentTS.TimeRemaining();

				if (node->m_averageRequestRTT)
				{
					// if the average has been established, update by averaging the average and the last RTT
					node->m_averageRequestRTT = (node->m_averageRequestRTT + node->m_lastRequestRTT) >> 1;
				}
				else
				{
					// if this is the first observed RTT, set the average to this value
					node->m_averageRequestRTT = node->m_lastRequestRTT;
				}
				Log::Write(LogLevel_Info, nodeId, "Request RTT %d Average Request RTT %d", node->m_lastRequestRTT, node->m_averageRequestRTT);
			}
			/* if the frame has txStatus message, then extract it */
			// petergebruers, changed test (_length > 7) to >= 23 to avoid extracting non-existent data, highest is _data[22]
			if (_length >= 23)
			{
				node->m_txStatusReportSupported = true;
				// petergebruers:
				// because OpenZWave uses "ms" everywhere, and wTransmitTicks
				// has "10 ms" as unit... multiply by 10. This wil avoid
				// confusion when people look at stats or log files.
				node->m_txTime = (_data[5] + (_data[4] << 8)) * 10;
				node->m_hops = _data[6];
				// petergebruers: there are 5 rssi values because there are
				// 4 repeaters + 1 sending node
				strncpy(node->m_rssi_1, Internal::rssi_to_string(_data[7]), sizeof(node->m_rssi_1) - 1);
				strncpy(node->m_rssi_2, Internal::rssi_to_string(_data[8]), sizeof(node->m_rssi_2) - 1);
				strncpy(node->m_rssi_3, Internal::rssi_to_string(_data[9]), sizeof(node->m_rssi_3) - 1);
				strncpy(node->m_rssi_4, Internal::rssi_to_string(_data[10]), sizeof(node->m_rssi_4) - 1);
				strncpy(node->m_rssi_5, Internal::rssi_to_string(_data[11]), sizeof(node->m_rssi_5) - 1);
				node->m_ackChannel = _data[12];
				node->m_lastTxChannel = _data[13];
				node->m_routeScheme = (TXSTATUS_ROUTING_SCHEME) _data[14];
				node->m_routeUsed[0] = _data[15];
				node->m_routeUsed[1] = _data[16];
				node->m_routeUsed[2] = _data[17];
				node->m_routeUsed[3] = _data[18];
				node->m_routeSpeed = (TXSTATUS_ROUTE_SPEED) _data[19];
				node->m_routeTries = _data[20];
				node->m_lastFailedLinkFrom = _data[21];
				node->m_lastFailedLinkTo = _data[22];
				Node::NodeData nd;
				node->GetNodeStatistics(&nd);
				// petergebruers: changed "ChannelAck" to "AckChannel", to be consistent with docs and "TxChannel"
				Log::Write(LogLevel_Detail, nodeId, "Extended TxStatus: Time: %d, Hops: %d, Rssi: %s %s %s %s %s, AckChannel: %d, TxChannel: %d, RouteScheme: %s, Route: %d %d %d %d, RouteSpeed: %s, RouteTries: %d, FailedLinkFrom: %d, FailedLinkTo: %d", nd.m_txTime, nd.m_hops, nd.m_rssi_1, nd.m_rssi_2, nd.m_rssi_3, nd.m_rssi_4, nd.m_rssi_4, nd.m_ackChannel, nd.m_lastTxChannel, Manager::GetNodeRouteScheme(&nd).c_str(), nd.m_routeUsed[0], nd.m_routeUsed[1], nd.m_routeUsed[2], nd.m_routeUsed[3],
						Manager::GetNodeRouteSpeed(&nd).c_str(), nd.m_routeTries, nd.m_lastFailedLinkFrom, nd.m_lastFailedLinkTo);
			}

		}

		// We do this here since HandleErrorResponse/MoveMessagesToWakeUpQueue can delete m_currentMsg
		if (m_currentMsg && m_currentMsg->IsNoOperation())
		{
			Notification* notification = new Notification(Notification::Type_Notification);
			notification->SetHomeAndNodeIds(m_homeId, GetNodeNumber(m_currentMsg));
			notification->SetNotification(Notification::Code_NoOperation);
			QueueNotification(notification);
		}

		// Callback ID matches our expectation
		if (_data[3] != TRANSMIT_COMPLETE_OK)
		{
			if (!HandleErrorResponse(_data[3], nodeId, _replication ? "ZW_REPLICATION_END_DATA" : "ZW_SEND_DATA", !_replication))
			{
				if (m_currentMsg && m_currentMsg->IsNoOperation() && node != NULL && (node->GetCurrentQueryStage() == Node::QueryStage_Probe || node->GetCurrentQueryStage() == Node::QueryStage_CacheLoad))
				{
					node->QueryStageRetry(node->GetCurrentQueryStage(), 3);
				}
			}
		}
		else if (node != NULL)
		{
			// If WakeUpNoMoreInformation request succeeds, update our status
			if (m_currentMsg && m_currentMsg->IsWakeUpNoMoreInformationCommand())
			{
				if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(node->GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
				{
					// Mark the node as asleep
					wakeUp->SetAwake(false);
				}
			}
			// If node is not alive, mark it alive now
			if (!node->IsNodeAlive())
			{
				node->SetNodeAlive(true);
			}
		}
		// Command reception acknowledged by node, error or not, but ignore any NONCE messages
		//
		//	m_expectedCallbackId = 0;
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleNetworkUpdateRequest>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleNetworkUpdateRequest(uint8* _data)
{
	ControllerState state = ControllerState_Failed;
	ControllerError error = ControllerError_None;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	switch (_data[3])
	{
		case SUC_UPDATE_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Success");
			state = ControllerState_Completed;
			break;
		}
		case SUC_UPDATE_ABORT:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - Error. Process aborted.");
			error = ControllerError_Failed;
			break;
		}
		case SUC_UPDATE_WAIT:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - SUC is busy.");
			error = ControllerError_Busy;
			break;
		}
		case SUC_UPDATE_DISABLED:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - SUC is disabled.");
			error = ControllerError_Disabled;
			break;
		}
		case SUC_UPDATE_OVERFLOW:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_REQUEST_NETWORK_UPDATE: Failed - Overflow. Full replication required.");
			error = ControllerError_Overflow;
			break;
		}
		default:
		{
		}
	}

	UpdateControllerState(state, error);
}

//-----------------------------------------------------------------------------
// <Driver::HandleAddNodeToNetworkRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleAddNodeToNetworkRequest(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "FUNC_ID_ZW_ADD_NODE_TO_NETWORK:");
	CommonAddNodeStatusRequestHandler( FUNC_ID_ZW_ADD_NODE_TO_NETWORK, _data);
}

//-----------------------------------------------------------------------------
// <Driver::HandleRemoveNodeFromNetworkRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleRemoveNodeFromNetworkRequest(uint8* _data)
{
	//uint8 nodeId = GetNodeNumber( m_currentMsg );
	if (m_currentControllerCommand == NULL)
	{
		return;
	}
	ControllerState state = m_currentControllerCommand->m_controllerState;
	Log::Write(LogLevel_Info, "FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:");

	switch (_data[3])
	{
		case REMOVE_NODE_STATUS_LEARN_READY:
		{
			Log::Write(LogLevel_Info, "REMOVE_NODE_STATUS_LEARN_READY");
			state = ControllerState_Waiting;
			m_currentControllerCommand->m_controllerCommandNode = 0;
			break;
		}
		case REMOVE_NODE_STATUS_NODE_FOUND:
		{
			Log::Write(LogLevel_Info, "REMOVE_NODE_STATUS_NODE_FOUND");
			state = ControllerState_InProgress;
			break;
		}
		case REMOVE_NODE_STATUS_REMOVING_SLAVE:
		{
			Log::Write(LogLevel_Info, "REMOVE_NODE_STATUS_REMOVING_SLAVE");
			if (_data[4] != 0)
			{
				Log::Write(LogLevel_Info, "Removing node ID %d", _data[4]);
				m_currentControllerCommand->m_controllerCommandNode = _data[4];
			}
			else
			{
				Log::Write(LogLevel_Warning, "Remove Node Failed - NodeID 0 Returned");
				state = ControllerState_Failed;
			}
			break;
		}
		case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
		{
			Log::Write(LogLevel_Info, "REMOVE_NODE_STATUS_REMOVING_CONTROLLER");
			m_currentControllerCommand->m_controllerCommandNode = _data[4];
			if (m_currentControllerCommand->m_controllerCommandNode == 0) // Some controllers don't return node number
			{
				if (_data[5] >= 3)
				{
					Internal::LockGuard LG(m_nodeMutex);
					for (int i = 0; i < 256; i++)
					{
						if (m_nodes[i] == NULL)
						{
							continue;
						}
						// Ignore primary controller
						if (m_nodes[i]->m_nodeId == m_Controller_nodeId)
						{
							continue;
						}
						// See if we can match another way
						if (m_nodes[i]->m_basic == _data[6] && m_nodes[i]->m_generic == _data[7] && m_nodes[i]->m_specific == _data[8])
						{
							if (m_currentControllerCommand->m_controllerCommandNode != 0)
							{
								Log::Write(LogLevel_Info, "Alternative controller lookup found more then one match. Using the first one found.");
							}
							else
							{
								m_currentControllerCommand->m_controllerCommandNode = m_nodes[i]->m_nodeId;
							}
						}
					}
					LG.Unlock();
				}
				else
				{
					Log::Write(LogLevel_Warning, "WARNING: Node is 0 but not enough data to perform alternative match.");
				}
			}
			else
			{
				m_currentControllerCommand->m_controllerCommandNode = _data[4];
			}
			WriteCache();
			Log::Write(LogLevel_Info, "Removing controller ID %d", m_currentControllerCommand->m_controllerCommandNode);
			break;
		}
		case REMOVE_NODE_STATUS_DONE:
		{
			Log::Write(LogLevel_Info, "REMOVE_NODE_STATUS_DONE");
			if (!m_currentControllerCommand->m_controllerCommandDone)
			{

				// Remove Node Stop calls back through here so make sure
				// we do't do it again.
				UpdateControllerState(ControllerState_Completed);
				//AddNodeStop( FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK );
				if (m_currentControllerCommand->m_controllerCommandNode == 0) // never received "removing" update
				{
					if (_data[4] != 0) // but message has the clue
					{
						m_currentControllerCommand->m_controllerCommandNode = _data[4];
					}
				}

				if (m_currentControllerCommand->m_controllerCommandNode != 0 && m_currentControllerCommand->m_controllerCommandNode != 0xff)
				{
					{
						Internal::LockGuard LG(m_nodeMutex);
						delete m_nodes[m_currentControllerCommand->m_controllerCommandNode];
						m_nodes[m_currentControllerCommand->m_controllerCommandNode] = NULL;
					}
					WriteCache();
					Notification* notification = new Notification(Notification::Type_NodeRemoved);
					notification->SetHomeAndNodeIds(m_homeId, m_currentControllerCommand->m_controllerCommandNode);
					QueueNotification(notification);
				}
			}
			return;
		}
		case REMOVE_NODE_STATUS_FAILED:
		{
			//AddNodeStop( FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK );
			Log::Write(LogLevel_Warning, "WARNING: REMOVE_NODE_STATUS_FAILED");
			state = ControllerState_Failed;
			break;
		}
		default:
		{
			break;
		}
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleControllerChangeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleControllerChangeRequest(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "FUNC_ID_ZW_CONTROLLER_CHANGE:");
	CommonAddNodeStatusRequestHandler( FUNC_ID_ZW_CONTROLLER_CHANGE, _data);
}

//-----------------------------------------------------------------------------
// <Driver::HandleCreateNewPrimaryRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleCreateNewPrimaryRequest(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "FUNC_ID_ZW_CREATE_NEW_PRIMARY:");
	CommonAddNodeStatusRequestHandler( FUNC_ID_ZW_CREATE_NEW_PRIMARY, _data);
}

//-----------------------------------------------------------------------------
// <Driver::HandleSetLearnModeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSetLearnModeRequest(uint8* _data)
{
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	if (m_currentControllerCommand == NULL)
	{
		return;
	}
	ControllerState state = m_currentControllerCommand->m_controllerState;
	Log::Write(LogLevel_Info, nodeId, "FUNC_ID_ZW_SET_LEARN_MODE:");

	switch (_data[3])
	{
		case LEARN_MODE_STARTED:
		{
			Log::Write(LogLevel_Info, nodeId, "LEARN_MODE_STARTED");
			state = ControllerState_Waiting;
			break;
		}
		case LEARN_MODE_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "LEARN_MODE_DONE");
			state = ControllerState_Completed;

			// Stop learn mode
			Internal::Msg* msg = new Internal::Msg("End Learn Mode", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false);
			msg->Append(0);
			SendMsg(msg, MsgQueue_Command);

			// Rebuild all the node info.  Group and scene data that we stored
			// during replication will be applied as we discover each node.
			InitAllNodes();
			break;
		}
		case LEARN_MODE_FAILED:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: LEARN_MODE_FAILED");
			state = ControllerState_Failed;

			// Stop learn mode
			Internal::Msg* msg = new Internal::Msg("End Learn Mode", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false);
			msg->Append(0);
			SendMsg(msg, MsgQueue_Command);

			// Rebuild all the node info, since it may have been partially
			// updated by the failed command.  Group and scene data that we
			// stored during replication will be applied as we discover each node.
			InitAllNodes();
			break;
		}
		case LEARN_MODE_DELETED:
		{
			Log::Write(LogLevel_Info, nodeId, "LEARN_MODE_DELETED");
			state = ControllerState_Failed;
			// Stop learn mode
			Internal::Msg* msg = new Internal::Msg("End Learn Mode", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false);
			msg->Append(0);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleRemoveFailedNodeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleRemoveFailedNodeRequest(uint8* _data)
{
	ControllerState state = ControllerState_Completed;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	switch (_data[3])
	{
		case FAILED_NODE_OK:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - Node %d is OK, so command failed", m_currentControllerCommand->m_controllerCommandNode);
			state = ControllerState_NodeOK;
			break;
		}
		case FAILED_NODE_REMOVED:
		{
			Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - node %d successfully moved to failed nodes list", m_currentControllerCommand->m_controllerCommandNode);
			state = ControllerState_Completed;
			{
				Internal::LockGuard LG(m_nodeMutex);
				delete m_nodes[m_currentControllerCommand->m_controllerCommandNode];
				m_nodes[m_currentControllerCommand->m_controllerCommandNode] = NULL;
			}
			WriteCache();
			Notification* notification = new Notification(Notification::Type_NodeRemoved);
			notification->SetHomeAndNodeIds(m_homeId, m_currentControllerCommand->m_controllerCommandNode);
			QueueNotification(notification);

			break;
		}
		case FAILED_NODE_NOT_REMOVED:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_REMOVE_FAILED_NODE_ID - unable to move node %d to failed nodes list", m_currentControllerCommand->m_controllerCommandNode);
			state = ControllerState_Failed;
			break;
		}
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleReplaceFailedNodeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleReplaceFailedNodeRequest(uint8* _data)
{
	ControllerState state = ControllerState_Completed;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	switch (_data[3])
	{
		case FAILED_NODE_OK:
		{
			Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Node is OK, so command failed");
			state = ControllerState_NodeOK;
			break;
		}
		case FAILED_NODE_REPLACE_WAITING:
		{
			Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Waiting for new node");
			state = ControllerState_Waiting;
			break;
		}
		case FAILED_NODE_REPLACE_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Node successfully replaced");
			state = ControllerState_Completed;

			// Request new node info for this device
			if (m_currentControllerCommand != NULL)
			{
				InitNode(m_currentControllerCommand->m_controllerCommandNode, true);
			}
			WriteCache();
			break;
		}
		case FAILED_NODE_REPLACE_FAILED:
		{
			Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_REPLACE_FAILED_NODE - Node replacement failed");
			state = ControllerState_Failed;
			break;
		}
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleApplicationCommandHandlerRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleApplicationCommandHandlerRequest(uint8* _data, bool encrypted)
{

	uint8 status = _data[2];
	uint8 nodeId = _data[3];
	uint8 classId = _data[5];
	Node* node = GetNodeUnsafe(nodeId);

	if ((status & RECEIVE_STATUS_ROUTED_BUSY) != 0)
	{
		m_routedbusy++;
	}
	if ((status & RECEIVE_STATUS_TYPE_BROAD) != 0)
	{
		m_broadcastReadCnt++;
	}
	if (node != NULL)
	{
		node->m_receivedCnt++;
		node->m_errors = 0;
		int cmp = memcmp(_data, node->m_lastReceivedMessage, sizeof(node->m_lastReceivedMessage));
		if (cmp == 0 && node->m_receivedTS.TimeRemaining() > -500)
		{
			// if the exact same sequence of bytes are received within 500ms
			node->m_receivedDups++;
		}
		else
		{
			memcpy(node->m_lastReceivedMessage, _data, sizeof(node->m_lastReceivedMessage));
		}
		node->m_receivedTS.SetTime();
		if (m_expectedReply == FUNC_ID_APPLICATION_COMMAND_HANDLER && m_expectedNodeId == nodeId)
		{
			// Need to confirm this is the correct response to the last sent request.
			// At least ignore any received messages prior to the send data request.
			node->m_lastResponseRTT = -node->m_sentTS.TimeRemaining();

			if (node->m_averageResponseRTT)
			{
				// if the average has been established, update by averaging the average and the last RTT
				node->m_averageResponseRTT = (node->m_averageResponseRTT + node->m_lastResponseRTT) >> 1;
			}
			else
			{
				// if this is the first observed RTT, set the average to this value
				node->m_averageResponseRTT = node->m_lastResponseRTT;
			}
			Log::Write(LogLevel_Info, nodeId, "Response RTT %d Average Response RTT %d", node->m_lastResponseRTT, node->m_averageResponseRTT);
		}
		else
		{
			node->m_receivedUnsolicited++;
		}
		if (!node->IsNodeAlive())
		{
			node->SetNodeAlive(true);
		}
	}
	if (Internal::CC::ApplicationStatus::StaticGetCommandClassId() == classId)
	{
		//TODO: Test this class function or implement
	}
	else if (Internal::CC::ControllerReplication::StaticGetCommandClassId() == classId)
	{
		if (m_controllerReplication && m_currentControllerCommand && (ControllerCommand_ReceiveConfiguration == m_currentControllerCommand->m_controllerCommand))
		{
			m_controllerReplication->HandleMsg(&_data[6], _data[4]);

			UpdateControllerState(ControllerState_InProgress);
		}
	}
	else
	{
		// Allow the node to handle the message itself
		if (node != NULL)
		{
			node->ApplicationCommandHandler(_data, encrypted);
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandlePromiscuousApplicationCommandHandlerRequest>
// Process a request from the Z-Wave PC interface when in promiscuous mode.
//-----------------------------------------------------------------------------
void Driver::HandlePromiscuousApplicationCommandHandlerRequest(uint8* _data)
{
	//uint8 nodeId = _data[3];
	//uint8 len = _data[4];
	//uint8 classId = _data[5];
	//uint8 destNodeId = _data[5+len];
}

//-----------------------------------------------------------------------------
// <Driver::HandleAssignReturnRouteRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleAssignReturnRouteRequest(uint8* _data)
{
	ControllerState state;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	if (m_currentControllerCommand == NULL)
	{
		return;
	}
	if (_data[3])
	{
		// Failed
		HandleErrorResponse(_data[3], m_currentControllerCommand->m_controllerCommandNode, "ZW_ASSIGN_RETURN_ROUTE", true);
		state = ControllerState_Failed;
	}
	else
	{
		// Success
		Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_ASSIGN_RETURN_ROUTE for node %d - SUCCESS", m_currentControllerCommand->m_controllerCommandNode);
		state = ControllerState_Completed;
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleDeleteReturnRouteRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleDeleteReturnRouteRequest(uint8* _data)
{
	ControllerState state;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	if (m_currentControllerCommand == NULL)
	{
		return;
	}
	if (_data[3] != TRANSMIT_COMPLETE_OK)
	{
		// Failed
		HandleErrorResponse(_data[3], m_currentControllerCommand->m_controllerCommandNode, "ZW_DELETE_RETURN_ROUTE", true);
		state = ControllerState_Failed;
	}
	else
	{
		// Success
		Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_DELETE_RETURN_ROUTE for node %d - SUCCESS", m_currentControllerCommand->m_controllerCommandNode);
		state = ControllerState_Completed;
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendNodeInformationRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendNodeInformationRequest(uint8* _data)
{
	ControllerState state;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	if (m_currentControllerCommand == NULL)
	{
		return;
	}
	if (_data[3] != TRANSMIT_COMPLETE_OK)
	{
		// Failed
		HandleErrorResponse(_data[3], m_currentControllerCommand->m_controllerCommandNode, "ZW_SEND_NODE_INFORMATION");
		state = ControllerState_Failed;
	}
	else
	{
		// Success
		Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_SEND_NODE_INFORMATION - SUCCESS");
		state = ControllerState_Completed;
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleNodeNeighborUpdateRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleNodeNeighborUpdateRequest(uint8* _data)
{
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	ControllerState state = ControllerState_Normal;
	switch (_data[3])
	{
		case REQUEST_NEIGHBOR_UPDATE_STARTED:
		{
			Log::Write(LogLevel_Info, nodeId, "REQUEST_NEIGHBOR_UPDATE_STARTED");
			state = ControllerState_InProgress;
			break;
		}
		case REQUEST_NEIGHBOR_UPDATE_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "REQUEST_NEIGHBOR_UPDATE_DONE");
			state = ControllerState_Completed;

			// We now request the neighbour information from the
			// controller and store it in our node object.
			if (m_currentControllerCommand != NULL)
			{
				RequestNodeNeighbors(m_currentControllerCommand->m_controllerCommandNode, 0);
			}
			break;
		}
		case REQUEST_NEIGHBOR_UPDATE_FAILED:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: REQUEST_NEIGHBOR_UPDATE_FAILED");
			state = ControllerState_Failed;
			break;
		}
		default:
		{
			break;
		}
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleApplicationUpdateRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleApplicationUpdateRequest(uint8* _data)
{
	bool messageRemoved = false;
	uint8 nodeId = _data[3];
	Node* node = GetNodeUnsafe(nodeId);

	// If node is not alive, mark it alive now
	if (node != NULL && !node->IsNodeAlive())
	{
		node->SetNodeAlive(true);
	}

	switch (_data[2])
	{
		case UPDATE_STATE_SUC_ID:
		{
			Log::Write(LogLevel_Info, nodeId, "UPDATE_STATE_SUC_ID from node %d", nodeId);
			m_SUCNodeId = nodeId; // need to confirm real data here
			break;
		}
		case UPDATE_STATE_DELETE_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "** Network change **: Z-Wave node %d was removed", nodeId);
			{
				Internal::LockGuard LG(m_nodeMutex);
				delete m_nodes[nodeId];
				m_nodes[nodeId] = NULL;
			}
			Notification* notification = new Notification(Notification::Type_NodeRemoved);
			notification->SetHomeAndNodeIds(m_homeId, nodeId);
			QueueNotification(notification);
			break;
		}
		case UPDATE_STATE_NEW_ID_ASSIGNED:
		{
			Log::Write(LogLevel_Info, nodeId, "** Network change **: ID %d was assigned to a new Z-Wave node", nodeId);
			// Check if the new node id is equal to the current one.... if so no operation is needed, thus no remove and add is necessary
			if (_data[3] != _data[6])
			{
				// Request the node protocol info (also removes any existing node and creates a new one)
				InitNode(nodeId);
			}
			else
			{
				Log::Write(LogLevel_Info, nodeId, "Not Re-assigning NodeID as old and new NodeID match");
			}

			break;
		}
		case UPDATE_STATE_ROUTING_PENDING:
		{
			Log::Write(LogLevel_Info, nodeId, "UPDATE_STATE_ROUTING_PENDING from node %d", nodeId);
			break;
		}
		case UPDATE_STATE_NODE_INFO_REQ_FAILED:
		{
			Log::Write(LogLevel_Warning, nodeId, "WARNING: FUNC_ID_ZW_APPLICATION_UPDATE: UPDATE_STATE_NODE_INFO_REQ_FAILED received");

			// Note: Unhelpfully, the nodeId is always zero in this message.  We have to
			// assume the message came from the last node to which we sent a request.
			if (m_currentMsg)
			{
				Node* tnode = GetNodeUnsafe(m_currentMsg->GetTargetNodeId());
				if (tnode)
				{
					// Retry the query twice
					tnode->QueryStageRetry(Node::QueryStage_NodeInfo, 2);

					// Just in case the failure was due to the node being asleep, we try
					// to move its pending messages to its wakeup queue.  If it is not
					// a sleeping device, this will have no effect.
					if (MoveMessagesToWakeUpQueue(tnode->GetNodeId(), true))
					{
						messageRemoved = true;
					}
				}
			}
			break;
		}
		case UPDATE_STATE_NODE_INFO_REQ_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "UPDATE_STATE_NODE_INFO_REQ_DONE from node %d", nodeId);
			break;
		}
		case UPDATE_STATE_NODE_INFO_RECEIVED:
		{
			Log::Write(LogLevel_Info, nodeId, "UPDATE_STATE_NODE_INFO_RECEIVED from node %d", nodeId);
			if (node)
			{
				node->UpdateNodeInfo(&_data[8], _data[4] - 3);
			}
			break;
		}
	}

	if (messageRemoved)
	{
		m_waitingForAck = false;
		m_expectedCallbackId = 0;
		m_expectedReply = 0;
		m_expectedCommandClassId = 0;
		m_expectedNodeId = 0;
	}

	return messageRemoved;
}

//-----------------------------------------------------------------------------
// <Driver::CommonAddNodeStatusRequestHandler>
// Handle common AddNode processing for many similar commands
//-----------------------------------------------------------------------------
void Driver::CommonAddNodeStatusRequestHandler(uint8 _funcId, uint8* _data)
{
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	ControllerState state = ControllerState_Normal;
	if (m_currentControllerCommand != NULL)
	{
		state = m_currentControllerCommand->m_controllerState;
	}
	switch (_data[3])
	{
		case ADD_NODE_STATUS_LEARN_READY:
		{
			Log::Write(LogLevel_Info, nodeId, "ADD_NODE_STATUS_LEARN_READY");
			m_currentControllerCommand->m_controllerAdded = false;
			state = ControllerState_Waiting;
			break;
		}
		case ADD_NODE_STATUS_NODE_FOUND:
		{
			Log::Write(LogLevel_Info, nodeId, "ADD_NODE_STATUS_NODE_FOUND");
			state = ControllerState_InProgress;
			break;
		}
		case ADD_NODE_STATUS_ADDING_SLAVE:
		{
			Log::Write(LogLevel_Info, nodeId, "ADD_NODE_STATUS_ADDING_SLAVE");
			Log::Write(LogLevel_Info, nodeId, "Adding node ID %d - %s", _data[4], m_currentControllerCommand->m_controllerCommandArg ? "Secure" : "Non-Secure");
			/* Discovered all the CC's are sent in this packet as well:
			 * position description
			 * 4 - Node ID
			 * 5 - Length
			 * 6 - Basic Device Class
			 * 7 - Generic Device Class
			 * 8 - Specific Device Class
			 * 9 to Length - Command Classes
			 * Last pck - CRC
			 */
			if (m_currentControllerCommand != NULL)
			{
				m_currentControllerCommand->m_controllerAdded = false;
				m_currentControllerCommand->m_controllerCommandNode = _data[4];
				/* make sure we dont overrun our buffer. Its ok to truncate */
				uint8 length = _data[5];
				if (length > 254)
					length = 254;
				memcpy(&m_currentControllerCommand->m_controllerDeviceProtocolInfo, &_data[6], length);
				m_currentControllerCommand->m_controllerDeviceProtocolInfoLength = length;
			}
			//			AddNodeStop( _funcId );
			//			sleep(1);
			break;
		}
		case ADD_NODE_STATUS_ADDING_CONTROLLER:
		{
			Log::Write(LogLevel_Info, nodeId, "ADD_NODE_STATUS_ADDING_CONTROLLER");
			Log::Write(LogLevel_Info, nodeId, "Adding controller ID %d", _data[4]);
			/* Discovered all the CC's are sent in this packet as well:
			 * position description
			 * 4 - Node ID
			 * 5 - Length
			 * 6 - Basic Device Class
			 * 7 - Generic Device Class
			 * 8 - Specific Device Class
			 * 9 to Length - Command Classes
			 * Last pck - CRC
			 */

			if (m_currentControllerCommand != NULL)
			{
				m_currentControllerCommand->m_controllerAdded = true;
				m_currentControllerCommand->m_controllerCommandNode = _data[4];
			}
			//			AddNodeStop( _funcId );
			break;
		}
		case ADD_NODE_STATUS_PROTOCOL_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "ADD_NODE_STATUS_PROTOCOL_DONE");
			// We added a device.
			// Get the controller out of add mode to avoid accidentally adding other devices.
			// We used to call replication here.
			AddNodeStop(_funcId);
			break;
		}
		case ADD_NODE_STATUS_DONE:
		{
			if (state == ControllerState_Failed)
			{
				/* if it was a failed add, we just move on */
				state = ControllerState_Completed;
				break;
			}

			Log::Write(LogLevel_Info, nodeId, "ADD_NODE_STATUS_DONE");
			state = ControllerState_Completed;
			if (m_currentControllerCommand != NULL && m_currentControllerCommand->m_controllerCommandNode != 0xff)
			{
				InitNode(m_currentControllerCommand->m_controllerCommandNode, true, m_currentControllerCommand->m_controllerCommandArg != 0, m_currentControllerCommand->m_controllerDeviceProtocolInfo, m_currentControllerCommand->m_controllerDeviceProtocolInfoLength);
			}

			// Not sure about the new controller function here.
			if (_funcId != FUNC_ID_ZW_ADD_NODE_TO_NETWORK && m_currentControllerCommand != NULL && m_currentControllerCommand->m_controllerAdded)
			{
				// Rebuild all the node info.  Group and scene data that we stored
				// during replication will be applied as we discover each node.
				InitAllNodes();
			}
			break;
		}
		case ADD_NODE_STATUS_FAILED:
		{
			Log::Write(LogLevel_Info, nodeId, "ADD_NODE_STATUS_FAILED");
			state = ControllerState_Failed;

			// Remove the AddNode command from the queue
			RemoveCurrentMsg();

			// Get the controller out of add mode to avoid accidentally adding other devices.
			AddNodeStop(_funcId);
			break;
		}
		default:
		{
			break;
		}
	}

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
//	Polling Z-Wave devices
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::EnablePoll>
// Enable polling of a value
//-----------------------------------------------------------------------------
bool Driver::EnablePoll(ValueID const &_valueId, uint8 const _intensity)
{
	// make sure the polling thread doesn't lock the node while we're in this function
	m_pollMutex->Lock();

	// confirm that this node exists
	uint8 nodeId = _valueId.GetNodeId();
	Internal::LockGuard LG(m_nodeMutex);
	Node* node = GetNode(nodeId);
	if (node != NULL)
	{
		// confirm that this value is in the node's value store
		if (Internal::VC::Value* value = node->GetValue(_valueId))
		{
			// update the value's pollIntensity
			value->SetPollIntensity(_intensity);

			// Add the valueid to the polling list
			// See if the node is already in the poll list.
			for (list<PollEntry>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it)
			{
				if ((*it).m_id == _valueId)
				{
					// It is already in the poll list, so we have nothing to do.
					Log::Write(LogLevel_Detail, "EnablePoll not required to do anything (value is already in the poll list)");
					value->Release();
					m_pollMutex->Unlock();
					return true;
				}
			}

			// Not in the list, so we add it
			PollEntry pe;
			pe.m_id = _valueId;
			pe.m_pollCounter = value->GetPollIntensity();
			m_pollList.push_back(pe);
			value->Release();
			m_pollMutex->Unlock();

			// send notification to indicate polling is enabled
			Notification* notification = new Notification(Notification::Type_PollingEnabled);
			notification->SetHomeAndNodeIds(m_homeId, _valueId.GetNodeId());
			notification->SetValueId(_valueId);
			QueueNotification(notification);
			Log::Write(LogLevel_Info, nodeId, "EnablePoll for HomeID 0x%.8x, value(cc=0x%02x,in=0x%02x,id=0x%02x)--poll list has %d items", _valueId.GetHomeId(), _valueId.GetCommandClassId(), _valueId.GetIndex(), _valueId.GetInstance(), m_pollList.size());
			return true;
		}

		// allow the poll thread to continue
		m_pollMutex->Unlock();

		Log::Write(LogLevel_Info, nodeId, "EnablePoll failed - value not found for node %d", nodeId);
		return false;
	}

	// allow the poll thread to continue
	m_pollMutex->Unlock();

	Log::Write(LogLevel_Info, "EnablePoll failed - node %d not found", nodeId);
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::DisablePoll>
// Disable polling of a node
//-----------------------------------------------------------------------------
bool Driver::DisablePoll(ValueID const &_valueId)
{
	// make sure the polling thread doesn't lock the node while we're in this function
	m_pollMutex->Lock();

	// confirm that this node exists
	uint8 nodeId = _valueId.GetNodeId();
	Internal::LockGuard LG(m_nodeMutex);
	Node* node = GetNode(nodeId);
	if (node != NULL)
	{
		// See if the value is already in the poll list.
		for (list<PollEntry>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it)
		{
			if ((*it).m_id == _valueId)
			{
				// Found it
				// remove it from the poll list
				m_pollList.erase(it);

				// get the value object and reset pollIntensity to zero (indicating no polling)
				Internal::VC::Value* value = GetValue(_valueId);
				if (!value)
					continue;
				value->SetPollIntensity(0);
				value->Release();
				m_pollMutex->Unlock();

				// send notification to indicate polling is disabled
				Notification* notification = new Notification(Notification::Type_PollingDisabled);
				notification->SetHomeAndNodeIds(m_homeId, _valueId.GetNodeId());
				notification->SetValueId(_valueId);
				QueueNotification(notification);
				Log::Write(LogLevel_Info, nodeId, "DisablePoll for HomeID 0x%.8x, value(cc=0x%02x,in=0x%02x,id=0x%02x)--poll list has %d items", _valueId.GetHomeId(), _valueId.GetCommandClassId(), _valueId.GetIndex(), _valueId.GetInstance(), m_pollList.size());
				return true;
			}
		}

		// Not in the list
		m_pollMutex->Unlock();
		Log::Write(LogLevel_Info, nodeId, "DisablePoll failed - value not on list");
		return false;
	}

	// allow the poll thread to continue
	m_pollMutex->Unlock();

	Log::Write(LogLevel_Info, "DisablePoll failed - node %d not found", nodeId);
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::isPolled>
// Check polling status of a value
//-----------------------------------------------------------------------------
bool Driver::isPolled(ValueID const &_valueId)
{
	bool bPolled;

	// make sure the polling thread doesn't lock the node while we're in this function
	m_pollMutex->Lock();

	Internal::VC::Value* value = GetValue(_valueId);
	if (value && value->GetPollIntensity() != 0)
	{
		bPolled = true;
	}
	else
	{
		bPolled = false;
	}

	if (value)
		value->Release();

	/*
	 * This code is retained for the moment as a belt-and-suspenders test to confirm that
	 * the pollIntensity member of each value and the pollList contents do not get out
	 * of sync.
	 */
	// confirm that this node exists
	uint8 nodeId = _valueId.GetNodeId();
	Internal::LockGuard LG(m_nodeMutex);
	Node* node = GetNode(nodeId);
	if (node != NULL)
	{

		// See if the value is already in the poll list.
		for (list<PollEntry>::iterator it = m_pollList.begin(); it != m_pollList.end(); ++it)
		{
			if ((*it).m_id == _valueId)
			{
				// Found it
				if (bPolled)
				{
					m_pollMutex->Unlock();
					return true;
				}
				else
				{
					Log::Write(LogLevel_Error, nodeId, "IsPolled setting for valueId 0x%016x is not consistent with the poll list", _valueId.GetId());
				}
			}
		}

		// Not in the list

		if (!bPolled)
		{
			m_pollMutex->Unlock();
			return false;
		}
		else
		{
			Log::Write(LogLevel_Error, nodeId, "IsPolled setting for valueId 0x%016x is not consistent with the poll list", _valueId.GetId());
		}
	}

	// allow the poll thread to continue
	m_pollMutex->Unlock();

	Log::Write(LogLevel_Info, "isPolled failed - node %d not found (the value reported that it is%s polled)", nodeId, bPolled ? "" : " not");
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::SetPollIntensity>
// Set the intensity with which this value is polled
//-----------------------------------------------------------------------------
void Driver::SetPollIntensity(ValueID const &_valueId, uint8 const _intensity)
{
	// make sure the polling thread doesn't lock the value while we're in this function
	m_pollMutex->Lock();

	Internal::VC::Value* value = GetValue(_valueId);
	if (!value)
		return;
	value->SetPollIntensity(_intensity);

	value->Release();
	m_pollMutex->Unlock();
}

//-----------------------------------------------------------------------------
// <Driver::PollThreadEntryPoint>
// Entry point of the thread for poll Z-Wave devices
//-----------------------------------------------------------------------------
void Driver::PollThreadEntryPoint(Internal::Platform::Event* _exitEvent, void* _context)
{
	Driver* driver = (Driver*) _context;
	if (driver)
	{
		driver->PollThreadProc(_exitEvent);
	}
}

//-----------------------------------------------------------------------------
// <Driver::PollThreadProc>
// Thread for poll Z-Wave devices
//-----------------------------------------------------------------------------
void Driver::PollThreadProc(Internal::Platform::Event* _exitEvent)
{
	while (1)
	{
		int32 pollInterval = m_pollInterval;

		if (m_awakeNodesQueried && !m_pollList.empty())
		{
			// We only bother getting the lock if the pollList is not empty
			m_pollMutex->Lock();

			// Get the next value to be polled
			PollEntry pe = m_pollList.front();
			m_pollList.pop_front();
			ValueID valueId = pe.m_id;

			// only execute this poll if pe.m_pollCounter == 1; otherwise decrement the counter and process the next polled value
			if (pe.m_pollCounter != 1)
			{
				pe.m_pollCounter--;
				m_pollList.push_back(pe);
				m_pollMutex->Unlock();
				continue;
			}

			// reset the poll counter to the full pollIntensity value and push it at the end of the list
			// release the value object referenced; call GetNode to ensure the node objects are locked during this period
			{
				Internal::LockGuard LG(m_nodeMutex);
				(void) GetNode(valueId.GetNodeId());
				Internal::VC::Value* value = GetValue(valueId);
				if (!value)
					continue;
				pe.m_pollCounter = value->GetPollIntensity();
				m_pollList.push_back(pe);
				value->Release();
			}
			// If the polling interval is for the whole poll list, calculate the time before the next poll,
			// so that all polls can take place within the user-specified interval.
			if (!m_bIntervalBetweenPolls)
			{
				if (pollInterval < 100)
				{
					Log::Write(LogLevel_Info, "The pollInterval setting is only %d, which appears to be a legacy setting.  Multiplying by 1000 to convert to ms.", pollInterval);
					pollInterval *= 1000;
				}
				pollInterval /= (int32) m_pollList.size();
			}

			{
				Internal::LockGuard LG(m_nodeMutex);
				// Request the state of the value from the node to which it belongs
				if (Node* node = GetNode(valueId.GetNodeId()))
				{
					bool requestState = true;
					if (!node->IsListeningDevice())
					{
						// The device is not awake all the time.  If it is not awake, we mark it
						// as requiring a poll.  The poll will be done next time the node wakes up.
						if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(node->GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
						{
							if (!wakeUp->IsAwake())
							{
								wakeUp->SetPollRequired();
								requestState = false;
							}
						}
					}

					if (requestState)
					{
						// Request an update of the value
						Internal::CC::CommandClass* cc = node->GetCommandClass(valueId.GetCommandClassId());
						if (cc)
						{
							uint16_t index = valueId.GetIndex();
							uint8_t instance = valueId.GetInstance();
							Log::Write(LogLevel_Detail, node->m_nodeId, "Polling: %s index = %d instance = %d (poll queue has %d messages)", cc->GetCommandClassName().c_str(), index, instance, m_msgQueue[MsgQueue_Poll].size());
							cc->RequestValue(0, index, instance, MsgQueue_Poll);
						}
					}

				}
			}

			m_pollMutex->Unlock();

			// Polling messages are only sent when there are no other messages waiting to be sent
			// While this makes the polls much more variable and uncertain if some other activity dominates
			// a send queue, that may be appropriate
			// TODO we can have a debate about whether to test all four queues or just the Poll queue
			// Wait until the library isn't actively sending messages (or in the midst of a transaction)
			int i32;
			int loopCount = 0;
			while (!m_msgQueue[MsgQueue_Poll].empty() || !m_msgQueue[MsgQueue_Send].empty() || !m_msgQueue[MsgQueue_Command].empty() || !m_msgQueue[MsgQueue_Query].empty() || m_currentMsg != NULL)
			{
				i32 = Internal::Platform::Wait::Single(_exitEvent, 10);		// test conditions every 10ms
				if (i32 == 0)
				{
					// Exit has been called
					return;
				}
				loopCount++;
				if (loopCount == 3000 * 10)		// 300 seconds worth of delay?  Something unusual is going on
				{
					Log::Write(LogLevel_Warning, "Poll queue hasn't been able to execute for 300 secs or more");
					Log::QueueDump();
					//					assert( 0 );
				}
			}

			// ready for next poll...insert the pollInterval delay
			i32 = Internal::Platform::Wait::Single(_exitEvent, pollInterval);
			if (i32 == 0)
			{
				// Exit has been called
				return;
			}
		}
		else		// poll list is empty or awake nodes haven't been fully queried yet
		{
			// don't poll just yet, wait for the pollInterval or exit before re-checking to see if the pollList has elements
			int32 i32 = Internal::Platform::Wait::Single(_exitEvent, 500);
			if (i32 == 0)
			{
				// Exit has been called
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
//	Retrieving Node information
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::InitAllNodes>
// Delete all nodes and fetch new node data from the Z-Wave network
//-----------------------------------------------------------------------------
void Driver::InitAllNodes()
{
	// Delete all the node data
	{
		Internal::LockGuard LG(m_nodeMutex);
		for (int i = 0; i < 256; ++i)
		{
			if (m_nodes[i])
			{
				delete m_nodes[i];
				m_nodes[i] = NULL;
			}
		}
	}
	// Fetch new node data from the Z-Wave network
	m_controller->PlayInitSequence(this);
}

//-----------------------------------------------------------------------------
// <Driver::InitNode>
// Queue a node to be interrogated for its setup details
//-----------------------------------------------------------------------------
void Driver::InitNode(uint8 const _nodeId, bool newNode, bool secure, uint8 const *_protocolInfo, uint8 const _length)
{
	// Delete any existing node and replace it with a new one
	{
		Internal::LockGuard LG(m_nodeMutex);
		if (m_nodes[_nodeId])
		{
			// Remove the original node
			delete m_nodes[_nodeId];
			WriteCache();
			Notification* notification = new Notification(Notification::Type_NodeRemoved);
			notification->SetHomeAndNodeIds(m_homeId, _nodeId);
			QueueNotification(notification);
		}

		// Add the new node
		m_nodes[_nodeId] = new Node(m_homeId, _nodeId);
		if (newNode == true)
			static_cast<Node *>(m_nodes[_nodeId])->SetAddingNode();
	}

	Notification* notification = new Notification(Notification::Type_NodeAdded);
	notification->SetHomeAndNodeIds(m_homeId, _nodeId);
	QueueNotification(notification);

	if (_length == 0)
	{
		// Request the node info
		m_nodes[_nodeId]->SetQueryStage(Node::QueryStage_ProtocolInfo);
	}
	else
	{
		if (isNetworkKeySet())
			m_nodes[_nodeId]->SetSecured(secure);
		else
			Log::Write(LogLevel_Info, _nodeId, "Network Key Not Set - Secure Option is %s", secure ? "required" : "not required");
		m_nodes[_nodeId]->SetProtocolInfo(_protocolInfo, _length);
	}
	Log::Write(LogLevel_Info, _nodeId, "Initializing Node. New Node: %s (%s)", static_cast<Node *>(m_nodes[_nodeId])->IsAddingNode() ? "true" : "false", newNode ? "true" : "false");
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeListeningDevice>
// Get whether the node is a listening device that does not go to sleep
//-----------------------------------------------------------------------------
bool Driver::IsNodeListeningDevice(uint8 const _nodeId)
{
	bool res = false;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		res = node->IsListeningDevice();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeFrequentListeningDevice>
// Get whether the node is a listening device that does not go to sleep
//-----------------------------------------------------------------------------
bool Driver::IsNodeFrequentListeningDevice(uint8 const _nodeId)
{
	bool res = false;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		res = node->IsFrequentListeningDevice();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeBeamingDevice>
// Get whether the node is a beam capable device.
//-----------------------------------------------------------------------------
bool Driver::IsNodeBeamingDevice(uint8 const _nodeId)
{
	bool res = false;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		res = node->IsBeamingDevice();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeRoutingDevice>
// Get whether the node is a routing device that passes messages to other nodes
//-----------------------------------------------------------------------------
bool Driver::IsNodeRoutingDevice(uint8 const _nodeId)
{
	bool res = false;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		res = node->IsRoutingDevice();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::IsNodeSecurityDevice>
// Get the security attribute for a node
//-----------------------------------------------------------------------------
bool Driver::IsNodeSecurityDevice(uint8 const _nodeId)
{
	bool security = false;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		security = node->IsSecurityDevice();
	}

	return security;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeMaxBaudRate>
// Get the maximum baud rate of a node's communications
//-----------------------------------------------------------------------------
uint32 Driver::GetNodeMaxBaudRate(uint8 const _nodeId)
{
	uint32 baud = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		baud = node->GetMaxBaudRate();
	}

	return baud;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeVersion>
// Get the version number of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeVersion(uint8 const _nodeId)
{
	uint8 version = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		version = node->GetVersion();
	}

	return version;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeSecurity>
// Get the security byte of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeSecurity(uint8 const _nodeId)
{
	uint8 security = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		security = node->GetSecurity();
	}

	return security;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeBasic>
// Get the basic type of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeBasic(uint8 const _nodeId)
{
	uint8 basic = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		basic = node->GetBasic();
	}

	return basic;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeGeneric>
// Get the generic type of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeGeneric(uint8 const _nodeId)
{
	uint8 genericType = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		genericType = node->GetGeneric();
	}

	return genericType;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeSpecific>
// Get the specific type of a node
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeSpecific(uint8 const _nodeId)
{
	uint8 specific = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		specific = node->GetSpecific();
	}

	return specific;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeType>
// Get the basic/generic/specific type of the specified node
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeType(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetType();
	}

	return "Unknown";
}

bool Driver::IsNodeZWavePlus(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->IsNodeZWavePlus();
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeNeighbors>
// Gets the neighbors for a node
//-----------------------------------------------------------------------------
uint32 Driver::GetNodeNeighbors(uint8 const _nodeId, uint8** o_neighbors)
{
	uint32 numNeighbors = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		numNeighbors = node->GetNeighbors(o_neighbors);
	}

	return numNeighbors;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeManufacturerName>
// Get the manufacturer name for the node with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeManufacturerName(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetManufacturerName();
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeProductName>
// Get the product name for the node with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeProductName(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetProductName();
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeName>
// Get the user-editable name for the node with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeName(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetNodeName();
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeLocation>
// Get the user-editable string for location of the specified node
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
string Driver::GetNodeLocation(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetLocation();
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeManufacturerId>
// Get the manufacturer Id string value with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
uint16 Driver::GetNodeManufacturerId(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetManufacturerId();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeProductType>
// Get the product type string value with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
uint16 Driver::GetNodeProductType(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetProductType();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeProductId>
// Get the product Id string value with the specified ID
// Returns a copy of the string rather than a const ref for thread safety
//-----------------------------------------------------------------------------
uint16 Driver::GetNodeProductId(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetProductId();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeDeviceType>
// Get the node device type as reported in the Z-Wave+ Info report.
//-----------------------------------------------------------------------------
uint16 Driver::GetNodeDeviceType(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetDeviceType();
	}

	return 0x00; // unknown
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeDeviceTypeString>
// Get the node DeviceType as a string as reported in the Z-Wave+ Info report.
//-----------------------------------------------------------------------------

string Driver::GetNodeDeviceTypeString(uint8 const _nodeId)
{

	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetDeviceTypeString();
	}

	return ""; // unknown
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeRole>
// Get the node role as reported in the Z-Wave+ Info report.
//-----------------------------------------------------------------------------
uint8 Driver::GetNodeRole(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetRoleType();
	}

	return 0x00; // unknown
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeRoleString>
// Get the node role as a string as reported in the Z-Wave+ Info report.
//-----------------------------------------------------------------------------
string Driver::GetNodeRoleString(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetRoleTypeString();
	}

	return ""; // unknown
}

//-----------------------------------------------------------------------------
// <Driver::GetNodePlusType>
// Get the node role as a string as reported in the Z-Wave+ Info report.
//-----------------------------------------------------------------------------
uint8 Driver::GetNodePlusType(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetNodeType();
	}
	return 0x00; // unknown
}

//-----------------------------------------------------------------------------
// <Driver::GetNodePlusTypeString>
// Get the node role as a string as reported in the Z-Wave+ Info report.
//-----------------------------------------------------------------------------
string Driver::GetNodePlusTypeString(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->GetNodeTypeString();
	}
	return ""; // unknown
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeManufacturerName>
// Set the manufacturer name for the node with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeManufacturerName(uint8 const _nodeId, string const& _manufacturerName)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->SetManufacturerName(_manufacturerName);
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeProductName>
// Set the product name string value with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeProductName(uint8 const _nodeId, string const& _productName)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->SetProductName(_productName);
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeName>
// Set the node name string value with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeName(uint8 const _nodeId, string const& _nodeName)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->SetNodeName(_nodeName);
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeLocation>
// Set the location string value with the specified ID
//-----------------------------------------------------------------------------
void Driver::SetNodeLocation(uint8 const _nodeId, string const& _location)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->SetLocation(_location);
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeLevel>
// Helper to set the node level through the basic command class
//-----------------------------------------------------------------------------
void Driver::SetNodeLevel(uint8 const _nodeId, uint8 const _level)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->SetLevel(_level);
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeOn>
// Helper to set the node on through the basic command class
//-----------------------------------------------------------------------------
void Driver::SetNodeOn(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->SetNodeOn();
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetNodeOff>
// Helper to set the node off through the basic command class
//-----------------------------------------------------------------------------
void Driver::SetNodeOff(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->SetNodeOff();
	}
}

//-----------------------------------------------------------------------------
// <Driver::GetValue>
// Get a pointer to a Value object for the specified ValueID
//-----------------------------------------------------------------------------
Internal::VC::Value* Driver::GetValue(ValueID const& _id)
{

	// This method is only called by code that has already locked the node
	if (Node* node = m_nodes[_id.GetNodeId()])
	{
		return node->GetValue(_id);
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Controller commands
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::ResetController>
// Reset controller and erase all node information
//-----------------------------------------------------------------------------
void Driver::ResetController(Internal::Platform::Event* _evt)
{
	m_controllerResetEvent = _evt;
	Log::Write(LogLevel_Info, "Reset controller and erase all node information");
	Internal::Msg* msg = new Internal::Msg("Reset controller and erase all node information", 0xff, REQUEST, FUNC_ID_ZW_SET_DEFAULT, true);
	SendMsg(msg, MsgQueue_Command);
}

//-----------------------------------------------------------------------------
// <Driver::SoftReset>
// Soft-reset the Z-Wave controller chip
//-----------------------------------------------------------------------------
void Driver::SoftReset()
{
	Log::Write(LogLevel_Info, "Soft-resetting the Z-Wave controller chip");
	Internal::Msg* msg = new Internal::Msg("Soft-resetting the Z-Wave controller chip", 0xff, REQUEST, FUNC_ID_SERIAL_API_SOFT_RESET, false, false);
	SendMsg(msg, MsgQueue_Command);
}

//-----------------------------------------------------------------------------
// <Driver::RequestNodeNeighbors>
// Get the neighbour information for a node from the controller
//-----------------------------------------------------------------------------
void Driver::RequestNodeNeighbors(uint8 const _nodeId, uint32 const _requestFlags)
{
	if (IsAPICallSupported( FUNC_ID_ZW_GET_ROUTING_INFO))
	{
		// Note: This is not the same as RequestNodeNeighbourUpdate.  This method
		// merely requests the controller's current neighbour information and
		// the reply will be copied into the relevant Node object for later use.
		Log::Write(LogLevel_Detail, GetNodeNumber(m_currentMsg), "Requesting routing info (neighbor list) for Node %d", _nodeId);
		Internal::Msg* msg = new Internal::Msg("Get Routing Info", _nodeId, REQUEST, FUNC_ID_ZW_GET_ROUTING_INFO, false);
		msg->Append(_nodeId);
		msg->Append(0); // don't remove bad links
		msg->Append(0); // don't remove non-repeaters
		msg->Append(3); // funcid
		SendMsg(msg, MsgQueue_Command);
	}
}

//-----------------------------------------------------------------------------
// <Driver::BeginControllerCommand>
// Start the controller performing one of its network management functions
// Create a ControllerCommand request.
//-----------------------------------------------------------------------------
bool Driver::BeginControllerCommand(ControllerCommand _command, pfnControllerCallback_t _callback, void* _context, bool _highPower, uint8 _nodeId, uint8 _arg)
{
	ControllerCommandItem* cci;
	MsgQueueItem item;

	if (_command == ControllerCommand_None)
	{
		return false;
	}

	Log::Write(LogLevel_Detail, _nodeId, "Queuing (%s) %s", c_sendQueueNames[MsgQueue_Controller], c_controllerCommandNames[_command]);
	cci = new ControllerCommandItem();
	cci->m_controllerCommand = _command;
	cci->m_controllerCallback = _callback;
	cci->m_controllerCallbackContext = _context;
	cci->m_highPower = _highPower;
	cci->m_controllerCommandNode = _nodeId;
	cci->m_controllerCommandArg = _arg;
	cci->m_controllerState = ControllerState_Normal;
	cci->m_controllerStateChanged = false;
	cci->m_controllerCommandDone = false;

	item.m_command = MsgQueueCmd_Controller;
	item.m_cci = cci;

	m_sendMutex->Lock();
	m_msgQueue[MsgQueue_Controller].push_back(item);
	m_queueEvent[MsgQueue_Controller]->Set();
	m_sendMutex->Unlock();

	return true;
}

//-----------------------------------------------------------------------------
// <Driver::DoControllerCommand>
// Start the controller performing one of its network management functions
//-----------------------------------------------------------------------------
void Driver::DoControllerCommand()
{
	UpdateControllerState(ControllerState_Starting);
	switch (m_currentControllerCommand->m_controllerCommand)
	{
		case ControllerCommand_AddDevice:
		{
			if (!IsPrimaryController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotPrimary);
			}
			else
			{
				Log::Write(LogLevel_Info, 0, "Add Device");
				Internal::Msg* msg = new Internal::Msg("ControllerCommand_AddDevice", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true);
				uint8 options = ADD_NODE_ANY;
				if (m_currentControllerCommand->m_highPower)
					options |= OPTION_HIGH_POWER;
				if (IsAPICallSupported(FUNC_ID_ZW_EXPLORE_REQUEST_INCLUSION))
					options |= OPTION_NWI;
				msg->Append(options);
				SendMsg(msg, MsgQueue_Command);
			}
			break;
		}
		case ControllerCommand_CreateNewPrimary:
		{
			if (IsPrimaryController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotSecondary);
			}
			else if (!IsStaticUpdateController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotSUC);
			}
			else
			{
				Log::Write(LogLevel_Info, 0, "Create New Primary");
				Internal::Msg* msg = new Internal::Msg("ControllerCommand_CreateNewPrimary", 0xff, REQUEST, FUNC_ID_ZW_CREATE_NEW_PRIMARY, true);
				msg->Append( CREATE_PRIMARY_START);
				SendMsg(msg, MsgQueue_Command);
			}
			break;
		}
		case ControllerCommand_ReceiveConfiguration:
		{
			Log::Write(LogLevel_Info, 0, "Receive Configuration");
			Internal::Msg* msg = new Internal::Msg("ControllerCommand_ReceiveConfiguration", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, true);
			msg->Append(0xff);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_RemoveDevice:
		{
			if (!IsPrimaryController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotPrimary);
			}
			else
			{
				Log::Write(LogLevel_Info, 0, "Remove Device");
				Internal::Msg* msg = new Internal::Msg("ControllerCommand_RemoveDevice", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, true);
				msg->Append(m_currentControllerCommand->m_highPower ? REMOVE_NODE_ANY | OPTION_HIGH_POWER : REMOVE_NODE_ANY);
				SendMsg(msg, MsgQueue_Command);
			}
			break;
		}
		case ControllerCommand_HasNodeFailed:
		{
			Log::Write(LogLevel_Info, 0, "Requesting whether node %d has failed", m_currentControllerCommand->m_controllerCommandNode);
			Internal::Msg* msg = new Internal::Msg("ControllerCommand_HasNodeFailed", 0xff, REQUEST, FUNC_ID_ZW_IS_FAILED_NODE_ID, false);
			msg->Append(m_currentControllerCommand->m_controllerCommandNode);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_RemoveFailedNode:
		{
			Log::Write(LogLevel_Info, 0, "ControllerCommand_RemoveFailedNode", m_currentControllerCommand->m_controllerCommandNode);
			Internal::Msg* msg = new Internal::Msg("ControllerCommand_RemoveFailedNode", 0xff, REQUEST, FUNC_ID_ZW_REMOVE_FAILED_NODE_ID, true);
			msg->Append(m_currentControllerCommand->m_controllerCommandNode);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_ReplaceFailedNode:
		{
			Log::Write(LogLevel_Info, 0, "Replace Failed Node %d", m_currentControllerCommand->m_controllerCommandNode);
			Internal::Msg* msg = new Internal::Msg("ControllerCommand_ReplaceFailedNode", 0xff, REQUEST, FUNC_ID_ZW_REPLACE_FAILED_NODE, true);
			msg->Append(m_currentControllerCommand->m_controllerCommandNode);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_TransferPrimaryRole:
		{
			if (!IsPrimaryController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotPrimary);
			}
			else
			{
				Log::Write(LogLevel_Info, 0, "Transfer Primary Role");
				Internal::Msg* msg = new Internal::Msg("ControllerCommand_TransferPrimaryRole", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true);
				msg->Append(m_currentControllerCommand->m_highPower ? CONTROLLER_CHANGE_START | OPTION_HIGH_POWER : CONTROLLER_CHANGE_START);
				SendMsg(msg, MsgQueue_Command);
			}
			break;
		}
		case ControllerCommand_RequestNetworkUpdate:
		{
			if (!IsStaticUpdateController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotSUC);
			}
			else
			{
				Log::Write(LogLevel_Info, 0, "Request Network Update");
				Internal::Msg* msg = new Internal::Msg("ControllerCommand_RequestNetworkUpdate", 0xff, REQUEST, FUNC_ID_ZW_REQUEST_NETWORK_UPDATE, true);
				SendMsg(msg, MsgQueue_Command);
			}
			break;
		}
		case ControllerCommand_RequestNodeNeighborUpdate:
		{
			if (!IsPrimaryController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotPrimary);
			}
			else
			{
				Log::Write(LogLevel_Info, 0, "Requesting Neighbor Update for node %d", m_currentControllerCommand->m_controllerCommandNode);
				bool opts = IsAPICallSupported( FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE_OPTIONS);
				Internal::Msg* msg;
				if (opts)
				{
					msg = new Internal::Msg("ControllerCommand_RequestNodeNeighborUpdate", m_currentControllerCommand->m_controllerCommandNode, REQUEST, FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE_OPTIONS, true);
				}
				else
				{
					msg = new Internal::Msg("ControllerCommand_RequestNodeNeighborUpdate", m_currentControllerCommand->m_controllerCommandNode, REQUEST, FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE, true);
				}
				msg->Append(m_currentControllerCommand->m_controllerCommandNode);
				if (opts)
				{
					msg->Append(GetTransmitOptions());
				}
				SendMsg(msg, MsgQueue_Command);
			}
			break;
		}
		case ControllerCommand_AssignReturnRoute:
		{
			Log::Write(LogLevel_Info, 0, "Assigning return route from node %d to node %d", m_currentControllerCommand->m_controllerCommandNode, m_currentControllerCommand->m_controllerCommandArg);
			Internal::Msg* msg = new Internal::Msg("ControllerCommand_AssignReturnRoute", m_currentControllerCommand->m_controllerCommandNode, REQUEST, FUNC_ID_ZW_ASSIGN_RETURN_ROUTE, true);
			msg->Append(m_currentControllerCommand->m_controllerCommandNode);		// from the node
			msg->Append(m_currentControllerCommand->m_controllerCommandArg);		// to the specific destination
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_DeleteAllReturnRoutes:
		{
			Log::Write(LogLevel_Info, 0, "Deleting all return routes from node %d", m_currentControllerCommand->m_controllerCommandNode);
			Internal::Msg* msg = new Internal::Msg("ControllerCommand_DeleteAllReturnRoutess", m_currentControllerCommand->m_controllerCommandNode, REQUEST, FUNC_ID_ZW_DELETE_RETURN_ROUTE, true);
			msg->Append(m_currentControllerCommand->m_controllerCommandNode);		// from the node
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_SendNodeInformation:
		{
			Log::Write(LogLevel_Info, 0, "Sending a node information frame");
			Internal::Msg* msg = new Internal::Msg("ControllerCommand_SendNodeInformation", m_currentControllerCommand->m_controllerCommandNode, REQUEST, FUNC_ID_ZW_SEND_NODE_INFORMATION, true);
			msg->Append(m_currentControllerCommand->m_controllerCommandNode);		// to the node
			msg->Append(GetTransmitOptions());
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_ReplicationSend:
		{
			if (!IsPrimaryController())
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotPrimary);
			}
			else
			{
				Log::Write(LogLevel_Info, 0, "Replication Send");
				Internal::Msg* msg = new Internal::Msg("ControllerCommand_ReplicationSend", 0xff, REQUEST, FUNC_ID_ZW_ADD_NODE_TO_NETWORK, true);
				msg->Append(m_currentControllerCommand->m_highPower ? ADD_NODE_CONTROLLER | OPTION_HIGH_POWER : ADD_NODE_CONTROLLER);
				SendMsg(msg, MsgQueue_Command);
			}
			break;
		}
		case ControllerCommand_CreateButton:
		{
			if (IsBridgeController())
			{
				Node* node = GetNodeUnsafe(m_currentControllerCommand->m_controllerCommandNode);
				if (node != NULL)
				{
					if (node->m_buttonMap.find(m_currentControllerCommand->m_controllerCommandArg) == node->m_buttonMap.end() && m_virtualNeighborsReceived)
					{
						bool found = false;
						for (uint8 n = 1; n <= 232 && !found; n++)
						{
							if (!IsVirtualNode(n))
								continue;

							map<uint8, uint8>::iterator it = node->m_buttonMap.begin();
							for (; it != node->m_buttonMap.end(); ++it)
							{
								// is virtual node already in map?
								if (it->second == n)
									break;
							}
							if (it == node->m_buttonMap.end()) // found unused virtual node
							{
								node->m_buttonMap[m_currentControllerCommand->m_controllerCommandArg] = n;
								SendVirtualNodeInfo(n, m_currentControllerCommand->m_controllerCommandNode);
								found = true;
							}
						}
						if (!found) // create a new virtual node
						{
							Log::Write(LogLevel_Info, 0, "AddVirtualNode");
							Internal::Msg* msg = new Internal::Msg("FUNC_ID_SERIAL_API_SLAVE_NODE_INFO", 0xff, REQUEST, FUNC_ID_SERIAL_API_SLAVE_NODE_INFO, false, false);
							msg->Append(0);		// node 0
							msg->Append(1);		// listening
							msg->Append(0x09);		// genericType window covering
							msg->Append(0x00);		// specificType undefined
							msg->Append(0);		// length
							SendMsg(msg, MsgQueue_Command);

							msg = new Internal::Msg("FUNC_ID_ZW_SET_SLAVE_LEARN_MODE", 0xff, REQUEST, FUNC_ID_ZW_SET_SLAVE_LEARN_MODE, true);
							msg->Append(0);		// node 0 to add
							if (IsPrimaryController() || IsInclusionController())
							{
								msg->Append( SLAVE_LEARN_MODE_ADD);
							}
							else
							{
								msg->Append( SLAVE_LEARN_MODE_ENABLE);
							}
							SendMsg(msg, MsgQueue_Command);
						}
					}
					else
					{
						UpdateControllerState(ControllerState_Error, ControllerError_ButtonNotFound);
					}
				}
				else
				{
					UpdateControllerState(ControllerState_Error, ControllerError_NodeNotFound);
				}
			}
			else
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotBridge);
			}
			break;
		}
		case ControllerCommand_DeleteButton:
		{
			if (IsBridgeController())
			{
				Node* node = GetNodeUnsafe(m_currentControllerCommand->m_controllerCommandNode);
				if (node != NULL)
				{
					// Make sure button is allocated to a virtual node.
					if (node->m_buttonMap.find(m_currentControllerCommand->m_controllerCommandArg) != node->m_buttonMap.end())
					{
#ifdef notdef
						// We would need a reference count to decide when to free virtual nodes
						// We could do this by making the bitmap of virtual nodes into a map that also holds a reference count.
						Log::Write( LogLevel_Info, 0, "RemoveVirtualNode %d", m_currentControllerCommand->m_controllerCommandNode );
						Msg* msg = new Msg( "Remove Virtual Node", 0xff, REQUEST, FUNC_ID_ZW_SET_SLAVE_LEARN_MODE, true );
						msg->Append( m_currentControllerCommand->m_controllerCommandNode );// from the node
						if( IsPrimaryController() || IsInclusionController() )
						msg->Append( SLAVE_LEARN_MODE_REMOVE );
						else
						msg->Append( SLAVE_LEARN_MODE_ENABLE );
						SendMsg( msg );
#endif
						node->m_buttonMap.erase(m_currentControllerCommand->m_controllerCommandArg);
						SaveButtons();

						Notification* notification = new Notification(Notification::Type_DeleteButton);
						notification->SetHomeAndNodeIds(m_homeId, m_currentControllerCommand->m_controllerCommandNode);
						notification->SetButtonId(m_currentControllerCommand->m_controllerCommandArg);
						QueueNotification(notification);
					}
					else
					{
						UpdateControllerState(ControllerState_Error, ControllerError_ButtonNotFound);
					}
				}
				else
				{
					UpdateControllerState(ControllerState_Error, ControllerError_NodeNotFound);
				}
			}
			else
			{
				UpdateControllerState(ControllerState_Error, ControllerError_NotBridge);
			}
			break;
		}
		case ControllerCommand_None:
		{
			// To keep gcc quiet
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::UpdateControllerState>
// Stop the current controller function
//-----------------------------------------------------------------------------
void Driver::UpdateControllerState(ControllerState const _state, ControllerError const _error)
{
	if (m_currentControllerCommand != NULL)
	{
		if (_state != m_currentControllerCommand->m_controllerState)
		{
			m_currentControllerCommand->m_controllerStateChanged = true;
			m_currentControllerCommand->m_controllerState = _state;
			switch (_state)
			{
				case ControllerState_Error:
				case ControllerState_Cancel:
				case ControllerState_Failed:
				case ControllerState_Sleeping:
				case ControllerState_NodeFailed:
				case ControllerState_NodeOK:
				case ControllerState_Completed:
				{
					m_currentControllerCommand->m_controllerCommandDone = true;
					m_sendMutex->Lock();
					m_queueEvent[MsgQueue_Controller]->Set();
					m_sendMutex->Unlock();
					break;
				}
				default:
				{
					break;
				}
			}

		}
		Notification* notification = new Notification(Notification::Type_ControllerCommand);

        // PR #1879
        // The change below sets the nodeId in the notifications for controller state changes. These state changes are
        // caused by controller commands. Below is a list of controller commands with what the nodeId gets set to,
        // along with the Manager method(s) that use the controller command.

        // Driver::ControllerCommand_RequestNodeNeighborUpdate: supplied nodeId (Manager::HealNetworkNode, Manager::HealNetwork)
        // Driver::ControllerCommand_AddDevice: nodeId of an added node (Manager::AddNode)
        // Driver::ControllerCommand_RemoveDevice: nodeId of a removed node (Manager::RemoveNode)
        // Driver::ControllerCommand_RemoveFailedNode: supplied nodeId (Manager::RemoveFailedNode)
        // Driver::ControllerCommand_HasNodeFailed supplied nodeId (Manager::HasNodeFailed)
        // Driver::ControllerCommand_AssignReturnRoute: supplied nodeId (Manager::AssignReturnRoute)
        // Driver::ControllerCommand_RequestNodeNeighborUpdate: supplied nodeId (Manager::RequestNodeNeighborUpdate)
        // Driver::ControllerCommand_DeleteAllReturnRoutes supplied nodeId (Manager::DeleteAllReturnRoutes)
        // Driver::ControllerCommand_SendNodeInformation: supplied nodeId (Manager::SendNodeInformation)
        // Driver::ControllerCommand_CreateNewPrimary: unknown (Manager::CreateNewPrimary)
        // Driver::ControllerCommand_ReceiveConfiguration: unknown (Manager::ReceiveConfiguration)
        // Driver::ControllerCommand_ReplaceFailedNode: could be the supplied nodeId or the nodeId of the node that was added (Manager::ReplaceFailedNode)
        // Driver::ControllerCommand_TransferPrimaryRole: unknown (Manager::TransferPrimaryRole)
        // Driver::ControllerCommand_RequestNetworkUpdate: supplied nodeId (Manager::RequestNetworkUpdate)
        // Driver::ControllerCommand_ReplicationSend: supplied nodeId (Manager::ReplicationSend)
        // Driver::ControllerCommand_CreateButton: supplied nodeId (Manager::CreateButton)
        // Driver::ControllerCommand_DeleteButton: supplied nodeId (Manager::DeleteButton)
		notification->SetHomeAndNodeIds(m_homeId, m_currentControllerCommand->m_controllerCommandNode);

		notification->SetCommand(m_currentControllerCommand->m_controllerCommand);
		notification->SetEvent(_state);

		if (_error != ControllerError_None)
		{
			m_currentControllerCommand->m_controllerReturnError = _error;
			/* Create a new Notification Callback */
			notification->SetNotification(_error);
		}
		QueueNotification(notification);
	}
}

//-----------------------------------------------------------------------------
// <Driver::CancelControllerCommand>
// Stop the current controller function
//-----------------------------------------------------------------------------
bool Driver::CancelControllerCommand()
{
	if (m_currentControllerCommand == NULL)
	{
		// Controller is not doing anything
		return false;
	}

	switch (m_currentControllerCommand->m_controllerCommand)
	{
		case ControllerCommand_AddDevice:
		{
			Log::Write(LogLevel_Info, 0, "Cancel Add Node");
			m_currentControllerCommand->m_controllerCommandNode = 0xff;		// identify the fact that there is no new node to initialize
			AddNodeStop( FUNC_ID_ZW_ADD_NODE_TO_NETWORK);
			break;
		}
		case ControllerCommand_CreateNewPrimary:
		{
			Log::Write(LogLevel_Info, 0, "Cancel Create New Primary");
			Internal::Msg* msg = new Internal::Msg("CreateNewPrimary Stop", 0xff, REQUEST, FUNC_ID_ZW_CREATE_NEW_PRIMARY, true);
			msg->Append( CREATE_PRIMARY_STOP);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_ReceiveConfiguration:
		{
			Log::Write(LogLevel_Info, 0, "Cancel Receive Configuration");
			Internal::Msg* msg = new Internal::Msg("ReceiveConfiguration Stop", 0xff, REQUEST, FUNC_ID_ZW_SET_LEARN_MODE, false, false);
			msg->Append(0);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_RemoveDevice:
		{
			Log::Write(LogLevel_Info, 0, "Cancel Remove Device");
			m_currentControllerCommand->m_controllerCommandNode = 0xff;		// identify the fact that there is no node to remove
			AddNodeStop( FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK);
			break;
		}
		case ControllerCommand_TransferPrimaryRole:
		{
			Log::Write(LogLevel_Info, 0, "Cancel Transfer Primary Role");
			Internal::Msg* msg = new Internal::Msg("Transfer Primary Role Stop", 0xff, REQUEST, FUNC_ID_ZW_CONTROLLER_CHANGE, true);
			msg->Append( CONTROLLER_CHANGE_STOP);
			SendMsg(msg, MsgQueue_Command);
			break;
		}
		case ControllerCommand_ReplicationSend:
		{
			Log::Write(LogLevel_Info, 0, "Cancel Replication Send");
			m_currentControllerCommand->m_controllerCommandNode = 0xff;		// identify the fact that there is no new node to initialize
			AddNodeStop( FUNC_ID_ZW_ADD_NODE_TO_NETWORK);
			break;
		}
		case ControllerCommand_CreateButton:
		case ControllerCommand_DeleteButton:
		{
			if (m_currentControllerCommand->m_controllerCommandNode != 0)
			{
				SendSlaveLearnModeOff();
			}
			break;
		}
		case ControllerCommand_None:
		case ControllerCommand_RequestNetworkUpdate:
		case ControllerCommand_RequestNodeNeighborUpdate:
		case ControllerCommand_AssignReturnRoute:
		case ControllerCommand_DeleteAllReturnRoutes:
		case ControllerCommand_RemoveFailedNode:
		case ControllerCommand_HasNodeFailed:
		case ControllerCommand_ReplaceFailedNode:
		case ControllerCommand_SendNodeInformation:
		{
			// Cannot cancel
			return false;
		}
	}

	UpdateControllerState(ControllerState_Cancel);
	return true;
}

//-----------------------------------------------------------------------------
// <Driver::AddNodeStop>
// Stop the Add Node mode based on API of controller
//-----------------------------------------------------------------------------
void Driver::AddNodeStop(uint8 const _funcId)
{
	if (m_currentControllerCommand == NULL)
	{
		// Controller is not doing anything
		return;
	}

	if (m_serialAPIVersion[0] == 2 && m_serialAPIVersion[1] == 76)
	{
		Internal::Msg* msg = new Internal::Msg("Add Node Stop", 0xff, REQUEST, _funcId, false, false);
		msg->Append( ADD_NODE_STOP);
		SendMsg(msg, Driver::MsgQueue_Command);
	}
	else
	{
		Internal::Msg* msg = new Internal::Msg("Add Node Stop", 0xff, REQUEST, _funcId, false, true);
		msg->Append( ADD_NODE_STOP);
		SendMsg(msg, Driver::MsgQueue_Command);
	}
}

//-----------------------------------------------------------------------------
// <Driver::TestNetwork>
// Run a series of messages to a single node or every node on the network.
//-----------------------------------------------------------------------------
void Driver::TestNetwork(uint8 const _nodeId, uint32 const _count)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (_nodeId == 0)	// send _count messages to every node
	{
		for (int i = 0; i < 256; ++i)
		{
			if (i == m_Controller_nodeId) // ignore sending to ourself
			{
				continue;
			}
			if (m_nodes[i] != NULL)
			{
				Internal::CC::NoOperation *noop = static_cast<Internal::CC::NoOperation*>(m_nodes[i]->GetCommandClass(Internal::CC::NoOperation::StaticGetCommandClassId()));
				for (int j = 0; j < (int) _count; j++)
				{
					noop->Set(true);
				}
			}
		}
	}
	else if (_nodeId != m_Controller_nodeId && m_nodes[_nodeId] != NULL)
	{
		Internal::CC::NoOperation *noop = static_cast<Internal::CC::NoOperation*>(m_nodes[_nodeId]->GetCommandClass(Internal::CC::NoOperation::StaticGetCommandClassId()));
		for (int i = 0; i < (int) _count; i++)
		{
			noop->Set(true);
		}
	}
}

//-----------------------------------------------------------------------------
//	SwitchAll
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Driver::SwitchAllOn>
// All devices that support the SwitchAll command class will be turned on
//-----------------------------------------------------------------------------
void Driver::SwitchAllOn()
{
	Internal::CC::SwitchAll::On(this, 0xff);

	Internal::LockGuard LG(m_nodeMutex);
	for (int i = 0; i < 256; ++i)
	{
		if (GetNodeUnsafe(i))
		{
			if (m_nodes[i]->GetCommandClass(Internal::CC::SwitchAll::StaticGetCommandClassId()))
			{
				Internal::CC::SwitchAll::On(this, (uint8) i);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::SwitchAllOff>
// All devices that support the SwitchAll command class will be turned off
//-----------------------------------------------------------------------------
void Driver::SwitchAllOff()
{
	Internal::CC::SwitchAll::Off(this, 0xff);

	Internal::LockGuard LG(m_nodeMutex);
	for (int i = 0; i < 256; ++i)
	{
		if (GetNodeUnsafe(i))
		{
			if (m_nodes[i]->GetCommandClass(Internal::CC::SwitchAll::StaticGetCommandClassId()))
			{
				Internal::CC::SwitchAll::Off(this, (uint8) i);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::SetConfigParam>
// Set the value of one of the configuration parameters of a device
//-----------------------------------------------------------------------------
bool Driver::SetConfigParam(uint8 const _nodeId, uint8 const _param, int32 _value, uint8 _size)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		return node->SetConfigParam(_param, _value, _size);
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Driver::RequestConfigParam>
// Request the value of one of the configuration parameters of a device
//-----------------------------------------------------------------------------
void Driver::RequestConfigParam(uint8 const _nodeId, uint8 const _param)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->RequestConfigParam(_param);
	}
}

//-----------------------------------------------------------------------------
// <Driver::GetNumGroups>
// Gets the number of association groups reported by this node
//-----------------------------------------------------------------------------
uint8 Driver::GetNumGroups(uint8 const _nodeId)
{
	uint8 numGroups = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		numGroups = node->GetNumGroups();
	}

	return numGroups;
}

//-----------------------------------------------------------------------------
// <Driver::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Driver::GetAssociations(uint8 const _nodeId, uint8 const _groupIdx, uint8** o_associations)
{
	uint32 numAssociations = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		numAssociations = node->GetAssociations(_groupIdx, o_associations);
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <Driver::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Driver::GetAssociations(uint8 const _nodeId, uint8 const _groupIdx, InstanceAssociation** o_associations)
{
	uint32 numAssociations = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		numAssociations = node->GetAssociations(_groupIdx, o_associations);
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <Driver::GetMaxAssociations>
// Gets the maximum number of associations for a group
//-----------------------------------------------------------------------------
uint8 Driver::GetMaxAssociations(uint8 const _nodeId, uint8 const _groupIdx)
{
	uint8 maxAssociations = 0;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		maxAssociations = node->GetMaxAssociations(_groupIdx);
	}

	return maxAssociations;
}

//-----------------------------------------------------------------------------
// <Driver::IsMultiInstance>
// Returns true if group supports multi instance
//-----------------------------------------------------------------------------
bool Driver::IsMultiInstance(uint8 const _nodeId, uint8 const _groupIdx)
{
	bool multiInstance = false;
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		multiInstance = node->IsMultiInstance(_groupIdx);
	}
	return multiInstance;
}

//-----------------------------------------------------------------------------
// <Driver::GetGroupLabel>
// Gets the label for a particular group
//-----------------------------------------------------------------------------
string Driver::GetGroupLabel(uint8 const _nodeId, uint8 const _groupIdx)
{
	string label = "";
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		label = node->GetGroupLabel(_groupIdx);
	}

	return label;
}

//-----------------------------------------------------------------------------
// <Driver::AddAssociation>
// Adds a node to an association group
//-----------------------------------------------------------------------------
void Driver::AddAssociation(uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId, uint8 const _instance)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->AddAssociation(_groupIdx, _targetNodeId, _instance);
	}
}

//-----------------------------------------------------------------------------
// <Driver::RemoveAssociation>
// Removes a node from an association group
//-----------------------------------------------------------------------------
void Driver::RemoveAssociation(uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId, uint8 const _instance)
{
	Internal::LockGuard LG(m_nodeMutex);
	if (Node* node = GetNode(_nodeId))
	{
		node->RemoveAssociation(_groupIdx, _targetNodeId, _instance);
	}
}

//-----------------------------------------------------------------------------
// <Driver::QueueNotification>
// Add a notification to the queue to be sent at a later, safe time.
//-----------------------------------------------------------------------------
void Driver::QueueNotification(Notification* _notification)
{
	m_notifications.push_back(_notification);
	m_notificationsEvent->Set();
}

//-----------------------------------------------------------------------------
// <Driver::NotifyWatchers>
// Notify any watching objects of a value change
//-----------------------------------------------------------------------------
void Driver::NotifyWatchers()
{
	list<Notification*>::iterator nit = m_notifications.begin();
	while (nit != m_notifications.end())
	{
		Notification* notification = m_notifications.front();
		m_notifications.pop_front();

		/* check the any ValueID's sent as part of the Notification are still valid */
		switch (notification->GetType())
		{
			case Notification::Type_ValueAdded:
			case Notification::Type_ValueChanged:
			case Notification::Type_ValueRefreshed:
			{
				Internal::VC::Value *val = GetValue(notification->GetValueID());
				if (!val)
				{
					Log::Write(LogLevel_Info, notification->GetNodeId(), "Dropping Notification as ValueID does not exist");
					nit = m_notifications.begin();
					delete notification;
					val->Release();
					continue;
				}
				val->Release();
				break;
			}
			default:
				break;
		}

		Log::Write(LogLevel_Detail, notification->GetNodeId(), "Notification: %s", notification->GetAsString().c_str());

		Manager::Get()->NotifyWatchers(notification);

		delete notification;
		nit = m_notifications.begin();
	}
	m_notificationsEvent->Reset();
}

//-----------------------------------------------------------------------------
// <Driver::HandleRfPowerLevelSetResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleRfPowerLevelSetResponse(uint8* _data)
{
	bool res = true;
	// the meaning of this command is currently unclear, and there
	// isn't any returned response data, so just log the function call
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_R_F_POWER_LEVEL_SET");

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleSerialApiSetTimeoutsResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleSerialApiSetTimeoutsResponse(uint8* _data)
{
	// the meaning of this command and its response is currently unclear
	bool res = true;
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_SERIAL_API_SET_TIMEOUTS");
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleMemoryGetByteResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleMemoryGetByteResponse(uint8* _data)
{
	bool res = true;
	// the meaning of this command and its response is currently unclear
	// it seems to return three bytes of data, so print them out
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_ZW_MEMORY_GET_BYTE, returned data: 0x%02hx 0x%02hx 0x%02hx", _data[0], _data[1], _data[2]);

	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleReadMemoryResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleReadMemoryResponse(uint8* _data)
{
	// the meaning of this command and its response is currently unclear
	bool res = true;
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "Received reply to FUNC_ID_MEMORY_GET_BYTE");
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleGetVirtualNodesResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleGetVirtualNodesResponse(uint8* _data)
{
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_GET_VIRTUAL_NODES");
	memcpy(m_virtualNeighbors, &_data[2], 29);
	m_virtualNeighborsReceived = true;
	bool bNeighbors = false;
	for (int by = 0; by < 29; by++)
	{
		for (int bi = 0; bi < 8; bi++)
		{
			if ((_data[2 + by] & (0x01 << bi)))
			{
				Log::Write(LogLevel_Info, nodeId, "    Node %d", (by << 3) + bi + 1);
				bNeighbors = true;
			}
		}
	}
	if (!bNeighbors)
		Log::Write(LogLevel_Info, nodeId, "    (none reported)");
}

//-----------------------------------------------------------------------------
// <Driver::GetVirtualNeighbors>
// Gets the virtual neighbors for a network
//-----------------------------------------------------------------------------
uint32 Driver::GetVirtualNeighbors(uint8** o_neighbors)
{
	int i;
	uint32 numNeighbors = 0;
	if (!m_virtualNeighborsReceived)
	{
		*o_neighbors = NULL;
		return 0;
	}
	for (i = 0; i < 29; i++)
	{
		for (unsigned char mask = 0x80; mask != 0; mask >>= 1)
			if (m_virtualNeighbors[i] & mask)
				numNeighbors++;
	}

	// handle the possibility that no neighbors are reported
	if (!numNeighbors)
	{
		*o_neighbors = NULL;
		return 0;
	}

	// create and populate an array with neighbor node ids
	uint8* neighbors = new uint8[numNeighbors];
	uint32 index = 0;
	for (int by = 0; by < 29; by++)
	{
		for (int bi = 0; bi < 8; bi++)
		{
			if ((m_virtualNeighbors[by] & (0x01 << bi)))
				neighbors[index++] = ((by << 3) + bi + 1);
		}
	}

	*o_neighbors = neighbors;
	return numNeighbors;
}

//-----------------------------------------------------------------------------
// <Driver::RequestVirtualNeighbors>
// Get the virtual neighbour information from the controller
//-----------------------------------------------------------------------------
void Driver::RequestVirtualNeighbors(MsgQueue const _queue)
{
	Internal::Msg* msg = new Internal::Msg("Get Virtual Neighbor List", 0xff, REQUEST, FUNC_ID_ZW_GET_VIRTUAL_NODES, false);
	SendMsg(msg, _queue);
}

//-----------------------------------------------------------------------------
// <Driver::SendVirtualNodeInfo>
// Send node info frame on behalf of a virtual node.
//-----------------------------------------------------------------------------
void Driver::SendVirtualNodeInfo(uint8 const _FromNodeId, uint8 const _ToNodeId)
{
	char str[80];

	snprintf(str, sizeof(str), "Send Virtual Node Info from %d to %d", _FromNodeId, _ToNodeId);
	Internal::Msg* msg = new Internal::Msg(str, 0xff, REQUEST, FUNC_ID_ZW_SEND_SLAVE_NODE_INFO, true);
	msg->Append(_FromNodeId);		// from the virtual node
	msg->Append(_ToNodeId);		// to the handheld controller
	msg->Append( TRANSMIT_OPTION_ACK);
	SendMsg(msg, MsgQueue_Command);
}

//-----------------------------------------------------------------------------
// <Driver::SendSlaveLearnModeOff>
// Disable Slave Learn Mode.
//-----------------------------------------------------------------------------
void Driver::SendSlaveLearnModeOff()
{
	if (!(IsPrimaryController() || IsInclusionController()))
	{
		Internal::Msg* msg = new Internal::Msg("Set Slave Learn Mode Off ", 0xff, REQUEST, FUNC_ID_ZW_SET_SLAVE_LEARN_MODE, true);
		msg->Append(0);	// filler node id
		msg->Append( SLAVE_LEARN_MODE_DISABLE);
		SendMsg(msg, MsgQueue_Command);
	}
}

//-----------------------------------------------------------------------------
// <Driver::SaveButtons>
// Save button info into file.
//-----------------------------------------------------------------------------
void Driver::SaveButtons()
{
	char str[16];

	// Create a new XML document to contain the driver configuration
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	TiXmlElement* nodesElement = new TiXmlElement("Nodes");
	doc.LinkEndChild(decl);
	doc.LinkEndChild(nodesElement);

	nodesElement->SetAttribute("xmlns", "http://code.google.com/p/open-zwave/");

	snprintf(str, sizeof(str), "%d", 1);
	nodesElement->SetAttribute("version", str);
	Internal::LockGuard LG(m_nodeMutex);
	for (int i = 1; i < 256; i++)
	{
		if (m_nodes[i] == NULL || m_nodes[i]->m_buttonMap.empty())
		{
			continue;
		}

		TiXmlElement* nodeElement = new TiXmlElement("Node");

		snprintf(str, sizeof(str), "%d", i);
		nodeElement->SetAttribute("id", str);

		for (map<uint8, uint8>::iterator it = m_nodes[i]->m_buttonMap.begin(); it != m_nodes[i]->m_buttonMap.end(); ++it)
		{
			TiXmlElement* valueElement = new TiXmlElement("Button");

			snprintf(str, sizeof(str), "%d", it->first);
			valueElement->SetAttribute("id", str);

			snprintf(str, sizeof(str), "%d", it->second);
			TiXmlText* textElement = new TiXmlText(str);
			valueElement->LinkEndChild(textElement);

			nodeElement->LinkEndChild(valueElement);
		}

		nodesElement->LinkEndChild(nodeElement);
	}

	string userPath;
	Options::Get()->GetOptionAsString("UserPath", &userPath);

	string filename = userPath + "zwbutton.xml";

	doc.SaveFile(filename.c_str());
}
//-----------------------------------------------------------------------------
// <Driver::ReadButtons>
// Read button info per node from file.
//-----------------------------------------------------------------------------
void Driver::ReadButtons(uint8 const _nodeId)
{
	int32 intVal;
	int32 nodeId;
	int32 buttonId;
	char const* str;

	// Load the XML document that contains the driver configuration
	string userPath;
	Options::Get()->GetOptionAsString("UserPath", &userPath);

	string filename = userPath + "zwbutton.xml";

	TiXmlDocument doc;
	if (!doc.LoadFile(filename.c_str(), TIXML_ENCODING_UTF8))
	{
		Log::Write(LogLevel_Debug, "Driver::ReadButtons - zwbutton.xml file not found.");
		return;
	}
	doc.SetUserData((void *) filename.c_str());
	TiXmlElement const* nodesElement = doc.RootElement();
	str = nodesElement->Value();
	if (str && strcmp(str, "Nodes"))
	{
		Log::Write(LogLevel_Warning, "WARNING: Driver::ReadButtons - zwbutton.xml is malformed");
		return;
	}

	// Version
	if (TIXML_SUCCESS == nodesElement->QueryIntAttribute("version", &intVal))
	{
		if ((uint32) intVal != 1)
		{
			Log::Write(LogLevel_Info, "Driver::ReadButtons - %s is from an older version of OpenZWave and cannot be loaded.", "zwbutton.xml");
			return;
		}
	}
	else
	{
		Log::Write(LogLevel_Warning, "WARNING: Driver::ReadButtons - zwbutton.xml is from an older version of OpenZWave and cannot be loaded.");
		return;
	}

	TiXmlElement const* nodeElement = nodesElement->FirstChildElement();
	while (nodeElement)
	{
		str = nodeElement->Value();
		if (str && !strcmp(str, "Node"))
		{
			Node* node = NULL;
			if (TIXML_SUCCESS == nodeElement->QueryIntAttribute("id", &intVal))
			{
				if (_nodeId == intVal)
				{
					node = GetNodeUnsafe(intVal);
				}
			}
			if (node != NULL)
			{
				TiXmlElement const* buttonElement = nodeElement->FirstChildElement();
				while (buttonElement)
				{
					str = buttonElement->Value();
					if (str && !strcmp(str, "Button"))
					{
						if (TIXML_SUCCESS != buttonElement->QueryIntAttribute("id", &buttonId))
						{
							Log::Write(LogLevel_Warning, "WARNING: Driver::ReadButtons - cannot find Button Id for node %d", _nodeId);
							return;
						}
						str = buttonElement->GetText();
						if (str)
						{
							char *p;
							nodeId = (int32) strtol(str, &p, 0);
						}
						else
						{
							Log::Write(LogLevel_Info, "Driver::ReadButtons - missing virtual node value for node %d button id %d", _nodeId, buttonId);
							return;
						}
						node->m_buttonMap[buttonId] = nodeId;
						Notification* notification = new Notification(Notification::Type_CreateButton);
						notification->SetHomeAndNodeIds(m_homeId, nodeId);
						notification->SetButtonId(buttonId);
						QueueNotification(notification);
					}
					buttonElement = buttonElement->NextSiblingElement();
				}
			}
		}
		nodeElement = nodeElement->NextSiblingElement();
	}
}
//-----------------------------------------------------------------------------
// <Driver::HandleSetSlaveLearnModeResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleSetSlaveLearnModeResponse(uint8* _data)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	if (_data[2])
	{
		Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_SET_SLAVE_LEARN_MODE - command in progress");
	}
	else
	{
		// Failed
		Log::Write(LogLevel_Warning, nodeId, "WARNING: Received reply to FUNC_ID_ZW_SET_SLAVE_LEARN_MODE - command failed");
		state = ControllerState_Failed;
		res = false;
		SendSlaveLearnModeOff();
	}

	UpdateControllerState(state);
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleSetSlaveLearnModeRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSetSlaveLearnModeRequest(uint8* _data)
{
	ControllerState state = ControllerState_Waiting;
	uint8 nodeId = GetNodeNumber(m_currentMsg);

	if (m_currentControllerCommand == NULL)
	{
		return;
	}

	SendSlaveLearnModeOff();
	switch (_data[3])
	{
		case SLAVE_ASSIGN_COMPLETE:
		{
			Log::Write(LogLevel_Info, nodeId, "SLAVE_ASSIGN_COMPLETE");
			if (_data[4] == 0) // original node is 0 so adding
			{
				Log::Write(LogLevel_Info, nodeId, "Adding virtual node ID %d", _data[5]);
				Node* node = GetNodeUnsafe(m_currentControllerCommand->m_controllerCommandNode);
				if (node != NULL)
				{
					node->m_buttonMap[m_currentControllerCommand->m_controllerCommandArg] = _data[5];
					SendVirtualNodeInfo(_data[5], m_currentControllerCommand->m_controllerCommandNode);
				}
			}
			else if (_data[5] == 0)
			{
				Log::Write(LogLevel_Info, nodeId, "Removing virtual node ID %d", _data[4]);
			}
			break;
		}
		case SLAVE_ASSIGN_NODEID_DONE:
		{
			Log::Write(LogLevel_Info, nodeId, "SLAVE_ASSIGN_NODEID_DONE");
			if (_data[4] == 0) // original node is 0 so adding
			{
				Log::Write(LogLevel_Info, nodeId, "Adding virtual node ID %d", _data[5]);
				Node* node = GetNodeUnsafe(m_currentControllerCommand->m_controllerCommandNode);
				if (node != NULL)
				{
					node->m_buttonMap[m_currentControllerCommand->m_controllerCommandArg] = _data[5];
					SendVirtualNodeInfo(_data[5], m_currentControllerCommand->m_controllerCommandNode);
				}
			}
			else if (_data[5] == 0)
			{
				Log::Write(LogLevel_Info, nodeId, "Removing virtual node ID %d", _data[4]);
			}
			break;
		}
		case SLAVE_ASSIGN_RANGE_INFO_UPDATE:
		{
			Log::Write(LogLevel_Info, nodeId, "SLAVE_ASSIGN_RANGE_INFO_UPDATE");
			break;
		}
	}
	m_currentControllerCommand->m_controllerAdded = false;

	UpdateControllerState(state);
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendSlaveNodeInfoResponse>
// Process a response from the Z-Wave PC interface
//-----------------------------------------------------------------------------
bool Driver::HandleSendSlaveNodeInfoResponse(uint8* _data)
{
	bool res = true;
	ControllerState state = ControllerState_InProgress;
	uint8 nodeId = GetNodeNumber(m_currentMsg);
	if (m_currentControllerCommand == NULL)
	{
		return false;
	}
	if (_data[2])
	{
		Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_SEND_SLAVE_NODE_INFO - command in progress");
	}
	else
	{
		// Failed
		Log::Write(LogLevel_Info, nodeId, "Received reply to FUNC_ID_ZW_SEND_SLAVE_NODE_INFO - command failed");
		state = ControllerState_Failed;
		// Undo button map settings
		Node* node = GetNodeUnsafe(m_currentControllerCommand->m_controllerCommandNode);
		if (node != NULL)
		{
			node->m_buttonMap.erase(m_currentControllerCommand->m_controllerCommandArg);
		}
		res = false;
	}

	UpdateControllerState(state);
	return res;
}

//-----------------------------------------------------------------------------
// <Driver::HandleSendSlaveNodeInfoRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleSendSlaveNodeInfoRequest(uint8* _data)
{
	if (m_currentControllerCommand == NULL)
	{
		return;
	}
	if (_data[3] == TRANSMIT_COMPLETE_OK)	// finish up
	{
		Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "SEND_SLAVE_NODE_INFO_COMPLETE OK");
		SaveButtons();
		Notification* notification = new Notification(Notification::Type_CreateButton);
		notification->SetHomeAndNodeIds(m_homeId, m_currentControllerCommand->m_controllerCommandNode);
		notification->SetButtonId(m_currentControllerCommand->m_controllerCommandArg);
		QueueNotification(notification);

		UpdateControllerState(ControllerState_Completed);
		RequestVirtualNeighbors(MsgQueue_Send);
	}
	else			// error. try again
	{
		HandleErrorResponse(_data[3], m_currentControllerCommand->m_controllerCommandNode, "SLAVE_NODE_INFO_COMPLETE");
		Node* node = GetNodeUnsafe(m_currentControllerCommand->m_controllerCommandNode);
		if (node != NULL)
		{
			SendVirtualNodeInfo(node->m_buttonMap[m_currentControllerCommand->m_controllerCommandArg], m_currentControllerCommand->m_controllerCommandNode);
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::HandleApplicationSlaveCommandRequest>
// Process a request from the Z-Wave PC interface
//-----------------------------------------------------------------------------
void Driver::HandleApplicationSlaveCommandRequest(uint8* _data)
{
	Log::Write(LogLevel_Info, GetNodeNumber(m_currentMsg), "APPLICATION_SLAVE_COMMAND_HANDLER rxStatus %x dest %d source %d len %d", _data[2], _data[3], _data[4], _data[5]);
	Node* node = GetNodeUnsafe(_data[4]);
	if (node != NULL && _data[5] == 3 && _data[6] == 0x20 && _data[7] == 0x01) // only support Basic Set for now
	{
		map<uint8, uint8>::iterator it = node->m_buttonMap.begin();
		for (; it != node->m_buttonMap.end(); ++it)
		{
			if (it->second == _data[3])
				break;
		}
		if (it != node->m_buttonMap.end())
		{
			Notification *notification;
			if (_data[8] == 0)
			{
				notification = new Notification(Notification::Type_ButtonOff);
			}
			else
			{
				notification = new Notification(Notification::Type_ButtonOn);
			}
			notification->SetHomeAndNodeIds(m_homeId, _data[4]);
			notification->SetButtonId(it->first);
			QueueNotification(notification);
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::NodeFromMessage>
// See if we can get node from incoming message data
//-----------------------------------------------------------------------------
uint8 Driver::NodeFromMessage(uint8 const* buffer)
{
	uint8 nodeId = 0;

	if (buffer[1] >= 5)
	{
		switch (buffer[3])
		{
			case FUNC_ID_APPLICATION_COMMAND_HANDLER:
				nodeId = buffer[5];
				break;
			case FUNC_ID_ZW_APPLICATION_UPDATE:
				nodeId = buffer[5];
				break;
		}
	}
	return nodeId;
}
//-----------------------------------------------------------------------------
// <Driver::UpdateNodeRoutes>
// Update a node's routing information
//-----------------------------------------------------------------------------
void Driver::UpdateNodeRoutes(uint8 const _nodeId, bool _doUpdate		// = false
		)
{
	// Only for routing slaves
	Node* node = GetNodeUnsafe(_nodeId);
	if (node != NULL && node->GetBasic() == 0x04)
	{
		uint8 numGroups = GetNumGroups(_nodeId);
		uint8 numNodes = 0;
		uint8 nodes[5];
		InstanceAssociation* associations;
		uint8 i;

		// Determine up to 5 destinations

		memset(nodes, 0, sizeof(nodes));
		for (i = 1; i <= numGroups && numNodes < sizeof(nodes); i++)
		{
			associations = NULL;
			uint32 len = GetAssociations(_nodeId, i, &associations);
			for (uint8 j = 0; j < len; j++)
			{
				uint8 k;
				/* there is a gcc bug that triggers here: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59124
				 * see also https://github.com/OpenZWave/open-zwave/issues/586
				 */
				for (k = 0; k < numNodes && k < sizeof(nodes); k++)
				{
					if (nodes[k] == associations[j].m_nodeId)
					{
						break;
					}
				}
				if (k >= numNodes && numNodes < sizeof(nodes))	// not in list so add it
				{
					nodes[numNodes++] = associations[j].m_nodeId;
				}
			}
			if (associations != NULL)
			{
				delete[] associations;
			}
		}
		if (_doUpdate || numNodes != node->m_numRouteNodes || memcmp(nodes, node->m_routeNodes, sizeof(node->m_routeNodes)) != 0)
		{
			// Figure out what to do if one of these fail.
			BeginControllerCommand(ControllerCommand_DeleteAllReturnRoutes, NULL, NULL, true, _nodeId, 0);
			for (i = 0; i < numNodes; i++)
			{
				BeginControllerCommand(ControllerCommand_AssignReturnRoute, NULL, NULL, true, _nodeId, nodes[i]);
			}
			node->m_numRouteNodes = numNodes;
			memcpy(node->m_routeNodes, nodes, sizeof(nodes));
		}
	}
}

//-----------------------------------------------------------------------------
// <Driver::GetDriverStatistics>
// Return driver statistics
//-----------------------------------------------------------------------------
void Driver::GetDriverStatistics(DriverData* _data)
{
	_data->m_SOFCnt = m_SOFCnt;
	_data->m_ACKWaiting = m_ACKWaiting;
	_data->m_readAborts = m_readAborts;
	_data->m_badChecksum = m_badChecksum;
	_data->m_readCnt = m_readCnt;
	_data->m_writeCnt = m_writeCnt;
	_data->m_CANCnt = m_CANCnt;
	_data->m_NAKCnt = m_NAKCnt;
	_data->m_ACKCnt = m_ACKCnt;
	_data->m_OOFCnt = m_OOFCnt;
	_data->m_dropped = m_dropped;
	_data->m_retries = m_retries;
	_data->m_callbacks = m_callbacks;
	_data->m_badroutes = m_badroutes;
	_data->m_noack = m_noack;
	_data->m_netbusy = m_netbusy;
	_data->m_notidle = m_notidle;
	_data->m_txverified = m_txverified;
	_data->m_nondelivery = m_nondelivery;
	_data->m_routedbusy = m_routedbusy;
	_data->m_broadcastReadCnt = m_broadcastReadCnt;
	_data->m_broadcastWriteCnt = m_broadcastWriteCnt;
}

//-----------------------------------------------------------------------------
// <Driver::GetNodeStatistics>
// Return per node statistics
//-----------------------------------------------------------------------------
void Driver::GetNodeStatistics(uint8 const _nodeId, Node::NodeData* _data)
{
	Internal::LockGuard LG(m_nodeMutex);
	Node* node = GetNode(_nodeId);
	if (node != NULL)
	{
		node->GetNodeStatistics(_data);
	}
}

//-----------------------------------------------------------------------------
// <Driver::LogDriverStatistics>
// Report driver statistics to the driver's log
//-----------------------------------------------------------------------------
void Driver::LogDriverStatistics()
{
	DriverData data;

	GetDriverStatistics(&data);
	int32 totalElapsed = -m_startTime.TimeRemaining();
	int32 days = totalElapsed / (1000 * 60 * 60 * 24);

	totalElapsed -= days * 1000 * 60 * 60 * 24;
	int32 hours = totalElapsed / (1000 * 60 * 60);

	totalElapsed -= hours * 1000 * 60 * 60;
	int32 minutes = totalElapsed / (1000 * 60);

	Log::Write(LogLevel_Always, "***************************************************************************");
	Log::Write(LogLevel_Always, "*********************  Cumulative Network Statistics  *********************");
	Log::Write(LogLevel_Always, "*** General");
	Log::Write(LogLevel_Always, "Driver run time: . .  . %ld days, %ld hours, %ld minutes", days, hours, minutes);
	Log::Write(LogLevel_Always, "Frames processed: . . . . . . . . . . . . . . . . . . . . %ld", data.m_SOFCnt);
	Log::Write(LogLevel_Always, "Total messages successfully received: . . . . . . . . . . %ld", data.m_readCnt);
	Log::Write(LogLevel_Always, "Total Messages successfully sent: . . . . . . . . . . . . %ld", data.m_writeCnt);
	Log::Write(LogLevel_Always, "ACKs received from controller:  . . . . . . . . . . . . . %ld", data.m_ACKCnt);
	// Consider tracking and adding:
	//		Initialization messages
	//		Ad-hoc command messages
	//		Polling messages
	//		Messages inititated by network
	//		Others?
	Log::Write(LogLevel_Always, "*** Errors");
	Log::Write(LogLevel_Always, "Unsolicited messages received while waiting for ACK:  . . %ld", data.m_ACKWaiting);
	Log::Write(LogLevel_Always, "Reads aborted due to timeouts:  . . . . . . . . . . . . . %ld", data.m_readAborts);
	Log::Write(LogLevel_Always, "Bad checksum errors:  . . . . . . . . . . . . . . . . . . %ld", data.m_badChecksum);
	Log::Write(LogLevel_Always, "CANs received from controller:  . . . . . . . . . . . . . %ld", data.m_CANCnt);
	Log::Write(LogLevel_Always, "NAKs received from controller:  . . . . . . . . . . . . . %ld", data.m_NAKCnt);
	Log::Write(LogLevel_Always, "Out of frame data flow errors:  . . . . . . . . . . . . . %ld", data.m_OOFCnt);
	Log::Write(LogLevel_Always, "Messages retransmitted: . . . . . . . . . . . . . . . . . %ld", data.m_retries);
	Log::Write(LogLevel_Always, "Messages dropped and not delivered: . . . . . . . . . . . %ld", data.m_dropped);
	Log::Write(LogLevel_Always, "***************************************************************************");
}

//-----------------------------------------------------------------------------
// <Driver::GetNetworkKey>
// Get the Network Key we will use for Security Command Class
//-----------------------------------------------------------------------------
uint8 *Driver::GetNetworkKey()
{
	std::string networkKey;
	std::vector<std::string> elems;
	unsigned int tempkey[16];
	static uint8 keybytes[16] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	static bool keySet = false;
	if (keySet == false)
	{
		Options::Get()->GetOptionAsString("NetworkKey", &networkKey);
		Internal::split(elems, networkKey, ",", true);
		if (elems.size() != 16)
		{
			Log::Write(LogLevel_Warning, "Invalid Network Key. Does not contain 16 Bytes - Contains %d", elems.size());
			Log::Write(LogLevel_Warning, "Raw Key: %s", networkKey.c_str());
			Log::Write(LogLevel_Warning, "Parsed Key:");
			int i = 0;
			for (std::vector<std::string>::iterator it = elems.begin(); it != elems.end(); it++)
				Log::Write(LogLevel_Warning, "%d) - %s", ++i, (*it).c_str());
			OZW_FATAL_ERROR(OZWException::OZWEXCEPTION_SECURITY_FAILED, "Failed to Read Network Key");
		}
		int i = 0;
		for (std::vector<std::string>::iterator it = elems.begin(); it != elems.end(); it++)
		{
			if (0 == sscanf(Internal::trim(*it).c_str(), "%x", &tempkey[i]))
			{
				Log::Write(LogLevel_Warning, "Cannot Convert Network Key Byte %s to Key", (*it).c_str());
				OZW_FATAL_ERROR(OZWException::OZWEXCEPTION_SECURITY_FAILED, "Failed to Convert Network Key");
			}
			else
			{
				keybytes[i] = (tempkey[i] & 0xFF);
			}
			i++;
		}
		keySet = true;
	}
	return keybytes;
}

//-----------------------------------------------------------------------------
// <Driver::SendEncryptedMessage>
// Send either a NONCE request, or the actual encrypted message, depending what state the Message Currently is in.
//-----------------------------------------------------------------------------
bool Driver::SendEncryptedMessage()
{

	uint8 *buffer = m_currentMsg->GetBuffer();
	uint8 length = m_currentMsg->GetLength();
	m_expectedCallbackId = m_currentMsg->GetCallbackId();
	Log::Write(LogLevel_Info, m_currentMsg->GetTargetNodeId(), "Sending (%s) message (Callback ID=0x%.2x, Expected Reply=0x%.2x) - %s", c_sendQueueNames[m_currentMsgQueueSource], m_expectedCallbackId, m_expectedReply, m_currentMsg->GetAsString().c_str());

	m_controller->Write(buffer, length);
	m_currentMsg->clearNonce();

	return true;
}

bool Driver::SendNonceRequest(string logmsg)
{

	uint8 m_buffer[11];

	/* construct a standard NONCE_GET message */
	m_buffer[0] = SOF;
	m_buffer[1] = 9;					// Length of the entire message
	m_buffer[2] = REQUEST;
	m_buffer[3] = FUNC_ID_ZW_SEND_DATA;
	m_buffer[4] = m_currentMsg->GetTargetNodeId();
	m_buffer[5] = 2; 					// Length of the payload
	m_buffer[6] = Internal::CC::Security::StaticGetCommandClassId();
	m_buffer[7] = Internal::CC::SecurityCmd_NonceGet;
	//m_buffer[8] = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
	m_buffer[8] = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
	/* this is the same as the Actual Message */
	//m_buffer[9] = m_expectedCallbackId;
	m_buffer[9] = 2;
	// Calculate the checksum
	m_buffer[10] = 0xff;
	for (uint32 i = 1; i < 10; ++i)
	{
		m_buffer[10] ^= m_buffer[i];
	}
	Log::Write(LogLevel_Info, m_currentMsg->GetTargetNodeId(), "Sending (%s) message (Callback ID=0x%.2x, Expected Reply=0x%.2x) - Nonce_Get(%s) - %s:", c_sendQueueNames[m_currentMsgQueueSource], 2, m_expectedReply, logmsg.c_str(), Internal::PktToString(m_buffer, 10).c_str());

	m_controller->Write(m_buffer, 11);

	return true;
}

bool Driver::initNetworkKeys(bool newnode)
{

	uint8_t EncryptPassword[16] =
	{ 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	uint8_t AuthPassword[16] =
	{ 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };

	uint8_t SecuritySchemes[1][16] =
	{
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
	this->m_inclusionkeySet = newnode;
	this->AuthKey = new aes_encrypt_ctx;
	this->EncryptKey = new aes_encrypt_ctx;

	Log::Write(LogLevel_Info, GetControllerNodeId(), "Setting Up %s Network Key for Secure Communications", newnode == true ? "Inclusion" : "Provided");

	if (!isNetworkKeySet())
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed - Network Key Not Set");
		return false;
	}

	if (aes_init() == EXIT_FAILURE)
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed to Init AES Engine");
		return false;
	}

	if (aes_encrypt_key128(newnode == false ? this->GetNetworkKey() : SecuritySchemes[0], this->EncryptKey) == EXIT_FAILURE)
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed to Set Initial Network Key for Encryption");
		return false;
	}

	if (aes_encrypt_key128(newnode == false ? this->GetNetworkKey() : SecuritySchemes[0], this->AuthKey) == EXIT_FAILURE)
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed to Set Initial Network Key for Authentication");
		return false;
	}

	uint8 tmpEncKey[32];
	uint8 tmpAuthKey[32];
	aes_mode_reset(this->EncryptKey);
	aes_mode_reset(this->AuthKey);

	if (aes_ecb_encrypt(EncryptPassword, tmpEncKey, 16, this->EncryptKey) == EXIT_FAILURE)
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed to Generate Encrypted Network Key for Encryption");
		return false;
	}
	if (aes_ecb_encrypt(AuthPassword, tmpAuthKey, 16, this->AuthKey) == EXIT_FAILURE)
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed to Generate Encrypted Network Key for Authentication");
		return false;
	}

	aes_mode_reset(this->EncryptKey);
	aes_mode_reset(this->AuthKey);
	if (aes_encrypt_key128(tmpEncKey, this->EncryptKey) == EXIT_FAILURE)
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed to set Encrypted Network Key for Encryption");
		return false;
	}
	if (aes_encrypt_key128(tmpAuthKey, this->AuthKey) == EXIT_FAILURE)
	{
		Log::Write(LogLevel_Warning, GetControllerNodeId(), "Failed to set Encrypted Network Key for Authentication");
		return false;
	}
	aes_mode_reset(this->EncryptKey);
	aes_mode_reset(this->AuthKey);
	return true;
}

void Driver::SendNonceKey(uint8 nodeId, uint8 *nonce)
{

	uint8 m_buffer[19];
	/* construct a standard NONCE_GET message */
	m_buffer[0] = SOF;
	m_buffer[1] = 17;					// Length of the entire message
	m_buffer[2] = REQUEST;
	m_buffer[3] = FUNC_ID_ZW_SEND_DATA;
	m_buffer[4] = nodeId;
	m_buffer[5] = 10; 					// Length of the payload
	m_buffer[6] = Internal::CC::Security::StaticGetCommandClassId();
	m_buffer[7] = Internal::CC::SecurityCmd_NonceReport;
	for (int i = 0; i < 8; ++i)
	{
		m_buffer[8 + i] = nonce[i];
	}
	m_buffer[16] = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
	/* this is the same as the Actual Message */
	m_buffer[17] = 1;
	// Calculate the checksum
	m_buffer[18] = 0xff;
	for (uint32 i = 1; i < 18; ++i)
	{
		m_buffer[18] ^= m_buffer[i];
	}
	Log::Write(LogLevel_Info, nodeId, "Sending (%s) message (Callback ID=0x%.2x, Expected Reply=0x%.2x) - Nonce_Report - %s:", c_sendQueueNames[m_currentMsgQueueSource], m_buffer[17], m_expectedReply, Internal::PktToString(m_buffer, 19).c_str());

	m_controller->Write(m_buffer, 19);

	m_nonceReportSent = nodeId;
}

aes_encrypt_ctx *Driver::GetAuthKey()
{
	if (m_currentControllerCommand != NULL && m_currentControllerCommand->m_controllerCommand == ControllerCommand_AddDevice && m_currentControllerCommand->m_controllerState == ControllerState_Completed)
	{
		/* we are adding a Node, so our AuthKey is different from normal comms */
		initNetworkKeys(true);
	}
	else if (m_inclusionkeySet)
	{
		initNetworkKeys(false);
	}
	return this->AuthKey;
}
;
aes_encrypt_ctx *Driver::GetEncKey()
{
	if (m_currentControllerCommand != NULL && m_currentControllerCommand->m_controllerCommand == ControllerCommand_AddDevice && m_currentControllerCommand->m_controllerState == ControllerState_Completed)
	{
		/* we are adding a Node, so our EncryptKey is different from normal comms */
		initNetworkKeys(true);
	}
	else if (m_inclusionkeySet)
	{
		initNetworkKeys(false);
	}

	return this->EncryptKey;
}
;

bool Driver::isNetworkKeySet()
{
	std::string networkKey;
	if (!Options::Get()->GetOptionAsString("NetworkKey", &networkKey))
	{
		return false;
	}
	else
	{
		return networkKey.length() <= 0 ? false : true;
	}
}

bool Driver::CheckNodeConfigRevision(Node *node)
{
	Internal::DNSLookup *lu = new Internal::DNSLookup;
	lu->NodeID = node->GetNodeId();
	/* make up a string of what we want to look up */
	std::stringstream ss;
	ss << std::hex << std::setw(4) << std::setfill('0') << node->GetProductId() << ".";
	ss << std::hex << std::setw(4) << std::setfill('0') << node->GetProductType() << ".";
	ss << std::hex << std::setw(4) << std::setfill('0') << node->GetManufacturerId() << ".db.openzwave.com";

	lu->lookup = ss.str();
	lu->type = Internal::DNS_Lookup_ConfigRevision;
	return m_dns->sendRequest(lu);
}

bool Driver::CheckMFSConfigRevision()
{
	Internal::DNSLookup *lu = new Internal::DNSLookup;
	lu->NodeID = 0;
	lu->lookup = "mfs.db.openzwave.com";
	lu->type = Internal::DNS_Lookup_ConfigRevision;
	return m_dns->sendRequest(lu);
}

void Driver::processConfigRevision(Internal::DNSLookup *result)
{
	if (result->status == Internal::Platform::DNSError_None)
	{
		if (result->type == Internal::DNS_Lookup_ConfigRevision)
		{
			if (result->NodeID > 0)
			{
				Internal::LockGuard LG(m_nodeMutex);
				Node *node = this->GetNode(result->NodeID);
				if (!node)
				{
					Log::Write(LogLevel_Warning, result->NodeID, "Node disappeared when processing Config Revision");
					return;
				}
				node->setLatestConfigRevision((unsigned long) atol(result->result.c_str()));
				if (node->getFileConfigRevision() < node->getLatestConfigRevision())
				{
					Log::Write(LogLevel_Warning, node->GetNodeId(), "Config File for Device \"%s\" is out of date", node->GetProductName().c_str());
					Notification* notification = new Notification(Notification::Type_UserAlerts);
					notification->SetHomeAndNodeIds(m_homeId, node->GetNodeId());
					notification->SetUserAlertNotification(Notification::Alert_ConfigOutOfDate);
					QueueNotification(notification);

					bool update = false;
					Options::Get()->GetOptionAsBool("AutoUpdateConfigFile", &update);

					if (update)
						m_mfs->updateConfigFile(this, node);

				}
			}
			else if (result->NodeID == 0)
			{
				/* manufacturer_specific */
				m_mfs->setLatestRevision((unsigned long) atol(result->result.c_str()));
				if (m_mfs->getRevision() < (unsigned long) atol(result->result.c_str()))
				{
					Log::Write(LogLevel_Warning, "Config Revision of ManufacturerSpecific Database is out of date");
					Notification* notification = new Notification(Notification::Type_UserAlerts);
					notification->SetUserAlertNotification(Notification::Alert_MFSOutOfDate);
					QueueNotification(notification);

					bool update = false;
					Options::Get()->GetOptionAsBool("AutoUpdateConfigFile", &update);

					if (update)
					{
						m_mfs->updateMFSConfigFile(this);
					}
					else
					{
						m_mfs->checkInitialized();
					}
				}
				else
				{
					/* its upto date - Check to make sure we have all the config files */
					m_mfs->checkConfigFiles(this);
				}
			}
			return;
		}
	}
	else if (result->status == Internal::Platform::DNSError_NotFound)
	{
		Log::Write(LogLevel_Info, "Not Found for Device record %s", result->lookup.c_str());
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_DNSError);
		QueueNotification(notification);
	}
	else if (result->status == Internal::Platform::DNSError_DomainError)
	{
		Log::Write(LogLevel_Warning, "Domain Error Looking up record %s", result->lookup.c_str());
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_DNSError);
		QueueNotification(notification);
	}
	else if (result->status == Internal::Platform::DNSError_InternalError)
	{
		Log::Write(LogLevel_Warning, "Internal DNS Error looking up record %s", result->lookup.c_str());
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_DNSError);
		QueueNotification(notification);
	}
	m_mfs->checkInitialized();
}

bool Driver::setHttpClient(Internal::i_HttpClient *client)
{
	if (m_httpClient)
		delete m_httpClient;
	m_httpClient = client;
	return true;
}

bool Driver::startConfigDownload(uint16 _manufacturerId, uint16 _productType, uint16 _productId, string configfile, uint8 node)
{
	Internal::HttpDownload *download = new Internal::HttpDownload();
	std::stringstream ss;
	ss << std::hex << std::setw(4) << std::setfill('0') << _productId << ".";
	ss << std::hex << std::setw(4) << std::setfill('0') << _productType << ".";
	ss << std::hex << std::setw(4) << std::setfill('0') << _manufacturerId << ".xml";
	download->url = "http://download.db.openzwave.com/" + ss.str();
	download->filename = configfile;
	download->operation = Internal::HttpDownload::Config;
	download->node = node;
	Log::Write(LogLevel_Info, "Queuing download for %s (Node %d)", download->url.c_str(), download->node);

	return m_httpClient->StartDownload(download);
}

bool Driver::startMFSDownload(string configfile)
{
	Internal::HttpDownload *download = new Internal::HttpDownload();
	download->url = "http://download.db.openzwave.com/mfs.xml";
	download->filename = configfile;
	download->operation = Internal::HttpDownload::MFSConfig;
	download->node = 0;
	Log::Write(LogLevel_Info, "Queuing download for %s", download->url.c_str());

	return m_httpClient->StartDownload(download);
}

bool Driver::refreshNodeConfig(uint8 _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	string action;
	Options::Get()->GetOptionAsString("ReloadAfterUpdate", &action);
	if (Internal::ToUpper(action) == "NEVER")
	{
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_NodeReloadRequired);
		QueueNotification(notification);
		return true;
	}
	else if (Internal::ToUpper(action) == "IMMEDIATE")
	{
		Log::Write(LogLevel_Info, _nodeId, "Reloading Node after new Config File loaded");
		/* this will reload the Node, ignoring any cache that exists etc */
		ReloadNode(_nodeId);
		return true;
	}
	else if (Internal::ToUpper(action) == "AWAKE")
	{
		Node *node = GetNode(_nodeId);
		if (!node->IsListeningDevice())
		{
			if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(node->GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
			{
				if (!wakeUp->IsAwake())
				{
					/* Node is Asleep. Queue it for WakeUp */
					Log::Write(LogLevel_Info, _nodeId, "Queuing Sleeping Node Reload after New Config File Loaded");
					MsgQueueItem item;
					item.m_command = MsgQueueCmd_ReloadNode;
					item.m_nodeId = _nodeId;
					wakeUp->QueueMsg(item);
				}
				else
				{
					/* Node is Awake. Reload it */
					Log::Write(LogLevel_Info, _nodeId, "Reloading Awake Node after new Config File loaded");
					ReloadNode(_nodeId);
					return true;
				}
			}
		}
		else
		{
			Log::Write(LogLevel_Info, _nodeId, "Reloading Node after new Config File Loaded");
			ReloadNode(_nodeId);
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Driver::ReloadNode>
// Reload a Node - Remove it from ozwcache, and re-initilize the node from scratch (doing a full  interview)
//-----------------------------------------------------------------------------
void Driver::ReloadNode(uint8 const _nodeId)
{
	Internal::LockGuard LG(m_nodeMutex);
	Log::Write(LogLevel_Detail, _nodeId, "Reloading Node");
	/* delete any cached information about this node so we start from fresh */
	char str[32];
	int32 intVal;

	string userPath;
	Options::Get()->GetOptionAsString("UserPath", &userPath);

	snprintf(str, sizeof(str), "ozwcache_0x%08x.xml", m_homeId);
	string filename = userPath + string(str);

	TiXmlDocument doc;
	if (doc.LoadFile(filename.c_str(), TIXML_ENCODING_UTF8))
	{
		doc.SetUserData((void *) filename.c_str());
		TiXmlElement * driverElement = doc.RootElement();

		TiXmlNode * nodeElement = driverElement->FirstChild();
		while (nodeElement)
		{
			if (nodeElement->ToElement())
			{
				char const* str2 = nodeElement->ToElement()->Value();
				if (str2 && !strcmp(str2, "Node"))
				{
					// Get the node Id from the XML
					if (TIXML_SUCCESS == nodeElement->ToElement()->QueryIntAttribute("id", &intVal))
					{
						if (intVal == _nodeId)
						{
							driverElement->RemoveChild(nodeElement);
							break;
						}
					}
				}
			}
			nodeElement = nodeElement->NextSibling();
		}
	}
	doc.SaveFile(filename.c_str());
	LG.Unlock();

	InitNode(_nodeId);
}

void Driver::processDownload(Internal::HttpDownload *download)
{
	if (download->transferStatus == Internal::HttpDownload::Ok)
	{
		Log::Write(LogLevel_Info, "Download Finished: %s (Node: %d)", download->filename.c_str(), download->node);
		if (download->operation == Internal::HttpDownload::Config)
		{
			m_mfs->configDownloaded(this, download->filename, download->node);
		}
		else if (download->operation == Internal::HttpDownload::MFSConfig)
		{
			m_mfs->mfsConfigDownloaded(this, download->filename);
		}
	}
	else
	{
		Log::Write(LogLevel_Warning, "Download of %s Failed (Node: %d)", download->url.c_str(), download->node);
		if (download->operation == Internal::HttpDownload::Config)
		{
			m_mfs->configDownloaded(this, download->filename, download->node, false);
		}
		else if (download->operation == Internal::HttpDownload::MFSConfig)
		{
			m_mfs->mfsConfigDownloaded(this, download->filename, false);
		}
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
		QueueNotification(notification);
	}

}

bool Driver::downloadConfigRevision(Node *node)
{
	/* only download if the revision is 1 or higher. Revision 0's are for local testing only */
	if (node->getFileConfigRevision() <= 0)
	{
		Log::Write(LogLevel_Warning, node->GetNodeId(), "Config File Revision is 0. Not Updating");
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
		QueueNotification(notification);
		return false;
	}
	if (node->getFileConfigRevision() >= node->getLatestConfigRevision())
	{
		Log::Write(LogLevel_Warning, node->GetNodeId(), "Config File Revision %d is equal to or greater than current revision %d", node->getFileConfigRevision(), node->getLatestConfigRevision());
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
		QueueNotification(notification);
		return false;
	}
	else
	{
		m_mfs->updateConfigFile(this, node);
		return true;
	}
}
bool Driver::downloadMFSRevision()
{
	if (m_mfs->getRevision() <= 0)
	{
		Log::Write(LogLevel_Warning, "ManufacturerSpecific Revision is 0. Not Updating");
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
		QueueNotification(notification);
		return false;
	}
	if (m_mfs->getRevision() >= m_mfs->getLatestRevision())
	{
		Log::Write(LogLevel_Warning, "ManufacturerSpecific Revision %d is equal to or greater than current revision %d", m_mfs->getRevision(), m_mfs->getLatestRevision());
		Notification* notification = new Notification(Notification::Type_UserAlerts);
		notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
		QueueNotification(notification);
		return false;
	}
	m_mfs->updateMFSConfigFile(this);
	return true;
}

void Driver::SubmitEventMsg(EventMsg *event)
{
	Internal::LockGuard LG(m_eventMutex);
	m_eventQueueMsg.push_back(event);
	m_queueMsgEvent->Set();
}

void Driver::ProcessEventMsg()
{
	EventMsg *event;
	{
		Internal::LockGuard LG(m_eventMutex);
		event = m_eventQueueMsg.front();
		m_eventQueueMsg.pop_front();
		if (m_eventQueueMsg.empty())
			m_queueMsgEvent->Reset();
	}
	switch (event->type)
	{
		case EventMsg::Event_DNS:
			processConfigRevision(event->event.lookup);
			delete event->event.lookup;
			break;
		case EventMsg::Event_Http:
			processDownload(event->event.httpdownload);
			delete event->event.httpdownload;
			break;
	}
	delete event;
}

//-----------------------------------------------------------------------------
// <Manager::GetMetaData>
// Retrieve MetaData about a Node.
//-----------------------------------------------------------------------------
string const Driver::GetMetaData(uint8 const _nodeId, Node::MetaDataFields _metadata)
{
	Internal::LockGuard LG(m_nodeMutex);
	Node* node = GetNode(_nodeId);
	if (node != NULL)
	{
		return node->GetMetaData(_metadata);
	}
	return "";
}

//-----------------------------------------------------------------------------
// <Manager::GetMetaData>
// Retrieve MetaData about a Node.
//-----------------------------------------------------------------------------
Node::ChangeLogEntry const Driver::GetChangeLog(uint8 const _nodeId, uint32_t revision)
{
	Internal::LockGuard LG(m_nodeMutex);
	Node* node = GetNode(_nodeId);
	if (node != NULL)
	{
		return node->GetChangeLog(revision);
	}
	Node::ChangeLogEntry cle;
	cle.revision = -1;
	return cle;
}

Internal::ManufacturerSpecificDB *Driver::GetManufacturerSpecificDB()
{
	return this->m_mfs;
}

