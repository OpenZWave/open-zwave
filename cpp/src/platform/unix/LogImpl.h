//
// LogImpl.h
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

#ifndef _LogImpl_H
#define _LogImpl_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

namespace OpenZWave
{
	class LogImpl
	{
	private:
		friend class Log;

		LogImpl( string const& _filename );
		~LogImpl();

		void Write( char const* _format, va_list _args );

		string m_filename;
	};

} // namespace OpenZWave

#endif //_LogImpl_H

