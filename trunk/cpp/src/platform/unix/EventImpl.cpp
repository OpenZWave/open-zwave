//-----------------------------------------------------------------------------
//
//	EventImpl.cpp
//
//	POSIX implementation of a cross-platform event
//
//	Copyright (c) 2010, Greg Satz <satz@iranger.com>
//	All rights reserved.
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
#include "EventImpl.h"

#include <sys/time.h>

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<EventImpl::EventImpl>
//	Constructor
//-----------------------------------------------------------------------------
EventImpl::EventImpl
(
):
	m_manualReset( true ),
	m_isSignaled( false ),
	m_waitingThreads( 0 )
{
	pthread_mutexattr_t ma;
	pthread_mutexattr_init( &ma );
	pthread_mutexattr_settype( &ma, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &m_lock, &ma );
	pthread_mutexattr_destroy( &ma );
	
	pthread_condattr_t ca;
	pthread_condattr_init( &ca );
	pthread_condattr_setpshared( &ca, PTHREAD_PROCESS_PRIVATE );
	pthread_cond_init( &m_condition, &ca );
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
	pthread_mutex_destroy( &m_lock );
	pthread_cond_destroy( &m_condition );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Set>
//	Set the event to signalled
//-----------------------------------------------------------------------------
void EventImpl::Set
(
)
{
	pthread_mutex_lock( &m_lock );
	if( m_manualReset )
	{
		m_isSignaled = true;
		pthread_cond_broadcast( &m_condition );
	}
	else
	{
		if( !m_waitingThreads )
		{
			m_isSignaled = true;
		}
		else
		{
			pthread_cond_signal( &m_condition );
		}
	}
	pthread_mutex_unlock( &m_lock );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Reset>
//	Set the event to not signalled
//-----------------------------------------------------------------------------
void EventImpl::Reset
(
)
{
	pthread_mutex_lock ( &m_lock );
	m_isSignaled = false;
	pthread_mutex_unlock( &m_lock );
}

//-----------------------------------------------------------------------------
//	<EventImpl::IsSignalled>
//	Test whether the event is set
//-----------------------------------------------------------------------------
bool EventImpl::IsSignalled
(
)
{
	return m_isSignaled;
}

//-----------------------------------------------------------------------------
//	<EventImpl::Wait>
//	Wait for the event to become signalled
//-----------------------------------------------------------------------------
bool EventImpl::Wait
(
	int32 const _timeout /* milliseconds */
)
{
	bool result = true;

	pthread_mutex_lock( &m_lock );
	if( m_isSignaled )
	{
		if ( !m_manualReset )
		{
			m_isSignaled = false;
		}
	}
	else
	{
		++m_waitingThreads;
	        if( _timeout == 0 )
		{
	            result = m_isSignaled;
		}
		else if( _timeout > 0 )
		{
			struct timeval now;
			struct timespec abstime;

			gettimeofday(&now, NULL);
            
			abstime.tv_sec = now.tv_sec + (_timeout / 1000);

			// Now add the remainder of our timeout to the microseconds part of 'now'
			now.tv_usec += (_timeout % 1000) * 1000;

			// Careful now! Did it wrap?
			if(now.tv_usec > (1000 * 1000))
			{
				// Yes it did so bump our seconds and modulo
				now.tv_usec %= (1000 * 1000);
				abstime.tv_sec++;
			}
            
			abstime.tv_nsec = now.tv_usec * 1000;
            
			while( !m_isSignaled )
			{
				int oldstate;
				pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

				int pr = pthread_cond_timedwait( &m_condition, &m_lock, &abstime );

				pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

				if( pr == ETIMEDOUT )
				{
					result = false;
					break;
				}
				else
				{
					result = true;
				}
			}
		}
		else
		{
			while( !m_isSignaled )
			{
				int oldstate;
				pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

				pthread_cond_wait( &m_condition, &m_lock );

				pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
			}
		}
		--m_waitingThreads;
	}

	pthread_mutex_unlock( &m_lock );
	return result;
}

