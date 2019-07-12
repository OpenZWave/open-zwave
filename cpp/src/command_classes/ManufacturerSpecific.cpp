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
#include "value_classes/ValueInt.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum ManufacturerSpecificCmd
			{
				ManufacturerSpecificCmd_Get = 0x04,
				ManufacturerSpecificCmd_Report = 0x05,
				ManufacturerSpecificCmd_DeviceGet = 0x06,
				ManufacturerSpecificCmd_DeviceReport = 0x07
			};

			enum
			{
				DeviceSpecificGet_DeviceIDType_FactoryDefault = 0x00,
				DeviceSpecificGet_DeviceIDType_SerialNumber = 0x01,
				DeviceSpecificGet_DeviceIDType_PseudoRandom = 0x02,
			};

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::ManufacturerSpecific>
// Constructor
//-----------------------------------------------------------------------------
			ManufacturerSpecific::ManufacturerSpecific(uint32 const _homeId, uint8 const _nodeId) :
					CommandClass(_homeId, _nodeId), m_fileConfigRevision(0), m_loadedConfigRevision(0), m_latestConfigRevision(0)
			{
				SetStaticRequest(StaticRequest_Values);
			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool ManufacturerSpecific::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{

				bool res = false;
				if (GetVersion() > 1)
				{
					if (_requestFlags & RequestFlag_Static)
					{
						{
							Msg* msg = new Msg("ManufacturerSpecificCmd_DeviceGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->SetInstance(this, _instance);
							msg->Append(GetNodeId());
							msg->Append(3);
							msg->Append(GetCommandClassId());
							msg->Append(ManufacturerSpecificCmd_DeviceGet);
							msg->Append(DeviceSpecificGet_DeviceIDType_FactoryDefault);
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, _queue);
						}
						{
							Msg* msg = new Msg("ManufacturerSpecificCmd_DeviceGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->SetInstance(this, _instance);
							msg->Append(GetNodeId());
							msg->Append(3);
							msg->Append(GetCommandClassId());
							msg->Append(ManufacturerSpecificCmd_DeviceGet);
							msg->Append(DeviceSpecificGet_DeviceIDType_SerialNumber);
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, _queue);
						}

						res = true;
					}
				}

				if ((_requestFlags & RequestFlag_Static) && HasStaticRequest(StaticRequest_Values))
				{
					res |= RequestValue(_requestFlags, 0, _instance, _queue);
				}

				return res;
			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool ManufacturerSpecific::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_instance != 1)
				{
					// This command class doesn't work with multiple instances
					return false;
				}
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Msg* msg = new Msg("ManufacturerSpecificCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(ManufacturerSpecificCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "ManufacturerSpecificCmd_Get Not Supported on this node");
				}
				return false;
			}

			void ManufacturerSpecific::SetProductDetails(uint16 manufacturerId, uint16 productType, uint16 productId)
			{

				string configPath = "";
				std::shared_ptr<Internal::ProductDescriptor> product = GetDriver()->GetManufacturerSpecificDB()->getProduct(manufacturerId, productType, productId);

				Node *node = GetNodeUnsafe();
				if (!product)
				{
					char str[64];
					snprintf(str, sizeof(str), "Unknown: id=%.4x", manufacturerId);
					string manufacturerName = str;

					snprintf(str, sizeof(str), "Unknown: type=%.4x, id=%.4x", productType, productId);
					string productName = str;

					node->SetManufacturerName(manufacturerName);
					node->SetProductName(productName);
				}
				else
				{
					node->SetManufacturerName(product->GetManufacturerName());
					node->SetProductName(product->GetProductName());
					node->SetProductDetails(product);
				}

				node->SetManufacturerId(manufacturerId);

				node->SetProductType(productType);

				node->SetProductId(productId);

			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool ManufacturerSpecific::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (ManufacturerSpecificCmd_Report == (ManufacturerSpecificCmd) _data[0])
				{

					// first two bytes are manufacturer id code
					uint16 manufacturerId = (((uint16) _data[1]) << 8) | (uint16) _data[2];

					// next four are product type and product id
					uint16 productType = (((uint16) _data[3]) << 8) | (uint16) _data[4];
					uint16 productId = (((uint16) _data[5]) << 8) | (uint16) _data[6];

					if (Node* node = GetNodeUnsafe())
					{
						// Attempt to create the config parameters
						SetProductDetails(manufacturerId, productType, productId);
						ClearStaticRequest(StaticRequest_Values);
						node->m_manufacturerSpecificClassReceived = true;

						if (node->getConfigPath().size() > 0)
						{
							LoadConfigXML();
						}

						Log::Write(LogLevel_Info, GetNodeId(), "Received manufacturer specific report from node %d: Manufacturer=%s, Product=%s", GetNodeId(), node->GetManufacturerName().c_str(), node->GetProductName().c_str());
						Log::Write(LogLevel_Info, GetNodeId(), "Node Identity Codes: %.4x:%.4x:%.4x", manufacturerId, productType, productId);
					}

					// Notify the watchers of the name changes
					Notification* notification = new Notification(Notification::Type_NodeNaming);
					notification->SetHomeAndNodeIds(GetHomeId(), GetNodeId());
					GetDriver()->QueueNotification(notification);

					return true;
				}
				else if (ManufacturerSpecificCmd_DeviceReport == (ManufacturerSpecificCmd) _data[0])
				{
					uint8 deviceIDType = (_data[1] & 0x07);
					uint8 dataFormat = (_data[2] & 0xe0) >> 0x05;
					uint8 data_length = (_data[2] & 0x1f);
					uint8 const* deviceIDData = &_data[3];
					string deviceID = "";
					for (int i = 0; i < data_length; i++)
					{
						char temp_chr[32];
						memset(temp_chr, 0, sizeof(temp_chr));
						if (dataFormat == 0x00)
						{
							temp_chr[0] = deviceIDData[i];
						}
						else
						{
							snprintf(temp_chr, sizeof(temp_chr), "%.2x", deviceIDData[i]);
						}
						deviceID += temp_chr;
					}
					if (deviceIDType == DeviceSpecificGet_DeviceIDType_FactoryDefault)
					{
						Internal::VC::ValueString *default_value = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_ManufacturerSpecific::DeviceID));
						default_value->OnValueRefreshed(deviceID);
						default_value->Release();
					}
					else if (deviceIDType == DeviceSpecificGet_DeviceIDType_SerialNumber)
					{
						Internal::VC::ValueString *serial_value = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_ManufacturerSpecific::SerialNumber));
						serial_value->OnValueRefreshed(deviceID);
						serial_value->Release();
					}
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::LoadConfigXML>
// Try to find and load an XML file describing the device's config params
//-----------------------------------------------------------------------------
			bool ManufacturerSpecific::LoadConfigXML()
			{
				if (GetNodeUnsafe()->getConfigPath().size() == 0)
					return false;

				string configPath;
				Options::Get()->GetOptionAsString("ConfigPath", &configPath);

				string filename = configPath + GetNodeUnsafe()->getConfigPath();

				TiXmlDocument* doc = new TiXmlDocument();
				Log::Write(LogLevel_Info, GetNodeId(), "  Opening config param file %s", filename.c_str());
				if (!doc->LoadFile(filename.c_str(), TIXML_ENCODING_UTF8))
				{
					delete doc;
					Log::Write(LogLevel_Info, GetNodeId(), "Unable to find or load Config Param file %s", filename.c_str());
					return false;
				}
				doc->SetUserData((void *) filename.c_str());
				/* make sure it has the right xmlns */
				TiXmlElement *product = doc->RootElement();
				char const *xmlns = product->Attribute("xmlns");
				if (xmlns && strcmp(xmlns, "https://github.com/OpenZWave/open-zwave"))
				{
					delete doc;
					Log::Write(LogLevel_Warning, GetNodeId(), "Invalid XML Namespace in %s - Ignoring", filename.c_str());
					return false;
				}

				Node::QueryStage qs = GetNodeUnsafe()->GetCurrentQueryStage();
				if (qs == Node::QueryStage_ManufacturerSpecific1)
				{
					GetNodeUnsafe()->ReadDeviceProtocolXML(doc->RootElement());
				}
				else
				{
					if (!GetNodeUnsafe()->m_manufacturerSpecificClassReceived)
					{
						GetNodeUnsafe()->ReadDeviceProtocolXML(doc->RootElement());
					}
				}
				GetNodeUnsafe()->ReadCommandClassesXML(doc->RootElement());
				GetNodeUnsafe()->ReadMetaDataFromXML(doc->RootElement());
				delete doc;
				return true;
			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::ReLoadConfigXML>
// Reload previously discovered device configuration.
//-----------------------------------------------------------------------------
			void ManufacturerSpecific::ReLoadConfigXML()
			{
				LoadConfigXML();
			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void ManufacturerSpecific::CreateVars(uint8 const _instance)
			{
				if (_instance == 1)
				{
					if (Node* node = GetNodeUnsafe())
					{
						node->CreateValueInt(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ManufacturerSpecific::LoadedConfig, "Loaded Config Revision", "", true, false, m_loadedConfigRevision, 0);
						node->CreateValueInt(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ManufacturerSpecific::LocalConfig, "Config File Revision", "", true, false, m_fileConfigRevision, 0);
						node->CreateValueInt(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ManufacturerSpecific::LatestConfig, "Latest Available Config File Revision", "", true, false, m_latestConfigRevision, 0);
						node->CreateValueString(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ManufacturerSpecific::DeviceID, "Device ID", "", true, false, "", 0);
						node->CreateValueString(ValueID::ValueGenre_System, GetCommandClassId(), _instance, ValueID_Index_ManufacturerSpecific::SerialNumber, "Serial Number", "", true, false, "", 0);
					}
				}

			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::setLatestRevision>
// Set the Latest Config Revision Available for this device
//-----------------------------------------------------------------------------
			void ManufacturerSpecific::setLatestConfigRevision(uint32 rev)
			{

				m_latestConfigRevision = rev;

				if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(1, ValueID_Index_ManufacturerSpecific::LatestConfig)))
				{
					value->OnValueRefreshed(rev);
					value->Release();
				}

			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::setFileConfigRevision>
// Set the File Config Revision for this device
//-----------------------------------------------------------------------------

			void ManufacturerSpecific::setFileConfigRevision(uint32 rev)
			{
				m_fileConfigRevision = rev;

				if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(1, ValueID_Index_ManufacturerSpecific::LocalConfig)))
				{
					value->OnValueRefreshed(rev);
					value->Release();
				}

			}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::setFileConfigRevision>
// Set the File Config Revision for this device
//-----------------------------------------------------------------------------

			void ManufacturerSpecific::setLoadedConfigRevision(uint32 rev)
			{
				m_latestConfigRevision = rev;

				if (Internal::VC::ValueInt* value = static_cast<Internal::VC::ValueInt*>(GetValue(1, ValueID_Index_ManufacturerSpecific::LoadedConfig)))
				{
					value->OnValueRefreshed(rev);
					value->Release();
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
