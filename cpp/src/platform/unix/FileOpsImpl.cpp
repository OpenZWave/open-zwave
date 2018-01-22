//-----------------------------------------------------------------------------
//
//	FileOpsImpl.cpp
//
//	Unix implementation of file operations
//
//	Copyright (c) 2012, Greg Satz <satz@iranger.com>
//	All rights reserved.
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

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>

#include "FileOpsImpl.h"
#include "Utils.h"

using namespace OpenZWave;
using std::ios_base;

//-----------------------------------------------------------------------------
//	<FileOpsImpl::FileOpsImpl>
//	Constructor
//-----------------------------------------------------------------------------
FileOpsImpl::FileOpsImpl
(
)
{
}

//-----------------------------------------------------------------------------
//	<FileOpsImpl::~FileOpsImpl>
//	Destructor
//-----------------------------------------------------------------------------
FileOpsImpl::~FileOpsImpl
(
)
{
}

//-----------------------------------------------------------------------------
//	<FileOpsImpl::FolderExists>
//	Determine if a folder exists
//-----------------------------------------------------------------------------
bool FileOpsImpl::FolderExists
( 
	const string _folderName
)
{
	DIR *dirp = opendir( _folderName.c_str() );
	if( dirp != NULL )
	{
		closedir( dirp );
		return true;
	}
	else
		return false;
}

bool FileOpsImpl::FileExists
(
	const string _filename
)
{
	  struct stat buffer;
	  return (stat (_filename.c_str(), &buffer) == 0);
}

bool FileOpsImpl::FileWriteable
(
	const string _filename
)
{
	if (!FileExists(_filename)) {
		string fn = ozwdirname(_filename);
		return (access(fn.c_str(), W_OK|F_OK) == 0);
	}

	return (access(_filename.c_str(), W_OK|F_OK) == 0);

}

bool FileOpsImpl::FileRotate
(
	const string _filename
)
{
	int i = 1;
	string newFile;
	/* find a filename not used yet */
	newFile = _filename;
	newFile.append(".").append(intToString(i));
	while (FileExists(newFile)) {
		i++;
		newFile = _filename;
		newFile.append(".").append(intToString(i));
	}
	/* copy the file */
	if (!FileCopy(_filename, newFile)) {
		Log::Write(LogLevel_Warning, "File Rotate Failed: %s -> %s", _filename.c_str(), newFile.c_str());
		return false;
	}

	/* remove the old file */
	if ( remove(_filename.c_str())) {
		Log::Write(LogLevel_Warning, "File Removal failed: %s", _filename.c_str());
		return false;
	}
	return true;
}


bool FileOpsImpl::FileCopy
(
	const string _sourcefile,
	const string _destfile
)
{

	if (!FileExists(_sourcefile)) {
		Log::Write(LogLevel_Warning, "Source File %s doesn't exist in FileCopy", _sourcefile.c_str());
		return false;
	}
	if (FileExists(_destfile)) {
		Log::Write(LogLevel_Warning, "Destination File %s exists in FileCopy", _destfile.c_str());
		return false;
	}

	/* make sure the Destination Folder Exists */
	if (!FolderExists(ozwdirname(_destfile))) {
		Log::Write(LogLevel_Warning, "Destination Folder %s Doesn't Exist", ozwdirname(_destfile).c_str());
		return false;
	}


	std::ifstream in(_sourcefile.c_str(), ios_base::in | ios_base::binary);
	std::ofstream out(_destfile.c_str(), ios_base::out | ios_base::binary);
	char buf[COPY_BUF_SIZE];

	do {
		in.read(&buf[0], COPY_BUF_SIZE);
		out.write(&buf[0], in.gcount());
	} while (in.gcount() > 0);
	in.close();
	out.close();
	return true;
}

bool FileOpsImpl::FolderCreate
(
	const string _dirname
)
{
	if (FolderExists(_dirname)) {
		Log::Write(LogLevel_Warning, "Folder %s Exists for FolderCreate", _dirname.c_str());
		return false;
	}
	int ret = mkdir(_dirname.c_str(), 0777);
	if (ret == 0)
		return true;
	Log::Write(LogLevel_Warning, "Create Directory Failed: %s - %s", _dirname.c_str(), strerror(errno));
	return false;
}


