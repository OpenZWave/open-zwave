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

#include <string>
#include <map>
#include <list>
#include "Defs.h"
#include "CommandClassCommands.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{
			class CommandClass;

			/** \brief Manages a map of command classes supported by a specific Z-Wave node.
			 * \ingroup CommandClass
			 */
			class CommandClasses
			{
				public:
					typedef CommandClass* (*pfnCreateCommandClass_t)(uint32 const _homeId, uint8 const _nodeId);

					static void RegisterCommandClasses();
					static CommandClass* CreateCommandClass(ZW_CommandClasses const _commandClassId, uint32 const _homeId, uint8 const _nodeId);

					static bool IsSupported(uint16_t const _commandClassId);
					static string GetName(uint16_t const _commandClassId);
					static ZW_CommandClasses GetZWCommandClass(uint16_t const _commandClassId);
					static list<ZW_CommandClasses> GetAdvertisedCommandClasses();
				private:
					CommandClasses();
					CommandClasses(CommandClasses const&);					// prevent copy
					CommandClasses& operator =(CommandClasses const&);		// prevent assignment

					static CommandClasses& Get()
					{
						static CommandClasses instance;
						return instance;
					}

					void Register(ZW_CommandClasses const _commandClassId, string const& _commandClassName, pfnCreateCommandClass_t _create, bool advertised = false);
					uint16_t GetCommandClassId(string const& _name);

					map<ZW_CommandClasses, pfnCreateCommandClass_t> m_commandClassCreators;
					map<string, uint16_t> m_namesToIDs;
					/* a list of CommandClasses that are advertised on the controllers NIF packet and can be controlled
					 * via other Nodes
					 */
					list<ZW_CommandClasses> m_advertisedCommandClasses;

					// m_supportedCommandClasses uses single bits to mark whether OpenZWave supports a command class
					// Checking this is not the same as looking for non-NULL entried in m_commandClassCreators, since
					// this may be modified by the program options --Include and --Ingnore to filter out support
					// for unwanted command classes.
					//list<ZW_CommandClasses> m_supportedCommandClasses;
//					uint32 m_supportedCommandClasses[16];
			};
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

#endif
