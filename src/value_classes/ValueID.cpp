//-----------------------------------------------------------------------------
//
//	ValueID.cpp
//
//	Unique identifier for a value object
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "tinyxml.h"
#include "ValueID.h"
#include "Msg.h"
#include "Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueID::ValueID>
// Constructor (from string)
//-----------------------------------------------------------------------------
ValueID::ValueID
(
	string const& _id
)
{
	char* pStop;
	m_id = strtol( _id.c_str(), &pStop, 16 ); 
}

//-----------------------------------------------------------------------------
// <ValueID::ToString>
// Convert a value ID to string form
//-----------------------------------------------------------------------------
string ValueID::ToString
(
)const
{
	char str[16];
	snprintf( str, "0x%.8x", m_id );
	return str;
}

