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
#include "Bitfield.h"

namespace OpenZWave
{
	class Driver;
	class Msg;
	class Node;
	class Value;

	/** \brief Base class for all Z-Wave command classes.
	 */
	class CommandClass
	{
	public:
		enum
		{
			RequestFlag_Static		= 0x00000001,	/**< Values that never change. */
			RequestFlag_Session		= 0x00000002,	/**< Values that change infrequently, and so only need to be requested at start up, or via a manual refresh. */
			RequestFlag_Dynamic		= 0x00000004,	/**< Values that change and will be requested if polling is enabled on the node. */
			RequestFlag_LowPriority	= 0x00000008	/**< Indictates that the request should be made with low priority messages */
		};

		CommandClass( uint32 const _homeId, uint8 const _nodeId );
		virtual ~CommandClass(){}

		virtual void ReadXML( TiXmlElement const* _ccElement );
		virtual void WriteXML( TiXmlElement* _ccElement );
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance ){ return false; }
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance ) { return false; }

		virtual uint8 const GetCommandClassId()const = 0;
		virtual string const GetCommandClassName()const = 0;
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) = 0;
		virtual bool SetValue( Value const& _value ){ return false; }
		virtual void SetVersion( uint8 const _version ){ m_version = _version; }

		bool RequestStateForAllInstances( uint32 const _requestFlags );

		// The highest version number of the command class implemented by OpenZWave.  We only need
		// to do version gets on command classes that override this with a number greater than one.
		virtual uint8 GetMaxVersion(){ return 1; }

		uint8 GetVersion()const{ return m_version; }
		Bitfield const* GetInstances()const{ return &m_instances; }
		uint32 GetHomeId()const{ return m_homeId; }
		uint8 GetNodeId()const{ return m_nodeId; }
		Driver* GetDriver()const;
		Node* GetNodeUnsafe()const;
		Value* GetValue( uint8 const _instance, uint8 const _index );

		void SetInstances( uint8 const _instances );
		void SetInstance( uint8 const _endPoint );
		void SetAfterMark(){ m_afterMark = true; }
		bool IsAfterMark()const{ return m_afterMark; }
		bool IsCreateVars()const{ return m_createVars; }

		// Helper methods
		string ExtractValue( uint8 const* _data, uint8* _scale, uint8* _precision, uint8 _valueOffset = 1 )const;

		/**
		 *  Append a floating-point value to a message.
		 *  \param _msg The message to which the value should be appended.
		 *  \param _value A string containing a decimal number to be appended.
		 *  \param _scale A byte indicating the scale corresponding to this value (e.g., 1=F and 0=C for temperatures).
		 *  \see Msg
		 */
		void AppendValue( Msg* _msg, string const& _value, uint8 const _scale )const;
		uint8 const GetAppendValueSize( string const& _value )const;
		int32 ValueToInteger( string const& _value, uint8* o_precision, uint8* o_size )const;

	protected:
		virtual void CreateVars( uint8 const _instance ){}

	public:
		virtual void CreateVars( uint8 const _instance, uint8 const _index ){}


	private:
		uint32		m_homeId;
		uint8		m_nodeId;
		uint8		m_version;
		Bitfield	m_instances;
		bool		m_afterMark;		// Set to true if the command class is listed after COMMAND_CLASS_MARK, and should not create any values.
		bool		m_createVars;		// Do we want to create variables

	//-----------------------------------------------------------------------------
	// Record which items of static data have been read from the device
	//-----------------------------------------------------------------------------
	public:
		enum StaticRequest
		{
			StaticRequest_Instances		= 0x01,
			StaticRequest_Values		= 0x02,
			StaticRequest_Version		= 0x04
		};

		bool HasStaticRequest( uint8 _request )const{ return( (m_staticRequests & _request) != 0 ); }
		void SetStaticRequest( uint8 _request ){ m_staticRequests |= _request; }
		void ClearStaticRequest( uint8 _request );

	private:
		uint8   m_staticRequests;
	};

} // namespace OpenZWave

#endif



