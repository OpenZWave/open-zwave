//-----------------------------------------------------------------------------
//
//	MutexImpl.cpp
//
//	Windows Implementation of the cross-platform mutex
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
#include "MutexImpl.h"


using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<MutexImpl::MutexImpl>
//	Constructor
//-----------------------------------------------------------------------------
MutexImpl::MutexImpl
(
)
{
	InitializeCriticalSection( &m_criticalSection );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::~MutexImpl>
//	Destructor
//-----------------------------------------------------------------------------
MutexImpl::~MutexImpl
(
)
{
	DeleteCriticalSection( &m_criticalSection );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::Lock>
//	Lock the mutex
//-----------------------------------------------------------------------------
bool MutexImpl::Lock
(
	bool const _bWait // = true;
)
{
	if( _bWait )
	{
		// We will wait for the lock
		EnterCriticalSection( &m_criticalSection );
		return true;
	}

	// Returns immediately, even if the lock was not available.
	return( TryEnterCriticalSection( &m_criticalSection ) != 0 );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::Release>
//	Release our lock on the mutex
//-----------------------------------------------------------------------------
void MutexImpl::Release
(
)
{
	LeaveCriticalSection( &m_criticalSection );
}
