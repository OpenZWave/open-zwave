//
// SerialControllerImpl.h
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

#ifndef _SerialControllerImpl_H
#define _SerialControllerImpl_H

#include <strings.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "Defs.h"
#include "SerialController.h"

// Use a less general name than 'DEBUG' as this is just bound to clash
//#define OZW_DEBUG

namespace OpenZWave
{
	class SerialControllerImpl
	{
	private:
		friend class SerialController;

		SerialControllerImpl();
		~SerialControllerImpl();

		static bool FindUSB( string &usbdevice );
		bool Open( string const& _SerialControllerName, uint32 const _baud, SerialController::Parity const _parity, SerialController::StopBits const _stopBits );
		void Close();

		uint32 Read( uint8* _buffer, uint32 _length, IController::ReadPacketSegment _segment );
		uint32 Write( uint8* _buffer, uint32 _length );
		bool Wait( int32 _timeout );

		int m_hSerialController;

#ifdef OZW_DEBUG
		int m_hdebug;
#endif
	};

} // namespace OpenZWave

#endif //_SerialControllerImpl_H

