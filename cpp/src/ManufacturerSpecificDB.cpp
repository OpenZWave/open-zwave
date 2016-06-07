//-----------------------------------------------------------------------------
//
//	ManufacturerSpecificDB.cpp
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


#include "ManufacturerSpecificDB.h"
#include "tinyxml.h"

#include "Options.h"
#include "platform/Log.h"
#include "platform/FileOps.h"

using namespace OpenZWave;

ManufacturerSpecificDB *ManufacturerSpecificDB::s_instance = NULL;
map<uint16,string> ManufacturerSpecificDB::s_manufacturerMap;
map<int64,ManufacturerSpecificDB::Product*> ManufacturerSpecificDB::s_productMap;
bool ManufacturerSpecificDB::s_bXmlLoaded = false;

ManufacturerSpecificDB *ManufacturerSpecificDB::Create
(
)
{

	if( NULL == s_instance )
	{
		s_instance = new ManufacturerSpecificDB();
	}
	return s_instance;

}


void ManufacturerSpecificDB::Destroy
(
)
{
	delete s_instance;
	s_instance = NULL;
}

ManufacturerSpecificDB::ManufacturerSpecificDB
(
)
{
	// Ensure the singleton instance is set
	s_instance = this;

	if (!s_bXmlLoaded) LoadProductXML();


}

ManufacturerSpecificDB::~ManufacturerSpecificDB
(
)
{

	if (!s_bXmlLoaded) UnloadProductXML();

}


//-----------------------------------------------------------------------------
// <ManufacturerSpecificDB::LoadProductXML>
// Load the XML that maps manufacturer and product IDs to human-readable names
//-----------------------------------------------------------------------------
bool ManufacturerSpecificDB::LoadProductXML
(
)
{
	s_bXmlLoaded = true;
	FileOps *fo = FileOps::Create();

	// Parse the Z-Wave manufacturer and product XML file.
	string configPath;
	Options::Get()->GetOptionAsString( "ConfigPath", &configPath );

	string filename =  configPath + "manufacturer_specific.xml";

	TiXmlDocument* pDoc = new TiXmlDocument();
	if( !pDoc->LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		delete pDoc;
		Log::Write( LogLevel_Info, "Unable to load %s", filename.c_str() );
		return false;
	}

	TiXmlElement const* root = pDoc->RootElement();

	char const* str;
	char* pStopChar;

	str = root->Attribute("Revision");
	if (str) {
		Log::Write(LogLevel_Info, "Manufacturer_Specific.xml file Revision is %s", str);
		m_revision = atoi(str);
	} else {
		Log::Write(LogLevel_Warning, "Manufacturer_Specific.xml file has no Revision");
		m_revision = 0;
	}

	TiXmlElement const* manufacturerElement = root->FirstChildElement();
	while( manufacturerElement )
	{
		str = manufacturerElement->Value();
		if( str && !strcmp( str, "Manufacturer" ) )
		{
			// Read in the manufacturer attributes
			str = manufacturerElement->Attribute( "id" );
			if( !str )
			{
				Log::Write( LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing manufacturer id attribute", manufacturerElement->Row() );
				delete pDoc;
				return false;
			}
			uint16 manufacturerId = (uint16)strtol( str, &pStopChar, 16 );

			str = manufacturerElement->Attribute( "name" );
			if( !str )
			{
				Log::Write( LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing manufacturer name attribute", manufacturerElement->Row() );
				delete pDoc;
				return false;
			}

			// Add this manufacturer to the map
			s_manufacturerMap[manufacturerId] = str;

			// Parse all the products for this manufacturer
			TiXmlElement const* productElement = manufacturerElement->FirstChildElement();
			while( productElement )
			{
				str = productElement->Value();
				if( str && !strcmp( str, "Product" ) )
				{
					str = productElement->Attribute( "type" );
					if( !str )
					{
						Log::Write( LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing product type attribute", productElement->Row() );
						delete pDoc;
						return false;
					}
					uint16 productType = (uint16)strtol( str, &pStopChar, 16 );

					str = productElement->Attribute( "id" );
					if( !str )
					{
						Log::Write( LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing product id attribute", productElement->Row() );
						delete pDoc;
						return false;
					}
					uint16 productId = (uint16)strtol( str, &pStopChar, 16 );

					str = productElement->Attribute( "name" );
					if( !str )
					{
						Log::Write( LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing product name attribute", productElement->Row() );
						delete pDoc;
						return false;
					}
					string productName = str;

					// Optional config path
					string dconfigPath;
					str = productElement->Attribute( "config" );
					if( str )
					{
						dconfigPath = str;
						/* check if the file exists */
						string path = configPath + dconfigPath;
						if (!fo->FileExists(path)) {
							Log::Write( LogLevel_Warning, "Config File for %s does not exist", productName.c_str());
							/* try to download it */

						}

					}



					// Add the product to the map
					Product* product = new Product( manufacturerId, productType, productId, productName, dconfigPath );
					if ( s_productMap[product->GetKey()] != NULL )
					{
						Product *c = s_productMap[product->GetKey()];
						Log::Write( LogLevel_Info, "Product name collision: %s type %x id %x manufacturerid %x, collides with %s, type %x id %x manufacturerid %x", productName.c_str(), productType, productId, manufacturerId, c->GetProductName().c_str(), c->GetProductType(), c->GetProductId(), c->GetManufacturerId());
						delete product;
					}
					else
					{
						s_productMap[product->GetKey()] = product;
					}
				}

				// Move on to the next product.
				productElement = productElement->NextSiblingElement();
			}
		}

		// Move on to the next manufacturer.
		manufacturerElement = manufacturerElement->NextSiblingElement();
	}

	delete pDoc;
	return true;
}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::UnloadProductXML>
// Free the XML that maps manufacturer and product IDs
//-----------------------------------------------------------------------------
void ManufacturerSpecificDB::UnloadProductXML
(
)
{
	if (s_bXmlLoaded)
	{
		map<int64,Product*>::iterator pit = s_productMap.begin();
		while( !s_productMap.empty() )
		{
		  	delete pit->second;
			s_productMap.erase( pit );
			pit = s_productMap.begin();
		}

		map<uint16,string>::iterator mit = s_manufacturerMap.begin();
		while( !s_manufacturerMap.empty() )
		{
			s_manufacturerMap.erase( mit );
			mit = s_manufacturerMap.begin();
		}

		s_bXmlLoaded = false;
	}
}
