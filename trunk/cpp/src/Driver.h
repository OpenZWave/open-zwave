//-----------------------------------------------------------------------------
//
//	Driver.h
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

#ifndef _Driver_H
#define _Driver_H

#include <string>
#include <map>
#include <list>

#include "Defs.h"
#include "ValueID.h"
#include "Node.h"

namespace OpenZWave
{
	class Msg;
	class Value;
	class Event;
	class Mutex;
	class SerialPort;
	class Thread;
	class ControllerReplication;
	class Notification;

	/** \brief The Driver class handles communication between OpenZWave 
	 *  and a device attached via a serial port (typically a controller).
	 */
	class Driver
	{
		friend class Manager;
		friend class Node;
		friend class Group;
		friend class CommandClass;
		friend class Value;
		friend class ValueStore;
		friend class Association;
		friend class Basic;
		friend class ManufacturerSpecific;
		friend class WakeUp;
		friend class Hail;

	//-----------------------------------------------------------------------------
	// Construction / Destruction
	//-----------------------------------------------------------------------------
	private:
		/** 
		 *  Creates threads, events and initializes member variables and the node array.
		 */
        Driver( string const& _serialPortName );
		/** Sets "exit" flags and stops the three background threads (pollThread, serialThread
		 *  and driverThread).  Then clears out the send queue and node array.  Notifies
		 *  watchers and exits.
		 */
		virtual ~Driver();

		/**
		 *  Start the driverThread
		 */
		void Start();
		/**
		 *  Entry point for driverThread
		 */
		static void DriverThreadEntryPoint( void* _context );
		/**
		 *  ThreadProc for driverThread.  This is where all the "action" takes place.  
		 *  <p>
		 *  First, the thread is initialized by calling Init().  If Init() fails, it will be retried 
		 *  every 5 seconds for the first two minutes and every 30 seconds thereafter.
		 *  <p>
		 *  After the thread is successfully initialized, the thread enters a loop with the
		 *  following elements:
		 *  - Confirm that m_exit is still false (or exit from the thread if it is true)
		 *  - Call ReadMsg() to consume any available messages from the controller
		 *  - Call NotifyWatchers() to send any pending notifications
		 *  - If the thread is not waiting for an ACK, a callback or a message reply, send [any][the next] queued message[s]
		 *  - If there was no message read or sent (workDone=false), sleep for 5 seconds.  If nothing happened
		 *  within this time frame and something was expected (ACK, callback or reply), retrieve the
		 *  last message from the send queue and examine GetSendAttempts().  If greater than 2, give up
		 *  and remove the message from the queue.  Otherwise, resend the message.
		 *  - If something did happen [reset m_wakeEvent]
		 */
		void DriverThreadProc();
		/**
		 *  Initialize the controller.  Open the specified serial port, start the serialThread 
		 *  and pollThread, then send a NAK to the device [presumably to flush it].
		 *  <p>
		 *  Then queue the commands to retrieve the Z-Wave interface:
		 *  - Get version
		 *  - Get home and node IDs
		 *  - Get controller capabilities
		 *  - Get serial API capabilties
		 *  - [Get SUC node ID]
		 *  - Get init data [identifying the nodes on the network]
		 *  Init() will return false if the serial port could not be opened.
		 */
		bool Init( uint32 _attempts );

		Thread*					m_driverThread;			/**< Thread for reading from the Z-Wave controller, and for creating and managing the other threads for sending, polling etc. */
		Event*					m_wakeEvent;			/**< Event that will be signalled when we should check for new work */
		Event*					m_exitEvent;			/**< Event that will be signalled when the application is exiting */
		bool					m_exit;					/**< Flag that is set when the application is exiting. */
		bool					m_init;					/**< Set to true once the driver has been initialised */
		bool					m_awakeNodesQueried;	/**< Set to true once the driver has polled all awake nodes */
		bool					m_allNodesQueried;		/**< Set to true once the driver has polled all nodes */

