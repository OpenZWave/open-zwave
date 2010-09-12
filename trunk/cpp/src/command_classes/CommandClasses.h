//-----------------------------------------------------------------------------
//
//	CommandClasses.h
//
//	Singleton holding methods to create each command class object
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

#ifndef _CommandClasses_H
#define _CommandClasses_H

#include "Defs.h"

namespace OpenZWave
{
	class CommandClass;

	class CommandClasses
	{
	public:
		typedef CommandClass* (*pfnCreateCommandClass_t)( uint32 const _homeId, uint8 const _nodeId );

		static void RegisterCommandClasses();
		static CommandClass* CreateCommandClass( uint8 const _commandClassId, uint32 const _homeId, uint8 const _nodeId );

	private:
		CommandClasses();										
		CommandClasses( CommandClasses const&	);					// prevent copy
		CommandClasses& operator = ( CommandClasses const& );		// prevent assignment

		static CommandClasses& Get()
		{
			static CommandClasses instance;
			return instance;
		}

		void Register( uint8 const _commandClassId, pfnCreateCommandClass_t _create );

		pfnCreateCommandClass_t m_commandClassCreators[256];
	};

} // namespace OpenZWave


#endif
