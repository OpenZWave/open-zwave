//-----------------------------------------------------------------------------
//
//	Utils.h
//
//	Miscellaneous helper functions
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

#include "Defs.h"
#include "Utils.h"
#include <functional>

namespace OpenZWave
{
	namespace Internal
	{

//-----------------------------------------------------------------------------
// <OpenZWave::ToUpper>
// Convert a string to all upper-case.
//-----------------------------------------------------------------------------
		std::string ToUpper(std::string const& _str)
		{
			std::string upper = _str;
			transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
			return upper;
		}

//-----------------------------------------------------------------------------
// <OpenZWave::ToLower>
// Convert a string to all lower-case.
//-----------------------------------------------------------------------------
		std::string ToLower(std::string const& _str)
		{
			std::string lower = _str;
			transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
			return lower;
		}

//-----------------------------------------------------------------------------
// <OpenZWave::trim>
// Remove WhiteSpaces from the begining and end of a string
//-----------------------------------------------------------------------------

		std::string &removewhitespace(std::string &s)
		{
			if (s.size() == 0)
			{
				return s;
			}

			int val = 0;
			for (size_t cur = 0; cur < s.size(); cur++)
			{
				if (s[cur] != ' ' && isalnum(s[cur]))
				{
					s[val] = s[cur];
					val++;
				}
			}
			s.resize(val);
			return s;
		}

		std::string& ltrim(std::string& s)
		{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c)
			{	return isgraph(c);}));
			return s;
		}

		std::string& rtrim(std::string& s)
		{
			s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c)
			{	return isgraph(c);}).base(), s.end());
			return s;
		}

		std::string& trim(std::string& s)
		{
			return Internal::ltrim(Internal::rtrim(s));
		}
//-----------------------------------------------------------------------------
// <OpenZWave::split>
// Split a String into a vector, seperated by anything specified in seperators.
//-----------------------------------------------------------------------------
		void split(std::vector<std::string>& lst, const std::string& input, const std::string& separators, bool remove_empty)
		{
			std::ostringstream word;
			for (size_t n = 0; n < input.size(); ++n)
			{
				if (std::string::npos == separators.find(input[n]))
					word << input[n];
				else
				{
					if (!word.str().empty() || !remove_empty)
						lst.push_back(word.str());
					word.str("");
				}
			}
			if (!word.str().empty() || !remove_empty)
				lst.push_back(word.str());
		}

		void PrintHex(std::string prefix, uint8_t const *data, uint32 const length)
		{
			Log::Write(LogLevel_Info, "%s: %s", prefix.c_str(), PktToString(data, length).c_str());
		}

		string PktToString(uint8 const *data, uint32 const length)
		{
			char byteStr[5];
			std::string str;
			for (uint32 i = 0; i < length; ++i)
			{
				if (i)
				{
					str += ", ";
				}

				snprintf(byteStr, sizeof(byteStr), "0x%.2x", data[i]);
				str += byteStr;
			}
			return str;

		}

		static const char* separators()
		{
#if __unix__
			return "/";
#else // __unix__
			return "\\/";
#endif // __unix__
		}

		string ozwdirname(string m_path)
		{
			const size_t lastSlash = m_path.find_last_of(separators());
			if (lastSlash == std::string::npos)
				return "";

			return m_path.substr(0, lastSlash);
		}

		string intToString(int x)
		{
#if __cplusplus==201103L || __APPLE__
			return to_string(x);
#else
			return static_cast< std::ostringstream & >( ( std::ostringstream() << std::dec << x ) ).str();
#endif
		}

		const char* rssi_to_string(uint8 _data)
		{
			static char buf[8];

			switch (_data)
			{
				case 127:
				{
					return "---";
					break;
				}
				case 126:
				{
					return "MAX";
					break;
				}
				case 125:
				{
					return "MIN";
					break;
				}
				default:
					if (_data >= 11 && _data <= 124)
					{
						return "UNK";
					}
					else
					{
						snprintf(buf, 5, "%4d", (unsigned int) _data - 256);
						return buf;
					}
			}
		}
	} // namespace Internal
} // namespace OpenZWave

#if (defined _WINDOWS || defined WIN32 || defined _MSC_VER) && (!defined MINGW && !defined __MINGW32__ && !defined __MINGW64__)

/* Windows doesn't have localtime_r - use the "secure" version instead */
struct tm *localtime_r(const time_t *_clock, struct tm *_result)
{
	_localtime64_s(_result, _clock);
	return _result;
}
#endif

