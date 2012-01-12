//-----------------------------------------------------------------------------
//
//	ThreadImpl.cpp
//
//	Windows implementation of a cross-platform thread
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
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
#include "ThreadImpl.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadImpl>
//	Constructor
//-----------------------------------------------------------------------------
ThreadImpl::ThreadImpl
(
	string const& _tname
):
	m_hThread( INVALID_HANDLE_VALUE ),
	m_bIsRunning( false ),
	m_name( _tname )
{
	// Create an event allowing us to tell the ZWave Thread to exit
	m_hExitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
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
	CloseHandle( m_hExitEvent );
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Start>
//	Start a function running on this thread
//-----------------------------------------------------------------------------
bool ThreadImpl::Start
(
	Thread::pfnThreadProc_t _pfnThreadProc, 
	void* _context 
)
{
	ResetEvent( m_hExitEvent );

	// Create a thread to run the specified function
	m_pfnThreadProc = _pfnThreadProc;
	m_context = _context;

	HANDLE hThread = ::CreateThread( NULL, 0, ThreadImpl::ThreadProc, this, CREATE_SUSPENDED, NULL );
	m_hThread = hThread;

	::ResumeThread( hThread );
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

	::TerminateThread( m_hThread, 0 );
	CloseHandle( m_hThread );
	m_hThread = INVALID_HANDLE_VALUE;

	return true;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Sleep>
//	Cause thread to sleep for the specified number of milliseconds
//-----------------------------------------------------------------------------
void ThreadImpl::Sleep
(
	uint32 _millisecs
)
{
	::Sleep(_millisecs);
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadProc>
//	Entry point for running a function on this thread
//-----------------------------------------------------------------------------
DWORD WINAPI ThreadImpl::ThreadProc
( 
	void* _pArg 
)
{
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
	m_pfnThreadProc( m_context );
	m_bIsRunning = false;
}

