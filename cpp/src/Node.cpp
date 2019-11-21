//-----------------------------------------------------------------------------
//
//	Node.cpp
//
//	A node in the Z-Wave network.
//
//	Copyright (c) 2009 Mal Lansell <xpl@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
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

#include <iomanip>

#include "Node.h"
#include "Defs.h"
#include "Group.h"
#include "Options.h"
#include "Manager.h"
#include "Driver.h"
#include "Localization.h"
#include "ManufacturerSpecificDB.h"
#include "Notification.h"
#include "Msg.h"
#include "ZWSecurity.h"
#include "platform/Log.h"
#include "platform/Mutex.h"
#include "Utils.h"

#include "tinyxml.h"

#include "command_classes/CommandClasses.h"
#include "command_classes/CommandClass.h"
#include "command_classes/Association.h"
#include "command_classes/Basic.h"
#include "command_classes/Configuration.h"
#include "command_classes/ControllerReplication.h"
#include "command_classes/ManufacturerSpecific.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/MultiChannelAssociation.h"
#include "command_classes/Security.h"
#include "command_classes/WakeUp.h"
#include "command_classes/NodeNaming.h"
#include "command_classes/NoOperation.h"
#include "command_classes/Version.h"
#include "command_classes/SwitchAll.h"
#include "command_classes/ZWavePlusInfo.h"
#include "command_classes/DeviceResetLocally.h"

#include "Scene.h"

#include "value_classes/ValueID.h"
#include "value_classes/Value.h"
#include "value_classes/ValueBitSet.h"
#include "value_classes/ValueBool.h"
#include "value_classes/ValueButton.h"
#include "value_classes/ValueByte.h"
#include "value_classes/ValueDecimal.h"
#include "value_classes/ValueInt.h"
#include "value_classes/ValueRaw.h"
#include "value_classes/ValueList.h"
#include "value_classes/ValueSchedule.h"
#include "value_classes/ValueShort.h"
#include "value_classes/ValueString.h"
#include "value_classes/ValueStore.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// Statics
//-----------------------------------------------------------------------------
bool Node::s_deviceClassesLoaded = false;
map<uint8, string> Node::s_basicDeviceClasses;
map<uint8, Node::GenericDeviceClass*> Node::s_genericDeviceClasses;
map<uint8, Node::DeviceClass*> Node::s_roleDeviceClasses;
map<uint16, Node::DeviceClass*> Node::s_deviceTypeClasses;
map<uint8, Node::DeviceClass*> Node::s_nodeTypes;

static char const* c_queryStageNames[] =
{ "None", "ProtocolInfo", "Probe", "WakeUp", "ManufacturerSpecific1", "NodeInfo", "NodePlusInfo", "SecurityReport", "ManufacturerSpecific2", "Versions", "Instances", "Static", "CacheLoad", "Associations", "Neighbors", "Session", "Dynamic", "Configuration", "Complete" };

//-----------------------------------------------------------------------------
// <Node::Node>
// Constructor
//-----------------------------------------------------------------------------
Node::Node(uint32 const _homeId, uint8 const _nodeId) :
		m_queryStage(QueryStage_None), m_queryPending(false), m_queryConfiguration(false), m_queryRetries(0), m_protocolInfoReceived(false), m_basicprotocolInfoReceived(false), m_nodeInfoReceived(false), m_nodePlusInfoReceived(false), m_manufacturerSpecificClassReceived(false), m_nodeInfoSupported(true), m_refreshonNodeInfoFrame(true), m_nodeAlive(true),	// assome live node
		m_listening(true),	// assume we start out listening
		m_frequentListening(false), m_beaming(false), m_routing(false), m_maxBaudRate(0), m_version(0), m_security(false), m_homeId(_homeId), m_nodeId(_nodeId), m_basic(0), m_generic(0), m_specific(0), m_type(""), m_addingNode(false), m_manufacturerName(""), m_productName(""), m_nodeName(""), m_location(""), m_manufacturerId(0), m_productType(0), m_productId(0), m_deviceType(0), m_role(0), m_nodeType(0), m_secured(false), m_nodeCache( NULL), m_Product( NULL), m_fileConfigRevision(0), m_loadedConfigRevision(
				0), m_latestConfigRevision(0), m_values(new Internal::VC::ValueStore()), m_sentCnt(0), m_sentFailed(0), m_retries(0), m_receivedCnt(0), m_receivedDups(0), m_receivedUnsolicited(0), m_lastRequestRTT(0), m_lastResponseRTT(0), m_averageRequestRTT(0), m_averageResponseRTT(0), m_quality(0), m_lastReceivedMessage(), m_errors(0), m_txStatusReportSupported(false), m_txTime(0), m_hops(0), m_ackChannel(0), m_lastTxChannel(0), m_routeScheme((TXSTATUS_ROUTING_SCHEME) 0), m_routeUsed
		{ }, m_routeSpeed((TXSTATUS_ROUTE_SPEED) 0), m_routeTries(0), m_lastFailedLinkFrom(0), m_lastFailedLinkTo(0), m_lastnonce(0)
{
	memset(m_neighbors, 0, sizeof(m_neighbors));
	memset(m_nonces, 0, sizeof(m_nonces));
	memset(m_rssi_1, 0, sizeof(m_rssi_1));
	memset(m_rssi_2, 0, sizeof(m_rssi_2));
	memset(m_rssi_3, 0, sizeof(m_rssi_3));
	memset(m_rssi_4, 0, sizeof(m_rssi_4));
	memset(m_rssi_5, 0, sizeof(m_rssi_5));
	/* Add NoOp Class */
	AddCommandClass(Internal::CC::NoOperation::StaticGetCommandClassId());

	/* Add ManufacturerSpecific Class */
	AddCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId());
}

//-----------------------------------------------------------------------------
// <Node::~Node>
// Destructor
//-----------------------------------------------------------------------------
Node::~Node()
{
	// Remove any messages from queues
	GetDriver()->RemoveQueues(m_nodeId);

	// Remove the values from the poll list
	for (Internal::VC::ValueStore::Iterator it = m_values->Begin(); it != m_values->End(); ++it)
	{
		ValueID const& valueId = it->second->GetID();
		if (GetDriver()->isPolled(valueId))
		{
			GetDriver()->DisablePoll(valueId);
		}
	}

	Internal::Scene::RemoveValues(m_homeId, m_nodeId);

	// Delete the values
	delete m_values;

	// Delete the command classes
	while (!m_commandClassMap.empty())
	{
		map<uint8, Internal::CC::CommandClass*>::iterator it = m_commandClassMap.begin();
		delete it->second;
		m_commandClassMap.erase(it);
	}

	// Delete the groups
	while (!m_groups.empty())
	{
		map<uint8, Group*>::iterator it = m_groups.begin();
		delete it->second;
		m_groups.erase(it);
	}

	// Delete the button map
	while (!m_buttonMap.empty())
	{
		map<uint8, uint8>::iterator it = m_buttonMap.begin();
		m_buttonMap.erase(it);
	}
	delete m_nodeCache;
}

