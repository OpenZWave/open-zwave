//-----------------------------------------------------------------------------
//
//	HidControllerImpl.cpp
//
//	Windows handler for accessing the HID port using HIDAPI.
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

#define CHECK_HIDAPI_RESULT(RESULT, ERRORLABEL) if (RESULT < 0) goto ERRORLABEL
#define PACKET_BUFFER_LENGTH 256

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>

#include "Defs.h"
#include "HidControllerImpl.h"

#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <HidControllerImpl::HidControllerImpl>
// Constructor
//-----------------------------------------------------------------------------
HidControllerImpl::HidControllerImpl
(
) :
  	m_hidControllerOpen( false ),
	m_hidFeatureReportReadBufferPtr( NULL ),
	m_hidFeatureReportReadBuffer( new uint8[PACKET_BUFFER_LENGTH] )
{
}

//-----------------------------------------------------------------------------
// <HidControllerImpl::~HidControllerImpl>
// Destructor
//-----------------------------------------------------------------------------
HidControllerImpl::~HidControllerImpl
(
)
{
	if ( m_hidControllerOpen )
	{
		hid_close( m_hHidController );
	}
	delete m_hidFeatureReportReadBuffer;
}

//-----------------------------------------------------------------------------
// <HidControllerImpl::Open>
// Open the HID port 
//-----------------------------------------------------------------------------
bool HidControllerImpl::Open
( 
	string const& _HidControllerName,
	uint32 const _vendorId,
	uint32 const _productId,
	string const& _serialNumber
)
{
	// HIDAPI result
  	int hidApiResult;
	const uint8 dataOutEnableZwave[3] = { 0x02, 0x01, 0x04 };

	Log::Write( "Open HID port %s", _HidControllerName.c_str() );
	m_hHidController = hid_open( _vendorId, _productId, NULL );
	if ( !m_hHidController )
	{   
		Log::Write( "Cannot find specified HID port with VID:%04hx and PID:0x%04hx.\n", _vendorId, _productId );

		// Enumerate connected HIDs for debugging purposes
		// Note: most OS intentionally hide keyboard/mouse devices from HID access
		struct hid_device_info *devices, *currentDevice;
		devices = hid_enumerate(0x0, 0x0);
		currentDevice = devices;

		Log::Write( "Enumerating connected HIDs:\n" );
		while (currentDevice)
			{
			  Log::Write( "\tVID:%04hx\tPID:0x%04hx\tSN:%ls\tMfg:%ls\tProd:%ls\tPath:%s\n",
				      currentDevice->vendor_id,
				      currentDevice->product_id,
				      currentDevice->serial_number,
				      currentDevice->manufacturer_string,
				      currentDevice->product_string,
				      currentDevice->path);
			  currentDevice = currentDevice->next;
			}
		hid_free_enumeration( devices );

		goto HidOpenFailure;
	}

	wchar_t hidInfoString[255];
	hidInfoString[0] = 0x0000;
   
	Log::Write( "Found HID ZWave controller:");
	Log::Write("\tVendor ID:    0x%04hx", _vendorId);
	Log::Write("\tProduct ID:   0x%04hx", _productId);

	hidApiResult = hid_get_manufacturer_string( m_hHidController, hidInfoString, 255 );
	if ( hidApiResult < 0 )
	{
		Log::Write( "\tManufacturer: <<ERROR READING: 0x%04hx>>", hidApiResult );
	}
	else
	{
      	  	Log::Write( "\tManufacturer: %ls", hidInfoString );
	}

	hidApiResult = hid_get_product_string( m_hHidController, hidInfoString, 255 );
	if (hidApiResult < 0)
	{
	  	Log::Write( "\tProduct name: <<ERROR READING: 0x%04hx>>", hidApiResult );
	}
	else
	{
	  	Log::Write( "\tProduct name: %ls", hidInfoString );
	}

	hidApiResult = hid_get_serial_number_string( m_hHidController, hidInfoString, 255 );
	if (hidApiResult < 0)
	{
        	Log::Write( "\tSerial #:     <<ERROR READING: 0x%04hx>>", hidApiResult );
	}
	else
	{
        	size_t serialLength = wcslen( hidInfoString );
		char* serialHex = new char[serialLength + 1];
		memset( serialHex, 0, serialLength + 1 );
		for ( size_t i = 0; i < serialLength; ++i )
		{
		  	snprintf( &serialHex[i], serialLength - i + 1, "%hx", hidInfoString[i] & 0x0f );
		}
		Log::Write( "\tSerial #:     %ls   --> %s", hidInfoString, serialHex );
	}
	Log::Write("\n");

	// Turn on ZWave data via a short series of feature reports
	uint8 dataIn[FEATURE_REPORT_LENGTH];

	// Get Report ID 2
	// Not sure what the result is for, we don't use it so far
	hidApiResult = GetFeatureReport( FEATURE_REPORT_LENGTH, 0x02, dataIn );
	CHECK_HIDAPI_RESULT( hidApiResult, HidOpenFailure );
    
	// Send Report ID 2 - 1 byte "0x04"
	// Enables ZWave packet reports on ID 4 (tx) and ID 5 (rx)
	hidApiResult = SendFeatureReport( 0x3, dataOutEnableZwave );
	CHECK_HIDAPI_RESULT( hidApiResult, HidOpenFailure );

	// Get Report ID 2
	// Not sure what the result is for, we don't use it so far
	hidApiResult = GetFeatureReport( FEATURE_REPORT_LENGTH, 0x02, dataIn );
	CHECK_HIDAPI_RESULT( hidApiResult, HidOpenFailure );

	// Ensure that reads for input reports are blocked until data arrives.
	// Input report data is polled in Wait() to check if there are feature
	// reports waiting to be retrieved that contain ZWave rx packets.
	hidApiResult = hid_set_nonblocking( m_hHidController, 0 );
	CHECK_HIDAPI_RESULT( hidApiResult, HidOpenFailure );
	
	// Open successful
	m_hidControllerOpen = true;
	return true;

HidOpenFailure:
	Log::Write( "Failed to open HID port %s", _HidControllerName.c_str() );
	const wchar_t* errString = hid_error( m_hHidController );
	Log::Write( "HIDAPI ERROR STRING (if any):\n%ls\n", errString );
	hid_close( m_hHidController );
	return false;
}

