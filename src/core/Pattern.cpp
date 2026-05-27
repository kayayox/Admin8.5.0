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
    if (sequence.empty()) return PatternType::SENTENCES;

    bool hasNegation = false;
    bool hasQuestion = false;

    for (WordType wt : sequence) {
        if (wt == WordType::NEGATION) {
            hasNegation = true;
        }
        if (wt == WordType::INTERROGATIVE) {
            hasQuestion = true;
        }
    }
    if (!hasQuestion && sequence[0] == WordType::VERB) {
        return PatternType::IMPERATIVE;
    }
    if (hasQuestion && sequence.size() > 3) return PatternType::COMPOUND_INTERROGATIVE;
    if (hasQuestion) return PatternType::SIMPLE_INTERROGATIVE;
    if (hasNegation && sequence.size() > 3) return PatternType::COMPOUND_NEGATIVE;
    if (hasNegation) return PatternType::SIMPLE_NEGATIVE;
    if (sequence.size() > 3) return PatternType::COMPOUND_AFFIRMATIVE;
    return PatternType::SIMPLE_AFFIRMATIVE;
}

Pattern patternFromSequence(const std::vector<WordType>& sequence) {
    return Pattern(sequence, classifySentencePattern(sequence));
}
