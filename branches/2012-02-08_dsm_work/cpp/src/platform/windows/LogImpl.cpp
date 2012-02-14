//-----------------------------------------------------------------------------
//
//	LogImpl.cpp
//
//	Windows implementation of message and error logging
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
#include <windows.h>

#include "Defs.h"
#include "LogImpl.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<LogImpl::LogImpl>
//	Constructor
//-----------------------------------------------------------------------------
LogImpl::LogImpl
(
	string const& _filename
):
	m_filenamebase( _filename )
{
	SYSTEMTIME time;
	::GetLocalTime( &time );
	m_filename = TimeBasedFileName(m_filenamebase, &time);
	FILE* pFile;
	if( !fopen_s( &pFile, m_filename.c_str(), "a" ) )
	{
		fprintf( pFile, "\nLogging started %04d-%02d-%02d %02d:%02d:%02d\n\n", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond );
		fclose( pFile );
	}
}

string LogImpl::TimeBasedFileName
(
	string const& _basename, 
	SYSTEMTIME* _time
)
{
	char outstring[50];
	sprintf( outstring, "OZW_LOG_%04d-%02d-%02d_%02d.txt",_time->wYear, _time->wMonth, _time->wDay, _time->wHour);
	return outstring;
}

//-----------------------------------------------------------------------------
//	<LogImpl::~LogImpl>
//	Destructor
//-----------------------------------------------------------------------------
LogImpl::~LogImpl
(
)
{
}

//-----------------------------------------------------------------------------
//	<LogImpl::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void LogImpl::Write
( 
	char const* _format, 
	va_list _args
)
{
	// Get a timestamp
	SYSTEMTIME time;
	::GetLocalTime( &time );
	m_filename = TimeBasedFileName(m_filenamebase, &time);
	char timeStr[50];
	DWORD dwThread = ::GetCurrentThreadId();
	sprintf_s( timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d:%03d %d ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, dwThread );

	FILE* pFile;
	if( !fopen_s( &pFile, m_filename.c_str(), "a" ) )
	{
		// Log to screen and file
		if( _format && ( _format[0] != 0 ) )
		{
			printf( "%s", timeStr );
			fprintf( pFile, "%s", timeStr );

			vprintf( _format, _args );
			vfprintf( pFile, _format, _args );
		}

		printf( "\n" );
		fprintf( pFile, "\n" ); 

		fclose( pFile );
	}
}

//-----------------------------------------------------------------------------
//	<LogImpl::Add>
//	Write to the LogQueue
//-----------------------------------------------------------------------------
void LogImpl::Add
( 
	char const* _format, 
	va_list _args
)
{
	// Get a timestamp
	SYSTEMTIME time;
	::GetLocalTime( &time );
	char timeStr[50];
	DWORD dwThread = ::GetCurrentThreadId();
	sprintf_s( timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d:%03d %04d ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, dwThread );

	char outString[500];
	char outString2[500];
	vsprintf(outString, _format, _args );
	sprintf(outString2, "%s%s",timeStr, outString);

	string logStr = outString2;

	m_logQueue.push_back(logStr);

	if( m_logQueue.size() > 500 )
	{
		m_logQueue.pop_front();
	}
}

//-----------------------------------------------------------------------------
//	<LogImpl::Dump>
//	Dump the LogQueue to output device
//-----------------------------------------------------------------------------
void LogImpl::Dump
( 
)
{
	Log::Write( "Dumping queued log messages");
	list<string>::iterator it = m_logQueue.begin();
	while( it != m_logQueue.end() )
	{
		string strTemp = *it;
		Log::Write( strTemp.c_str() );
		it++;
	}
	m_logQueue.clear();
	Log::Write( "End of queued log message dump");
}

//-----------------------------------------------------------------------------
//	<LogImpl::Clear>
//	Clear the LogQueue
//-----------------------------------------------------------------------------
void LogImpl::Clear
( 
)
{
	m_logQueue.clear();
}
