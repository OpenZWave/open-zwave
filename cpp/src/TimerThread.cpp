//-----------------------------------------------------------------------------
//
//	TimerThread.cpp
//
//  Timer for scheduling future events
//
//	Copyright (c) 2017 h3ctrl <h3ctrl@gmail.com>
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

#include "TimerThread.h"
#include "Utils.h"
#include "platform/Log.h"
#include "Driver.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <TimerThread::TimerThreadEntryPoint>
// Main entry point for the timer thread.
//-----------------------------------------------------------------------------
void TimerThread::TimerThreadEntryPoint
(
		Event* _exitEvent,
		void* _context
)
{
	TimerThread* timer = (TimerThread*)_context;
	if( timer )
	{
		timer->TimerThreadProc( _exitEvent );
	}
}

//-----------------------------------------------------------------------------
// <TimerThread::TimerThread>
// Constructor.
//-----------------------------------------------------------------------------
TimerThread::TimerThread
(
		Driver *_driver
):
m_driver( _driver ),
m_timerEvent( new Event() ),
m_timerMutex( new Mutex() ),
m_timerTimeout( Wait::Timeout_Infinite )
{
}

//-----------------------------------------------------------------------------
// <TimerThread::~TimerThread>
// Destructor.
//-----------------------------------------------------------------------------
TimerThread::~TimerThread
(
)
{
	{
		LockGuard LG(m_timerMutex);
		for ( list<TimerEventEntry *>::iterator it = m_timerEventList.begin(); it != m_timerEventList.end(); ++it ) {
			delete (*it);
		}
	}
	m_timerMutex->Release();
	m_timerEvent->Release();
}


//-----------------------------------------------------------------------------
// <TimerThread::TimerThreadProc>
// Thread for timer based actions
//-----------------------------------------------------------------------------
void TimerThread::TimerThreadProc
(
		Event* _exitEvent
)
{
	Log::Write( LogLevel_Info, "Timer: thread starting" );

	Wait* waitObjects[2];
	waitObjects[0] = _exitEvent;
	waitObjects[1] = m_timerEvent;
	uint32 count = 2;

	// Initially no timer events so infinite timeout.
	m_timerTimeout = Wait::Timeout_Infinite;

	while( 1 )
	{
		Log::Write( LogLevel_Detail, "Timer: waiting with timeout %d ms", m_timerTimeout );
		int32 res = Wait::Multiple( waitObjects, count, m_timerTimeout );

		if (res == 0)
		{
			// Exit has been signalled
			return;

		} else {
			// Timeout or new entry to timer list.
			m_timerTimeout = Wait::Timeout_Infinite;

			// Go through all waiting actions, and see if any need to be performed.
			LockGuard LG(m_timerMutex);
			list<TimerEventEntry *>::iterator it = m_timerEventList.begin();
			while( it != m_timerEventList.end() ) {
				int32 tr = (*it)->timestamp.TimeRemaining();
				if (tr <= 0) {
					// Expired so perform action and remove from list.
					Log::Write( LogLevel_Info, "Timer: delayed event" );
					(*it)->callback();
					delete (*it);
					it = m_timerEventList.erase( it );

				} else {
					// Time remaining.
					m_timerTimeout = (m_timerTimeout == Wait::Timeout_Infinite) ? tr : std::min(m_timerTimeout, tr);
					++it;
				}
			}
			m_timerEvent->Reset();
		}
	} // while( 1 )
}

//-----------------------------------------------------------------------------
// <TimerThread::TimerSetEvent>
//-----------------------------------------------------------------------------
void TimerThread::TimerSetEvent
(
		int32 _milliseconds,
		TimerCallback _callback
)
{
	Log::Write( LogLevel_Info, "Timer: adding event in %d ms", _milliseconds );
	TimerEventEntry *te = new TimerEventEntry();
	te->timestamp.SetTime(_milliseconds);
	te->callback = _callback;

	// Don't want driver thread and timer thread accessing list at the same time.
	LockGuard LG(m_timerMutex);
	m_timerEventList.push_back(te);
	m_timerEvent->Set();
	return;
}
