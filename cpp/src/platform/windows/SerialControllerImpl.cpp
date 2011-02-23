//-----------------------------------------------------------------------------
//
//	SerialControllerImpl.cpp
//
//	Windows Implementation of the cross-platform serial port
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

#include "Defs.h"
#include "SerialControllerImpl.h"

#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <SerialControllerImpl::SerialControllerImpl>
// Constructor
//-----------------------------------------------------------------------------
SerialControllerImpl::SerialControllerImpl
(
)
{
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::~SerialControllerImpl>
// Destructor
//-----------------------------------------------------------------------------
SerialControllerImpl::~SerialControllerImpl
(
)
{
	CloseHandle( m_hSerialController );
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Open>
// Open the serial port 
//-----------------------------------------------------------------------------
bool SerialControllerImpl::Open
( 
	string const& _SerialControllerName,
	uint32 const _baud,
	SerialController::Parity const _parity,
	SerialController::StopBits const _stopBits
)
{
	Log::Write( "  Open serial port %s", _SerialControllerName.c_str() );

	m_hSerialController = CreateFile( _SerialControllerName.c_str(), 
							 GENERIC_READ | GENERIC_WRITE, 
							 0, 
							 NULL,
							 OPEN_EXISTING,
						 	 FILE_FLAG_OVERLAPPED, 
							 NULL );

	if( INVALID_HANDLE_VALUE == m_hSerialController )
	{
		//Error
		Log::Write( "Cannot open serial port %s. Error code %d\n", _SerialControllerName.c_str(), GetLastError() );
		goto SerialOpenFailure;
	}

	// Configure the serial device parameters
	// Build on the current configuration
	DCB dcb;
	if( !GetCommState( m_hSerialController, &dcb ) )
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
	
	if( !SetCommState( m_hSerialController, &dcb) )
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
	if( !SetCommTimeouts( m_hSerialController, &commTimeouts ) )
	{
		// Error.  Clean up and exit
		Log::Write( "Failed to set serial port timeouts" );
		goto SerialOpenFailure;
	}

	// Set the serial port to signal when data is received
	if( !SetCommMask( m_hSerialController, EV_RXCHAR ) )
	{
		//Error.  Clean up and exit
		Log::Write( "Failed to set serial port mask" );
		goto SerialOpenFailure;
	}

	// Clear any residual data from the serial port
	PurgeComm( m_hSerialController, PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR );
	
	// Open successful
	return true;

SerialOpenFailure:
 	Log::Write( "Failed to open serial port %s", _SerialControllerName.c_str() );
	CloseHandle( m_hSerialController );
	return false;
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Open>
// Close the serial port 
//-----------------------------------------------------------------------------
void SerialControllerImpl::Close
( 
)
{
	CloseHandle( m_hSerialController );
	m_hSerialController = INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Read>
// Read data from the serial port
//-----------------------------------------------------------------------------
uint32 SerialControllerImpl::Read
(
	uint8* _buffer,
	uint32 _length,
    IController::ReadPacketSegment _segment
)
{
	if( INVALID_HANDLE_VALUE == m_hSerialController )
	{
		//Error
		Log::Write( "Error: Serial port must be opened before reading\n" );
		return 0;
	}

	OVERLAPPED overlapped;
	memset( &overlapped, 0, sizeof(overlapped) );
	overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	DWORD bytesRead;
	if( !::ReadFile( m_hSerialController, _buffer, _length, &bytesRead, &overlapped ) )
	{
		//Wait for the read to complete
		if( ERROR_IO_PENDING == GetLastError() )
		{
			GetOverlappedResult( m_hSerialController, &overlapped, &bytesRead, TRUE );
		}
		else
		{
			Log::Write( "Error: Serial port read (0x%.8x)", GetLastError() );
		}
	}

	CloseHandle( overlapped.hEvent );
	return (uint32)bytesRead;
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Write>
// Send data to the serial port
//-----------------------------------------------------------------------------
uint32 SerialControllerImpl::Write
(
	uint8* _buffer,
	uint32 _length
)
{
	if( INVALID_HANDLE_VALUE == m_hSerialController )
	{
		//Error
		Log::Write( "Error: Serial port must be opened before writing\n" );
		return 0;
	}

	// Write the data
	OVERLAPPED overlapped;
	memset( &overlapped, 0, sizeof(overlapped) );
	overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	DWORD bytesWritten;
	if( !::WriteFile( m_hSerialController, _buffer, _length, &bytesWritten, &overlapped ) )
	{
		//Wait for the write to complete
		if( ERROR_IO_PENDING == GetLastError() )
		{
			GetOverlappedResult( m_hSerialController, &overlapped, &bytesWritten, TRUE );
		}
		else
		{
			Log::Write( "Error: Serial port write (0x%.8x)", GetLastError() );
		}
	}

	CloseHandle( overlapped.hEvent );
	return (uint32)bytesWritten;
}

//-----------------------------------------------------------------------------
//	<SerialControllerImpl::Wait>
//	Wait for incoming data to arrive at the serial port
//-----------------------------------------------------------------------------
bool SerialControllerImpl::Wait
(
	int32 _timeout
)
{
	if( INVALID_HANDLE_VALUE != m_hSerialController )
	{
		OVERLAPPED overlapped;
		memset( &overlapped, 0, sizeof(overlapped) );
		overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
		
		// Wait for the next rx char event
		DWORD dwEvtMask;
		if( !::WaitCommEvent( m_hSerialController, &dwEvtMask, &overlapped ) )
		{
			if( ERROR_IO_PENDING == GetLastError() )
			{
				// Wait for some data to arrive
				if( WAIT_TIMEOUT == ::WaitForSingleObject( overlapped.hEvent, _timeout ) )
				{
					::CancelIo( m_hSerialController );
					CloseHandle( overlapped.hEvent );
					return false;
				}
				
				DWORD bytesRead;
				::GetOverlappedResult( m_hSerialController, &overlapped, &bytesRead, TRUE );
			}
		}

		CloseHandle( overlapped.hEvent );
		return true;

	}

	return false;
}

