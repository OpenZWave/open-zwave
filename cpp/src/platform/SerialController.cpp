//-----------------------------------------------------------------------------
//
//	SerialController.h
//
//	Cross-platform serial port handler
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
#include "Msg.h"
#include "SerialController.h"

#include "SerialControllerImpl.h"	// Platform-specific implementation of a serial port

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<SerialController::SerialController>
//	Constructor
//-----------------------------------------------------------------------------
SerialController::SerialController
(
):
	m_pImpl( new SerialControllerImpl() ),
	m_bOpen( false ),
    m_baud ( 115200 ),
    m_parity ( SerialController::Parity_None ),
    m_stopBits ( SerialController::StopBits_One ),
    m_pMsgInitializationSequence( new list<Msg*> )
{
    m_pMsgInitializationSequence->push_back(new Msg( "FUNC_ID_ZW_GET_VERSION", 0xff, REQUEST, FUNC_ID_ZW_GET_VERSION, false ));
    m_pMsgInitializationSequence->push_back(new Msg( "FUNC_ID_ZW_MEMORY_GET_ID", 0xff, REQUEST, FUNC_ID_ZW_MEMORY_GET_ID, false ));
    m_pMsgInitializationSequence->push_back(new Msg( "FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES", 0xff, REQUEST, FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES, false ));
    m_pMsgInitializationSequence->push_back(new Msg( "FUNC_ID_SERIAL_API_GET_CAPABILITIES", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_CAPABILITIES, false ));
    //m_pMsgInitializationSequence->push_back(new Msg( "FUNC_ID_ZW_GET_SUC_NODE_ID", 0xff, REQUEST, FUNC_ID_ZW_GET_SUC_NODE_ID, false ));
    m_pMsgInitializationSequence->push_back(new Msg( "FUNC_ID_SERIAL_API_GET_INIT_DATA", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false ));
}

//-----------------------------------------------------------------------------
//	<SerialController::~SerialController>
//	Destructor
//-----------------------------------------------------------------------------
SerialController::~SerialController
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//  <SerialController::GetMsgInitializationSequence>
//  Retrieves an array of Msg object pointers in the correct order needed to initialize the SerialController implementation.
//-----------------------------------------------------------------------------
list<Msg*>* const SerialController::GetMsgInitializationSequence
(
)
{
	return m_pMsgInitializationSequence;
}

//-----------------------------------------------------------------------------
//  <SerialController::SetBaud>
//  Set the serial port baud rate.  The serial port must be closed for the setting to be accepted.
//-----------------------------------------------------------------------------
bool SerialController::SetBaud
(
    uint32 const _baud
)
{
	if( m_bOpen )
	{
		return false;
	}

    m_baud = _baud;
    return true;
}

//-----------------------------------------------------------------------------
//  <SerialController::SetParity>
//  Set the serial port parity.  The serial port must be closed for the setting to be accepted.
//-----------------------------------------------------------------------------
bool SerialController::SetParity
(
    Parity const _parity
)
{
	if( m_bOpen )
	{
		return false;
	}

    m_parity = _parity;
    return true;
}

//-----------------------------------------------------------------------------
//  <SerialController::SetStopBits>
//  Set the serial port stop bits.  The serial port must be closed for the setting to be accepted.
//-----------------------------------------------------------------------------
bool SerialController::SetStopBits
(
    StopBits const _stopBits
)
{
	if( m_bOpen )
	{
		return false;
	}

    m_stopBits = _stopBits;
    return true;
}

//-----------------------------------------------------------------------------
//	<SerialController::Open>
//	Open and configure a serial port
//-----------------------------------------------------------------------------
bool SerialController::Open
(
	string const& _SerialControllerName /*,
	uint32 const _baud,
	Parity const _parity,
	StopBits const _stopBits */
)
{
	if( m_bOpen )
	{
		return false;
	}

	m_bOpen = m_pImpl->Open( _SerialControllerName, m_baud, m_parity, m_stopBits );

	// Create a thread to watch for incoming data

	return m_bOpen;
}

//-----------------------------------------------------------------------------
//	<SerialController::Close>
//	Close a serial port
//-----------------------------------------------------------------------------
bool SerialController::Close
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
//	<SerialController::Read>
//	Read data from an open serial port
//-----------------------------------------------------------------------------
uint32 SerialController::Read
(
	uint8* _buffer,
	uint32 _length,
    ReadPacketSegment _segment
)
{
	if( !m_bOpen )
	{
		return 0;
	}

	return( m_pImpl->Read( _buffer, _length, _segment ) );
}

//-----------------------------------------------------------------------------
//	<SerialController::Write>
//	Write data to an open serial port
//-----------------------------------------------------------------------------
uint32 SerialController::Write
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
//	<SerialController::Wait>
//	Wait for incoming data to arrive at the serial port
//-----------------------------------------------------------------------------
bool SerialController::Wait
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

