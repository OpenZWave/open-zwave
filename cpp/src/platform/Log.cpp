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
#include "Options.h"
#include "platform/Mutex.h"
#include "platform/Log.h"
#include "platform/TimeStamp.h"

#ifdef WIN32
#include "platform/windows/LogImpl.h"	// Platform-specific implementation of a log
#elif defined WINRT
#include "platform/winRT/LogImpl.h"	// Platform-specific implementation of a log
#else
#include "platform/unix/LogImpl.h"	// Platform-specific implementation of a log
#endif

using namespace OpenZWave;

char const *OpenZWave::LogLevelString[] =
{ 
"Internal", /**< LogLevel_Internal Used only within the log class (uses existing timestamp, etc.) */
"StreamDetail", /**< LogLevel_StreamDetail Will include low-level byte transfers from controller to buffer to application and back */
"Debug", /**< LogLevel_Debug Very detailed information on progress that will create a huge log file quickly
 But this level (as others) can be queued and sent to the log only on an error or warning */
"Detail", /**< LogLevel_Detail Detailed information on the progress of each message */
"Info", /**< LogLevel_Info Everything's working fine...these messages provide streamlined feedback on each message */
"Alert", /**< LogLevel_Alert Something unexpected by the library about which the controlling application should be aware */
"Warning", /**< LogLevel_Warning A minor issue from which the library should be able to recover */
"Error", /**< LogLevel_Error A serious issue with the library or the network */
"Fatal", /**< LogLevel_Fatal A likely fatal issue in the library */
"Always", /**< LogLevel_Always These messages should always be shown */
"None", /**< LogLevel_None Disable all logging */	
"Invalid" /**< Invalid Log Level Status - Used to Indicate error from Importing bad Options.xml */
};

Log* Log::s_instance = NULL;
std::vector<i_LogImpl*> Log::m_pImpls;
LogLevel Log::m_minLogLevel = LogLevel_Debug;
uint32 Log::m_currentDay = -1;
bool Log::m_doRotate = false;


//-----------------------------------------------------------------------------
//	<LogImpl::GetLogLevelString>
//	Provide a new log file name (applicable to future writes)
//-----------------------------------------------------------------------------
std::string i_LogImpl::GetLogLevelString(LogLevel _level)
{
	if ((_level >= LogLevel_Internal) && (_level <= LogLevel_Invalid))
	{
		char buf[20];
		snprintf(buf, sizeof(buf), "%s, ", LogLevelString[_level]);
		return buf;
	}
	else
		return "Unknown, ";
}

//-----------------------------------------------------------------------------
//	<Log::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
Log* Log::Create(string const& _filename, bool const _bAppend, bool const _bConsoleOutput, LogLevel const _saveLevel)
{
	if ( NULL == s_instance)
	{
		s_instance = new Log(_filename, _bAppend, _bConsoleOutput, _saveLevel);
	}
	else
	{
		Log::Destroy();
		s_instance = new Log(_filename, _bAppend, _bConsoleOutput, _saveLevel);
	}

	return s_instance;
}

//-----------------------------------------------------------------------------
//	<Log::Destroy>
//	Static method to destroy the logging singleton.
//-----------------------------------------------------------------------------
void Log::Destroy()
{
	delete s_instance;
	s_instance = NULL;
}

//-----------------------------------------------------------------------------
//	<Log::SetLoggingClass>
//	Set log class
//-----------------------------------------------------------------------------
bool Log::SetLoggingClass(i_LogImpl *LogClass, bool Append)
{
	if (!Append) {
		for (std::vector<i_LogImpl*>::iterator it = s_instance->m_pImpls.begin(); it != s_instance->m_pImpls.end();) {
			i_LogImpl *lc = *it;
			delete lc;
			it = s_instance->m_pImpls.erase(it);
		}
	}
	s_instance->m_pImpls.push_back(LogClass);
	return true;
}

//-----------------------------------------------------------------------------
//	<Log::SetLoggingState>
//	Set flag to actually write to log or skip it
//-----------------------------------------------------------------------------
void Log::SetLoggingState(LogLevel _saveLevel)
{
	m_minLogLevel = _saveLevel;
}

//-----------------------------------------------------------------------------
//	<Log::SetLoggingState>
//	Set flag to actually write to log or skip it
//-----------------------------------------------------------------------------
LogLevel Log::GetLoggingState()
{
	return m_minLogLevel;
}