	//-----------------------------------------------------------------------------
	//	Configuration
	//-----------------------------------------------------------------------------
	private:
		void RequestConfig();						/**< Get the network configuration from the Z-Wave network */
		bool ReadConfig();							/**< Read the configuration from a file */
		void WriteConfig();							/**< Save the configuration to a file */

	//-----------------------------------------------------------------------------
	//	Controller
	//-----------------------------------------------------------------------------
	private:
		// Controller Capabilities (return in FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES)
		enum
		{
			ControllerCaps_Secondary		= 0x01,		/**< The controller is a secondary. */
			ControllerCaps_OnOtherNetwork	= 0x02,		/**< The controller is not using its default HomeID. */
			ControllerCaps_SIS				= 0x04,		/**< There is a SUC ID Server on the network. */
			ControllerCaps_RealPrimary		= 0x08,		/**< Controller was the primary before the SIS was added. */
			ControllerCaps_SUC				= 0x10		/**< Controller is a static update controller. */
		};

		// Init Capabilities (return in FUNC_ID_SERIAL_API_GET_INIT_DATA)
		enum
		{
			InitCaps_Slave					= 0x01,		/**<  */
			InitCaps_TimerSupport			= 0x02,		/**< Controller supports timers. */
			InitCaps_Secondary				= 0x04,		/**< Controller is a secondary. */
			InitCaps_SUC					= 0x08,		/**< Controller is a static update controller. */
		};

		bool IsPrimaryController()const{ return ((m_initCaps & InitCaps_Secondary) == 0); }
		bool IsStaticUpdateController()const{ return ((m_initCaps & InitCaps_SUC) != 0); }
		bool IsBridgeController()const{ return (m_libraryType == 7); }
		bool IsInclusionController()const{ return ((m_controllerCaps & ControllerCaps_SIS) != 0); }

		uint32 GetHomeId()const{ return m_homeId; }
		uint8 GetNodeId()const{ return m_nodeId; }
		string GetSerialPortName()const{ return m_serialPortName; }
		string GetLibraryVersion()const{ return m_libraryVersion; }
		string GetLibraryTypeName()const{ return m_libraryTypeName; }

		/**
		 *  A version of GetNode that does not have the protective "lock" and "release" requirement.  
		 *  This function can be used within driverThread, which "knows" that the node will not be
		 *  changed or deleted while it is being used.
		 *  \param _nodeId The nodeId (index into the node array) identifying the node to be returned
		 *  \return
		 *  A pointer to the specified node (if it exists) or NULL if not.
		 *  \see GetNode
		 */
		Node* GetNodeUnsafe( uint8 _nodeId );
		/**
		 *  Locks the node array and returns the specified node (if it exists).  If a node is returned,
		 *  the lock must be released after the node has been processed via a call to ReleaseNodes().
		 *  If the node specified by _nodeId does not exist, the lock is released and NULL is returned.
		 *  \param _nodeId The nodeId (index into the node array) identifying the node to be returned
		 *  \return
		 *  A pointer to the specified node (if it exists) or NULL if not.
		 *  \see LockNodes, ReleaseNodes
		 */
		Node* GetNode( uint8 _nodeId );
		/**
		 *  Lock the nodes so no other thread can modify them.
		 */
		void LockNodes();
		/**
		 *  Release the lock on the nodes so other threads can modify them.
		 */
		void ReleaseNodes();

		static void SerialThreadEntryPoint( void* _context );
		void SerialThreadProc();

		string					m_serialPortName;							// name used to open the serial port.
		uint32					m_homeId;									// Home ID of the Z-Wave controller.  Not valid until the DriverReady notification has been received.
		SerialPort*				m_serialPort;								// Handles communications with the controller hardware.
		Thread*					m_serialThread;								// Watches for data arriving at the serial port.
		
		string					m_libraryVersion;							// Verison of the Z-Wave Library used by the controller.
		string					m_libraryTypeName;							// Name describing the library type.
		uint8					m_libraryType;								// Type of library used by the controller.

