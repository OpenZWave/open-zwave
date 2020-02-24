//-----------------------------------------------------------------------------
//
//	Msg.h
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

#ifndef _MsgNew_H
#define _MsgNew_H

#include <cstdio>
#include <string>
#include <string.h>
#include "Defs.h"
#include "Driver.h"
#include "command_classes/CommandClass.h"
#include "command_classes/MultiInstance.h"

namespace OpenZWave
{
	class Driver;

	namespace Internal
	{
		namespace CC
		{
			class CommandClass;
		}

		/** \brief Message object to be passed to and from devices on the Z-Wave network.
		 */
		class OPENZWAVE_EXPORT MsgBase
		{
			public:
				enum MessageFlags
				{
					m_MultiChannel = 0x01,		// Indicate MultiChannel encapsulation
					m_MultiInstance = 0x02,		// Indicate MultiInstance encapsulation
				};

				MsgBase(CC::CommandClass *cc, ZW_Message_Type const _msgType, ZW_Msg_Func const _function, bool const _bCallbackRequired, bool const _bReplyRequired = true, ZW_Callback_Func const _expectedReply = ZW_Callback_Func::None, uint8 const _expectedCommandClassId = 0);
				~MsgBase()
				{
				}

				void Finalize();
				void UpdateCallbackId();

				/**
				 * \brief Identifies the Node ID of the "target" node (if any) for this function.
				 * \return Node ID of the target.
				 */
				uint8 GetTargetNodeId() const
				{
					return m_targetNodeId;
				}

				/**
				 * \brief Identifies the Callback ID (if any) for this message.  Callback ID is a value (OpenZWave uses sequential IDs) that
				 * helps the application associate message responses with the original message request.
				 * \return Callback ID for this message.
				 */
				uint8 GetCallbackId() const
				{
					return m_callbackId;
				}

				/**
				 * \brief Identifies the expected reply type (if any) for this message. The expected reply is a function code...one
				 * of the FUNC_ID... values defined in Defs.h.  Many Z-Wave functions generate responses with the same function code
				 * (for example, a FUNC_ID_ZW_GET_VERSION message generates a FUNC_ID_ZW_GET_VERSION response.  But other functions
				 * generate a different response. FUNC_ID_ZW_SEND_DATA triggers several responses, but ultimately, a "Get" sent with
				 * this function should result in a FUNC_ID_APPLICATION_COMMAND_HANDLER response.
				 * \return Expected reply (function code) for this message.
				 */
				uint8 GetExpectedReply() const
				{
					return m_expectedReply;
				}

				/**
				 * \brief Identifies the expected Command Class ID (if any) for this message.
				 * \return Expected command class ID for this message.
				 */
				uint8 GetExpectedCommandClassId() const
				{
					return m_expectedCommandClassId;
				}

				/**
				 * \brief For messages that request a Report for a specified command class, identifies the expected Instance
				 * for the variable being obtained in the report.
				 * \return Expected Instance value for this message.
				 */
				uint8 GetExpectedInstance() const
				{
					return m_instance;
				}

				/**
				 * \brief get the LogText Associated with this message
				 * \return the LogText used during the constructor
				 */
				string GetLogText() const
				{
					return m_logText;
				}

				uint32 GetLength() const
				{
					return m_encrypted == true ? m_length + 20 + 6 : m_length;
				}
				uint8* GetBuffer();
				string GetAsString();

				uint8 GetSendAttempts() const
				{
					return m_sendAttempts;
				}
				void SetSendAttempts(uint8 _count)
				{
					m_sendAttempts = _count;
				}

				uint8 GetMaxSendAttempts() const
				{
					return m_maxSendAttempts;
				}
				void SetMaxSendAttempts(uint8 _count)
				{
					if (_count < MAX_MAX_TRIES)
						m_maxSendAttempts = _count;
				}

				bool IsWakeUpNoMoreInformationCommand()
				{
					return (m_bFinal && (m_length == 11) && (m_buffer[3] == 0x13) && (m_buffer[6] == 0x84) && (m_buffer[7] == 0x08));
				}
				bool IsNoOperation()
				{
					return (m_bFinal && (m_length == 11) && (m_buffer[3] == 0x13) && (m_buffer[6] == 0x00) && (m_buffer[7] == 0x00));
				}

