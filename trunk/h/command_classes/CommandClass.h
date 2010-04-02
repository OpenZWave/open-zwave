//-----------------------------------------------------------------------------
//
//	CommandClass.h
//
//	Base class for all Z-Wave Command Classes
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

#ifndef _CommandClass_H
#define _CommandClass_H

#include <string>
#include <vector>
#include "tinyxml.h"
#include "Defs.h"

namespace OpenZWave
{
	class Driver;
	class Msg;
	class Node;
	class Value;
	class ValueID;

	class CommandClass
	{
	public:
		CommandClass( uint32 const _homeId, uint8 const _nodeId ): m_homeId( _homeId ), m_nodeId( _nodeId ), m_version( 1 ), m_instances( 0 ), m_polledInstances( NULL ){}
		virtual ~CommandClass(){}

		virtual void ReadXML( TiXmlElement const* _ccElement );
		virtual void WriteXML( TiXmlElement* _ccElement );
		virtual void RequestStatic(){}							// For static node data
		virtual void RequestState( uint8 const _instance ){}	// For dynamic node data
		
		virtual uint8 const GetCommandClassId()const = 0;		
		virtual string const GetCommandClassName()const = 0;
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) = 0;
		virtual bool SetValue( Value const& _value ){ return false; }

		void Poll();
		void SetPolled( uint8 const _instance, bool const _state );
		bool RequiresPolling();

		uint8 GetVersion()const{ return m_version; }
		uint8 GetInstances()const{ return m_instances; }
		uint32 GetHomeId()const{ return m_homeId; }
		uint8 GetNodeId()const{ return m_nodeId; }
		Node* GetNode()const;
		Driver* GetDriver()const;

		void SetVersion( uint8 const _version ){ m_version = _version; }
		void SetInstances( uint8 const _instances );

		// Helper methods
		float32 ExtractValue( uint8 const* _data, uint8* _scale )const;
		string ExtractValueAsString( uint8 const* _data, uint8* _scale )const;
		void AppendValue( Msg* _msg, float32 const _value, uint8 const _precision, uint8 const _scale )const;
		uint8 const GetAppendValueSize( float32 const _value, uint8 const _precision )const;

	protected:
		virtual void CreateVars( uint8 const _instance ){}

	private:
		uint32	m_homeId;
		uint8	m_nodeId;
		uint8	m_version;
		uint8	m_instances;
		bool*	m_polledInstances;
	};

} // namespace OpenZWave

#endif



