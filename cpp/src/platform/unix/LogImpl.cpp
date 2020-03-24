//-----------------------------------------------------------------------------
//
//	LogImpl.cpp
//
//  Unix implementation of message and error logging
//
//	Copyright (c) 2010, Greg Satz <satz@iranger.com>
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
#include <string>
#include <cstring>
#include <pthread.h>
#include <iostream>
#include "Defs.h"
#include "platform/TimeStamp.h"
#include "LogImpl.h"

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
					m_bConsoleOutput(_bConsoleOutput),		// true to provide a copy of output to console
					m_bAppendLog(_bAppendLog),				// true to append (and not overwrite) any existing log
					pFile( NULL)
			{
				OpenLogFile();
				setlinebuf(stdout);	// To prevent buffering and lock contention issues
			}

//-----------------------------------------------------------------------------
//	<LogImpl::~LogImpl>
//	Destructor
//-----------------------------------------------------------------------------
			LogImpl::~LogImpl()
			{
				CloseLogFile();
			}

			void LogImpl::OpenLogFile()
			{
				if (m_filename.empty())
					return;

				if (!m_bAppendLog)
				{
					this->pFile = fopen(m_filename.c_str(), "w");
				}
				else
				{
					this->pFile = fopen(m_filename.c_str(), "a");
				}
				if (this->pFile == NULL)
				{
					std::cerr << "Could Not Open OZW Log File." << std::endl;
				}
				else
				{
					setlinebuf(this->pFile);
				}
			}

			void LogImpl::CloseLogFile()
			{
				if (this->pFile)
					fclose(this->pFile);
			}

//-----------------------------------------------------------------------------
//	<LogImpl::ReopenLogFile>
//	Reopens log file so that for example Logrotate can do its job
//-----------------------------------------------------------------------------
			void LogImpl::ReopenLogFile()
			{
				CloseLogFile();
				OpenLogFile();
			}

			unsigned int LogImpl::toEscapeCode(LogLevel _level)
			{
				unsigned int code = 39;

				switch (_level)
				{
					case LogLevel_Internal:
					case LogLevel_StreamDetail:
						code = 97;
						break;   // 97=bright white
					case LogLevel_Debug:
						code = 36;
						break;   // 36=cyan
					case LogLevel_Detail:
						code = 94;
						break;	 // 94=bright blue
					case LogLevel_Info:
						code = 39;
						break;   // 39=white
					case LogLevel_Alert:
						code = 93;
						break;   // 93=bright yellow
					case LogLevel_Warning:
						code = 33;
						break;   // 33=yellow
					case LogLevel_Error:
						code = 31;
						break;   // 31=red
					case LogLevel_Fatal:
						code = 95;
						break;	 // 95=bright magenta
					case LogLevel_Always:
						code = 32;
						break;   // 32=green
					case LogLevel_Invalid:
					case LogLevel_None:
						code = 39;
						break;   // 39=white (reset to default)
				}

				return code;
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
				string loglevelStr = GetLogLevelString(_logLevel);

				char lineBuf[1024] = { 0 };
				//int lineLen = 0;
				if (_format != NULL && _format[0] != '\0')
				{
					va_list saveargs;
					va_copy(saveargs, _args);

					vsnprintf(lineBuf, sizeof(lineBuf), _format, _args);
					va_end(saveargs);
				}

				std::string outBuf;

				outBuf.append(timeStr.GetAsString());
				outBuf.append(loglevelStr);
				outBuf.append(nodeStr);
				outBuf.append(lineBuf);
				outBuf.append("\n");

				// print message to file (and possibly screen)
				if (this->pFile != NULL)
				{
					fputs(outBuf.c_str(), pFile);
				}
				if (m_bConsoleOutput)
				{
					fprintf(stdout, "\x1B[%02um", toEscapeCode(_logLevel));
					fputs(outBuf.c_str(), stdout);
					fprintf(stdout, "\x1b[39m");
					/* always return to normal */
					fprintf(stdout, "\x1B[%02um", toEscapeCode(LogLevel_Info));
				}
			}

//-----------------------------------------------------------------------------
//	<LogImpl::GetNodeString>
//	Generate a string with formatted node id
//-----------------------------------------------------------------------------
			std::string LogImpl::GetNodeString(uint8 const _nodeId)
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
			std::string LogImpl::GetThreadId()
			{
				char buf[20];
				snprintf(buf, sizeof(buf), "%08lx ", (long unsigned int) pthread_self());
				string str = buf;
				return str;
			}

//-----------------------------------------------------------------------------
//	<LogImpl::SetLogFileName>
//	Provide a new log file name (applicable to future writes)
//-----------------------------------------------------------------------------
			void LogImpl::SetLogFileName(const string &_filename)
			{
				CloseLogFile();
				m_filename = _filename;
				OpenLogFile();
			}

		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave
