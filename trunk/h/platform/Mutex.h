//-----------------------------------------------------------------------------
//
//	Mutex.h
//
//	Cross-platform mutex
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
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

#ifndef _Mutex_H
#define _Mutex_H

namespace OpenZWave
{
	class MutexImpl;

	class Mutex
	{
	public:

		/// <summary>	Constructor. </summary>
		/// <remarks>	Creates a mutex object that can be used to serialize access to a shared resource. </remarks>
		Mutex();

		/// <summary>	Destructor. </summary>
		/// <remarks>	Destroys the mutex object. </remarks>
		~Mutex();

		/// <summary>	Attempt to lock the mutex. </summary>
		/// <remarks>	There must be a matching call to Release for every call to Lock.</remarks>
		/// <param name="_bWait">	Defaults to true.  Set this argument to false if the method should return immediately, even if the lock is not available. </param>
		/// <returns>	true if the lock was obtained. </returns>
		bool Lock( bool const _bWait = true );

		/// <summary>	Releases the mutex. </summary>
		/// <remarks>	There must be a matching call to Release for every call to Lock. </remarks>
		void Release();

	private:
		Mutex( Mutex const&	);					// declared but not implemented, to prevent copying
		Mutex& operator = ( Mutex const& );		// declared but not implemented, to prevent assignment

		MutexImpl*	m_pImpl;					// Pointer to an object that encapsulates the platform-specific implementation of a mutex.
		int m_lockCount;
	};

} // namespace OpenZWave

#endif //_Mutex_H

