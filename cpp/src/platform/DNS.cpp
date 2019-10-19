//-----------------------------------------------------------------------------
//
//	DNS.cpp
//
//	Cross-platform DNS Operations
//
//	Copyright (c) 2015 Justin Hammond <justin@dynam.ac>
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
#include "platform/DNS.h"

#ifdef WIN32
#include "platform/windows/DNSImpl.h"	// Platform-specific implementation of a DNS Operations
#elif defined WINRT
#include "platform/winRT/DNSImpl.h"	// Platform-specific implementation of a DNS Operations
#else
#include "platform/unix/DNSImpl.h"	// Platform-specific implementation of a DNS Operations
#endif

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{

			DNS::DNS() :
					status(DNSError_None)
			{
				this->m_pImpl = new DNSImpl();
			}
			DNS::~DNS()
			{
				delete this->m_pImpl;
			}

			bool DNS::LookupTxT(string lookup, string &result)
			{
				bool ret = this->m_pImpl->LookupTxT(lookup, result);
				status = this->m_pImpl->status;
				return ret;
			}
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave
