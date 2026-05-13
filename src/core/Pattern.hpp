/**
 * @file Pattern.hpp
 * @brief Represents a grammatical pattern as a sequence of word types with its
 *        classification (affirmative, negative, interrogative, etc.).
 * @author Soubhi Khayat Najjar
 * @date 2026
 * @note No persistence dependencies. Pattern classification uses heuristic rules
 *       based on the presence of adverbs (potential negation) and interrogatives.
 */

#ifndef ADMIN850_PATTERN_HPP
#define ADMIN850_PATTERN_HPP

#include "../common/types.hpp"
#include <vector>
#include <cstdint>

/**
 * @struct Pattern
 * @brief A lightweight data structure that holds a sequence of word types and
 *        the overall pattern type classification.
 */
struct Pattern {
    std::vector<WordType> sequence;   ///< Ordered word-type sequence.
    PatternType           type;       ///< Classified pattern type.
    uint32_t timestamp_ = 0;           ///< Last access timestamp (Unix seconds)
    float    frequency_ = 1.0f;        ///< Usage frequency counter

    /// Default constructor: empty sequence, SENTENCES type.
    Pattern() : type(PatternType::SENTENCES) {}

    /**
     * @brief Constructs a Pattern from a sequence and optional classification.
     * @param seq The word-type sequence.
     * @param patternType The classification (default SENTENCES).
     */
    explicit Pattern(const std::vector<WordType>& seq,
                     PatternType patternType = PatternType::SENTENCES)
        : sequence(seq), type(patternType) {}
};

/**
 * @brief Classifies a sequence of word types into a PatternType.
 * @param sequence The word-type sequence to classify.
 * @return The inferred PatternType based on heuristics (presence of
 *         interrogative, possible negation via adverb, and sequence length).
 */
PatternType classifySentencePattern(const std::vector<WordType>& sequence);

/**
 * @brief Creates a Pattern from a word-type sequence, automatically classifying it.
 * @param sequence The word-type sequence.
 * @return A Pattern object with the sequence and its inferred type.
 */
Pattern patternFromSequence(const std::vector<WordType>& sequence);

#endif // ADMIN850_PATTERN_HPP
