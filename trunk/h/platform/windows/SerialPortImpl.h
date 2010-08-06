//-----------------------------------------------------------------------------
//
//	SerialPortImpl.h
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

#ifndef _SerialPortImpl_H
#define _SerialPortImpl_H

#include <Windows.h>

#include "Defs.h"
#include "SerialPort.h"

#define DEBUG

namespace OpenZWave
{
	class SerialPortImpl
	{
	private:
		friend class SerialPort;

		SerialPortImpl();
		~SerialPortImpl();

		bool Open( string const& _SerialPortName, uint32 const _baud, SerialPort::Parity const _parity, SerialPort::StopBits const _stopBits );
		void Close();

		uint32 Read( uint8* _buffer, uint32 _length );
		uint32 Write( uint8* _buffer, uint32 _length );
		bool Wait( int32 _timeout );

		HANDLE				m_hSerialPort;
		OVERLAPPED			m_overlapped;
	};

} // namespace OpenZWave

#endif //_SerialPortImpl_H

