//-----------------------------------------------------------------------------
//
//	Msg.cpp
//
//	Represents a Z-Wave message
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

#include "Defs.h"
#include "MsgNew.h"
#include "Node.h"
#include "Manager.h"
#include "Utils.h"
#include "ZWSecurity.h"
#include "platform/Log.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/Security.h"
#include "aes/aescpp.h"

namespace OpenZWave
{ 
	namespace Internal 
	{
		void test() {
			//OpenZWave::Internal::MsgNew<OpenZWave::Alarm_Cmd>* msg = new OpenZWave::Internal::MsgNew<OpenZWave::Alarm_Cmd>(ZW_Message_Type::Request, ZW_Msg_Func::Send_Data, true, true, ZW_Callback_Func::Command_Handler, 1);
			//msg->setCommandClassCmd(Alarm_Cmd::Get_Supported);
			//msg->setTargetNode(1);
			//std::cout << msg->GetAsString() << std::endl;

			//OpenZWave::Internal::MsgBase *msgbase = msg;
			//std::cout << msgbase->GetAsString() << std::endl;
			//test2(msg);

		}
		void test2(MsgBase *test) {
			//std::cout << test->GetAsString() << std::endl;
		}

		/* Callback for normal messages start at 10. Special Messages using a Callback prior to 10 */
		uint8 MsgBase::s_nextCallbackId = 10;

//-----------------------------------------------------------------------------
// <MsgBase::MsgBase>
// Constructor
//-----------------------------------------------------------------------------
		MsgBase::MsgBase(CC::CommandClass *cc, ZW_Message_Type const _msgType, ZW_Msg_Func const _function, bool const _bCallbackRequired, bool const _bReplyRequired, ZW_Callback_Func const _expectedReply, uint8 const _expectedCommandClassId) :
				m_logText("INVALID"), 
				m_bFinal(false), 
				m_bCallbackRequired(_bCallbackRequired), 
				m_callbackId(0), 
				m_expectedReply(0), 
				m_expectedCommandClassId(_expectedCommandClassId), 
				m_length(0), 
				m_targetNodeId(0), 
				m_sendAttempts(0), 
				m_maxSendAttempts( MAX_TRIES), 
				m_instance(1), 
				m_endPoint(0), 
				m_flags(0), 
				m_encrypted(false), 
				m_noncerecvd(false), 
				m_msgType(_msgType),
				m_msgFunc(_function),
				m_isBridgeController(false),
				m_cc(cc)
		{
			if (_bReplyRequired)
			{
				// Wait for this message before considering the transaction complete
				m_expectedReply = _expectedReply ? _expectedReply : _function;
			}

			memset(m_buffer, 0x00, 256);
			memset(e_buffer, 0x00, 256);
			memset(m_databuffer, 0x00, 256);

			//m_buffer[0] = SOF;
			//m_buffer[1] = 0;					// Length of the following data, filled in during Finalize.
			//m_buffer[2] = _msgType;
			//m_buffer[3] = _function;
		}



//-----------------------------------------------------------------------------
// <MsgBase::Finalize>
// Fill in the length and checksum values for the message
//-----------------------------------------------------------------------------
		void MsgBase::Finalize()
		{
			if (m_bFinal)
			{
				// Already finalized
				return;
			}

			// Deal with Multi-Channel/Instance encapsulation
			if ((m_flags & (m_MultiChannel | m_MultiInstance)) != 0)
			{
				MultiEncap();
			}

			// Add the callback id
			if (m_bCallbackRequired)
			{
				// Set the length byte
				m_buffer[1] = m_length;		// Length of following data

				if (0 == s_nextCallbackId)
				{
					s_nextCallbackId = 10;
				}

				m_buffer[m_length++] = s_nextCallbackId;
				m_callbackId = s_nextCallbackId++;
			}
			else
			{
				// Set the length byte
				m_buffer[1] = m_length - 1;		// Length of following data
			}

			// Calculate the checksum
			uint8 checksum = 0xff;
			for (uint32 i = 1; i < m_length; ++i)
			{
				checksum ^= m_buffer[i];
			}
			m_buffer[m_length++] = checksum;

			m_bFinal = true;
		}

//-----------------------------------------------------------------------------
// <MsgBase::UpdateCallbackId>
// If this message has a callback ID, increment it and recalculate the checksum
//-----------------------------------------------------------------------------
		void MsgBase::UpdateCallbackId()
		{
			if (m_bCallbackRequired)
			{
				if (0 == s_nextCallbackId)
				{
					s_nextCallbackId = 10;
				}

				// update the callback ID
				m_buffer[m_length - 2] = s_nextCallbackId;
				m_callbackId = s_nextCallbackId++;

				// Recalculate the checksum
				uint8 checksum = 0xff;
				for (int32 i = 1; i < m_length - 1; ++i)
				{
					checksum ^= m_buffer[i];
				}
				m_buffer[m_length - 1] = checksum;
			}
		}

//-----------------------------------------------------------------------------
// <MsgBase::GetAsString>
// Create a string containing the raw data
//-----------------------------------------------------------------------------
		std::string MsgBase::GetAsString()
		{
			string str = m_logText;

			char byteStr[16];
			if (m_targetNodeId != 0xff)
			{
				snprintf(byteStr, sizeof(byteStr), " (Node=%d)", m_targetNodeId);
				str += byteStr;
			}

			str += ": ";

			for (uint32 i = 0; i < m_length; ++i)
			{
				if (i)
				{
					str += ", ";
				}

				snprintf(byteStr, sizeof(byteStr), "0x%.2x", m_buffer[i]);
				str += byteStr;
			}

			return str;
		}

//-----------------------------------------------------------------------------
// <MsgBase::MultiEncap>
// Encapsulate the data inside a MultiInstance/Multicommand message
//-----------------------------------------------------------------------------
		void MsgBase::MultiEncap()
		{
			char str[256];
			if (m_buffer[3] != FUNC_ID_ZW_SEND_DATA)
			{
				return;
			}

			// Insert the encap header
			if ((m_flags & m_MultiChannel) != 0)
			{
				// MultiChannel
				for (uint32 i = m_length - 1; i >= 6; --i)
				{
					m_buffer[i + 4] = m_buffer[i];
				}

				m_buffer[5] += 4;
				m_buffer[6] = Internal::CC::MultiInstance::StaticGetCommandClassId();
				m_buffer[7] = Internal::CC::MultiInstance::MultiChannelCmd_Encap;
				m_buffer[8] = 1;
				m_buffer[9] = m_endPoint;
				m_length += 4;

				snprintf(str, sizeof(str), "MultiChannel Encapsulated (instance=%d): %s", m_instance, m_logText.c_str());
				m_logText = str;
			}
			else
			{
				// MultiInstance
				for (uint32 i = m_length - 1; i >= 6; --i)
				{
					m_buffer[i + 3] = m_buffer[i];
				}

				m_buffer[5] += 3;
				m_buffer[6] = Internal::CC::MultiInstance::StaticGetCommandClassId();
				m_buffer[7] = Internal::CC::MultiInstance::MultiInstanceCmd_Encap;
				m_buffer[8] = m_instance;
				m_length += 3;

				snprintf(str, sizeof(str), "MultiInstance Encapsulated (instance=%d): %s", m_instance, m_logText.c_str());
				m_logText = str;
			}
		}

//-----------------------------------------------------------------------------
// <Node::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
		OpenZWave::Driver* MsgBase::GetDriver() const
		{
			return (this->m_cc->GetDriver());
		}

		uint8* MsgBase::GetBuffer()
		{
			Log::Write(LogLevel_Info, m_targetNodeId, "Encrypted Flag is %d", m_encrypted);
			if (m_encrypted == false)
				return m_buffer;
			else if (EncryptBuffer(m_buffer, m_length, GetDriver(), GetDriver()->GetControllerNodeId(), m_targetNodeId, m_nonce, e_buffer))
			{
				return e_buffer;
			}
			else
			{
				Log::Write(LogLevel_Warning, m_targetNodeId, "Failed to Encrypt Packet");
				return NULL;
			}
		}
	} // namespace Internal
} // namespace OpenZWave
