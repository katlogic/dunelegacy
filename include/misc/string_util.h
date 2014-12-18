/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <sstream>
#include <stdexcept>

bool splitString(const std::string& parseString, unsigned int numStringPointers,...);

std::vector<std::string> splitString(const std::string& parseString, const std::string& delim = ",", bool keepDelim = false);

std::string replaceAll(std::string str, const std::map<std::string, std::string>& replacementMap);

std::string strprintf(const std::string& fmt, ...);

template<typename T>
inline std::string stringify(T x)
{
    std::ostringstream os;
    if (!(os << x))
        throw std::runtime_error("stringify() failed!");
    return os.str();
}

template<typename T>
inline bool parseString(const std::string& str, T& t) {
    std::istringstream is(str);
    is >> t;
    return !is.fail();
}


inline void convertToLower(std::string& str) {
    std::transform(str.begin(),str.end(), str.begin(), (int(*)(int)) tolower);
}

inline std::string strToLower(const std::string& str) {
    std::string result = str;
    convertToLower(result);
    return result;
}

inline void convertToUpper(std::string& str) {
    std::transform(str.begin(),str.end(), str.begin(), (int(*)(int)) toupper);
}

inline std::string strToUpper(const std::string& str) {
    std::string result = str;
    convertToUpper(result);
    return result;
}

inline std::string trim(const std::string& str) {
    size_t firstChar = str.find_first_not_of(" \t");
    size_t lastChar = str.find_last_not_of(" \t");

    if((firstChar == std::string::npos) || (lastChar == std::string::npos)) {
        return "";
    } else {
        return str.substr(firstChar, lastChar - firstChar + 1);
    }
}

inline int stringCaseInsensitiveCompare(const std::string& str1, const std::string& str2) {
    const char* pStr1 = str1.c_str();
    const char* pStr2 = str2.c_str();

    while((*pStr1 != '\0') && (*pStr2 != '\0')) {
        char c1 = tolower(*pStr1);
        char c2 = tolower(*pStr2);

        if(c1 != c2) {
            return c1 - c2;
        }
    }

    return tolower(*pStr1) - tolower(*pStr2);
}

std::string convertCP850ToISO8859_1(const std::string& text);

std::string convertUTF8ToISO8859_1(const std::string& text);

/// This function decodes a string to CP850 Code.
/**
	The parameter text is decoded to CP850 Code and returned
	\param text	Text to decode
	\return	The decoded text
*/
std::string decodeString(std::string text);

#endif // STRING_UTIL_H
