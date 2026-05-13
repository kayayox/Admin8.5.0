/**
 * @file ResponseTemplates.cpp
 * @brief Implementation of template matching and auto‑generation.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "ResponseTemplates.hpp"
#include "StringConversions.hpp"
#include <algorithm>
#include <regex>
#include <sstream>

// ---------------------------------------------------------------------------
// ResponseTemplate
// ---------------------------------------------------------------------------
std::vector<std::string> ResponseTemplate::extractSlots(const std::string& tmpl) {
    std::vector<std::string> slots;
    std::regex re(R"(\{(\w+)\})");
    auto begin = std::sregex_iterator(tmpl.begin(), tmpl.end(), re);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        slots.push_back((*it)[1].str());
    }
    return slots;
}

// ---------------------------------------------------------------------------
// TemplateMatcher
// ---------------------------------------------------------------------------
void TemplateMatcher::registerTemplate(const ResponseTemplate& tmpl) {
    templates_.push_back(tmpl);
}

const ResponseTemplate* TemplateMatcher::matchTemplate(const std::string& lang,
                                                       PatternType patternType,
                                                       const std::vector<std::string>& keywords) const {
    std::vector<const ResponseTemplate*> candidates;

    // Collect all templates matching language and pattern type (or SENTENCIAS as wildcard)
    for (const auto& tmpl : templates_) {
        if (tmpl.language != lang) continue;
        if (tmpl.patternType == patternType || tmpl.patternType == PatternType::SENTENCES) {
            candidates.push_back(&tmpl);
        }
    }
    if (candidates.empty()) return nullptr;

    // Sort by priority desc, then keyword overlap score desc
    std::sort(candidates.begin(), candidates.end(),
              [&](const ResponseTemplate* a, const ResponseTemplate* b) {
                  if (a->priority != b->priority) return a->priority > b->priority;
                  int scoreA = 0, scoreB = 0;
                  for (const auto& kw : keywords) {
                      for (const auto& ckw : a->contextKeywords) {
                          if (kw.find(ckw) != std::string::npos || ckw.find(kw) != std::string::npos)
                              ++scoreA;
                      }
                      for (const auto& ckw : b->contextKeywords) {
                          if (kw.find(ckw) != std::string::npos || ckw.find(kw) != std::string::npos)
                              ++scoreB;
                      }
                  }
                  return scoreA > scoreB;
              });

    return candidates.front();
}

ResponseTemplate TemplateMatcher::createAndRegisterTemplate(const std::string& lang,
                                                            PatternType patternType,
                                                            const std::vector<WordType>& typeSequence,
                                                            const std::vector<std::string>& keywords) {
    // Build a generic template using the word-type sequence as slots
    std::ostringstream tmplStream;
    for (size_t i = 0; i < typeSequence.size(); ++i) {
        if (i > 0) tmplStream << ' ';
        // Use the enum name as placeholder (e.g., {NOUN}, {VERB})
        tmplStream << '{' << wordTypeToString(typeSequence[i]) << '}';
    }

    ResponseTemplate newTmpl;
    newTmpl.language = lang;
    newTmpl.patternType = patternType;
    newTmpl.templateText = tmplStream.str();
    newTmpl.slots = ResponseTemplate::extractSlots(newTmpl.templateText);
    newTmpl.priority = 1;               // low priority for auto‑generated
    newTmpl.frequency_ = 1;
    newTmpl.contextKeywords = keywords;

    templates_.push_back(newTmpl);
    return newTmpl;
}

std::string TemplateMatcher::fillTemplate(const ResponseTemplate& tmpl,
                                          const std::unordered_map<std::string, std::string>& slotValues) const {
    std::string result = tmpl.templateText;
    for (const auto& slot : tmpl.slots) {
        std::string placeholder = '{' + slot + '}';
        auto it = slotValues.find(slot);
        std::string replacement = (it != slotValues.end()) ? it->second : '[' + slot + ']';
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }
    // Remove any remaining placeholders
    std::regex leftover(R"(\{\w+\})");
    result = std::regex_replace(result, leftover, "");
    // Collapse multiple spaces
    result = std::regex_replace(result, std::regex(" +"), " ");
    // Trim
    result = std::regex_replace(result, std::regex("^ +| +$"), "");
    return result;
}

void TemplateMatcher::loadDefaultTemplates() {
    // ============================================================
    // Spanish (es)
    // ============================================================
    // Simple affirmative: ART + NOUN + VERB + ADV
    registerTemplate({-1, "es", PatternType::SIMPLE_AFFIRMATIVE,
        {"ARTICLE","NOUN","VERB","ADVERB"},
        "{ARTICLE} {NOUN} {VERB} {ADVERB}.", 10, 1, {}});
    // Simple affirmative: PRON + VERB + NOUN
    registerTemplate({-1, "es", PatternType::SIMPLE_AFFIRMATIVE,
        {"PRONOUN","VERB","NOUN"},
        "{PRONOUN} {VERB} {NOUN}.", 10, 1, {}});
    // Simple negative: NOUN + no + VERB + ADV
    registerTemplate({-1, "es", PatternType::SIMPLE_NEGATIVE,
        {"NOUN","VERB","ADVERB"},
        "{NOUN} no {VERB} {ADVERB}.", 10, 1, {"no"}});
    // Simple interrogative: VERB + NOUN
    registerTemplate({-1, "es", PatternType::SIMPLE_INTERROGATIVE,
        {"VERB","NOUN"},
        "¿{VERB} {NOUN}?", 10, 1, {"?"}});
    // Compound affirmative: PREP + ART + NOUN + VERB + CONJ + VERB
    registerTemplate({-1, "es", PatternType::COMPOUND_AFFIRMATIVE,
        {"PREPOSITION","ARTICLE","NOUN","VERB","CONJUNCTION","VERB"},
        "{PREPOSITION} {ARTICLE} {NOUN} {VERB} {CONJUNCTION} {VERB}.", 8, 1, {}});
    // Compound negative: aunque + NOUN + VERB + CONJ + no + VERB
    registerTemplate({-1, "es", PatternType::COMPOUND_NEGATIVE,
        {"NOUN","VERB","CONJUNCTION","VERB"},
        "Aunque {NOUN} {VERB} {CONJUNCTION} no {VERB}.", 7, 1, {"no"}});
    // Compound interrogative: PREP + qué + NOUN + VERB
    registerTemplate({-1, "es", PatternType::COMPOUND_INTERROGATIVE,
        {"PREPOSITION","NOUN","VERB"},
        "¿{PREPOSITION} qué {NOUN} {VERB}?", 8, 1, {"?"}});

    // ============================================================
    // English (en)
    // ============================================================
    registerTemplate({-1, "en", PatternType::SIMPLE_AFFIRMATIVE,
        {"ARTICLE","NOUN","VERB","ADVERB"},
        "{ARTICLE} {NOUN} {VERB} {ADVERB}.", 10, 1, {}});
    registerTemplate({-1, "en", PatternType::SIMPLE_AFFIRMATIVE,
        {"PRONOUN","VERB","NOUN"},
        "{PRONOUN} {VERB} {NOUN}.", 10, 1, {}});
    registerTemplate({-1, "en", PatternType::SIMPLE_NEGATIVE,
        {"NOUN","VERB","ADVERB"},
        "{NOUN} does not {VERB} {ADVERB}.", 10, 1, {"not"}});
    registerTemplate({-1, "en", PatternType::SIMPLE_INTERROGATIVE,
        {"VERB","NOUN"},
        "{VERB} {NOUN}?", 10, 1, {"?"}});
    registerTemplate({-1, "en", PatternType::COMPOUND_AFFIRMATIVE,
        {"PREPOSITION","ARTICLE","NOUN","VERB","CONJUNCTION","VERB"},
        "{PREPOSITION} {ARTICLE} {NOUN} {VERB} {CONJUNCTION} {VERB}.", 8, 1, {}});
    registerTemplate({-1, "en", PatternType::COMPOUND_NEGATIVE,
        {"NOUN","VERB","CONJUNCTION","VERB"},
        "Although {NOUN} {VERB} {CONJUNCTION} does not {VERB}.", 7, 1, {"not"}});
    registerTemplate({-1, "en", PatternType::COMPOUND_INTERROGATIVE,
        {"PREPOSITION","NOUN","VERB"},
        "{PREPOSITION} what {NOUN} {VERB}?", 8, 1, {"?"}});
}