//-----------------------------------------------------------------------------
// <Node::AdvanceQueries>
// Proceed through the initialisation process
//-----------------------------------------------------------------------------
void Node::AdvanceQueries()
{
	// For OpenZWave to discover everything about a node, we have to follow a certain
	// order of queries, because the results of one stage may affect what is requested
	// in the next stage.  The stage is saved with the node data, so that any incomplete
	// queries can be restarted the next time the application runs.
	// The individual command classes also store some state as to whether they have
	// had a response to certain queries.  This state is initilized by the SetStaticRequests
	// call in QueryStage_None.  It is also saved, so we do not need to request state
	// from every command class if some have previously responded.
	//
	// Each stage must generate all the messages for its particular	stage as
	// assumptions are made in later code (RemoveMsg) that this is the case. This means
	// each stage is only visited once.

	Log::Write(LogLevel_Detail, m_nodeId, "AdvanceQueries queryPending=%d queryRetries=%d queryStage=%s live=%d", m_queryPending, m_queryRetries, c_queryStageNames[m_queryStage], m_nodeAlive);
	bool addQSC = false;			// We only want to add a query stage complete if we did some work.
	while (!m_queryPending && m_nodeAlive)
	{
		switch (m_queryStage)
		{
			case QueryStage_None:
			{
				// Init the node query process
				m_queryStage = QueryStage_ProtocolInfo;
				m_queryRetries = 0;
				break;
			}
			case QueryStage_ProtocolInfo:
			{
				// determines, among other things, whether this node is a listener, its maximum baud rate and its device classes
				if (!ProtocolInfoReceived())
				{
					Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_ProtocolInfo");
					Internal::Msg* msg = new Internal::Msg("Get Node Protocol Info", m_nodeId, REQUEST, FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO, false);
					msg->Append(m_nodeId);
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Query);
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					// This stage has been done already, so move to the Probe stage
					m_queryStage = QueryStage_Probe;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Probe:
			{
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Probe");
				//
				// Send a NoOperation message to see if the node is awake
				// and alive. Based on the response or lack of response
				// will determine next step.
				//
				Internal::CC::NoOperation* noop = static_cast<Internal::CC::NoOperation*>(GetCommandClass(Internal::CC::NoOperation::StaticGetCommandClassId()));
				/* don't Probe the Controller */
				if (GetDriver()->GetControllerNodeId() != m_nodeId)
				{
					noop->Set(true);
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					m_queryStage = QueryStage_WakeUp;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_WakeUp:
			{
				// For sleeping devices other than controllers, we need to defer the usual requests until
				// we have told the device to send it's wake-up notifications to the PC controller.
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_WakeUp");

				Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId()));

				// if this device is a "sleeping device" and not a controller and not a
				// FLiRS device. FLiRS will wake up when you send them something and they
				// don't seem to support Wakeup
				if (wakeUp && !IsController() && !IsFrequentListeningDevice())
				{
					// start the process of requesting node state from this sleeping device
					wakeUp->Init();
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					// this is not a sleeping device, so move to the ManufacturerSpecific1 stage
					m_queryStage = QueryStage_ManufacturerSpecific1;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_ManufacturerSpecific1:
			{
				// Obtain manufacturer, product type and product ID code from the node device
				// Manufacturer Specific data is requested before the other command class data so
				// that we can modify the supported command classes list through the product XML files.
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_ManufacturerSpecific1");

				/* if its the Controller, then we can just load up the XML straight away */
				if (GetDriver()->GetControllerNodeId() == m_nodeId)
				{
					Log::Write(LogLevel_Detail, m_nodeId, "Load Controller Manufacturer Specific Config");
					Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
					if (cc)
					{
						cc->SetInstance(1);
						cc->SetProductDetails(GetDriver()->GetManufacturerId(), GetDriver()->GetProductType(), GetDriver()->GetProductId());
						cc->LoadConfigXML();
					}
					m_queryStage = QueryStage_NodeInfo;
					m_queryRetries = 0;
				}
				else
				{
					Log::Write(LogLevel_Detail, m_nodeId, "Checking for ManufacturerSpecific CC and Requesting values if present on this node");
					/* if the ManufacturerSpecific CC was not specified in the ProtocolInfo packet for the Generic/Specific Device type (as part a Mandatory Command Class)
					 * then this will fail, but we will retry in ManufacturerSpecific2
					 *
					 * XXX TODO: This could probably be reworked a bit to make this a Mandatory CC for all devices regardless
					 * of Generic/Specific Type. Then we can drop the Second ManufacturerSpecific QueryStage later.
					 */
					Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
					if (cc)
					{
						cc->SetInstance(1);
						m_queryPending = cc->RequestState(Internal::CC::CommandClass::RequestFlag_Static, 1, Driver::MsgQueue_Query);
						addQSC = m_queryPending;
					}
					if (!m_queryPending)
					{
						m_queryStage = QueryStage_NodeInfo;
						m_queryRetries = 0;
					}
				}
				break;
			}
			case QueryStage_NodeInfo:
			{
				if (!NodeInfoReceived() && m_nodeInfoSupported && (GetDriver()->GetControllerNodeId() != m_nodeId))
				{
					// obtain from the node a list of command classes that it 1) supports and 2) controls (separated by a mark in the buffer)
					Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_NodeInfo");
					Internal::Msg* msg = new Internal::Msg("Request Node Info", m_nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_INFO, false, true, FUNC_ID_ZW_APPLICATION_UPDATE);
					msg->Append(m_nodeId);
					GetDriver()->SendMsg(msg, Driver::MsgQueue_Query);
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					// This stage has been done already, so move to the Manufacturer Specific stage
					m_queryStage = QueryStage_NodePlusInfo;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_NodePlusInfo:
			{
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_NodePlusInfo");
				Internal::CC::ZWavePlusInfo* pluscc = static_cast<Internal::CC::ZWavePlusInfo*>(GetCommandClass(Internal::CC::ZWavePlusInfo::StaticGetCommandClassId()));

				if (pluscc)
				{
					m_queryPending = pluscc->RequestState(Internal::CC::CommandClass::RequestFlag_Static, 1, Driver::MsgQueue_Query);
				}
				if (m_queryPending)
				{
					addQSC = m_queryPending;
				}
				else
				{
					// this is not a Zwave+ node, so move onto the next querystage
					m_queryStage = QueryStage_SecurityReport;
					m_queryRetries = 0;
				}

				break;
			}
			case QueryStage_SecurityReport:
			{
				/* For Devices that Support the Security Class, we have to request a list of
				 * Command Classes that Require Security.
				 */
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_SecurityReport");

				Internal::CC::Security* seccc = static_cast<Internal::CC::Security*>(GetCommandClass(Internal::CC::Security::StaticGetCommandClassId()));

				if (seccc)
				{
					// start the process setting up the Security CommandClass
					m_queryPending = seccc->Init();
					/* Dont add a QueryStageComplete flag here, as this is a multipacket exchange.
					 * the Security Command Class will automatically advance the Query Stage
					 * when we recieve a SecurityCmd_SupportedReport
					 */
					addQSC = true;
				}
				else
				{
					// this is not a Security Device, so move onto the next querystage
					m_queryStage = QueryStage_ManufacturerSpecific2;
					m_queryRetries = 0;
				}

				break;
			}
			case QueryStage_ManufacturerSpecific2:
			{
				if (!m_manufacturerSpecificClassReceived)
				{
					// Obtain manufacturer, product type and product ID code from the node device
					// Manufacturer Specific data is requested before the other command class data so
					// that we can modify the supported command classes list through the product XML files.
					Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_ManufacturerSpecific2");
					Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
					/* don't do this if its the Controller Node */
					if (cc && (GetDriver()->GetControllerNodeId() != m_nodeId))
					{
						cc->SetInstance(1);
						m_queryPending = cc->RequestState(Internal::CC::CommandClass::RequestFlag_Static, 1, Driver::MsgQueue_Query);
						addQSC = m_queryPending;
					}
					if (!m_queryPending)
					{
						m_queryStage = QueryStage_Versions;
						m_queryRetries = 0;
					}
				}
				else
				{
					Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
					if (cc)
					{
						cc->SetInstance(1);
						cc->ReLoadConfigXML();
					}
					m_queryStage = QueryStage_Versions;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Versions:
			{
				// Get the version information (if the device supports COMMAND_CLASS_VERSION
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Versions");
				Internal::CC::Version* vcc = static_cast<Internal::CC::Version*>(GetCommandClass(Internal::CC::Version::StaticGetCommandClassId()));
				if (vcc)
				{
					Log::Write(LogLevel_Info, m_nodeId, "Requesting Versions");
					for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
					{
						Internal::CC::CommandClass* cc = it->second;
						Log::Write(LogLevel_Info, m_nodeId, "Requesting Versions for %s", cc->GetCommandClassName().c_str());

						if (cc->GetMaxVersion() > 1)
						{
							// Get the version for each supported command class that
							// we have implemented at greater than version one.
							m_queryPending |= vcc->RequestCommandClassVersion(it->second);
						}
						else
						{
							// set the Version to 1 
							cc->SetVersion(1);
						}
					}
					addQSC = m_queryPending;
				}
				// advance to Instances stage when finished
				if (!m_queryPending)
				{
					m_queryStage = QueryStage_Instances;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Instances:
			{
				// if the device at this node supports multiple instances, obtain a list of these instances
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Instances");
				Internal::CC::MultiInstance* micc = static_cast<Internal::CC::MultiInstance*>(GetCommandClass(Internal::CC::MultiInstance::StaticGetCommandClassId()));
				if (micc)
				{
					m_queryPending = micc->RequestInstances();
					addQSC = m_queryPending;
				}

				// when done, advance to the Static stage
				if (!m_queryPending)
				{
					m_queryStage = QueryStage_Static;
					m_queryRetries = 0;

					Log::Write(LogLevel_Info, m_nodeId, "Essential node queries are complete");
					Notification* notification = new Notification(Notification::Type_EssentialNodeQueriesComplete);
					notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
					GetDriver()->QueueNotification(notification);
				}
				break;
			}
			case QueryStage_Static:
			{
				// Request any other static values associated with each command class supported by this node
				// examples are supported thermostat operating modes, setpoints and fan modes
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Static");
				/* Dont' do this for Controller Nodes */
				if (GetDriver()->GetControllerNodeId() != m_nodeId)
				{
					for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
					{
						if (!it->second->IsAfterMark())
						{
							m_queryPending |= it->second->RequestStateForAllInstances(Internal::CC::CommandClass::RequestFlag_Static, Driver::MsgQueue_Query);
						}
						else
						{
							/* Controlling CC's might still need to retrieve some info */
							m_queryPending |= it->second->RequestStateForAllInstances(Internal::CC::CommandClass::RequestFlag_AfterMark, Driver::MsgQueue_Query);
						}
					}
				}
				addQSC = m_queryPending;

				if (!m_queryPending)
				{
					// when all (if any) static information has been retrieved, advance to the Associations stage
					// CacheLoad stage is for Nodes that are read in via the zw state file, as is skipped as we would
					// have already queried it at the discovery phase in Probe.
					m_queryStage = QueryStage_Associations;
					m_queryRetries = 0;
				}
				break;
			}
				/* CacheLoad is where we start if we are loading a device from our zwcfg_*.xml file rather than
				 * a brand new device.
				 */
			case QueryStage_CacheLoad:
			{
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_CacheLoad");
				Log::Write(LogLevel_Info, GetNodeId(), "Loading Cache for node %d: Manufacturer=%s, Product=%s", GetNodeId(), GetManufacturerName().c_str(), GetProductName().c_str());
				Log::Write(LogLevel_Info, GetNodeId(), "Node Identity Codes: %.4x:%.4x:%.4x", GetManufacturerId(), GetProductType(), GetProductId());
				//
				// Send a NoOperation message to see if the node is awake
				// and alive. Based on the response or lack of response
				// will determine next step. Called here when configuration exists.
				//
				Internal::CC::NoOperation* noop = static_cast<Internal::CC::NoOperation*>(GetCommandClass(Internal::CC::NoOperation::StaticGetCommandClassId()));
				/* Don't do this if its to the Controller */
				if (GetDriver()->GetControllerNodeId() != m_nodeId)
				{
					noop->Set(true);
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					m_queryStage = QueryStage_Associations;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Associations:
			{
				// if this device supports COMMAND_CLASS_ASSOCIATION, determine to which groups this node belong
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Associations");
				Internal::CC::MultiChannelAssociation* macc = static_cast<Internal::CC::MultiChannelAssociation*>(GetCommandClass(Internal::CC::MultiChannelAssociation::StaticGetCommandClassId()));
				if (macc)
				{
					macc->RequestAllGroups(0);
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					Internal::CC::Association* acc = static_cast<Internal::CC::Association*>(GetCommandClass(Internal::CC::Association::StaticGetCommandClassId()));
					if (acc)
					{
						acc->RequestAllGroups(0);
						m_queryPending = true;
						addQSC = true;
					}
					else
					{
						// if this device doesn't support Associations, move to retrieve Session information
						m_queryStage = QueryStage_Neighbors;
						m_queryRetries = 0;
					}
				}
				break;
			}
			case QueryStage_Neighbors:
			{
				// retrieves this node's neighbors and stores the neighbor bitmap in the node object
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Neighbors");
				GetDriver()->RequestNodeNeighbors(m_nodeId, 0);
				m_queryPending = true;
				addQSC = true;
				break;
			}
			case QueryStage_Session:
			{
				// Request the session values from the command classes in turn
				// examples of Session information are: current thermostat setpoints, node names and climate control schedules
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Session");
				for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
				{
					if (!it->second->IsAfterMark())
					{
						m_queryPending |= it->second->RequestStateForAllInstances(Internal::CC::CommandClass::RequestFlag_Session, Driver::MsgQueue_Query);
					}
				}
				addQSC = m_queryPending;
				if (!m_queryPending)
				{
					m_queryStage = QueryStage_Dynamic;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Dynamic:
			{
				// Request the dynamic values from the node, that can change at any time
				// Examples include on/off state, heating mode, temperature, etc.
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Dynamic");
				m_queryPending = RequestDynamicValues();
				addQSC = m_queryPending;

				if (!m_queryPending)
				{
					m_queryStage = QueryStage_Configuration;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Configuration:
			{
				// Request the configurable parameter values from the node.
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Configuration");
				if (m_queryConfiguration)
				{
					if (RequestAllConfigParams(0))
					{
						m_queryPending = true;
						addQSC = true;
					}
					m_queryConfiguration = false;
				}
				if (!m_queryPending)
				{
					m_queryStage = QueryStage_Complete;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Complete:
			{
				ClearAddingNode();
				// Notify the watchers that the queries are complete for this node
				Log::Write(LogLevel_Detail, m_nodeId, "QueryStage_Complete");
				Notification* notification = new Notification(Notification::Type_NodeQueriesComplete);
				notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
				GetDriver()->QueueNotification(notification);

				/* if its a sleeping node, this will send a NoMoreInformation Packet to the device */
				Internal::CC::WakeUp* cc = static_cast<Internal::CC::WakeUp*>(GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId()));
				if (cc)
				{
					cc->SendPending();
				}
				// Check whether all nodes are now complete
				GetDriver()->CheckCompletedNodeQueries();
				return;
			}
			default:
			{
				break;
			}
		}
	}

	if (addQSC && m_nodeAlive)
	{
		// Add a marker to the query queue so this advance method
		// gets called again once this stage has completed.
		GetDriver()->SendQueryStageComplete(m_nodeId, m_queryStage);
	}
}

//-----------------------------------------------------------------------------
// <Node::QueryStageComplete>
// We are done with a stage in the query process
//-----------------------------------------------------------------------------
void Node::QueryStageComplete(QueryStage const _stage)
{
	// Check that we are actually on the specified stage
	if (_stage != m_queryStage)
	{
		return;
	}

	if (m_queryStage != QueryStage_Complete)
	{
		// Move to the next stage
		m_queryPending = false;
		m_queryStage = (QueryStage) ((uint32) m_queryStage + 1);
		if (m_queryStage == QueryStage_CacheLoad)
		{
			m_queryStage = (QueryStage) ((uint32) m_queryStage + 1);
		}
		m_queryRetries = 0;
	}
}

//-----------------------------------------------------------------------------
// <Node::QueryStageRetry>
// Retry a stage up to the specified maximum
//-----------------------------------------------------------------------------
void Node::QueryStageRetry(QueryStage const _stage, uint8 const _maxAttempts // = 0
		)
{
	Log::Write(LogLevel_Info, m_nodeId, "QueryStageRetry stage %s requested stage %s max %d retries %d pending %d", c_queryStageNames[_stage], c_queryStageNames[m_queryStage], _maxAttempts, m_queryRetries, m_queryPending);

	// Check that we are actually on the specified stage
	if (_stage != m_queryStage)
	{
		return;
	}

	m_queryPending = false;
	if (_maxAttempts && (++m_queryRetries >= _maxAttempts))
	{
		m_queryRetries = 0;
		// We've retried too many times. Move to the next stage but only if
		// we aren't in any of the probe stages.
		if (m_queryStage != QueryStage_Probe && m_queryStage != QueryStage_CacheLoad)
		{
			m_queryStage = (Node::QueryStage) ((uint32) (m_queryStage + 1));
		}
	}
	// Repeat the current query stage
	GetDriver()->RetryQueryStageComplete(m_nodeId, m_queryStage);
}

//-----------------------------------------------------------------------------
// <Node::SetQueryStage>
// Set the query stage (but only to an earlier stage)
//-----------------------------------------------------------------------------
void Node::SetQueryStage(QueryStage const _stage, bool const _advance	// = true
		)
{
	if ((int) _stage < (int) m_queryStage)
	{
		m_queryStage = _stage;
		m_queryPending = false;

		if (QueryStage_Configuration == _stage)
		{
			m_queryConfiguration = true;
		}
	}
	if (_advance)
	{
		AdvanceQueries();
	}
}

//-----------------------------------------------------------------------------
// <Node::GetQueryStageName>
// Gets the query stage name
//-----------------------------------------------------------------------------
string Node::GetQueryStageName(QueryStage const _stage)
{
	return c_queryStageNames[_stage];
}

//-----------------------------------------------------------------------------
// <Node::GetNeighbors>
// Gets the neighbors of a node
//-----------------------------------------------------------------------------
uint32 Node::GetNeighbors(uint8** o_neighbors)
{
	// determine how many neighbors there are
	int i;
	uint32 numNeighbors = 0;
	if (m_queryStage < QueryStage_Session)
	{
		*o_neighbors = NULL;
		return 0;
	}
	for (i = 0; i < 29; i++)
	{
		for (unsigned char mask = 0x80; mask != 0; mask >>= 1)
			if ((m_neighbors[i] & mask) != 0)
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
			if ((m_neighbors[by] & (0x01 << bi)))
				neighbors[index++] = ((by << 3) + bi + 1);
		}
	}

	*o_neighbors = neighbors;
	return numNeighbors;
}

//-----------------------------------------------------------------------------
// <Node::ReadXML>
// Read the node config from the OZW Cache File...
//-----------------------------------------------------------------------------
void Node::ReadXML(TiXmlElement const* _node)
{
	char const* str;
	int intVal;

	str = _node->Attribute("query_stage");
	if (str)
	{
		// After restoring state from a file, we need to at least refresh the session and dynamic values.
		QueryStage queryStage = QueryStage_Session;
		for (uint32 i = 0; i < (uint32) QueryStage_Session; ++i)
		{
			if (!strcmp(str, c_queryStageNames[i]))
			{
				queryStage = (QueryStage) i;
				break;
			}
		}

		/* we cant use the SetQueryStage method here, as it only allows us to
		 * go to a lower QueryStage, and not a higher QueryStage. As QueryStage_Complete is higher than
		 * QueryStage_None (the default) we manually set it here. Note - in Driver::HandleSerialAPIGetInitDataResponse the
		 * QueryStage is set to CacheLoad (which is less than QueryStage_Associations) if this is a existing node read in via the zw state file.
		 *
		 */
		m_queryStage = queryStage;
		m_queryPending = false;

		if (QueryStage_Configuration == queryStage)
		{
			m_queryConfiguration = true;
		}
	}

	if (m_queryStage != QueryStage_None)
	{
		if (m_queryStage > QueryStage_ProtocolInfo)
		{
			// Notify the watchers of the protocol info.
			// We do the notification here so that it gets into the queue ahead of
			// any other notifications generated by adding command classes etc.
			m_protocolInfoReceived = true;
			Notification* notification = new Notification(Notification::Type_NodeProtocolInfo);
			notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
			GetDriver()->QueueNotification(notification);
		}

		if (m_queryStage > QueryStage_NodeInfo)
		{
			m_nodeInfoReceived = true;
		}

		if (m_queryStage > QueryStage_Instances)
		{
			Notification* notification = new Notification(Notification::Type_EssentialNodeQueriesComplete);
			notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
			GetDriver()->QueueNotification(notification);
		}
		if (m_queryStage > QueryStage_CacheLoad)
		{
			m_nodeCache = _node->Clone();
		}
	}

	str = _node->Attribute("name");
	if (str)
	{
		m_nodeName = str;
	}

	str = _node->Attribute("location");
	if (str)
	{
		m_location = str;
	}

	if (TIXML_SUCCESS == _node->QueryIntAttribute("basic", &intVal))
	{
		m_basic = (uint8) intVal;
	}

	if (TIXML_SUCCESS == _node->QueryIntAttribute("generic", &intVal))
	{
		m_generic = (uint8) intVal;
	}

	if (TIXML_SUCCESS == _node->QueryIntAttribute("specific", &intVal))
	{
		m_specific = (uint8) intVal;
	}

	if (TIXML_SUCCESS == _node->QueryIntAttribute("roletype", &intVal))
	{
		m_role = (uint8) intVal;
		m_nodePlusInfoReceived = true;
	}

	if (TIXML_SUCCESS == _node->QueryIntAttribute("devicetype", &intVal))
	{
		m_deviceType = (uint16) intVal;
		m_nodePlusInfoReceived = true;
	}

	if (TIXML_SUCCESS == _node->QueryIntAttribute("nodetype", &intVal))
	{
		m_nodeType = (uint8) intVal;
		m_nodePlusInfoReceived = true;
	}

	str = _node->Attribute("type");
	if (str)
	{
		m_type = str;
	}

	m_listening = true;
	str = _node->Attribute("listening");
	if (str)
	{
		m_listening = !strcmp(str, "true");
	}

	m_frequentListening = false;
	str = _node->Attribute("frequentListening");
	if (str)
	{
		m_frequentListening = !strcmp(str, "true");
	}

	m_beaming = false;
	str = _node->Attribute("beaming");
	if (str)
	{
		m_beaming = !strcmp(str, "true");
	}

	m_routing = true;
	str = _node->Attribute("routing");
	if (str)
	{
		m_routing = !strcmp(str, "true");
	}

	m_maxBaudRate = 0;
	if (TIXML_SUCCESS == _node->QueryIntAttribute("max_baud_rate", &intVal))
	{
		m_maxBaudRate = (uint32) intVal;
	}

	m_version = 0;
	if (TIXML_SUCCESS == _node->QueryIntAttribute("version", &intVal))
	{
		m_version = (uint8) intVal;
	}

	m_security = false;
	str = _node->Attribute("security");
	if (str)
	{
		m_security = !strcmp(str, "true");
	}

	m_secured = false;
	str = _node->Attribute("secured");
	if (str)
	{
		m_secured = !strcmp(str, "true");
	}

	m_nodeInfoSupported = true;
	str = _node->Attribute("nodeinfosupported");
	if (str)
	{
		m_nodeInfoSupported = !strcmp(str, "true");
	}

	m_refreshonNodeInfoFrame = true;
	str = _node->Attribute("refreshonnodeinfoframe");
	if (str)
		m_refreshonNodeInfoFrame = !strcmp(str, "true");

	/* this is the revision of the config file that was present when we created the cache */
	str = _node->Attribute("configrevision");
	if (str)
		this->setLoadedConfigRevision(atol(str));
	else
		this->setLoadedConfigRevision(0);

	// Read the manufacturer info and create the command classes
	TiXmlElement const* child = _node->FirstChildElement();
	while (child)
	{
		str = child->Value();
		if (str)
		{
			if (!strcmp(str, "Neighbors"))
			{
				TiXmlNode const *NeighborList = child->FirstChild();
				char const *neighbors = NeighborList->Value();
				int i = 0;
				char* pos = const_cast<char*>(neighbors);
				while (*pos && i < 29)
				{
					m_neighbors[i] = strtol(pos, &pos, 10);
					if ((*pos) == ',')
					{
						++pos;
						++i;
					}
				}
			}
			else if (!strcmp(str, "CommandClasses"))
			{
				ReadCommandClassesXML(child);
			}
			else if (!strcmp(str, "Manufacturer"))
			{
				uint16 manufacturerId = 0;
				uint16 productType = 0;
				uint16 productId = 0;
				str = child->Attribute("id");
				if (str)
				{
					manufacturerId = (uint16_t) (strtol(str, NULL, 16) & 0xFFFF);
				}

				str = child->Attribute("name");
				if (str)
				{
					m_manufacturerName = str;
				}

				TiXmlElement const* product = child->FirstChildElement();
				if (!strcmp(product->Value(), "Product"))
				{
					str = product->Attribute("type");
					if (str)
					{
						productType = (uint16_t) (strtol(str, NULL, 16) & 0xFFFF);
					}

					str = product->Attribute("id");
					if (str)
					{
						productId = (uint16_t) (strtol(str, NULL, 16) & 0xFFFF);
					}

					str = product->Attribute("name");
					if (str)
					{
						m_productName = str;
					}

					Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
					/* don't do this if its the Controller Node */
					if (cc)
					{
						cc->SetProductDetails(manufacturerId, productType, productId);
						cc->setLoadedConfigRevision(m_loadedConfigRevision);
					}
					else
					{
						Log::Write(LogLevel_Warning, GetNodeId(), "ManufacturerSpecific Class not loaded for ReadXML");
					}

					ReadMetaDataFromXML(product);
				}
			}
		}

		// Move to the next child node
		child = child->NextSiblingElement();
	}

	if (m_nodeName.length() > 0 || m_location.length() > 0 || m_manufacturerId > 0)
	{
		// Notify the watchers of the name changes
		Notification* notification = new Notification(Notification::Type_NodeNaming);
		notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
		GetDriver()->QueueNotification(notification);
	}
}

//-----------------------------------------------------------------------------
// <Node::ReadDeviceProtocolXML>
// Read the device's protocol configuration from a device config file (Not 
// cache)
//-----------------------------------------------------------------------------
void Node::ReadDeviceProtocolXML(TiXmlElement const* _ccsElement)
{
	char const *str = _ccsElement->Attribute("Revision");
	if (str)
	{
		/* as we are loading the Node from the device config file, both the
		 * File and Loaded Revisions will be the same
		 */
		this->setFileConfigRevision(atol(str));
		this->setLoadedConfigRevision(m_fileConfigRevision);
		Log::Write(LogLevel_Info, GetNodeId(), "  Configuration File Revision is %d", m_fileConfigRevision);
	}
	else
	{
		this->setFileConfigRevision(0);
		this->setLoadedConfigRevision(m_fileConfigRevision);
	}

	TiXmlElement const* ccElement = _ccsElement->FirstChildElement();
	while (ccElement)
	{
		str = ccElement->Value();
		if (str && !strcmp(str, "Protocol"))
		{
			str = ccElement->Attribute("nodeinfosupported");
			if (str)
			{
				m_nodeInfoSupported = !strcmp(str, "true");
			}

			str = ccElement->Attribute("refreshonnodeinfoframe");
			if (str)
			{
				m_refreshonNodeInfoFrame = !strcmp(str, "true");
			}

			// Some controllers support API calls that aren't advertised in their returned data.
			// So provide a way to manipulate the returned data to reflect reality.
			TiXmlElement const* childElement = _ccsElement->FirstChildElement();
			while (childElement)
			{
				str = childElement->Value();
				if (str && !strcmp(str, "APIcall"))
				{
					char const* funcStr = childElement->Attribute("function");
					char *p;
					uint8 func = (uint8) strtol(funcStr, &p, 16);
					if (p != funcStr)
					{
						char const* presStr = ccElement->Attribute("present");
						GetDriver()->SetAPICall(func, !strcmp(presStr, "true"));
					}
				}
				childElement = childElement->NextSiblingElement();
			}
			return;
		}
		ccElement = ccElement->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::ReadCommandClassesXML>
// Read the command classes from XML
//-----------------------------------------------------------------------------
void Node::ReadCommandClassesXML(TiXmlElement const* _ccsElement)
{
	char const* str;
	int32 intVal;

	TiXmlElement const* ccElement = _ccsElement->FirstChildElement();
	while (ccElement)
	{
		str = ccElement->Value();
		if (str && !strcmp(str, "CommandClass"))
		{
			if (TIXML_SUCCESS == ccElement->QueryIntAttribute("id", &intVal))
			{
				uint8 id = (uint8) intVal;

				// Check whether this command class is to be removed (product XMLs might
				// request this if a class is not implemented properly by the device)
				bool remove = false;
				char const* action = ccElement->Attribute("action");
				if (action && !strcasecmp(action, "remove"))
				{
					remove = true;
				}

				Internal::CC::CommandClass* cc = GetCommandClass(id);
				if (remove)
				{
					// Remove support for the command class
					RemoveCommandClass(id);
				}
				else
				{
					if ( NULL == cc)
					{
						if (Internal::CC::Security::StaticGetCommandClassId() == id && !GetDriver()->isNetworkKeySet())
						{
							Log::Write(LogLevel_Warning, "Security Command Class cannot be Loaded. NetworkKey is not set");
							ccElement = ccElement->NextSiblingElement();
							continue;
						}

						// Command class support does not exist yet, so we create it
						cc = AddCommandClass(id);
					}

					if ( NULL != cc)
					{
						cc->ReadXML(ccElement);
					}
				}
			}
		}

		ccElement = ccElement->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::WriteXML>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void Node::WriteXML(TiXmlElement* _driverElement)
{
	if (m_queryStage <= QueryStage_CacheLoad)
	{
		/* Just return our cached copy of the "Cache" as nothing new should be here */
		_driverElement->LinkEndChild(m_nodeCache->Clone());
		return;
	}

	char str[32];

	TiXmlElement* nodeElement = new TiXmlElement("Node");
	_driverElement->LinkEndChild(nodeElement);

	snprintf(str, 32, "%d", m_nodeId);
	nodeElement->SetAttribute("id", str);

	nodeElement->SetAttribute("name", m_nodeName.c_str());
	nodeElement->SetAttribute("location", m_location.c_str());

	snprintf(str, 32, "%d", m_basic);
	nodeElement->SetAttribute("basic", str);

	snprintf(str, 32, "%d", m_generic);
	nodeElement->SetAttribute("generic", str);

	snprintf(str, 32, "%d", m_specific);
	nodeElement->SetAttribute("specific", str);

	if (m_nodePlusInfoReceived)
	{
		snprintf(str, 32, "%d", m_role);
		nodeElement->SetAttribute("roletype", str);

		snprintf(str, 32, "%d", m_deviceType);
		nodeElement->SetAttribute("devicetype", str);

		snprintf(str, 32, "%d", m_nodeType);
		nodeElement->SetAttribute("nodetype", str);
	}

	nodeElement->SetAttribute("type", m_type.c_str());

	nodeElement->SetAttribute("listening", m_listening ? "true" : "false");
	nodeElement->SetAttribute("frequentListening", m_frequentListening ? "true" : "false");
	nodeElement->SetAttribute("beaming", m_beaming ? "true" : "false");
	nodeElement->SetAttribute("routing", m_routing ? "true" : "false");

	snprintf(str, 32, "%d", m_maxBaudRate);
	nodeElement->SetAttribute("max_baud_rate", str);

	snprintf(str, 32, "%d", m_version);
	nodeElement->SetAttribute("version", str);

	if (m_security)
	{
		nodeElement->SetAttribute("security", "true");
	}

	if (m_secured)
	{
		nodeElement->SetAttribute("secured", "true");
	}

	if (!m_nodeInfoSupported)
	{
		nodeElement->SetAttribute("nodeinfosupported", "false");
	}

	if (!m_refreshonNodeInfoFrame)
	{
		nodeElement->SetAttribute("refreshonnodeinfoframe", "false");
	}

	snprintf(str, 32, "%d", m_loadedConfigRevision);
	nodeElement->SetAttribute("configrevision", str);

	nodeElement->SetAttribute("query_stage", c_queryStageNames[m_queryStage]);

	TiXmlElement* neighborElement = new TiXmlElement("Neighbors");
	nodeElement->LinkEndChild(neighborElement);
	{
		std::string NeighborList;
		for (int i = 0; i < 29; i++)
		{
			if ((i > 0) && (i != 29))
				NeighborList.append(",");
			NeighborList.append(Internal::intToString(m_neighbors[i]));
		}
		TiXmlText* textElement = new TiXmlText(NeighborList.c_str());
		neighborElement->LinkEndChild(textElement);
	}

	// Write the manufacturer and product data in the same format
	// as used in the ManyfacturerSpecfic.xml file.  This will
	// allow new devices to be added via a simple cut and paste.
	TiXmlElement* manufacturerElement = new TiXmlElement("Manufacturer");
	nodeElement->LinkEndChild(manufacturerElement);

	/* this should be written in hex to avoid confusion... */
	{
		std::stringstream ss;
		ss << std::hex << m_manufacturerId;
		manufacturerElement->SetAttribute("id", ss.str().c_str());
	}
	manufacturerElement->SetAttribute("name", m_manufacturerName.c_str());

	TiXmlElement* productElement = new TiXmlElement("Product");
	manufacturerElement->LinkEndChild(productElement);

	/* this should be written in hex to avoid confusion... */
	{
		std::stringstream ss;
		ss << std::hex << m_productType;
		productElement->SetAttribute("type", ss.str().c_str());
	}

	/* this should be written in hex to avoid confusion... */
	{
		std::stringstream ss;
		ss << std::hex << m_productId;
		productElement->SetAttribute("id", ss.str().c_str());
	}
	productElement->SetAttribute("name", m_productName.c_str());

	// Write the MetaData out
	TiXmlElement* mdElement = new TiXmlElement("MetaData");
	productElement->LinkEndChild(mdElement);
	WriteMetaDataXML(mdElement);

	// Write the command classes
	TiXmlElement* ccsElement = new TiXmlElement("CommandClasses");
	nodeElement->LinkEndChild(ccsElement);

	for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
	{
		if (it->second->GetCommandClassId() == Internal::CC::NoOperation::StaticGetCommandClassId()) // don't output NoOperation
		{
			continue;
		}
		TiXmlElement* ccElement = new TiXmlElement("CommandClass");
		ccsElement->LinkEndChild(ccElement);
		it->second->WriteXML(ccElement);
	}
}

//-----------------------------------------------------------------------------
// <Node::UpdateProtocolInfo>
// Handle the FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO response
//-----------------------------------------------------------------------------
void Node::UpdateProtocolInfo(uint8 const* _data)
{
	if (ProtocolInfoReceived())
	{
		// We already have this info
		return;
	}

	if (_data[4] == 0)
	{
		// Node doesn't exist if Generic class is zero.
		Log::Write(LogLevel_Info, m_nodeId, "  Protocol Info for Node %d reports node nonexistent", m_nodeId);
		SetNodeAlive(false);
		return;
	}

	// Capabilities
	m_listening = ((_data[0] & 0x80) != 0);
	m_routing = ((_data[0] & 0x40) != 0);

	m_maxBaudRate = 9600;
	if ((_data[0] & 0x38) == 0x10)
	{
		m_maxBaudRate = 40000;
	}

	// Reverse engineered by looking at Zniffer capture of crafted NIF packets
	// Note 2019-05-19 I have never seen a 200k device, but Zniffer decodes it,
	// this is not a typo...

	int speed_extension = _data[2] & 0x07;

	switch (speed_extension)
	{
		case 0:
			// No speed_extension
			break;
		case 1:
			m_maxBaudRate = 100000;
			break;
		case 2:
			m_maxBaudRate = 200000;
			break;
		default:
			Log::Write(LogLevel_Warning, m_nodeId, "  Protocol Info speed_extension = %d is 'Reserved', reported Max Baud Rate might be wrong.", speed_extension);
			break;
	}

	m_version = (_data[0] & 0x07) + 1;

	m_frequentListening = ((_data[1] & (SecurityFlag_Sensor250ms | SecurityFlag_Sensor1000ms)) != 0);
	m_beaming = ((_data[1] & SecurityFlag_BeamCapability) != 0);

	// Security
	m_security = ((_data[1] & SecurityFlag_Security) != 0);

	// Optional flag is true if the device reports optional command classes.
	// NOTE: We stopped using this because not all devices report it properly,
	// and now just request the optional classes regardless.
	// bool optional = (( _data[1] & 0x80 ) != 0 );
	/* dont do any further processing if we have already received our Protocol Info, or basicprotocolInfo */
	if (ProtocolInfoReceived())
	{
		// We already have this info
		return;
	}

	Log::Write(LogLevel_Info, m_nodeId, "  Protocol Info for Node %d:", m_nodeId);
	if (m_listening)
		Log::Write(LogLevel_Info, m_nodeId, "    Listening     = true");
	else
	{
		Log::Write(LogLevel_Info, m_nodeId, "    Listening     = false");
		Log::Write(LogLevel_Info, m_nodeId, "    Frequent      = %s", m_frequentListening ? "true" : "false");
	}
	Log::Write(LogLevel_Info, m_nodeId, "    Beaming       = %s", m_beaming ? "true" : "false");
	Log::Write(LogLevel_Info, m_nodeId, "    Routing       = %s", m_routing ? "true" : "false");
	Log::Write(LogLevel_Info, m_nodeId, "    Max Baud Rate = %d", m_maxBaudRate);
	Log::Write(LogLevel_Info, m_nodeId, "    Version       = %d", m_version);
	Log::Write(LogLevel_Info, m_nodeId, "    Security      = %s", m_security ? "true" : "false");

	if (m_basicprotocolInfoReceived == false)
	{

		// Notify the watchers of the protocol info.
		// We do the notification here so that it gets into the queue ahead of
		// any other notifications generated by adding command classes etc.
		Notification* notification = new Notification(Notification::Type_NodeProtocolInfo);
		notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
		GetDriver()->QueueNotification(notification);

		// Set up the device class based data for the node, including mandatory command classes
		SetDeviceClasses(_data[3], _data[4], _data[5]);
		// Do this for every controller. A little extra work but it won't be a large file.
		if (IsController())
		{
			GetDriver()->ReadButtons(m_nodeId);
		}
#if 0
		/* come back to this. We need to find a better way to Route Messages
		 * from Nodes to CC's that are advertised by the ControllerNode
		 */
		/* load the Advertised CommandClasses on the Controller Node
		 *
		 */
		if( GetDriver()->GetControllerNodeId() == m_nodeId )
		{
			Log::Write( LogLevel_Info, m_nodeId, "  Advertised CommandClasses on Controller Node:");
			list<uint8> advertisedCommandClasses = CommandClasses::GetAdvertisedCommandClasses();
			for (list<uint8>::iterator it = advertisedCommandClasses.begin(); it != advertisedCommandClasses.end(); ++it)
			{
				CommandClass *cc = AddCommandClass(*it, true);
				if ( cc )
				{
					Log::Write(LogLevel_Info, m_nodeId, "    %s", cc->GetCommandClassName().c_str());
				}
			}
		}
#endif
		m_basicprotocolInfoReceived = true;
	}
	else
	{
		/* we have to setup the Wakeup CC if needed here, because
		 * it wouldn't have been created in the SetProtocolInfo function, as we didn't
		 * have the Device Flags then
		 */
		if (!m_listening && !IsFrequentListeningDevice())
		{
			// Device does not always listen, so we need the WakeUp handler.  We can't
			// wait for the command class list because the request for the command
			// classes may need to go in the wakeup queue itself!
			if (Internal::CC::CommandClass* pCommandClass = AddCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId()))
			{
				pCommandClass->SetInstance(1);
			}
		}

	}
	m_protocolInfoReceived = true;
}

void Node::SetProtocolInfo(uint8 const* _protocolInfo, uint8 const _length)
{

	if (ProtocolInfoReceived() || m_basicprotocolInfoReceived == true)
	{
		// We already have this info
		return;
	}

	if (_protocolInfo[1] == 0)
	{
		// Node doesn't exist if Generic class is zero.
		Log::Write(LogLevel_Info, m_nodeId, "  Protocol Info for Node %d reports node nonexistent", m_nodeId);
		SetNodeAlive(false);
		return;
	}

	// Notify the watchers of the protocol info.
	// We do the notification here so that it gets into the queue ahead of
	// any other notifications generated by adding command classes etc.
	Notification* notification = new Notification(Notification::Type_NodeProtocolInfo);
	notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
	GetDriver()->QueueNotification(notification);

	// Set up the device class based data for the node, including mandatory command classes
	SetDeviceClasses(_protocolInfo[0], _protocolInfo[1], _protocolInfo[2]);

	/* Remaining Bytes in _protocolInfo are the CommandClasses this device supports */
	/* first iterate over them and check for the Security CC, as we want to quickly start exchanging the Network Keys
	 * first (before other CC's start sending stuff and slowing down our exchange
	 */
	if (m_secured)
	{
		if (Internal::CC::Security *pCommandClass = static_cast<Internal::CC::Security *>(GetCommandClass(Internal::CC::Security::StaticGetCommandClassId())))
		{
			/* Security CC has already been loaded, most likely via the SetDeviceClasses Function above */
			if (!GetDriver()->isNetworkKeySet())
			{
				Log::Write(LogLevel_Warning, m_nodeId, "Security Command Class Disabled. NetworkKey is not Set");
			}
			else
			{
				pCommandClass->ExchangeNetworkKeys();
			}
		}
		else
		{
			/* Security CC is not loaded, see if its in our NIF frame and load if necessary */
			for (int i = 3; i < _length; i++)
			{
				if (_protocolInfo[i] == Internal::CC::Security::StaticGetCommandClassId())
				{
					pCommandClass = static_cast<Internal::CC::Security *>(AddCommandClass(_protocolInfo[i]));
					if (!GetDriver()->isNetworkKeySet())
					{
						Log::Write(LogLevel_Warning, m_nodeId, "Security Command Class Disabled. NetworkKey is not Set");
					}
					else
					{
						pCommandClass->ExchangeNetworkKeys();
					}
				}
			}
		}
	}
	UpdateNodeInfo(&_protocolInfo[3], _length - 3);

	m_basicprotocolInfoReceived = true;
}

void Node::SetSecured(bool secure)
{
	m_secured = secure;
}

bool Node::IsSecured()
{
	return m_secured;
}

void Node::SetInstanceLabel(uint8 const _instance, char *label)
{
	m_globalInstanceLabel[_instance] = string(label);
}

string Node::GetInstanceLabel(uint8 const _ccid, uint8 const _instance)
{
	string label;
	/* find the CommandClass */
	Internal::CC::CommandClass *_cc = GetCommandClass(_ccid);
	if (_cc)
		label = _cc->GetInstanceLabel(_instance);
	/* if the Label is Empty - Then use the Global Label */
	if (label.empty())
	{
		if (m_globalInstanceLabel.count(_instance))
			label = m_globalInstanceLabel[_instance];
		else
		{
			/* construct a Default Label */
			std::ostringstream sstream;
			sstream << Internal::Localization::Get()->GetGlobalLabel("Instance") << " " << (int) _instance << ":";
			label = sstream.str();
		}
	}
	return label;
}

uint8 Node::GetNumInstances(uint8 const _ccid)
{
	uint8 ccid = _ccid;
	int instances = 1;
	if (_ccid == 0)
	{
		ccid = Internal::CC::MultiInstance::StaticGetCommandClassId();
	}
	if (Internal::CC::CommandClass *cc = GetCommandClass(ccid))
	{
		return cc->GetNumInstances();
	}

	return instances;
}

void Node::SetSecuredClasses(uint8 const* _data, uint8 const _length, uint32 const _instance)
{
	uint32 i;
	m_secured = true;
	Log::Write(LogLevel_Info, m_nodeId, "  Secured CommandClasses for node %d (instance %d):", m_nodeId, _instance);
	Log::Write(LogLevel_Info, m_nodeId, "  Controlled CommandClasses:");
	if (!GetDriver()->isNetworkKeySet())
	{
		Log::Write(LogLevel_Warning, m_nodeId, "  Secured CommandClasses cannot be enabled as Network Key is not set");
		return;
	}

	bool afterMark = false;
	for (i = 0; i < _length; ++i)
	{
		if (_data[i] == 0xef)
		{
			// COMMAND_CLASS_MARK.
			// Marks the end of the list of supported command classes.  The remaining classes
			// are those that can be controlled by the device.  These classes are created
			// without values.  Messages received cause notification events instead.
			afterMark = true;
			Log::Write(LogLevel_Info, m_nodeId, "  Controlling CommandClasses:");
			continue;
		}
		/* Check if this is a CC that is already registered with the node */
		if (Internal::CC::CommandClass *pCommandClass = GetCommandClass(_data[i]))
		{
			/* if it was specified the he NIF frame, and came in as part of the Security SupportedReport message
			 * then it can support both Clear Text and Secured Comms. So do a check first
			 */
			if (pCommandClass->IsInNIF())
			{
				/* if the CC Supports Security and our SecurityStrategy says we should encrypt it, then mark it as encrypted */
				if (pCommandClass->IsSecureSupported() && (Internal::ShouldSecureCommandClass(_data[i]) == Internal::SecurityStrategy_Supported))
				{
					pCommandClass->SetSecured();
					Log::Write(LogLevel_Info, m_nodeId, "    %s (Secured) - %s", pCommandClass->GetCommandClassName().c_str(), pCommandClass->IsInNIF() ? "InNIF" : "NotInNIF");
				}
				/* if it wasn't in the NIF frame, then it will only support Secured Comms. */
			}
			else
			{
				if (pCommandClass->IsSecureSupported())
				{
					pCommandClass->SetSecured();
					Log::Write(LogLevel_Info, m_nodeId, "    %s (Secured) - %s", pCommandClass->GetCommandClassName().c_str(), pCommandClass->IsInNIF() ? "InNIF" : "NotInNIF");
				}
			}
			if (_instance > 1)
			{
				/* we need to get the endpoint from the Security CC, to map over to the target CC if this
				 * is triggered by a SecurityCmd_SupportedReport from a instance
				 */
				Internal::CC::CommandClass *secc = GetCommandClass(Internal::CC::Security::StaticGetCommandClassId());
				int ep = secc->GetEndPoint(_instance);
				pCommandClass->SetEndPoint(_instance, ep);
				pCommandClass->SetInstance(_instance);
			}
		}
		/* it might be a new CC we havn't seen as part of the NIF. In that case
		 * its only supported via the Security CC, so no need to check our SecurityStrategy, just
		 * encrypt it regardless */
		else if (Internal::CC::CommandClasses::IsSupported(_data[i]))
		{
			if (Internal::CC::CommandClass* pCommandClass = AddCommandClass(_data[i]))
			{
				// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
				if (afterMark)
				{
					pCommandClass->SetAfterMark();
				}
				if (pCommandClass->IsSecureSupported())
				{
					pCommandClass->SetSecured();
					Log::Write(LogLevel_Info, m_nodeId, "    %s (Secured) - %s", pCommandClass->GetCommandClassName().c_str(), pCommandClass->IsInNIF() ? "InNIF" : "NotInNIF");
				}
				// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
				// then some command class instance counts will increase once the responses to the RequestState
				// call at the end of this method have been processed.
				if (_instance > 1)
					pCommandClass->SetInstance(_instance);
				else
					pCommandClass->SetInstance(1);

				/* set our Static Request Flags */
				uint8 request = 0;

				if (GetCommandClass(Internal::CC::MultiInstance::StaticGetCommandClassId()))
				{
					// Request instances
					request |= (uint8) Internal::CC::CommandClass::StaticRequest_Instances;
				}

				if (GetCommandClass(Internal::CC::Version::StaticGetCommandClassId()))
				{
					// Request versions
					request |= (uint8) Internal::CC::CommandClass::StaticRequest_Version;
				}

				if (request)
				{
					pCommandClass->SetStaticRequest(request);
				}
			}
		}
		else
		{
			Log::Write(LogLevel_Info, m_nodeId, "    Secure CommandClass 0x%.2x - NOT SUPPORTED", _data[i]);
		}
	}
	Log::Write(LogLevel_Info, m_nodeId, "  UnSecured command classes for node %d (instance %d):", m_nodeId, _instance);
	for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
	{
		if (!it->second->IsSecured())
			Log::Write(LogLevel_Info, m_nodeId, "    %s (Unsecured) - %s", it->second->GetCommandClassName().c_str(), it->second->IsInNIF() ? "InNIF" : "NotInNIF");
	}

}
//-----------------------------------------------------------------------------
// <Node::UpdateNodeInfo>
// Set up the command classes from the node info frame
//-----------------------------------------------------------------------------
void Node::UpdateNodeInfo(uint8 const* _data, uint8 const _length)
{
	if (!NodeInfoReceived())
	{
		// Add the command classes specified by the device
		Log::Write(LogLevel_Info, m_nodeId, "  Optional CommandClasses for node %d:", m_nodeId);

		bool newCommandClasses = false;
		uint32 i;

		bool afterMark = false;
		for (i = 0; i < _length; ++i)
		{
			if (_data[i] == 0xef)
			{
				// COMMAND_CLASS_MARK.
				// Marks the end of the list of supported command classes.  The remaining classes
				// are those that can be controlled by the device.  These classes are created
				// without values.  Messages received cause notification events instead.
				afterMark = true;

				if (!newCommandClasses)
				{
					Log::Write(LogLevel_Info, m_nodeId, "    None");
				}
				Log::Write(LogLevel_Info, m_nodeId, "  Optional CommandClasses controlled by node %d:", m_nodeId);
				newCommandClasses = false;
				continue;
			}

			if (Internal::CC::CommandClasses::IsSupported(_data[i]))
			{
				if (Internal::CC::Security::StaticGetCommandClassId() == _data[i] && !GetDriver()->isNetworkKeySet())
				{
					Log::Write(LogLevel_Info, m_nodeId, "    %s (Disabled - Network Key Not Set)", Internal::CC::Security::StaticGetCommandClassName().c_str());
					continue;
				}
				if (Internal::CC::CommandClass* pCommandClass = AddCommandClass(_data[i]))
				{
					/* this CC was in the NIF frame */
					pCommandClass->SetInNIF();
					// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
					if (afterMark)
					{
						pCommandClass->SetAfterMark();
					}

					// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
					// then some command class instance counts will increase once the responses to the RequestState
					// call at the end of this method have been processed.
					pCommandClass->SetInstance(1);
					newCommandClasses = true;
					Log::Write(LogLevel_Info, m_nodeId, "    %s", pCommandClass->GetCommandClassName().c_str());
				}
				else if (Internal::CC::CommandClass *pCommandClass = GetCommandClass(_data[i]))
				{
					/* this CC was in the NIF frame */
					pCommandClass->SetInNIF();
					Log::Write(LogLevel_Info, m_nodeId, "    %s (Existing)", pCommandClass->GetCommandClassName().c_str());
				}
			}
			else
			{
				Log::Write(LogLevel_Info, m_nodeId, "  CommandClass 0x%.2x - NOT REQUIRED", _data[i]);
			}
		}

		if (!newCommandClasses)
		{
			// No additional command classes over the mandatory ones.
			Log::Write(LogLevel_Info, m_nodeId, "    None");
		}

		SetStaticRequests();
		m_nodeInfoReceived = true;
	}
	else
	{
		/* Only Refresh if the Device Config Specifies it  - Only the dynamic stuff */
		if (m_refreshonNodeInfoFrame)
			SetQueryStage(QueryStage_Dynamic);
	}

	// Treat the node info frame as a sign that the node is awake
	if (Internal::CC::WakeUp* wakeUp = static_cast<Internal::CC::WakeUp*>(GetCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId())))
	{
		wakeUp->SetAwake(true);
	}

}

//-----------------------------------------------------------------------------
// <Node::SetNodeAlive>
// Track alive state of a node for dead node detection.
//-----------------------------------------------------------------------------
void Node::SetNodeAlive(bool const _isAlive)
{
	Notification* notification;

	if (_isAlive)
	{
		Log::Write(LogLevel_Error, m_nodeId, "WARNING: node revived");
		m_nodeAlive = true;
		m_errors = 0;
		if (m_queryStage != Node::QueryStage_Complete)
		{
			m_queryRetries = 0; // restart at last stage
			AdvanceQueries();
		}
		notification = new Notification(Notification::Type_Notification);
		notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
		notification->SetNotification(Notification::Code_Alive);
	}
	else
	{
		Log::Write(LogLevel_Error, m_nodeId, "ERROR: node presumed dead");
		m_nodeAlive = false;
		if (m_queryStage != Node::QueryStage_Complete)
		{
			// Check whether all nodes are now complete
			GetDriver()->CheckCompletedNodeQueries();
		}
		notification = new Notification(Notification::Type_Notification);
		notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
		notification->SetNotification(Notification::Code_Dead);
	}
	GetDriver()->QueueNotification(notification);
}

//-----------------------------------------------------------------------------
// <Node::SetStaticRequests>
// The first time we hear from a node, we set flags to indicate the
// need to request certain static data from the device.  This is so that
// we can track which data has been received, and which has not.
//-----------------------------------------------------------------------------
void Node::SetStaticRequests()
{
	uint8 request = 0;

	if (GetCommandClass(Internal::CC::MultiInstance::StaticGetCommandClassId()))
	{
		// Request instances
		request |= (uint8) Internal::CC::CommandClass::StaticRequest_Instances;
	}

	if (GetCommandClass(Internal::CC::Version::StaticGetCommandClassId()))
	{
		// Request versions
		request |= (uint8) Internal::CC::CommandClass::StaticRequest_Version;
	}

	if (request)
	{
		for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
		{
			it->second->SetStaticRequest(request);
		}
		SetQueryStage(QueryStage_ManufacturerSpecific2);
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeName>
// Set the name of the node
//-----------------------------------------------------------------------------
void Node::SetNodeName(string const& _nodeName)
{
	m_nodeName = _nodeName;
	// Notify the watchers of the name changes
	Notification* notification = new Notification(Notification::Type_NodeNaming);
	notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
	GetDriver()->QueueNotification(notification);
	if (Internal::CC::NodeNaming* cc = static_cast<Internal::CC::NodeNaming*>(GetCommandClass(Internal::CC::NodeNaming::StaticGetCommandClassId())))
	{
		// The node supports naming, so we try to write the name into the device
		cc->SetName(_nodeName);
	}
}

//-----------------------------------------------------------------------------
// <Node::SetLocation>
// Set the location of the node
//-----------------------------------------------------------------------------
void Node::SetLocation(string const& _location)
{
	m_location = _location;
	// Notify the watchers of the name changes
	Notification* notification = new Notification(Notification::Type_NodeNaming);
	notification->SetHomeAndNodeIds(m_homeId, m_nodeId);
	GetDriver()->QueueNotification(notification);
	if (Internal::CC::NodeNaming* cc = static_cast<Internal::CC::NodeNaming*>(GetCommandClass(Internal::CC::NodeNaming::StaticGetCommandClassId())))
	{
		// The node supports naming, so we try to write the location into the device
		cc->SetLocation(_location);
	}
}

//-----------------------------------------------------------------------------
// <Node::ApplicationCommandHandler>
// Handle a command class message
//-----------------------------------------------------------------------------
void Node::ApplicationCommandHandler(uint8 const* _data, bool encrypted

)
{
	if (Internal::CC::CommandClass* pCommandClass = GetCommandClass(_data[5]))
	{
		if (pCommandClass->IsSecured() && !encrypted)
		{
			Log::Write(LogLevel_Warning, m_nodeId, "Received a Clear Text Message for the CommandClass %s which is Secured", pCommandClass->GetCommandClassName().c_str());
			bool drop = true;
			Options::Get()->GetOptionAsBool("EnforceSecureReception", &drop);
			if (drop)
			{
				Log::Write(LogLevel_Warning, m_nodeId, "   Dropping Message");
				return;
			}
			else
			{
				Log::Write(LogLevel_Warning, m_nodeId, "   Allowing Message (EnforceSecureReception is not set)");
			}
		}

		pCommandClass->ReceivedCntIncr();
		if (!pCommandClass->IsAfterMark())
		{
			if (!pCommandClass->HandleMsg(&_data[6], _data[4]))
			{
				Log::Write(LogLevel_Warning, m_nodeId, "CommandClass %s HandlerMsg Returned False", pCommandClass->GetCommandClassName().c_str());
			}
		}
		else
		{
			if (!pCommandClass->HandleIncomingMsg(&_data[6], _data[4]))
			{
				Log::Write(LogLevel_Warning, m_nodeId, "CommandClass %s HandleIncomingMsg Returned False", pCommandClass->GetCommandClassName().c_str());
			}
		}
	}
	else
	{
		if (_data[5] == Internal::CC::ControllerReplication::StaticGetCommandClassId())
		{
			// This is a controller replication message, and we do not support it.
			// We have to at least acknowledge the message to avoid locking the sending device.
			Log::Write(LogLevel_Info, m_nodeId, "ApplicationCommandHandler - Default acknowledgment of controller replication data");

			Internal::Msg* msg = new Internal::Msg("Replication Command Complete", m_nodeId, REQUEST, FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE, false);
			GetDriver()->SendMsg(msg, Driver::MsgQueue_Command);
		}
		else if (_data[5] == Internal::CC::MultiInstance::StaticGetCommandClassId())
		{
			// Devices that support MultiChannelAssociation may send a MultiChannel Encapsulated message if there is a Instance set in the Association Groups
			// So we will dynamically load the MultiChannel CC if we receive a encapsulated message
			// We only do this after the QueryStage is Complete as we don't want to Add this CC to the list, and then confusing OZW that
			// The device supports the MultiChannel CC (eg, the Device might be asleep, we have not got the NIF and it actually does support it versus sending to a MultiChannelAssociation Endpoint
			if (GetCurrentQueryStage() != QueryStage_Complete)
			{
				Log::Write(LogLevel_Info, m_nodeId, "ApplicationCommandHandler - Received a MultiInstance Message, but QueryStage Isn't Complete yet");
				return;
			}

			Log::Write(LogLevel_Info, m_nodeId, "ApplicationCommandHandler - Received a MultiInstance Message but MulitInstance CC isn't loaded. Loading it... ");
			if (Internal::CC::CommandClass* pCommandClass = AddCommandClass(Internal::CC::MultiInstance::StaticGetCommandClassId()))
			{
				pCommandClass->ReceivedCntIncr();
				if (!pCommandClass->IsAfterMark())
				{
					if (!pCommandClass->HandleMsg(&_data[6], _data[4]))
					{
						Log::Write(LogLevel_Warning, m_nodeId, "CommandClass %s HandleMsg returned false", pCommandClass->GetCommandClassName().c_str());
					}
				}
				else
				{
					if (!pCommandClass->HandleIncomingMsg(&_data[6], _data[4]))
					{
						Log::Write(LogLevel_Warning, m_nodeId, "CommandClass %s HandleIncommingMsg returned false", pCommandClass->GetCommandClassName().c_str());
					}
				}
			}
		}
		else
		{
			Log::Write(LogLevel_Info, m_nodeId, "ApplicationCommandHandler - Unhandled Command Class 0x%.2x", _data[5]);
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::GetCommandClass>
// Get the specified command class object if supported, otherwise NULL
//-----------------------------------------------------------------------------
Internal::CC::CommandClass* Node::GetCommandClass(uint8 const _commandClassId) const
{
	map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.find(_commandClassId);
	if (it != m_commandClassMap.end())
	{
		return it->second;
	}

	// Not found
	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::AddCommandClass>
// Add a command class to the node
//-----------------------------------------------------------------------------
Internal::CC::CommandClass* Node::AddCommandClass(uint8 const _commandClassId)
{
	if (GetCommandClass(_commandClassId))
	{
		// Class and instance have already been added
		return NULL;
	}

	// Create the command class object and add it to our map
	if (Internal::CC::CommandClass* pCommandClass = Internal::CC::CommandClasses::CreateCommandClass(_commandClassId, m_homeId, m_nodeId))
	{
		m_commandClassMap[_commandClassId] = pCommandClass;
		return pCommandClass;
	}
	else
	{
		Log::Write(LogLevel_Info, m_nodeId, "AddCommandClass - Unsupported CommandClass 0x%.2x", _commandClassId);
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::RemoveCommandClass>
// Remove a command class from the node
//-----------------------------------------------------------------------------
void Node::RemoveCommandClass(uint8 const _commandClassId)
{
	map<uint8, Internal::CC::CommandClass*>::iterator it = m_commandClassMap.find(_commandClassId);
	if (it == m_commandClassMap.end())
	{
		// Class is not found
		return;
	}

	// Remove all the values associated with this class
	if (Internal::VC::ValueStore* store = GetValueStore())
	{
		store->RemoveCommandClassValues(_commandClassId);
	}

	// Destroy the command class object and remove it from our map
	Log::Write(LogLevel_Info, m_nodeId, "RemoveCommandClass - Removed support for %s", it->second->GetCommandClassName().c_str());

	delete it->second;
	m_commandClassMap.erase(it);
}

//-----------------------------------------------------------------------------
// <Node::SetConfigParam>
// Set a configuration parameter in a device
//-----------------------------------------------------------------------------
bool Node::SetConfigParam(uint8 const _param, int32 _value, uint8 const _size)
{
	if (Internal::CC::Configuration* cc = static_cast<Internal::CC::Configuration*>(GetCommandClass(Internal::CC::Configuration::StaticGetCommandClassId())))
	{
		// First try to find an existing value representing the parameter, and set that.
		if (Internal::VC::Value* value = cc->GetValue(1, _param))
		{
			switch (value->GetID().GetType())
			{
				case ValueID::ValueType_Bool:
				{
					Internal::VC::ValueBool* valueBool = static_cast<Internal::VC::ValueBool*>(value);
					valueBool->Set(_value != 0);
					break;
				}
				case ValueID::ValueType_Byte:
				{
					Internal::VC::ValueByte* valueByte = static_cast<Internal::VC::ValueByte*>(value);
					valueByte->Set((uint8) _value);
					break;
				}
				case ValueID::ValueType_Short:
				{
					Internal::VC::ValueShort* valueShort = static_cast<Internal::VC::ValueShort*>(value);
					valueShort->Set((uint16) _value);
					break;
				}
				case ValueID::ValueType_Int:
				{
					Internal::VC::ValueInt* valueInt = static_cast<Internal::VC::ValueInt*>(value);
					valueInt->Set(_value);
					break;
				}
				case ValueID::ValueType_List:
				{
					Internal::VC::ValueList* valueList = static_cast<Internal::VC::ValueList*>(value);
					valueList->SetByValue(_value);
					break;
				}
				default:
				{
				}
			}

			return true;
		}

		// Failed to find an existing value object representing this
		// configuration parameter, so we try using the default or
		// included size through the Configuration command class.
		cc->Set(_param, _value, _size);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Node::RequestConfigParam>
// Request the value of a configuration parameter from the device
//-----------------------------------------------------------------------------
void Node::RequestConfigParam(uint8 const _param)
{
	if (Internal::CC::Configuration* cc = static_cast<Internal::CC::Configuration*>(GetCommandClass(Internal::CC::Configuration::StaticGetCommandClassId())))
	{
		cc->RequestValue(0, _param, 1, Driver::MsgQueue_Send);
	}
}

//-----------------------------------------------------------------------------
// <Node::RequestAllConfigParams>
// Request the values of all known configuration parameters from the device
//-----------------------------------------------------------------------------
bool Node::RequestAllConfigParams(uint32 const _requestFlags)
{
	bool res = false;
	if (Internal::CC::Configuration* cc = static_cast<Internal::CC::Configuration*>(GetCommandClass(Internal::CC::Configuration::StaticGetCommandClassId())))
	{
		// Go through all the values in the value store, and request all those which are in the Configuration command class
		for (Internal::VC::ValueStore::Iterator it = m_values->Begin(); it != m_values->End(); ++it)
		{
			Internal::VC::Value* value = it->second;
			if (value->GetID().GetCommandClassId() == Internal::CC::Configuration::StaticGetCommandClassId() && !value->IsWriteOnly())
			{
				/* put the ConfigParams Request into the MsgQueue_Query queue. This is so MsgQueue_Send doesn't get backlogged with a
				 * lot of ConfigParams requests, and should help speed up any user generated messages being sent out (as the MsgQueue_Send has a higher
				 * priority than MsgQueue_Query
				 */
				res |= cc->RequestValue(_requestFlags, value->GetID().GetIndex(), 1, Driver::MsgQueue_Query);
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Node::RequestDynamicValues>
// Request an update of all known dynamic values from the device
//-----------------------------------------------------------------------------
bool Node::RequestDynamicValues()
{
	bool res = false;
	for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
	{
		if (!it->second->IsAfterMark())
		{
			res |= it->second->RequestStateForAllInstances(Internal::CC::CommandClass::RequestFlag_Dynamic, Driver::MsgQueue_Send);
		}
	}

	return res;
}
//-----------------------------------------------------------------------------
// <Node::RefreshValuesOnWakeup>
// Request an update of all known dynamic values from the device
//-----------------------------------------------------------------------------
void Node::RefreshValuesOnWakeup()
{
	for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
	{
		if (!it->second->IsAfterMark())
		{
			it->second->refreshValuesOnWakeup();
		}
	}

}
//-----------------------------------------------------------------------------
// <Node::SetLevel>
// Helper method to set a device's basic level
//-----------------------------------------------------------------------------
void Node::SetLevel(uint8 const _level)
{
	// Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
	uint8 adjustedLevel = _level;
	if ((_level > 99) && (_level < 255))
	{
		adjustedLevel = 99;
	}

	if (Internal::CC::Basic* cc = static_cast<Internal::CC::Basic*>(GetCommandClass(Internal::CC::Basic::StaticGetCommandClassId())))
	{
		cc->Set(adjustedLevel);
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeOn>
// Helper method to set a device to be on
//-----------------------------------------------------------------------------
void Node::SetNodeOn()
{
	// Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
	if (Internal::CC::Basic* cc = static_cast<Internal::CC::Basic*>(GetCommandClass(Internal::CC::Basic::StaticGetCommandClassId())))
	{
		cc->Set(255);
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeOff>
// Helper method to set a device to be off
//-----------------------------------------------------------------------------
void Node::SetNodeOff()
{
	// Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
	if (Internal::CC::Basic* cc = static_cast<Internal::CC::Basic*>(GetCommandClass(Internal::CC::Basic::StaticGetCommandClassId())))
	{
		cc->Set(0);
	}
}

//-----------------------------------------------------------------------------
// <Node::CreateValueID>
// Helper to create a ValueID
//-----------------------------------------------------------------------------
ValueID Node::CreateValueID(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, ValueID::ValueType const _type)
{
	return ValueID(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _type);
}

//-----------------------------------------------------------------------------
// <Node::CreateValueBitSet>
// Helper to create a BitSet ValueID
//-----------------------------------------------------------------------------
bool Node::CreateValueBitSet(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, int32 const _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueBitSet* value = new Internal::VC::ValueBitSet(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueBool>
// Helper to create a new bool value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueBool(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, bool const _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueBool* value = new Internal::VC::ValueBool(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueButton>
// Helper to create a new trigger value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueButton(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, uint8 const _pollIntensity)
{
	Internal::VC::ValueButton* value = new Internal::VC::ValueButton(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueByte>
// Helper to create a new byte value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueByte(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint8 const _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueByte* value = new Internal::VC::ValueByte(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueDecimal>
// Helper to create a new decimal value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueDecimal(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, string const& _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueDecimal* value = new Internal::VC::ValueDecimal(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueInt>
// Helper to create a new int value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueInt(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, int32 const _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueInt* value = new Internal::VC::ValueInt(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueList>
// Helper to create a new list value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueList(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint8 const _size, vector<Internal::VC::ValueList::Item> const& _items, int32 const _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueList* value = new Internal::VC::ValueList(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _items, _default, _pollIntensity, _size);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}
	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueRaw>
// Helper to create a new raw value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueRaw(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint8 const* _default, uint8 const _length, uint8 const _pollIntensity)
{
	Internal::VC::ValueRaw* value = new Internal::VC::ValueRaw(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _length, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueSchedule>
// Helper to create a new schedule value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueSchedule(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint8 const _pollIntensity)
{
	Internal::VC::ValueSchedule* value = new Internal::VC::ValueSchedule(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueShort>
// Helper to create a new short value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueShort(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, int16 const _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueShort* value = new Internal::VC::ValueShort(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueString>
// Helper to create a new string value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueString(ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, string const& _default, uint8 const _pollIntensity)
{
	Internal::VC::ValueString* value = new Internal::VC::ValueString(m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity);
	Internal::VC::ValueStore* store = GetValueStore();
	if (store->AddValue(value))
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::RemoveValueList>
// Helper to remove an existing list value from the value store
//-----------------------------------------------------------------------------
void Node::RemoveValueList(Internal::VC::ValueList* _value)
{
	Internal::VC::ValueStore* store = GetValueStore();
	store->RemoveValue(_value->GetID().GetValueStoreKey());
}

//-----------------------------------------------------------------------------
// <Node::CreateValueFromXML>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
bool Node::CreateValueFromXML(uint8 const _commandClassId, TiXmlElement const* _valueElement)
{
	Internal::VC::Value* value = NULL;

	// Create the value
	ValueID::ValueType type = Internal::VC::Value::GetTypeEnumFromName(_valueElement->Attribute("type"));

	switch (type)
	{
		case ValueID::ValueType_BitSet:
		{
			value = new Internal::VC::ValueBitSet();
			break;
		}
		case ValueID::ValueType_Bool:
		{
			value = new Internal::VC::ValueBool();
			break;
		}
		case ValueID::ValueType_Byte:
		{
			value = new Internal::VC::ValueByte();
			break;
		}
		case ValueID::ValueType_Decimal:
		{
			value = new Internal::VC::ValueDecimal();
			break;
		}
		case ValueID::ValueType_Int:
		{
			value = new Internal::VC::ValueInt();
			break;
		}
		case ValueID::ValueType_List:
		{
			value = new Internal::VC::ValueList();
			break;
		}
		case ValueID::ValueType_Schedule:
		{
			value = new Internal::VC::ValueSchedule();
			break;
		}
		case ValueID::ValueType_Short:
		{
			value = new Internal::VC::ValueShort();
			break;
		}
		case ValueID::ValueType_String:
		{
			value = new Internal::VC::ValueString();
			break;
		}
		case ValueID::ValueType_Button:
		{
			value = new Internal::VC::ValueButton();
			break;
		}
		case ValueID::ValueType_Raw:
		{
			value = new Internal::VC::ValueRaw();
			break;
		}
	}

	if (value)
	{
		value->ReadXML(m_homeId, m_nodeId, _commandClassId, _valueElement);
		Internal::VC::ValueStore* store = GetValueStore();
		if (store->AddValue(value))
		{
			value->Release();
			return true;
		}

		value->Release();
	}
	else
	{
		Log::Write(LogLevel_Info, m_nodeId, "Unknown ValueType in XML: %s", _valueElement->Attribute("type"));
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Node::ReadValueFromXML>
// Apply XML differences to a value
//-----------------------------------------------------------------------------
void Node::ReadValueFromXML(uint8 const _commandClassId, TiXmlElement const* _valueElement)
{
	int32 intVal;

	ValueID::ValueGenre genre = Internal::VC::Value::GetGenreEnumFromName(_valueElement->Attribute("genre"));
	ValueID::ValueType type = Internal::VC::Value::GetTypeEnumFromName(_valueElement->Attribute("type"));

	uint8 instance = 0;
	if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("instance", &intVal))
	{
		instance = (uint8) intVal;
	}

	uint16 index = 0;
	if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("index", &intVal))
	{
		index = (uint16) intVal;
	}

	ValueID id = ValueID(m_homeId, m_nodeId, genre, _commandClassId, instance, index, type);

	// Try to get the value from the ValueStore (everything except configuration parameters
	// should already have been created when the command class instance count was read in).
	// Create it if it doesn't already exist.
	if (Internal::VC::ValueStore* store = GetValueStore())
	{
		if (Internal::VC::Value* value = store->GetValue(id.GetValueStoreKey()))
		{
			// Check if values type are the same
			ValueID::ValueType v_type = value->GetID().GetType();
			if (v_type == type)
			{
				value->ReadXML(m_homeId, m_nodeId, _commandClassId, _valueElement);
				value->Release();
			}
			else
			{
				Log::Write(LogLevel_Info, m_nodeId, "xml value type (%s) is different to stored value type (%s). Value is recreate with xml params.", value->GetTypeNameFromEnum(type), value->GetTypeNameFromEnum(v_type));
				store->RemoveValue(value->GetID().GetValueStoreKey());
				CreateValueFromXML(_commandClassId, _valueElement);
			}
		}
		else
		{
			CreateValueFromXML(_commandClassId, _valueElement);
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::GetValue>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
Internal::VC::Value* Node::GetValue(ValueID const& _id)
{
	// This increments the value's reference count
	return GetValueStore()->GetValue(_id.GetValueStoreKey());
}

//-----------------------------------------------------------------------------
// <Node::GetValue>
// Get the value object with the specified settings
//-----------------------------------------------------------------------------
Internal::VC::Value* Node::GetValue(uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex)
{
	Internal::VC::Value* value = NULL;
	Internal::VC::ValueStore* store = GetValueStore();
	// This increments the value's reference count
	value = store->GetValue(ValueID::GetValueStoreKey(_commandClassId, _instance, _valueIndex));
	return value;
}

//-----------------------------------------------------------------------------
// <Node::RemoveValue>
// Remove the value object with the specified settings
//-----------------------------------------------------------------------------
bool Node::RemoveValue(uint8 const _commandClassId, uint8 const _instance, uint16 const _valueIndex)
{
	Internal::VC::ValueStore* store = GetValueStore();
	return store->RemoveValue(ValueID::GetValueStoreKey(_commandClassId, _instance, _valueIndex));
}

//-----------------------------------------------------------------------------
// <Node::GetGroup>
// Get a Group from the node's map
//-----------------------------------------------------------------------------
Group* Node::GetGroup(uint8 const _groupIdx)
{
	map<uint8, Group*>::iterator it = m_groups.find(_groupIdx);
	if (it == m_groups.end())
	{
		return NULL;
	}

	return it->second;
}

//-----------------------------------------------------------------------------
// <Node::AddGroup>
// Add a group into the node's map
//-----------------------------------------------------------------------------
void Node::AddGroup(Group* _group)
{
	map<uint8, Group*>::iterator it = m_groups.find(_group->GetIdx());
	if (it != m_groups.end())
	{
		// There is already a group with this id.  We will replace it.
		delete it->second;
		m_groups.erase(it);
	}

	m_groups[_group->GetIdx()] = _group;
}

//-----------------------------------------------------------------------------
// <Node::WriteGroups>
// Save the group data
//-----------------------------------------------------------------------------
void Node::WriteGroups(TiXmlElement* _associationsElement)
{
	for (map<uint8, Group*>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
	{
		Group* group = it->second;

		TiXmlElement* groupElement = new TiXmlElement("Group");
		_associationsElement->LinkEndChild(groupElement);
		group->WriteXML(groupElement);
	}
}

//-----------------------------------------------------------------------------
// <Node::GetNumGroups>
// Gets the number of association groups reported by this node
//-----------------------------------------------------------------------------
uint8 Node::GetNumGroups()
{
	return (uint8) m_groups.size();
}

//-----------------------------------------------------------------------------
// <Node::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Node::GetAssociations(uint8 const _groupIdx, uint8** o_associations)
{
	uint32 numAssociations = 0;
	if (Group* group = GetGroup(_groupIdx))
	{
		numAssociations = group->GetAssociations(o_associations);
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <Node::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Node::GetAssociations(uint8 const _groupIdx, InstanceAssociation** o_associations)
{
	uint32 numAssociations = 0;
	if (Group* group = GetGroup(_groupIdx))
	{
		numAssociations = group->GetAssociations(o_associations);
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <Node::GetMaxAssociations>
// Gets the maximum number of associations for a group
//-----------------------------------------------------------------------------
uint8 Node::GetMaxAssociations(uint8 const _groupIdx)
{
	uint8 maxAssociations = 0;
	if (Group* group = GetGroup(_groupIdx))
	{
		maxAssociations = group->GetMaxAssociations();
	}

	return maxAssociations;
}

//-----------------------------------------------------------------------------
// <Node::IsMultiInstance>
// Returns true if group supports multi instance
//-----------------------------------------------------------------------------
bool Node::IsMultiInstance(uint8 const _groupIdx)
{
	bool multiInstance = false;
	if (Group* group = GetGroup(_groupIdx))
	{
		multiInstance = group->IsMultiInstance();
	}
	return multiInstance;
}

//-----------------------------------------------------------------------------
// <Node::GetGroupLabel>
// Gets the label for a particular group
//-----------------------------------------------------------------------------
string Node::GetGroupLabel(uint8 const _groupIdx)
{
	string label = "";
	if (Group* group = GetGroup(_groupIdx))
	{
		label = group->GetLabel();
	}

	return label;
}

//-----------------------------------------------------------------------------
// <Node::AddAssociation>
// Adds a node to an association group
//-----------------------------------------------------------------------------
void Node::AddAssociation(uint8 const _groupIdx, uint8 const _targetNodeId, uint8 const _instance)
{
	if (Group* group = GetGroup(_groupIdx))
	{
		group->AddAssociation(_targetNodeId, _instance);
	}
}

//-----------------------------------------------------------------------------
// <Node::RemoveAssociation>
// Removes a node from an association group
//-----------------------------------------------------------------------------
void Node::RemoveAssociation(uint8 const _groupIdx, uint8 const _targetNodeId, uint8 const _instance)
{
	if (Group* group = GetGroup(_groupIdx))
	{
		group->RemoveAssociation(_targetNodeId, _instance);
	}
}

//-----------------------------------------------------------------------------
// <Node::AutoAssociate>
// Automatically associate the controller with certain groups
//-----------------------------------------------------------------------------
void Node::AutoAssociate()
{
	bool autoAssociate = false;
	Options::Get()->GetOptionAsBool("Associate", &autoAssociate);
	if (autoAssociate)
	{
		// Try to automatically associate with any groups that have been flagged.
		uint8 controllerNodeId = GetDriver()->GetControllerNodeId();

		for (map<uint8, Group*>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
		{
			Group* group = it->second;
			if (group->IsAuto() && !group->Contains(controllerNodeId))
			{
				// Associate the controller into the group
				Log::Write(LogLevel_Info, m_nodeId, "Adding the controller to group %d (%s) of node %d", group->GetIdx(), group->GetLabel().c_str(), GetNodeId());
				group->AddAssociation(controllerNodeId);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* Node::GetDriver() const
{
	return (Manager::Get()->GetDriver(m_homeId));
}

//-----------------------------------------------------------------------------
// Device Classes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Node::GetEndPointDeviceClassLabel>
// Use the device class data to get a label for a MultiChannel EndPoint.
//-----------------------------------------------------------------------------
string Node::GetEndPointDeviceClassLabel(uint8 const _generic, uint8 const _specific)
{
	char str[32];
	string label;

	snprintf(str, sizeof(str), "Generic 0x%.2x Specific 0x%.2x", _generic, _specific);
	label = str;

	// Read in the device class data if it has not been read already.
	if (!s_deviceClassesLoaded)
	{
		ReadDeviceClasses();
	}

	// Get the Generic device class label
	map<uint8, GenericDeviceClass*>::iterator git = s_genericDeviceClasses.find(_generic);
	if (git != s_genericDeviceClasses.end())
	{
		GenericDeviceClass* genericDeviceClass = git->second;
		label = genericDeviceClass->GetLabel();

		// Override with any specific device class label
		if (DeviceClass* specificDeviceClass = genericDeviceClass->GetSpecificDeviceClass(_specific))
		{
			label = specificDeviceClass->GetLabel();
		}
	}

	return label;
}

//-----------------------------------------------------------------------------
// <Node::SetDeviceClasses>
// Set the device class data for the node
//-----------------------------------------------------------------------------
bool Node::SetDeviceClasses(uint8 const _basic, uint8 const _generic, uint8 const _specific)
{
	m_basic = _basic;
	m_generic = _generic;
	m_specific = _specific;

	// Read in the device class data if it has not been read already.
	if (!s_deviceClassesLoaded)
	{
		ReadDeviceClasses();
	}

	// Get the basic device class label
	map<uint8, string>::iterator bit = s_basicDeviceClasses.find(_basic);
	if (bit != s_basicDeviceClasses.end())
	{
		m_type = bit->second;
		Log::Write(LogLevel_Info, m_nodeId, "  Basic device class    (0x%.2x) - %s", m_basic, m_type.c_str());
	}
	else
	{
		Log::Write(LogLevel_Info, m_nodeId, "  Basic device class unknown");
	}

	// Apply any Generic device class data
	uint8 basicMapping = 0;
	map<uint8, GenericDeviceClass*>::iterator git = s_genericDeviceClasses.find(_generic);
	if (git != s_genericDeviceClasses.end())
	{
		GenericDeviceClass* genericDeviceClass = git->second;
		m_type = genericDeviceClass->GetLabel();

		Log::Write(LogLevel_Info, m_nodeId, "  Generic device Class  (0x%.2x) - %s", m_generic, m_type.c_str());

		// Add the mandatory command classes for this generic class type
		AddMandatoryCommandClasses(genericDeviceClass->GetMandatoryCommandClasses());

		// Get the command class that COMMAND_CLASS_BASIC maps to.
		basicMapping = genericDeviceClass->GetBasicMapping();

		// Apply any Specific device class data
		if (DeviceClass* specificDeviceClass = genericDeviceClass->GetSpecificDeviceClass(_specific))
		{
			m_type = specificDeviceClass->GetLabel();

			Log::Write(LogLevel_Info, m_nodeId, "  Specific device class (0x%.2x) - %s", m_specific, m_type.c_str());

			// Add the mandatory command classes for this specific class type
			AddMandatoryCommandClasses(specificDeviceClass->GetMandatoryCommandClasses());

			if (specificDeviceClass->GetBasicMapping())
			{
				// Override the generic device class basic mapping with the specific device class one.
				basicMapping = specificDeviceClass->GetBasicMapping();
			}
		}
		else
		{
			Log::Write(LogLevel_Info, m_nodeId, "  No specific device class defined");
		}
	}
	else
	{
		Log::Write(LogLevel_Info, m_nodeId, "  No generic or specific device classes defined");
	}

	// Deal with sleeping devices
	if (!m_listening && !IsFrequentListeningDevice())
	{
		// Device does not always listen, so we need the WakeUp handler.  We can't
		// wait for the command class list because the request for the command
		// classes may need to go in the wakeup queue itself!
		if (Internal::CC::CommandClass* pCommandClass = AddCommandClass(Internal::CC::WakeUp::StaticGetCommandClassId()))
		{
			pCommandClass->SetInstance(1);
		}
	}

	// Apply any COMMAND_CLASS_BASIC remapping
	if (Internal::CC::Basic* cc = static_cast<Internal::CC::Basic*>(GetCommandClass(Internal::CC::Basic::StaticGetCommandClassId())))
	{
		cc->SetMapping(basicMapping);
	}

	// Write the mandatory command classes to the log
	if (!m_commandClassMap.empty())
	{
		map<uint8, Internal::CC::CommandClass*>::const_iterator cit;

		Log::Write(LogLevel_Info, m_nodeId, "  Mandatory Command Classes for Node %d:", m_nodeId);
		bool reportedClasses = false;
		for (cit = m_commandClassMap.begin(); cit != m_commandClassMap.end(); ++cit)
		{
			if (!cit->second->IsAfterMark() && cit->second->GetCommandClassId() != Internal::CC::NoOperation::StaticGetCommandClassId())
			{
				Log::Write(LogLevel_Info, m_nodeId, "    %s", cit->second->GetCommandClassName().c_str());
				reportedClasses = true;
			}
		}
		if (!reportedClasses)
		{
			Log::Write(LogLevel_Info, m_nodeId, "    None");
		}

		Log::Write(LogLevel_Info, m_nodeId, "  Mandatory Command Classes controlled by Node %d:", m_nodeId);
		reportedClasses = false;
		for (cit = m_commandClassMap.begin(); cit != m_commandClassMap.end(); ++cit)
		{
			if (cit->second->IsAfterMark())
			{
				Log::Write(LogLevel_Info, m_nodeId, "    %s", cit->second->GetCommandClassName().c_str());
				reportedClasses = true;
			}
		}
		if (!reportedClasses)
		{
			Log::Write(LogLevel_Info, m_nodeId, "    None");
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Node::SetPlusDeviceClasses>
// Set the device class data for the node based on the Zwave+ info report
//-----------------------------------------------------------------------------
bool Node::SetPlusDeviceClasses(uint8 const _role, uint8 const _nodeType, uint16 const _deviceType)
{
	if (m_nodePlusInfoReceived)
	{
		return false; // already set
	}

	if (!s_deviceClassesLoaded)
	{
		ReadDeviceClasses();
	}

	m_nodePlusInfoReceived = true;
	m_role = _role;
	m_deviceType = _deviceType;
	m_nodeType = _nodeType;

	Log::Write(LogLevel_Info, m_nodeId, "ZWave+ Info Received from Node %d", m_nodeId);
	map<uint8, DeviceClass*>::iterator nit = s_nodeTypes.find(m_nodeType);
	if (nit != s_nodeTypes.end())
	{
		DeviceClass* deviceClass = nit->second;

		Log::Write(LogLevel_Info, m_nodeId, "  Zwave+ Node Type  (0x%02x) - %s. Mandatory Command Classes:", m_nodeType, deviceClass->GetLabel().c_str());
		uint8 const *_commandClasses = deviceClass->GetMandatoryCommandClasses();

		/* no CommandClasses to add */
		if (_commandClasses != NULL)
		{
			int i = 0;
			while (uint8 ccid = _commandClasses[i++])
			{
				if (Internal::CC::CommandClasses::IsSupported(ccid))
				{
					Log::Write(LogLevel_Info, m_nodeId, "    %s", Internal::CC::CommandClasses::GetName(ccid).c_str());
				}
				else
				{
					Log::Write(LogLevel_Info, m_nodeId, "    0x%02x (Not Supported)", ccid);
				}
			}

			// Add the mandatory command classes for this Roletype
			AddMandatoryCommandClasses(deviceClass->GetMandatoryCommandClasses());
		}
		else
		{
			Log::Write(LogLevel_Info, m_nodeId, "    NONE");
		}
	}
	else
	{
		Log::Write(LogLevel_Warning, m_nodeId, "  Zwave+ Node Type  (0x%02x) - NOT FOUND. No Mandatory Command Classes Loaded:", m_nodeType);
	}

	// Apply any Zwave+ device class data
	map<uint16, DeviceClass*>::iterator dit = s_deviceTypeClasses.find(_deviceType);
	if (dit != s_deviceTypeClasses.end())
	{
		DeviceClass* deviceClass = dit->second;
		// m_type = deviceClass->GetLabel(); // do we what to update the type with the zwave+ info??

		Log::Write(LogLevel_Info, m_nodeId, "  Zwave+ Device Type  (0x%04x) - %s. Mandatory Command Classes:", _deviceType, deviceClass->GetLabel().c_str());
		uint8 const *_commandClasses = deviceClass->GetMandatoryCommandClasses();

		/* no CommandClasses to add */
		if (_commandClasses != NULL)
		{
			int i = 0;
			while (uint8 ccid = _commandClasses[i++])
			{
				if (Internal::CC::CommandClasses::IsSupported(ccid))
				{
					Log::Write(LogLevel_Info, m_nodeId, "    %s", Internal::CC::CommandClasses::GetName(ccid).c_str());
				}
				else
				{
					Log::Write(LogLevel_Info, m_nodeId, "    0x%02x (Not Supported)", ccid);
				}
			}

			// Add the mandatory command classes for this device class type
			AddMandatoryCommandClasses(deviceClass->GetMandatoryCommandClasses());
		}
		else
		{
			Log::Write(LogLevel_Info, m_nodeId, "    NONE");
		}
	}
	else
	{
		Log::Write(LogLevel_Warning, m_nodeId, "  Zwave+ Device Type  (0x%04x) - NOT FOUND. No Mandatory Command Classes Loaded:", _deviceType);
	}

	// Apply any Role device class data
	map<uint8, DeviceClass*>::iterator rit = s_roleDeviceClasses.find(_role);
	if (rit != s_roleDeviceClasses.end())
	{
		DeviceClass* roleDeviceClass = rit->second;

		Log::Write(LogLevel_Info, m_nodeId, "  ZWave+ Role Type  (0x%02x) - %s", _role, roleDeviceClass->GetLabel().c_str());

		uint8 const *_commandClasses = roleDeviceClass->GetMandatoryCommandClasses();

		/* no CommandClasses to add */
		if (_commandClasses != NULL)
		{
			int i = 0;
			while (uint8 ccid = _commandClasses[i++])
			{
				if (Internal::CC::CommandClasses::IsSupported(ccid))
				{
					Log::Write(LogLevel_Info, m_nodeId, "    %s", Internal::CC::CommandClasses::GetName(ccid).c_str());
				}
				else
				{
					Log::Write(LogLevel_Info, m_nodeId, "    0x%02x (Not Supported)", ccid);
				}
			}

			// Add the mandatory command classes for this role class type
			AddMandatoryCommandClasses(roleDeviceClass->GetMandatoryCommandClasses());
		}
		else
		{
			Log::Write(LogLevel_Info, m_nodeId, "    NONE");
		}

	}
	else
	{
		Log::Write(LogLevel_Warning, m_nodeId, "  ZWave+ Role Type  (0x%02x) - NOT FOUND. No Mandatory Command Classes Loaded:", _role);
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Node::AddMandatoryCommandClasses>
// Add mandatory command classes to the node
//-----------------------------------------------------------------------------
bool Node::AddMandatoryCommandClasses(uint8 const* _commandClasses)
{
	if ( NULL == _commandClasses)
	{
		// No command classes to add
		return false;
	}

	int i = 0;
	bool afterMark = false;
	while (uint8 cc = _commandClasses[i++])
	{
		if (cc == 0xef)
		{
			// COMMAND_CLASS_MARK.
			// Marks the end of the list of supported command classes.  The remaining classes
			// are those that can be controlled by this device, which we can ignore.
			afterMark = true;
			continue;
		}

		if (Internal::CC::CommandClasses::IsSupported(cc))
		{
			if (Internal::CC::Security::StaticGetCommandClassId() == cc && !GetDriver()->isNetworkKeySet())
			{
				Log::Write(LogLevel_Warning, m_nodeId, "Security Command Class Cannot be Enabled - NetworkKey is not set");
				continue;
			}

			if (Internal::CC::CommandClass* commandClass = AddCommandClass(cc))
			{
				// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
				if (afterMark)
				{
					commandClass->SetAfterMark();
				}

				// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
				// then some command class instance counts will increase.
				commandClass->SetInstance(1);
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Node::ReadDeviceClasses>
// Read the static device class data from the device_classes.xml file
//-----------------------------------------------------------------------------
void Node::ReadDeviceClasses()
{
	// Load the XML document that contains the device class information
	string configPath;
	Options::Get()->GetOptionAsString("ConfigPath", &configPath);

	string filename = configPath + string("device_classes.xml");

	TiXmlDocument doc;
	if (!doc.LoadFile(filename.c_str(), TIXML_ENCODING_UTF8))
	{
		Log::Write(LogLevel_Info, "Failed to load device_classes.xml");
		Log::Write(LogLevel_Info, "Check that the config path provided when creating the Manager points to the correct location.");
		return;
	}
	doc.SetUserData((void *) filename.c_str());
	TiXmlElement const* deviceClassesElement = doc.RootElement();

	// Read the basic and generic device classes
	TiXmlElement const* child = deviceClassesElement->FirstChildElement();
	while (child)
	{
		char const* str = child->Value();
		if (str)
		{
			char const* keyStr = child->Attribute("key");
			if (keyStr)
			{
				char* pStop;
				/* we do a short here, as they key can be a DeviceType Key which is short */
				uint16_t key = (uint16) strtol(keyStr, &pStop, 16);

				if (!strcmp(str, "Generic"))
				{
					if (s_genericDeviceClasses.find((uint8_t)(key & 0xFF)) == s_genericDeviceClasses.end()) {
						s_genericDeviceClasses[(uint8_t) (key & 0xFF)] = new GenericDeviceClass(child);
					} else {
						Log::Write(LogLevel_Warning, "Duplicate Entry for Generic Device Class %d", key);
					}
				}
				else if (!strcmp(str, "Basic"))
				{
					if (s_basicDeviceClasses.find((uint8_t)(key & 0xFF)) == s_basicDeviceClasses.end()) { 
						char const* label = child->Attribute("label");
						if (label)
						{
							s_basicDeviceClasses[(uint8_t) (key & 0xFF)] = label;
						}
					} else {
						Log::Write(LogLevel_Warning, "Duplicate Entry for Basic Device Class %d", key);
					}
				}
				else if (!strcmp(str, "Role"))
				{
					if (s_roleDeviceClasses.find((uint8_t)(key & 0xFF)) == s_roleDeviceClasses.end()) { 
						s_roleDeviceClasses[(uint8_t) (key & 0xFF)] = new DeviceClass(child);
					} else {
						Log::Write(LogLevel_Warning, "Duplicate Entry for Role Device Classes %d", key);
					}
				}
				else if (!strcmp(str, "DeviceType"))
				{
					if (s_deviceTypeClasses.find(key) == s_deviceTypeClasses.end()) { 
						s_deviceTypeClasses[key] = new DeviceClass(child);
					} else {
						Log::Write(LogLevel_Warning, "Duplicate Entry for Device Type Class %d", key);
					}
				}
				else if (!strcmp(str, "NodeType"))
				{
					if (s_nodeTypes.find((uint8_t)(key & 0xFF)) == s_nodeTypes.end()) {
						s_nodeTypes[(uint8_t) (key & 0xFF)] = new DeviceClass(child);
					} else {
						Log::Write(LogLevel_Warning, "Duplicate Entry for Node Type %d", key);
					}
				}
			}
		}

		child = child->NextSiblingElement();
	}

	s_deviceClassesLoaded = true;
}

//-----------------------------------------------------------------------------
// <Node::GetNoderStatistics>
// Return driver statistics
//-----------------------------------------------------------------------------
void Node::GetNodeStatistics(NodeData* _data)
{
	_data->m_sentCnt = m_sentCnt;
	_data->m_sentFailed = m_sentFailed;
	_data->m_retries = m_retries;
	_data->m_receivedCnt = m_receivedCnt;
	_data->m_receivedDups = m_receivedDups;
	_data->m_receivedUnsolicited = m_receivedUnsolicited;
	_data->m_lastRequestRTT = m_lastRequestRTT;
	_data->m_lastResponseRTT = m_lastResponseRTT;
	_data->m_sentTS = m_sentTS.GetAsString();
	_data->m_receivedTS = m_receivedTS.GetAsString();
	_data->m_averageRequestRTT = m_averageRequestRTT;
	_data->m_averageResponseRTT = m_averageResponseRTT;
	_data->m_txStatusReportSupported = m_txStatusReportSupported;
	_data->m_txTime = m_txTime;
	_data->m_hops = m_hops;
	// petergebruers swap src and dst
	// petergebruers there are 5 rssi values because there are
	// 4 repeaters + 1 sending node
	strncpy(_data->m_rssi_1, m_rssi_1, sizeof(_data->m_rssi_1));
	strncpy(_data->m_rssi_2, m_rssi_2, sizeof(_data->m_rssi_2));
	strncpy(_data->m_rssi_3, m_rssi_3, sizeof(_data->m_rssi_3));
	strncpy(_data->m_rssi_4, m_rssi_4, sizeof(_data->m_rssi_4));
	strncpy(_data->m_rssi_5, m_rssi_5, sizeof(_data->m_rssi_5));
	_data->m_ackChannel = m_ackChannel;
	_data->m_lastTxChannel = m_lastTxChannel;
	_data->m_routeScheme = m_routeScheme;
	_data->m_routeUsed[0] = m_routeUsed[0];
	_data->m_routeUsed[1] = m_routeUsed[1];
	_data->m_routeUsed[2] = m_routeUsed[2];
	_data->m_routeUsed[3] = m_routeUsed[3];
	//petergebruers: missed m_routeSpeed
	_data->m_routeSpeed = m_routeSpeed;
	_data->m_routeTries = m_routeTries;
	_data->m_lastFailedLinkFrom = m_lastFailedLinkFrom;
	_data->m_lastFailedLinkTo = m_lastFailedLinkTo;

	_data->m_quality = m_quality;
	memcpy(_data->m_lastReceivedMessage, m_lastReceivedMessage, sizeof(m_lastReceivedMessage));
	for (map<uint8, Internal::CC::CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it)
	{
		CommandClassData ccData;
		ccData.m_commandClassId = it->second->GetCommandClassId();
		ccData.m_sentCnt = it->second->GetSentCnt();
		ccData.m_receivedCnt = it->second->GetReceivedCnt();
		_data->m_ccData.push_back(ccData);
	}
}

//-----------------------------------------------------------------------------
// <DeviceClass::DeviceClass>
// Constructor
//-----------------------------------------------------------------------------
Node::DeviceClass::DeviceClass(TiXmlElement const* _el) :
		m_mandatoryCommandClasses(NULL), m_basicMapping(0)
{
	char const* str = _el->Attribute("label");
	if (str)
	{
		m_label = str;
	}

	str = _el->Attribute("command_classes");
	if (str)
	{
		// Parse the comma delimted command class
		// list into a temporary vector.
		vector<uint8> ccs;
		char* pos = const_cast<char*>(str);
		while (*pos)
		{
			ccs.push_back((uint8) strtol(pos, &pos, 16));
			if ((*pos) == ',')
			{
				++pos;
			}
		}

		// Copy the vector contents into an array.
		size_t numCCs = ccs.size();
		m_mandatoryCommandClasses = new uint8[numCCs + 1];
		m_mandatoryCommandClasses[numCCs] = 0;	// Zero terminator

		for (uint32 i = 0; i < numCCs; ++i)
		{
			m_mandatoryCommandClasses[i] = ccs[i];
		}
	}

	str = _el->Attribute("basic");
	if (str)
	{
		char* pStop;
		m_basicMapping = (uint8) strtol(str, &pStop, 16);
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::GenericDeviceClass>
// Constructor
//-----------------------------------------------------------------------------
Node::GenericDeviceClass::GenericDeviceClass(TiXmlElement const* _el) :
		DeviceClass(_el)
{
	// Add any specific device classes
	TiXmlElement const* child = _el->FirstChildElement();
	while (child)
	{
		char const* str = child->Value();
		if (str && !strcmp(str, "Specific"))
		{
			char const* keyStr = child->Attribute("key");
			if (keyStr)
			{
				char* pStop;
				uint8 key = (uint8) strtol(keyStr, &pStop, 16);

				m_specificDeviceClasses[key] = new DeviceClass(child);
			}
		}

		child = child->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::~GenericDeviceClass>
// Destructor
//-----------------------------------------------------------------------------
Node::GenericDeviceClass::~GenericDeviceClass()
{
	while (!m_specificDeviceClasses.empty())
	{
		map<uint8, DeviceClass*>::iterator it = m_specificDeviceClasses.begin();
		delete it->second;
		m_specificDeviceClasses.erase(it);
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::GetSpecificDeviceClass>
// Get a specific device class object
//-----------------------------------------------------------------------------
Node::DeviceClass* Node::GenericDeviceClass::GetSpecificDeviceClass(uint8 const& _specific)
{
	map<uint8, DeviceClass*>::iterator it = m_specificDeviceClasses.find(_specific);
	if (it != m_specificDeviceClasses.end())
	{
		return it->second;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::GenerateNonceKey>
// Generate a NONCE key for this node
//-----------------------------------------------------------------------------
uint8 *Node::GenerateNonceKey()
{
	uint8 idx = this->m_lastnonce;

	/* The first byte must be unique and non-zero.  The others are random.
	 Per Numerical Recipes in C its best to use the high-order byte.  The
	 floating point calculation here doesn't assume the size of the random
	 integer, otherwise we could just shift the high byte over.
	 */
	uint8 match = 0;
	do
	{
		this->m_nonces[idx][0] = 1 + (uint8) (255.0 * rand() / (RAND_MAX + 1.0));
		match = 0;
		for (int i = 0; i < 8; i++)
		{
			if (i == idx)
			{
				continue;
			}
			if (this->m_nonces[idx][0] == this->m_nonces[i][0])
			{
				match = 1;
			}
		}
	} while (match);

	/* The other bytes have no restrictions. */
	for (int i = 1; i < 8; i++)
	{
		this->m_nonces[idx][i] = (int) (256.0 * rand() / (RAND_MAX + 1.0));
	}

	this->m_lastnonce++;
	if (this->m_lastnonce >= 8)
		this->m_lastnonce = 0;
	for (uint8 i = 0; i < 8; i++)
	{
		Internal::PrintHex("NONCES", (const uint8_t*) this->m_nonces[i], 8);
	}
	return &this->m_nonces[idx][0];
}
//-----------------------------------------------------------------------------
// <Node::GetNonceKey>
// Get a NONCE key for this node that matches the nonceid.
//-----------------------------------------------------------------------------

uint8 *Node::GetNonceKey(uint32 nonceid)
{
	for (uint8 i = 0; i < 8; i++)
	{
		/* make sure the nonceid matches the first byte of our stored Nonce */
		if (nonceid == this->m_nonces[i][0])
		{
			return &this->m_nonces[i][0];
		}
	}
	Log::Write(LogLevel_Warning, m_nodeId, "A Nonce with id %x does not exist", nonceid);
	for (uint8 i = 0; i < 8; i++)
	{
		Internal::PrintHex("NONCES", (const uint8_t*) this->m_nonces[i], 8);
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::GetDeviceTypeString>
// Get the ZWave+ DeviceType as a String
//-----------------------------------------------------------------------------
string Node::GetDeviceTypeString()
{

	if (!s_deviceClassesLoaded)
	{
		ReadDeviceClasses();
	}
	map<uint16, DeviceClass*>::iterator nit = s_deviceTypeClasses.find(m_deviceType);
	if (nit != s_deviceTypeClasses.end())
	{
		DeviceClass* deviceClass = nit->second;
		return deviceClass->GetLabel();
	}
	return "";
}
//-----------------------------------------------------------------------------
// <Node::GetRoleTypeString>
// Get the ZWave+ RoleType as a String
//-----------------------------------------------------------------------------
string Node::GetRoleTypeString()
{
	if (!s_deviceClassesLoaded)
	{
		ReadDeviceClasses();
	}
	map<uint8, DeviceClass*>::iterator nit = s_roleDeviceClasses.find(m_role);
	if (nit != s_roleDeviceClasses.end())
	{
		DeviceClass* deviceClass = nit->second;
		return deviceClass->GetLabel();
	}
	return "";
}
//-----------------------------------------------------------------------------
// <Node::GetRoleTypeString>
// Get the ZWave+ NodeType as a String
//-----------------------------------------------------------------------------
string Node::GetNodeTypeString()
{
	if (!s_deviceClassesLoaded)
	{
		ReadDeviceClasses();
	}
	map<uint8, DeviceClass*>::iterator nit = s_nodeTypes.find(m_nodeType);
	if (nit != s_nodeTypes.end())
	{
		DeviceClass* deviceClass = nit->second;
		return deviceClass->GetLabel();
	}
	return "";
}

//-----------------------------------------------------------------------------
// <Node::GetRoleTypeString>
// Is the Node Reset?
//-----------------------------------------------------------------------------
bool Node::IsNodeReset()
{
	Internal::CC::DeviceResetLocally *drl = static_cast<Internal::CC::DeviceResetLocally *>(GetCommandClass(Internal::CC::DeviceResetLocally::StaticGetCommandClassId()));
	if (drl)
		return drl->IsDeviceReset();
	else
		return false;

}

//-----------------------------------------------------------------------------
// <Node::SetProductDetails>
// Assign a ProductDetails class to this node
//-----------------------------------------------------------------------------
void Node::SetProductDetails(std::shared_ptr<Internal::ProductDescriptor> product)
{
	/* add the new ProductDescriptor */
	m_Product = product;
}

//-----------------------------------------------------------------------------
// <Node::getConfigPath>
// get the Path to the configFile for this device.
//-----------------------------------------------------------------------------
string Node::getConfigPath()
{
	if (m_Product)
		return m_Product->GetConfigPath();
	else
		return "";

}

//-----------------------------------------------------------------------------
// <Node::setFileConfigRevision>
// Set Loaded Config File Revision
//-----------------------------------------------------------------------------
void Node::setFileConfigRevision(uint32 rev)
{
	m_fileConfigRevision = rev;
	Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
	if (cc)
	{
		cc->setFileConfigRevision(rev);
	}
	/* check if this is the latest */
	checkLatestConfigRevision();
}

//-----------------------------------------------------------------------------
// <Node::setLoadedConfigRevision>
// Set Loaded Config File Revision
//-----------------------------------------------------------------------------
void Node::setLoadedConfigRevision(uint32 rev)
{
	m_loadedConfigRevision = rev;
	Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
	if (cc)
	{
		cc->setLoadedConfigRevision(rev);
	}

}

//-----------------------------------------------------------------------------
// <Node::setLatestConfigRevisionn>
// Set Latest Config File Revision
//-----------------------------------------------------------------------------
void Node::setLatestConfigRevision(uint32 rev)
{
	m_latestConfigRevision = rev;
	Internal::CC::ManufacturerSpecific* cc = static_cast<Internal::CC::ManufacturerSpecific*>(GetCommandClass(Internal::CC::ManufacturerSpecific::StaticGetCommandClassId()));
	if (cc)
	{
		cc->setLatestConfigRevision(rev);
	}
}

//-----------------------------------------------------------------------------
// <Node::checkConfigRevision>
// Check if our Config File is the latest
//-----------------------------------------------------------------------------

void Node::checkLatestConfigRevision()
{
	if (m_fileConfigRevision != 0)
	{
		GetDriver()->CheckNodeConfigRevision(this);
	}
}

//-----------------------------------------------------------------------------
// <Node::GetMetaData>
// Get MetaData about a node
//-----------------------------------------------------------------------------

string const Node::GetMetaData(MetaDataFields field)
{
	if (this->m_metadata.find(field) != this->m_metadata.end())
	{
		return this->m_metadata[field];
	}
	return string();
}

//-----------------------------------------------------------------------------
// <Node::GetChangeLog>
// Get MetaData about a node
//-----------------------------------------------------------------------------
Node::ChangeLogEntry const Node::GetChangeLog(uint32_t revision)
{
	if (this->m_changeLog.find(revision) != this->m_changeLog.end())
	{
		return this->m_changeLog[revision];
	}
	ChangeLogEntry cle;
	cle.revision = -1;
	return cle;
}

//-----------------------------------------------------------------------------
// <Node::GetMetaDataId>
// Translate the MetaData String to a ID
//-----------------------------------------------------------------------------
Node::MetaDataFields Node::GetMetaDataId(string name)
{
	if (name == "OzwInfoPage")
		return MetaData_OzwInfoPage_URL;
	if (name == "ZWProductPage")
		return MetaData_ZWProductPage_URL;
	if (name == "ProductPic")
		return MetaData_ProductPic;
	if (name == "Description")
		return MetaData_Description;
	if (name == "ProductManual")
		return MetaData_ProductManual_URL;
	if (name == "ProductPage")
		return MetaData_ProductPage_URL;
	if (name == "InclusionDescription")
		return MetaData_InclusionHelp;
	if (name == "ExclusionDescription")
		return MetaData_ExclusionHelp;
	if (name == "ResetDescription")
		return MetaData_ResetHelp;
	if (name == "WakeupDescription")
		return MetaData_WakeupHelp;
	if (name == "ProductSupport")
		return MetaData_ProductSupport_URL;
	if (name == "FrequencyName")
		return MetaData_Frequency;
	if (name == "Name")
		return MetaData_Name;
	if (name == "Identifier")
		return MetaData_Identifier;
	return MetaData_Invalid;
}
//-----------------------------------------------------------------------------
// <Node::GetMetaDataString(>
// Translate the MetaDataID to a String
//-----------------------------------------------------------------------------
string const Node::GetMetaDataString(Node::MetaDataFields id)
{
	switch (id)
	{
		case MetaData_OzwInfoPage_URL:
			return "OzwInfoPage";
		case MetaData_ZWProductPage_URL:
			return "ZWProductPage";
		case MetaData_ProductPic:
			return "ProductPic";
		case MetaData_Description:
			return "Description";
		case MetaData_ProductManual_URL:
			return "ProductManual";
		case MetaData_ProductPage_URL:
			return "ProductPage";
		case MetaData_InclusionHelp:
			return "InclusionDescription";
		case MetaData_ExclusionHelp:
			return "ExclusionDescription";
		case MetaData_ResetHelp:
			return "ResetDescription";
		case MetaData_WakeupHelp:
			return "WakeupDescription";
		case MetaData_ProductSupport_URL:
			return "ProductSupport";
		case MetaData_Frequency:
			return "FrequencyName";
		case MetaData_Name:
			return "Name";
		case MetaData_Identifier:
			return "Identifier";
		case MetaData_Invalid:
			return "";
	}
	return "";
}

//-----------------------------------------------------------------------------
// <Node::ReadMetaDataFromXML(>
// Read the MetaData from the Config File
//-----------------------------------------------------------------------------
void Node::ReadMetaDataFromXML(TiXmlElement const* _valueElement)
{
	TiXmlElement const* ccElement = _valueElement->FirstChildElement();
	while (ccElement)
	{
		if (!strcmp(ccElement->Value(), "MetaData"))
		{
			TiXmlElement const* metadata = ccElement->FirstChildElement();
			while (metadata)
			{
				if (!strcmp(metadata->Value(), "MetaDataItem"))
				{
					string name = metadata->Attribute("name");
					if (GetMetaDataId(name) == MetaData_Invalid)
					{
						Log::Write(LogLevel_Warning, m_nodeId, "Invalid MetaData Name in Config: %s", name.c_str());
						metadata = metadata->NextSiblingElement();
						continue;
					}
					/* these are the MetaData that have type/id entries */
					switch (GetMetaDataId(name))
					{
						case MetaData_ZWProductPage_URL:
						case MetaData_Identifier:
						case MetaData_Frequency:
						{
							int intVal;
							uint16_t type = 0;
							uint16_t id = 0;
							if (TIXML_SUCCESS == metadata->QueryIntAttribute("type", &intVal))
							{
								type = (uint16_t) intVal;
							}
							if (TIXML_SUCCESS == metadata->QueryIntAttribute("id", &intVal))
							{
								id = (uint16_t) intVal;
							}
							if ((type != GetProductType()) || (id != GetProductId()))
							{
								metadata = metadata->NextSiblingElement();
								continue;
							}
							break;
						}
							/* for the rest, just take the standard entry */
						default:
							break;
					}
					if (metadata->GetText())
						this->m_metadata[GetMetaDataId(name)] = metadata->GetText();
				}
				else if (!strcmp(metadata->Value(), "ChangeLog"))
				{
					TiXmlElement const* entry = metadata->FirstChildElement("Entry");
					while (entry)
					{
						ChangeLogEntry cle;
						cle.author = entry->Attribute("author");
						cle.date = entry->Attribute("date");
						cle.description = entry->GetText();
						entry->QueryIntAttribute("revision", &cle.revision);
						m_changeLog.insert(std::pair<uint32_t, ChangeLogEntry>(cle.revision, cle));
						entry = entry->NextSiblingElement("Entry");
					}
				}
				metadata = metadata->NextSiblingElement();
			}
		}
		ccElement = ccElement->NextSiblingElement();
	}
}
//-----------------------------------------------------------------------------
// <Node::WriteMetaDataXML>
// Write the MetaData to the Cache File
//-----------------------------------------------------------------------------
void Node::WriteMetaDataXML(TiXmlElement *mdElement)
{
	// Write out the MetaDataItems
	for (map<MetaDataFields, string>::iterator it = m_metadata.begin(); it != m_metadata.end(); ++it)
	{
		/* only write if its a valid MetaData */
		if (it->first < Node::MetaData_Invalid)
		{
			TiXmlElement* mdiElement = new TiXmlElement("MetaDataItem");
			mdiElement->SetAttribute("name", GetMetaDataString(it->first).c_str());
			switch (it->first)
			{
				case MetaData_ZWProductPage_URL:
				case MetaData_Identifier:
				case MetaData_Frequency:
					mdiElement->SetAttribute("type", GetProductType());
					mdiElement->SetAttribute("id", GetProductId());
					break;
					/* for the rest, just take the standard entry */
				default:
					break;
			}
			TiXmlText* textElement = new TiXmlText(it->second.c_str());
			mdiElement->LinkEndChild(textElement);
			mdElement->LinkEndChild(mdiElement);

		}
	}
	if (m_changeLog.size() > 0)
	{
		TiXmlElement* cl = new TiXmlElement("ChangeLog");
		for (map<uint32_t, ChangeLogEntry>::iterator it = m_changeLog.begin(); it != m_changeLog.end(); ++it)
		{
			TiXmlElement* cle = new TiXmlElement("Entry");
			cle->SetAttribute("author", it->second.author.c_str());
			cle->SetAttribute("date", it->second.date.c_str());
			cle->SetAttribute("revision", it->second.revision);
			TiXmlText* textElement = new TiXmlText(it->second.description.c_str());
			cle->LinkEndChild(textElement);
			cl->LinkEndChild(cle);
		}
		mdElement->LinkEndChild(cl);
	}
}
