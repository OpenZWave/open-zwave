//-----------------------------------------------------------------------------
//
//	Event.h
//
//	Cross-platform manual-reset event
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

#ifndef _Event_H
#define _Event_H

namespace OpenZWave
{
	class EventImpl;

	class Event
	{
	public:
		enum
		{
			Timeout_Immediate = 0,
			Timeout_Infinite = -1
		};

		/**
		 * Constructor.
		 * Creates a cross-platform event object equivalent to the Windows manual-reset event
		 */
		Event();

		/**
		 * Destructor.
		 * Destroys the event object.
		 */
		~Event();

		/**
		 * Set the event to signalled.
		 * @see Reset, Wait
		 */
		void Set();

		/**
		 * Set the event to not signalled.
		 * @see Set, Wait
		 */
		void Reset();

		/**
		 * Waits for an event to become signalled.
		 * Waits for an event to become signalled, or for the wait to time-out.
		 * @param _timeout maximum time in milliseconds to wait for the event
		 * to become signalled. If the timeout is zero, the method will 
		 * return immediately.  If the timeout is Timeout_Infinite, the 
		 * method will not return until the event is signalled.
		 * @return the state of the event.  True if it is signalled, false otherwise.
		 * @see Set, Reset
		 */
		bool Wait( int32 _timeout );

	private:
		Event( Event const&	);					// prevent copy
		Event& operator = ( Event const& );		// prevent assignment

		EventImpl*	m_pImpl;	// Pointer to an object that encapsulates the platform-specific implementation of a event.
	};

} // namespace OpenZWave

#endif //_Event_H

