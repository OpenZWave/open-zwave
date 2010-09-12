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

#include "Defs.h"
#include "Thread.h"
#include "ThreadImpl.h"

#ifdef DARWIN
#define pthread_yield pthread_yield_np
#endif

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadImpl>
//	Constructor
//-----------------------------------------------------------------------------
ThreadImpl::ThreadImpl
(
	string const& _tname
):
	m_hThread( NULL ),
	m_bIsRunning( false ),
	m_name( _tname )
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
	if ( m_bIsRunning )
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
	//fprintf(stderr, "thread %s starting %08x\n", m_name.c_str(), m_hThread);
	//fflush(stderr);

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
	void *data;

	//fprintf(stderr, "thread %s stopping %08x running %d\n", m_name.c_str(), m_hThread, m_bIsRunning );
	//fflush(stderr);
	if( !m_bIsRunning )
	{
		return false;
	}

	// This will kill an app that doesn't catch and ignore it.
	// We need to find another way to interrupt select.
	pthread_kill( m_hThread, SIGALRM );
	pthread_join( m_hThread, &data );
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
	ThreadImpl* pImpl = (ThreadImpl*)_pArg;
	//fprintf(stderr, "thread %s run begin %08x running %d\n", pImpl->m_name.c_str(), pImpl->m_hThread, pImpl->m_bIsRunning );
	//fflush(stderr);
	pImpl->Run();
	//fprintf(stderr, "thread %s run end %08x running %d\n", pImpl->m_name.c_str(), pImpl->m_hThread, pImpl->m_bIsRunning );
	//fflush(stderr);
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
}
