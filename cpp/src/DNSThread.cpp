//-----------------------------------------------------------------------------
//
//	DNSThread.h
//
//	Async DNS Lookups
//
//	Copyright (c) 2016 Justin Hammond <justin@dynam.ac>
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

#include "DNSThread.h"
#include "Utils.h"

using namespace OpenZWave;


DNSThread::DNSThread
(
Driver *driver
):
m_driver (driver),
m_dnsMutex ( new Mutex() ),
m_dnsRequestEvent ( new Event() )
{
}

DNSThread::~DNSThread
(
)
{

}

void DNSThread::DNSThreadEntryPoint
(
		Event* _exitEvent,
		void* _context
)
{
	DNSThread* dns = (DNSThread*)_context;
	if( dns )
	{
		dns->DNSThreadProc( _exitEvent );
	}
}

void DNSThread::DNSThreadProc
(
Event* _exitEvent
)
{
	Log::Write(LogLevel_Info, "Starting DNSThread");
	while( true )
	{
		// DNSThread has been initialised
		uint32 count = 2;

		Wait* waitObjects[count];

		int32 timeout = Wait::Timeout_Infinite;
//		timeout = 5000;


		waitObjects[0] = _exitEvent;				// Thread must exit.
		waitObjects[1] = m_dnsRequestEvent;			// DNS Request
		// Wait for something to do

		int32 res = Wait::Multiple( waitObjects, count, timeout );

		switch (res) {
			case -1: /* timeout */
				Log::Write(LogLevel_Warning, "DNSThread Timeout...");
				break;
			case 0: /* exitEvent */
				Log::Write(LogLevel_Info, "Stopping DNSThread");
				return;
			case 1: /* dnsEvent */
				processRequest();
				break;
		}
	}
}

bool DNSThread::sendRequest
(
DNSLookup *lookup
)
{
	Log::Write(LogLevel_Info, lookup->NodeID, "Performing Lookup on %s for Node %d", lookup->lookup.c_str(), lookup->NodeID);
	LockGuard LG(m_dnsMutex);
	m_dnslist.push_back(lookup);
	m_dnsRequestEvent->Set();
	return true;
}

void DNSThread::processRequest
(
)
{
	string result;
	LockGuard LG(m_dnsMutex);
	DNSLookup *lookup = m_dnslist.front();
	m_dnslist.pop_front();

	Log::Write(LogLevel_Debug, "LookupTxT Checking %s", lookup->lookup.c_str());
	if (!m_dnsresolver.LookupTxT(lookup->lookup, lookup->result)) {
		Log::Write(LogLevel_Warning, "Lookup on %s Failed", lookup->lookup.c_str());
	} else {
		Log::Write(LogLevel_Debug, "Lookup for %s returned %s", lookup->lookup.c_str(), lookup->result.c_str());
	}
	lookup->status = m_dnsresolver.status;
	m_dnsRequestEvent->Reset();
	/* send the response back to the Driver for processing */

	this->m_driver->processConfigRevision(lookup);
}


