//
// SerialControllerImpl.cpp
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

#ifdef __linux__
#include <libudev.h>
#endif

using namespace OpenZWave;

static void SerialReadThreadEntryPoint(	Event* _exitEvent, void* _context );

//-----------------------------------------------------------------------------
// <SerialControllerImpl::SerialControllerImpl>
// Constructor
//-----------------------------------------------------------------------------
SerialControllerImpl::SerialControllerImpl
(
	SerialController* _owner
):
	m_owner( _owner ),
	m_exit( false )
{
	m_hSerialController = -1;
#ifdef OZW_DEBUG
	m_hdebug = -1;
#endif
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::~SerialControllerImpl>
// Destructor
//-----------------------------------------------------------------------------
SerialControllerImpl::~SerialControllerImpl
(
)
{
	flock(m_hSerialController, LOCK_UN);
    if(m_hSerialController >= 0)
	close( m_hSerialController );
	m_exit = true;
#ifdef OZW_DEBUG
    if(m_hdebug >= 0)
	close( m_hdebug );
#endif
}


#ifdef __linux__
bool SerialControllerImpl::FindUSB
(
	string &usbdevice
)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev, *parent;
	bool found = false;
	
	string vendor  = usbdevice.substr(0, 4);
 	string product = usbdevice.substr(5, 4);
	
	udev = udev_new();
	if (!udev) {
		Log::Write("Can't create udev");
		return false;
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "tty");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		
		parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
		
		if (parent) {
			Log::Write("USB: %s %s", udev_device_get_sysattr_value(parent, "idVendor"),
								               udev_device_get_sysattr_value(parent, "idProduct"));
			if (strcasecmp(udev_device_get_sysattr_value(parent, "idVendor"), vendor.c_str()) == 0 &&
				  strcasecmp(udev_device_get_sysattr_value(parent, "idProduct"), product.c_str()) == 0) {
				Log::Write("Found USB serial device at %s", udev_device_get_devnode(dev));
				usbdevice = udev_device_get_devnode(dev);
				found = true;
			}
		}
		udev_device_unref(dev);
		
		if (found) break;
	}
	
	udev_enumerate_unref(enumerate);

	udev_unref(udev);
	
	if (!found) {
		Log::Write("No serial USB device matching %s", usbdevice.c_str());
	}
	
	return found;
}
#endif


