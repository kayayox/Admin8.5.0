/**
 * @file Refiner.cpp
 * @brief Implementation of contextual tag refinement.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Refiner.hpp"
#include <algorithm>

TagConfidence refineTag(WordType prev2, WordType prev, WordType current,
                        WordType next, float currentConfidence) {
    // If the current tag has medium‑high confidence, keep it.
    if (current != WordType::UNDEFINED && currentConfidence >= 0.5f) {
        return {current, currentConfidence};
    }

    auto trigram = TagStats::getTrigramProbs(prev, next);
    auto bigram  = TagStats::getBigramProbs(prev2, prev);
    auto unigram = TagStats::getUnigramProbs(prev);

    TagConfidence result{WordType::UNDEFINED, 0.0f};

    if (current == WordType::UNDEFINED) {
        // No current tag: pick the best prediction among all models.
        auto updateResult = [&](const std::vector<std::pair<WordType, float>>& probs) {
            auto best = getBestPrediction(probs);
            if (best.second > result.confidence) {
                result = {best.first, best.second};
            }
        };
        updateResult(trigram);
        updateResult(bigram);
        updateResult(unigram);
        return result;
    }

    // A tag already exists: try to confirm it, otherwise look for a better one.
    auto tryConfirm = [&](const std::vector<std::pair<WordType, float>>& probs, float boost) {
        auto best = getBestPrediction(probs);
        if (best.first == current) {
            return TagConfidence{current, std::min(1.0f, currentConfidence + boost)};
        }
        if (best.second > result.confidence) {
            result = {best.first, best.second};
        }
        return TagConfidence{WordType::UNDEFINED, 0.0f}; // sentinel
    };

    if (!trigram.empty()) {
        auto confirmed = tryConfirm(trigram, 0.2f);
        if (confirmed.confidence > 0.0f) return confirmed;
    }
    if (!bigram.empty()) {
        auto confirmed = tryConfirm(bigram, 0.1f);
        if (confirmed.confidence > 0.0f) return confirmed;
    }
    if (!unigram.empty()) {
        auto best = getBestPrediction(unigram);
        if (best.first == current) {
            return {current, std::min(1.0f, currentConfidence + 0.05f)};
        } else if (best.second > result.confidence) {
            result = {best.first, best.second};
        }
    }
    return result;
}
