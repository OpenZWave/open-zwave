//-----------------------------------------------------------------------------
//
//	NotificationCCTypes.h
//
//	NotificationCCTypes for Notification Command Class
//
//	Copyright (c) 2018 Justin Hammond <justin@dynam.ac>
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

#ifndef NOTIFICATIONCCTYPES_H
#define NOTIFICATIONCCTYPES_H

#include <cstdio>
#include <string>
#include <map>
#include "Defs.h"
#include "Driver.h"
#include "command_classes/CommandClass.h"

namespace OpenZWave
{

class NotificationCCTypes
{
	enum NotificationEventParamTypes {
		NEPT_Location = 0x01,
		NEPT_List,
		NEPT_UserCodeReport
	};

	class NotificationEventParams {
	public:
		uint32 id;
		string name;
		uint8 type;
	};
	class NotificationEvents {
	public:
		uint32 id;
		string name;
		std::map<uint32, NotificationCCTypes::NotificationEventParams* > EventParams;
	};
	class NotificationTypes {
	public:
		uint32 id;
		string name;
		std::map<uint32, NotificationCCTypes::NotificationEvents *> Events;
	};


	//-----------------------------------------------------------------------------
    // Construction
    //-----------------------------------------------------------------------------
private:
    NotificationCCTypes();
    ~NotificationCCTypes();
    static void ReadXML();
public:
    static NotificationCCTypes* Get();
    void test() { return; };
    //-----------------------------------------------------------------------------
    // Instance Functions
    //-----------------------------------------------------------------------------
private:
    static NotificationCCTypes* m_instance;
    static std::map<uint32, NotificationCCTypes::NotificationTypes *> Notifications;
};

};
#endif // VALUEHELP_H
