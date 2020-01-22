//-----------------------------------------------------------------------------
//
//	DNSImpl.cpp
//
//	Windows DNS Lookup Routines.
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

#include <windows.h>
#include <winerror.h>
#include <windns.h>
#include <string.h>

#include "DNSImpl.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{

			DNSImpl::DNSImpl()
			{

			}

			DNSImpl::~DNSImpl()
			{

			}

			bool DNSImpl::LookupTxT(string lookup, string &result)
			{

				PDNS_RECORD qr, rp;
				DNS_STATUS rc;

				rc = DnsQuery(lookup.c_str(), DNS_TYPE_TEXT, DNS_QUERY_STANDARD, NULL, &qr, NULL);
				if (rc != ERROR_SUCCESS)
				{
					Log::Write(LogLevel_Warning, "Error looking up txt Record: %s - %d", lookup.c_str(), rc);
					status = DNSError_InternalError;
					return false;
				}

				for (rp = qr; rp != NULL; rp = rp->pNext)
				{
					if (rp->wType == DNS_TYPE_TEXT)
					{
						result = rp->Data.TXT.pStringArray[0];
						status = DNSError_None;
						break;
					}
				}

				DnsRecordListFree(qr, DnsFreeRecordList);

				return true;
			}
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave
