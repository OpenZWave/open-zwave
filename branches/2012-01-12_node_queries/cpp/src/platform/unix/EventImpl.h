//
// EventImpl.h
//
// POSIX implementation of the cross-platform event
//
// Copyright (c) 2010, Greg Satz <satz@iranger.com>
// All rights reserved.
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

#ifndef _EventImpl_H
#define _EventImpl_H

#include <pthread.h>
#include <errno.h>

namespace OpenZWave
{
	class EventImpl
	{
	private:
		friend class Event;

		EventImpl();
		~EventImpl();

		void Set();
		void Reset();
		bool Wait( int32 _timeout );

		pthread_mutex_t lock;
		pthread_cond_t condition;
		bool manual_reset;
		bool is_signaled;
		unsigned int waiting_threads;
	};

} // namespace OpenZWave

#endif //_EventImpl_H