		uint8					m_initVersion;								// Version of the Serial API used by the controller.
		uint8					m_initCaps;									// Set of flags indicating the serial API capabilities (See IsSlave, HasTimerSupport, IsPrimaryController and IsStaticUpdateController above).
		uint8					m_controllerCaps;							// Set of flags indicating the controller's capabilities (See IsInclusionController above).
		uint8					m_nodeId;									// Z-Wave Controller's own node ID.
		Node*					m_nodes[256];								// Array containing all the node objects.
		Mutex*					m_nodeMutex;								// Serializes access to node data

		ControllerReplication*	m_controllerReplication;					// Controller replication is handled separately from the other command classes, due to older hand-held controllers using invalid node IDs.

	//-----------------------------------------------------------------------------
	//	Sending Z-Wave messages
	//-----------------------------------------------------------------------------
	public:
		void SendMsg( Msg* _msg );

	private:
		/**
		 *  If there are messages in the send queue (m_sendQueue), gets the next message in the
		 *  queue and writes it to the serial port.  In sending the message, SendMsg also initializes
		 *  variables tracking the message's callback ID (m_expectedCallbackId), expected reply
		 *  (m_expectedReply) and expected command class ID (m_expectedCommandClassId).  It also
		 *  sets m_waitingForAck to true and increments the message's send attempts counter.
		 *  <p>
		 *  If there are no messages in the send queue, then SendMsg checks the query queue to
		 *  see if there are any outstanding queries that can be processed (target node not asleep).
		 *  If so, it retrieves the Node object that needs to be queried and calls that node's
		 *  AdvanceQueries member function.  If this call results in all of the node's queries to be
		 *  completed, SendMsg will remove the node query item from the query queue.
		 *  \return TRUE if data was written, FALSE if not
		 *  \see Msg, m_sendQueue, m_expectedCallbackId, m_expectedReply, m_expectedCommandClassId,
		 *  m_waitingForAck, Msg::GetSendAttempts, Node::AdvanceQueries, GetCurrentNodeQuery,
		 *  RemoveNodeQuery, Node::AllQueriesCompleted
		 */
		bool WriteMsg();
		void RemoveMsg();													// Remove the first message from the send queue.  This happens when the send was successful, or after three failed attempts.
		void TriggerResend();												// Causes the first message to be sent again, in response to a NAK or CAN from the controller.
		bool MoveMessagesToWakeUpQueue(	uint8 const _targetNodeId );		// If a node does not respond, and is of a type that can sleep, this method is used to move all its pending messages to another queue ready for when it mext wakes up.

		list<Msg*>				m_sendQueue;								// Messages waiting to be sent
		Mutex*					m_sendMutex;								// Serialize access to the send and wakeup queues

	//-----------------------------------------------------------------------------
	//	Receiving Z-Wave messages
	//-----------------------------------------------------------------------------
	private:
		bool ReadMsg();
		void ProcessMsg( uint8* _data );

