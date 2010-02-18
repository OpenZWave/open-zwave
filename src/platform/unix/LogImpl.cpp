//
// LogImpl.cpp
//
// Unix implementation of message and error logging
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

#include <string>
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
	FILE* pFile = fopen( m_filename.c_str(), "w" );
	if ( pFile != NULL )
	{
		long now;
		time( &now );
		struct tm *tm;
		tm = localtime( &now );
		fprintf( pFile, "\nLogging started %04d-%02d-%02d %02d:%02d:%02d\n\n",
			 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			 tm->tm_hour, tm->tm_min, tm->tm_sec );
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
	char* _format, 
	va_list _args
)
{
	// Get a timestamp
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm *tm;
	tm = localtime( &tv.tv_sec );
	char timeStr[32];
	snprintf( timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d:%03d ",
		  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		  tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 1000 );

	FILE* pFile = fopen( m_filename.c_str(), "a" );
	if ( pFile != NULL )
	{
		// Log to screen and file
		if( _format && ( _format[0] != 0 ) )
		{
			printf( "%s", timeStr );
			fprintf( pFile, "%s", timeStr );

			va_list saveargs;
			va_copy( saveargs, _args );
			vprintf( _format, _args );
			vfprintf( pFile, _format, saveargs );
		}

		printf( "\n" );
		fprintf( pFile, "\n" ); 

		fclose( pFile );
	}
}
