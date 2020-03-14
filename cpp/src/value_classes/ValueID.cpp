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
#include <sstream>
#include <iomanip>


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

	string const ValueID::GetAsString() const
	{
		// Match constructor order
		// ValueID(uint32 const _homeId, uint8 const _nodeId, ValueGenre const _genre, uint8 const _commandClassId,
		// uint8 const _instance, uint16 const _valueIndex, ValueType const _type)
		std::ostringstream s;

		s
			<< "HomeID: 0x" << hex << setfill('0') << setw(8) << GetHomeId()
			<< ", ValueID: (Id 0x" << setw(16) << GetId() << dec << setfill(' ')
			<< ", NodeID " << static_cast<unsigned int>(GetNodeId())
			<< ", Genre " << GetGenreAsString()
			<< ", CC 0x" << hex << setfill('0') << setw(2) << static_cast<unsigned int>(GetCommandClassId()) << dec << setfill(' ')
			<< ", Instance " << static_cast<unsigned int>(GetInstance())
			<< ", Index " << static_cast<unsigned int>(GetIndex())
			<< ", Type " << GetTypeAsString() << ')';
		return s.str();
	}
}// namespace OpenZWave
