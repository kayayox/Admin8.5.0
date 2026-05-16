/**
 * @file Classifier.cpp
 * @brief Implementation of the word classifier.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Classifier.hpp"
#include "Morphology.hpp"
#include "Refiner.hpp"
#include "TagStats.hpp"
#include "../db/WordRepository.hpp"
#include <algorithm>
#include <iostream>

namespace {

/// Confidence thresholds
constexpr float kHighConfThreshold   = 0.9f;
constexpr float kLowConfForReeval    = 0.2f;
constexpr float kConfMax             = 0.99f;
constexpr float kConfMin             = 0.10f;

/// Weight blending in contextual refinement
constexpr float kContextWeight = 0.6f;
constexpr float kMorphWeight   = 0.4f;

/// Confidence adjustment factors
constexpr float kCorrectBoostFactor = 0.25f;
constexpr float kErrorPenaltyFactor = 0.75f;

} // anonymous namespace

Classifier::Classifier() = default;

void Classifier::updateMorphAttributes(Word& word, WordType tag) {
    const std::string& text = word.getWord();

    switch (tag) {
        case WordType::VERB:
            word.setTense(morphology::detectTense(text));
            word.setPerson(morphology::detectPerson(text));
            word.setQuantity(morphology::endsWith(text, "n") ||
                             morphology::endsWith(text, "mos")
                                 ? Quantity::PLURAL
                                 : Quantity::SINGULAR);
            break;
        case WordType::NOUN:
        case WordType::ADJECTIVE:
            word.setQuantity(morphology::isPlural(text) ? Quantity::PLURAL
                                                        : Quantity::SINGULAR);
            word.setGender(morphology::detectGender(text));
            if (tag == WordType::ADJECTIVE) {
                word.setDegree(morphology::detectAdjectiveDegree(text));
            }
            break;
        case WordType::ARTICLE:
            word.setQuantity(morphology::isPlural(text) ? Quantity::PLURAL
                                                        : Quantity::SINGULAR);
            break;
        default:
            break;
    }
    word.generateStructuredMeaning();
}

void Classifier::classifySentence(std::vector<Word>& words) {
    if (words.empty()) return;

    // -------------------------------------------------------------------------
    // Pass 1: initial classification + DB load
    // -------------------------------------------------------------------------
    for (auto& w : words) {
        WordRepository::load(w.getWord(), w); // load persisted attributes

        if (w.getConfidence() >= kHighConfThreshold) continue;

        WordType commonTag;
        float    commonConf;
        if (morphology::isCommonWord(w.getWord(), commonTag, commonConf)) {
            w.setType(commonTag);
            w.setConfidence(commonConf);
            updateMorphAttributes(w, commonTag);
            WordRepository::save(w);
            continue;
        }

        WordType guessedTag = morphology::guessInitialTag(w.getWord());
        float morphConf     = morphology::validateTag(w.getWord(), guessedTag);

        if (morphConf >= kHighConfThreshold) {
            w.setType(guessedTag);
            w.setConfidence(morphConf);
            updateMorphAttributes(w, guessedTag);
            WordRepository::save(w);
        } else {
            w.setType(WordType::UNDEFINED);
            w.setConfidence(kLowConfForReeval);
        }
    }

    // -------------------------------------------------------------------------
    // Pass 2: iterative contextual refinement
    // -------------------------------------------------------------------------
    for (size_t i = 0; i < words.size(); ++i) {
        Word& w = words[i];
        if (w.getConfidence() >= kHighConfThreshold) continue;

        WordType prev2 = (i >= 2) ? words[i - 2].getType() : WordType::UNDEFINED;
        WordType prev  = (i >= 1) ? words[i - 1].getType() : WordType::UNDEFINED;
        WordType next  = (i + 1 < words.size()) ? words[i + 1].getType() : WordType::UNDEFINED;

        TagConfidence refined = refineTag(prev2, prev, w.getType(), next, w.getConfidence());
        float morphScore      = morphology::validateTag(w.getWord(), refined.tag);
        float combined        = kContextWeight * refined.confidence + kMorphWeight * morphScore;

        w.setType(refined.tag);
        w.setConfidence(std::min(1.0f, combined));
        updateMorphAttributes(w, refined.tag);
        WordRepository::save(w);
    }

    // -------------------------------------------------------------------------
    // Pass 3: update transition statistics only if the whole sentence is high confidence
    // -------------------------------------------------------------------------
    bool allHighConf = std::all_of(words.begin(), words.end(),
                                   [](const Word& w) {
                                       return w.getConfidence() >= kHighConfThreshold;
                                   });

    if (allHighConf) {
        for (size_t i = 1; i < words.size(); ++i) {
            TagStats::updateUnigram(words[i - 1].getType(), words[i].getType());
        }
        for (size_t i = 2; i < words.size(); ++i) {
            TagStats::updateBigram(words[i - 2].getType(), words[i - 1].getType(), words[i].getType());
        }
        for (size_t i = 0; i + 2 < words.size(); ++i) {
            TagStats::updateTrigram(words[i].getType(), words[i + 1].getType(), words[i + 2].getType());
        }
    }
}

void Classifier::updateConfidence(Word& word, bool wasCorrect) {
    float conf = word.getConfidence();

    if (wasCorrect) {
        conf += (1.0f - conf) * kCorrectBoostFactor;
    } else {
        conf *= kErrorPenaltyFactor;
    }

    conf = std::clamp(conf, kConfMin, kConfMax);
    word.setConfidence(conf);
    WordRepository::save(word);
}
