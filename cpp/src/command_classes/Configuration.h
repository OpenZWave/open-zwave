//-----------------------------------------------------------------------------
//
//	Configuration.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_CONFIGURATION
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

#ifndef _Configuration_H
#define _Configuration_H

#include <list>
#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			/** \brief Implements COMMAND_CLASS_CONFIGURATION (0x70), a Z-Wave device command class.
			 * \ingroup CommandClass
			 */
			class Configuration: public CommandClass
			{
					friend class Node;

				public:
					static CommandClass* Create(uint32 const _homeId, uint8 const _nodeId)
					{
						return new Configuration(_homeId, _nodeId);
					}
					virtual ~Configuration()
					{
					}

					static uint8 const StaticGetCommandClassId()
					{
						return 0x70;
					}
					static string const StaticGetCommandClassName()
					{
						return "COMMAND_CLASS_CONFIGURATION";
					}

					virtual bool RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue) override;
					virtual bool RequestValue(uint32 const _requestFlags, uint16 const _parameter, uint8 const _index, Driver::MsgQueue const _queue) override;
					void Set(uint16 const _parameter, int32 const _value, uint8 const _size);

					// From CommandClass
					virtual uint8 const GetCommandClassId() const override
					{
						return StaticGetCommandClassId();
					}
					virtual string const GetCommandClassName() const override
					{
						return StaticGetCommandClassName();
					}
					virtual bool HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance = 1) override;
					virtual bool SetValue(Internal::VC::Value const& _value) override;
					uint8 GetMaxVersion() override
					{
						return 4;
					}
				private:
					Configuration(uint32 const _homeId, uint8 const _nodeId) :
							CommandClass(_homeId, _nodeId)
					{
					}
					

					enum CC_Param_Format {
						CC_Param_Format_Signed = 0x00,
						CC_Param_Format_Unsigned = 0x01,
						CC_Param_Format_List = 0x02,
						CC_Param_Format_BitSet = 0x03
					};
					enum CC_Param_Size {
						CC_Param_Size_Unassigned = 0x00,
						CC_Param_Size_Byte = 0x01,
						CC_Param_Size_Short = 0x02,
						CC_Param_Size_Int = 0x04,
					};

					uint32 getField(const uint8 *data, CC_Param_Size size, uint8 &pos);
					bool processConfigParams();

					class ConfigParam {
						public:
							uint16 paramNo;
							uint8 instance;
							string name;
							string help;
							uint32 min;
							uint32 max;
							uint32 defaultval;
							CC_Param_Size size;
							CC_Param_Format format;
							uint8 flags;
					};
					enum ConfigParamFlags {
						ConfigParamFlags_Info_Done = 0x01,
						ConfigParamFlags_Name_Done = 0x02,
						ConfigParamFlags_Help_Done = 0x04,
					};
					std::map<uint16, ConfigParam> m_ConfigParams;

			};
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

#endif

