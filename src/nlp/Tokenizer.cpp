/**
 * @file Tokenizer.cpp
 * @brief Implementation of tokenization and sentence segmentation.
 * @author Soubhi Khayat Najjar
 * @date 2026
 *
 * @note Token classification: dates (ISO, EU formats), numbers, or generic words.
 *       Sentence splitting uses punctuation (., !, ?) followed by a capital letter,
 *       avoiding splits on known abbreviations (Spanish abbreviations are hardcoded).
 *       UTF-8 aware lowercasing for Spanish accented characters.
 */

#include "Tokenizer.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <regex>
#include <set>

namespace {

// ---------------------------------------------------------------------------
// Token classification helpers
// ---------------------------------------------------------------------------

/**
 * @brief Checks if a token matches a recognised date format.
 * Supported formats:
 *   - ISO:       YYYY-MM-DD
 *   - EU:        DD/MM/YYYY
 *   - ISO-Slash: YYYY/MM/DD
 *   - EU-dot:    DD.MM.YYYY
 * Basic validation of month and day ranges is performed.
 */
bool isDateFormat(const std::string& token) {
    static const std::regex reIso(R"(^\d{4}-\d{1,2}-\d{1,2}$)");
    static const std::regex reEu(R"(^\d{1,2}/\d{1,2}/\d{4}$)");
    static const std::regex reIsoSlash(R"(^\d{4}/\d{1,2}/\d{1,2}$)");
    static const std::regex reEuDot(R"(^\d{1,2}\.\d{1,2}\.\d{4}$)");

    int y = 0, m = 0, d = 0;
    auto validMD = [](int month, int day) {
        return month >= 1 && month <= 12 && day >= 1 && day <= 31;
    };

    if (std::regex_match(token, reIso)) {
        std::sscanf(token.c_str(), "%d-%d-%d", &y, &m, &d);
        return validMD(m, d);
    }
    if (std::regex_match(token, reEu)) {
        std::sscanf(token.c_str(), "%d/%d/%d", &d, &m, &y);
        return validMD(m, d);
    }
    if (std::regex_match(token, reIsoSlash)) {
        std::sscanf(token.c_str(), "%d/%d/%d", &y, &m, &d);
        return validMD(m, d);
    }
    if (std::regex_match(token, reEuDot)) {
        std::sscanf(token.c_str(), "%d.%d.%d", &d, &m, &y);
        return validMD(m, d);
    }
    return false;
}

/**
 * @brief Checks whether a token is a valid number (using std::strtod).
 */
bool isNumber(const std::string& token) {
    char* end = nullptr;
    std::strtod(token.c_str(), &end);
    return (end == token.c_str() + token.size());
}

/**
 * @brief Classifies a token into TokenType.
 */
TokenType classifyToken(const std::string& token) {
    if (isDateFormat(token)) return TokenType::DATE;
    if (isNumber(token))     return TokenType::NUMBER;
    return TokenType::WORD;
}

// ---------------------------------------------------------------------------
// Character classification for token extraction
// ---------------------------------------------------------------------------

/**
 * @brief Determines if a character is part of a word token.
 * Allows alphanumeric, apostrophe, hyphen, and common punctuation that may
 * appear inside words (but strips them later if needed). Disallows quotes,
 * colons, brackets, etc. UTF-8 continuation bytes (>= 0x80) are accepted.
 */
bool isWordChar(unsigned char c) {
    // Exclude certain punctuation that breaks words.
    if (c == '"' || c == ':' || c == ';' || c == '\\' ||
        c == '(' || c == ')' || c == '[' || c == ']' ||
        c == '{' || c == '}' || c == '<' || c == '>') {
        return false;
    }
    // UTF-8 multi-byte characters
    if (c >= 0x80) return true;
    return std::isalnum(c) || c == '\'' || c == '-' || c == '/' || c == '.' ||
           c == ',' || c == '?' || c == '!';
}

/**
 * @brief Trims punctuation characters from both ends of a string.
 */
void trimPunctuation(std::string& text) {
    static const std::string punct = ".,;:!?¡¿\"()[]{}<>";
    while (!text.empty() && punct.find(text.back()) != std::string::npos) {
        text.pop_back();
    }
    while (!text.empty() && punct.find(text.front()) != std::string::npos) {
        text.erase(0, 1);
    }
}

// ---------------------------------------------------------------------------
// UTF-8 lowercasing (Spanish‑specific accents)
// ---------------------------------------------------------------------------

/**
 * @brief Converts a UTF-8 string to lowercase, handling Spanish accented vowels.
 * For non‑ASCII characters, only the two‑byte sequences starting with 0xC3 are
 * mapped; other multi‑byte sequences are copied unchanged.
 */
std::string toLowerUtf8(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            result.push_back(static_cast<char>(std::tolower(c)));
            ++i;
            continue;
        }
        // 2‑byte sequence (U+00C0 – U+00FF)
        if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
            unsigned char c2 = static_cast<unsigned char>(s[i + 1]);
            if (c == 0xC3) {
                switch (c2) {
                    case 0x80: result += "\xC3\xA0"; break; // À -> à
                    case 0x81: result += "\xC3\xA1"; break; // Á -> á
                    case 0x82: result += "\xC3\xA2"; break; // Â -> â
                    case 0x83: result += "\xC3\xA3"; break; // Ã -> ã
                    case 0x84: result += "\xC3\xA4"; break; // Ä -> ä
                    case 0x85: result += "\xC3\xA5"; break; // Å -> å
                    case 0x88: result += "\xC3\xA8"; break; // È -> è
                    case 0x89: result += "\xC3\xA9"; break; // É -> é
                    case 0x8A: result += "\xC3\xAA"; break; // Ê -> ê
                    case 0x8B: result += "\xC3\xAB"; break; // Ë -> ë
                    case 0x8C: result += "\xC3\xAC"; break; // Ì -> ì
                    case 0x8D: result += "\xC3\xAD"; break; // Í -> í
                    case 0x8E: result += "\xC3\xAE"; break; // Î -> î
                    case 0x8F: result += "\xC3\xAF"; break; // Ï -> ï
                    case 0x92: result += "\xC3\xB2"; break; // Ò -> ò
                    case 0x93: result += "\xC3\xB3"; break; // Ó -> ó
                    case 0x94: result += "\xC3\xB4"; break; // Ô -> ô
                    case 0x95: result += "\xC3\xB5"; break; // Õ -> õ
                    case 0x96: result += "\xC3\xB6"; break; // Ö -> ö
                    case 0x99: result += "\xC3\xB9"; break; // Ù -> ù
                    case 0x9A: result += "\xC3\xBA"; break; // Ú -> ú
                    case 0x9B: result += "\xC3\xBB"; break; // Û -> û
                    case 0x9C: result += "\xC3\xBC"; break; // Ü -> ü
                    case 0x91: result += "\xC3\xB1"; break; // Ñ -> ñ
                    default:   result += s.substr(i, 2); break;
                }
            } else {
                result += s.substr(i, 2);
            }
            i += 2;
            continue;
        }
        // 3‑byte sequence or other: copy as-is
        if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
            result += s.substr(i, 3);
            i += 3;
            continue;
        }
        // Fallback: copy the byte
        result += s[i];
        ++i;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Spanish abbreviation list (used for sentence splitting)