				bool operator ==(MsgBase const& _other) const
				{
					if (m_bFinal && _other.m_bFinal)
					{
						// Do not include the callback Id or checksum in the comparison.
						uint8 length = m_length - (m_bCallbackRequired ? 2 : 1);
						return (!memcmp(m_buffer, _other.m_buffer, length));
					}

					return false;
				}
				uint8 GetSendingCommandClass()
				{
					if (m_buffer[3] == 0x13)
					{
						return m_buffer[6];
					}
					return 0;
				}
				bool isEncrypted()
				{
					return m_encrypted;
				}
				void setEncrypted()
				{
					m_encrypted = true;
				}
				bool isNonceRecieved()
				{
					return m_noncerecvd;
				}
				void setNonce(uint8 nonce[8])
				{
					memcpy(m_nonce, nonce, 8);
					m_noncerecvd = true;
					UpdateCallbackId();
				}
				void clearNonce()
				{
					memset((m_nonce), '\0', 8);
					m_noncerecvd = false;
				}
				/** Returns a pointer to the driver (interface with a Z-Wave controller)
				 *  associated with this node.
				 */
				Driver* GetDriver() const;
			protected:

				void MultiEncap();						// Encapsulate the data inside a MultiInstance/Multicommand message
				string m_logText;
				bool m_bFinal;
				bool m_bCallbackRequired;

				uint8 m_callbackId;
				uint8 m_expectedReply;
				uint8 m_expectedCommandClassId;
				uint8 m_length;
				uint8 m_buffer[256];
				uint8 e_buffer[256];
				uint8 m_databuffer[256];

				uint8 m_targetNodeId;
				uint8 m_sendAttempts;
				uint8 m_maxSendAttempts;

				uint8 m_instance;
				uint8 m_endPoint;				// Endpoint to use if the message must be wrapped in a multiInstance or multiChannel command class
				uint8 m_flags;
				uint8 m_transmitoptions;

				bool m_encrypted;
				bool m_noncerecvd;
				uint8 m_nonce[8];
				static uint8 s_nextCallbackId;		// counter to get a unique callback id
				ZW_Message_Type m_msgType;
				ZW_Msg_Func m_msgFunc;
				bool m_isBridgeController;
				CC::CommandClass *m_cc;

		};

		/** \brief Message object to be passed to and from devices on the Z-Wave network.
		 */
		template <class t>
		class OPENZWAVE_EXPORT MsgNew : public MsgBase
		{
			public:
				MsgNew(CC::CommandClass *cc, ZW_Message_Type const _msgType, ZW_Msg_Func const _function, bool const _bCallbackRequired, bool const _bReplyRequired = true, ZW_Callback_Func const _expectedReply = ZW_Callback_Func::None, uint8 const _expectedCommandClassId = 0) :
					MsgBase::MsgBase(cc, _msgType, _function, _bCallbackRequired, _bReplyRequired, _expectedReply, _expectedCommandClassId),
					m_messageCommand(t::Invalid),
					m_CommandClass(ZW_CommandClasses::Invalid),
					m_dataLength(0)
				{
					m_isBridgeController = GetDriver()->IsBridgeController();
					m_transmitoptions = GetDriver()->GetTransmitOptions();
					//std::cout << this->GetAsString() << std::endl;
				}

