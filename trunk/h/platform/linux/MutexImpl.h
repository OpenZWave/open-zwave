//-----------------------------------------------------------------------------
//
//	MutexImpl.h
//
//	Linux implementation of the cross-platform mutex
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#ifndef _MutexImpl_H
#define _MutexImpl_H

#include <pthread.h>


namespace OpenZWave
{
	class MutexImpl
	{
	private:
		friend class Mutex;

		MutexImpl();
		~MutexImpl();

		bool Lock( bool const _bWait = true );
		void Release();

		pthread_mutex_t	m_mutex;
	};

} // namespace OpenZWave

#endif //_MutexIF_H

