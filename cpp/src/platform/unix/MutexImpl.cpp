//----------------------------------------------------------------------------
//
//  MutexImpl.cpp
//
//  POSIX implementation of the cross-platform mutex
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
#include "platform/Log.h"
#include "MutexImpl.h"

#include <stdio.h>
#include <errno.h>

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{

//-----------------------------------------------------------------------------
//	<MutexImpl::MutexImpl>
//	Constructor
//-----------------------------------------------------------------------------
			MutexImpl::MutexImpl() :
					m_lockCount(0)
			{
				pthread_mutexattr_t ma;

				pthread_mutexattr_init(&ma);
				pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
				int err = pthread_mutex_init(&m_criticalSection, &ma);
				if (err != 0)
				{
					Log::Write(LogLevel_Error, "MutexImpl::MutexImpl error %d (%d)\n", errno, err);
				}
				pthread_mutexattr_destroy(&ma);
			}

//-----------------------------------------------------------------------------
//	<MutexImpl::~MutexImpl>
//	Destructor
//-----------------------------------------------------------------------------
			MutexImpl::~MutexImpl()
			{
				if (m_lockCount != 0)
				{
					Log::Write(LogLevel_Error, "MutexImpl:~MutexImpl: - Destroying a Locked Mutex: %d", m_lockCount);
				}
				pthread_mutex_destroy(&m_criticalSection);
			}

//-----------------------------------------------------------------------------
//	<MutexImpl::Lock>
//	Lock the mutex
//-----------------------------------------------------------------------------
			bool MutexImpl::Lock(bool const _bWait)
			{
				if (m_lockCount < 0)
				{
					Log::Write(LogLevel_Error, "MutexImpl:Lock - Lock is Negative: %d", m_lockCount);
					m_lockCount = 0;
				}

				if (_bWait)
				{
					// We will wait for the lock
					int err = pthread_mutex_lock(&m_criticalSection);
					if (err == 0)
					{
						++m_lockCount;
						return true;
					}
					Log::Write(LogLevel_Error, "MutexImpl::Lock failed with error: %d (%d)", errno, err);
					return false;
				}

				// Returns immediately, even if the lock was not available.
				if (pthread_mutex_trylock(&m_criticalSection))
				{
					return false;
				}

				++m_lockCount;
				return true;
			}

//-----------------------------------------------------------------------------
//	<MutexImpl::Unlock>
//	Release our lock on the mutex
//-----------------------------------------------------------------------------
			void MutexImpl::Unlock()
			{
				if (m_lockCount < 0)
				{
					// No locks - we have a mismatched lock/release pair
					Log::Write(LogLevel_Error, "MutexImpl:Unlock - Lock is Negative - MisMatched Lock/Release Pair: %d", m_lockCount);
					/* reset the lockCount to 0 */
					m_lockCount = 0;
				}
				else
				{
					--m_lockCount;
				}
				/* try to unlock Regardless of the lockCount */
				int err = pthread_mutex_unlock(&m_criticalSection);
				if (err != 0)
				{
					Log::Write(LogLevel_Error, "MutexImpl::UnLock failed with error: %d (%d)\n", errno, err);
				}
			}

//-----------------------------------------------------------------------------
//	<MutexImpl::IsSignalled>
//	Test whether the mutex is free
//-----------------------------------------------------------------------------
			bool MutexImpl::IsSignalled()
			{
				return (0 == m_lockCount);
			}
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave
