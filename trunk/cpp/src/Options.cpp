//-----------------------------------------------------------------------------
//
//	Options.h
//
//	Program options read from XML files or the command line.
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

#include <algorithm> 
#include <string>  

#include "Defs.h"
#include "Options.h"
#include "Utils.h"
#include "Manager.h"
#include "Log.h"
#include "tinyxml.h"

using namespace OpenZWave;

Options* Options::s_instance = NULL;

//-----------------------------------------------------------------------------
// <Options::Create>
// Static method to create an Options object
//-----------------------------------------------------------------------------
Options* Options::Create
( 
	string const& _configPath,
	string const& _userPath,
	string const& _commandLine
)
{
	if( s_instance == NULL )
	{
		s_instance = new Options( _configPath, _userPath, _commandLine );

		// Add the default options
		s_instance->AddOptionString(	"ConfigPath",			_configPath,	false );	// Path to the OpenZWave config folder.
		s_instance->AddOptionString(	"UserPath",				_userPath,		false );	// Path to the user's data folder.
		s_instance->AddOptionBool(		"Logging",				true );						// Enable logging of library activity.
		s_instance->AddOptionBool(		"Associate",			true );						// Enable automatic association of the controller with group one of every device.
		s_instance->AddOptionString(	"Exclude",				string(""),		true );		// Remove support for the listed command classes.
		s_instance->AddOptionString(	"Include",				string(""),		true );		// Only handle the specified command classes.  The Exclude option is ignored if anything is listed here.
		s_instance->AddOptionBool(		"NotifyTransactions",	false );					// Notifications when transaction complete is reported.
	}

	return s_instance;
}

//-----------------------------------------------------------------------------
// <Options::Create>
// Static method to create an Options object
//-----------------------------------------------------------------------------
bool Options::Destroy
( 
)
{
	if( Manager::Get() )
	{
		// Cannot delete Options because Manager object still exists
		assert(0);
		return false;
	}

	delete s_instance;
	s_instance = NULL;

	return true;
}

//-----------------------------------------------------------------------------
// <Options::Options>
// Destructor
//-----------------------------------------------------------------------------
Options::Options
(
	string const& _configPath,
	string const& _userPath,
	string const& _commandLine
):
	m_locked( false ),
	m_xml( _userPath + "options.xml" ),
	m_commandLine( _commandLine )
{
}

//-----------------------------------------------------------------------------
// <Options::Options>
// Destructor
//-----------------------------------------------------------------------------
Options::~Options
(
)
{
	// Clear the options map
	while( !m_options.empty() )
	{
		map<string,Option*>::iterator it = m_options.begin();
		delete it->second;
		m_options.erase( it );
	}
}

//-----------------------------------------------------------------------------
// <Options::AddOptionBool>
// Add a boolean option.
//-----------------------------------------------------------------------------
bool Options::AddOptionBool
( 
	string const& _name,
	bool const _default
)
{
	if( m_locked )
	{
		// Options are final, no more may be added
		assert(0);
		return false;
	}

	if( Find( _name ) )
	{
		// Option already exists
		return false;
	}

	Option* option	= new Option( _name, _default );
	string lowerName = ToLower( _name );

	m_options[lowerName] = option;
	return true;
}

//-----------------------------------------------------------------------------
// <Options::AddOptionInt>
// Add an integer option.
//-----------------------------------------------------------------------------
bool Options::AddOptionInt
(
	string const& _name,
	int32 const _default
)
{
	if( m_locked )
	{
		// Options are final, no more may be added
		assert(0);
		return false;
	}

	if( Find( _name ) )
	{
		// Option already exists
		return false;
	}

	Option* option	= new Option( _name, _default );
	string lowerName = ToLower( _name );

	m_options[lowerName] = option;
	return true;
}

