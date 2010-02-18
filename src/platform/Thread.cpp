//-----------------------------------------------------------------------------
//
//	Thread.cpp
//
//	Cross-platform threads
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
#include "Thread.h"

#include "ThreadImpl.h"	// Platform-specific implementation of a thread

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<Thread::Thread>
//	Constructor
//-----------------------------------------------------------------------------
Thread::Thread
(
):
	m_pImpl( new ThreadImpl() )
{
}

//-----------------------------------------------------------------------------
//	<Thread::~Thread>
//	Destructor
//-----------------------------------------------------------------------------
Thread::~Thread
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//	<Thread::Start>
//	Start a function running on this thread
//-----------------------------------------------------------------------------
bool Thread::Start
(
	pfnThreadProc_t _pfnThreadProc, 
	void* _context 
)
{
	return( m_pImpl->Start( _pfnThreadProc, _context ) );
}

//-----------------------------------------------------------------------------
//	<Thread::Stop>
//	Stop a function running on this thread
//-----------------------------------------------------------------------------
bool Thread::Stop
(
)
{
	return( m_pImpl->Stop() );
}

//-----------------------------------------------------------------------------
//	<Thread::IsRunning>
//	Test whether a function is running on this thread
//-----------------------------------------------------------------------------
bool Thread::IsRunning
(
)const
{
	return( m_pImpl->IsRunning() );
}

