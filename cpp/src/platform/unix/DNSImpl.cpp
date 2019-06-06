//-----------------------------------------------------------------------------
//
//	DNSImpl.cpp
//
//	Unix DNS Lookup Routines.
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

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <string.h>
#include <netdb.h>

#include "DNSImpl.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{

			DNSImpl::DNSImpl() :
					status(DNSError_None)
			{
				res_init();
			}

			DNSImpl::~DNSImpl()
			{

			}

			bool DNSImpl::LookupTxT(string lookup, string &result)
			{

				int response;
				unsigned char query_buffer[1024];
				ns_msg nsMsg;
				ns_rr rr;
				const unsigned char *p, *start;
				unsigned char l;
				int rrlen;

				char outb[1025];

#ifdef __APPLE_CC__
				response = res_query(lookup.c_str(), ns_c_in, ns_t_txt, query_buffer, sizeof(query_buffer));
#else
				response= res_query(lookup.c_str(), C_IN, ns_t_txt, query_buffer, sizeof(query_buffer));
#endif
				if (response < 0)
				{
					Log::Write(LogLevel_Warning, "Error looking up txt Record: %s - %s", lookup.c_str(), hstrerror(h_errno));
					switch (h_errno)
					{
						case HOST_NOT_FOUND:
							status = DNSError_NotFound;
							break;
						case NO_DATA:
							status = DNSError_NotFound;
							break;
						case NO_RECOVERY:
							status = DNSError_InternalError;
							break;
						case TRY_AGAIN:
							status = DNSError_InternalError;
							break;
						default:
							status = DNSError_InternalError;
							break;

					}
					return false;
				}

				ns_initparse(query_buffer, response, &nsMsg);

				ns_parserr(&nsMsg, ns_s_an, 0, &rr);

				p = start = ns_rr_rdata(rr);
				rrlen = ns_rr_rdlen(rr);
				if (rrlen > 1024)
				{
					status = DNSError_InternalError;
					return false;
				}
				while (p < start + rrlen)
				{
					l = *p;
					p++;
					if (p + l > start + rrlen)
					{
						break;
					}
					memcpy(outb, p, l);
					outb[l] = 0;
					p += l;
				}
				result = outb;
				status = DNSError_None;
				return true;
			}
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave
