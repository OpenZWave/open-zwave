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

#include "Http.h"


using namespace OpenZWave;





HttpClient::HttpClient
(
)
{
std::cout << "HttpClient Constructed" << std::endl;
}

HttpClient::~HttpClient
(
)
{
	std::cout << "HttpClient Destoryed" << std::endl;

}

bool HttpClient::StartDownload
(
string url
)
{
	std::cout << "Downloading " << url << std::endl;
	return true;
}
