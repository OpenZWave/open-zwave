//-----------------------------------------------------------------------------
//
//	Http.cpp
//
//	Simple HTTP Client Interface to download updated config files
//
//	Copyright (c) 2015 Justin Hammond <Justin@dynam.ac>
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

#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdio>

#include "Driver.h"
#include "Http.h"
#include "platform/HttpClient.h"
#include "platform/FileOps.h"
#include "Utils.h"

using namespace OpenZWave;

i_HttpClient::i_HttpClient
(
		Driver *driver
):
m_driver(driver)
{
};

void i_HttpClient::FinishDownload(HttpDownload *transfer) {
	/* send the response back to the Driver for processing */
	Driver::EventMsg *event = new Driver::EventMsg();
	event->type = Driver::EventMsg::Event_Http;
	event->event.httpdownload = transfer;
	this->m_driver->SubmitEventMsg(event);
}



HttpClient::HttpClient
(
		OpenZWave::Driver *drv
):
i_HttpClient(drv),
m_exitEvent( new Event() ),
m_httpThread ( new Thread( "HttpThread" ) ),
m_httpThreadRunning(false),
m_httpMutex ( new Mutex() ),
m_httpDownloadEvent ( new Event() )
{
}

HttpClient::~HttpClient
(
)
{
	m_exitEvent->Set();
}



bool HttpClient::StartDownload
(
		HttpDownload *transfer
)
{

	if (!m_httpThreadRunning)
		m_httpThread->Start(HttpClient::HttpThreadProc, this);

	LockGuard LG(m_httpMutex);
	switch (transfer->operation) {
		case HttpDownload::None:
			Log::Write(LogLevel_Warning, "Got a Transfer Type of NONE for %s", transfer->url.c_str());
			delete transfer;
			return false;
		case HttpDownload::File:
		case HttpDownload::Config:
		case HttpDownload::MFSConfig:
			/* make sure it has everything */
			if ((transfer->url.size() <= 0) || (transfer->filename.size() <= 0)) {
				Log::Write(LogLevel_Warning, "File Transfer had incomplete Params");
				delete transfer;
				return false;
			}

			/* make sure the Folder Exists */
			if (!FileOps::Create()->FolderExists(ozwdirname(transfer->filename))) {
				if (!FileOps::Create()->FolderCreate(ozwdirname(transfer->filename))) {
					Log::Write(LogLevel_Warning, "File Transfer Failed. Could not create Destination Folder: %s", ozwdirname(transfer->filename).c_str());
					delete transfer;
					return false;
				}
			}

			/* does the file exist, if so, rotate it out (by doing a copy) */
			if (FileOps::Create()->FileExists(transfer->filename)) {
				if (!FileOps::Create()->FileRotate(transfer->filename)) {
					Log::Write(LogLevel_Warning, "File Transfer Failed. Could not Rotate Existing File: %s", transfer->filename.c_str());
					delete transfer;
					return false;
				}
			}


			/* make sure the target file is writeable */
			if (!FileOps::Create()->FileWriteable(transfer->filename)) {
				Log::Write(LogLevel_Warning, "File %s is not writable", transfer->filename.c_str());
				delete transfer;
				return false;
			}
			break;
	}

	m_httpDownlist.push_back(transfer);
	m_httpDownloadEvent->Set();
	return true;
}
void HttpClient::HttpThreadProc
(
		Event* _exitEvent,
		void* _context
)
{
	HttpClient *client = (HttpClient *)_context;
	client->m_httpThreadRunning = true;

	SimpleHTTPClient::InitNetwork();
	bool keepgoing = true;
	while( keepgoing )
	{
		const uint32 count = 2;

		Wait* waitObjects[count];

		int32 timeout = Wait::Timeout_Infinite;
		timeout = 10000;

		waitObjects[0] = client->m_exitEvent;					// Thread must exit.
		waitObjects[1] = client->m_httpDownloadEvent;			// Http Request
		// Wait for something to do

		int32 res = Wait::Multiple( waitObjects, count, timeout );

		switch (res) {
			case -1: /* timeout */
				Log::Write(LogLevel_Info, "HttpThread Exiting. No Transfers in timeout period");
				keepgoing = false;
				break;
			case 0: /* exitEvent */
				Log::Write(LogLevel_Info, "HttpThread Exiting.");
				keepgoing = false;
				break;
			case 1: /* HttpEvent */
				HttpDownload *download;
				{
					LockGuard LG(client->m_httpMutex);
					download = client->m_httpDownlist.front();
					client->m_httpDownlist.pop_front();
					if (client->m_httpDownlist.empty())
						client->m_httpDownloadEvent->Reset();
				}
				Log::Write(LogLevel_Debug, "Download Starting for %s (%s)", download->url.c_str(), download->filename.c_str());
				SimpleHTTPClient::HttpSocket *ht = new SimpleHTTPClient::HttpSocket();
			    ht->SetKeepAlive(0);
			    ht->SetBufsizeIn(64 * 1024);
			    ht->SetDownloadFile(download->filename);
			    ht->Download(download->url);
			    while (ht->isOpen())
			    	ht->update();

			    if (ht->IsSuccess())
			    	download->transferStatus = HttpDownload::Ok;
			    else
			    	download->transferStatus = HttpDownload::Failed;
			    delete ht;
			    client->FinishDownload(download);
				break;
		}
	}
	SimpleHTTPClient::StopNetwork();
    client->m_httpThreadRunning = false;
}
