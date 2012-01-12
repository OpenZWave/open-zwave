//-----------------------------------------------------------------------------
//
//	Controller.cpp
//
//	Cross-platform, hardware-abstracted controller data interface
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
#include "Controller.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<Controller::Read>
//	Read from a controller
//-----------------------------------------------------------------------------
uint32 Controller::Read
(
	uint8* _buffer,
	uint32 _length
)
{
	// Fetch the data from the ring buffer (which is an all or nothing read)
	if( Get( _buffer, _length ) )
	{
		return _length;
	}

	return 0;
}

