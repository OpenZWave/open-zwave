//
// SerialPortImpl.cpp
//
// POSIX implementation of a cross-platform serial port
//
// Copyright (c) 2010, Greg Satz <satz@iranger.com>
// All rights reserved.
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
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
	m_hSerialPort = -1;
}

//-----------------------------------------------------------------------------
// <SerialPortImpl::~SerialPortImpl>
// Destructor
//-----------------------------------------------------------------------------
SerialPortImpl::~SerialPortImpl
(
)
{
	close( m_hSerialPort );
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

	m_hSerialPort = open( _serialPortName.c_str(), O_RDWR | O_NOCTTY, 0 );

	if( -1 == m_hSerialPort )
	{
		//Error
		Log::Write( "Cannot open serial port %s. Error code %d", _serialPortName.c_str(), errno );
		goto SerialOpenFailure;
	}

	int bits;
	bits = 0;
	ioctl( m_hSerialPort, TIOCMSET, &bits );

	// Configure the serial device parameters
	// Build on the current configuration
	struct termios tios;

	bzero( &tios, sizeof(tios) );
	tcgetattr( m_hSerialPort, &tios );
	switch (_parity)
	{
		case SerialPort::Parity_None:
			tios.c_iflag = IGNPAR;
			break;
		case SerialPort::Parity_Odd:
			tios.c_iflag = INPCK;
			tios.c_cflag = PARENB | PARODD;
			break;
		default:
			Log::Write( "Parity not supported" );
			goto SerialOpenFailure;
	}
	switch (_stopBits)
	{
		case SerialPort::StopBits_One:
			break;		// default
		case SerialPort::StopBits_Two:
			tios.c_cflag |= CSTOPB;
			break;
		default:
			Log::Write( "Stopbits not supported" );
			goto SerialOpenFailure;
	}
	tios.c_iflag |= IGNBRK;
	tios.c_cflag |= CS8 | CREAD | CLOCAL;
	tios.c_oflag = 0;
	tios.c_lflag = 0;
	for( int i = 0; i < NCCS; i++ )
		tios.c_cc[i] = 0;
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;
	switch (_baud)
	{
		case 300:
			cfsetspeed( &tios, B300 );
			break;
		case 1200:
			cfsetspeed( &tios, B1200 );
			break;
		case 2400:
			cfsetspeed( &tios, B2400 );
			break;
		case 4800:
			cfsetspeed( &tios, B4800 );
			break;
		case 9600:
			cfsetspeed( &tios, B9600 );
			break;
		case 19200:
			cfsetspeed( &tios, B19200 );
			break;
		case 38400:
			cfsetspeed( &tios, B38400 );
			break;
		case 57600:
			cfsetspeed( &tios, B57600 );
			break;
#ifdef DARWIN
		case 76800:
			cfsetspeed( &tios, B76800 );
			break;
#endif
		case 115200:
			cfsetspeed( &tios, B115200 );
			break;
		case 230400:
			cfsetspeed( &tios, B230400 );
			break;
		default:
			Log::Write( "Baud rate not supported" );
			goto SerialOpenFailure;
	}
	if ( tcsetattr( m_hSerialPort, TCSANOW, &tios ) == -1 )
	{
		// Error.  Clean up and exit
		Log::Write( "Failed to set serial port parameters" );
		goto SerialOpenFailure;
	}

	tcflush( m_hSerialPort, TCIOFLUSH );

	// Open successful
	return true;

SerialOpenFailure:
 	Log::Write( "Failed to open serial port %s", _serialPortName.c_str() );
	close( m_hSerialPort );
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
	close( m_hSerialPort );
	m_hSerialPort = -1;
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
	if( -1 == m_hSerialPort )
	{
		//Error
		Log::Write( "Error: Serial port must be opened before reading" );
		return 0;
	}

	uint32 bytesRead;
	bytesRead = read( m_hSerialPort, _buffer, _length );
	return bytesRead;
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
	if( -1 == m_hSerialPort )
	{
		//Error
		Log::Write( "Error: Serial port must be opened before writing" );
		return 0;
	}

	// Write the data
	uint32 bytesWritten;
	bytesWritten = write( m_hSerialPort, _buffer, _length);
	return bytesWritten;
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
	if( -1 != m_hSerialPort )
	{
		struct timeval when;
		struct timeval *whenp;
		fd_set rds;
		int err;

		FD_ZERO( &rds );
		FD_SET( m_hSerialPort, &rds );
		if( _timeout == -1 ) // infinite
			whenp = NULL;
		else if( _timeout == 0 ) // immediate
		{
			when.tv_sec = 0;
			when.tv_usec = 0;
			whenp = &when;
		}
		else
		{
			when.tv_sec = _timeout / 1000;
			when.tv_usec = _timeout % 1000 * 1000;
			whenp = &when;
		}
		err = select( FD_SETSIZE, &rds, NULL, NULL, whenp );
		if( err > 0 )
			return true;
		else if( err < 0 )
		{
			Log::Write( "select error %d", errno );
		}
	}

	return false;
}


