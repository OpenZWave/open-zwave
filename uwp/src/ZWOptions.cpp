#include "pch.h"
#include "ZWOptions.h"

using namespace OpenZWaveUWP;
using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<ZWOptions::Create>
//	Create the unmanaged Options singleton object
//-----------------------------------------------------------------------------
void ZWOptions::Create
(
	String^ _configPath,
	String^	_userPath,
	String^	_commandLine
)
{
	// Create the Manager singleton
	std::string config = ConvertString(_configPath);
	std::string user = ConvertString(_userPath);
	std::string command = ConvertString(_commandLine);
	
	Options::Create(config, user, command);
}

//-----------------------------------------------------------------------------
// <ZWOptions::AddOptionBool>
// Add a boolean option to the program
//-----------------------------------------------------------------------------
bool ZWOptions::AddOptionBool
(
	String^ _name,
	bool _default
)
{
	std::string name = ConvertString(_name);
	return Options::Get()->AddOptionBool(name, _default);
}

//-----------------------------------------------------------------------------
// <ZWOptions::AddOptionInt>
// Add an integer option to the program
//-----------------------------------------------------------------------------
bool ZWOptions::AddOptionInt
(
	String^ _name,
	int32 _default
)
{
	std::string name = ConvertString(_name);
	return Options::Get()->AddOptionInt(name, _default);
}

//-----------------------------------------------------------------------------
// <ZWOptions::AddOptionString>
// Add a string option to the program
//-----------------------------------------------------------------------------
bool ZWOptions::AddOptionString
(
	String^ _name,
	String^ _default,
	bool _append
)
{
	std::string name = ConvertString(_name);
	std::string defaultStr = ConvertString(_default);
	return Options::Get()->AddOptionString(name, defaultStr, _append);
}

//-----------------------------------------------------------------------------
// <ZWOptions::GetOptionAsBool>
// Gets the value of a boolean option
//-----------------------------------------------------------------------------
bool ZWOptions::GetOptionAsBool
(
	String^ _name,
	bool *o_value
)
{
	bool value;
	std::string name = ConvertString(_name);
	if (Options::Get()->GetOptionAsBool(name, &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWOptions::GetOptionAsInt>
// Gets the value of an integer option
//-----------------------------------------------------------------------------
bool ZWOptions::GetOptionAsInt
(
	String^ _name,
	int *o_value
)
{
	int32 value;
	std::string name = ConvertString(_name);
	if (Options::Get()->GetOptionAsInt(name, &value))
	{
		*o_value = value;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWOptions::GetOptionAsString>
// Gets the value of a string option
//-----------------------------------------------------------------------------
bool ZWOptions::GetOptionAsString
(
	String^ _name,
	String^ *o_value
)
{
	std::string value;
	std::string name = ConvertString(_name);
	if (Options::Get()->GetOptionAsString(name, &value))
	{
		*o_value = ConvertString(value);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ZWOptions::GetOptionType>
// Gets the type of the value stored by the option
//-----------------------------------------------------------------------------
ZWOptionType ZWOptions::GetOptionType
(
	String^ _name
)
{
	std::string name = ConvertString(_name);
	return (ZWOptionType)Options::Get()->GetOptionType(name);
}