//-----------------------------------------------------------------------------
//
//	LogImpl.cpp
//
//	Windows implementation of message and error logging
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
	m_filename( _filename )
{
	FILE* pFile = NULL;
	if( !(pFile = fopen(m_filename.c_str(), "w" )) )
	{
		SYSTEMTIME time;
		::GetLocalTime( &time );
		fprintf( pFile, "\nLogging started %04d-%02d-%02d %02d:%02d:%02d\n\n", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond );
		fclose( pFile );
	}
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
	char timeStr[32];
	sprintf_s( timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d:%03d ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds );

	FILE* pFile = fopen(m_filename.c_str(), "a" );
	if( pFile != NULL )
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