		void HandleGetVersionResponse( uint8* _data );
		void HandleGetControllerCapabilitiesResponse( uint8* _data );
		void HandleGetSerialAPICapabilitiesResponse( uint8* _data );
		void HandleEnableSUCResponse( uint8* _data );
		void HandleSetSUCNodeIdResponse( uint8* _data );
		void HandleGetSUCNodeIdResponse( uint8* _data );
		void HandleMemoryGetIdResponse( uint8* _data );
		/**
		 *  Process a response to a FUNC_ID_SERIAL_API_GET_INIT_DATA request.
		 *  <p>
		 *  The response message contains a bitmap identifying which of the 232 possible nodes
		 *  in the network are actually present.  These bitmap values are compared with the
		 *  node map (read in from zwcfg_0x[homeid].xml) to see if the node has already been registered
		 *  by the OpenZWave library.  If it has (the log will show it as "Known") and this is 
		 *  the first time this message was sent (m_init is false), then AddNodeQuery() is called
		 *  to retrieve its current state.  If this is a "New" node to OpenZWave, then InitNode()
		 *  is called.
		 *  \see AddNodeQuery, InitNode, GetNode, ReleaseNodes
		 */
		void HandleSerialAPIGetInitDataResponse( uint8* _data );
		void HandleGetNodeProtocolInfoResponse( uint8* _data );
		bool HandleRemoveFailedNodeResponse( uint8* _data );
		void HandleIsFailedNodeResponse( uint8* _data );
		bool HandleReplaceFailedNodeResponse( uint8* _data );
		bool HandleAssignReturnRouteResponse( uint8* _data );
		bool HandleDeleteReturnRouteResponse( uint8* _data );
		void HandleSendDataResponse( uint8* _data, bool _replication );
		bool HandleNetworkUpdateResponse( uint8* _data );
		void HandleGetRoutingInfoResponse( uint8* _data );

		bool HandleSendDataRequest( uint8* _data, bool _replication );
		void HandleAddNodeToNetworkRequest( uint8* _data );
		void HandleCreateNewPrimaryRequest( uint8* _data );
		void HandleControllerChangeRequest( uint8* _data );
		void HandleSetLearnModeRequest( uint8* _data );
		void HandleRemoveFailedNodeRequest( uint8* _data );
		void HandleReplaceFailedNodeRequest( uint8* _data );
		void HandleRemoveNodeFromNetworkRequest( uint8* _data );
		void HandleApplicationCommandHandlerRequest( uint8* _data );
		void HandleAssignReturnRouteRequest( uint8* _data );
		void HandleDeleteReturnRouteRequest( uint8* _data );
		void HandleNodeNeighborUpdateRequest( uint8* _data );
		void HandleNetworkUpdateRequest( uint8* _data );
		bool HandleApplicationUpdateRequest( uint8* _data );

		void CommonAddNodeStatusRequestHandler( uint8 _funcId, uint8* _data );

		Thread*					m_readThread;								// Thread for handling messages received from the Z-Wave network
		bool					m_waitingForAck;							// True when we are waiting for an ACK from the dongle
		uint8					m_expectedCallbackId;						// If non-zero, we wait for a message with this callback Id
		uint8					m_expectedReply;							// If non-zero, we wait for a message with this function Id
		uint8					m_expectedCommandClassId;					// If the expected reply is FUNC_ID_APPLICATION_COMMAND_HANDLER, this value stores the command class we're waiting to hear from

	//-----------------------------------------------------------------------------
	//	Polling Z-Wave devices
	//-----------------------------------------------------------------------------
	private:
		void SetPollInterval( int32 _seconds ){ m_pollInterval = _seconds; }
		bool EnablePoll( uint8 _nodeId );
		bool DisablePoll( uint8 _nodeId );

		static void PollThreadEntryPoint( void* _context );
		void PollThreadProc();

		Thread*					m_pollThread;								// Thread for polling devices on the Z-Wave network
		list<uint8>				m_pollList;									// List of nodes that need to be polled
		Mutex*					m_pollMutex;								// Serialize access to the polling list
		int32					m_pollInterval;								// Time interval during which all nodes must be polled

