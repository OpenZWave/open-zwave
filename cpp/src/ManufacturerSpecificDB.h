//-----------------------------------------------------------------------------
//
//	ManufacturerSpecificDB.h
//
//	Interface for Handling Device Configuration Files.
//
//	Copyright (c) 2016 Justin Hammond <justin@dynam.ac>
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


#ifndef _ManufacturerSpecificDB_H
#define _ManufacturerSpecificDB_H

#include <string>
#include <map>
#include <list>

#include "Defs.h"

namespace OpenZWave
{

	/** \brief The _ManufacturerSpecificDB class handles the Config File Database
	 * that we use to configure devices.
	 */
	class OPENZWAVE_EXPORT ManufacturerSpecificDB {
	public:
		static ManufacturerSpecificDB *Create();
		static ManufacturerSpecificDB *Get() { return s_instance; }
		static void Destroy();

		bool LoadProductXML();
		void UnloadProductXML();
		uint8 GetRevision() { return m_revision;}

	private:
		ManufacturerSpecificDB();
		~ManufacturerSpecificDB();

		static ManufacturerSpecificDB *s_instance;

		class Product
				{
				public:
					Product
					(
						uint16 _manufacturerId,
						uint16 _productType,
						uint16 _productId,
						string const& _productName,
						string const& _configPath
					):
						m_manufacturerId( _manufacturerId ),
						m_productType( _productType ),
						m_productId( _productId ),
						m_productName( _productName ),
						m_configPath( _configPath )
					{
					}

					int64 GetKey()const
					{
						return( GetKey( m_manufacturerId, m_productType, m_productId ) );
					}

					static int64 GetKey( uint16 _manufacturerId, uint16 _productType, uint16 _productId )
					{
						int64 key = (((int64)_manufacturerId)<<32) | (((int64)_productType)<<16) | (int64)_productId;
						return key;
					}

					uint16 GetManufacturerId()const{ return m_manufacturerId; }
					uint16 GetProductType()const{ return m_productType; }
					uint16 GetProductId()const{ return m_productId; }
					string GetProductName()const{ return m_productName; }
					string GetConfigPath()const{ return m_configPath; }

				private:
					uint16	m_manufacturerId;
					uint16	m_productType;
					uint16	m_productId;
					string	m_productName;
					string	m_configPath;
				};

		static map<uint16,string>	s_manufacturerMap;
		static map<int64,Product*>	s_productMap;
		static bool					s_bXmlLoaded;

		uint8 m_revision;

	};


}


#endif
