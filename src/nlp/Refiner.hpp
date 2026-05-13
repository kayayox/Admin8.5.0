/**
 * @file Refiner.hpp
 * @brief Contextual tag refinement using unigram, bigram, and trigram models.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_REFINER_HPP
#define ADMIN850_REFINER_HPP

#include "../common/types.hpp"
#include "TagStats.hpp"
#include <algorithm>
#include <utility>
#include <vector>

/**
 * @struct TagConfidence
 * @brief Holds a predicted word type and its confidence score.
 */
struct TagConfidence {
    WordType tag;
    float    confidence;
};

/**
 * @brief Returns the best (most probable) tag and its probability from a list.
 * @param predictions A list of tag-probability pairs.
 * @return A pair (tag, probability). If empty, returns (UNDEFINED, 0.0f).
 */
inline std::pair<WordType, float> getBestPrediction(
    const std::vector<std::pair<WordType, float>>& predictions) {
    if (predictions.empty()) return {WordType::UNDEFINED, 0.0f};
    return *std::max_element(predictions.begin(), predictions.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
}

/**
 * @brief Refines a word's tag given the surrounding context and its current confidence.
 *
 * Uses n-gram statistics from TagStats to either confirm or override the current tag.
 * The language used is the one currently active in TagStats (set via TagStats::setLanguage).
 *
 * @param prev2            Type of the word two positions before the current word.
 * @param prev             Type of the immediately preceding word.
 * @param current          Current (possibly tentative) type of the word.
 * @param next             Type of the immediately following word.
 * @param currentConfidence Current confidence in the 'current' type.
 * @return A TagConfidence with the refined tag and updated confidence.
 */
TagConfidence refineTag(WordType prev2, WordType prev, WordType current,
                        WordType next, float currentConfidence);

#endif // ADMIN850_REFINER_HPP
