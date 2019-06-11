//-----------------------------------------------------------------------------
//
//	ValueID.cpp
//
//	Represents a Device Variable in OZW
//
//	Copyright (c) 2019 Justin Hammond <justin@dynam.ac>
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

#include "ValueID.h"
#include "Value.h"

namespace OpenZWave
{
	//-----------------------------------------------------------------------------
	// <ValueID::GetGenreAsString>
	// Get the Genre as a String
	//-----------------------------------------------------------------------------
	string ValueID::GetGenreAsString() const
	{
		return Internal::VC::Value::GetGenreNameFromEnum(GetGenre());
	}

	//-----------------------------------------------------------------------------
	// <ValueID::GetTypeAsString>
	// Get the Type as a String
	//-----------------------------------------------------------------------------
	string ValueID::GetTypeAsString() const
	{
		return Internal::VC::Value::GetTypeNameFromEnum(GetType());
	}

}// namespace OpenZWave
