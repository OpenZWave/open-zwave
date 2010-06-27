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

using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace Runtime::InteropServices;
using namespace OpenZWave;


namespace OpenZWaveDotNet
{
	// Delegates for handling notification callbacks
	public delegate void ManagedWatchersHandler(ZWNotification^ notification);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	private delegate void OnNotificationFromUnmanagedDelegate(Notification* _notification, void* _context);

	public ref class ZWManager
	{
	//-----------------------------------------------------------------------------
	// Events
	//-----------------------------------------------------------------------------
	public:
		ManagedWatchersHandler^ m_event;
		event ManagedWatchersHandler^ OnZWNotification
		{
			void add( ManagedWatchersHandler ^ d )
			{ 
				m_event += d;
			} 
			
			void remove(ManagedWatchersHandler ^ d)
			{ 
				m_event -= d;
			} 
			
			void raise(ZWNotification^ notification)
			{ 
				ManagedWatchersHandler^ tmp = m_event; 
				if (tmp)
				{ 
					tmp->Invoke( notification );
				} 
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
		 * @param _configPath a string containing the path to the OpenZWave library config folder, which contains XML descriptions of Z-Wave manufacturers and products.
		 * @param _userPath a string containing the path to the application's user data folder where the OpenZWave should store the Z-Wave network configuration and state.
		 * @return a pointer to the newly created Manager object.
		 * @see Destroy, AddWatcher, AddDriver
		 */
		void Create( String^ _configPath, String^ _userPath );

		/**
		 * Deletes the Manager and cleans up any associated objects.  
		 * @see Create, Get
		 */
		void Destroy(){ Manager::Get()->Destroy(); }

		/**
		 * Get the path to the OpenZWave library config folder.
		 * Gets the string that was passed into the Manager::Create method as the _configPath argument.  
		 * @return A string containing the path of the OpenZWave library config folder.
		 * @see Create
		 */
		String^ GetConfigPath(){ return gcnew String(Manager::Get()->GetConfigPath().c_str()); }

		/**
		 * Get the path to the application's user data folder.
		 * Gets the string that was passed into the Manager::Create method as the _userPath argument.  
		 * @return A string containing the path of the application's user data folder.
		 * @see Create
		 */
		String^ GetUserPath(){ return gcnew String(Manager::Get()->GetUserPath().c_str()); }
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
		 * @param _homeId The Home ID of the Z-Wave controller to save.
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
		 * @param _serialPortName The string used to open the serial port, for example "\\.\COM3".
		 * @return True if a new driver was created, false if a driver for the controller already exists.
		 * @see Create, Get, RemoveDriver
		 */
		bool AddDriver( String^ serialPortName ){ return Manager::Get()->AddDriver((const char*)(Marshal::StringToHGlobalAnsi(serialPortName)).ToPointer()); }

		/**
		 * Removes the driver for a Z-Wave controller, and closes the serial port.
		 * Drivers do not need to be explicitly removed before calling Destroy - this is handled automatically.
		 * @paaram _serialPortName The same string as was passed in the original call to AddDriver.
		 * @returns True if the driver was removed, false if it could not be found.
		 * @see Destroy, AddDriver
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
		 * @param _homeId The Home ID of the Z-Wave controller.
		 * @return true if it is a primary controller, false if not.
		 */
		bool IsPrimaryController( uint32 homeId ){ return Manager::Get()->IsPrimaryController(homeId); }

		/**
		 * Query if the controller is a static update controller.
		 * A Static Update Controller (SUC) is a controller that must never be moved in normal operation
		 * and which can be used by other nodes to receive information about network changes.
		 * @param _homeId The Home ID of the Z-Wave controller.
		 * @return true if it is a static update controller, false if not.
		 */
		bool IsStaticUpdateController( uint32 homeId ){ return Manager::Get()->IsStaticUpdateController(homeId); }

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
		 * @param _seconds The length of the polling interval in seconds.
		 */
		void SetPollInterval( int32 seconds ){ Manager::Get()->SetPollInterval(seconds); }

		/**
		 * Enable the polling of a device's state.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to start polling.
		 * @return true if polling was enabled.
		 */
		bool EnablePoll( uint32 homeId, uint8 nodeId ){ return Manager::Get()->EnablePoll(homeId,nodeId); }

		/**
		 * Disable the polling of a device's state.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to stop polling.
		 * @return true if polling was disabled.
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return True if the request was sent successfully.
		 */
		bool RefreshNodeInfo( uint32 homeId, uint8 nodeId ){ return Manager::Get()->RefreshNodeInfo(homeId,nodeId); }
 		
		/**
		 * Trigger the fetching of dynamic value data for a node.
		 * Causes the node's values to be requested from the Z-Wave network.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return True if the request was sent successfully.
		 */
		void RequestNodeState( uint32 homeId, uint8 nodeId ){ Manager::Get()->RequestNodeState(homeId,nodeId); }

		/**
		 * Get a human-readable label describing the node
		 * The label is taken from the Z-Wave specific, generic or basic type, depending on which of those values are specified by the node.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the label text.
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the node's manufacturer name.
		 * @see SetNodeManufacturerName, GetNodeProductName, SetNodeProductName
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the node's product name.
		 * @see SetNodeProductName, GetNodeManufacturerName, SetNodeManufacturerName
		 */
		String^ GetNodeProductName( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeProductName(homeId,nodeId).c_str()); }

		/**
		 * Get the name of a node
		 * The node name is a user-editable label for the node that would normally be handled by the
		 * Node Naming commmand class, but many devices do not support it.  So that a node can always
		 * be named, OpenZWave stores it with the node data, and provides access through this method
		 * and SetNodeName, rather than reporting it via a command class Value object.
		 * The maximum length of a node name is 16 characters.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the node's name.
		 * @see SetNodeName, GetNodeLocation, SetNodeLocation
		 */
		String^ GetNodeName( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeName(homeId,nodeId).c_str()); }

		/**
		 * Get the location of a node
		 * The node location is a user-editable string that would normally be handled by the Node Naming
		 * commmand class, but many devices do not support it.  So that a node can always report its
		 * location, OpenZWave stores it with the node data, and provides access through this method
		 * and SetNodeLocation, rather than reporting it via a command class Value object.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the node's location.
		 * @see SetNodeLocation, GetNodeName, SetNodeName
		 */
		String^ GetNodeLocation( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeLocation(homeId,nodeId).c_str()); }

