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

using namespace OpenZWave;

uint8 Msg::s_nextCallbackId = 1;


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
	m_sendAttempts( 0 )
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
