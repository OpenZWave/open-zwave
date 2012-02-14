//-----------------------------------------------------------------------------
//
//	Log.cpp
//
//	Cross-platform message and error logging
//
//	Copyright (c) 2010 Mal Lansell <mal@lansell.org>
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
#include <stdarg.h>

#include "Defs.h"
#include "Mutex.h"
#include "Log.h"

#include "LogImpl.h"	// Platform-specific implementation of a log


using namespace OpenZWave;

Log* Log::s_instance = NULL;
i_LogImpl* Log::m_pImpl = NULL;
static bool s_dologging;

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
		s_dologging = true; // default logging to true so no change to what people experience now
	}

	return s_instance;
}

//-----------------------------------------------------------------------------
//	<Log::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
Log* Log::Create
(
	i_LogImpl *LogClass
)
{
	if (NULL == s_instance )
	{
		s_instance = new Log( "" );
		s_dologging = true;
	}
	SetLoggingClass( LogClass );
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
//	<Log::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
bool Log::SetLoggingClass
(
	i_LogImpl *LogClass
)
{
	delete m_pImpl;
	m_pImpl = LogClass;
	return true;
}

//-----------------------------------------------------------------------------
//	<Log::SetLogggingState>
//	Set flag to actually write to log or skip it
//-----------------------------------------------------------------------------
void Log::SetLoggingState
(
	bool _dologging
)
{
	s_dologging = _dologging;
}

//-----------------------------------------------------------------------------
//	<Log::GetLoggingState>
//	Return a flag to indicate whether logging is enabled
//-----------------------------------------------------------------------------
bool Log::GetLoggingState
(
)
{
	return s_dologging;
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
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		va_list args;
		va_start( args, _format );
		s_instance->m_pImpl->Write( _format, args );
		va_end( args );
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void Log::Add
(
	char const* _format,
	...
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		va_list args;
		va_start( args, _format );
		s_instance->m_pImpl->Add( _format, args );
		va_end( args );
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::Dump>
//	
//-----------------------------------------------------------------------------
void Log::Dump
(
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		s_instance->m_pImpl->Dump();
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::Clear>
//	
//-----------------------------------------------------------------------------
void Log::Clear
(
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		s_instance->m_pImpl->Clear();
		s_instance->m_logMutex->Unlock();
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
	m_logMutex( new Mutex() )
{
	m_pImpl = new LogImpl( _filename );

}

//-----------------------------------------------------------------------------
//	<Log::~Log>
//	Destructor
//-----------------------------------------------------------------------------
Log::~Log
(
)
{
	m_logMutex->Release();
	delete m_pImpl;
}
