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
#include "Msg.h"
#include "Node.h"
#include "Manager.h"
#include "Utils.h"
#include "platform/Log.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/Security.h"
#include "aes/aescpp.h"

using namespace OpenZWave;

uint8 Msg::s_nextCallbackId = 1;

#define DEBUG 1

//-----------------------------------------------------------------------------
// <Msg::Msg>
// Constructor
//-----------------------------------------------------------------------------
Msg::Msg
(
	string const& _logText,
	uint8 _targetNodeId,
	uint8 const _msgType,
	uint8 const _function,
	bool const _bCallbackRequired,
	bool const _bReplyRequired,			// = true
	uint8 const _expectedReply,			// = 0
	uint8 const _expectedCommandClassId	// = 0
):
	m_logText( _logText ),
	m_bFinal( false ),
	m_bCallbackRequired( _bCallbackRequired ),
	m_callbackId( 0 ),
	m_expectedReply( 0 ),
	m_expectedCommandClassId( _expectedCommandClassId ),
	m_length( 4 ),
	m_targetNodeId( _targetNodeId ),
	m_sendAttempts( 0 ),
	m_sendNonceAttempts ( 0 ),
	m_maxSendAttempts( MAX_TRIES ),
	m_instance( 1 ),
	m_endPoint( 0 ),
	m_flags( 0 ),
	m_encrypted ( false ),
	m_noncerecvd ( false ),
	m_homeId ( 0 )
{
	if( _bReplyRequired )
	{
		// Wait for this message before considering the transaction complete
		m_expectedReply = _expectedReply ? _expectedReply : _function;
	}

	m_buffer[0] = SOF;
	m_buffer[1] = 0;					// Length of the following data, filled in during Finalize.
	m_buffer[2] = _msgType;
	m_buffer[3] = _function;
}

