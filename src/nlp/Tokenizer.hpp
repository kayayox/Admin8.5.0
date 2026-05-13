/**
 * @file Tokenizer.hpp
 * @brief Text tokenization into words/numbers/dates and sentence segmentation.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_TOKENIZER_HPP
#define ADMIN850_TOKENIZER_HPP

#include "../common/types.hpp"
#include <string>
#include <vector>

/**
 * @struct Token
 * @brief A single token with its text and classified type.
 */
struct Token {
    std::string text;   ///< The token string (lowercased for words).
    TokenType   type;   ///< Classified token type (WORD, NUMBER, DATE).
};

/**
 * @brief Tokenizes an input string into a sequence of tokens.
 * @param input The raw text to tokenize.
 * @return A vector of Token objects.
 */
std::vector<Token> tokenize(const std::string& input);

/**
 * @brief Splits a text into sentences based on punctuation and capitalisation.
 * @param input The raw text.
 * @return A vector of sentence strings.
 */
std::vector<std::string> splitIntoSentences(const std::string& input);

#endif // ADMIN850_TOKENIZER_HPP
