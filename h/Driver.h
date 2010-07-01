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
#include <deque>

#include "Defs.h"
#include "ValueID.h"

namespace OpenZWave
{
	class Node;
	class Msg;
	class Value;
	class Event;
	class Mutex;
	class SerialPort;
	class Thread;
	class ValueBool;
	class ValueByte;
	class ValueDecimal;
	class ValueInt;
	class ValueList;
	class ValueShort;
	class ValueString;
	class ControllerReplication;
	class Notification;

	class Driver
	{
		friend class Manager;
		friend class Node;
		friend class Group;
		friend class CommandClass;
		friend class Value;
		friend class ValueStore;
		friend class ManufacturerSpecific;
		friend class WakeUp;

	//-----------------------------------------------------------------------------
	// Construction / Destruction
	//-----------------------------------------------------------------------------
	private:
		Driver( string const& _serialPortName );
		virtual ~Driver();

		void Start();
		static void DriverThreadEntryPoint( void* _context );
		void DriverThreadProc();
		bool Init( uint32 _attempts );

		Thread*					m_driverThread;								// Thread for reading from the Z-Wave controller, and for creating and managing the other threads for sending, polling etc.
		Event*					m_exitEvent;								// Event that will be signalled when the threads should exit
		bool					m_exit;										// Flag that is set when the application is exiting.
		bool					m_init;										// Set to true once the driver has been initialised

	//-----------------------------------------------------------------------------
	//	Configuration
	//-----------------------------------------------------------------------------
	private:
		void RequestConfig();												// Get the network configuration from the Z-Wave network
		bool ReadConfig();													// Read the configuration from a file
		void WriteConfig();													// Save the configuration to a file

	//-----------------------------------------------------------------------------
	//	Controller
	//-----------------------------------------------------------------------------
	private:
		bool IsPrimaryController()const{ return ((m_capabilities & 0x04) == 0); }
		bool IsStaticUpdateController()const{ return ((m_capabilities & 0x08) != 0); }
		uint32 GetHomeId()const{ return m_homeId; }
		uint8 GetNodeId()const{ return m_nodeId; }
		string GetSerialPortName()const{ return m_serialPortName; }
		Node* GetNode( uint8 _nodeId );
		void LockNodes();
		void ReleaseNodes();

		string					m_serialPortName;							// name used to open the serial port.
		uint32					m_homeId;									// Home ID of the Z-Wave controller.  Not valid until the DriverReady notification has been received.
		SerialPort*				m_serialPort;								// Handles communications with the controller hardware.
		Mutex*					m_serialMutex;								// Ensure only one thread at a time can access the serial port.
		
		uint8					m_capabilities;								// Set of flags indicating the controller's capabilities (See IsSlave, HasTimerSupport, IsPrimaryController and IsStaticUpdateController above).
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
		static void SendThreadEntryPoint( void* _context );					// Static method called by the m_sendThread object as the entry point for its thread code.  The _context will contain a pointer to this Driver object.
		void SendThreadProc();												// Implementation of the send thread, called from the static SendThreadEntryPoint.

		void RemoveMsg();													// Remove the first message from the send queue.  This happens when the send was successful, or after three failed attempts.
		void TriggerResend();												// Causes the first message to be sent again, in response to a NAK or CAN from the controller.
		bool MoveMessagesToWakeUpQueue(	uint8 const _targetNodeId );		// If a node does not respond, and is of a type that can sleep, this method is used to move all its pending messages to another queue ready for when it mext wakes up.
		void SetNodeAwake( uint8 const _nodeId );							// Used to mark a node as awake when we receive a message from it.

		Thread*					m_sendThread;								// Thread for sending messages to the Z-Wave network	
		list<Msg*>				m_sendQueue;								// Messages waiting to be sent
		Mutex*					m_sendMutex;								// Serialize access to the send and wakeup queues
		Event*					m_sendEvent;								// Signalled when there is something waiting to be sent

	//-----------------------------------------------------------------------------
	//	Receiving Z-Wave messages
	//-----------------------------------------------------------------------------
	private:
		bool ReadMsg();
		void ProcessMsg( uint8* _data );

		void HandleGetCapabilitiesResponse( uint8* pData );
		void HandleEnableSUCResponse( uint8* pData );
		void HandleRequestNetworkUpdate( uint8* pData );
		void HandleSetSUCNodeIdResponse( uint8* pData );
		void HandleGetSUCNodeIdResponse( uint8* pData );
		void HandleMemoryGetIdResponse( uint8* pData );
		void HandleSerialAPIGetInitDataResponse( uint8* pData );
		void HandleGetNodeProtocolInfoResponse( uint8* pData );
		void HandleSendDataResponse( uint8* pData );
		bool HandleSendDataRequest( uint8* pData );
		void HandleAddNodeToNetworkRequest( uint8* pData );
		void HandleCreateNewPrimary( uint8* pData );
		void HandleControllerChange( uint8* pData );
		void HandleSetLearnMode( uint8* pData );
		void HandleRemoveNodeFromNetworkRequest( uint8* pData );
		void HandleApplicationCommandHandlerRequest( uint8* pData );
		bool HandleApplicationUpdateRequest( uint8* pData );

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
		void AddNodeInfoRequest( uint8 const _nodeId );
		void RemoveNodeInfoRequest();
		void RequestNodeState( uint8 const _nodeId, uint32 const _flags );
		
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

