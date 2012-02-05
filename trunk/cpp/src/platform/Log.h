//-----------------------------------------------------------------------------
//
//	Log.h
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
#ifndef _Log_H
#define _Log_H

#include <stdarg.h>
#include <string>
#include "Defs.h"

namespace OpenZWave
{
	class Mutex;
	class i_LogImpl
	{
	public:
		i_LogImpl() { } ;
		virtual ~i_LogImpl() { } ;
		virtual void Write( char const* _format, va_list _args ) = 0;
	};


	/** \brief Implements a platform-independent log...written to the console and, optionally, a file.
	 */
	class Log
	{
	public:
		/**
		 * Create a log.
		 * Creates the cross-platform logging singleton.
		 * Any previous log will be cleared.
		 * \return a pointer to the logging object.
		 * \see Destroy, Write
		 */
		static Log* Create( string const& _filename );

		/**
		 * Create a log.
		 * Creates the cross-platform logging singleton.
		 * Any previous log will be cleared.
		 * \param LogClass a Logging Class that inherits the i_LogImpl Class to use to Log
		 * \return a pointer to the logging object.
		 * \see Destroy, Write
		 */

		static Log* Create( i_LogImpl *LogClass );

		/**
		 * Destroys the log.
		 * Destroys the logging singleton.  The log can no longer
		 * be written to without another call to Create.
		 * \see Create, Write
		 */
		static void Destroy();

		/**
		 * \brief Set the Logging Implmentation Class to replace the standard File/Console Loggin
		 * \param LogClass A Logging Class that inherits the i_LogImpl Class used to Log to
		 * \return Bool Value indicating success or failure
		 */
		static bool SetLoggingClass(i_LogImpl *LogClass );

		/**
		 * \brief Enable or disable library logging.
		 * \param _dologging  If true, logging is enabled; if false, disabled
		*/
		static void SetLoggingState(bool _dologging);

		/**
		 * \brief Determine whether logging is enabled or not.
		 * \param _dologging  If true, logging is enabled; if false, disabled
		*/
		static bool GetLoggingState();

		/**
		 * Write an entry to the log.
		 * Writes a formatted string to the log.
		 * \param _format.  A string formatted in the same manner as used with printf etc.
		 * \param ... a variable number of arguments, to be included in the formatted string.
		 * \see Create, Destroy
		 */
		static void Write( char const* _format, ... );

	private:
		Log( string const& _filename );
		~Log();

		static i_LogImpl*	m_pImpl;	// Pointer to an object that encapsulates the platform-specific logging implementation.
		static Log*	s_instance;
		Mutex*		m_logMutex;
	};

} // namespace OpenZWave

#endif //_Log_H