		/**
		 * Get the manufacturer ID of a device
		 * The manufacturer ID is a four digit hex code and would normally be handled by the Manufacturer
		 * Specific commmand class, but not all devices support it.  Although the value reported by this
		 * method will be an empty string if the command class is not supported and cannot be set by the 
		 * user, the manufacturer ID is still stored with the node data (rather than being reported via a
		 * command class Value object) to retain a consistent approach with the other manufacturer specific data.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the node's manufacturer ID, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * @see GetNodeProductType, GetNodeProductId, GetNodeManufacturerName, GetNodeProductName
		 */
		String^ GetNodeManufacturerId( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeManufacturerId(homeId,nodeId).c_str()); }

		/**
		 * Get the product type of a device
		 * The product type is a four digit hex code and would normally be handled by the Manufacturer Specific
		 * commmand class, but not all devices support it.  Although the value reported by this method will
		 * be an empty string if the command class is not supported and cannot be set by the user, the product
		 * type is still stored with the node data (rather than being reported via a command class Value object)
		 * to retain a consistent approach with the other manufacturer specific data.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the node's product type, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * @see GetNodeManufacturerId, GetNodeProductId, GetNodeManufacturerName, GetNodeProductName
		 */
		String^ GetNodeProductType( uint32 homeId, uint8 nodeId ){ return gcnew String(Manager::Get()->GetNodeProductType(homeId,nodeId).c_str()); }

		/**
		 * Get the product ID of a device
		 * The product ID is a four digit hex code and would normally be handled by the Manufacturer Specific
		 * commmand class, but not all devices support it.  Although the value reported by this method will
		 * be an empty string if the command class is not supported and cannot be set by the user, the product
		 * ID is still stored with the node data (rather than being reported via a command class Value object)
		 * to retain a consistent approach with the other manufacturer specific data.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @return A string containing the node's product ID, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * @see GetNodeManufacturerId, GetNodeProductType, GetNodeManufacturerName, GetNodeProductName
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @param _manufacturerName	A string containing the node's manufacturer name.
		 * @see GetNodeManufacturerName, GetNodeProductName, SetNodeProductName
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @param _productName A string containing the node's product name.
		 * @see GetNodeProductName, GetNodeManufacturerName, SetNodeManufacturerName
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @param _nodeName A string containing the node's name.
		 * @see GetNodeName, GetNodeLocation, SetNodeLocation
		 */
		void SetNodeName( uint32 homeId, uint8 nodeId, String^ _nodeName ){ Manager::Get()->SetNodeName( homeId, nodeId, (const char*)(Marshal::StringToHGlobalAnsi(_nodeName)).ToPointer()); }

		/**
		 * Set the location of a node
		 * The node location is a user-editable string that would normally be handled by the Node Naming
		 * commmand class, but many devices do not support it.  So that a node can always report its
		 * location, OpenZWave stores it with the node data, and provides access through this method
		 * and GetNodeLocation, rather than reporting it via a command class Value object.
		 * If the device does support the Node Naming command class, the new location will be sent to the node.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to query.
		 * @param _location A string containing the node's location.
		 * @see GetNodeLocation, GetNodeName, SetNodeName
		 */
		void SetNodeLocation( uint32 homeId, uint8 nodeId, String^ _location ){ Manager::Get()->SetNodeLocation( homeId, nodeId, (const char*)(Marshal::StringToHGlobalAnsi(_location)).ToPointer()); }
	/*@}*/

	//-----------------------------------------------------------------------------
	// Values
	//-----------------------------------------------------------------------------
	/*@{*/
	public:
		/**
		 * Gets the user-friendly label for the value.
		 * @param _id The unique identifier of the value.
		 * @return The value label.
		 * @see ValueID
		 */
		String^ GetValueLabel( ZWValueID^ id ){ return gcnew String(Manager::Get()->GetValueLabel(id->CreateUnmanagedValueID()).c_str()); }

		/**
		 * Gets the units that the value is measured in.
		 * @param _id The unique identifier of the value.
		 * @return The value units.
		 * @see ValueID
		 */
		String^ GetValueUnits( ZWValueID^ id ){ return gcnew String(Manager::Get()->GetValueUnits(id->CreateUnmanagedValueID()).c_str()); }
		
		/**
		 * Gets a help string describing the value's purpose and usage.
		 * @param _id The unique identifier of the value.
		 * @return The value help text.
		 * @see ValueID
		 */
		String^ GetValueHelp( ZWValueID^ id ){ return gcnew String(Manager::Get()->GetValueHelp(id->CreateUnmanagedValueID()).c_str()); }
		
		/**
		 * Test whether the value is read-only.
		 * @param _id The unique identifier of the value.
		 * @return true if the value cannot be changed by the user.	
		 * @see ValueID
		 */
		bool IsValueReadOnly( ZWValueID^ id ){ return Manager::Get()->IsValueReadOnly(id->CreateUnmanagedValueID()); }

		/**
		 * Test whether the value has been set.
		 * @param _id The unique identifier of the value.
		 * @return true if the value has actually been set by a status message from the device, rather than simply being the default.	
		 * @see ValueID
		 */
		bool IsValueSet( ZWValueID^ id ){ return Manager::Get()->IsValueSet(id->CreateUnmanagedValueID()); }

		/**
		 * Gets a value as a bool.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to a bool that will be filled with the value.
		 * @return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Bool. The type can be tested with a call to ValueID::GetType.
		 * @see ValueID::GetType, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsBool( ZWValueID^ id, bool* o_value ){ return Manager::Get()->GetValueAsBool(id->CreateUnmanagedValueID(), o_value); }

		/**
		 * Gets a value as an 8-bit unsigned integer.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to a uint8 that will be filled with the value.
		 * @return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Byte. The type can be tested with a call to ValueID::GetType
		 * @see ValueID::GetType, GetValueAsBool, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsByte( ZWValueID^ id, uint8* o_value ){ return Manager::Get()->GetValueAsByte(id->CreateUnmanagedValueID(), o_value); }

		/**
		 * Gets a value as a float.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to a float that will be filled with the value.
		 * @return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Decimal. The type can be tested with a call to ValueID::GetType
		 * @see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsFloat( ZWValueID^ id, float* o_value ){ return Manager::Get()->GetValueAsFloat(id->CreateUnmanagedValueID(), o_value); }

		/**
		 * Gets a value as a 32-bit signed integer.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to an int32 that will be filled with the value.
		 * @return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Int. The type can be tested with a call to ValueID::GetType
		 * @see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsInt( ZWValueID^ id, int32* o_value ){ return Manager::Get()->GetValueAsInt(id->CreateUnmanagedValueID(), o_value); }

		/**
		 * Gets a value as a 16-bit signed integer.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to an int16 that will be filled with the value.
		 * @return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Short. The type can be tested with a call to ValueID::GetType
		 * @see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsString, GetValueListSelection, GetValueListItems 
		 */
		bool GetValueAsShort( ZWValueID^ id, int16* o_value ){ return Manager::Get()->GetValueAsShort(id->CreateUnmanagedValueID(), o_value); }
		
		/**
		 * Gets a value as a string.
		 * Creates a string representation of a value, regardless of type.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to a string that will be filled with the value.
		 * @return true if the value was obtained.</returns>
		 * @see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueListSelection, GetValueListItems 
		 */
		//bool GetValueAsString( ZWValueID^ id, String^ o_value ){ return Manager::Get()->GetValueAsString(id->CreateUnmanagedValueID(), o_value); }
		
		/**
		 * Gets the selected item from a list value.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to a string that will be filled with the selected item.
		 * @return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_List. The type can be tested with a call to ValueID::GetType
		 * @see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListItems 
		 */
		//bool GetValueListSelection( ZWValueID^ id, String^ o_value ){ return Manager::Get()->GetValueListSelection(id->CreateUnmanagedValueID(), o_value); }

		/**
		 * Gets the list of items from a list value.
		 * @param _id The unique identifier of the value.
		 * @param o_value Pointer to a vector of strings that will be filled with list items. The vector will be cleared before the items are added.
		 * @return true if the list items were obtained.  Returns false if the value is not a ValueID::ValueType_List. The type can be tested with a call to ValueID::GetType
		 * @see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection 
		 */
		//bool GetValueListItems( ValueID const& _id, vector<string>* o_value );

		/**
		 * Sets the state of a bool.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _id The unique identifier of the bool value.
		 * @param o_value The new value of the bool.
		 * @return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Bool. The type can be tested with a call to ValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, bool value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value of a byte.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _id The unique identifier of the byte value.
		 * @param o_value The new value of the byte.
		 * @return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Byte. The type can be tested with a call to ValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, uint8 value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value of a decimal.
		 * It is usually better to handle decimal values using strings rather than floats, to avoid floating point accuracy issues.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _id The unique identifier of the decimal value.
		 * @param o_value The new value of the decimal.
		 * @return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Decimal. The type can be tested with a call to ValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, float value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }
		
		/**
		 * Sets the value of a 32-bit signed integer.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _id The unique identifier of the integer value.
		 * @param o_value The new value of the integer.
		 * @return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Int. The type can be tested with a call to ValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, int32 value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value of a 16-bit signed integer.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _id The unique identifier of the integer value.
		 * @param o_value The new value of the integer.
		 * @return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Short. The type can be tested with a call to ValueID::GetType
		 */
		bool SetValue( ZWValueID^ id, int16 value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), value); }

		/**
		 * Sets the value from a string, regardless of type.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _id The unique identifier of the integer value.
		 * @param o_value The new value of the string.
		 * @return true if the value was set.  Returns false if the value could not be parsed into the correct type for the value.</returns>
		 */
		bool SetValue( ZWValueID^ id, String^ value ){ return Manager::Get()->SetValue(id->CreateUnmanagedValueID(), string((const char*)((Marshal::StringToHGlobalAnsi(value)).ToPointer())) ); }

		/**
		 * Sets the selected item in a list.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _id The unique identifier of the list value.
		 * @param o_value A string matching the new selected item in the list.
		 * @return true if the value was set.  Returns false if the selection is not in the list, or if the value is not a ValueID::ValueType_List.
		 * The type can be tested with a call to ValueID::GetType
		 */
		bool SetValueListSelection( ZWValueID^ id, String^ selectedItem ){ return Manager::Get()->SetValueListSelection(id->CreateUnmanagedValueID(), (const char*)(Marshal::StringToHGlobalAnsi(selectedItem)).ToPointer()); }
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to configure.
		 * @param _param The index of the parameter.
		 * @param _value The value to which the parameter should be set.
		 * @return true if the a message setting the value was sent to the device.
		 * @see RequestConfigParam
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node to configure.
		 * @param _param The index of the parameter.
		 * @see SetConfigParam, ValueID, Notification
		 */
		void RequestConfigParam( uint32 homeId, uint8 nodeId, uint8 param ){ Manager::Get()->RequestConfigParam( homeId, nodeId, param ); }
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
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node whose groups we are interested in.
		 * @return The number of groups.
		 * @see GetAssociations, AddAssociation, RemoveAssociation
		 */
		uint8 GetNumGroups( uint32 homeId, uint8 nodeId ){ return Manager::Get()->GetNumGroups( homeId, nodeId ); }

		/**
		 * Gets the associations for a group.
		 * Makes a copy of the list of associated nodes in the group, and returns it in an array of uint8's.
		 * The caller is responsible for freeing the array memory with a call to delete [].
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node whose associations we are interested in.
		 * @param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * @param o_associations If the number of associations returned is greater than zero, o_associations will be set to point to an array containing the IDs of the associated nodes.
		 * @return The number of nodes in the associations array.  If zero, the array will point to NULL, and does not need to be deleted.
		 * @see GetNumGroups, AddAssociation, RemoveAssociation
		 */
		//uint32 GetAssociations( uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx, uint8** o_associations );

		/**
		 * Adds a node to an association group.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the association data
		 * held in this class is updated directly.  This will be reverted by a future Association message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node whose associations are to be changed.
		 * @param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * @param _targetNodeId Identifier for the node that will be added to the association group.
		 * @see GetNumGroups, GetAssociations, RemoveAssociation
		 */
		void AddAssociation( uint32 homeId, uint8 nodeId, uint8 groupIdx, uint8 targetNodeId ){ return Manager::Get()->AddAssociation( homeId, nodeId, groupIdx, targetNodeId ); }

		/**
		 * Removes a node from an association group.
		 * Due to the possibility of a device being asleep, the command is assumed to suceeed, and the association data
		 * held in this class is updated directly.  This will be reverted by a future Association message from the device
		 * if the Z-Wave message actually failed to get through.   Notification callbacks will be sent in both cases.
		 * @param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * @param _nodeId The ID of the node whose associations are to be changed.
		 * @param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * @param _targetNodeId Identifier for the node that will be removed from the association group.
		 * @see GetNumGroups, GetAssociations, AddAssociation
		 */
		void RemoveAssociation( uint32 homeId, uint8 nodeId, uint8 groupIdx, uint8 targetNodeId ){ return Manager::Get()->RemoveAssociation( homeId, nodeId, groupIdx, targetNodeId ); }
	/*@}*/

	//-----------------------------------------------------------------------------
	//	Notifications
	//-----------------------------------------------------------------------------
	/*@{*/
	public:
		/**
		 * Add a notification watcher.
		 * In OpenZWave, all feedback from the Z-Wave network is sent to the application via callbacks.
		 * This method allows the application to add a notification callback handler, known as a "watcher" to OpenZWave.
		 * An application needs only add a single watcher - all notifications will be reported to it.
		 * @param _watcher pointer to a function that will be called by the notification system.
		 * @param _context pointer to user defined data that will be passed to the watcher function with each notification.
		 * @return true if the watcher was successfully added.
		 * @see RemoveWatcher, Notification
		 */
		//bool AddWatcher( pfnOnNotification_t _watcher, void* _context );

		/**
		 * Remove a notification watcher.
		 * @param _watcher pointer to a function that must match that passed to a previous call to AddWatcher
		 * @param _context pointer to user defined data that must match the one passed in that same previous call to AddWatcher.
		 * @return true if the watcher was successfully removed.
		 * @see AddWatcher, Notification
		 */
		//bool RemoveWatcher( pfnOnNotification_t _watcher, void* _context );
	/*@}*/

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	/*@{*/
	public:	
		/**
		 * Hard Reset a PC Z-Wave Controller.
		 * Resets a controller and erases its network configuration settings.  The controller becomes a primary controller ready to add devices to a new network.
		 * @param _homeId The Home ID of the Z-Wave controller to be reset.
		 * @see SoftReset
		 */
		void ResetController( uint32 homeId ){ Manager::Get()->ResetController( homeId ); }

		/**
		 * Soft Reset a PC Z-Wave Controller.
		 * Resets a controller without erasing its network configuration settings.
		 * @param _homeId The Home ID of the Z-Wave controller to be reset.
		 * @see SoftReset
		 */
		void SoftReset( uint32 homeId ){ Manager::Get()->SoftReset( homeId ); }

		/**
		 * Start a controller command process.
		 * @param _homeId The Home ID of the Z-Wave controller.
		 * @param _command The command to be sent to the controller.
		 * <p> Commands
		 * - Driver::ControllerCommand_AddController - Add a new secondary controller to the Z-Wave network.
		 * - Driver::ControllerCommand_AddDevice - Add a new device (but not a controller) to the Z-Wave network.
		 * - Driver::ControllerCommand_CreateNewPrimary (Not yet implemented)
		 * - Driver::ControllerCommand_ReceiveConfiguration -   
		 * - Driver::ControllerCommand_RemoveController - remove a controller from the Z-Wave network.
		 * - Driver::ControllerCommand_RemoveDevice - remove a device (but not a controller) from the Z-Wave network.
		 * - Driver::ControllerCommand_ReplaceFailedDevice (Not yet implemented) - 
		 * - Driver:: ControllerCommand_TransferPrimaryRole	(Not yet implemented) - Add a new controller to the network and
		 * make it the primary.  The existing primary will become a secondary controller.  
		 * @param _callback pointer to a function that will be called at various stages during the command process
		 * to notify the user of progress or to request actions on the user's part.  Defaults to NULL.
		 * <p> Callbacks
		 * - Driver::ControllerState_Waiting, the controller is waiting for a user action.  A notice should be displayed 
		 * to the user at this point, telling them what to do next.
		 * For the add, remove, replace and transfer primary role commands, the user needs to be told to press the 
		 * inclusion button on the device that  is going to be added or removed.  For ControllerCommand_ReceiveConfiguration, 
		 * they must set their other controller to send its data, and for ControllerCommand_CreateNewPrimary, set the other
		 * controller to learn new data.
		 * - Driver::ControllerState_InProgress - the controller is in the process of adding or removing the chosen node.
		 * - Driver::ControllerState_Complete - the controller has finished adding or removing the node, and the command is complete.
		 * - Driver::ControllerState_Failed - will be sent if the command fails for any reason.
		 * @param _context pointer to user defined data that will be passed into to the callback function.  Defaults to NULL.
		 * @param _highPower used only with the AddDevice, AddController, RemoveDevice and RemoveController commands. 
		 * Usually when adding or removing devices, the controller operates at low power so that the controller must
		 * be physically close to the device for security reasons.  If _highPower is true, the controller will 
		 * operate at normal power levels instead.  Defaults to false.
		 * @see CancelControllerCommand, Driver::ControllerCommand, Driver::pfnControllerCallback_t, 
		 */
		//bool BeginControllerCommand( uint32 const _homeId, Driver::ControllerCommand _command, Driver::pfnControllerCallback_t _callback = NULL, void* _context = NULL, bool _highPower = false );
			
		/**
		 * Cancels any in-progress command running on a controller.
		 * @param _homeId The Home ID of the Z-Wave controller.
		 * @see BeginControllerCommand 
		 */
		bool CancelControllerCommand( uint32 homeId ){ return Manager::Get()->CancelControllerCommand( homeId ); }

		// TBD...
		//void RequestNodeNeighborUpdate( uint32 const _homeId, uint8 const _nodeId );
		//void AssignReturnRoute( uint32 const _homeId, uint8 const _srcNodeId, uint8 const _dstNodeId );
		//void RequestNetworkUpdate( uint32 const _homeId );
		//void ReadMemory( uint32 const _homeId,  uint16 const offset );
	/*@}*/

	public:
		ZWManager()
		{
			m_criticalSection = gcnew Object();
		}

	private:

		void  OnNotificationFromUnmanaged(Notification* _notification,void* _context);	// Forward notification to managed delegates hooked via Event addhandler 

		GCHandle								m_gch;
		OnNotificationFromUnmanagedDelegate^	m_onNotification;
		Object^									m_criticalSection;
	};
}