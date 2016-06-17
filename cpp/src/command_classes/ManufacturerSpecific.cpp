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

#include "command_classes/CommandClasses.h"
#include "command_classes/ManufacturerSpecific.h"
#include "tinyxml.h"

#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Options.h"
#include "Manager.h"
#include "Driver.h"
#include "ManufacturerSpecificDB.h"
#include "Notification.h"
#include "platform/Log.h"

#include "value_classes/ValueStore.h"
#include "value_classes/ValueString.h"

using namespace OpenZWave;

enum ManufacturerSpecificCmd
{
	ManufacturerSpecificCmd_Get		= 0x04,
	ManufacturerSpecificCmd_Report	= 0x05
};


//-----------------------------------------------------------------------------
// <ManufacturerSpecific::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool ManufacturerSpecific::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool ManufacturerSpecific::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "ManufacturerSpecificCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ManufacturerSpecificCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "ManufacturerSpecificCmd_Get Not Supported on this node");
	}
	return false;
}

void ManufacturerSpecific::SetProductDetails
(
	uint16 manufacturerId,
	uint16 productType,
	uint16 productId
)
{

	string configPath = "";
	ProductDescriptor *product = GetDriver()->GetManufacturerSpecificDB()->getProduct(manufacturerId, productType, productId);

	Node *node = GetNodeUnsafe();
	if (!product) {
			char str[64];
			snprintf( str, sizeof(str), "Unknown: id=%.4x", manufacturerId );
			string manufacturerName = str;

			snprintf( str, sizeof(str), "Unknown: type=%.4x, id=%.4x", productType, productId );
			string productName = str;

			node->SetManufacturerName( manufacturerName );
			node->SetProductName( productName );
	} else {
			node->SetManufacturerName( product->GetManufacturerName() );
			node->SetProductName( product->GetProductName() );
			node->SetProductDetails(product);
	}

	node->SetManufacturerId( manufacturerId );

	node->SetProductType( productType );

	node->SetProductId( productId );

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
	if( ManufacturerSpecificCmd_Report == (ManufacturerSpecificCmd)_data[0] )
	{

		// first two bytes are manufacturer id code
		uint16 manufacturerId = (((uint16)_data[1])<<8) | (uint16)_data[2];

		// next four are product type and product id
		uint16 productType = (((uint16)_data[3])<<8) | (uint16)_data[4];
		uint16 productId = (((uint16)_data[5])<<8) | (uint16)_data[6];

		if( Node* node = GetNodeUnsafe() )
		{
			// Attempt to create the config parameters
			SetProductDetails(manufacturerId, productType, productId);

			if( node->getConfigPath().size() > 0 )
			{
				LoadConfigXML( );
			}

			Log::Write( LogLevel_Info, GetNodeId(), "Received manufacturer specific report from node %d: Manufacturer=%s, Product=%s",
				    GetNodeId(), node->GetManufacturerName().c_str(), node->GetProductName().c_str() );
			Log::Write( LogLevel_Info, GetNodeId(), "Node Identity Codes: %.4x:%.4x:%.4x", manufacturerId, productType, productId );
			ClearStaticRequest( StaticRequest_Values );
			node->m_manufacturerSpecificClassReceived = true;
		}

		// Notify the watchers of the name changes
		Notification* notification = new Notification( Notification::Type_NodeNaming );
		notification->SetHomeAndNodeIds( GetHomeId(), GetNodeId() );
		GetDriver()->QueueNotification( notification );

		return true;
	}

	return false;
}



//-----------------------------------------------------------------------------
// <ManufacturerSpecific::LoadConfigXML>
// Try to find and load an XML file describing the device's config params
//-----------------------------------------------------------------------------
bool ManufacturerSpecific::LoadConfigXML
(
)
{
	if (GetNodeUnsafe()->getConfigPath().size() == 0)
		return false;


	string configPath;
	Options::Get()->GetOptionAsString( "ConfigPath", &configPath );

	string filename =  configPath + GetNodeUnsafe()->getConfigPath();

	TiXmlDocument* doc = new TiXmlDocument();
	Log::Write( LogLevel_Info, GetNodeId(), "  Opening config param file %s", filename.c_str() );
	if( !doc->LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		delete doc;
		Log::Write( LogLevel_Info, GetNodeId(), "Unable to find or load Config Param file %s", filename.c_str() );
		return false;
	}
	Node::QueryStage qs = GetNodeUnsafe()->GetCurrentQueryStage();
	if( qs == Node::QueryStage_ManufacturerSpecific1 )
	{
		 GetNodeUnsafe()->ReadDeviceProtocolXML( doc->RootElement() );
	}
	else
	{
		if( ! GetNodeUnsafe()->m_manufacturerSpecificClassReceived )
		{
			 GetNodeUnsafe()->ReadDeviceProtocolXML( doc->RootElement() );
		}
		 GetNodeUnsafe()->ReadCommandClassesXML( doc->RootElement() );
	}
	GetNodeUnsafe()->ReadMetaDataFromXML( doc->RootElement() );
	delete doc;
	return true;
}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::ReLoadConfigXML>
// Reload previously discovered device configuration.
//-----------------------------------------------------------------------------
void ManufacturerSpecific::ReLoadConfigXML
(
)
{
			LoadConfigXML( );
}
