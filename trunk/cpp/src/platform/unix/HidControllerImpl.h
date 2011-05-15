//-----------------------------------------------------------------------------
//
//	HidControllerImpl.h
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

#ifndef _HidControllerImpl_H
#define _HidControllerImpl_H

// are these specific to Wayne-Dalton?
#define FEATURE_REPORT_LENGTH 0x40
#define INPUT_REPORT_LENGTH 0x5
#define OUTPUT_REPORT_LENGTH 0x0

#include "Defs.h"
#include "HidController.h"
#include "hidapi.h"

namespace OpenZWave
{
	class HidControllerImpl
	{
	private:
		friend class HidController;

		HidControllerImpl();
		~HidControllerImpl();

		bool Open( string const& _HidControllerName, uint32 const _vendorId, uint32 const _productId, string const& _serialNumber );
		void Close();

		uint32 Read( uint8* _buffer, uint32 _length, IController::ReadPacketSegment _segment );
		uint32 Write( uint8* _buffer, uint32 _length );
		bool Wait( int32 _timeout );


		// helpers for internal use only

		/**
		 * Read bytes from the specified HID feature report
		 * @param _buffer Buffer array for receiving the feature report bytes.
		 * @param _length Length of the buffer array.
		 * @param _reportId ID of the report to read.
		 * @return Actual number of bytes retrieved, or -1 on error.
		 */
		int GetFeatureReport( uint32 _length, uint8 _reportId, uint8* _buffer );

		/**
		 * Write bytes to the specified HID feature report
		 * @param _data Bytes to be written to the feature report.
		 * @param _length Length of bytes to be written.
		 * @return Actal number of bytes written, or -1 on error.
		 */
		int SendFeatureReport( uint32 _length, const uint8* _data );

		hid_device*         m_hHidController;
		bool                m_hidControllerOpen;
		uint8*              m_hidFeatureReportReadBuffer;
		int		    m_hidFeatureReportReadBufferBytes;
		uint8*		    m_hidFeatureReportReadBufferPtr;
	};

} // namespace OpenZWave

#endif //_HidControllerImpl_H

