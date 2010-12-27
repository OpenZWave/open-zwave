//-----------------------------------------------------------------------------
//
//      ZWManager.h
//
//      Cli/C++ wrapper for the C++ OpenZWave Manager class
//
//      Copyright (c) 2010 Amer Harb <harb_amer@hotmail.com>
//
//      SOFTWARE NOTICE AND LICENSE
//
//      This file is part of OpenZWave.
//
//      OpenZWave is free software: you can redistribute it and/or modify
//      it under the terms of the GNU Lesser General Public License as published
//      by the Free Software Foundation, either version 3 of the License,
//      or (at your option) any later version.
//
//      OpenZWave is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public License
//      along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include <stdio.h>
#include <msclr/auto_gcroot.h>
#include <msclr/lock.h>

#include "ZWValueID.h"
#include "ZWNotification.h"

#include "Manager.h"
#include "ValueID.h"
#include "Notification.h"
#include "Driver.h"

using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace Runtime::InteropServices;
using namespace OpenZWave;


namespace OpenZWaveDotNet
{
	// Delegate for handling notification callbacks
	public delegate void ManagedNotificationsHandler(ZWNotification^ notification);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	private delegate void OnNotificationFromUnmanagedDelegate(Notification* _notification, void* _context);

	// Delegate for handling controller command callbacks
	public enum class ZWControllerState
	{
		Normal		= Driver::ControllerState_Normal,								/**< No command in progress. */
		Waiting		= Driver::ControllerState_Waiting,								/**< Controller is waiting for a user action. */
		InProgress	= Driver::ControllerState_InProgress,							/**< The controller is communicating with the other device to carry out the command. */
		Completed	= Driver::ControllerState_Completed,							/**< The command has completed successfully. */
		Failed		= Driver::ControllerState_Failed,								/**< The command has failed. */
		NodeOK		= Driver::ControllerState_NodeOK,								/**< Used with the HasNodeFailed, RemoveFailedNode and ReplaceFailedNode commands to indicate that the controller thinks the node is OK. */
		NodeFailed	= Driver::ControllerState_NodeFailed							/**< Used only with HasNodeFailed to indicate that the controller thinks the node has failed. */
	};

	public enum class ZWControllerCommand
	{
		None						= Driver::ControllerCommand_None,						/**< No command. */
		AddController				= Driver::ControllerCommand_AddController,				/**< Add a new controller to the Z-Wave network.  The new controller will be a secondary. */
		AddDevice					= Driver::ControllerCommand_AddDevice,					/**< Add a new device (but not a controller) to the Z-Wave network. */
		CreateNewPrimary			= Driver::ControllerCommand_CreateNewPrimary,			/**< Add a new controller to the Z-Wave network.  The new controller will be the primary, and the current primary will become a secondary controller. */
		ReceiveConfiguration		= Driver::ControllerCommand_ReceiveConfiguration,		/**< Receive Z-Wave network configuration information from another controller. */
		RemoveController			= Driver::ControllerCommand_RemoveController,			/**< Remove a controller from the Z-Wave network. */
		RemoveDevice				= Driver::ControllerCommand_RemoveDevice,				/**< Remove a new device (but not a controller) from the Z-Wave network. */
		RemoveFailedNode			= Driver::ControllerCommand_RemoveFailedNode,			/**< Move a node to the controller's failed nodes list. This command will only work if the node cannot respond. */
		HasNodeFailed				= Driver::ControllerCommand_HasNodeFailed,				/**< Check whether a node is in the controller's failed nodes list. */
		ReplaceFailedNode			= Driver::ControllerCommand_ReplaceFailedNode,			/**< Replace a non-responding device with another. */
		TransferPrimaryRole			= Driver::ControllerCommand_TransferPrimaryRole,		/**< Make a different controller the primary. */
		RequestNetworkUpdate		= Driver::ControllerCommand_RequestNetworkUpdate,		/**< Request network information from the SUC/SIS. */
		RequestNodeNeighborUpdate	= Driver::ControllerCommand_RequestNodeNeighborUpdate,	/**< Get a node to rebuild it's neighbour list.  This method also does ControllerCommand_RequestNodeNeighbors */
		AssignReturnRoute			= Driver::ControllerCommand_AssignReturnRoute,			/**< Assign a network return route to a device. */
		DeleteAllReturnRoutes		= Driver::ControllerCommand_DeleteAllReturnRoutes		/**< Delete all network return routes from a device. */
	};

	public delegate void ManagedControllerStateChangedHandler( ZWControllerState _state);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	private delegate void OnControllerStateChangedFromUnmanagedDelegate(Driver::ControllerState _state, void* _context);

