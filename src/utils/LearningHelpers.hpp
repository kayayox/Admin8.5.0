/**
 * @file LearningHelpers.hpp
 * @brief Helper functions for learning text into contextual and pattern correlators.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_LEARNING_HELPERS_HPP
#define ADMIN850_LEARNING_HELPERS_HPP

#include "../db/WordRepository.hpp"
#include "../dialogue/ContextualCorrelator.hpp"
#include "../dialogue/PatternCorrelator.hpp"
#include "../dialogue/LetterCorrelator.hpp"
#include "../dialogue/SyllableCorrelator.hpp"
#include <string>
#include <vector>

/**
 * @brief Builds an unclassified Word vector from raw text (tokenizes and sets DATE/NUMERAL types).
 * @param words Output vector to fill.
 * @param input Raw input text.
 * @return True if at least one token was created.
 */
bool createWordVector(std::vector<Word>& words, const std::string& input);

/**
 * @brief Overload that returns the vector by value.
 */
std::vector<Word> createWordVector(const std::string& input);

/**
 * @brief Trains a ContextualCorrelator with up to two preceding words.
 */
void learnContextual(ContextualCorrelator& ctx, const std::string& text);

/**
 * @brief Trains a ContextualCorrelator with direct (no‑context) associations.
 */
void learnDirect(ContextualCorrelator& ctx, const std::string& text);

/**
 * @brief Combines pattern learning and contextual learning on the same text.
 */
void learnTextWithContext(ContextualCorrelator& ctx, PatternCorrelator& corr, const std::string& text);

void learnLetterCorrelations(LetterCorrelator& lttCorr, const std::string& text);

void learnSyllableCorrelations(SyllableCorrelator& sllCorr, const std::string& text);

/** @brief Clears the console input buffer. */
void clearInputBuffer();

/** @brief Trims leading and trailing whitespace from a string. */
void trimString(std::string& line);

std::string formatTimestamp(uint32_t ts);

std::vector<int> getcomparetime(uint32_t ts);

#endif // ADMIN850_LEARNING_HELPERS_HPP
