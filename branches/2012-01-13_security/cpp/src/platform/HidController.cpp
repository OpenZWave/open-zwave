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

#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "HidController.h"
#include "HidControllerImpl.h"	// Platform-specific implementation of a HID port

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<HidController::HidController>
//	Constructor
//-----------------------------------------------------------------------------
HidController::HidController
(
):
	m_vendorId( 0x1b5f ),	// Wayne Dalton
	m_productId( 0x01 ),	// ControlThink ThinkStick
	m_serialNumber( "" ),
	m_bOpen( false )
{
	m_pImpl = new HidControllerImpl( this );
}

//-----------------------------------------------------------------------------
//	<HidController::~HidController>
//	Destructor
//-----------------------------------------------------------------------------
HidController::~HidController
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//  <HidController::PlayInitSequence>
//  Queues up the controller's initialization commands.
//-----------------------------------------------------------------------------
void HidController::PlayInitSequence
(
	Driver* _driver
)
{
	_driver->SendMsg( new Msg( "FUNC_ID_ZW_GET_VERSION", 0xff, REQUEST, FUNC_ID_ZW_GET_VERSION, false ), Driver::MsgQueue_Command );
    _driver->SendMsg( new Msg( "FUNC_ID_ZW_MEMORY_GET_ID", 0xff, REQUEST, FUNC_ID_ZW_MEMORY_GET_ID, false ), Driver::MsgQueue_Command);
    _driver->SendMsg( new Msg( "FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES", 0xff, REQUEST, FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES, false ), Driver::MsgQueue_Command);
    _driver->SendMsg( new Msg( "FUNC_ID_SERIAL_API_GET_CAPABILITIES", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_CAPABILITIES, false ), Driver::MsgQueue_Command);
    _driver->SendMsg( new Msg( "FUNC_ID_SERIAL_API_GET_INIT_DATA", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_INIT_DATA, false ), Driver::MsgQueue_Command);
}

//-----------------------------------------------------------------------------
//  <HidController::SetVendorId>
//  Set the USB vendor ID search value.  The HID port must be closed for the setting to be accepted.
//-----------------------------------------------------------------------------
bool HidController::SetVendorId
(
    uint32 const _vendorId
)
{
	if( m_bOpen )
	{
		return false;
	}

    m_vendorId = _vendorId;
    return true;
}

//-----------------------------------------------------------------------------
//  <HidController::SetProductId>
//  Set the USB product ID search value.  The HID port must be closed for the setting to be accepted.
//-----------------------------------------------------------------------------
bool HidController::SetProductId
(
    uint32 const _productId
)
{
	if( m_bOpen )
	{
		return false;
	}

    m_productId = _productId;
    return true;
}

//-----------------------------------------------------------------------------
//  <HidController::SetSerialNumber>
//  Set the USB serial number search value.  The HID port must be closed for the setting to be accepted.
//-----------------------------------------------------------------------------
bool HidController::SetSerialNumber
(
    string const& _serialNumber
)
{
	if( m_bOpen )
	{
		return false;
	}

    m_serialNumber = _serialNumber;
    return true;
}

//-----------------------------------------------------------------------------
//	<HidController::Open>
//	Open and configure a HID port
//-----------------------------------------------------------------------------
bool HidController::Open
(
	string const& _hidControllerName
)
{
	if( m_bOpen )
	{
		return false;
	}

	m_hidControllerName = _hidControllerName;
	m_bOpen = m_pImpl->Open( m_hidControllerName, m_vendorId, m_productId, m_serialNumber );
	return m_bOpen;
}

//-----------------------------------------------------------------------------
//	<HidController::Close>
//	Close a HID port
//-----------------------------------------------------------------------------
bool HidController::Close
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
//	<HidController::Write>
//	Write data to an open HID port
//-----------------------------------------------------------------------------
uint32 HidController::Write
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

