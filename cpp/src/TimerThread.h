//-----------------------------------------------------------------------------
//
//	TimerThread.h
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

#ifndef _TIMERTHREAD_H_
#define _TIMERTHREAD_H_

#define USE_TR1
#ifdef USE_TR1
#include <tr1/functional>
#define TT_STDBIND std::tr1::bind
#define TT_STDFUNCTION std::tr1::function
#else
#include <functional>
#define TT_STDBIND std::bind
#define TT_STDFUNCTION std::function
#endif

#include "Defs.h"
#include "platform/Event.h"
#include "platform/Mutex.h"
#include "platform/TimeStamp.h"

namespace OpenZWave
{
	class Thread;

	/** \brief The TimerThread class makes it possible to schedule events to happen
	 *  at a certain time in the future.
	 */
	class OPENZWAVE_EXPORT TimerThread
	{

	//-----------------------------------------------------------------------------
	//  Timer based actions
	//-----------------------------------------------------------------------------
	public:
		/** A timer callback function. */
		typedef TT_STDFUNCTION<void()> TimerCallback;

		/**
		 * Constructor.
		 */
		TimerThread();

		/**
		 * Destructor.
		 */
		~TimerThread();

		/**
		 * Schedule an event.
		 * \param _milliseconds The number of milliseconds before the event should happen
		 * \param _callback The function to be called when the time is reached
		 */
		void TimerSetEvent( int32 _milliseconds, TimerCallback _callback );

		/**
		 * Main entry point for the timer thread. Wrapper around TimerThreadProc.
		 * \param _exitEvent Exit event indicating the thread should exit
		 * \param _context A TimerThread object
		 */
		static void TimerThreadEntryPoint( Event* _exitEvent, void* _context );

	private:

		/**
		 * Main class entry point for the timer thread. Contains the main timer loop.
		 * \param _exitEvent Exit event indicating the thread should exit
		 */
		void TimerThreadProc( Event* _exitEvent );

		struct TimerEventEntry
		{
			TimeStamp timestamp;
			TimerCallback callback;
		};

    /** A list of upcoming timer events */
		list<TimerEventEntry *> m_timerEventList;

		Event*				m_timerEvent;   // Event to signal new timed action requested
		Mutex*				m_timerMutex;   // Serialize access to class members
		int32					m_timerTimeout; // Time in milliseconds to wait until next event
	};

} // namespace OpenZWave

#endif // _TIMERTHREAD_H_
