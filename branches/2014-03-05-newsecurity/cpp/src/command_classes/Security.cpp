//-----------------------------------------------------------------------------
//
//	Security.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_Security
//
//	Copyright (c) 2011 Mal Lansell <openzwave@lansell.org>
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

#include <ctime>
#include "CommandClasses.h"
#include "Security.h"
#include "Association.h"
#include "Defs.h"
#include "AES.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "Notification.h"
#include "Log.h"

#include "ValueByte.h"

using namespace OpenZWave;


#define UNUSED(x) (void)(x)




enum SecurityCmd
{
	SecurityCmd_SupportedGet			= 0x02,
	SecurityCmd_SupportedReport			= 0x03,
	SecurityCmd_SchemeGet				= 0x04,
	SecurityCmd_SchemeReport			= 0x05,
	SecurityCmd_NetworkKeySet			= 0x06,
	SecurityCmd_NetworkKeyVerify		= 0x07,
	SecurityCmd_SchemeInherit			= 0x08,
	SecurityCmd_NonceGet				= 0x40,
	SecurityCmd_NonceReport				= 0x80,
	SecurityCmd_MessageEncap			= 0x81,
	SecurityCmd_MessageEncapNonceGet	= 0xc1
};

enum
{
	SecurityScheme_Zero					= 0x01,
	SecurityScheme_Reserved1			= 0x02,
	SecurityScheme_Reserved2			= 0x04,
	SecurityScheme_Reserved3			= 0x08,
	SecurityScheme_Reserved4			= 0x10,
	SecurityScheme_Reserved5			= 0x20,
	SecurityScheme_Reserved6			= 0x40,
	SecurityScheme_Reserved7			= 0x80
};


//-----------------------------------------------------------------------------
// <Security::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
bool Security::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Static )
	{
		Msg* msg = new Msg( "SecurityCmd_SchemeGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SecurityCmd_SchemeGet );
		msg->Append( (uint8)SecurityScheme_Zero );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Security);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Security::RequestValue>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
bool Security::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _index,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	return true;
}

