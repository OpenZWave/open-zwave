//-----------------------------------------------------------------------------
//
//	Supervision.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SUPERVISION
//
//	Copyright (c) 2020 Mark Ruys <mark@paracas.nl>
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

#ifndef _Supervision_H
#define _Supervision_H

#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{
			/** \brief Implements COMMAND_CLASS_SUPERVISION (0x6c), a Z-Wave device command class.
			 * \ingroup CommandClass
			 */
			class Supervision: public CommandClass
			{
				public:
                    enum SupervisionCmd
                    {
                        SupervisionCmd_Get = 0x01,
                        SupervisionCmd_Report = 0x02,
                    };
                    enum SupervisionMoreStatusUpdates
                    {
                        SupervisionMoreStatusUpdates_LastReport = 0x00,
                        SupervisionMoreStatusUpdates_MoreReports = 0x80,
                    };
                    enum SupervisionStatus
                    {
                        SupervisionStatus_NoSupport = 0x00,
                        SupervisionStatus_Working = 0x01,
                        SupervisionStatus_Fail = 0x02,
                        SupervisionStatus_Success = 0xff
                    };

					static CommandClass* Create(uint32 const _homeId, uint8 const _nodeId)
					{
						return new Supervision(_homeId, _nodeId);
					}
					virtual ~Supervision()
					{
					}

					static uint8 const StaticGetCommandClassId()
					{
						return 0x6c;
					}
					static string const StaticGetCommandClassName()
					{
						return "COMMAND_CLASS_SUPERVISION";
					}

// 					virtual uint8 GetMaxVersion() override
// 					{
// 						return 2;   // TODO support v2 too
// 					}

					uint8 GetSession(uint8 _command_class_id) ;
					static uint8 const StaticNoSessionId()
					{
						return 0xff; // As sessions are only 5 bits, this value will never match
					}

					// From CommandClass
					virtual uint8 const GetCommandClassId() const override
					{
						return StaticGetCommandClassId();
					}
					virtual string const GetCommandClassName() const override
					{
						return StaticGetCommandClassName();
					}
					virtual bool HandleIncomingMsg(uint8 const* _data, uint32 const _length, uint32 const _instance = 1) override;
					virtual bool HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance = 1) override;

				private:
					Supervision(uint32 const _homeId, uint8 const _nodeId) :
							CommandClass(_homeId, _nodeId)
					{
					}

                    static uint m_session_id;
                    uint m_command_class_id;    // TODO as implemented now we support only a single concurrent supervision call per CC

					void HandleSupervisionReport(uint8 const* _data, uint32 const _length, uint32 const _instance);
			};
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

#endif

