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
#include "Driver.h"

namespace OpenZWave
{
	namespace Internal
	{

		DNSThread::DNSThread(Driver *driver) :
				m_driver(driver), m_dnsMutex(new Internal::Platform::Mutex()), m_dnsRequestEvent(new Internal::Platform::Event())
		{
		}

		DNSThread::~DNSThread()
		{
			m_dnsMutex->Release();
			m_dnsRequestEvent->Release();
		}

		void DNSThread::DNSThreadEntryPoint(Internal::Platform::Event* _exitEvent, void* _context)
		{
			DNSThread* dns = (DNSThread*) _context;
			if (dns)
			{
				dns->DNSThreadProc(_exitEvent);
			}
		}

		void DNSThread::DNSThreadProc(Internal::Platform::Event* _exitEvent)
		{
			Log::Write(LogLevel_Info, "Starting DNSThread");
			while (true)
			{
				// DNSThread has been initialized
				const uint32 count = 2;

				Internal::Platform::Wait* waitObjects[count];

				int32 timeout = Internal::Platform::Wait::Timeout_Infinite;
//		timeout = 5000;

				waitObjects[0] = _exitEvent;				// Thread must exit.
				waitObjects[1] = m_dnsRequestEvent;			// DNS Request
				// Wait for something to do

				int32 res = Internal::Platform::Wait::Multiple(waitObjects, count, timeout);

				switch (res)
				{
					case -1: /* timeout */
						Log::Write(LogLevel_Warning, "DNSThread Timeout...");
						break;
					case 0: /* exitEvent */
						Log::Write(LogLevel_Info, "Stopping DNSThread");
						return;
					case 1: /* dnsEvent */
						processResult();
						break;
				}
			}
		}

		bool DNSThread::sendRequest(DNSLookup *lookup)
		{
			Log::Write(LogLevel_Info, lookup->NodeID, "Queuing Lookup on %s for Node %d", lookup->lookup.c_str(), lookup->NodeID);
			LockGuard LG(m_dnsMutex);
			m_dnslist.push_back(lookup);
			m_dnsRequestEvent->Set();
			return true;
		}

		void DNSThread::processResult()
		{
			string result;
			Internal::DNSLookup *lookup;
			{
				LockGuard LG(m_dnsMutex);
				lookup = m_dnslist.front();
				m_dnslist.pop_front();
				if (m_dnslist.empty())
					m_dnsRequestEvent->Reset();
			}
			Log::Write(LogLevel_Info, "LookupTxT Checking %s", lookup->lookup.c_str());
			if (!m_dnsresolver.LookupTxT(lookup->lookup, lookup->result))
			{
				Log::Write(LogLevel_Warning, "Lookup on %s Failed", lookup->lookup.c_str());
			}
			else
			{
				Log::Write(LogLevel_Info, "Lookup for %s returned %s", lookup->lookup.c_str(), lookup->result.c_str());
			}
			lookup->status = m_dnsresolver.status;

			/* send the response back to the Driver for processing */
			Driver::EventMsg *event = new Driver::EventMsg();
			event->type = Driver::EventMsg::Event_DNS;
			event->event.lookup = lookup;
			this->m_driver->SubmitEventMsg(event);

		}
	} // namespace Internal
} // namespace OpenZWave