//-----------------------------------------------------------------------------
// <Security::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Security::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	switch( (SecurityCmd)_data[0] )
	{
		case SecurityCmd_SupportedReport:
		{
			Log::Write(LogLevel_Info, "Received SecurityCmd_SupportedReport from node %d", GetNodeId() );
			break;
		}
		case SecurityCmd_SchemeReport:
		{
			Log::Write(LogLevel_Info, "Received SecurityCmd_SchemeReport from node %d", GetNodeId() );
			uint8 schemes = _data[1];
			if( schemes & SecurityScheme_Zero )
			{
				// We're good to go.  We now wait for a key to be sent by the device.
				Log::Write(LogLevel_Info, "    Security scheme agreed.  Key expected from device." );
			}
			else
			{
				// No common security scheme.  The device will
				// continue as an unsecured node.
				Log::Write(LogLevel_Warning,  "    No common security scheme.  The device will continue as an unsecured node." );
				
				// TBD - turn off security support for this node
			}
			break;
		}
		case SecurityCmd_NetworkKeySet:
		{
			Log::Write(LogLevel_Info,  "Received SecurityCmd_NetworkKeySet from node %d", GetNodeId() );
			break;
		}
		case SecurityCmd_NetworkKeyVerify:
		{
			Log::Write(LogLevel_Info,  "Received SecurityCmd_NetworkKeyVerify from node %d", GetNodeId() );
			break;
		}
		case SecurityCmd_SchemeInherit:
		{
			Log::Write(LogLevel_Info,  "Received SecurityCmd_SchemeInherit from node %d", GetNodeId() );
			break;
		}
		case SecurityCmd_NonceGet:
		{
			Log::Write(LogLevel_Info,  "Received SecurityCmd_NonceGet from node %d", GetNodeId() );
			SendNonceReport();
			break;
		}
		case SecurityCmd_NonceReport:
		{
			Log::Write(LogLevel_Info,  "Received SecurityCmd_NonceReport from node %d", GetNodeId() );
			EncryptMessage( &_data[1] );
			break;
		}
		case SecurityCmd_MessageEncap:
		{
			Log::Write(LogLevel_Info,  "Received SecurityCmd_MessageEncap from node %d", GetNodeId() );
			DecryptMessage( _data, _length );
			break;
		}
		case SecurityCmd_MessageEncapNonceGet:
		{
			Log::Write(LogLevel_Info,  "Received SecurityCmd_MessageEncapNonceGet from node %d", GetNodeId() );
			if( DecryptMessage( _data, _length ) )
			{
				SendNonceReport();
			}
			break;
		}
		default:
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Security::SendMsg>
// Queue a message to be securely sent by the Z-Wave PC Interface
//-----------------------------------------------------------------------------
void Security::SendMsg
( 
	Msg* _msg
)
{
	_msg->Finalize();

	uint8* buffer = _msg->GetBuffer();
	if( _msg->GetLength() < 7 )
	{
		// Message too short
		assert(0);
		return;
	}

	if( buffer[3] != FUNC_ID_ZW_SEND_DATA )
	{
		// Invalid message type
		assert(0);
		return;
	}

	uint8 length = buffer[5];
	if( length > 28 )
	{
		// Message must be split into two parts
		struct SecurityPayload payload1;
		payload1.m_length = 28;
		payload1.m_part = 1;
		memcpy( payload1.m_data, &buffer[6], payload1.m_length );
		QueuePayload( payload1 );

		struct SecurityPayload payload2;
		payload2.m_length = length-28;
		payload2.m_part = 2;
		memcpy( payload2.m_data, &buffer[34], payload2.m_length );
		QueuePayload( payload2 );
	}
	else
	{
		// The entire message can be encapsulated as one
		struct SecurityPayload payload;
		payload.m_length = length;
		payload.m_part = 0;				// Zero means not split into separate messages
		memcpy( payload.m_data, &buffer[6], payload.m_length );
		QueuePayload( payload );
	}
}

//-----------------------------------------------------------------------------
// <Security::QueuePayload>
// Queue data to be encapsulated by the Security Command Class, on
// receipt of a nonce value from the remote node.
//-----------------------------------------------------------------------------
void Security::QueuePayload
(
	SecurityPayload const& _payload
)
{
	m_queueMutex->Lock();
	m_queue.push_back( _payload );

	if( !m_waitingForNonce )
	{
		// Request a nonce from the node.  Its arrival
		// will trigger the sending of the first payload
		RequestNonce();
	}

	m_queueMutex->Release();
}


//-----------------------------------------------------------------------------
// <Security::EncryptMessage>
// Encrypt and send a Z-Wave message securely.
//-----------------------------------------------------------------------------
bool Security::EncryptMessage
(
	uint8 const* _nonce
)
{
	if( m_nonceTimer.GetMilliseconds() > 10000 )
	{
		// The nonce was  not received within 10 seconds
		// of us sending the nonce request.  Send it again
		RequestNonce();
		return false;
	}

	// Fetch the next payload from the queue and encapsulate it
	m_queueMutex->Lock();
	if( m_queue.empty() )
	{
		// Nothing to do
		m_queueMutex->Release();
		return false;
	}

	struct SecurityPayload const& payload = m_queue.front();
	uint32 queueSize = m_queue.size();
	m_queueMutex->Release();

	// Encapsulate the message fragment
	Msg* msg = new Msg( "Security Encapsulated message fragment", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( payload.m_length + 20 );
	msg->Append( GetCommandClassId() );
	msg->Append( (queueSize>1) ? SecurityCmd_MessageEncapNonceGet : SecurityCmd_MessageEncap );
	
	// Append the initialization vector
	uint32 i;
	for( i=0; i<8; ++i )
	{
		msg->Append( m_initializationVector[i] );
	}

	// Append the sequence data
	uint8 sequence;
	if( payload.m_part == 0 )
	{
		sequence = 0;
	}
	else if( payload.m_part == 1 )
	{
		sequence = (++m_sequenceCounter) & 0x0f;
		sequence |= 0x10;		// Sequenced, first frame
	}
	if( payload.m_part == 2 )
	{
		sequence = m_sequenceCounter & 0x0f;
		sequence |= 0x30;		// Sequenced, second frame
	}

	msg->Append( sequence );

	// Append the message payload
	for( i=0; i<payload.m_length; ++i )
	{
		msg->Append( payload.m_data[i] );
	}

	// Append the nonce identifier

	// Append space for the authentication data
	for( i=0; i<8; ++i )
	{
		msg->Append( 0 );
	}







	// Encrypt the encapsulated message fragment






	return true;
}

//-----------------------------------------------------------------------------
// <Security::DecryptMessage>
// Decrypt a security-encapsulated message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Security::DecryptMessage
(
	uint8 const* _data,
	uint32 const _length
)
{
	if( m_nonceTimer.GetMilliseconds() > 10000 )
	{
		// The message was not received within 10 seconds
		// of us sending the nonce report.
		
		// TBD - clear any partial message that has been stored.

		return false;
	}

	uint8 const* pPrivateNonce = &_data[1];				// 8 bytes in length
	bool secondFrame = ((_data[9] & 0x20) != 0);
	bool sequenced = ((_data[9] & 0x10) != 0);
	uint8 sequenceCount = _data[9] & 0x0f;
	uint8 nonceId = _data[_length-10];
	uint8 const* pAuthentication = &_data[_length-9];		// 8 bytes in length

	UNUSED(pPrivateNonce);
	UNUSED(secondFrame);
	UNUSED(sequenced);
	UNUSED(sequenceCount);
	UNUSED(nonceId);
	UNUSED(pAuthentication);
	

	return true;
}

//-----------------------------------------------------------------------------
// <Security::GenerateAuthentication>
// Generate authentication data from a security-encrypted message
//-----------------------------------------------------------------------------
void Security::GenerateAuthentication
(
	uint8 const* _data,				// Starting from the command class command
	uint32 const _length,		
	uint8 const _sendingNode,
	uint8 const _receivingNode,
	uint8* _authentication			// 8-byte buffer that will be filled with the authentication data
)
{
	// Build a buffer containing a 4-byte header and the encrypted
	// message data, padded with zeros to a 16-byte boundary.
	char buffer[256];
	
	buffer[0] = _data[0];							// Security command class command
	buffer[1] = _sendingNode;
	buffer[2] = _receivingNode;
	buffer[3] = _length - 18;						// Subtract 18 to account for the 9 security command class bytes that come before and after the encrypted data
	memcpy( &buffer[4], &_data[9], _length-18 );	// Encrypted message
	
	// End of data = (_length-18) + 4.
	uint32 i = _length-14;							

	// Pad data with zeros to the next 16-byte boundary
	while( i & 0x0f )								
	{
		buffer[i] = 0;
		i++;
	}

	uint32 numBlocks = i/16;

	// Create the initial authentication value from the initialization vector
	AES aes;
	aes.EncryptBlock( (const char*)m_initializationVector, (char *)_authentication );

	// Combine with the header and message data
	for( i=0; i<numBlocks; ++i )
	{
		// Xor the authentication with the block data
		uint32 start = i<<4;
		for( uint32 j=0; j<16; ++j )
		{
			_authentication[j] ^= buffer[start+j];
		}

		// Encrypt the result
		aes.EncryptBlock( (const char *)_authentication, (char *)_authentication );
	}
}

//-----------------------------------------------------------------------------
// <Security::RequestNonce>
// Request a nonce from the node
//-----------------------------------------------------------------------------
void Security::RequestNonce
(
)
{
	m_waitingForNonce = true;

	Msg* msg = new Msg( "SecurityCmd_NonceGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SecurityCmd_NonceGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Security);

	// Reset the nonce timer.  The nonce report
	// must be received within 10 seconds.
	m_nonceTimer.Reset();
}

//-----------------------------------------------------------------------------
// <Security::SendNonceReport>
// Send a nonce to the node
//-----------------------------------------------------------------------------
void Security::SendNonceReport
(
)
{
	uint8 publicNonce[8];

	// TBD - fill public nonce with next value

	Msg* msg = new Msg( "SecurityCmd_NonceReport", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 10 );
	msg->Append( GetCommandClassId() );
	msg->Append( SecurityCmd_NonceReport );
	for( int i=0; i<8; ++i )
	{
		msg->Append( publicNonce[i] );
	}
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Security);

	// Reset the nonce timer.  The encapsulated message
	// must be received within 10 seconds.
	m_nonceTimer.Reset();
}
