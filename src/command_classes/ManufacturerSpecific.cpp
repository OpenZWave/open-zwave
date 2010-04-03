//-----------------------------------------------------------------------------
//
//	ManufacturerSpecific.cpp
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

#include "CommandClasses.h"
#include "ManufacturerSpecific.h"
#include "tinyxml.h"

#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Manager.h"
#include "Driver.h"
#include "Log.h"

#include "ValueStore.h"
#include "ValueString.h"

using namespace OpenZWave;

enum ManufacturerSpecificCmd
{
	ManufacturerSpecificCmd_Get		= 0x04,
	ManufacturerSpecificCmd_Report	= 0x05
};

map<uint16,string> ManufacturerSpecific::s_manufacturerMap;
map<int64,ManufacturerSpecific::Product*> ManufacturerSpecific::s_productMap;
bool ManufacturerSpecific::s_bXmlLoaded = false;


//-----------------------------------------------------------------------------
// <ManufacturerSpecific::RequestState>												   
// Request current state from the device									   
//-----------------------------------------------------------------------------
void ManufacturerSpecific::RequestState
(
	uint32 const _requestFlags
)
{
	if( _requestFlags & RequestFlag_Static )
	{
		Msg* msg = new Msg( "ManufacturerSpecificCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ManufacturerSpecificCmd_Get );
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
		GetDriver()->SendMsg( msg );
	}
}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ManufacturerSpecific::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	char str[64];
	if( ManufacturerSpecificCmd_Report == (ManufacturerSpecificCmd)_data[0] )
	{
		if( !s_bXmlLoaded )
		{
			LoadProductXML();
		}

		uint16 manufacturerId = (((uint16)_data[1])<<8) | (uint16)_data[2];
		snprintf( str, sizeof(str), "Unknown: id=%.4x", manufacturerId );
		string manufacturerName = str;

		uint16 productType = (((uint16)_data[3])<<8) | (uint16)_data[4];
		uint16 productId = (((uint16)_data[5])<<8) | (uint16)_data[6];

		snprintf( str, sizeof(str), "Unknown: type=%.4x, id=%.4x", productType, productId );
		string productName = str;

		string configPath = "";

		// Try to get the real manufacturer and product names
		map<uint16,string>::iterator mit = s_manufacturerMap.find( manufacturerId );
		if( mit != s_manufacturerMap.end() )
		{
			// Replace the id with the real name
			manufacturerName = mit->second;

			// Get the product
			map<int64,Product*>::iterator pit = s_productMap.find( Product::GetKey( manufacturerId, productType, productId ) );
			if( pit != s_productMap.end() )
			{
				productName = pit->second->GetProductName();
				configPath = pit->second->GetConfigPath();
			}
		}

		if( Node* node = GetNode() )
		{
			// Set the values into the node
			node->SetManufacturerName( manufacturerName );
			node->SetProductName( productName );

			snprintf( str, sizeof(str), "%.4x", manufacturerId );
			node->SetManufacturerId( str );

			snprintf( str, sizeof(str), "%.4x", productType );
			node->SetProductType( str );

			snprintf( str, sizeof(str), "%.4x", productId );
			node->SetProductId( str );

			// Attempt to create the config parameters
			if( configPath.size() > 0 )
			{
				LoadConfigXML( configPath );
			}
		}
		
		Log::Write( "Received manufacturer specific report from node %d: Manufacturer=%s, Product=%s", GetNodeId(), manufacturerName.c_str(), productName.c_str() );
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::LoadProductXML>
// Load the XML that maps manufacturer and product IDs to human-readable names
//-----------------------------------------------------------------------------
bool ManufacturerSpecific::LoadProductXML
(
)
{
	s_bXmlLoaded = true;

	// Parse the Z-Wave manufacturer and product XML file.
	string filename =  Manager::Get()->GetConfigPath() + "manufacturer_specific.xml";

	TiXmlDocument* pDoc = new TiXmlDocument();
	if( !pDoc->LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		delete pDoc;	
		Log::Write( "Unable to load manufacturer_specific.xml" );
		return false;
	}

	TiXmlElement const* root = pDoc->RootElement();

	char const* str;
	char* pStopChar;
	
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
				Log::Write( "Error in manufacturer_specific.xml at line %d - missing manufacturer id attribute", manufacturerElement->Row() );
				delete pDoc;	
				return false;
			}
			uint16 manufacturerId = (uint16)strtol( str, &pStopChar, 16 );

			str = manufacturerElement->Attribute( "name" );
			if( !str )
			{
				Log::Write( "Error in manufacturer_specific.xml at line %d - missing manufacturer name attribute", manufacturerElement->Row() );
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
						Log::Write( "Error in manufacturer_specific.xml at line %d - missing product type attribute", productElement->Row() );
						delete pDoc;	
						return false;
					}
					uint16 productType = (uint16)strtol( str, &pStopChar, 16 );
					
					str = productElement->Attribute( "id" );
					if( !str )
					{
						Log::Write( "Error in manufacturer_specific.xml at line %d - missing product id attribute", productElement->Row() );
						delete pDoc;	
						return false;
					}
					uint16 productId = (uint16)strtol( str, &pStopChar, 16 );

					str = productElement->Attribute( "name" );
					if( !str )
					{
						Log::Write( "Error in manufacturer_specific.xml at line %d - missing product name attribute", productElement->Row() );
						delete pDoc;	
						return false;
					}
					string productName = str;

					// Optional config path
					string configPath;
					str = productElement->Attribute( "config" );
					if( str )
					{
						configPath = str;
					}

					// Add the product to the map
					Product* product = new Product( manufacturerId, productType, productId, productName, configPath );
					s_productMap[product->GetKey()] = product;
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
// <ManufacturerSpecific::LoadConfigXML>
// Try to find and load an XML file describing the device's config params
//-----------------------------------------------------------------------------
bool ManufacturerSpecific::LoadConfigXML
(
	string const& _configPath
)
{
	if( Node* node = GetNode() )
	{
		string filename =  Manager::Get()->GetConfigPath() + _configPath;

		TiXmlDocument* doc = new TiXmlDocument();
		if( !doc->LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
		{
			delete doc;	
			Log::Write( "Unable to find or load Config Param file %s", filename.c_str() );
			return false;
		}

		node->ReadCommandClassesXML( doc->RootElement() );
		delete doc;	
		return true;
	}

	return false;
}

