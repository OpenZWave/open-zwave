//-----------------------------------------------------------------------------
//
//	Log.cpp
//
//	Cross-platform message and error logging
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

#include <stdarg.h>

#include "Defs.h"
#include "Mutex.h"
#include "Log.h"

#include "LogImpl.h"	// Platform-specific implementation of a log


using namespace OpenZWave;

Log* Log::s_instance = NULL;


//-----------------------------------------------------------------------------
//	<Log::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
Log* Log::Create
(
	string const& _filename 
)
{
	if( NULL == s_instance )
	{
		s_instance = new Log( _filename );
	}

	return s_instance;
}

//-----------------------------------------------------------------------------
//	<Log::Destroy>
//	Static method to destroy the logging singleton.
//-----------------------------------------------------------------------------
void Log::Destroy
(
)
{
	delete s_instance;
	s_instance = NULL;
}

//-----------------------------------------------------------------------------
//	<Log::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void Log::Write
( 
	char const* _format,
	... 
)
{
	if( s_instance )
	{
	  	s_instance->m_logMutex->Lock();
		va_list args;
		va_start( args, _format );
		s_instance->m_pImpl->Write( _format, args );
		va_end( args );
		s_instance->m_logMutex->Release();
	}
}

//-----------------------------------------------------------------------------
//	<Log::Log>
//	Constructor
//-----------------------------------------------------------------------------
Log::Log
(
	string const& _filename 
):
	m_pImpl( new LogImpl( _filename ) ),
	m_logMutex( new Mutex() )
{
}

//-----------------------------------------------------------------------------
//	<Log::~Log>
//	Destructor
//-----------------------------------------------------------------------------
Log::~Log
(
)
{
	delete m_logMutex;
	delete m_pImpl;
}
