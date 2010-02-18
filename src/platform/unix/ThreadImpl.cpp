//
// ThreadImpl.cpp
//
// PThreads implementation of a cross-platform thread
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

#include "Defs.h"
#include "Thread.h"
#include "ThreadImpl.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadImpl>
//	Constructor
//-----------------------------------------------------------------------------
ThreadImpl::ThreadImpl
(
):
	m_hThread( NULL ),
	m_bIsRunning( false )
{
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::~ThreadImpl>
//	Destructor
//-----------------------------------------------------------------------------
ThreadImpl::~ThreadImpl
(
)
{
	Stop();
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Start>
//	Start a function running on this thread
//-----------------------------------------------------------------------------
bool ThreadImpl::Start
(
	Thread::pfnThreadProc_t _pfnThreadProc, 
	void* _pContext 
)
{
	pthread_attr_t ta;

	pthread_attr_init( &ta );
	pthread_attr_setstacksize ( &ta, 0 );
	pthread_attr_setdetachstate ( &ta, PTHREAD_CREATE_JOINABLE );

	// Create a thread to run the specified function
	m_pfnThreadProc = _pfnThreadProc;
	m_pContext = _pContext;

	pthread_create ( &m_hThread, &ta, ThreadImpl::ThreadProc, this );

	pthread_attr_destroy ( &ta );
	return true;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Stop>
//	Stop a function running on this thread
//-----------------------------------------------------------------------------
bool ThreadImpl::Stop
(
)
{
	if( !m_bIsRunning )
	{
		return false;
	}

	pthread_cancel( m_hThread );
	m_hThread = NULL;
	m_bIsRunning = false;

	return true;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadProc>
//	Entry point for running a function on this thread
//-----------------------------------------------------------------------------
void *ThreadImpl::ThreadProc
( 
	void* _pArg 
)
{
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
	pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL );
	ThreadImpl* pImpl = (ThreadImpl*)_pArg;
	pImpl->Run();
	return 0;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Run>
//	Entry point for running a function on this thread
//-----------------------------------------------------------------------------
void ThreadImpl::Run
( 
)
{
	m_bIsRunning = true;
	m_pfnThreadProc( m_pContext );
	m_bIsRunning = false;
}