	public ref class ZWManager
	{
	//-----------------------------------------------------------------------------
	// Events
	//-----------------------------------------------------------------------------
	private:
		ManagedNotificationsHandler^ m_notificationEvent;
		event ManagedNotificationsHandler^ ZWOnNotification
		{
			void add( ManagedNotificationsHandler ^ d )
			{ 
				m_notificationEvent += d;
			} 
			
			void remove(ManagedNotificationsHandler ^ d)
			{ 
				m_notificationEvent -= d;
			} 
			
			void raise(ZWNotification^ notification)
			{ 
				ManagedNotificationsHandler^ tmp = m_notificationEvent; 
				if (tmp)
				{ 
					tmp->Invoke( notification );
				} 
			} 
		}

	public:
		property ManagedNotificationsHandler^ OnNotification
		{
			ManagedNotificationsHandler^ get()
			{
				return m_notificationEvent;
			}
			void set( ManagedNotificationsHandler^ value )
			{
				m_notificationEvent = value;
			}
		}

	private:
		ManagedControllerStateChangedHandler^ m_controllerStateChangedEvent;
		event ManagedControllerStateChangedHandler^ ZWOnControllerStateChanged
		{
			void add( ManagedControllerStateChangedHandler ^ d )
			{ 
				m_controllerStateChangedEvent += d;
			} 
			
			void remove(ManagedControllerStateChangedHandler ^ d)
			{ 
				m_controllerStateChangedEvent -= d;
			} 
			
			void raise(ZWControllerState state)
			{ 
				ManagedControllerStateChangedHandler^ tmp = m_controllerStateChangedEvent; 
				if (tmp)
				{ 
					tmp->Invoke( state );
				} 
			} 
		} 

		ManagedControllerStateChangedHandler^ m_onControllerStateChanged;

	public:
		property ManagedControllerStateChangedHandler^ OnControllerStateChanged
		{
			ManagedControllerStateChangedHandler^ get()
			{
				return m_controllerStateChangedEvent;
			}
			void set( ManagedControllerStateChangedHandler^ value )
			{
				m_controllerStateChangedEvent = value;
			}
		}

	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	/*@{*/
	public:
   		/**
		 * Creates the Manager singleton object.  
		 * The Manager provides the public interface to OpenZWave, exposing all the functionality required to add Z-Wave support to an application.
		 * There can be only one Manager in an OpenZWave application.  Once the Manager has been created, call AddWatcher to install a notification
		 * callback handler, and then call the AddDriver method for each attached PC Z-Wave controller in turn.
		 * \param _configPath a string containing the path to the OpenZWave library config folder, which contains XML descriptions of Z-Wave manufacturers and products.
		 * \param _userPath a string containing the path to the application's user data folder where the OpenZWave should store the Z-Wave network configuration and state.
		 * \return a pointer to the newly created Manager object.
		 * \see Destroy, AddWatcher, AddDriver
		 */
		void Create();

		/**
		 * Deletes the Manager and cleans up any associated objects.  
		 * \see Create, Get
		 */
		void Destroy(){ Manager::Get()->Destroy(); }
	/*@}*/					   

	//-----------------------------------------------------------------------------
	// Configuration
	//-----------------------------------------------------------------------------
	/*@{*/
	public:
		/**
		 * Saves the configuration of a PC Controller's Z-Wave network to the application's user data folder.
		 * This method does not normally need to be called, since OpenZWave will save the state automatically
		 * during the shutdown process.  It is provided here only as an aid to development.
		 * The configuration of each PC Controller's Z-Wave network is stored in a separate file.  The filename 
		 * consists of the 8 digit hexadecimal version of the controller's Home ID, prefixed with the string 'zwcfg_'.
		 * This convention allows OpenZWave to find the correct configuration file for a controller, even if it is
		 * attached to a different serial port.
		 * \param homeId The Home ID of the Z-Wave controller to save.
		 */
		void WriteConfig(uint32 homeId){ Manager::Get()->WriteConfig(homeId); }

	//-----------------------------------------------------------------------------
	//	Drivers
	//-----------------------------------------------------------------------------
	public:
		/**
		 * Creates a new driver for a Z-Wave controller.
		 * This method creates a Driver object for handling communications with a single Z-Wave controller.  In the background, the  
		 * driver first tries to read configuration data saved during a previous run.  It then queries the controller directly for any
		 * missing information, and a refresh of the list of nodes that it controls.  Once this information
		 * has been received, a DriverReady notification callback is sent, containing the Home ID of the controller.  This Home ID is
		 * required by most of the OpenZWave Manager class methods.
		 * \param _serialPortName The string used to open the serial port, for example "\\.\COM3".
		 * \return True if a new driver was created, false if a driver for the controller already exists.
		 * \see Create, Get, RemoveDriver
		 */
		bool AddDriver( String^ serialPortName ){ return Manager::Get()->AddDriver((const char*)(Marshal::StringToHGlobalAnsi(serialPortName)).ToPointer()); }

		/**
		 * Removes the driver for a Z-Wave controller, and closes the serial port.
		 * Drivers do not need to be explicitly removed before calling Destroy - this is handled automatically.
		 * @paaram _serialPortName The same string as was passed in the original call to AddDriver.
		 * \returns True if the driver was removed, false if it could not be found.
		 * \see Destroy, AddDriver
		 */
		bool RemoveDriver( String^ serialPortName ){ return Manager::Get()->RemoveDriver((const char*)(Marshal::StringToHGlobalAnsi(serialPortName)).ToPointer()); }

		/**
		 * Query if the controller is a primary controller.
		 * The primary controller is the main device used to configure and control a Z-Wave network.
		 * There can only be one primary controller - all other controllers are secondary controllers.
		 * <p> 
		 * The only difference between a primary and secondary controller is that the primary is the
		 * only one that can be used to add or remove other devices.  For this reason, it is usually
		 * better for the promary controller to be portable, since most devices must be added when
		 * installed in their final location.
		 * <p>
		 * Calls to BeginControllerCommand will fail if the controller is not the primary.
		 * \param homeId The Home ID of the Z-Wave controller.
		 * \return true if it is a primary controller, false if not.
		 */
		bool IsPrimaryController( uint32 homeId ){ return Manager::Get()->IsPrimaryController(homeId); }

		/**
		 * Query if the controller is a static update controller.
		 * A Static Update Controller (SUC) is a controller that must never be moved in normal operation
		 * and which can be used by other nodes to receive information about network changes.
		 * \param homeId The Home ID of the Z-Wave controller.
		 * \return true if it is a static update controller, false if not.
		 */
		bool IsStaticUpdateController( uint32 homeId ){ return Manager::Get()->IsStaticUpdateController(homeId); }

		/**
		 * Query if the controller is using the bridge controller library.
		 * A bridge controller is able to create virtual nodes that can be associated
		 * with other controllers to enable events to be passed on.
		 * \param homeId The Home ID of the Z-Wave controller.
		 * \return true if it is a bridge controller, false if not.
		 */
		bool IsBridgeController( uint32 const homeId ){ return Manager::Get()->IsBridgeController(homeId); }

		/**
		 * Get the version of the Z-Wave API library used by a controller.
		 * \param homeId The Home ID of the Z-Wave controller.
		 * \return a string containing the library version. For example, "Z-Wave 2.48".
		 */
		String^ GetLibraryVersion( uint32 const homeId ){ return gcnew String(Manager::Get()->GetLibraryVersion(homeId).c_str()); }

		/**
		 * Get a string containing the Z-Wave API library type used by a controller.
		 * The possible library types are:
		 * - Static Controller
		 * - Controller
		 * - Enhanced Slave
		 * - Slave            
	     * - Installer
	     * - Routing Slave
	     * - Bridge Controller
		 * - Device Under Test
		 * The controller should never return a slave library type.
		 * For a more efficient test of whether a controller is a Bridge Controller, use
		 * the IsBridgeController method.
		 * \param homeId The Home ID of the Z-Wave controller.
		 * \return a string containing the library type.
		 * \see GetLibraryVersion, IsBridgeController
		 */
		String^ GetLibraryTypeName( uint32 const homeId ){ return gcnew String(Manager::Get()->GetLibraryTypeName(homeId).c_str()); }

	//-----------------------------------------------------------------------------
	//	Polling Z-Wave devices
	//-----------------------------------------------------------------------------
	public:
		/**
		 * Set the time period between polls of a node's state.
		 * Due to patent concerns, some devices do not report state changes automatically to the controller.
		 * These devices need to have their state polled at regular intervals.  The length of the interval
		 * is the same for all devices.  To even out the Z-Wave network traffic generated by polling, OpenZWave
		 * divides the polling interval by the number of devices that have polling enabled, and polls each
		 * in turn.  It is recommended that if possible, the interval should not be set shorter than the
		 * number of polled devices in seconds (so that the network does not have to cope with more than one
		 * poll per second).
		 * \param _seconds The length of the polling interval in seconds.
		 */
		void SetPollInterval( int32 seconds ){ Manager::Get()->SetPollInterval(seconds); }

		/**
		 * Enable the polling of a device's state.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to start polling.
		 * \return true if polling was enabled.
		 */
		bool EnablePoll( uint32 homeId, uint8 nodeId ){ return Manager::Get()->EnablePoll(homeId,nodeId); }

		/**
		 * Disable the polling of a device's state.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to stop polling.
		 * \return true if polling was disabled.
		 */
		bool DisablePoll( uint32 homeId, uint8 nodeId ){ return Manager::Get()->DisablePoll(homeId,nodeId); }
	/*@}*/

	//-----------------------------------------------------------------------------
	//	Node information
	//-----------------------------------------------------------------------------
	/*@{*/
	public:
		/**
		 * Trigger the fetching of fixed data about a node.
		 * Causes the node's data to be obtained from the Z-Wave network in the same way as if it had just been added.
		 * This method would normally be called automatically by OpenZWave, but if you know that a node has been
		 * changed, calling this method will force a refresh of the data held by the library.  This can be especially 
		 * useful for devices that were asleep when the application was first run.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the request was sent successfully.
		 */
		bool RefreshNodeInfo( uint32 homeId, uint8 nodeId ){ return Manager::Get()->RefreshNodeInfo(homeId,nodeId); }
 		
		/**
		 * Trigger the fetching of session and dynamic value data for a node.
		 * Causes the node's values to be requested from the Z-Wave network.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the request was sent successfully.
		 */
		void RequestNodeState( uint32 homeId, uint8 nodeId ){ Manager::Get()->RequestNodeState(homeId,nodeId); }

		/**
		 * Get a human-readable label describing the node
		 * The label is taken from the Z-Wave specific, generic or basic type, depending on which of those values are specified by the node.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the label text.
		 */
		String^ GetNodeType( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeType(homeId,nodeId).c_str()); }

		/**
		 * Get the manufacturer name of a device
		 * The manufacturer name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the manufacturer ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's manufacturer name.
		 * \see SetNodeManufacturerName, GetNodeProductName, SetNodeProductName
		 */
		String^ GetNodeManufacturerName( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeManufacturerName(homeId,nodeId).c_str()); }

		/**
		 * Get the product name of a device
		 * The product name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the product Type and ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's product name.
		 * \see SetNodeProductName, GetNodeManufacturerName, SetNodeManufacturerName
		 */
		String^ GetNodeProductName( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeProductName(homeId,nodeId).c_str()); }

		/**
		 * Get the name of a node
		 * The node name is a user-editable label for the node that would normally be handled by the
		 * Node Naming commmand class, but many devices do not support it.  So that a node can always
		 * be named, OpenZWave stores it with the node data, and provides access through this method
		 * and SetNodeName, rather than reporting it via a command class Value object.
		 * The maximum length of a node name is 16 characters.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's name.
		 * \see SetNodeName, GetNodeLocation, SetNodeLocation
		 */
		String^ GetNodeName( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeName(homeId,nodeId).c_str()); }

		/**
		 * Get the location of a node
		 * The node location is a user-editable string that would normally be handled by the Node Naming
		 * commmand class, but many devices do not support it.  So that a node can always report its
		 * location, OpenZWave stores it with the node data, and provides access through this method
		 * and SetNodeLocation, rather than reporting it via a command class Value object.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's location.
		 * \see SetNodeLocation, GetNodeName, SetNodeName
		 */
		String^ GetNodeLocation( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeLocation(homeId,nodeId).c_str()); }

		/**
		 * Get the manufacturer ID of a device
		 * The manufacturer ID is a four digit hex code and would normally be handled by the Manufacturer
		 * Specific commmand class, but not all devices support it.  Although the value reported by this
		 * method will be an empty string if the command class is not supported and cannot be set by the 
		 * user, the manufacturer ID is still stored with the node data (rather than being reported via a
		 * command class Value object) to retain a consistent approach with the other manufacturer specific data.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's manufacturer ID, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * \see GetNodeProductType, GetNodeProductId, GetNodeManufacturerName, GetNodeProductName
		 */
		String^ GetNodeManufacturerId( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeManufacturerId(homeId,nodeId).c_str()); }

		/**
		 * Get the product type of a device
		 * The product type is a four digit hex code and would normally be handled by the Manufacturer Specific
		 * commmand class, but not all devices support it.  Although the value reported by this method will
		 * be an empty string if the command class is not supported and cannot be set by the user, the product
		 * type is still stored with the node data (rather than being reported via a command class Value object)
		 * to retain a consistent approach with the other manufacturer specific data.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's product type, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * \see GetNodeManufacturerId, GetNodeProductId, GetNodeManufacturerName, GetNodeProductName
		 */
		String^ GetNodeProductType( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeProductType(homeId,nodeId).c_str()); }

		/**
		 * Get the product ID of a device
		 * The product ID is a four digit hex code and would normally be handled by the Manufacturer Specific
		 * commmand class, but not all devices support it.  Although the value reported by this method will
		 * be an empty string if the command class is not supported and cannot be set by the user, the product
		 * ID is still stored with the node data (rather than being reported via a command class Value object)
		 * to retain a consistent approach with the other manufacturer specific data.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's product ID, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * \see GetNodeManufacturerId, GetNodeProductType, GetNodeManufacturerName, GetNodeProductName
		 */
		String^ GetNodeProductId( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeProductId(homeId,nodeId).c_str()); }

		/**
		 * Set the manufacturer name of a device
		 * The manufacturer name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the manufacturer ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _manufacturerName	A string containing the node's manufacturer name.
		 * \see GetNodeManufacturerName, GetNodeProductName, SetNodeProductName
		 */
		void SetNodeManufacturerName( uint32 homeId, uint8 nodeId, String^ _manufacturerName ){ Manager::Get()->SetNodeManufacturerName( homeId, nodeId, (const char*)(Marshal::StringToHGlobalAnsi(_manufacturerName)).ToPointer()); }
		
		/**
		 * Set the product name of a device
		 * The product name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the product Type and ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _productName A string containing the node's product name.
		 * \see GetNodeProductName, GetNodeManufacturerName, SetNodeManufacturerName
		 */
		void SetNodeProductName( uint32 homeId, uint8 nodeId, String^ _productName ){ Manager::Get()->SetNodeProductName( homeId, nodeId, (const char*)(Marshal::StringToHGlobalAnsi(_productName)).ToPointer()); }

		/**
		 * Set the name of a node
		 * The node name is a user-editable label for the node that would normally be handled by the
		 * Node Naming commmand class, but many devices do not support it.  So that a node can always
		 * be named, OpenZWave stores it with the node data, and provides access through this method
		 * and GetNodeName, rather than reporting it via a command class Value object.
		 * If the device does support the Node Naming command class, the new name will be sent to the node.
		 * The maximum length of a node name is 16 characters.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _nodeName A string containing the node's name.
		 * \see GetNodeName, GetNodeLocation, SetNodeLocation
		 */
		void SetNodeName( uint32 homeId, uint8 nodeId, String^ _nodeName ){ Manager::Get()->SetNodeName( homeId, nodeId, (const char*)(Marshal::StringToHGlobalAnsi(_nodeName)).ToPointer()); }

		/**
		 * Set the location of a node
		 * The node location is a user-editable string that would normally be handled by the Node Naming
		 * commmand class, but many devices do not support it.  So that a node can always report its
		 * location, OpenZWave stores it with the node data, and provides access through this method
		 * and GetNodeLocation, rather than reporting it via a command class Value object.
		 * If the device does support the Node Naming command class, the new location will be sent to the node.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _location A string containing the node's location.
		 * \see GetNodeLocation, GetNodeName, SetNodeName
		 */
		void SetNodeLocation( uint32 homeId, uint8 nodeId, String^ _location ){ Manager::Get()->SetNodeLocation( homeId, nodeId, (const char*)(Marshal::StringToHGlobalAnsi(_location)).ToPointer()); }
	
		/**
		 * Turns a node on
		 * This is a helper method to simplify basic control of a node.  It is the equivalent of
		 * changing the level reported by the node's Basic command class to 255, and will generate a 
		 * ValueChanged notification from that class.  This command will turn on the device at its
		 * last known level, if supported by the device, otherwise it will turn	it on at 100%.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to be changed.
		 * \see SetNodeOff, SetNodeLevel
		 */
		void SetNodeOn( uint32 homeId, uint8 nodeId ){ Manager::Get()->SetNodeOn( homeId, nodeId ); }

		/**
		 * Turns a node off
		 * This is a helper method to simplify basic control of a node.  It is the equivalent of
		 * changing the level reported by the node's Basic command class to zero, and will generate
		 * a ValueChanged notification from that class.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to be changed.
		 * \see SetNodeOn, SetNodeLevel
		 */
		void SetNodeOff( uint32 homeId, uint8 nodeId ){ Manager::Get()->SetNodeOff( homeId, nodeId ); }

		/**
		 * Sets the basic level of a node
		 * This is a helper method to simplify basic control of a node.  It is the equivalent of
		 * changing the value reported by the node's Basic command class and will generate a 
		 * ValueChanged notification from that class.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to be changed.
		 * \param _level The level to set the node.  Valid values are 0-99 and 255.  Zero is off and
		 * 99 is fully on.  255 will turn on the device at its last known level (if supported).
		 * \see SetNodeOn, SetNodeOff
		 */
		void SetNodeLevel( uint32 homeId, uint8 nodeId, uint8 level ){ Manager::Get()->SetNodeLevel( homeId, nodeId, level ); }
		
	/*@}*/

	//-----------------------------------------------------------------------------
	// Values
	//-----------------------------------------------------------------------------
	/*@{*/
	public:

		/**
		 * Gets the user-friendly label for the value.
		 * \param _id The unique identifier of the value.
		 * \return The value label.
		 * \see ValueID
		 */
		String^ GetValueLabel( ZWValueID^ id ){ return gcnew String(Manager::Get()->GetValueLabel(id->CreateUnmanagedValueID()).c_str()); }

		/**
		 * Gets the units that the value is measured in.
		 * \param _id The unique identifier of the value.
		 * \return The value units.
		 * \see ValueID
		 */
		String^ GetValueUnits( ZWValueID^ id ){ return gcnew String(Manager::Get()->GetValueUnits(id->CreateUnmanagedValueID()).c_str()); }
		
		/**
		 * Gets a help string describing the value's purpose and usage.
		 * \param _id The unique identifier of the value.
		 * \return The value help text.
		 * \see ValueID
		 */
		String^ GetValueHelp( ZWValueID^ id ){ return gcnew String(Manager::Get()->GetValueHelp(id->CreateUnmanagedValueID()).c_str()); }
		
		/**
		 * Test whether the value is read-only.
		 * \param _id The unique identifier of the value.
		 * \return true if the value cannot be changed by the user.	
		 * \see ValueID
		 */
		bool IsValueReadOnly( ZWValueID^ id ){ return Manager::Get()->IsValueReadOnly(id->CreateUnmanagedValueID()); }

		/**
		 * Test whether the value has been set.
		 * \param _id The unique identifier of the value.
		 * \return true if the value has actually been set by a status message from the device, rather than simply being the default.	
		 * \see ValueID
		 */
		bool IsValueSet( ZWValueID^ id ){ return Manager::Get()->IsValueSet(id->CreateUnmanagedValueID()); }

		/**
		 * Gets a value as a bool.
		 * \param _id The unique identifier of the value.
		 * \param o_value a Boolean that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ZWValueID::ValueType_Bool. The type can be tested with a call to ZWValueID::GetType.
		 * \see ValueID::GetType, GetValueAsByte, GetValueAsDecimal, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsBool( ZWValueID^ id, [Out] System::Boolean %o_value );

		/**
		 * Gets a value as an 8-bit unsigned integer.
		 * \param _id The unique identifier of the value.
		 * \param o_value a Byte that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ZWValueID::ValueType_Byte. The type can be tested with a call to ZWValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsDecimal, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsByte( ZWValueID^ id, [Out] System::Byte %o_value );

		/**
		 * Gets a value as a decimal.
		 * \param _id The unique identifier of the value.
		 * \param o_value a Decimal that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ZWValueID::ValueType_Decimal. The type can be tested with a call to ZWValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsDecimal( ZWValueID^ id, [Out] System::Decimal %o_value );

		/**
		 * Gets a value as a 32-bit signed integer.
		 * \param _id The unique identifier of the value.
		 * \param o_value an Int32 that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ZWValueID::ValueType_Int. The type can be tested with a call to ZWValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsDecimal, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsInt( ZWValueID^ id, [Out] System::Int32 %o_value );

		/**
		 * Gets a value as a 16-bit signed integer.
		 * \param _id The unique identifier of the value.
		 * \param o_value an Int16 that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ZWValueID::ValueType_Short. The type can be tested with a call to ZWValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsDecimal, GetValueAsInt, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsShort( ZWValueID^ id, [Out] System::Int16 %o_value );
		
		/**
		 * Gets a value as a string.
		 * Creates a string representation of a value, regardless of type.
		 * \param _id The unique identifier of the value.
		 * \param o_value a String that will be filled with the value.
		 * \return true if the value was obtained.</returns>
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsDecimal, GetValueAsInt, GetValueAsShort, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsString( ZWValueID^ id, [Out] String^ %o_value );
		
		/**
		 * Gets the selected item from a list value.
		 * \param _id The unique identifier of the value.
		 * \param o_value a String that will be filled with the selected item.
		 * \return true if the value was obtained.  Returns false if the value is not a ZWValueID::ValueType_List. The type can be tested with a call to ZWValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsDecimal, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListItems 
		 */
		bool GetValueListSelection( ZWValueID^ id, [Out] String^ %o_value );

		/**
		 * Gets the list of items from a list value.
		 * \param id The unique identifier of the value.
		 * \param o_value List that will be filled with list items.
		 * \return true if the list items were obtained.  Returns false if the value is not a ZWValueID::ValueType_List. The type can be tested with a call to ZWValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsDecimal, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection 
		 */
		bool GetValueListItems( ZWValueID^ id, [Out] array<String^>^ %o_value );

		/**
		 * Sets the state of a bool.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param id The unique identifier of the bool value.
		 * \param value The new value of the bool.
		 * \return true if the value was set.  Returns false if the value is not a ZWValueID::ValueType_Bool. The type can be tested with a call to ZWValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, bool value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value of a byte.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param id The unique identifier of the byte value.
		 * \param value The new value of the byte.
		 * \return true if the value was set.  Returns false if the value is not a ZWValueID::ValueType_Byte. The type can be tested with a call to ZWValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, uint8 value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value of a decimal.
		 * It is usually better to handle decimal values using strings rather than floats, to avoid floating point accuracy issues.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param id The unique identifier of the decimal value.
		 * \param value The new value of the decimal.
		 * \return true if the value was set.  Returns false if the value is not a ZWValueID::ValueType_Decimal. The type can be tested with a call to ZWValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, float value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }
		
		/**
		 * Sets the value of a 32-bit signed integer.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param id The unique identifier of the integer value.
		 * \param value The new value of the integer.
		 * \return true if the value was set.  Returns false if the value is not a ZWValueID::ValueType_Int. The type can be tested with a call to ZWValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, int32 value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value of a 16-bit signed integer.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param id The unique identifier of the integer value.
		 * \param value The new value of the integer.
		 * \return true if the value was set.  Returns false if the value is not a ZWValueID::ValueType_Short. The type can be tested with a call to ZWValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, int16 value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value from a string, regardless of type.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param id The unique identifier of the integer value.
		 * \param value The new value of the string.
		 * \return true if the value was set.  Returns false if the value could not be parsed into the correct type for the value.</returns>
		 */
		bool SetValue( ZWValueID^ id, String^ value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), string((const char*)((Marshal::StringToHGlobalAnsi(value)).ToPointer())) ); }

		/**
		 * Sets the selected item in a list.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param id The unique identifier of the list value.
		 * \param value A string matching the new selected item in the list.
		 * \return true if the value was set.  Returns false if the selection is not in the list, or if the value is not a ZWValueID::ValueType_List.
		 * The type can be tested with a call to ZWValueID::GetType
		 */
		bool SetValueListSelection( ZWValueID^ id, String^ selectedItem ){ return Manager::Get()->SetValueListSelection(id->CreateUnmanagedValueID(), (const char*)(Marshal::StringToHGlobalAnsi(selectedItem)).ToPointer()); }
	
		/**
		 * Starts an activity in a device.
		 * Since buttons are write-only values that do not report a state, no notification callbacks are sent.
		 * \param id The unique identifier of the integer value.
		 * \return true if the activity was started.  Returns false if the value is not a ZWValueID::ValueType_Button. The type can be tested with a call to ZWValueID::GetType
		 */
		bool PressButton( ZWValueID^ id ){ return Manager::Get()->PressButton(id->CreateUnmanagedValueID()); }

		/**
		 * Stops an activity in a device.
		 * Since buttons are write-only values that do not report a state, no notification callbacks are sent.
		 * \param id The unique identifier of the integer value.
		 * \return true if the activity was stopped.  Returns false if the value is not a ZWValueID::ValueType_Button. The type can be tested with a call to ZWValueID::GetType
		 */
		bool ReleaseButton( ZWValueID^ id ){ return Manager::Get()->ReleaseButton(id->CreateUnmanagedValueID()); }
		
	/*@}*/

	//-----------------------------------------------------------------------------
	// Configuration Parameters
	//-----------------------------------------------------------------------------
	/*@{*/
	public:		
		/**
		 * Set the value of a configurable parameter in a device.
		 * Some devices have various parameters that can be configured to control the device behaviour.
		 * These are not reported by the device over the Z-Wave network, but can usually be found in
		 * the device's user manual.
		 * This method returns immediately, without waiting for confirmation from the device that the
		 * change has been made.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to configure.
		 * \param _param The index of the parameter.
		 * \param _value The value to which the parameter should be set.
		 * \return true if the a message setting the value was sent to the device.
		 * \see RequestConfigParam
		 */
		bool SetConfigParam( uint32 homeId, uint8 nodeId, uint8 param, int32 value ){ return Manager::Get()->SetConfigParam( homeId, nodeId, param, value ); }

		/**
		 * Request the value of a configurable parameter from a device.
		 * Some devices have various parameters that can be configured to control the device behaviour.
		 * These are not reported by the device over the Z-Wave network, but can usually be found in
		 * the device's user manual.
		 * This method requests the value of a parameter from the device, and then returns immediately, 
		 * without waiting for a response.  If the parameter index is valid for this device, and the 
		 * device is awake, the value will eventually be reported via a ValueChanged notification callback.
		 * The ValueID reported in the callback will have an index set the same as _param and a command class
		 * set to the same value as returned by a call to Configuration::StaticGetCommandClassId. 
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to configure.
		 * \param _param The index of the parameter.
		 * \see SetConfigParam, ValueID, Notification
		 */
		void RequestConfigParam( uint32 homeId, uint8 nodeId, uint8 param ){ Manager::Get()->RequestConfigParam( homeId, nodeId, param ); }

		/**
		 * Request the values of all known configurable parameters from a device.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to configure.
		 * \see SetConfigParam, RequestConfigParam, ValueID, Notification
		 */
		void RequestAllConfigParams( uint32 homeId, uint8 nodeId ){ Manager::Get()->RequestAllConfigParams( homeId, nodeId ); }
	/*@}*/

	//-----------------------------------------------------------------------------
	// Groups (wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	/*@{*/
	public:		
		/**
		 * Gets the number of association groups reported by this node
		 * In Z-Wave, groups are numbered starting from one.  For example, if a call to GetNumGroups returns 4, the _groupIdx 
		 * value to use in calls to GetAssociations, AddAssociation and RemoveAssociation will be a number between 1 and 4.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose groups we are interested in.
		 * \return The number of groups.
		 * \see GetAssociations, AddAssociation, RemoveAssociation
		 */
		uint8 GetNumGroups( uint32 homeId, uint8 nodeId ){ return Manager::Get()->GetNumGroups( homeId, nodeId ); }

		/**
		 * Gets the associations for a group.
		 * Makes a copy of the list of associated nodes in the group, and returns it in an array of uint8's.
		 * The caller is responsible for freeing the array memory with a call to delete [].
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations we are interested in.
		 * \param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \param o_associations If the number of associations returned is greater than zero, o_associations will be set to point to an array containing the IDs of the associated nodes.
		 * \return The number of nodes in the associations array.  If zero, the array will point to NULL, and does not need to be deleted.
		 * \see GetNumGroups, AddAssociation, RemoveAssociation
		 */
		uint32 GetAssociations( uint32 const homeId, uint8 const nodeId, uint8 const groupIdx, [Out] array<Byte>^ %o_associations );

		/**
		 * Gets the maximum number of associations for a group.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations we are interested in.
		 * \param _groupIdx one-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \return The maximum number of nodes that can be associated into the group.
		 * \see GetNumGroups, AddAssociation, RemoveAssociation
		 */
		uint8 GetMaxAssociations( uint32 const homeId, uint8 const nodeId, uint8 const groupIdx ){ return Manager::Get()->GetMaxAssociations( homeId, nodeId, groupIdx ); }

		/**
		 * Adds a node to an association group.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the association data
		 * held in this class is updated directly.  This will be reverted by a future Association message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations are to be changed.
		 * \param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \param _targetNodeId Identifier for the node that will be added to the association group.
		 * \see GetNumGroups, GetAssociations, RemoveAssociation
		 */
		void AddAssociation( uint32 homeId, uint8 nodeId, uint8 groupIdx, uint8 targetNodeId ){ return Manager::Get()->AddAssociation( homeId, nodeId, groupIdx, targetNodeId ); }

		/**
		 * Removes a node from an association group.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the association data
		 * held in this class is updated directly.  This will be reverted by a future Association message from the device
		 * if the Z-Wave message actually failed to get through.   Notification callbacks will be sent in both cases.
		 * \param homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations are to be changed.
		 * \param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \param _targetNodeId Identifier for the node that will be removed from the association group.
		 * \see GetNumGroups, GetAssociations, AddAssociation
		 */
		void RemoveAssociation( uint32 homeId, uint8 nodeId, uint8 groupIdx, uint8 targetNodeId ){ return Manager::Get()->RemoveAssociation( homeId, nodeId, groupIdx, targetNodeId ); }
	/*@}*/

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	/*@{*/
	public:	
		/**
		 * Hard Reset a PC Z-Wave Controller.
		 * Resets a controller and erases its network configuration settings.  The controller becomes a primary controller ready to add devices to a new network.
		 * \param homeId The Home ID of the Z-Wave controller to be reset.
		 * \see SoftReset
		 */
		void ResetController( uint32 homeId ){ Manager::Get()->ResetController( homeId ); }

		/**
		 * Soft Reset a PC Z-Wave Controller.
		 * Resets a controller without erasing its network configuration settings.
		 * \param homeId The Home ID of the Z-Wave controller to be reset.
		 * \see SoftReset
		 */
		void SoftReset( uint32 homeId ){ Manager::Get()->SoftReset( homeId ); }

		/**
		 * Start a controller command process.
		 * \param homeId The Home ID of the Z-Wave controller.
		 * \param command The command to be sent to the controller.
		 * \param highPower used only with the AddDevice, AddController, RemoveDevice and RemoveController commands. 
		 * Usually when adding or removing devices, the controller operates at low power so that the controller must
		 * be physically close to the device for security reasons.  If _highPower is true, the controller will 
		 * operate at normal power levels instead.  Defaults to false.
		 * \param _nodeId used only with the ReplaceFailedNode command, to specify the node that is going to be replaced.
		 * \return true if the command was accepted and has started.
		 * \see CancelControllerCommand, HasNodeFailed, RemoveFailedNode, Driver::ControllerCommand, Driver::pfnControllerCallback_t, 
		 * to notify the user of progress or to request actions on the user's part.  Defaults to NULL.
		 * <p> Commands
		 * - ZWControllerCommand.AddController - Add a new secondary controller to the Z-Wave network.
		 * - ZWControllerCommand.AddDevice - Add a new device (but not a controller) to the Z-Wave network.
		 * - ZWControllerCommand.CreateNewPrimary (Not yet implemented)
		 * - ZWControllerCommand.ReceiveConfiguration -   
		 * - ZWControllerCommand.RemoveController - remove a controller from the Z-Wave network.
		 * - ZWControllerCommand.RemoveDevice - remove a device (but not a controller) from the Z-Wave network.
 		 * - ZWControllerCommand.RemoveFailedNode - move a node to the controller's list of failed nodes.  The node must actually
		 * have failed or have been disabled since the command will fail if it responds.  A node must be in the controller's failed nodes list
		 * for ControllerCommand_ReplaceFailedNode to work.
		 * - ZWControllerCommand.HasNodeFailed - Check whether a node is in the controller's failed nodes list.
		 * - ZWControllerCommand.ReplaceFailedNode - replace a failed device with another. If the node is not in 
		 * the controller's failed nodes list, or the node responds, this command will fail.
		 * - ZWControllerCommand.TransferPrimaryRole (Not yet implemented) - Add a new controller to the network and
		 * make it the primary.  The existing primary will become a secondary controller.  
		 * - ZWControllerCommand.RequestNetworkUpdate - Update the controller with network information from the SUC/SIS.
		 * - ZWControllerCommand.RequestNodeNeighborUpdate - Get a node to rebuild it's neighbour list.  This method also does ControllerCommand_RequestNodeNeighbors afterwards.
		 * - ZWControllerCommand.AssignReturnRoute - Assign network routes to a device.
		 * - ZWControllerCommand.DeleteReturnRoute - Delete network routes from a device.
		 * <p>These processes are asynchronous, and at various stages OpenZWave will trigger a callback
		 * to notify the user of progress or to request actions on the user's part.
		 * <p> Controller States
		 * - ZWControllerState.Waiting, the controller is waiting for a user action.  A notice should be displayed 
		 * to the user at this point, telling them what to do next.
		 * For the add, remove, replace and transfer primary role commands, the user needs to be told to press the 
		 * inclusion button on the device that  is going to be added or removed.  For ControllerCommand_ReceiveConfiguration, 
		 * they must set their other controller to send its data, and for ControllerCommand_CreateNewPrimary, set the other
		 * controller to learn new data.
		 * - ZWControllerState.InProgress - the controller is in the process of adding or removing the chosen node.
		 * - ZWControllerState.Complete - the controller has finished adding or removing the node, and the command is complete.
		 * - ZWControllerState.Failed - will be sent if the command fails for any reason.
		 * <p>To register for these notifications, create an event handler with the same signature as
		 * the ManagedControllerStateChangedHandler delegate.  Just before calling the BeginControllerCommand
		 * method, subscribe to the OnControllerStateChanged event.  Once the command has completed, remember
		 * to unsubscribe from the event.
		 * /code
		 * private UInt32 m_homeId;
		 * private ZWManager m_manager;
		 * private ManagedControllerStateChangedHandler m_myEventHandler = new ManagedControllerStateChangedHandler( MyControllerStateChangedHandler );
		 *
		 * public void MyAddControllerMethod()
		 * {
		 *     m_manager.OnControllerStateChanged += m_myEventHandler;
		 *     m_manager.BeginControllerCommand( m_homeId, ZWControllerCommand::AddController, false );		
		 * }
		 *
		 * public void MyControllerStateChangedHandler( ZWControllerState state )
		 * {
		 *     // Handle the controller state notifications here.
		 *     bool complete = false;
		 *     switch( state )
		 *     {
		 *         case ZWControllerState::Waiting:
		 *         {
	     *             // Display a message to tell the user to press the include button on the controller
		 *             break;
		 *         }
		 *         case ZWControllerState::InProgress:
		 *         {
		 *             // Tell the user that the controller has been found and the adding process is in progress.
		 *             break;
		 *         }
		 *         case ZWControllerState::Completed:
		 *         {
		 *             // Tell the user that the controller has been successfully added.
		 *             // The command is now complete
		 *             complete = true;
		 *             break;
		 *         }
		 *         case ZWControllerState::Failed:
		 *         {
		 *             // Tell the user that the controller addition process has failed.
		 *             // The command is now complete
		 *             complete = true;
		 *             break;
		 *         }
		 *         case ZWControllerState::NodeOK:
		 *         {
		 *             // Tell the user that the node referenced by one of the Failed commands is actually working.
		 *             // The command is now complete
		 *             complete = true;
		 *             break;
		 *         }
		 *         case ZWControllerState::Failed:
		 *         {
		 *             // Tell the user that the node referenced in the HasNodeFailed command has failed.
		 *             // The command is now complete
		 *             complete = true;
		 *             break;
		 *         }
		 *     }
		 *
		 *     if( complete )
		 *     {
		 *         // Remove the event handler
		 *         m_manager.OnControllerStateChanged -= m_myEventHandler;
		 *     }
		 * }
		 * /endcode
		 */
		bool BeginControllerCommand( uint32 homeId, ZWControllerCommand command, bool highPower, uint8 nodeId );
			
		/**
		 * Cancels any in-progress command running on a controller.
		 * \param homeId The Home ID of the Z-Wave controller.
		 * \return true if a command was running and was cancelled.
		 * \see BeginControllerCommand 
		 */
		bool CancelControllerCommand( uint32 homeId ){ return Manager::Get()->CancelControllerCommand( homeId ); }
	/*@}*/

	public:
		ZWManager(){}

	private:

		void  OnNotificationFromUnmanaged(Notification* _notification,void* _context);					// Forward notification to managed delegates hooked via Event addhandler 
		void  OnControllerStateChangedFromUnmanaged(Driver::ControllerState _state,void* _context);		// Forward controller state change to managed delegates hooked via Event addhandler 

		GCHandle										m_gchNotification;
		OnNotificationFromUnmanagedDelegate^			m_onNotification;

		GCHandle										m_gchControllerState;
		OnControllerStateChangedFromUnmanagedDelegate^	m_onStateChanged;
	};
}
