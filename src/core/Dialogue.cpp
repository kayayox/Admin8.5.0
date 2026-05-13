/**
 * @file Dialogue.cpp
 * @brief Implementation of dialogue history and hypothesis generation.
 *
 * Uses a weighted random selection among five strategies:
 *  1. dialogue history  2. contextual continuation  3. template filling
 *  4. chunk correlation  5. fallback.
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Dialogue.hpp"
#include "InferenceEngine.hpp"
#include "../dialogue/PatternCorrelator.hpp"
#include "../dialogue/ContextualCorrelator.hpp"
#include "../dialogue/ChunkCorrelator.hpp"
#include "../utils/SlotFiller.hpp"
#include "../utils/ResponseTemplates.hpp"
#include "../utils/SentenceUtils.hpp"
#include "../utils/LearningHelpers.hpp"
#include "../utils/Chunker.hpp"
#include "../nlp/Morphology.hpp"
#include "../db/WordRepository.hpp"
#include "../db/SentenceRepository.hpp"
#include "../db/DialogueRepository.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <queue>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Returns the last N words of a sentence (by block text).
static std::vector<std::string> lastWords(const Sentence& s, int n) {
    const auto& blocks = s.getBlocks();
    std::vector<std::string> res;
    int start = static_cast<int>(blocks.size()) - n;
    if (start < 0) start = 0;
    for (size_t i = static_cast<size_t>(start); i < blocks.size(); ++i) {
        res.push_back(blocks[i].text);
    }
    return res;
}

/// Simple similarity: ratio of common words between two keyword lists.
static float simpleSimilarity(const std::vector<std::string>& a,
                              const std::vector<std::string>& b) {
    if (a.empty() && b.empty()) return 0.0f;
    int common = 0;
    for (const auto& wa : a) {
        if (std::find(b.begin(), b.end(), wa) != b.end()) ++common;
    }
    return static_cast<float>(common) / std::max(a.size(), b.size());
}

/// Fallback hypothesis when no other method succeeds.
static std::string fallbackHypothesis(const utils::ParsedPremise& parsed,
                                      float creativity) {
    std::string lang = morphology::getLanguage();

    if (lang == "en") {
        // English fallbacks
        if (!parsed.verb.empty() && !parsed.subject.empty()) {
            std::string base = "I understand that " + parsed.subject + " " + parsed.verb;
            if (!parsed.object.empty()) base += " " + parsed.object;
            if (creativity > 0.5f) base = "?" + base + "?";
            else base += ".";
            return base;
        } else if (!parsed.verb.empty()) {
            return "?" + parsed.verb + "?";
        } else {
            return "I didn't understand the premise. Could you rephrase?";
        }
    } else {
        // Spanish fallbacks
        if (!parsed.verb.empty() && !parsed.subject.empty()) {
            std::string base = "Entiendo que " + parsed.subject + " " + parsed.verb;
            if (!parsed.object.empty()) base += " " + parsed.object;
            if (creativity > 0.5f) base = "¿" + base + "?";
            else base += ".";
            return base;
        } else if (!parsed.verb.empty()) {
            return "¿" + parsed.verb + "?";
        } else {
            return "No he comprendido la premisa. ¿Podrías reformularla?";
        }
    }
}

// ============================================================================
// Core: generateHypothesis
// ============================================================================
Sentence generateHypothesis(const Sentence& premise,
                            DialogueContext& ctx,
                            Pattern* pattern,
                            const std::string& keyword,
                            float creativity) {
    // 1. Parse premise
    auto parsed = utils::parsePremise(premise);
    const auto& blocks = premise.getBlocks();
    std::vector<WordType> prevTagContext;
    std::vector<std::string> prevWordContext;
    for (const auto& b : blocks) {
        prevTagContext.push_back(b.type);
        prevWordContext.push_back(b.text);
    }

    // 2. Determine keyword
    std::string key = keyword;
    if (key.empty() && !parsed.subject.empty()) key = parsed.subject;
    else if (key.empty() && !parsed.object.empty()) key = parsed.object;
    else if (key.empty()) key = premise.getKey().text;
    if (key.empty()) return utils::buildSentenceFromText(fallbackHypothesis(parsed, creativity));

    // 3. Semantic expansion
    std::vector<std::string> related = WordRepository::getRelatedWordsRecursive(key, 2, 15);
    std::unordered_set<std::string> wordSet;
    wordSet.insert(key);
    wordSet.insert(related.begin(), related.end());

    // -----------------------------------------------------------------------
    // Strategy lambdas (each returns {hypothesisText, score}, score < 0 if no result)
    // -----------------------------------------------------------------------
    auto method_dialogue = [&]() -> std::pair<std::string, float> {
        auto dialogues = DialogueRepository::loadHistoryContainingWords({key}, 10);
        std::vector<std::pair<std::string, float>> candidates;
        for (const auto& d : dialogues) {
            if (d.premise.toString() == premise.toString()) continue;
            auto parsedHist = utils::parsePremise(d.premise);
            float sim = simpleSimilarity(parsed.keywords, parsedHist.keywords);
            if (sim > 0.3f) {
                float score = (d.creativity + sim) * 0.6f;
                candidates.emplace_back(d.hypothesis.toString(), score);
            }
        }
        if (candidates.empty()) return {"", -1.0f};
        return *std::max_element(candidates.begin(), candidates.end(),
                                 [](const auto& a, const auto& b) { return a.second < b.second; });
    };

    auto method_context = [&]() -> std::pair<std::string, float> {
        if (!ctx.ctxCorr) return {"", -1.0f};
        auto lastTwo = lastWords(premise, 2);
        std::string current = lastTwo.empty() ? "" : lastTwo.back();
        std::vector<std::string> prev;
        if (lastTwo.size() >= 2) prev = {lastTwo[0]};
        std::vector<std::pair<WordPattern, double>> outcomes;
        if (current.empty() || !ctx.ctxCorr->queryNext(current, prev, outcomes))
            return {"", -1.0f};
        for (const auto& out : outcomes) {
            if (!out.first.empty()) {
                std::string pred = out.first.begin()->first;
                if (wordSet.count(pred) || creativity > 0.7f) {
                    float score = static_cast<float>(out.second) * 0.8f;
                    return {premise.toString() + " " + pred, score};
                }
            }
        }
        return {"", -1.0f};
    };

    auto method_template = [&]() -> std::pair<std::string, float> {
        if (!ctx.templateMatcher || !ctx.slotFiller) return {"", -1.0f};
        PatternType targetType = (pattern) ? pattern->type : parsed.patternType;
        std::string lang = morphology::getLanguage();  // use current NLP language
        const ResponseTemplate* tmpl = ctx.templateMatcher->matchTemplate(lang, targetType, parsed.keywords);
        if (!tmpl) return {"", -1.0f};
        std::unordered_map<std::string, std::string> slotVals;
        for (const auto& slot : tmpl->slots) {
            WordType expected = SlotFiller::inferTypeFromSlotName(slot);
            std::string bestWord;
            if (slot == "sujeto" && !parsed.subject.empty()) bestWord = parsed.subject;
            else if (slot == "verbo" && !parsed.verb.empty()) bestWord = parsed.verb;
            else if (slot == "objeto" && !parsed.object.empty()) bestWord = parsed.object;
            else {
                for (const auto& w : wordSet) {
                    Word wobj;
                    if (WordRepository::load(w, wobj) &&
                        (expected == WordType::UNDEFINED || wobj.getType() == expected)) {
                        bestWord = w;
                        break;
                    }
                }
            }
            if (bestWord.empty()) {
                bestWord = ctx.slotFiller->predictForSlot(slot, expected,
                                                          prevTagContext, prevWordContext);
            }
            slotVals[slot] = bestWord;
        }
        std::string filled = ctx.templateMatcher->fillTemplate(*tmpl, slotVals);
        if (filled.empty()) return {"", -1.0f};
        float score = 0.6f + creativity * 0.2f;
        return {filled, score};
    };

    auto method_chunk = [&]() -> std::pair<std::string, float> {
        if (!ctx.chcCorr) return {"", -1.0f};
        std::vector<Word> words = createWordVector(premise.toString());
        std::vector<std::string> chunks = Chunker::chunk(words);
        if (chunks.empty()) return {"", -1.0f};
        std::vector<std::pair<WordPattern, double>> outcomes;
        if (!ctx.chcCorr->queryNext(chunks.back(), {}, outcomes)) return {"", -1.0f};
        for (const auto& out : outcomes) {
            if (!out.first.empty()) {
                std::string nextChunk = out.first.begin()->first;
                float score = static_cast<float>(out.second) * 0.7f;
                return {premise.toString() + " " + nextChunk, score};
            }
        }
        return {"", -1.0f};
    };

    auto method_fallback = [&]() -> std::pair<std::string, float> {
        return {fallbackHypothesis(parsed, creativity), 0.5f};
    };

    // -----------------------------------------------------------------------
    // Weighted random method selection
    // -----------------------------------------------------------------------
    float c = std::max(0.0f, std::min(1.0f, creativity));
    float wDial  = 1.0f - c;
    float wCtx   = 0.5f;
    float wTmpl  = c * 0.8f;
    float wChunk = c * 0.5f;
    float wFall  = 0.1f;
    float total  = wDial + wCtx + wTmpl + wChunk + wFall;
    wDial /= total; wCtx /= total; wTmpl /= total; wChunk /= total; wFall /= total;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    float r = dis(gen);

    std::pair<std::string, float> result;
    if (r < wDial) {
        result = method_dialogue();
    } else if (r < wDial + wCtx) {
        result = method_context();
    } else if (r < wDial + wCtx + wTmpl) {
        result = method_template();
    } else if (r < wDial + wCtx + wTmpl + wChunk) {
        result = method_chunk();
    } else {
        result = method_fallback();
    }

    std::string bestText = result.first;
    if (bestText.empty()) {
        bestText = fallbackHypothesis(parsed, creativity);
    }

    // Apply creative transformations (limited)
    utils::advancedCreativeTransform(bestText, std::min(creativity, 0.7f), parsed);
    return utils::buildSentenceFromText(bestText);
}

// ============================================================================
// DialogueHistory
// ============================================================================
void DialogueHistory::addDialogue(const Sentence& premise,
                                  const Sentence& hypothesis,
                                  const Pattern& pattern,
                                  float creativity) {
    history_.push_back({premise, hypothesis, pattern, creativity});
    updateThreshold();
}

void DialogueHistory::updateThreshold() {
    if (history_.empty()) return;
    float sum = 0.0f;
    for (const auto& d : history_) sum += d.creativity;
    thresholdCreativity_ = sum / static_cast<float>(history_.size());
}