// ---------------------------------------------------------------------------

const std::set<std::string> kAbbreviations = {
    "dr", "dra", "sr", "sra", "srta", "srl", "d", "s", "v",
    "ej", "p.ej", "etc", "fig", "pág", "vol", "cap", "ed",
    "apdo", "c", "f", "j", "n", "p", "q", "rr", "ss", "vv"
};

/**
 * @brief Checks whether a lowercase word is a known abbreviation
 *        (prevents sentence splitting after it).
 */
bool isAbbreviation(const std::string& word) {
    return kAbbreviations.find(word) != kAbbreviations.end();
}

} // anonymous namespace

// ============================================================================
// Tokenization
// ============================================================================

std::vector<Token> tokenize(const std::string& input) {
    std::vector<Token> tokens;
    size_t i = 0;
    const size_t len = input.size();

    while (i < len) {
        // Skip whitespace
        while (i < len && std::isspace(static_cast<unsigned char>(input[i]))) {
            ++i;
        }
        if (i >= len) break;

        // Skip characters not allowed in a word
        if (!isWordChar(static_cast<unsigned char>(input[i]))) {
            ++i;
            continue;
        }

        // Extract a word candidate
        size_t start = i;
        while (i < len && isWordChar(static_cast<unsigned char>(input[i]))) {
            ++i;
        }
        std::string tokenText = input.substr(start, i - start);
        if (tokenText.empty()) continue;

        Token token;
        token.text = toLowerUtf8(tokenText);
        token.type = classifyToken(token.text);

        // For word tokens, trim leading/trailing punctuation
        if (token.type == TokenType::WORD) {
            trimPunctuation(token.text);
        }

        tokens.push_back(std::move(token));
    }
    return tokens;
}

// ============================================================================
// Sentence segmentation
// ============================================================================

std::vector<std::string> splitIntoSentences(const std::string& input) {
    std::vector<std::string> sentences;
    std::string current;
    size_t i = 0;
    const size_t n = input.size();

    while (i < n) {
        current += input[i];
        char c = input[i];

        // Check sentence-ending punctuation
        if (c == '.' || c == '!' || c == '?' || c == '\n') {
            // Find the word that precedes the punctuation
            size_t start = current.rfind(' ', current.size() - 2);
            if (start == std::string::npos) {
                start = 0;
            } else {
                ++start;
            }
            std::string word = current.substr(start, current.size() - start - 1);
            // Clean any trailing punctuation from the word
            while (!word.empty() && std::ispunct(static_cast<unsigned char>(word.back()))) {
                word.pop_back();
            }
            std::transform(word.begin(), word.end(), word.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

            // If it's an abbreviation, do not split
            if (!isAbbreviation(word)) {
                // Look ahead to decide if this truly ends a sentence:
                // the next non-whitespace character, if any, should be uppercase.
                size_t next = i + 1;
                while (next < n && (input[next] == ' ' || input[next] == '\t' || input[next] == '\r')) {
                    ++next;
                }
                if (next == n || std::isupper(static_cast<unsigned char>(input[next]))) {
                    sentences.push_back(current);
                    current.clear();
                    if (next == n) break;
                    i = next - 1; // loop increment will set i = next
                }
            }
        }
        ++i;
    }

    // Append any remaining text as a final sentence
    if (!current.empty()) {
        size_t start = current.find_first_not_of(" \t\r\n");
        if (start != std::string::npos) {
            size_t end = current.find_last_not_of(" \t\r\n");
            current = current.substr(start, end - start + 1);
            if (!current.empty()) {
                sentences.push_back(std::move(current));
            }
        }
    }
    return sentences;
}
