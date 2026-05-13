/**
 * @file Pattern.cpp
 * @brief Implementation of pattern classification utilities.
 * @author Soubhi Khayat Najjar
 * @date 2026
 * @note Heuristic classification: an interrogative word (INTERROGATIVE) indicates
 *       a question; an adverb (ADVERB) is used as a simple proxy for negation.
 *       For a more accurate system, lexical content would be required.
 */

#include "Pattern.hpp"

PatternType classifySentencePattern(const std::vector<WordType>& sequence) {
    bool hasNegation = false;
    bool hasQuestion = false;

    for (WordType wordType : sequence) {
        // Heuristic: adverbs may indicate negation ("no", "never", etc.)
        if (wordType == WordType::ADVERB) {
            hasNegation = true;
        }
        // Interrogative words indicate a question.
        if (wordType == WordType::INTERROGATIVE) {
            hasQuestion = true;
        }
    }

    if (hasQuestion && sequence.size() > 3) return PatternType::COMPOUND_INTERROGATIVE;
    if (hasQuestion)                      return PatternType::SIMPLE_INTERROGATIVE;
    if (hasNegation && sequence.size() > 3) return PatternType::COMPOUND_NEGATIVE;
    if (hasNegation)                      return PatternType::SIMPLE_NEGATIVE;
    if (sequence.size() > 3)              return PatternType::COMPOUND_AFFIRMATIVE;
    return PatternType::SIMPLE_AFFIRMATIVE;
}

Pattern patternFromSequence(const std::vector<WordType>& sequence) {
    return Pattern(sequence, classifySentencePattern(sequence));
}
