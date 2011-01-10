//-----------------------------------------------------------------------------
//
//	IController.h
//
//	Cross-platform, hardware-abstracted controller data interface
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

#ifndef _IController_H
#define _IController_H

#include <string>
#include "Defs.h"

namespace OpenZWave
{
    class IController
	{
	public:
		/**
		 * Destructor.
		 * Destroys the controller object.
		 */
        virtual ~IController() {};

		/**
		 * Open a controller.
		 * Attempts to open a controller and initialize it with the specified paramters.
		 * @param _controllerName The name of the port to open.  For example, ttyS1 on Linux, or \\.\COM2 in Windows.
		 * @see Close, Read, Write
		 */
		virtual bool Open( string const& _controllerName ) = 0;

		/**
		 * Close a controller.
		 * Closes the controller.
		 * @return True if the controller was closed successfully, or false if the controller was already closed, or an error occurred.
		 * @see Open
		 */
		virtual bool Close() = 0;

		/**
		 * Read from a controller.
		 * Attempts to read data from an open controller.
		 * @param _buffer Pointer to a block of memory large enough to hold the requested data.
		 * @param _length Length in bytes of the data to be read.
		 * @return The number of bytes read.
		 * @see Write, Open, Close
		 */
		virtual uint32 Read( uint8* _buffer, uint32 _length ) = 0;

		/**
		 * Write to a controller.
		 * Attempts to write data to an open controller.
		 * @param _buffer Pointer to a block of memory containing the data to be written.
		 * @param _length Length in bytes of the data.
		 * @return The number of bytes written.
		 * @see Read, Open, Close
		 */
		virtual uint32 Write( uint8* _buffer, uint32 _length ) = 0;

		/**
		 * Waits for data to arrive at the controller
		 * @param _timeout maximum time in milliseconds to wait for the event
		 * to become signalled. If the timeout is zero, the method will 
		 * return immediately.  If the timeout is Event::Timeout_Infinite, the 
		 * method will not return until the event is signalled.
		 * @return true if data is available, false if the wait timed out.
		 * @see Set, Reset
		 */
		virtual bool Wait( int32 _timeout ) = 0;
	};

} // namespace OpenZWave

#endif //_IController_H

