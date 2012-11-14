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

#ifndef _Msg_H
#define _Msg_H

#include <cstdio>
#include <string>
#include <string.h>
#include "Defs.h"

namespace OpenZWave
{
	class CommandClass;

	/** \brief Message object to be passed to and from devices on the Z-Wave network.
	 */
	class Msg
	{
	public:
		Msg( string const& _logtext, uint8 _targetNodeId, uint8 const _msgType, uint8 const _function, bool const _bCallbackRequired, bool const _bReplyRequired = true, uint8 const _expectedReply = 0, uint8 const _expectedCommandClassId = 0 );
		~Msg(){}

		void SetInstance( CommandClass* _cc, uint8 const _instance );	// Used to enable wrapping with MultiInstance/MultiChannel during finalize.

		void Append( uint8 const _data );
		void Finalize();

		uint8 GetTargetNodeId()const{ return m_targetNodeId; }
		uint8 GetCallbackId()const{ return m_callbackId; }
		uint8 GetExpectedReply()const{ return m_expectedReply; }
		uint8 GetExpectedCommandClassId()const{ return m_expectedCommandClassId; }
		uint32 GetLength()const{ return m_length; }
		uint8* GetBuffer(){ return m_buffer; }
		string GetAsString();

		uint8 GetSendAttempts()const{ return m_sendAttempts; }
		void SetSendAttempts( uint8 _count ){ m_sendAttempts = _count; }

		uint8 GetMaxSendAttempts()const{ return m_maxSendAttempts; }
		void SetMaxSendAttempts( uint8 _count ){ if( _count < MAX_MAX_TRIES ) m_maxSendAttempts = _count; }

		bool IsWakeUpNoMoreInformationCommand()
		{
			return( m_bFinal && (m_length==11) && (m_buffer[3]==0x13) && (m_buffer[6]==0x84) && (m_buffer[7]==0x08) );
		}

		bool operator == ( Msg const& _other )const
		{ 
			if( m_bFinal && _other.m_bFinal )
			{
				// Do not include the callback Id or checksum in the comparison.
				uint8 length = m_length - (m_bCallbackRequired ? 2: 1 );
				return( !memcmp( m_buffer, _other.m_buffer, length ) ); 
			}

			return false;
		}
		static uint8 GetLastCallbackId(){ return (s_nextCallbackId > 1? s_nextCallbackId -1 : 255);}

	private:
		void Encap();						// Encapsulate the data inside a MultiInstance/Multicommand message

		string			m_logText;
		bool			m_bFinal;
		bool			m_bCallbackRequired;

		uint8			m_callbackId;
		uint8			m_expectedReply;
		uint8			m_expectedCommandClassId;
		uint8			m_length;
		uint8			m_buffer[256];
		
		uint8			m_targetNodeId;
		uint8			m_sendAttempts;
		uint8			m_maxSendAttempts;

		uint8			m_instance;			// If this value is not one, the message must be wrapped in a multiInstance or multiChannel command class

		static uint8	s_nextCallbackId;	// counter to get a unique callback id
	};

} // namespace OpenZWave

#endif //_Msg_H

