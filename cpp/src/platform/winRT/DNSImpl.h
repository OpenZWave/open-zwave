//-----------------------------------------------------------------------------
//
//	DNSImpl.h
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

#ifndef _DNSImpl_H
#define _DNSImpl_H

#include "Defs.h"
#include "platform/DNS.h"

namespace OpenZWave
{
	class DNSImpl {
		public:
			DNSImpl();
			virtual ~DNSImpl();
			virtual bool LookupTxT(string, string &);
			DNSError status;
		private:

	};
}
#endif