	//-----------------------------------------------------------------------------
	//	Retrieving Node information
	//-----------------------------------------------------------------------------
	private:
		/**
		 *  Creates a new Node object (deleting any previous Node object with the same nodeId) and
		 *  queues a full query of the node's parameters (starting at the beginning of the query
		 *  stages--Node::QueryStage_None).  This function will send Notification::Type_NodeAdded
		 *  and Notification::Type_NodeRemoved messages to identify these modifications.
		 *  \param _nodeId The node ID of the node to create and query.
		 *  \see Notification::Type_NodeAdded, Notification::Type_NodeRemoved, Node::QueryStage_None, 
		 *  AddNodeQuery
		 */
		void InitNode( uint8 const _nodeId );
		/**
		 *  Adds an existing node to the query queue if it is not already there.
		 *  \param _nodeId The node ID of the node to query.
		 *  \param _stage The Node::QueryStage at which to start querying.  This allows OpenZWave to 
		 *  either continue an interrupted query at the required stage or to start the update at a
		 *  late stage (for example, to read dynamic data that will have changed since OpenZWave was
		 *  last run).
		 *  \see Node::QueryStage, Node::GoBackToQueryStage, m_nodeQueries
		 */
		void AddNodeQuery( uint8 const _nodeId, Node::QueryStage const _stage );
		/**
		 *  Removes the specified node from the node query queue.
		 *  \param _nodeId The node ID of the node to remove from the queue.
		 *  \see m_nodeQueries
		 */
		void RemoveNodeQuery( uint8 const _nodeId );
		/** 
		 *  Gets the "awake" node that is nearest the front of the queue of nodes to be queried.
		 *  This function iterates through the m_nodeQueries queue and will return the nodeId of the
		 *  first node it finds that is either always "listening" or isn't always listening but 
		 *  happens to be awake.
		 *  \return Node ID of a node in the query queue.
		 */
		uint8 GetCurrentNodeQuery();

		void InitAllNodes();												// Delete all nodes and fetch the data from the Z-Wave network again.
		void RequestNodeState( uint8 const _nodeId );
		
		bool IsNodeListeningDevice( uint8 const _nodeId );
		bool IsNodeRoutingDevice( uint8 const _nodeId );
		uint32 GetNodeMaxBaudRate( uint8 const _nodeId );
		uint8 GetNodeVersion( uint8 const _nodeId );
		uint8 GetNodeSecurity( uint8 const _nodeId );
		uint8 GetNodeBasic( uint8 const _nodeId );
		uint8 GetNodeGeneric( uint8 const _nodeId );
		uint8 GetNodeSpecific( uint8 const _nodeId );
		string GetNodeType( uint8 const _nodeId );

		string GetNodeManufacturerName( uint8 const _nodeId );
		string GetNodeProductName( uint8 const _nodeId );
		string GetNodeName( uint8 const _nodeId );
		string GetNodeLocation( uint8 const _nodeId );

		string GetNodeManufacturerId( uint8 const _nodeId );
		string GetNodeProductType( uint8 const _nodeId );
		string GetNodeProductId( uint8 const _nodeId );

		void SetNodeManufacturerName( uint8 const _nodeId, string const& _manufacturerName );
		void SetNodeProductName( uint8 const _nodeId, string const& _productName );
		void SetNodeName( uint8 const _nodeId, string const& _nodeName );
		void SetNodeLocation( uint8 const _nodeId, string const& _location );
		void SetNodeLevel( uint8 const _nodeId, uint8 const _level );
        void SetNodeOn( uint8 const _nodeId );
        void SetNodeOff( uint8 const _nodeId );

		Value* GetValue( ValueID const& _id );

		list<uint8>				m_nodeQueries;		/**< Queue of node IDs of nodes that we wish to interrogate for setup details */
		Mutex*					m_queryMutex;		/**< Serialize access to the info queue */

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	public:	
		/** 
		 * Controller Commands.
		 * Commands to be used with the BeginControllerCommand method.
		 * \see Manager::BeginControllerCommand
	     */
		enum ControllerCommand
		{
			ControllerCommand_None = 0,						/**< No command. */
			ControllerCommand_AddController,				/**< Add a new controller to the Z-Wave network.  The new controller will be a secondary. */
			ControllerCommand_AddDevice,					/**< Add a new device (but not a controller) to the Z-Wave network. */
			ControllerCommand_CreateNewPrimary,				/**< Add a new controller to the Z-Wave network.  The new controller will be the primary, and the current primary will become a secondary controller. */
			ControllerCommand_ReceiveConfiguration,			/**< Receive Z-Wave network configuration information from another controller. */
			ControllerCommand_RemoveController,				/**< Remove a controller from the Z-Wave network. */
			ControllerCommand_RemoveDevice,					/**< Remove a new device (but not a controller) from the Z-Wave network. */
			ControllerCommand_RemoveFailedNode,				/**< Move a node to the controller's failed nodes list. This command will only work if the node cannot respond. */
			ControllerCommand_HasNodeFailed,				/**< Check whether a node is in the controller's failed nodes list. */
			ControllerCommand_ReplaceFailedNode,			/**< Replace a non-responding node with another. The node must be in the controller's list of failed nodes for this command to succeed. */
			ControllerCommand_TransferPrimaryRole,			/**< Make a different controller the primary. */
			ControllerCommand_RequestNetworkUpdate,			/**< Request network information from the SUC/SIS. */
			ControllerCommand_RequestNodeNeighborUpdate,	/**< Get a node to rebuild it's neighbour list.  This method also does ControllerCommand_RequestNodeNeighbors */
			ControllerCommand_AssignReturnRoute,			/**< Assign a network return routes to a device. */
			ControllerCommand_DeleteAllReturnRoutes			/**< Delete all return routes from a device. */
		};

		/** 
		 * Controller States.
		 * States reported via the callback handler passed into the BeginControllerCommand method.
		 * \see Manager::BeginControllerCommand
	     */
		enum ControllerState
		{
			ControllerState_Normal = 0,				/**< No command in progress. */
			ControllerState_Waiting,				/**< Controller is waiting for a user action. */
			ControllerState_InProgress,				/**< The controller is communicating with the other device to carry out the command. */
			ControllerState_Completed,			    /**< The command has completed successfully. */
			ControllerState_Failed,					/**< The command has failed. */
			ControllerState_NodeOK,					/**< Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node is OK. */
			ControllerState_NodeFailed				/**< Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node has failed. */
		};

		typedef void (*pfnControllerCallback_t)( ControllerState _state, void* _context );

	private:
		// The public interface is provided via the wrappers in the Manager class
		void ResetController();
		void SoftReset();
		void RequestNodeNeighbors( uint8 const _nodeId );

		bool BeginControllerCommand( ControllerCommand _command, pfnControllerCallback_t _callback, void* _context, bool _highPower, uint8 _nodeId );
		bool CancelControllerCommand();

		ControllerState				m_controllerState;
		ControllerCommand			m_controllerCommand;
		pfnControllerCallback_t		m_controllerCallback;
		void*						m_controllerCallbackContext;
		bool						m_controllerAdded;
		uint8						m_controllerCommandNode;

	//-----------------------------------------------------------------------------
	// Configuration Parameters	(wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	private:		
		// The public interface is provided via the wrappers in the Manager class
		bool SetConfigParam( uint8 const _nodeId, uint8 const _param, int32 _value );
		void RequestConfigParam( uint8 const _nodeId, uint8 const _param );
		void RequestAllConfigParams( uint8 const _nodeId );

	//-----------------------------------------------------------------------------
	// Groups (wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	private:		
		// The public interface is provided via the wrappers in the Manager class
		uint8 GetNumGroups( uint8 const _nodeId );
		uint32 GetAssociations( uint8 const _nodeId, uint8 const _groupIdx, uint8** o_associations );
		uint8 GetMaxAssociations( uint8 const _nodeId, uint8 const _groupIdx );
		void AddAssociation( uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );
		void RemoveAssociation( uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );

	//-----------------------------------------------------------------------------
	//	Notifications
	//-----------------------------------------------------------------------------
	private:
		void QueueNotification( Notification* _notification );				// Adds a notification to the list.  Notifications are queued until a point in the thread where we know we do not have any nodes locked.
		void NotifyWatchers();												// Passes the notifications to all the registered watcher callbacks in turn.

		list<Notification*>	m_notifications;
	};

} // namespace OpenZWave

#endif // _Driver_H

