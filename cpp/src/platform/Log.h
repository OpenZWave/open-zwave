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
#include <vector>
#include "Defs.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{
			class Mutex;
		}
	}
	extern char const *LogLevelString[];

	/** \brief Various LogLevels available to the Application
	 * \ingroup Platform
	 *
	 * \see Log::SetLoggingState
	 */
	enum LogLevel
	{
		LogLevel_Always = 5, /**< These messages should always be shown */
		LogLevel_Error = 4, /**< A serious issue with the library or the network */
		LogLevel_Warning = 3, /**< A minor issue from which the library should be able to recover */
		LogLevel_Info = 2, /**< Everything is working fine...these messages provide streamlined feedback on each message */
		LogLevel_Debug = 1, /**< Very detailed information on progress */
		LogLevel_None = 0, /**< Disable all logging */
	};

	/** \brief A Abstract class to create a Custom Logging Method
	 * \ingroup Platform
	 *
	 * Use this as the basis to create a custom logging class for your applation.
	 * \see Log::SetLoggingClass
	 */
	class i_LogImpl
	{
		public:
			i_LogImpl()
			{
			}
			;
			virtual ~i_LogImpl()
			{
			}
			;
			virtual void Write(LogLevel _level, uint8 const _nodeId, char const* _format, va_list _args) = 0;
			virtual void SetLogFileName(const string &_filename) = 0;
			virtual void RotateLogFile() = 0;
			std::string GetLogLevelString(LogLevel _level);

	};

	/** \brief Implements a platform-independent log...written to the console and, optionally, a file.
	 * \ingroup Platform
	 */
	class OPENZWAVE_EXPORT Log
	{
		public:
			/** \brief Create a log.
			 *
			 * Creates the cross-platform logging singleton.
			 * Any previous log will be cleared.
			 * \return a pointer to the logging object.
			 * \see Destroy, Write
			 */
			static Log* Create(string const& _filename, bool const _bAppend, bool const _bConsoleOutput, LogLevel const _saveLevel);

			/** \brief Create a log.
			 *
			 * Creates the cross-platform logging singleton.
			 * Any previous log will be cleared.
			 * \param LogClass a Logging Class that inherits the i_LogImpl Class to use to Log
			 * \return a pointer to the logging object.
			 * \see Destroy, Write
			 */

			static Log* Create(i_LogImpl *LogClass);

			/** \brief Destroys the log.
			 *
			 * Destroys the logging singleton.  The log can no longer
			 * be written to without another call to Create.
			 * \see Create, Write
			 */
			static void Destroy();

			/**
			 * \brief Set the Logging Implementation Class to replace the standard File/Console logging
			 *
			 * \param LogClass A Logging Class that inherits the i_LogImpl Class used to Log to
			 * \param Append if this new Logging Class should be appended to the list of Logging Implementations,
			 * or replace the existing Logging Class
			 * \return Bool Value indicating success or failure
			 */
			static bool SetLoggingClass(i_LogImpl *LogClass, bool Append = false);

			/**\brief Enable or disable library logging.
			 *
			 * To disable, set _saveLevel to LogLevel_None.
			 *
			 * \param _saveLevel	LogLevel of messages to write in real-time
			 */
			static void SetLoggingState(LogLevel _saveLevel);

			/**\brief Obtain the various logging levels.
			 *	
			 */
			static LogLevel GetLoggingState();

			/** \brief Change the log file name.
			 *
			 * This will start a new log file (or potentially start appending
			 * information to an existing one.  Developers might want to use this function, together with a timer
			 * in the controlling application, to create timestamped log file names.
			 * \param _filename Name of the new (or existing) file to use for log output.
			 */
			static void SetLogFileName(const string &_filename);

			/**\brief Write an entry to the log.
			 *
			 * Writes a formatted string to the log.
			 * \param _level	Specifies the type of log message (Error, Warning, Debug, etc.)
			 * \param _format.  A string formatted in the same manner as used with printf etc.
			 * \param ... a variable number of arguments, to be included in the formatted string.
			 * \see Create, Destroy
			 */
			static void Write(LogLevel _level, char const* _format, ...);

			/**\brief Write an entry to the log.
			 *
			 * Writes a formatted string to the log.
			 * \param _level	Specifies the type of log message (Error, Warning, Debug, etc.)
			 * \param _nodeId	Node Id this entry is about.
			 * \param _format.  A string formatted in the same manner as used with printf etc.
			 * \param ... a variable number of arguments, to be included in the formatted string.
			 * \see Create, Destroy
			 */
			static void Write(LogLevel _level, uint8 const _nodeId, char const* _format, ...);
			/**\brief Rotate the Log File
			 * 
			 * Starts writting to a new Log File and Moves the Current Logfile to a old file
			 * 
			 */
			static void RotateLogFile();
			/** \brief Setup LogFile Rotation
			 * 
			 * Sets up the Log Class to do Automatic Log File Rotation At Midnight
			 */
			static void SetupLogFileRotation();
		private:
			Log(string const& _filename, bool const _bAppend, bool const _bConsoleOutput, LogLevel _saveLevel);
			~Log();

			static std::vector<i_LogImpl*> m_pImpls; /**< Pointer to an object that encapsulates the platform-specific logging implementation. */
			static Log* s_instance;
			Internal::Platform::Mutex* m_logMutex;
			static LogLevel m_minLogLevel;
			static uint32 m_currentDay;
			static bool m_doRotate;
	};
} // namespace OpenZWave

#endif //_Log_H