//-----------------------------------------------------------------------------
// <SerialControllerImpl::Open>
// Open the serial port 
//-----------------------------------------------------------------------------
bool SerialControllerImpl::Open
(
)
{
	// Try to init the serial port
	if( !Init( 1 ) )
	{
		// Failed.  We bail to allow the app a chance to take over, rather than retry
		// automatically.  Automatic retries only occur after a successful init.
		return false;
	}

  Log::Write("start serial thread");

	m_serialThread = new Thread( "serial" );
	m_serialThread->Start( SerialReadThreadEntryPoint, this);

	return true;
}
	
	
bool SerialControllerImpl::Init
(
	uint32 const _attempts
)
{  
	string device = m_owner->m_serialControllerName;
	
	Log::Write( "Open serial port %s",device.c_str() );
	
#ifdef __linux__
	if (device.find(':') == 4) {
		if (!FindUSB(device)) {
			return false;
		}
	}
#endif
	
	
	m_hSerialController = open( device.c_str(), O_RDWR | O_NOCTTY, 0 );

	if( -1 == m_hSerialController )
	{
		//Error
		Log::Write( "Cannot open serial port %s. Error code %d", device.c_str(), errno );
		goto SerialOpenFailure;
	}

	if( flock( m_hSerialController, LOCK_EX) == -1 )
	{
		Log::Write( "Cannot get exclusive lock for serial port %s. Error code %d", device.c_str(), errno );
	}

	int bits;
	bits = 0;
	ioctl( m_hSerialController, TIOCMSET, &bits );

	// Configure the serial device parameters
	// Build on the current configuration
	struct termios tios;

	bzero( &tios, sizeof(tios) );
	tcgetattr( m_hSerialController, &tios );
	switch (m_owner->m_parity)
	{
		case SerialController::Parity_None:
			tios.c_iflag = IGNPAR;
			break;
		case SerialController::Parity_Odd:
			tios.c_iflag = INPCK;
			tios.c_cflag = PARENB | PARODD;
			break;
		default:
			Log::Write( "Parity not supported" );
			goto SerialOpenFailure;
	}
	switch (m_owner->m_stopBits)
	{
		case SerialController::StopBits_One:
			break;		// default
		case SerialController::StopBits_Two:
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
	tios.c_cc[VTIME] = 1;
	switch (m_owner->m_baud)
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
	if ( tcsetattr( m_hSerialController, TCSANOW, &tios ) == -1 )
	{
		// Error.  Clean up and exit
		Log::Write( "Failed to set serial port parameters" );
		goto SerialOpenFailure;
	}

	tcflush( m_hSerialController, TCIOFLUSH );

	// Open successful
#ifdef OZW_DEBUG
	m_hdebug = open("data.log", O_WRONLY|O_CREAT, 0666);
#endif
	return true;

SerialOpenFailure:
 	Log::Write( "Failed to open serial port %s", device.c_str() );
    if(m_hSerialController >= 0)
	close( m_hSerialController );
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
	close( m_hSerialController );
	m_hSerialController = -1;
#ifdef OZW_DEBUG
	close( m_hdebug );
	m_hdebug = -1;
#endif
	m_serialThread->Stop();
	m_serialThread->Release();
}

//-----------------------------------------------------------------------------
// <SerialReadThreadEntryPoint>
// Entry point of the thread for receiving data from the serial port
//-----------------------------------------------------------------------------
static void SerialReadThreadEntryPoint
(
	Event* _exitEvent,
	void* _context
)
{
	SerialControllerImpl* impl = (SerialControllerImpl*)_context;
	if( impl )
	{
		impl->ReadThreadProc( _exitEvent );
	}
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::ReadThreadProc>
// Handle receiving data
//-----------------------------------------------------------------------------
void SerialControllerImpl::ReadThreadProc
(
	Event *_exitEvent
)
{
	uint32 attempts = 0;
	while( true )
	{
		// Init must have been called successfully during Open, so we
		// don't do it again until the end of the loop
		if(m_hSerialController != -1)
		{
			// Enter read loop.  Call will only return if
			// an exit is requested or an error occurs
			Read();

			// Reset the attempts, so we get a rapid retry for temporary errors
			attempts = 0;
		}

		if( attempts < 25 )		
		{
			sleep(5);
		}
		else
		{
			sleep(30);
		}

		Init( ++attempts );
	}
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Read>
// Read data from the serial port
//-----------------------------------------------------------------------------
void SerialControllerImpl::Read
(
)
{
	uint8 buffer[256];
	
	Log::Write("read");
	
	while (!m_exit) {
	
		if (!Wait(5000)) continue;
		
		int32 bytesRead;
		bytesRead = read( m_hSerialController, buffer, sizeof(buffer) );
#ifdef OZW_DEBUG
		if ( m_hdebug >= 0 && bytesRead > 0 )
		{
			unsigned int now = htonl(time(NULL));
			unsigned char c = (char)bytesRead;
			write( m_hdebug, "r", 1 );
			write( m_hdebug, &c, 1);
			write( m_hdebug, &now, sizeof(now));
			write( m_hdebug, buffer, bytesRead);
		}
#endif

		if (bytesRead > 0) {
			m_owner->Put( buffer, bytesRead);
		}
	}

 Log::Write("read done");
	//return bytesRead;
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
	if( -1 == m_hSerialController )
	{
		//Error
		Log::Write( "Error: Serial port must be opened before writing" );
		return 0;
	}

	// Write the data
	uint32 bytesWritten;
	bytesWritten = write( m_hSerialController, _buffer, _length);
#ifdef OZW_DEBUG
	if ( m_hdebug >= 0 )
	{
		unsigned int now = htonl(time(NULL));
		unsigned char c = (char)bytesWritten;
		write( m_hdebug, "w", 1 );
		write( m_hdebug, &c, 1);
		write( m_hdebug, &now, sizeof(now));
		write( m_hdebug, _buffer, bytesWritten);
	}
#endif
	return bytesWritten;
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
	if( -1 != m_hSerialController )
	{
		struct timeval when;
		struct timeval *whenp;
		fd_set rds, eds;
		int err;

		FD_ZERO( &rds );
		FD_SET( m_hSerialController, &rds );
		FD_ZERO( &eds );
		FD_SET( m_hSerialController, &eds );
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

		int oldstate;
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

		err = select( m_hSerialController + 1, &rds, NULL, &eds, whenp );

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

		if( err > 0 )
			return true;
		else if( err < 0 )
		{
			//Log::Write( "select error %d", errno );
		}
	}

	return false;
}
