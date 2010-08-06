//-----------------------------------------------------------------------------
//
//	SerialPortImpl.cpp
//
//	Windows Implementation of the cross-platform serial port
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
#include "SerialPortImpl.h"

#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <SerialPortImpl::SerialPortImpl>
// Constructor
//-----------------------------------------------------------------------------
SerialPortImpl::SerialPortImpl
(
)
{
	// Init an OVERLAPPED object to be used when reading or writing the serial port
	memset( &m_overlapped, 0, sizeof(m_overlapped) );
	m_overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
}

//-----------------------------------------------------------------------------
// <SerialPortImpl::~SerialPortImpl>
// Destructor
//-----------------------------------------------------------------------------
SerialPortImpl::~SerialPortImpl
(
)
{
	CloseHandle( m_hSerialPort );
	CloseHandle( m_overlapped.hEvent );
}

//-----------------------------------------------------------------------------
// <SerialPortImpl::Open>
// Open the serial port 
//-----------------------------------------------------------------------------
bool SerialPortImpl::Open
( 
	string const& _serialPortName,
	uint32 const _baud,
	SerialPort::Parity const _parity,
	SerialPort::StopBits const _stopBits
)
{
	Log::Write( "Open serial port %s", _serialPortName.c_str() );

	m_hSerialPort = CreateFile( _serialPortName.c_str(), 
							 GENERIC_READ | GENERIC_WRITE, 
							 0, 
							 NULL,
							 OPEN_EXISTING,
						 	 FILE_FLAG_OVERLAPPED, 
							 NULL );

	if( INVALID_HANDLE_VALUE == m_hSerialPort )
	{
		//Error
		Log::Write( "Cannot open serial port %s. Error code %d\n", _serialPortName.c_str(), GetLastError() );
		goto SerialOpenFailure;
	}

	// Configure the serial device parameters
	// Build on the current configuration
	DCB dcb;
	if( !GetCommState( m_hSerialPort, &dcb ) )
	{
		//Error.  Clean up and exit
		Log::Write( "Failed to read serial port state" );
		goto SerialOpenFailure;
	}

	// Fill in the Device Control Block
	dcb.BaudRate = (DWORD)_baud;		
	dcb.ByteSize = 8;			
	dcb.Parity = (BYTE)_parity;		
	dcb.StopBits = (BYTE)_stopBits;	
	
	if( !SetCommState( m_hSerialPort, &dcb) )
	{
		//Error. Clean up and exit
		Log::Write( "Failed to set serial port state" );
		goto SerialOpenFailure;
	}

	// Set the timeouts for the serial port
	COMMTIMEOUTS commTimeouts;
	commTimeouts.ReadIntervalTimeout = MAXDWORD;
	commTimeouts.ReadTotalTimeoutConstant = 0;
	commTimeouts.ReadTotalTimeoutMultiplier = 0;
	commTimeouts.WriteTotalTimeoutConstant = 0;
	commTimeouts.WriteTotalTimeoutMultiplier = 0;
	if( !SetCommTimeouts( m_hSerialPort, &commTimeouts ) )
	{
		// Error.  Clean up and exit
		Log::Write( "Failed to set serial port timeouts" );
		goto SerialOpenFailure;
	}

	// Set the serial port to signal when data is received
	if( !SetCommMask( m_hSerialPort, EV_RXCHAR ) )
	{
		//Error.  Clean up and exit
		Log::Write( "Failed to set serial port mask" );
		goto SerialOpenFailure;
	}

	// Clear any residual data from the serial port
	PurgeComm( m_hSerialPort, PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR );
	
	// Open successful
	return true;

SerialOpenFailure:
 	Log::Write( "Failed to open serial port %s", _serialPortName.c_str() );
	CloseHandle( m_hSerialPort );
	return false;
}

//-----------------------------------------------------------------------------
// <SerialPortImpl::Open>
// Close the serial port 
//-----------------------------------------------------------------------------
void SerialPortImpl::Close
( 
)
{
	CloseHandle( m_hSerialPort );
	m_hSerialPort = INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// <SerialPortImpl::Read>
// Read data from the serial port
//-----------------------------------------------------------------------------
uint32 SerialPortImpl::Read
(
	uint8* _buffer,
	uint32 _length
)
{
	if( INVALID_HANDLE_VALUE == m_hSerialPort )
	{
		//Error
		Log::Write( "Error: Serial port must be opened before reading\n" );
		return 0;
	}

	DWORD bytesRead;
	if( !::ReadFile( m_hSerialPort, _buffer, _length, &bytesRead, &m_overlapped ) )
	{
		//Wait for the read to complete
		if( ERROR_IO_PENDING == GetLastError() )
		{
			GetOverlappedResult( m_hSerialPort, &m_overlapped, &bytesRead, TRUE );
		}
	}

	return (uint32)bytesRead;
}

//-----------------------------------------------------------------------------
// <SerialPortImpl::Write>
// Send data to the serial port
//-----------------------------------------------------------------------------
uint32 SerialPortImpl::Write
(
	uint8* _buffer,
	uint32 _length
)
{
	if( INVALID_HANDLE_VALUE == m_hSerialPort )
	{
		//Error
		Log::Write( "Error: Serial port must be opened before writing\n" );
		return 0;
	}

	// Write the data
	DWORD bytesWritten;
	if( !::WriteFile( m_hSerialPort, _buffer, _length, &bytesWritten, &m_overlapped ) )
	{
		//Wait for the write to complete
		if( ERROR_IO_PENDING == GetLastError() )
		{
			GetOverlappedResult( m_hSerialPort, &m_overlapped, &bytesWritten, TRUE );
		}
	}

	return (uint32)bytesWritten;
}

//-----------------------------------------------------------------------------
//	<SerialPortImpl::Wait>
//	Wait for incoming data to arrive at the serial port
//-----------------------------------------------------------------------------
bool SerialPortImpl::Wait
(
	int32 _timeout
)
{
	if( INVALID_HANDLE_VALUE != m_hSerialPort )
	{
		OVERLAPPED overlapped;
		memset( &overlapped, 0, sizeof(overlapped) );
		overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
		
		// Wait for the next rx char event
		DWORD dwEvtMask;
		if( !::WaitCommEvent( m_hSerialPort, &dwEvtMask, &overlapped ) )
		{
			if( ERROR_IO_PENDING == GetLastError() )
			{
				// Wait for either some data to arrive or 
				// the signal that this thread should exit.
				if( WAIT_TIMEOUT == ::WaitForSingleObject( overlapped.hEvent, _timeout ) )
				{
					::CancelIo( m_hSerialPort );
					CloseHandle( overlapped.hEvent );
					return false;
				}
				
				DWORD bytesRead;
				::GetOverlappedResult( m_hSerialPort, &overlapped, &bytesRead, TRUE );
			}
		}

		CloseHandle( overlapped.hEvent );
		return true;

	}

	return false;
}


