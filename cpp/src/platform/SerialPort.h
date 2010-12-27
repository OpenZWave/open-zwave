//-----------------------------------------------------------------------------
//
//	SerialPort.h
//
//	Cross-platform serial port handler
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

#ifndef _SerialPort_H
#define _SerialPort_H

#include <string>
#include "Defs.h"

namespace OpenZWave
{
	class SerialPortImpl;

	/** \brief Implements a platform-independent serial port access class.
	 */
	class SerialPort
	{
	public:
		enum Parity
		{
			Parity_None = 0,
			Parity_Odd,
			Parity_Even,
			Parity_Mark,
			Parity_Space
		};

		enum StopBits
		{
			StopBits_One = 0,
			StopBits_OneAndAHalf = 1,
			StopBits_Two = 2
		};

		/**
		 * Constructor.
		 * Creates an object that represents a serial port.
		 */
		SerialPort();

		/**
		 * Destructor.
		 * Destroys the serial port object.
		 */
		~SerialPort();

		/**
		 * Open a serial port.
		 * Attempts to open a serial port and initialize it with the specified paramters.
		 * @param _serialPortName The name of the port to open.  For example, ttyS1 on Linux, or \\.\COM2 in Windows.
		 * @param _baud Integer containing the baud-rate of the serial connection.  Most Z-Wave interfaces run at 115200 baud.
		 * @param _parity Boolean set to true if there the data contains a parity bits.
 		 * @param _stopBits Integer containing the number of stop-bits, usually one or two.
		 * @return True if the port was opened and configured successfully.
		 * @see Close, Read, Write
		 */
		bool Open( string const& _serialPortName, uint32 const _baud, Parity const _parity, StopBits const _stopBits );

		/**
		 * Close a serial port.
		 * Closes the serial port.
		 * @return True if the port was closed successfully, or false if the port was already closed, or an error occurred.
		 * @see Open
		 */
		bool Close();

		/**
		 * Read from a serial port.
		 * Attempts to read data from an open serial port.
		 * @param _buffer Pointer to a block of memory large enough to hold the requested data.
		 * @param _length Length in bytes of the data to be read.
		 * @return The number of bytes read.
		 * @see Write, Open, Close
		 */
		uint32 Read( uint8* _buffer, uint32 _length );

		/**
		 * Write to a serial port.
		 * Attempts to write data to an open serial port.
		 * @param _buffer Pointer to a block of memory containing the data to be written.
		 * @param _length Length in bytes of the data.
		 * @return The number of bytes written.
		 * @see Read, Open, Close
		 */
		uint32 Write( uint8* _buffer, uint32 _length );

		/**
		 * Waits for data to arrive at the serial port
		 * @param _timeout maximum time in milliseconds to wait for the event
		 * to become signalled. If the timeout is zero, the method will 
		 * return immediately.  If the timeout is Event::Timeout_Infinite, the 
		 * method will not return until the event is signalled.
		 * @return true if data is available, false if the wait timed out.
		 * @see Set, Reset
		 */
		bool Wait( int32 _timeout );

	private:
		SerialPortImpl*	m_pImpl;	// Pointer to an object that encapsulates the platform-specific implementation of the serial port.
		bool			m_bOpen;
	};

} // namespace OpenZWave

#endif //_SerialPort_H

