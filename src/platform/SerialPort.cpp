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

#include "Defs.h"
#include "SerialPort.h"

#include "SerialPortImpl.h"	// Platform-specific implementation of a serial port

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<SerialPort::SerialPort>
//	Constructor
//-----------------------------------------------------------------------------
SerialPort::SerialPort
(
):
	m_pImpl( new SerialPortImpl() ),
	m_bOpen( false )
{
}

//-----------------------------------------------------------------------------
//	<SerialPort::~SerialPort>
//	Destructor
//-----------------------------------------------------------------------------
SerialPort::~SerialPort
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//	<SerialPort::Open>
//	Open and configure a serial port
//-----------------------------------------------------------------------------
bool SerialPort::Open
(
	string const& _serialPortName,
	uint32 const _baud,
	Parity const _parity,
	StopBits const _stopBits
)
{
	if( m_bOpen )
	{
		return false;
	}

	m_bOpen = m_pImpl->Open( _serialPortName, _baud, _parity, _stopBits );

	// Create a thread to watch for incoming data

	return m_bOpen;
}

//-----------------------------------------------------------------------------
//	<SerialPort::Close>
//	Close a serial port
//-----------------------------------------------------------------------------
bool SerialPort::Close
(
)
{
	if( !m_bOpen )
	{
		return false;
	}

	m_pImpl->Close();
	m_bOpen = false;
	return true;
}

//-----------------------------------------------------------------------------
//	<SerialPort::Read>
//	Read data from an open serial port
//-----------------------------------------------------------------------------
uint32 SerialPort::Read
(
	uint8* _buffer,
	uint32 _length
)
{
	if( !m_bOpen )
	{
		return 0;
	}

	return( m_pImpl->Read( _buffer, _length ) );
}

//-----------------------------------------------------------------------------
//	<SerialPort::Write>
//	Write data to an open serial port
//-----------------------------------------------------------------------------
uint32 SerialPort::Write
(
	uint8* _buffer,
	uint32 _length
)
{
	if( !m_bOpen )
	{
		return 0;
	}

	return( m_pImpl->Write( _buffer, _length ) );
}

//-----------------------------------------------------------------------------
//	<SerialPort::Wait>
//	Wait for incoming data to arrive at the serial port
//-----------------------------------------------------------------------------
bool SerialPort::Wait
(
	int32 _timeout
)
{
	if( !m_bOpen )
	{
		return false;
	}

	return( m_pImpl->Wait( _timeout ) );
}

