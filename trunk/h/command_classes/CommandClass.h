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
		enum
		{
			RequestFlag_Static	= 0x00000001,	// Values that never change
			RequestFlag_Session = 0x00000002,	// Values that change infrequently, and so only need to be requested at start up, or via a manual refresh.
			RequestFlag_Dynamic	= 0x00000004	// Values that change and will be requested if polling is enabled on the node.
		};

		CommandClass( uint32 const _homeId, uint8 const _nodeId ): m_homeId( _homeId ), m_nodeId( _nodeId ), m_version( 1 ), m_instances( 0 ){}
		virtual ~CommandClass(){}

		virtual void ReadXML( TiXmlElement const* _ccElement );
		virtual void WriteXML( TiXmlElement* _ccElement );
		virtual void RequestState( uint32 const _requestFlags ){}
		
		virtual uint8 const GetCommandClassId()const = 0;		
		virtual string const GetCommandClassName()const = 0;
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) = 0;
		virtual bool SetValue( Value const& _value ){ return false; }

		uint8 GetVersion()const{ return m_version; }
		uint8 GetInstances()const{ return m_instances; }
		uint32 GetHomeId()const{ return m_homeId; }
		uint8 GetNodeId()const{ return m_nodeId; }
		Driver* GetDriver()const;
		Node* GetNode()const;
		void ReleaseNode()const;

		void SetVersion( uint8 const _version ){ m_version = _version; }
		void SetInstances( uint8 const _instances );

		// Helper methods
		float32 ExtractValue( uint8 const* _data, uint8* _scale )const;
		string ExtractValueAsString( uint8 const* _data, uint8* _scale )const;
		void AppendValue( Msg* _msg, float32 const _value, uint8 const _precision, uint8 const _scale )const;
		uint8 const GetAppendValueSize( float32 const _value, uint8 const _precision )const;

	protected:
		template <class T>
		class ValueInstances
		{
		public:
			ValueInstances(): m_instances(NULL), m_numInstances(0){}
			~ValueInstances()
			{
				for( int i=0; i<m_numInstances; ++i )
				{
					if( m_instances[i] )
					{
						T* instance = static_cast<T*>( m_instances[i] );
						instance->Release();
					}
				}

				delete [] m_instances;
			}

			void AddInstance( uint8 _idx, T* _instance )
			{
				if( _idx > m_numInstances )
				{
					Grow( _idx );
				}

				if( m_instances[_idx-1] )
				{
					m_instances[_idx-1]->Release();
				}
				m_instances[_idx-1] = _instance;
			}

			bool HasInstances()const{ return( m_numInstances != 0 ); }

			T* GetInstance( uint8 _idx )
			{ 
				return( ( _idx > m_numInstances ) ? NULL : m_instances[_idx-1] ); 
			}

		private:
			void Grow( uint8 _numInstances )
			{
				if( _numInstances > m_numInstances )
				{
					// Realloc the array
					T** newInstances = new T*[_numInstances];
					memcpy( newInstances, m_instances, sizeof(T*) * m_numInstances );
					memset( &newInstances[m_numInstances], 0, sizeof(T*) * (_numInstances-m_numInstances) );
					delete [] m_instances;
					m_instances = newInstances;
					m_numInstances = _numInstances;
				}
			}

			T**	m_instances;
			int m_numInstances;
		};

		virtual void CreateVars( uint8 const _instance ){}

	private:
		uint32	m_homeId;
		uint8	m_nodeId;
		uint8	m_version;
		uint8	m_instances;
	};

} // namespace OpenZWave

#endif