//-----------------------------------------------------------------------------
// <Msg::SetInstance>
// Used to enable wrapping with MultiInstance/MultiChannel during finalize.
//-----------------------------------------------------------------------------
void Msg::SetInstance
(
	CommandClass* _cc,
	uint8 const _instance
)
{
	// Determine whether we should encapsulate the message in MultiInstance or MultiCommand
	if( Node* node = _cc->GetNodeUnsafe() )
	{
		MultiInstance* micc = static_cast<MultiInstance*>( node->GetCommandClass( MultiInstance::StaticGetCommandClassId() ) );
		m_instance = _instance;
		if( micc )
		{
			if( micc->GetVersion() > 1 )
			{
				m_endPoint = _cc->GetEndPoint( _instance );
				if( m_endPoint != 0 )
				{
					// Set the flag bit to indicate MultiChannel rather than MultiInstance
					m_flags |= m_MultiChannel;
					m_expectedCommandClassId = MultiInstance::StaticGetCommandClassId();
				}
			}
			else if( m_instance > 1 )
			{
				// Set the flag bit to indicate MultiInstance rather than MultiChannel
				m_flags |= m_MultiInstance;
				m_expectedCommandClassId = MultiInstance::StaticGetCommandClassId();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Msg::Append>
// Add a byte to the message
//-----------------------------------------------------------------------------
void Msg::Append
(
	uint8 const _data
)
{
	m_buffer[m_length++] = _data;
}

//-----------------------------------------------------------------------------
// <Msg::Finalize>
// Fill in the length and checksum values for the message
//-----------------------------------------------------------------------------
void Msg::Finalize()
{
	if( m_bFinal )
	{
		// Already finalized
		return;
	}

	// Deal with Multi-Channel/Instance encapsulation
	if( ( m_flags & ( m_MultiChannel | m_MultiInstance ) ) != 0 )
	{
		MultiEncap();
	}

	// Add the callback id
	if( m_bCallbackRequired )
	{
		// Set the length byte
		m_buffer[1] = m_length;		// Length of following data

		if( 0 == s_nextCallbackId )
		{
			s_nextCallbackId = 1;
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
	for( uint32 i=1; i<m_length; ++i )
	{
		checksum ^= m_buffer[i];
	}
	m_buffer[m_length++] = checksum;

	m_bFinal = true;
}


//-----------------------------------------------------------------------------
// <Msg::UpdateCallbackId>
// If this message has a callback ID, increment it and recalculate the checksum
//-----------------------------------------------------------------------------
void Msg::UpdateCallbackId()
{
	if( m_bCallbackRequired )
	{
		// update the callback ID
		m_buffer[m_length-2] = s_nextCallbackId;
		m_callbackId = s_nextCallbackId++;

		// Recalculate the checksum
		uint8 checksum = 0xff;
		for( int32 i=1; i<m_length-1; ++i )
		{
			checksum ^= m_buffer[i];
		}
		m_buffer[m_length-1] = checksum;
	}
}


//-----------------------------------------------------------------------------
// <Msg::GetAsString>
// Create a string containing the raw data
//-----------------------------------------------------------------------------
string Msg::GetAsString()
{
	string str = m_logText;

	char byteStr[16];
	if( m_targetNodeId != 0xff )
	{
		snprintf( byteStr, sizeof(byteStr), " (Node=%d)", m_targetNodeId );
		str += byteStr;
	}

	str += ": ";

	for( uint32 i=0; i<m_length; ++i )
	{
		if( i )
		{
			str += ", ";
		}

		snprintf( byteStr, sizeof(byteStr), "0x%.2x", m_buffer[i] );
		str += byteStr;
	}

	return str;
}

//-----------------------------------------------------------------------------
// <Msg::MultiEncap>
// Encapsulate the data inside a MultiInstance/Multicommand message
//-----------------------------------------------------------------------------
void Msg::MultiEncap
(
)
{
	char str[256];
	if( m_buffer[3]	!= FUNC_ID_ZW_SEND_DATA )
	{
		return;
	}

	// Insert the encap header
	if( ( m_flags & m_MultiChannel ) != 0 )
	{
		// MultiChannel
		for( uint32 i=m_length-1; i>=6; --i )
		{
			m_buffer[i+4] = m_buffer[i];
		}

		m_buffer[5] += 4;
		m_buffer[6] = MultiInstance::StaticGetCommandClassId();
		m_buffer[7] = MultiInstance::MultiChannelCmd_Encap;
		m_buffer[8] = 1;
		m_buffer[9] = m_endPoint;
		m_length += 4;

		snprintf( str, sizeof(str), "MultiChannel Encapsulated (instance=%d): %s", m_instance, m_logText.c_str() );
		m_logText = str;
	}
	else
	{
		// MultiInstance
		for( uint32 i=m_length-1; i>=6; --i )
		{
			m_buffer[i+3] = m_buffer[i];
		}

		m_buffer[5] += 3;
		m_buffer[6] = MultiInstance::StaticGetCommandClassId();
		m_buffer[7] = MultiInstance::MultiInstanceCmd_Encap;
		m_buffer[8] = m_instance;
		m_length += 3;

		snprintf( str, sizeof(str), "MultiInstance Encapsulated (instance=%d): %s", m_instance, m_logText.c_str() );
		m_logText = str;
	}
}

//-----------------------------------------------------------------------------
// <Node::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* Msg::GetDriver
(
)const
{
	return( Manager::Get()->GetDriver( m_homeId ) );
}


uint8* Msg::GetBuffer() {
	if (m_encrypted == false)
		return m_buffer;
#if 0
	m_nonce[0] = 0x09;
	m_nonce[1] = 0x0d;
	m_nonce[2] = 0x93;
	m_nonce[3] = 0xd3;
	m_nonce[4] = 0x61;
	m_nonce[5] = 0x61;
	m_nonce[6] = 0x1d;
	m_nonce[7] = 0xd6;
#endif
	uint8 len = 0;
	e_buffer[len++] = SOF;
	e_buffer[len++] = m_length + 18; // length of full packet
	e_buffer[len++] = REQUEST;
	e_buffer[len++] = FUNC_ID_ZW_SEND_DATA;
	e_buffer[len++] = m_targetNodeId;
	e_buffer[len++] = m_length + 11; 					// Length of the payload
	e_buffer[len++] = Security::StaticGetCommandClassId();
	e_buffer[len++] = SecurityCmd_MessageEncap;

	/* create our IV */
	uint8 initializationVector[16];
	/* the first 8 bytes of a outgoing IV are random
	 * and we add it also to the start of the payload
	 */
	for (int i = 0; i < 8; i++) {
		//initializationVector[i] = (rand()%0xFF)+1;
		initializationVector[i] = 0xAA;
		e_buffer[len++] = initializationVector[i];
	}
	/* the remaining 8 bytes are the NONCE we got from the device */
	for (int i = 0; i < 8; i++) {
		initializationVector[8+i] = m_nonce[i];
	}


	uint8 plaintextmsg[32];
	/* add the Sequence Flag
	 * - Since we dont currently handle multipacket encryption
	 * just set this to 0
	 */
	plaintextmsg[0] = 0;
	/* now add the actual message to be encrypted */
	for (int i = 0; i < m_length-6-3; i++)
		plaintextmsg[i+1] = m_buffer[6+i];

	/* now encrypt */
	uint8 encryptedpayload[30];
	aes_mode_reset(GetDriver()->GetEncKey());
#ifdef DEBUG
	PrintHex("Plain Text Packet:", plaintextmsg, m_length-5-3);
#endif
	if (aes_ofb_encrypt(plaintextmsg, encryptedpayload, m_length-5-3, initializationVector, GetDriver()->GetEncKey()) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, m_targetNodeId, "Failed to Encrypt Packet");
		m_length = 0;
		return NULL;
	}
#ifdef DEBUG
	PrintHex("Encrypted Packet", encryptedpayload, m_length-5-3);
#endif
	/* now add the Encrypted output to the packet */
	for (int i = 0; i < m_length-5-3; i++) {
		e_buffer[len++] = encryptedpayload[i];
	}

	// Append the nonce identifier :)
	e_buffer[len++] = m_nonce[0];


	/* regenerate the IV */
	for (int i = 0; i < 8; i++) {
		//initializationVector[i] = (rand()%0xFF)+1;
		initializationVector[i] = 0xAA;
	}
	/* the remaining 8 bytes are the NONCE we got from the device */
	for (int i = 0; i < 8; i++) {
		initializationVector[8+i] = m_nonce[i];
	}

	/* now calculate the MAC and append it */
	uint8 mac[8];
	GenerateAuthentication(&e_buffer[7], e_buffer[5], GetDriver()->GetControllerNodeId(), m_targetNodeId, initializationVector, mac);
	for(int i=0; i<8; ++i )
	{
		e_buffer[len++] = mac[i];
	}

	e_buffer[len++] = GetDriver()->GetTransmitOptions();
	/* this is the same as the Actual Message */
	e_buffer[len++] = m_buffer[m_length-2];
	// Calculate the checksum
	uint8 csum = 0xff;
	for( int32 i=1; i<len; ++i )
	{
		csum ^= e_buffer[i];
	}
	e_buffer[len++] = csum;
	return e_buffer;
}

//-----------------------------------------------------------------------------
// <Msg::GenerateAuthentication>
// Generate authentication data from a security-encrypted message
//-----------------------------------------------------------------------------
bool Msg::GenerateAuthentication
(
	uint8 const* _data,				// Starting from the command class command
	uint32 const _length,
	uint8 const _sendingNode,
	uint8 const _receivingNode,
	uint8 *iv,
	uint8* _authentication			// 8-byte buffer that will be filled with the authentication data
)
{
	// Build a buffer containing a 4-byte header and the encrypted
	// message data, padded with zeros to a 16-byte boundary.
	uint8 buffer[256];
	uint8 tmpauth[16];
	memset(buffer, 0, 256);
	memset(tmpauth, 0, 16);
	buffer[0] = _data[0];							// Security command class command
	buffer[1] = _sendingNode;
	buffer[2] = _receivingNode;
	buffer[3] = _length - 19; // Subtract 19 to account for the 9 security command class bytes that come before and after the encrypted data
	memcpy( &buffer[4], &_data[9], _length-19 );	// Encrypted message

	uint8 bufsize = _length - 19 + 4; /* the size of buffer */
#ifdef DEBUG
	PrintHex("Raw Auth (minus IV)", buffer, bufsize);
	Log::Write(LogLevel_Debug, m_targetNodeId, "Raw Auth (Minus IV) Size: %d (%d)", bufsize, bufsize+16);
#endif

	aes_mode_reset(GetDriver()->GetAuthKey());
	/* encrypt the IV with ecb */
	if (aes_ecb_encrypt(iv, tmpauth, 16, GetDriver()->GetAuthKey()) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, m_targetNodeId, "Failed Initial ECB Encrypt of Auth Packet");
		return false;
	}

	/* our temporary holding var */
	uint8 encpck[16];

	int block = 0;
	/* reset our encpck temp var */
	memset(encpck, 0, 16);
	/* now xor the buffer with our encrypted IV */
	for (int i = 0; i < bufsize; i++) {
		encpck[block] = buffer[i];
		block++;
		/* if we hit a blocksize, then encrypt */
		if (block == 16) {
			for (int j = 0; j < 16; j++) {
				/* here we do our xor */
				tmpauth[j] = encpck[j] ^ tmpauth[j];
				/* and reset encpck for good measure */
				encpck[j] = 0;
			}
			/* reset our block counter back to 0 */
			block = 0;
			aes_mode_reset(GetDriver()->GetAuthKey());
			if (aes_ecb_encrypt(tmpauth, tmpauth, 16, GetDriver()->GetAuthKey()) == EXIT_FAILURE) {
				Log::Write(LogLevel_Warning, m_targetNodeId, "Failed Subsequent (%d) ECB Encrypt of Auth Packet", i);
				return false;
			}
		}
	}
	/* any left over data that isn't a full block size*/
	if (block > 0) {
		for (int i= 0; i < 16; i++) {
			/* encpck from block to 16 is already gauranteed to be 0
			 * so its safe to xor it with out tmpmac */
			tmpauth[i] = encpck[i] ^ tmpauth[i];
		}
		aes_mode_reset(GetDriver()->GetAuthKey());
		if (aes_ecb_encrypt(tmpauth, tmpauth, 16, GetDriver()->GetAuthKey()) == EXIT_FAILURE) {
			Log::Write(LogLevel_Warning, m_targetNodeId, "Failed Final ECB Encrypt of Auth Packet");
			return false;
		}
	}
	/* we only care about the first 8 bytes of tmpauth as the mac */
#ifdef DEBUG
	PrintHex("Computed Auth", tmpauth, 8);
#endif
	/* so only copy 8 bytes to the _authentication var */
	memcpy(_authentication, tmpauth, 8);
	return true;
}

