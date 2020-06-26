//-----------------------------------------------------------------------------
//
//	Indicator.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_INDICATOR
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

#ifndef _Indicator_H
#define _Indicator_H

#include "command_classes/CommandClass.h"
#include "TimerThread.h"


namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			/** \brief Implements COMMAND_CLASS_INDICATOR (0x87), a Z-Wave device command class.
			 * \ingroup CommandClass
			 */
			class Indicator: public CommandClass, private Timer
			{
				public:
					static CommandClass* Create(uint32 const _homeId, uint8 const _nodeId)
					{
						return new Indicator(_homeId, _nodeId);
					}
					virtual ~Indicator()
					{
					}

					static uint8 const StaticGetCommandClassId()
					{
						return 0x87;
					}
					static string const StaticGetCommandClassName()
					{
						return "COMMAND_CLASS_INDICATOR";
					}

					// From CommandClass
					virtual bool RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue) override;
					virtual bool RequestValue(uint32 const _requestFlags, uint16 const _index, uint8 const _instance, Driver::MsgQueue const _queue) override;
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
					virtual uint8 GetMaxVersion() override
					{
						return 4;
					}
					virtual void SetValueBasic(uint8 const _instance, uint8 const _value) override;
				protected:
					virtual void CreateVars(uint8 const _instance) override;

				private:
					struct Properties {
						uint8 id;
						uint8 instance;
						uint8 properties;
						string label;
					};
					void createIndicatorConfigValues(uint8 id);
					void setIndicatorValue(uint8 id, uint8 _instance, uint8 property, uint8 value);
					void refreshIndicator(uint32 id);
					Indicator(uint32 const _homeId, uint8 const _nodeId);
					std::map<uint8, std::shared_ptr<Properties> > m_indicatorLists;
			};
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

#endif

