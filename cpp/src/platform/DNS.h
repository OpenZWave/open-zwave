//-----------------------------------------------------------------------------
//
//	DNS.h
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
#ifndef _DNS_H
#define _DNS_H

#include <stdarg.h>
#include <string>
#include "Defs.h"
#include "platform/Log.h"


namespace OpenZWave
{
	class DNSImpl;

	enum DNSError
	{
		DNSError_None = 0,
		DNSError_NotFound,					/**< No Record Exists - There for no Config File exists */
		DNSError_DomainError,					/**< Domain didn't resolve etc */
		DNSError_InternalError					/**< A Internal Error Occured */
	};


	/** \brief Implements platform-independent File Operations.
	 */
	class DNS
	{
	public:
			DNS();
			~DNS();
			bool LookupTxT(string, string &);
			DNSError status;
	private:
			DNSImpl *m_pImpl;
	};
}

#endif
