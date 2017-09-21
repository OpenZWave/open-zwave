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

#if __cplusplus >= 201103L || __APPLE__
#include <functional>
using std::bind;
using std::function;
#else
#include <tr1/functional>
using std::tr1::bind;
using std::tr1::function;
#endif

#include "Defs.h"
#include "platform/Event.h"
#include "platform/Mutex.h"
#include "platform/TimeStamp.h"

namespace OpenZWave
{
	class Driver;
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
		typedef function<void()> TimerCallback;

		/**
		 * Constructor.
		 */
		TimerThread( Driver *_driver );

		/**
		 * Destructor.
		 */
		~TimerThread();

		struct TimerEventEntry
		{
			TimeStamp timestamp;
			TimerCallback callback;
		};

		/**
		 * Schedule an event.
		 * \param _milliseconds The number of milliseconds before the event should happen
		 * \param _callback The function to be called when the time is reached
		 */
		TimerEventEntry* TimerSetEvent( int32 _milliseconds, TimerCallback _callback );

		/**
		 * Main entry point for the timer thread. Wrapper around TimerThreadProc.
		 * \param _exitEvent Exit event indicating the thread should exit
		 * \param _context A TimerThread object
		 */
		static void TimerThreadEntryPoint( Event* _exitEvent, void* _context );

	private:
		//Driver*	m_driver;

		/**
		 * Main class entry point for the timer thread. Contains the main timer loop.
		 * \param _exitEvent Exit event indicating the thread should exit
		 */
		void TimerThreadProc( Event* _exitEvent );


    /** A list of upcoming timer events */
		list<TimerEventEntry *> m_timerEventList;

		Event*				m_timerEvent;   // Event to signal new timed action requested
		Mutex*				m_timerMutex;   // Serialize access to class members
		int32					m_timerTimeout; // Time in milliseconds to wait until next event
	};

} // namespace OpenZWave

#endif // _TIMERTHREAD_H_