//-----------------------------------------------------------------------------
//	<Log::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void Log::Write(LogLevel _level, char const* _format, ...)
{
 	if (_level < m_minLogLevel) 
		return;

	if (Log::m_doRotate)
	{ 
		Internal::Platform::TimeStamp ts;
		if (Log::m_currentDay != ts.getDay())
		{
			Log::RotateLogFile();
			Log::m_currentDay = ts.getDay();
		}
	}
	if (s_instance && (s_instance->m_pImpls.size() > 0))
	{
		if (_level != LogLevel_Internal)
			s_instance->m_logMutex->Lock();
		va_list args;
		va_start(args, _format);
		for (std::vector<i_LogImpl*>::iterator it = s_instance->m_pImpls.begin(); it != s_instance->m_pImpls.end(); it++)
			(*it)->Write(_level, 0, _format, args);
		va_end(args);
		if (_level != LogLevel_Internal)
			s_instance->m_logMutex->Unlock();
	}

}

//-----------------------------------------------------------------------------
//	<Log::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void Log::Write(LogLevel _level, uint8 const _nodeId, char const* _format, ...)
{
	if (_level <= m_minLogLevel)
		return;
	
	if (Log::m_doRotate)
	{ 
		Internal::Platform::TimeStamp ts;
		if (Log::m_currentDay != ts.getDay())
		{
			Log::RotateLogFile();
			Log::m_currentDay = ts.getDay();
		}
	}	
	
	if (s_instance && (s_instance->m_pImpls.size() > 0))
	{
		if (_level != LogLevel_Internal)
			s_instance->m_logMutex->Lock();
		va_list args;
		va_start(args, _format);
		for (std::vector<i_LogImpl*>::iterator it = s_instance->m_pImpls.begin(); it != s_instance->m_pImpls.end(); it++)
			(*it)->Write(_level, _nodeId, _format, args);
		va_end(args);
		if (_level != LogLevel_Internal)
			s_instance->m_logMutex->Unlock();
	}

}

//-----------------------------------------------------------------------------
//	<Log::SetLogFileName>
//	Change the name of the log file (will start writing a new file)
//-----------------------------------------------------------------------------
void Log::SetLogFileName(const string &_filename)
{
	if (s_instance && (s_instance->m_pImpls.size() > 0))
	{
		s_instance->m_logMutex->Lock();
		for (std::vector<i_LogImpl*>::iterator it = s_instance->m_pImpls.begin(); it !=s_instance->m_pImpls.end(); it++)
			(*it)->SetLogFileName(_filename);
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::ReopenLogFile>
//	Reopens currently open log file. (So for example logrotate can do its job)
//-----------------------------------------------------------------------------
void Log::RotateLogFile()
{
  if (s_instance && (s_instance->m_pImpls.size() > 0))
	{
		s_instance->m_logMutex->Lock();
		for (std::vector<i_LogImpl*>::iterator it = s_instance->m_pImpls.begin(); it !=s_instance->m_pImpls.end(); it++)
			(*it)->RotateLogFile();
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::Log>
//	Constructor
//-----------------------------------------------------------------------------
Log::Log(string const& _filename, bool const _bAppend, bool const _bConsoleOutput, LogLevel const _saveLevel) :
		m_logMutex(new Internal::Platform::Mutex())
{
	if (m_pImpls.size() == 0)
	{
		m_pImpls.push_back(new Internal::Platform::LogImpl(_filename, _bAppend, _bConsoleOutput));
		m_minLogLevel = _saveLevel;
	}

}

//-----------------------------------------------------------------------------
//	<Log::~Log>
//	Destructor
//-----------------------------------------------------------------------------
Log::~Log()
{
	m_logMutex->Release();
	for (std::vector<i_LogImpl*>::iterator it = s_instance->m_pImpls.begin(); it != s_instance->m_pImpls.end();) {
		i_LogImpl *lc = *it;
		delete lc;
		it = s_instance->m_pImpls.erase(it);
	}
}

//-----------------------------------------------------------------------------
//	<Log::SetupLogFileRotation>
//	Sets the Log Class to do Automatic Log Rotation at Midnight
//-----------------------------------------------------------------------------

void Log::SetupLogFileRotation()
{
	Options::Get()->GetOptionAsBool("RotateLogFile", &Log::m_doRotate);
	Internal::Platform::TimeStamp ts;
	Log::m_currentDay = ts.getDay();
}