				~MsgNew()
				{
				}
				void setTargetNode(uint8 node) {
					m_targetNodeId = node;
				}
				void setCommandClass(ZW_CommandClasses cc) {
					m_CommandClass = cc;
				}
				void setCommandClassCmd(t msgCmd) {
					m_messageCommand = msgCmd;
					m_logText = msgCmd._to_full_string();
				}
				void setTransmitOptions(uint8 options) {
					m_transmitoptions = options;
				}
				void send(OpenZWave::Driver::MsgQueue _queue) {
					this->Finish();
					std::cout << GetAsString() << std::endl;
					//GetDriver()->SendMsg(this, _queue);
				}
				void Finish() {
					m_buffer[m_length++] = SOF;
					m_buffer[m_length++] = 0;
					m_buffer[m_length++] = m_msgType;
					m_buffer[m_length++] = translateMsgFunc(m_msgFunc);
					m_buffer[m_length++] = m_targetNodeId;
					m_buffer[m_length++] = m_dataLength +2; /* add m_CommandClass and m_messageCommand */
					m_buffer[m_length++] = (uint8_t)(m_CommandClass & 0xFF);
					m_buffer[m_length++] = m_messageCommand;
					for (int j = 0; j < m_dataLength; ++j) {
						std::cout << "adding" << std::endl;
						m_buffer[m_length++] = m_databuffer[j];
					}
					m_buffer[m_length++] = m_transmitoptions;
								// Add the callback id
					if (m_bCallbackRequired)
					{
						if (0 == s_nextCallbackId)
						{
							s_nextCallbackId = 10;
						}
						m_buffer[m_length++] = s_nextCallbackId;
						m_callbackId = s_nextCallbackId++;
					}
					/* fix up Length Byte */
					m_buffer[1] = m_length -1;

					// Calculate the checksum
					uint8 checksum = 0xff;
					for (uint32 i = 1; i < m_length; ++i)
					{
						checksum ^= m_buffer[i];
					}
					m_buffer[m_length++] = checksum;

					std::cout << std::hex << (int)(m_CommandClass & 0xFF) << std::endl;
					std::cout << m_CommandClass._to_string() << std::endl;
					std::cout << std::hex << (int)m_messageCommand << std::endl;
					std::cout << std::hex << (int)m_length << std::endl;
					std::cout << std::hex << (int)m_dataLength << std::endl;
					std::cout << std::hex << (int)m_transmitoptions << std::endl;
					std::cout << std::hex << (int)m_callbackId << std::endl;
				}
				//-----------------------------------------------------------------------------
				// <MsgBase::SetInstance>
				// Used to enable wrapping with MultiInstance/MultiChannel during finalize.
				//-----------------------------------------------------------------------------
				void SetInstance(OpenZWave::Internal::CC::CommandClass * _cc, uint8 const _instance) 
				{
					// Determine whether we should encapsulate the message in MultiInstance or MultiCommand
					if (Node* node = _cc->GetNodeUnsafe())
					{
						Internal::CC::MultiInstance* micc = static_cast<Internal::CC::MultiInstance*>(node->GetCommandClass(Internal::CC::MultiInstance::StaticGetCommandClassId()));
						m_instance = _instance;
						if (micc)
						{
							if (micc->GetVersion() > 1)
							{
								m_endPoint = _cc->GetEndPoint(_instance);
								if (m_endPoint != 0)
								{
									// Set the flag bit to indicate MultiChannel rather than MultiInstance
									m_flags |= m_MultiChannel;
									m_expectedCommandClassId = Internal::CC::MultiInstance::StaticGetCommandClassId();
								}
							}
							else if (m_instance > 1)
							{
								// Set the flag bit to indicate MultiInstance rather than MultiChannel
								m_flags |= m_MultiInstance;
								m_expectedCommandClassId = Internal::CC::MultiInstance::StaticGetCommandClassId();
							}
						}
					}
				}
				//-----------------------------------------------------------------------------
				// <MsgBase::Append>
				// Add a byte to the message
				//-----------------------------------------------------------------------------
				void Append(uint8 const _data) 
				{
					m_databuffer[m_dataLength++] = _data;
				}
				//-----------------------------------------------------------------------------
				// <MsgBase::AppendArray>
				// Add a byte array to the message
				//-----------------------------------------------------------------------------
				void AppendArray(const uint8* const _data, const uint8 _length) 
				{
					for (uint8 i = 0; i < _length; i++)
					{
						this->Append(_data[i]);
					}
				}

			private:
				uint8_t translateMsgFunc(ZW_Msg_Func _msgFunc) {
					m_isBridgeController = false;
					switch (_msgFunc) {
						case ZW_Msg_Func::Send_Data: {
							if (m_isBridgeController)
								return FUNC_ID_ZW_SEND_DATA_BRIDGE;
							else 
								return FUNC_ID_ZW_SEND_DATA;
							break;
						}
						case ZW_Msg_Func::Invalid: {
							Log::Write(LogLevel_Warning, "Invalid ZW_Msg_Function Set");
							return 0;
							break;
						}
					}
				}

				t m_messageCommand;
				ZW_CommandClasses m_CommandClass;
				uint8 m_dataLength;
				
		};


		void test();
		void test2(MsgBase *base);
	} // namespace Internal
} // namespace OpenZWave

#endif //_Msg_H

