/**==============================================================================
    Admin8.2.1 - PatternUtils.hpp
    Purpose: Serialization and deserialization of Pattern objects (map<string, float>)
             for database storage.
    Author: Soubhi Khayat Najjar
    Year: 2026
==============================================================================*/

#ifndef PATTERN_UTILS_HPP
#define PATTERN_UTILS_HPP

#include <map>
#include <string>

using WordPattern = std::map<std::string, float>;

std::string serializePattern(const WordPattern& pat);
WordPattern deserializePattern(const std::string& serialized);

#endif