//-----------------------------------------------------------------------------
// <HidControllerImpl::Open>
// Close the HID port 
//-----------------------------------------------------------------------------
void HidControllerImpl::Close
( 
)
{
  	hid_close( m_hHidController );
	m_hidControllerOpen = false;
	m_hidFeatureReportReadBufferPtr = NULL;
}

//-----------------------------------------------------------------------------
// <HidControllerImpl::Read>
// Read data from the HID port
//-----------------------------------------------------------------------------
uint32 HidControllerImpl::Read
(
	uint8* _buffer,
	uint32 _length,
	IController::ReadPacketSegment _segment
)
{
  	if( !m_hidControllerOpen )
	{
	  	//Error
		Log::Write( "Error: HID port must be opened before reading\n" );
		return 0;
	}

#ifdef HIDDEBUG
	Log::Write("HID read requested %d", _length);
#endif
	if ( m_hidFeatureReportReadBufferPtr == NULL )
	{
		// Rx feature report buffer should contain
		// [0]      - 0x05 (rx feature report ID)
		// [1]      - length of rx data (or 0x00 and no further bytes if no rx data waiting)
		// [2]      - Z-Wave packet data
		memset( m_hidFeatureReportReadBuffer, 0, PACKET_BUFFER_LENGTH );
		m_hidFeatureReportReadBufferBytes = GetFeatureReport( FEATURE_REPORT_LENGTH, 0x5, m_hidFeatureReportReadBuffer );
		CHECK_HIDAPI_RESULT( m_hidFeatureReportReadBufferBytes, HidPortError );
#ifdef HIDDEBUG
		char nstr[16];
		snprintf( nstr, sizeof(nstr), "%d", m_hidFeatureReportReadBufferBytes );
		string str = "HID read(";
		str += + nstr;
		str += "): ";
		for ( int i = 0; i < m_hidFeatureReportReadBufferBytes; i++ )
		{
			if ( i )
				str += ", ";
			snprintf( nstr, sizeof(nstr), "0x%.2x", m_hidFeatureReportReadBuffer[i] );
			str += nstr;
		}
		Log::Write( str.c_str() );
#endif

		m_hidFeatureReportReadBufferBytes = m_hidFeatureReportReadBuffer[1];
		if ( m_hidFeatureReportReadBufferBytes > 0 ) // non-empty buffer
	    		m_hidFeatureReportReadBufferPtr = &m_hidFeatureReportReadBuffer[2];
	}

	if ( m_hidFeatureReportReadBufferPtr == NULL )
    		return 0;

	if ( _length > m_hidFeatureReportReadBufferBytes )
    		_length = m_hidFeatureReportReadBufferBytes;
	memcpy( _buffer, m_hidFeatureReportReadBufferPtr, _length );
	m_hidFeatureReportReadBufferPtr += _length;
	m_hidFeatureReportReadBufferBytes -= _length;
	if ( m_hidFeatureReportReadBufferBytes == 0 )
    		m_hidFeatureReportReadBufferPtr = NULL;
#ifdef HIDDEBUG
	Log::Write( "HID read returning %d", _length );
#endif
	return _length;
HidPortError:
	//Error
	Log::Write( "Error: HID port returned error reading rest of packet: 0x%08hx, HIDAPI error string:", m_hidFeatureReportReadBufferBytes );
	Log::Write("%ls\n", hid_error(m_hHidController));
	return 0;
}

