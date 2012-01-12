//-----------------------------------------------------------------------------
//
//	HidController.h
//
//	Cross-platform HID port handler
//
//	Copyright (c) 2010 Jason Frazier <frazierjason@gmail.com>
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

#ifndef _HidController_H
#define _HidController_H

#include <string>
#include "Defs.h"
#include "Controller.h"

namespace OpenZWave
{
	class Driver;
    class Msg;
	class HidControllerImpl;

    class HidController: public Controller
	{
		friend class HidControllerImpl;

	public:

		/**
		 * Constructor.
		 * Creates an object that represents a HID port.
		 */
		HidController();

		/**
		 * Destructor.
		 * Destroys the HID port object.
		 */
		virtual ~HidController();

		/**
		 * Queues up the controller's initialization commands.
		 * @param _driver Pointer to the driver object used to send the commands.
		 * @see Driver::Init
		 */
		void PlayInitSequence( Driver* _driver );

		/**
		 * Set the USB vendor ID search value.  The HID port must be closed for the setting to be accepted.
		 * @param _baud Vendor ID value to match when enumerating USB HID devices.
		 * @return True if the vendor ID value was accepted.
		 * @see Open, Close
		 */
		bool SetVendorId( uint32 const _vendorId );

		/**
		 * Set the USB product ID search value.  The HID port must be closed for the setting to be accepted.
		 * @param _parity Product ID value to match when enumerating USB HID devices.
		 * @return True if the product ID value was accepted.
		 * @see Open, Close
		 */
		bool SetProductId( uint32 const _productId );

		/**
		 * Set the USB serial number search value.  The HID port must be closed for the setting to be accepted.
 		 * @param _parity Serial number string to match when enumerating USB HID devices. If empty, any serial number will be accepted.
		 * @return True if the serial number value was accepted.
		 * @see Open, Close
		 */
		bool SetSerialNumber( string const& _serialNumber );

		/**
		 * Open a HID port.
		 * Attempts to open a HID port and initialize it with the specified paramters.
		 * @param _HidControllerName The name of the port to open.  For example, ttyS1 on Linux, or \\.\COM2 in Windows.
		 * @return True if the port was opened and configured successfully.
		 * @see Close, Read, Write
		 */
		bool Open( string const& _hidControllerName );

		/**
		 * Close a HID port.
		 * Closes the HID port.
		 * @return True if the port was closed successfully, or false if the port was already closed, or an error occurred.
		 * @see Open
		 */
		bool Close();

		/**
		 * Write to a HID port.
		 * Attempts to write data to an open HID port.
		 * @param _buffer Pointer to a block of memory containing the data to be written.
		 * @param _length Length in bytes of the data.
		 * @return The number of bytes written.
		 * @see Read, Open, Close
		 */
		uint32 Write( uint8* _buffer, uint32 _length );

    private:
        uint32                      m_vendorId;
        uint32                      m_productId;
        string                      m_serialNumber;
		string						m_hidControllerName;

		HidControllerImpl*	        m_pImpl;	// Pointer to an object that encapsulates the platform-specific implementation of the HID port.
		bool			            m_bOpen;
	};

} // namespace OpenZWave

#endif //_HidController_H

