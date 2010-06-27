//-----------------------------------------------------------------------------
//
//      ZWManager.cpp
//
//      Cli/C++ wrapper for the C++ OpenZWave Manager class
//
//      Copyright (c) 2010 Amer Harb <harb_amer@hotmail.com>
//
//      SOFTWARE NOTICE AND LICENSE
//
//      This file is part of OpenZWave.
//
//      OpenZWave is free software: you can redistribute it and/or modify
//      it under the terms of the GNU Lesser General Public License as published
//      by the Free Software Foundation, either version 3 of the License,
//      or (at your option) any later version.
//
//      OpenZWave is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public License
//      along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------
#include "ZWManager.h"

using namespace OpenZWaveDotNet;
using namespace OpenZWave;
using namespace Runtime::InteropServices;

//-----------------------------------------------------------------------------
//	<ZWManager::Create>
//	Create the unmanaged Manager singleton object, and add a watcher
//-----------------------------------------------------------------------------
void ZWManager::Create
(
	String^ configPath,
	String^	userPath
)
{
	// Create the Manager singleton
	const char* config = (const char*)(Marshal::StringToHGlobalAnsi(configPath)).ToPointer();
	const char* user = (const char*)(Marshal::StringToHGlobalAnsi(userPath)).ToPointer();
	Manager::Create( config, user );

	// Add a notification handler
	m_onNotification = gcnew OnNotificationFromUnmanagedDelegate( this, &ZWManager::OnNotificationFromUnmanaged );
	m_gch = GCHandle::Alloc( m_onNotification ); 
	IntPtr ip = Marshal::GetFunctionPointerForDelegate( m_onNotification );
	Manager::Get()->AddWatcher( (Manager::pfnOnNotification_t)ip.ToPointer(), NULL );
}

//-----------------------------------------------------------------------------
//	<ZWManager::OnNotificationFromUnmanaged>
//	Trigger an event from the unmanaged notification callback
//-----------------------------------------------------------------------------
void ZWManager::OnNotificationFromUnmanaged
(
	Notification* _notification,
	void* _context
)
{
	ZWNotification^ notification = gcnew ZWNotification( _notification );
	OnZWNotification(notification);
}
