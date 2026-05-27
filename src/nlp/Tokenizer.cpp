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
#include "../utils/StringUtils.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <regex>
#include <set>



/**
 * @brief Classifies a token into TokenType.
 */
TokenType classifyToken(const std::string& token) {
    if (StringUtils::isDateFormat(token)) return TokenType::DATE;
    if (StringUtils::isNumber(token))     return TokenType::NUMBER;
    if (StringUtils::isEmail(token))      return TokenType::EMAIL;
    if (StringUtils::isMoney(token))      return TokenType::MONEY;
    if (StringUtils::isPhone(token))      return TokenType::PHONE;
    return TokenType::WORD;
}
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
        if (!StringUtils::isWordChar(static_cast<unsigned char>(input[i]))) {
            ++i;
            continue;
        }

        // Extract a word candidate
        size_t start = i;
        while (i < len && StringUtils::isWordChar(static_cast<unsigned char>(input[i]))) {
            ++i;
        }
        std::string tokenText = input.substr(start, i - start);
        if (tokenText.empty()) continue;

        Token token;
        token.text = StringUtils::toLowerUtf8(tokenText);
        token.type = classifyToken(token.text);

        // For word tokens, trim leading/trailing punctuation
        if (token.type == TokenType::WORD && !StringUtils::isAbbreviation(token.text)) {
            StringUtils::trimPunctuation(token.text);
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
            if (!StringUtils::isAbbreviation(word)) {
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