//-----------------------------------------------------------------------------
// <Options::AddOptionString>
// Add a string option.
//-----------------------------------------------------------------------------
bool Options::AddOptionString
( 
	string const& _name,
	string const& _default,
	bool const _append
)
{
	if( m_locked )
	{
		// Options are final, no more may be added
		assert(0);
		return false;
	}

	if( Find( _name ) )
	{
		// Option already exists
		return false;
	}

	Option* option	= new Option( _name, _default, _append );
	string lowerName = ToLower( _name );

	m_options[lowerName] = option;
	return true;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionAsBool>
// Get the value of a boolean option.
//-----------------------------------------------------------------------------
bool Options::GetOptionAsBool
( 
	string const& _name, 
	bool* o_value 
)
{
	Option* option = Find( _name );
	if( o_value && option && ( OptionType_Bool == option->m_type ) )
	{
		*o_value = option->m_valueBool;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionAsInt>
// Get the value of an integer option.
//-----------------------------------------------------------------------------
bool Options::GetOptionAsInt
( 
	string const& _name, 
	int32* o_value 
)
{
	Option* option = Find( _name );
	if( o_value && option && ( OptionType_Int == option->m_type ) )
	{
		*o_value = option->m_valueInt;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionAsString>
// Get the value of a string option.
//-----------------------------------------------------------------------------
bool Options::GetOptionAsString
( 
	string const& _name, 
	string* o_value 
)
{
	Option* option = Find( _name );
	if( o_value && option && ( OptionType_String == option->m_type ) )
	{
		*o_value = option->m_valueString;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionType>
// Get the type of value stored in an option.
//-----------------------------------------------------------------------------
Options::OptionType Options::GetOptionType
( 
	string const& _name
)
{
	Option* option = Find( _name );
	if( option )
	{
		return option->m_type;
	}

	// Option not found
	return OptionType_Invalid;
}

//-----------------------------------------------------------------------------
// <Options::Lock>
// Read all the option XMLs and Command Lines, and lock their values.
//-----------------------------------------------------------------------------
bool Options::Lock
(
)
{
	if( m_locked )
	{
		// Options are already final
		assert(0);
		return false;
	}

	ParseOptionsXML( m_xml );
	ParseOptionsString( m_commandLine );
	m_locked = true;

	return true;
}

//-----------------------------------------------------------------------------
// <Options::ParseOptionsString>
// Parse a string containing program options, such as a command line
//-----------------------------------------------------------------------------
bool Options::ParseOptionsString
(
	string const& _commandLine
)
{
	bool res = true;

	int pos = 0;
	int start = 0;
	while( 1 )
	{
		pos = _commandLine.find_first_of( "--", start );
		if( string::npos == pos )
		{
			break;
		}
		start = pos + 2;

		// found an option.  Get the name.
		string optionName;
		pos = _commandLine.find( " ", start );
		if( string::npos == pos )
		{
			optionName = _commandLine.substr( start );
			start = pos;
		}
		else
		{
			optionName = _commandLine.substr( start, pos-start );
			start = pos + 1;
		}

		// Find the matching option object
		Option* option = Find( optionName );
		if( option )
		{
			// Read the values
			int numValues = 0;
			bool parsing = true;
			while( parsing )
			{
				string value;
				pos = _commandLine.find( " ", start );
				if( string::npos == pos )
				{
					// Last value in string
					value = _commandLine.substr( start );
					parsing = false;
					start = pos;
				}
				else
				{
					value = _commandLine.substr( start, pos-start );
					start = pos+1;
				}

				if( !value.compare( 0, 2, "--" ) )
				{
					// Value is actually the next option.
					if( !numValues )
					{
						// No values were read for this option
						// This is ok only for bool options, where we assume no value means "true".
						if( OptionType_Bool == option->m_type )
						{
							option->m_valueBool = true;
						}
						else
						{
							res = false;
						}
					}
				}
				else if( value.size() > 0 )
				{
					// Set the value
					option->SetValueFromString( value );
				}
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Options::ParseOptionsXML>
// Parse an XML file containing program options
//-----------------------------------------------------------------------------
bool Options::ParseOptionsXML
(
	string const& _filename
)
{
	TiXmlDocument doc;
	if( !doc.LoadFile( _filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		return false;
	}

	TiXmlElement const* optionsElement = doc.RootElement();

	// Read the options
	TiXmlElement const* optionElement = optionsElement->FirstChildElement();
	while( optionElement )
	{
		char const* str = optionElement->Value();
		if( str && !strcmp( str, "Option" ) )
		{
			char const* name = optionElement->Attribute( "name" );
			if( name )
			{
				Option* option = Find( name );
				if( option )
				{
					char const* value = optionElement->Attribute( "value" );
					if( value )
					{	
						// Set the value
						option->SetValueFromString( value );
					}
				}
			}
		}

		optionElement = optionElement->NextSiblingElement();
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Options::Find>
// Find an option by name
//-----------------------------------------------------------------------------
Options::Option* Options::Find
(
	string const& _name
)
{
	string lowername = ToLower( _name );
	map<string,Option*>::iterator it = m_options.find( lowername );
	if( it != m_options.end() )
	{
		return it->second;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Options::Option::SetValueFromString>
// Find an option by name
//-----------------------------------------------------------------------------
bool Options::Option::SetValueFromString
(
	string const& _value
)
{
	if( OptionType_Bool == m_type )
	{
		string lowerValue = ToLower( _value );
		if( ( lowerValue == "true" ) || ( lowerValue == "1" ) )
		{
			m_valueBool = true;
			return true;
		}

		if( ( lowerValue == "false" ) || ( lowerValue == "0" ) )
		{
			m_valueBool = false;
			return true;
		}

		return false;
	}

	if( OptionType_Int == m_type )
	{
		m_valueInt = (int32)atol( _value.c_str() );
		return true;
	}

	if( OptionType_String == m_type )
	{
		if( m_append && ( m_valueString.size() > 0 ) )
		{
			m_valueString += ( string(",") + _value );	
		}
		else
		{
			m_valueString = _value;
		}
		return true;
	}

	return false;
}
