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

#include <windows.h>
#include "FileOpsImpl.h"
#include "Utils.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{

//-----------------------------------------------------------------------------
//	<FileOpsImpl::FileOpsImpl>
//	Constructor
//-----------------------------------------------------------------------------
			FileOpsImpl::FileOpsImpl()
			{
			}

//-----------------------------------------------------------------------------
//	<FileOpsImpl::~FileOpsImpl>
//	Destructor
//-----------------------------------------------------------------------------
			FileOpsImpl::~FileOpsImpl()
			{
			}

//-----------------------------------------------------------------------------
//	<FileOpsImpl::FolderExists>
//	Determine if a folder exists
//-----------------------------------------------------------------------------
			bool FileOpsImpl::FolderExists(const string &_folderName)
			{
				uint32 ftype = GetFileAttributesA(_folderName.c_str());
				if (ftype == INVALID_FILE_ATTRIBUTES)
					return false;			// something is wrong with _foldername path
				if (ftype & FILE_ATTRIBUTE_DIRECTORY)
					return true;

				return false;
			}

			bool FileOpsImpl::FileExists(const string _filename)
			{
				DWORD dwAttrib = GetFileAttributesA(_filename.c_str());

				return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
			}

			bool FileOpsImpl::FileWriteable(const string _filename)
			{
				DWORD dwAttrib;
				if (!FileExists(_filename))
				{
					/* check if the directory is writtable */
					dwAttrib = GetFileAttributesA(ozwdirname(_filename).c_str());
				}
				else
				{
					dwAttrib = GetFileAttributesA(_filename.c_str());
				}

				return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_READONLY));

			}

			bool FileOpsImpl::FileRotate(const string _filename)
			{
				int i = 1;
				string newFile;
				/* find a filename not used yet */
				newFile = _filename;
				newFile.append(".").append(intToString(i));
				while (FileExists(newFile))
				{
					i++;
					newFile = _filename;
					newFile.append(".").append(intToString(i));
				}
				/* copy the file */
				if (!FileCopy(_filename, newFile))
				{
					Log::Write(LogLevel_Warning, "File Rotate Failed: %s -> %s", _filename.c_str(), newFile.c_str());
					return false;
				}
				/* remove the old file */
				if (DeleteFileA(_filename.c_str()) == 0)
				{
					Log::Write(LogLevel_Warning, "File Removal failed: %s", _filename.c_str());
					return false;
				}
				return true;
			}

			bool FileOpsImpl::FileCopy(const string _sourcefile, const string _destfile)
			{

				if (!FileExists(_sourcefile))
				{
					Log::Write(LogLevel_Warning, "Source File %s doesn't exist in FileCopy", _sourcefile.c_str());
					return false;
				}
				if (FileExists(_destfile))
				{
					Log::Write(LogLevel_Warning, "Destination File %s exists in FileCopy", _destfile.c_str());
					return false;
				}

				/* make sure the Destination Folder Exists */
				if (!FolderExists(ozwdirname(_destfile)))
				{
					Log::Write(LogLevel_Warning, "Destination Folder %s Doesn't Exist", ozwdirname(_destfile));
					return false;
				}

				if (CopyFileA(_sourcefile.c_str(), _destfile.c_str(), FALSE) == 0)
				{
					Log::Write(LogLevel_Warning, "CopyFile Failed %s - %s", _sourcefile.c_str(), _destfile.c_str());
					return false;
				}
				return true;
			}

			bool FileOpsImpl::FolderCreate(const string _dirname)
			{
				if (FolderExists(_dirname))
				{
					Log::Write(LogLevel_Warning, "Folder %s Exists for FolderCreate", _dirname.c_str());
					return false;
				}
				if (CreateDirectoryA(_dirname.c_str(), NULL) == 0)
				{
					Log::Write(LogLevel_Warning, "Create Directory Failed: %s", _dirname.c_str());
					return false;
				}
				return true;
			}
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave
