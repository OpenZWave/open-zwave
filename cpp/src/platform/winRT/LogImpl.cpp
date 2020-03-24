//-----------------------------------------------------------------------------
//
//	LogImpl.cpp
//
//	WinRT implementation of message and error logging
//
//	Copyright (c) 2015 Microsoft Corporation
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
#include <list>

#include "Defs.h"
#include "platform/TimeStamp.h"
#include "LogImpl.h"

#ifdef MINGW

#define vsprintf_s vsnprintf

#define strcpy_s(DEST, NUM, SOURCE) strncpy(DEST, SOURCE, NUM)

errno_t fopen_s(FILE** pFile, const char *filename, const char *mode)
{
	if (!pFile)
	{
		_set_errno(EINVAL);
		return EINVAL;
	}

	*pFile = fopen(filename, mode);

	if (!*pFile)
	{
		return errno;
	}

	return 0;
}

#endif

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{

//-----------------------------------------------------------------------------
//	<LogImpl::LogImpl>
//	Constructor
//-----------------------------------------------------------------------------
			LogImpl::LogImpl(string const& _filename, bool const _bAppendLog, bool const _bConsoleOutput) :
					m_filename(_filename),					// name of log file
					m_bAppendLog(_bAppendLog),				// true to append (and not overwrite) any existing log
					m_bConsoleOutput(_bConsoleOutput)		// true to provide a copy of output to console
			{
				string accessType;

				if (m_bAppendLog)
				{
					accessType = "a";
				}
				else
				{
					accessType = "w";
				}

				if (!m_filename.empty()) { 
					FILE* pFile;
					if (!fopen_s(&pFile, m_filename.c_str(), accessType.c_str()))
					{
						TimeStamp timeStr;
						fprintf(pFile, "\nLogging started %s\n\n", timeStr.GetAsString().c_str());
						fclose(pFile);
					}
				}
			}

//-----------------------------------------------------------------------------
//	<LogImpl::~LogImpl>
//	Destructor
//-----------------------------------------------------------------------------
			LogImpl::~LogImpl()
			{
			}

//-----------------------------------------------------------------------------
//	<LogImpl::ReopenLogFile>
//	Currently empty, because this implementation
//	opens/closes logfile at every write
//-----------------------------------------------------------------------------
			void LogImpl::ReopenLogFile()
			{
			}

//-----------------------------------------------------------------------------
//	<LogImpl::Write>
//	Write to the log
//-----------------------------------------------------------------------------
			void LogImpl::Write(LogLevel _logLevel, uint8 const _nodeId, char const* _format, va_list _args)
			{
				// create a timestamp string
				TimeStamp timeStr;
				string nodeStr = GetNodeString(_nodeId);
				string logLevelStr = GetLogLevelString(_logLevel);

				char lineBuf[1024];
				if (!_format || (_format[0] == 0))
				{
					strcpy_s(lineBuf, 1024, "");
				}
				else
				{
					vsprintf_s(lineBuf, sizeof(lineBuf), _format, _args);
				}

				if (!m_filename.empty()) { 
					// save to file
					FILE* pFile = NULL;
					if (!fopen_s(&pFile, m_filename.c_str(), "a"))
					{
						if (pFile != NULL)
						{
							fprintf(pFile, "%s%s%s%s\n", timeStr.GetAsString().c_str(), logLevelStr.c_str(), nodeStr.c_str(), lineBuf);
							fclose(pFile);
						}
					}
				}
				if (m_bConsoleOutput)
				{
					printf("%s%s%s%s\n", timeStr.GetAsString().c_str(), logLevelStr.c_str(), nodeStr.c_str(), lineBuf);
				}
			}

//-----------------------------------------------------------------------------
//	<LogImpl::GetNodeString>
//	Generate a string with formatted node id
//-----------------------------------------------------------------------------
			string LogImpl::GetNodeString(uint8 const _nodeId)
			{
				if (_nodeId == 0)
				{
					return "";
				}
				else if (_nodeId == 255) // should make distinction between broadcast and controller better for SwitchAll broadcast
				{
					return "contrlr, ";
				}
				else
				{
					char buf[20];
					snprintf(buf, sizeof(buf), "Node%03d, ", _nodeId);
					return buf;
				}
			}

//-----------------------------------------------------------------------------
//	<LogImpl::GetThreadId>
//	Generate a string with formatted thread id
//-----------------------------------------------------------------------------
			string LogImpl::GetThreadId()
			{
				char buf[20];
				DWORD dwThread = ::GetCurrentThreadId();
				sprintf_s(buf, sizeof(buf), "%04d ", dwThread);
				string str = buf;
				return str;
			}

//-----------------------------------------------------------------------------
//	<LogImpl::SetLogFileName>
//	Provide a new log file name (applicable to future writes)
//-----------------------------------------------------------------------------
			void LogImpl::SetLogFileName(const string &_filename)
			{
				m_filename = _filename;
			}
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave
