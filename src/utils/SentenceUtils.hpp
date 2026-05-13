/**
 * @file SentenceUtils.hpp
 * @brief Utility functions for sentence analysis, building, and creative transformations.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_SENTENCE_UTILS_HPP
#define ADMIN850_SENTENCE_UTILS_HPP

#include "../core/Pattern.hpp"   // for PatternType
#include "../core/Sentence.hpp"  // for Sentence, Block
#include <string>
#include <vector>

namespace utils {

/**
 * @struct ParsedPremise
 * @brief Contains the extracted subject, verb, object, keywords, and pattern type of a premise.
 */
struct ParsedPremise {
    std::string              subject = "";
    std::string              verb = "";
    std::string              object = "";
    std::vector<std::string> keywords;
    PatternType              patternType = PatternType::SENTENCES;
};

/**
 * @brief Analyses a Sentence and extracts its subject, verb, object, keywords, and pattern type.
 * @param premise The sentence to parse.
 * @return A ParsedPremise with the extracted information.
 */
ParsedPremise parsePremise(const Sentence& premise);

/**
 * @brief Builds a Sentence from a raw text string using tokenisation and WordRepository.
 * @param text The input text.
 * @return A Sentence object (words may have types loaded from DB).
 */
Sentence buildSentenceFromText(const std::string& text);

/**
 * @brief Applies basic creativity transformations (synonym replacement, doubt prefix) to a string.
 * @param text        The text to modify (in/out).
 * @param creativity  A value between 0 and 1; higher means more creativity.
 */
void applyCreativity(std::string& text, float creativity);

/**
 * @brief Applies advanced creative transformations (filler words, punctuation, type conversion).
 * @param text         The text to modify (in/out).
 * @param creativity   Creativity level (0‑1).
 * @param premiseInfo  Optional premise metadata (may influence transformation).
 */
void advancedCreativeTransform(std::string& text, float creativity,
                               const ParsedPremise& premiseInfo);

/**
 * @brief Computes a creativity score between a premise and a hypothesis.
 *        A value near 0 means similar; near 1 means very different/creative.
 * @param premise    The original premise sentence.
 * @param hypothesis The generated hypothesis sentence.
 * @param pattern    The pattern used (unused for now).
 * @return A creativity score in [0,1].
 */
float computeCreativity(const Sentence& premise, const Sentence& hypothesis,
                        const Pattern& pattern);

/**
 * @brief Convenience wrapper that returns a modified copy of the text.
 * @param text       Original text.
 * @param creativity Creativity level.
 * @return Transformed text.
 */
std::string applyCreativityToText(const std::string& text, float creativity);

} // namespace utils

#endif // ADMIN850_SENTENCE_UTILS_HPP
