//-----------------------------------------------------------------------------
//
//	ManufacturerSpecific.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_MANUFACTURER_SPECIFIC
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#ifndef _ManufacturerSpecific_H
#define _ManufacturerSpecific_H

#include <map>
#include "CommandClass.h"

namespace OpenZWave
{
	class ManufacturerSpecific: public CommandClass
	{
	public:
		static CommandClass* Create( uint8 const _nodeId ){ return new ManufacturerSpecific( _nodeId ); }
		virtual ~ManufacturerSpecific(){}

		static uint8 const StaticGetCommandClassId(){ return 0x72; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_MANUFACTURER_SPECIFIC"; }

		// From CommandClass
		virtual void SaveStatic( FILE* _file );
		virtual void RequestStatic();
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 0 );

	private:
		ManufacturerSpecific( uint8 const _nodeId ): CommandClass( _nodeId ){}
		bool LoadProductXML();

		class Product
		{
		public:
			Product
			( 
				uint16 _manufacturerId,
				uint16 _productType,
				uint16 _productId,
				string const& _productName
			):	m_manufacturerId( _manufacturerId ),
				m_productType( _productType ),
				m_productId( _productId ),
				m_productName( _productName )
			{
			}

			int64 GetKey()const
			{
				return( GetKey( m_manufacturerId, m_productType, m_productId ) );
			}

			static int64 GetKey( uint16 _manufacturerId, uint16 _productType, uint16 _productId )
			{
				int64 key = (((int64)_manufacturerId)<<16) | (((int64)_productType)<<8) | (int64)_productId;
				return key;
			}

			uint16 GetManufacturerId()const{ return m_manufacturerId; }
			uint16 GetProductType()const{ return m_productType; }
			uint16 GetProductId()const{ return m_productId; }
			string GetProductName()const{ return m_productName; }

		private:
			uint16	m_manufacturerId;
			uint16	m_productType;
			uint16	m_productId;
			string	m_productName;
		};

		static map<uint16,string>	s_manufacturerMap;
		static map<int64,Product*>	s_productMap;
		static bool					s_bXmlLoaded;
	};

} // namespace OpenZWave

#endif

