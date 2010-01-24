//-----------------------------------------------------------------------------
//
//	Mutex.cpp
//
//	Cross-platform mutex
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
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
#include "Mutex.h"

#include "MutexImpl.h"	// Platform-specific implementation of a mutex


using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<Mutex::Mutex>
//	Constructor
//-----------------------------------------------------------------------------
Mutex::Mutex
(
):
	m_pImpl( new MutexImpl() )
{
}

//-----------------------------------------------------------------------------
//	<Mutex::~Mutex>
//	Destructor
//-----------------------------------------------------------------------------
Mutex::~Mutex
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//	<Mutex::Lock>
//	Lock the mutex
//-----------------------------------------------------------------------------
bool Mutex::Lock
(
	bool const _bWait // = true;
)
{
	return m_pImpl->Lock( _bWait );
}

//-----------------------------------------------------------------------------
//	<Mutex::Release>
//	Release our lock on the mutex
//-----------------------------------------------------------------------------
void Mutex::Release
(
)
{
	m_pImpl->Release();
}
