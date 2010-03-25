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

enum
{
	ValueIndex_Manufacturer = 0,
	ValueIndex_Product
};

map<uint16,string> ManufacturerSpecific::s_manufacturerMap;
map<int64,ManufacturerSpecific::Product*> ManufacturerSpecific::s_productMap;
bool ManufacturerSpecific::s_bXmlLoaded = false;


//-----------------------------------------------------------------------------
// <ManufacturerSpecific::SaveStatic>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void ManufacturerSpecific::SaveStatic
( 
	FILE* _file	
)
{
	fprintf( _file, "	<CommandClass id=\"%d\" name=\"%s\" version=\"%d\">\n", GetNodeId(), GetCommandClassId(), GetCommandClassName().c_str(), GetVersion() );
	fprintf( _file, "	</CommandClass>\n" );
}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::RequestStatic>												   
// Request the static manufacturer specific data									   
//-----------------------------------------------------------------------------
void ManufacturerSpecific::RequestStatic
(
)
{
	Log::Write( "Requesting the manufacturer specific data from node %d", GetNodeId() );
	Msg* msg = new Msg( "ManufacturerSpecificCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ManufacturerSpecificCmd_Get );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg );
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
	string filename =  Manager::Get()->GetConfigPath() + "ManufacturerSpecific.xml";

	TiXmlDocument* pDoc = new TiXmlDocument();
	if( !pDoc->LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		delete pDoc;	
		Log::Write( "Unable to load ManufacturerSpecific.xml" );
		return false;
	}

	TiXmlElement const* root = pDoc->RootElement();

	char const* str;
	char* pStopChar;
	
	TiXmlNode const* node = root->FirstChild();
	while( node )
	{
		str = node->Value();
		if( str && !strcmp( str, "Manufacturer" ) )
		{
			TiXmlElement const* pManufacturerElement = node->ToElement();
			if( pManufacturerElement )
			{
				// Read in the manufacturer attributes
				str = pManufacturerElement->Attribute( "id" );
				if( !str )
				{
					Log::Write( "Error in ManufacturerSpecific.xml at line %d - missing manufacturer id attribute", pManufacturerElement->Row() );
					return false;
				}
				uint16 manufacturerId = (uint16)strtol( str, &pStopChar, 16 );

				str = pManufacturerElement->Attribute( "name" );
				if( !str )
				{
					Log::Write( "Error in ManufacturerSpecific.xml at line %d - missing manufacturer name attribute", pManufacturerElement->Row() );
					return false;
				}
				
				// Add this manufacturer to the map
				s_manufacturerMap[manufacturerId] = str;

				// Parse all the products for this manufacturer
				TiXmlNode const* pProductNode = pManufacturerElement->FirstChild();
				while( pProductNode )
				{
					str = pProductNode->Value();
					if( str && !strcmp( str, "Product" ) )
					{
						TiXmlElement const* pProductElement = pProductNode->ToElement();
						if( pProductElement )
						{
							str = pProductElement->Attribute( "type" );
							if( !str )
							{
								Log::Write( "Error in ManufacturerSpecific.xml at line %d - missing product type attribute", pProductElement->Row() );
								return false;
							}
							uint16 productType = (uint16)strtol( str, &pStopChar, 16 );
							
							str = pProductElement->Attribute( "id" );
							if( !str )
							{
								Log::Write( "Error in ManufacturerSpecific.xml at line %d - missing product id attribute", pProductElement->Row() );
								return false;
							}
							uint16 productId = (uint16)strtol( str, &pStopChar, 16 );

							str = pProductElement->Attribute( "name" );
							if( !str )
							{
								Log::Write( "Error in ManufacturerSpecific.xml at line %d - missing product name attribute", pProductElement->Row() );
								return false;
							}
							string productName = str;

							// Add the product to the map
							Product* product = new Product( manufacturerId, productType, productId, productName );
							s_productMap[product->GetKey()] = product;
						}
					}

					// Move on to the next product.
					pProductNode = pProductNode->NextSibling();
				}
			}
		}

		// Move on to the next manufacturer.
		node = node->NextSibling();
	}

	return true;
}