		Value* GetValue( ValueID const& _id );

		void RefreshNodeInfo();												// Delete all nodes and fetch the data from the Z-Wave network again.
		uint8 GetNodeInfoRequest();

		deque<uint8>			m_infoQueue;								// Queue holding nodes that we wish to interogate for setup details
		Mutex*					m_infoMutex;								// Serialize access to the info queue				

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	public:	
		/** 
		 * Controller Commands.
		 * Commands to be used with the BeginControllerCommand method.
		 * @see Manager::BeginControllerCommand
	     */
		enum ControllerCommand
		{
			ControllerCommand_None = 0,				/**< No command. */
			ControllerCommand_AddController,		/**< Add a new controller to the Z-Wave network.  The new controller will be a secondary. */
			ControllerCommand_AddDevice,			/**< Add a new device (but not a controller) to the Z-Wave network. */
			ControllerCommand_CreateNewPrimary,		/**< Add a new controller to the Z-Wave network.  The new controller will be the primary, and the current primary will become a secondary controller. */
			ControllerCommand_ReceiveConfiguration, /**< Receive Z-Wave network configuration information from another controller. */
			ControllerCommand_RemoveController,		/**< Remove a controller from the Z-Wave network. */
			ControllerCommand_RemoveDevice,			/**< Remove a new device (but not a controller) from the Z-Wave network. */
			ControllerCommand_ReplaceFailedDevice,	/**< Replace a non-responding device with another. */
			ControllerCommand_TransferPrimaryRole	/**< Make a different controller the primary. */
		};

		/** 
		 * Controller States.
		 * States reported via the callback handler passed into the BeginControllerCommand method.
		 * @see Manager::BeginControllerCommand
	     */
		enum ControllerState
		{
			ControllerState_Normal = 0,				/**< No command in progress. */
			ControllerState_Waiting,				/**< Controller is waiting for a user action. */
			ControllerState_InProgress,				/**< The controller is communicating with the other device to carry out the command. */
			ControllerState_Completed,			    /**< The command has completed successfully. */
			ControllerState_Failed					/**< The command has failed. */
		};

		typedef void (*pfnControllerCallback_t)( ControllerState _state, void* _context );

	private:
		// The public interface is provided via the wrappers in the Manager class
		void ResetController();
		void SoftReset();

		void RequestNodeNeighborUpdate( uint8 _nodeId );
		void AssignReturnRoute( uint8 _srcNodeId, uint8 _dstNodeId );
		void RequestNetworkUpdate();

		bool BeginControllerCommand( ControllerCommand _command, pfnControllerCallback_t _callback, void* _context, bool _highPower );
		bool CancelControllerCommand();

		ControllerState				m_controllerState;
		ControllerCommand			m_controllerCommand;
		pfnControllerCallback_t		m_controllerCallback;
		void*						m_controllerCallbackContext;
		bool						m_controllerAdded;
		uint8						m_nodeAdded;

	//-----------------------------------------------------------------------------
	// Configuration Parameters	(wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	private:		
		// The public interface is provided via the wrappers in the Manager class
		bool SetConfigParam( uint8 const _nodeId, uint8 const _param, int32 _value );
		void RequestConfigParam( uint8 const _nodeId, uint8 const _param );

	//-----------------------------------------------------------------------------
	// Groups (wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	private:		
		// The public interface is provided via the wrappers in the Manager class
		uint8 GetNumGroups( uint8 const _nodeId );
		uint32 GetAssociations( uint8 const _nodeId, uint8 const _groupIdx, uint8** o_associations );
		void AddAssociation( uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );
		void RemoveAssociation( uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );

	//-----------------------------------------------------------------------------
	//	Notifications
	//-----------------------------------------------------------------------------
	private:
		void QueueNotification( Notification* _notification );				// Adds a notification to the list.  Notifications are queued until a point in the thread where we know we do not have any nodes locked.
		void NotifyWatchers();												// Passes the notifications to all the registered watcher callbacks in turn.

		list<Notification*>	m_notifications;

	//-----------------------------------------------------------------------------
	//	Misc
	//-----------------------------------------------------------------------------
	private:
		void UpdateEvents();												// Set and Reset events according to the states of various queues and flags
	};

} // namespace OpenZWave

#endif // _Driver_H

