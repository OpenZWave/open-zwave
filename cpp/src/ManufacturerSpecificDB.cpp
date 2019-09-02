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
#include "Driver.h"
#include "platform/Log.h"
#include "platform/FileOps.h"
#include "platform/Mutex.h"
#include "Notification.h"
#include "Utils.h"

namespace OpenZWave
{
	namespace Internal
	{

		ManufacturerSpecificDB *ManufacturerSpecificDB::s_instance = NULL;
		std::map<uint16, std::string> ManufacturerSpecificDB::s_manufacturerMap;
		std::map<int64, std::shared_ptr<ProductDescriptor> > ManufacturerSpecificDB::s_productMap;
		bool ManufacturerSpecificDB::s_bXmlLoaded = false;

		ManufacturerSpecificDB *ManufacturerSpecificDB::Create()
		{

			if ( NULL == s_instance)
			{
				s_instance = new ManufacturerSpecificDB();
			}
			return s_instance;

		}

		void ManufacturerSpecificDB::Destroy()
		{
			delete s_instance;
			s_instance = NULL;
		}

		ManufacturerSpecificDB::ManufacturerSpecificDB() :
				m_MfsMutex(new Internal::Platform::Mutex()), m_revision(0), m_latestRevision(0), m_initializing(true)
		{
			// Ensure the singleton instance is set
			s_instance = this;

			if (!s_bXmlLoaded)
				LoadProductXML();

		}