//-----------------------------------------------------------------------------
// <HidControllerImpl::Write>
// Send data to the HID port
//-----------------------------------------------------------------------------
uint32 HidControllerImpl::Write
(
	uint8* _buffer,
	uint32 _length
)
{
	if( !m_hidControllerOpen )
	{
		//Error
		Log::Write( "Error: HID port must be opened before writing\n" );
		return 0;
	}

	if ( FEATURE_REPORT_LENGTH - 2 < _length)
	  {
		//Error
		Log::Write( "Error: Write buffer length %d exceeded feature report data capacity %d\n",
                    _length,
                    FEATURE_REPORT_LENGTH - 2 );
		return 0;
	  }

	// transmit buffer should be arranged:
	// byte 0 - 0x04 (tx feature report)
	// byte 1 - length of tx data
	// byte 2....  - tx data
	uint8* hidBuffer = new uint8[FEATURE_REPORT_LENGTH];
	memset( hidBuffer, 0, FEATURE_REPORT_LENGTH );
	hidBuffer[0] = 0x4;
	hidBuffer[1] = (uint8)_length;
	memcpy( &hidBuffer[2], _buffer, _length );

	int bytesSent = SendFeatureReport( FEATURE_REPORT_LENGTH, hidBuffer );
	if (bytesSent < 2)
	{
		//Error
        	Log::Write( "Error: HID port returned error sending bytes: 0x%08hx, HIDAPI error string:", bytesSent );
		const wchar_t* errString = hid_error( m_hHidController );
		Log::Write( "%ls\n", errString );
		return 0;
	}

	return (uint32)bytesSent - 2;
}

//-----------------------------------------------------------------------------
//	<HidControllerImpl::Wait>
//	Wait for incoming data to arrive at the HID port
//-----------------------------------------------------------------------------
bool HidControllerImpl::Wait
(
	int32 _timeout
)
{
	if( m_hidControllerOpen )
	{
        	uint8 inputReport[INPUT_REPORT_LENGTH] = { 0 };
		int hidApiResult = 0;  // prime the loop

		// We can know if a feature report is waiting to be read, based on the input data 
		// that is always being sent from the HID port.

		// Wayne-Dalton input report data is structured as follows (best guess):
		// [0] 0x03      - input report ID
		// [1] 0x01      - ??? never changes
		// [2] 0xNN      - if 0x01, no feature reports waiting
		//                 if 0x02, feature report ID 0x05 is waiting to be retrieved
		// [3,4] 0xNNNN  - Number of ZWave rx bytes waiting to be read.
		//                 This value will increase with every input report retrieved
		//                 until the rx data is fully buffered in the controller.
		while ( hidApiResult > -1 )
		{
                	hidApiResult = hid_read(m_hHidController, inputReport, INPUT_REPORT_LENGTH);
			switch (inputReport[2])
			{
				case 0x01:
				{
					// no rx feature reports recieved yet
					break;
				}
				case 0x02:
				{
					// rx feature report data is buffering or available on the HID port
					return true;
					break;
				}
				default:
				{
	                		//Error
					Log::Write( "Error: HID port returned unexpected input report data in byte 2 during Wait(): 0x%08hx\n", inputReport[2] );
					return false;
					break;
				}
			}
			// continue looping until some rx feature report data is reported via input report
		}

        	//Error
		Log::Write( "Error: HID port returned error reading input bytes: 0x%08hx, HIDAPI error string:", hidApiResult );
		const wchar_t* errString = hid_error( m_hHidController );
		Log::Write( "%ls\n", errString );
	}

	return false;
}

//-----------------------------------------------------------------------------
//	<HidControllerImpl::GetFeatureReport>
//	Read bytes from the specified HID feature report
//-----------------------------------------------------------------------------
int HidControllerImpl::GetFeatureReport
(
	uint32 _length,
	uint8  _reportId,
	uint8* _buffer
)
{
	int result;
	_buffer[0] = _reportId;
	result = hid_get_feature_report( m_hHidController, _buffer, _length );
	if (result < 0)
	{
        	Log::Write( "Error: HID GetFeatureReport on ID 0x%hx returned (0x%.8x)\n", _reportId, result );
	}
	return result;
}

//-----------------------------------------------------------------------------
//	<HidControllerImpl::SendFeatureReport>
//	Write bytes to the specified HID feature report
//-----------------------------------------------------------------------------
int HidControllerImpl::SendFeatureReport
(
	uint32 _length,
	const uint8* _data
)
{
	int result;

	result = hid_send_feature_report( m_hHidController, _data, _length );
	if (result < 0)
	{
        	Log::Write( "Error: HID SendFeatureReport on ID 0x%hx returned (0x%.8x)\n", _data[0], result );
	}
	return result;
}
