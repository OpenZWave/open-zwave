//-----------------------------------------------------------------------------
//
//	Version.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_VERSION
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

#include "command_classes/CommandClasses.h"
#include "command_classes/Version.h"
#include "Defs.h"
#include "Msg.h"
#include "Driver.h"
#include "Node.h"
#include "platform/Log.h"

#include "value_classes/ValueString.h"

#include "tinyxml.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum VersionCmd
			{
				VersionCmd_Get = 0x11,
				VersionCmd_Report = 0x12,
				VersionCmd_CommandClassGet = 0x13,
				VersionCmd_CommandClassReport = 0x14
			};

//-----------------------------------------------------------------------------
// <Version::Version>
// Constructor
//-----------------------------------------------------------------------------
			Version::Version(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId)
			{
				m_com.EnableFlag(COMPAT_FLAG_VERSION_GETCLASSVERSION, true);
				SetStaticRequest(StaticRequest_Values);
			}

//-----------------------------------------------------------------------------
// <Version::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool Version::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
				{
					return RequestValue(_requestFlags, 0, _instance, _queue);
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <Version::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool Version::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,		// = 0
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_instance != 1)
				{
					// This command class doesn't work with multiple instances
					return false;
				}
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Msg* msg = new Msg("VersionCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(VersionCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "VersionCmd_Get Not Supported on this node");
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <Version::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool Version::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (Node* node = GetNodeUnsafe())
				{
					if (VersionCmd_Report == (VersionCmd) _data[0])
					{
						char library[8];
						char protocol[16];
						char application[16];

						snprintf(library, sizeof(library), "%d", _data[1]);
						snprintf(protocol, sizeof(protocol), "%d.%.2d", _data[2], _data[3]);
						snprintf(application, sizeof(application), "%d.%.2d", _data[4], _data[5]);

						Log::Write(LogLevel_Info, GetNodeId(), "Received Version report from node %d: Library=%s, Protocol=%s, Application=%s", GetNodeId(), library, protocol, application);
						ClearStaticRequest(StaticRequest_Values);

						if (Internal::VC::ValueString* libraryValue = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_Version::Library)))
						{
							libraryValue->OnValueRefreshed(library);
							libraryValue->Release();
						}
						if (Internal::VC::ValueString* protocolValue = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_Version::Protocol)))
						{
							protocolValue->OnValueRefreshed(protocol);
							protocolValue->Release();
						}
						if (Internal::VC::ValueString* applicationValue = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_Version::Application)))
						{
							applicationValue->OnValueRefreshed(application);
							applicationValue->Release();
						}

						return true;
					}

					if (VersionCmd_CommandClassReport == (VersionCmd) _data[0])
					{
						if (CommandClass* pCommandClass = node->GetCommandClass(_data[1]))
						{
							Log::Write(LogLevel_Info, GetNodeId(), "Received CommandClass Version report from node %d: CommandClass=%s, Version=%d", GetNodeId(), pCommandClass->GetCommandClassName().c_str(), _data[2]);
							pCommandClass->ClearStaticRequest(StaticRequest_Version);
							/* some devices advertise CommandClasses, but return version as 0. In General this means
							 * that the device doesn't actually support the CommandClass. So lets Remove it
							 */

							if (_data[2] > 0)
							{
								pCommandClass->SetVersion(_data[2]);
							}
							else
							{
								Log::Write(LogLevel_Warning, GetNodeId(), "CommandClass Version is 0, Removing CommandClass %s", pCommandClass->GetCommandClassName().c_str());
								GetNodeUnsafe()->RemoveCommandClass(_data[2]);
							}
						}

						return true;
					}
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <Version::RequestCommandClassVersion>
// Request the version of a command class used by the device
//-----------------------------------------------------------------------------
			bool Version::RequestCommandClassVersion(CommandClass const* _commandClass)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_VERSION_GETCLASSVERSION))
				{
					if (_commandClass->HasStaticRequest(StaticRequest_Version))
					{
						Msg* msg = new Msg("VersionCmd_CommandClassGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->Append(GetNodeId());
						msg->Append(3);
						msg->Append(GetCommandClassId());
						msg->Append(VersionCmd_CommandClassGet);
						msg->Append(_commandClass->GetCommandClassId());
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);
						return true;
					}
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <Version::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void Version::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					node->CreateValueString(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_Version::Library, "Library Version", "", true, false, "Unknown", 0);
					node->CreateValueString(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_Version::Protocol, "Protocol Version", "", true, false, "Unknown", 0);
					node->CreateValueString(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_Version::Application, "Application Version", "", true, false, "Unknown", 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
