/**
 * @file Dialogue.hpp
 * @brief Dialogue structures, history, and hypothesis generation (advanced).
 *
 * Uses multiple correlators, templates and creativity heuristics to produce
 * a hypothesis sentence from a premise.
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_DIALOGUE_HPP
#define ADMIN850_DIALOGUE_HPP

#include "Sentence.hpp"
#include "Pattern.hpp"
#include "InferenceRule.hpp"
#include <vector>
#include <string>
#include <cstdint>

// Forward declarations
class PatternCorrelator;
class ContextualCorrelator;
class ChunkCorrelator;
class TemplateMatcher;
class SlotFiller;

// ============================================================================
// Dialogue context – groups all resources needed for generation
// ============================================================================
struct DialogueContext {
    PatternCorrelator*   patternCorr     = nullptr;
    ContextualCorrelator* ctxCorr        = nullptr;
    ChunkCorrelator*      chcCorr        = nullptr;
    TemplateMatcher*      templateMatcher = nullptr;
    SlotFiller*           slotFiller      = nullptr;

    std::vector<InferenceRule> inferenceRules;

    DialogueContext() = default;
};

// ============================================================================
// Dialogue history
// ============================================================================
struct Dialogue {
    Sentence premise;
    Sentence hypothesis;
    Pattern  pattern;
    uint32_t timestamp_ = 0;           ///< Last access timestamp (Unix seconds)
    uint32_t frequency_ = 1;          ///< Usage counter
    float    creativity = 0.0f;
};

class DialogueHistory {
public:
    void addDialogue(const Sentence& premise, const Sentence& hypothesis,
                     const Pattern& pattern, float creativity);
    const std::vector<Dialogue>& getHistory() const { return history_; }
    float getThresholdCreativity() const { return thresholdCreativity_; }

private:
    std::vector<Dialogue> history_;
    float thresholdCreativity_ = 0.5f;
    void updateThreshold();
};

// ============================================================================
// Main hypothesis generation function
// ============================================================================
Sentence generateHypothesis(const Sentence& premise,
                            DialogueContext& ctx,
                            Pattern* pattern = nullptr,
                            const std::string& keyword = "",
                            float creativity = 0.5f);

/// Loads default inference rules (defined in DefaultRules.cpp)
void loadDefaultInferenceRules(DialogueContext& ctx);

#endif // ADMIN850_DIALOGUE_HPP
