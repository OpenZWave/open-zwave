//
// EventImpl.cpp
//
// POSIX implementation of a cross-platform event
//
// Copyright (c) 2010, Greg Satz <satz@iranger.com>
// All rights reserved.
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
#include "EventImpl.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<EventImpl::EventImpl>
//	Constructor
//-----------------------------------------------------------------------------
EventImpl::EventImpl
(
)
{
	// Create a manual reset event
	manual_reset = true;
	is_signaled = false;
	waiting_threads = 0;
	pthread_mutexattr_t ma;
	pthread_mutexattr_init( &ma );
	pthread_mutexattr_settype( &ma, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &lock, &ma );
	pthread_mutexattr_destroy( &ma );
	pthread_condattr_t ca;
	pthread_condattr_init( &ca );
	pthread_condattr_setpshared( &ca, PTHREAD_PROCESS_PRIVATE );
	pthread_cond_init( &condition, &ca );
	pthread_condattr_destroy( &ca );
}

//-----------------------------------------------------------------------------
//	<EventImpl::~EventImpl>
//	Destructor
//-----------------------------------------------------------------------------
EventImpl::~EventImpl
(
)
{
	pthread_mutex_destroy( &lock );
	pthread_cond_destroy( &condition );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Set>
//	Set the event to signalled
//-----------------------------------------------------------------------------
void EventImpl::Set
(
)
{
	pthread_mutex_lock( &lock );
	if( manual_reset )
	{
	  is_signaled = true;
	  pthread_cond_broadcast( &condition );
	}
	else
	{
	  if( waiting_threads == 0 )
	    is_signaled = true;
	  else
	    pthread_cond_signal( &condition );
	}
	pthread_mutex_unlock( &lock );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Reset>
//	Set the event to not signalled
//-----------------------------------------------------------------------------
void EventImpl::Reset
(
)
{
	pthread_mutex_lock (&lock );
	is_signaled = false;
	pthread_mutex_unlock( &lock );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Wait>
//	Wait for the event to become signalled
//-----------------------------------------------------------------------------
bool EventImpl::Wait
(
	int32 const _timeout
)
{
	bool result = true;

	pthread_mutex_lock( &lock );
	if( is_signaled )
	{
		if ( !manual_reset )
			is_signaled = false;
	}
	else
	{
		waiting_threads++;
		if( _timeout > 0 )
		{
			long now;
			struct timespec abstime;

			time(&now);
			abstime.tv_sec = now + _timeout / 1000;
			abstime.tv_nsec = _timeout % 1000 * 1000;
			while( !is_signaled )
			{
				if( pthread_cond_timedwait( &condition, &lock, &abstime ) == ETIMEDOUT )
					result = false;
				else
					result = true;
				break;
			}
		}
		else
		{
			while( !is_signaled )
			{
				pthread_cond_wait( &condition, &lock );
			}
		}
		waiting_threads--;
	}
	pthread_mutex_unlock( &lock );
	return result;
}

