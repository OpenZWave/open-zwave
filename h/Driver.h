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

	class Driver
	{
		friend class Manager;

	//-----------------------------------------------------------------------------
	// Construction / Destruction
	//-----------------------------------------------------------------------------
	public:
		Driver( string const& _serialPortName );
		virtual ~Driver();

	private:
		void Start();
		static void DriverThreadEntryPoint( void* _context );
		void DriverThreadProc();
		bool Init( uint32 _attempts );

		Thread*					m_driverThread;	// Thread for creating and managing the driver worker threads
		Event*					m_exitEvent;		// Event that will be signalled when the threads should exit
		bool					m_exit;
		bool					m_init;

	//-----------------------------------------------------------------------------
	//	Configuration
	//-----------------------------------------------------------------------------
	private:
		void RequestConfig();	// Get the network configuration from the Z-Wave network
		bool ReadConfig();		// Read the configuration from a file
		void WriteConfig();		// Save the configuration to a file

	//-----------------------------------------------------------------------------
	//	Controller
	//-----------------------------------------------------------------------------
	public:
		bool IsSlave()const{ return ((m_capabilities & 0x01) != 0); }
		bool HasTimerSupport()const{ return ((m_capabilities & 0x02) != 0); }
		bool IsPrimaryController()const{ return ((m_capabilities & 0x04) == 0); }
		bool IsStaticUpdateController()const{ return ((m_capabilities & 0x08) != 0); }

		Node* GetNode( uint8 _nodeId ){ return m_nodes[_nodeId]; }
		uint32 GetHomeId()const{ return m_homeId; }
		string GetSerialPortName()const{ return m_serialPortName; }

	private:
		string					m_serialPortName;	// name used to open the serial port
		uint32					m_homeId;
		SerialPort*				m_serialPort;
		Mutex*					m_serialMutex;
		
		uint8					m_capabilities;	
		uint8					m_nodeId;			// PC Controller's Z-Wave node ID
		Node*					m_nodes[256];		// Z-Wave node details	

	//-----------------------------------------------------------------------------
	//	Sending Z-Wave messages
	//-----------------------------------------------------------------------------
	public:
		void SendMsg( Msg* _msg );

	private:
		static void SendThreadEntryPoint( void* _context );
		void SendThreadProc();

		void RemoveMsg();
		void TriggerResend();
		bool MoveMessagesToWakeUpQueue(	uint8 const _targetNodeId );
		void SetNodeAwake( uint8 const _nodeId );

		Thread*					m_sendThread;		// Thread for sending messages to the Z-Wave network	
		list<Msg*>				m_sendQueue;		// Messages waiting to be sent
		Mutex*					m_sendMutex;		// Serialize access to the send and wakeup queues
		Event*					m_sendEvent;		// Signalled when there is something waiting to be sent

	//-----------------------------------------------------------------------------
	//	Receiving Z-Wave messages
	//-----------------------------------------------------------------------------
	private:
		static void ReadThreadEntryPoint( void* _context );		
		void ReadThreadproc();

		bool ReadMsg();
		void ProcessMsg( uint8* _data );

		void HandleGetCapabilitiesResponse( uint8* pData );
		void HandleEnableSUCResponse( uint8* pData );
		void HandleRequestNetworkUpdate( uint8* pData );
		void HandleControllerChange( uint8* pData );
		void HandleSetSUCNodeIdResponse( uint8* pData );
		void HandleGetSUCNodeIdResponse( uint8* pData );
		void HandleMemoryGetIdResponse( uint8* pData );
		void HandleSerialAPIGetInitDataResponse( uint8* pData );
		void HandleGetNodeProtocolInfoResponse( uint8* pData );
		void HandleSendDataResponse( uint8* pData );
		bool HandleSendDataRequest( uint8* pData );
		void HandleAddNodeToNetworkRequest( uint8* pData );
		void HandleRemoveNodeFromNetworkRequest( uint8* pData );
		void HandleApplicationCommandHandlerRequest( uint8* pData );
		bool HandleApplicationUpdateRequest( uint8* pData );

		Thread*					m_readThread;				// Thread for handling messages received from the Z-Wave network
		bool					m_waitingForAck;			// True when we are waiting for an ACK from the dongle
		uint8					m_expectedCallbackId;		// If non-zero, we wait for a message with this callback Id
		uint8					m_expectedReply;			// If non-zero, we wait for a message with this function Id
		uint8					m_expectedCommandClassId;	// If the expected reply is FUNC_ID_APPLICATION_COMMAND_HANDLER, this value stores the command class we're waiting to hear from

	//-----------------------------------------------------------------------------
	//	Polling Z-Wave devices
	//-----------------------------------------------------------------------------
	public:
		void SetPollInterval( int32 _seconds ){ m_pollInterval = _seconds; }
		bool EnablePoll( uint8 _nodeId );
		bool DisablePoll( uint8 _nodeId );

	private:
		static void PollThreadEntryPoint( void* _context );
		void PollThreadProc();

		Thread*					m_pollThread;		// Thread for polling devices on the Z-Wave network
		list<uint8>				m_pollList;			// List of nodes that need to be polled
		Mutex*					m_pollMutex;		// Serialize access to the polling list
		int32					m_pollInterval;		// Time interval during which all nodes must be polled

	//-----------------------------------------------------------------------------
	//	Retrieving Node information
	//-----------------------------------------------------------------------------
	public:
		void AddInfoRequest( uint8 const _nodeId );
		void RemoveInfoRequest();
		void RequestState( uint8 const _nodeId, uint32 const _flags );
		string GetBasicLabel( uint8 const _nodeId );
		string GetGenericLabel( uint8 const _nodeId );

		// Helpers for fetching values
		ValueBool* GetValueBool( ValueID const& _id );
		ValueByte* GetValueByte( ValueID const& _id );
		ValueDecimal* GetValueDecimal( ValueID const& _id );
		ValueInt* GetValueInt( ValueID const& _id );
		ValueList* GetValueList( ValueID const& _id );
		ValueShort* GetValueShort( ValueID const& _id );
		ValueString* GetValueString( ValueID const& _id );

	private:
		uint8 GetInfoRequest();

		deque<uint8>			m_infoQueue;		// Queue holding nodes that we wish to interogate for setup details
		Mutex*					m_infoMutex;		// Serialize access to the info queue				

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	public:	
		void ResetController();
		void SoftReset();

		void RequestNodeNeighborUpdate( uint8 _nodeId );
		void AssignReturnRoute( uint8 _srcNodeId, uint8 _dstNodeId );
		
		void BeginAddNode( bool _bHighpower = false );
		void BeginAddController( bool _bHighpower = false );
		void EndAddNode();
		
		void BeginRemoveNode();
		void EndRemoveNode();

		void BeginReplicateController();
		void EndReplicateController();

		void RequestNetworkUpdate();
		void ControllerChange();

	//-----------------------------------------------------------------------------
	//	Misc
	//-----------------------------------------------------------------------------
	private:
		void UpdateEvents();						// Set and Reset events according to the states of various queues and flags
	};

} // namespace OpenZWave

#endif // _Driver_H

