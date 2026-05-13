/**
 * @file SlotFiller.cpp
 * @brief Implementation of slot filling with contextual prediction.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "SlotFiller.hpp"
#include "../db/WordRepository.hpp"
#include <algorithm>

SlotFiller::SlotFiller(ContextualCorrelator& correlator) : ctxCorr(correlator) {}

WordType SlotFiller::inferTypeFromSlotName(const std::string& slotName) {
    // Map common slot names to WordType
    static const std::unordered_map<std::string, WordType> map = {
        {"SUBJECT",    WordType::NOUN},
        {"OBJECT",     WordType::NOUN},
        {"NOUN",       WordType::NOUN},
        {"VERB",       WordType::VERB},
        {"ADJECTIVE",  WordType::ADJECTIVE},
        {"ADVERB",     WordType::ADVERB},
        {"PRONOUN",    WordType::PRONOUN},
        {"ARTICLE",    WordType::ARTICLE},
        {"PREPOSITION",WordType::PREPOSITION},
        {"CONJUNCTION",WordType::CONJUNCTION},
        {"NUMERAL",    WordType::NUMERAL},
        {"DEMONSTRATIVE", WordType::DEMONSTRATIVE},
        {"INTERROGATIVE", WordType::INTERROGATIVE}
    };
    auto it = map.find(slotName);
    return (it != map.end()) ? it->second : WordType::UNDEFINED;
}

std::unordered_map<std::string, std::string> SlotFiller::fillSlots(
    const std::vector<std::string>& slots,
    const std::unordered_map<std::string, WordType>& slotTypes,
    const std::vector<WordType>& initialTagContext,
    const std::vector<std::string>& initialWordContext) {

    std::unordered_map<std::string, std::string> result;
    auto currentTagCtx  = initialTagContext;
    auto currentWordCtx = initialWordContext;

    for (const auto& slot : slots) {
        WordType expected = WordType::UNDEFINED;
        auto typeIt = slotTypes.find(slot);
        if (typeIt != slotTypes.end()) {
            expected = typeIt->second;
        } else {
            expected = inferTypeFromSlotName(slot);
        }

        std::string value = predictForSlot(slot, expected, currentTagCtx, currentWordCtx);
        result[slot] = value;

        if (!value.empty()) {
            currentWordCtx.push_back(value);
            currentTagCtx.push_back(expected);
        }
    }
    return result;
}

void SlotFiller::setPremiseContext(const std::string& subject, const std::string& verb, const std::string& object) {
    premiseSubject_ = subject;
    premiseVerb_    = verb;
    premiseObject_  = object;
}

void SlotFiller::clearPremiseContext() {
    premiseSubject_.clear();
    premiseVerb_.clear();
    premiseObject_.clear();
}

std::string SlotFiller::predictForSlot(const std::string& slotName,
                                       WordType expectedType,
                                       const std::vector<WordType>& prevTagContext,
                                       const std::vector<std::string>& prevWordContext) {
    // 1. Try premise context if slot matches subject/verb/object
    if (slotName == "SUBJECT" && !premiseSubject_.empty()) {
        Word w;
        if (WordRepository::load(premiseSubject_, w) && w.getType() == WordType::NOUN)
            return premiseSubject_;
    }
    if (slotName == "VERB" && !premiseVerb_.empty()) {
        Word w;
        if (WordRepository::load(premiseVerb_, w) && w.getType() == WordType::VERB)
            return premiseVerb_;
    }
    if (slotName == "OBJECT" && !premiseObject_.empty()) {
        Word w;
        if (WordRepository::load(premiseObject_, w) && w.getType() == WordType::NOUN)
            return premiseObject_;
    }

    // 2. Contextual correlation prediction
    if (!prevWordContext.empty()) {
        std::string current = prevWordContext.back();
        std::vector<std::string> prev;
        if (prevWordContext.size() > 1) {
            prev.assign(prevWordContext.begin(), prevWordContext.end() - 1);
        }
        std::vector<std::pair<WordPattern, double>> outcomes;
        if (ctxCorr.queryNext(current, prev, outcomes)) {
            for (const auto& [pattern, prob] : outcomes) {
                for (const auto& [word, weight] : pattern) {
                    Word w;
                    if (WordRepository::load(word, w) && w.getType() == expectedType) {
                        return word;
                    }
                }
            }
        }
    }
    return "";
}

std::string SlotFiller::predictForSlot(const std::string& slotName,
                                       const std::vector<WordType>& prevTagContext,
                                       const std::vector<std::string>& prevWordContext) {
    WordType expected = inferTypeFromSlotName(slotName);
    return predictForSlot(slotName, expected, prevTagContext, prevWordContext);
}
