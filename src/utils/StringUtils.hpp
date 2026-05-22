// StringUtils.hpp
#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>
#include "../common/types.hpp"

namespace StringUtils {
    std::string toLowerNoAccentsSafe(const std::string& s);
    bool isDateFormat(const std::string& token);
    bool isNumber(const std::string& token);
    bool isEmail(const std::string& token);
    bool isMoney(const std::string& token);
    bool isPhone(const std::string& token);
    bool validateValue(const std::string& value, TokenType type);
    bool isWordChar(unsigned char c);
    void trimPunctuation(std::string& text);
    std::string toLowerUtf8(const std::string& s);
    bool isAbbreviation(const std::string& word);

}

#endif
