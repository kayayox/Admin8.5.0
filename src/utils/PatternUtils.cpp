/**==============================================================================
    Admin8.2.1 - PatternUtils.cpp
    Purpose: Implementation of pattern serialization.
    Author: Soubhi Khayat Najjar
    Year: 2026
==============================================================================*/

#include "PatternUtils.hpp"
#include <sstream>
#include <iostream>

std::string serializePattern(const WordPattern& pat) {
    std::stringstream ss;
    for (auto it = pat.begin(); it != pat.end(); ++it) {
        if (it != pat.begin()) ss << ";";
        ss << it->first << ":" << it->second;
    }
    return ss.str();
}

WordPattern deserializePattern(const std::string& serialized) {
    WordPattern pat;
    if (serialized.empty()) return pat;    // Avoid processing empty string

    std::stringstream ss(serialized);
    std::string item;
    while (std::getline(ss, item, ';')) {
        size_t colon = item.find(':');
        if (colon != std::string::npos) {
            std::string word = item.substr(0, colon);
            std::string weightStr = item.substr(colon + 1);
            float weight = 0.0f;
            try {
                weight = std::stof(weightStr);
            } catch (const std::invalid_argument& e) {
                std::cerr << "[ERROR] PatternUtils: Non-numeric value while deserializing: '"
                          << weightStr << "' for word '" << word << "'" << std::endl;
                continue;       // Skip this corrupt element
            } catch (const std::out_of_range& e) {
                std::cerr << "[ERROR] PatternUtils: Number out of range: '"
                          << weightStr << "' for word '" << word << "'" << std::endl;
                continue;
            }
            pat[word] = weight;
        } else {
            // Skip this corrupt element
            std::cerr << "[ERROR] PatternUtils: Invalid format in item: '" << item << "'" << std::endl;
        }
    }
    return pat;
}
