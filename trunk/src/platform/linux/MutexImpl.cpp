//-----------------------------------------------------------------------------
//
//	MutexImpl.cpp
//
//	Linux implementation of the cross-platform mutex
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
	// Create attributes for a recursive mutex
	pthread_mutexattr_t mta;
	pthread_mutexattr_init( &mta );
	pthread_mutexattr_settype( &mta, PTHREAD_MUTEX_RECURSIVE );

	// Create the mutex
	pthread_mutex_init( &m_mutex, &mta );
	
	// Cleanup
	pthread_mutexattr_destroy( &mta );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::~MutexImpl>
//	Destructor
//-----------------------------------------------------------------------------
MutexImpl::~MutexImpl
(
)
{
	pthread_mutex_destroy( &m_mutex );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::Lock>
//	Lock the mutex.  Returns true if successful.
//-----------------------------------------------------------------------------
bool MutexImpl::Lock
(
	bool const _bWait // = true;
)
{
	if( _bWait )
	{
		// We will wait for the lock
		pthread_mutex_lock( &m_mutex );
		return true;
	}

	// Returns immediately, even if the lock was not available.
	return( !pthread_mutex_trylock( &m_mutex ) );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::Release>
//	Release our lock on the mutex
//-----------------------------------------------------------------------------
void MutexImpl::Release
(
)
{
	pthread_mutex_unlock( &m_mutex );
}