		ManufacturerSpecificDB::~ManufacturerSpecificDB()
		{

			if (!s_bXmlLoaded)
				UnloadProductXML();

		}

//-----------------------------------------------------------------------------
// <ManufacturerSpecificDB::LoadConfigFileRevision>
// Load the Config File Revision from each config file specified in our 
// ManufacturerSpecific.xml file
//-----------------------------------------------------------------------------
		void ManufacturerSpecificDB::LoadConfigFileRevision(ProductDescriptor *product)
		{
			// Parse the Z-Wave manufacturer and product XML file.
			string configPath;
			Options::Get()->GetOptionAsString("ConfigPath", &configPath);

			if (product->GetConfigPath().size() > 0)
			{
				string path = configPath + product->GetConfigPath();

				TiXmlDocument* pDoc = new TiXmlDocument();
				if (!pDoc->LoadFile(path.c_str(), TIXML_ENCODING_UTF8))
				{
					delete pDoc;
					Log::Write(LogLevel_Info, "Unable to load config file %s", path.c_str());
					return;
				}
				pDoc->SetUserData((void *) path.c_str());
				TiXmlElement const* root = pDoc->RootElement();
				char const *str = root->Value();
				if (str && !strcmp(str, "Product"))
				{
					str = root->Attribute("xmlns");
					if (str && strcmp(str, "https://github.com/OpenZWave/open-zwave"))
					{
						Log::Write(LogLevel_Info, "Product Config File % has incorrect xml Namespace", path.c_str());
						delete pDoc;
						return;
					}
					// Read in the revision attributes
					str = root->Attribute("Revision");
					if (!str)
					{
						Log::Write(LogLevel_Info, "Error in Product Config file at line %d - missing Revision  attribute", root->Row());
						delete pDoc;
						return;
					}
					product->SetConfigRevision(atol(str));
				}
				delete pDoc;
			}
		}

//-----------------------------------------------------------------------------
// <ManufacturerSpecificDB::LoadProductXML>
// Load the XML that maps manufacturer and product IDs to human-readable names
//-----------------------------------------------------------------------------
		bool ManufacturerSpecificDB::LoadProductXML()
		{
			LockGuard LG(m_MfsMutex);

			// Parse the Z-Wave manufacturer and product XML file.
			string configPath;
			Options::Get()->GetOptionAsString("ConfigPath", &configPath);

			string filename = configPath + "manufacturer_specific.xml";

			TiXmlDocument* pDoc = new TiXmlDocument();
			if (!pDoc->LoadFile(filename.c_str(), TIXML_ENCODING_UTF8))
			{
				delete pDoc;
				Log::Write(LogLevel_Info, "Unable to load %s", filename.c_str());
				return false;
			}
			pDoc->SetUserData((void *) filename.c_str());
			TiXmlElement const* root = pDoc->RootElement();

			char const* str;
			char* pStopChar;

			str = root->Attribute("Revision");
			if (str)
			{
				Log::Write(LogLevel_Info, "Manufacturer_Specific.xml file Revision is %s", str);
				m_revision = atoi(str);
			}
			else
			{
				Log::Write(LogLevel_Warning, "Manufacturer_Specific.xml file has no Revision");
				m_revision = 0;
			}

			TiXmlElement const* manufacturerElement = root->FirstChildElement();
			while (manufacturerElement)
			{
				str = manufacturerElement->Value();
				if (str && !strcmp(str, "Manufacturer"))
				{
					// Read in the manufacturer attributes
					str = manufacturerElement->Attribute("id");
					if (!str)
					{
						Log::Write(LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing manufacturer id attribute", manufacturerElement->Row());
						delete pDoc;
						return false;
					}
					uint16 manufacturerId = (uint16) strtol(str, &pStopChar, 16);

					str = manufacturerElement->Attribute("name");
					if (!str)
					{
						Log::Write(LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing manufacturer name attribute", manufacturerElement->Row());
						delete pDoc;
						return false;
					}

					// Add this manufacturer to the map
					s_manufacturerMap[manufacturerId] = str;

					// Parse all the products for this manufacturer
					TiXmlElement const* productElement = manufacturerElement->FirstChildElement();
					while (productElement)
					{
						str = productElement->Value();
						if (str && !strcmp(str, "Product"))
						{
							str = productElement->Attribute("type");
							if (!str)
							{
								Log::Write(LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing product type attribute", productElement->Row());
								delete pDoc;
								return false;
							}
							uint16 productType = (uint16) strtol(str, &pStopChar, 16);

							str = productElement->Attribute("id");
							if (!str)
							{
								Log::Write(LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing product id attribute", productElement->Row());
								delete pDoc;
								return false;
							}
							uint16 productId = (uint16) strtol(str, &pStopChar, 16);

							str = productElement->Attribute("name");
							if (!str)
							{
								Log::Write(LogLevel_Info, "Error in manufacturer_specific.xml at line %d - missing product name attribute", productElement->Row());
								delete pDoc;
								return false;
							}
							string productName = str;

							// Optional config path
							string dconfigPath;
							str = productElement->Attribute("config");
							if (str)
							{
								dconfigPath = str;
							}

							// Add the product to the map
							ProductDescriptor* product = new ProductDescriptor(manufacturerId, productType, productId, productName, s_manufacturerMap[manufacturerId], dconfigPath);
							if (s_productMap[product->GetKey()] != NULL)
							{
								std::shared_ptr<ProductDescriptor> c = s_productMap[product->GetKey()];
								Log::Write(LogLevel_Info, "Product name collision: %s type %x id %x manufacturerid %x, collides with %s, type %x id %x manufacturerid %x", productName.c_str(), productType, productId, manufacturerId, c->GetProductName().c_str(), c->GetProductType(), c->GetProductId(), c->GetManufacturerId());
								delete product;
							}
							else
							{
								LoadConfigFileRevision(product);
								s_productMap[product->GetKey()] = std::shared_ptr<ProductDescriptor>(product);
							}
						}

						// Move on to the next product.
						productElement = productElement->NextSiblingElement();
					}
				}

				// Move on to the next manufacturer.
				manufacturerElement = manufacturerElement->NextSiblingElement();
			}
			s_bXmlLoaded = true;

			delete pDoc;
			return true;
		}

//-----------------------------------------------------------------------------
// <ManufacturerSpecific::UnloadProductXML>
// Free the XML that maps manufacturer and product IDs
//-----------------------------------------------------------------------------
		void ManufacturerSpecificDB::UnloadProductXML()
		{
			LockGuard LG(m_MfsMutex);
			if (s_bXmlLoaded)
			{
				map<int64, std::shared_ptr<ProductDescriptor> >::iterator pit = s_productMap.begin();
				while (!s_productMap.empty())
				{
					s_productMap.erase(pit);
					pit = s_productMap.begin();
				}

				map<uint16, string>::iterator mit = s_manufacturerMap.begin();
				while (!s_manufacturerMap.empty())
				{
					s_manufacturerMap.erase(mit);
					mit = s_manufacturerMap.begin();
				}

				s_bXmlLoaded = false;
			}
		}

		void ManufacturerSpecificDB::checkConfigFiles(Driver *driver)
		{
			LockGuard LG(m_MfsMutex);
			if (!s_bXmlLoaded)
				LoadProductXML();

			string configPath;
			Options::Get()->GetOptionAsString("ConfigPath", &configPath);

			map<int64, std::shared_ptr<ProductDescriptor> >::iterator pit;
			for (pit = s_productMap.begin(); pit != s_productMap.end(); pit++)
			{
				std::shared_ptr<ProductDescriptor> c = pit->second;
				if (c->GetConfigPath().size() > 0)
				{
					string path = configPath + c->GetConfigPath();

					/* check if we are downloading already */
					std::list<string>::iterator iter = std::find(m_downloading.begin(), m_downloading.end(), path);
					/* check if the file exists */
					if (iter == m_downloading.end() && !Internal::Platform::FileOps::Create()->FileExists(path))
					{
						Log::Write(LogLevel_Warning, "Config File for %s does not exist - %s", c->GetProductName().c_str(), path.c_str());
						/* try to download it */
						if (driver->startConfigDownload(c->GetManufacturerId(), c->GetProductType(), c->GetProductId(), path))
						{
							m_downloading.push_back(path);
						}
						else
						{
							Log::Write(LogLevel_Warning, "Can't download file %s", path.c_str());
							Notification* notification = new Notification(Notification::Type_UserAlerts);
							notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
							driver->QueueNotification(notification);
						}
					}
					else if (iter != m_downloading.end())
					{
						Log::Write(LogLevel_Debug, "Config file for %s already queued", c->GetProductName().c_str());
					}
				}
			}
			checkInitialized();
		}

		void ManufacturerSpecificDB::configDownloaded(Driver *driver, string file, uint8 node, bool success)
		{
			/* check if we are downloading already */
			std::list<string>::iterator iter = std::find(m_downloading.begin(), m_downloading.end(), file);
			if (iter != m_downloading.end())
			{
				m_downloading.erase(iter);
				if ((node > 0) && success)
				{
					driver->refreshNodeConfig(node);
				}
				else
				{
					checkInitialized();
				}
			}
			else
			{
				Log::Write(LogLevel_Warning, "File is not in the list of downloading files: %s", file.c_str());
				checkInitialized();
			}
		}

		void ManufacturerSpecificDB::mfsConfigDownloaded(Driver *driver, string file, bool success)
		{
			/* check if we are downloading already */
			std::list<string>::iterator iter = std::find(m_downloading.begin(), m_downloading.end(), file);
			if (iter != m_downloading.end())
			{
				m_downloading.erase(iter);
				if (success)
				{
					UnloadProductXML();
					LoadProductXML();
					checkConfigFiles(driver);
				}
			}
			else
			{
				Log::Write(LogLevel_Warning, "File is not in the list of downloading files: %s", file.c_str());
			}
			checkInitialized();
		}

		bool ManufacturerSpecificDB::isReady()
		{
			if (!m_initializing && (m_downloading.size() == 0))
				return true;
			return false;
		}

		void ManufacturerSpecificDB::checkInitialized()
		{
			if (!m_initializing)
				return;
			Log::Write(LogLevel_Debug, "Downloads Remaining: %d", m_downloading.size());
			if (m_downloading.size() == 0)
			{
				Log::Write(LogLevel_Info, "ManufacturerSpecificDB Initialized");
				m_initializing = false;
			}
		}

		std::shared_ptr<ProductDescriptor> ManufacturerSpecificDB::getProduct(uint16 _manufacturerId, uint16 _productType, uint16 _productId)
		{

			if (!s_bXmlLoaded)
				LoadProductXML();

			// Try to get the real manufacturer and product names
			map<uint16, string>::iterator mit = s_manufacturerMap.find(_manufacturerId);
			if (mit != s_manufacturerMap.end())
			{
				// Get the product
				map<int64, std::shared_ptr<ProductDescriptor> >::iterator pit = s_productMap.find(ProductDescriptor::GetKey(_manufacturerId, _productType, _productId));
				if (pit != s_productMap.end())
				{
					return pit->second;
				}
			}
			return NULL;
		}

		bool ManufacturerSpecificDB::updateConfigFile(Driver *driver, Node *node)
		{
			string configPath;
			bool ret = false;
			Options::Get()->GetOptionAsString("ConfigPath", &configPath);
			string path = configPath + node->getConfigPath();

			if (driver->startConfigDownload(node->GetManufacturerId(), node->GetProductType(), node->GetProductId(), path, node->GetNodeId()))
			{
				m_downloading.push_back(path);
				ret = true;
			}
			else
			{
				Log::Write(LogLevel_Warning, "Can't download Config file %s", node->getConfigPath().c_str());
				Notification* notification = new Notification(Notification::Type_UserAlerts);
				notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
				driver->QueueNotification(notification);
			}
			checkInitialized();
			return ret;
		}
		bool ManufacturerSpecificDB::updateMFSConfigFile(Driver *driver)
		{
			bool ret = false;
			string configPath;
			Options::Get()->GetOptionAsString("ConfigPath", &configPath);
			string path = configPath + "manufacturer_specific.xml";

			if (driver->startMFSDownload(path))
			{
				m_downloading.push_back(path);
				ret = true;
			}
			else
			{
				Log::Write(LogLevel_Warning, "Can't download ManufacturerSpecifix.xml Config file");
				Notification* notification = new Notification(Notification::Type_UserAlerts);
				notification->SetUserAlertNotification(Notification::Alert_ConfigFileDownloadFailed);
				driver->QueueNotification(notification);
			}

			checkInitialized();
			return ret;
		}
	} // namespace Internal
} // namespace OpenZWave
