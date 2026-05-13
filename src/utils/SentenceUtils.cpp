/**
 * @file SentenceUtils.cpp
 * @brief Implementation of sentence utility functions.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "SentenceUtils.hpp"
#include "../db/WordRepository.hpp"
#include "../nlp/Tokenizer.hpp"
#include "../nlp/Morphology.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <regex>

namespace utils {

// ---------------------------------------------------------------------------
// One‑time initialisation of the random generator (for creativity functions)
// ---------------------------------------------------------------------------
static const bool randInit = []() -> bool {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    return true;
}();

// ========================================================================
// parsePremise
// ========================================================================
ParsedPremise parsePremise(const Sentence& premise) {
    ParsedPremise result;
    const auto& blocks = premise.getBlocks();
    bool afterPrep = false;

    // 1. Subject following a preposition (simplified)
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].type == WordType::PREPOSITION) {
            afterPrep = true;
            continue;
        }
        if (afterPrep && (blocks[i].type == WordType::NOUN || blocks[i].type == WordType::PRONOUN)) {
            result.subject = blocks[i].text;
            afterPrep = false;
            break;
        }
    }

    // 2. General subject (first noun / pronoun if not found above)
    if (result.subject.empty()) {
        for (const auto& b : blocks) {
            if (b.type == WordType::NOUN || b.type == WordType::PRONOUN) {
                result.subject = b.text;
                break;
            }
        }
    }

    // 3. Verb (first one)
    for (const auto& b : blocks) {
        if (b.type == WordType::VERB) {
            result.verb = b.text;
            break;
        }
    }

    // 4. Object (first noun / pronoun after the verb)
    bool verbPassed = false;
    for (const auto& b : blocks) {
        if (!verbPassed && b.type == WordType::VERB) {
            verbPassed = true;
            continue;
        }
        if (verbPassed && (b.type == WordType::NOUN || b.type == WordType::PRONOUN)) {
            result.object = b.text;
            break;
        }
    }

    // 5. Keywords (nouns + adjectives)
    for (const auto& b : blocks) {
        if (b.type == WordType::NOUN || b.type == WordType::PRONOUN || b.type == WordType::ADJECTIVE) {
            result.keywords.push_back(b.text);
        }
    }

    result.patternType = classifySentencePattern(premise.getTypeSequence());
    return result;
}

// ========================================================================
// buildSentenceFromText
// ========================================================================
Sentence buildSentenceFromText(const std::string& text) {
    auto tokens = tokenize(text);
    std::vector<Word> words;
    words.reserve(tokens.size());
    for (const auto& tok : tokens) {
        Word w(tok.text);
        WordRepository::load(tok.text, w);  // loads type etc. if exists
        words.push_back(std::move(w));
    }
    return Sentence(words);
}

// ========================================================================
// applyCreativity
// ========================================================================
void applyCreativity(std::string& text, float creativity) {
    if (creativity < 0.3f) return;

    std::string lang = morphology::getLanguage();

    // Add doubt prefix for high creativity (if not already)
    if (creativity > 0.8f) {
        if (lang == "en") {
            if (text.find("maybe") == std::string::npos &&
                text.find('?') == std::string::npos) {
                text = "Maybe " + text;
            }
        } else { // Spanish
            if (text.find("no") == std::string::npos &&
                text.find('?') == std::string::npos) {
                text = "Quizás " + text;
            }
        }
    }

    // Random synonym replacement (language independent)
    if (creativity > 0.5f) {
        std::vector<std::string> words;
        std::istringstream iss(text);
        std::string w;
        while (iss >> w) words.push_back(w);

        for (auto& w : words) {
            Word wordObj;
            if (WordRepository::load(w, wordObj)) {
                const auto& rels = wordObj.getRelated();
                if (!rels.empty() && (std::rand() % 100) < static_cast<int>(creativity * 100)) {
                    w = rels[0].first;
                    break;
                }
            }
        }

        std::string rebuilt;
        for (const auto& word : words) {
            if (!rebuilt.empty()) rebuilt += ' ';
            rebuilt += word;
        }
        text = rebuilt;
    }
}

void advancedCreativeTransform(std::string& text, float creativity,
                               const ParsedPremise& /*premiseInfo*/) {
    if (creativity < 0.2f) return;

    std::string lang = morphology::getLanguage();

    // 1. Add filler word at beginning
    if (creativity > 0.7f) {
        if (lang == "en") {
            if (text.find("maybe") == std::string::npos &&
                text.find("perhaps") == std::string::npos &&
                text.front() != '?') {
                if (std::rand() % 100 < 40)
                    text = "Maybe " + text;
            }
        } else { // Spanish
            if (text.find("quizás") == std::string::npos &&
                text.find("tal vez") == std::string::npos &&
                text.front() != '¿') {
                if (std::rand() % 100 < 40)
                    text = "Quizás " + text;
            }
        }
    }

    // 2. Synonym replacement - language independent
    if (creativity > 0.4f) {
        std::vector<std::string> words;
        std::istringstream iss(text);
        std::string w;
        while (iss >> w) words.push_back(w);

        for (auto& w : words) {
            Word wordObj;
            if (WordRepository::load(w, wordObj)) {
                const auto& rels = wordObj.getRelated();
                if (!rels.empty() && (std::rand() % 100) < static_cast<int>(creativity * 40)) {
                    int idx = std::rand() % rels.size();
                    w = rels[idx].first;
                    break;
                }
            }
        }

        std::string rebuilt;
        for (const auto& word : words) {
            if (!rebuilt.empty()) rebuilt += ' ';
            rebuilt += word;
        }
        text = rebuilt;
    }

    // 3. Punctuation / type change
    if (creativity > 0.8f) {
        if (lang == "en") {
            if (text.find('?') == std::string::npos && text.find("not") == std::string::npos) {
                // English doesn't use inverted question marks
                text = "?" + text + "?";
            }
        } else { // Spanish
            if (text.find('?') == std::string::npos &&
                text.find("no") == std::string::npos &&
                text.back() != '?') {
                text = "¿" + text + "?";
            }
        }
    } else if (text.back() != '.' && text.back() != '?' && text.back() != '!') {
        text += '.';
    }

    // 4. Capitalise first letter
    if (!text.empty()) {
        text[0] = std::toupper(text[0]);
    }
}

// ========================================================================
// computeCreativity
// ========================================================================
float computeCreativity(const Sentence& premise, const Sentence& hypothesis,
                        const Pattern& /*pattern*/) {
    std::vector<std::string> premiseWords, hypoWords;
    for (const auto& b : premise.getBlocks()) {
        if (b.type == WordType::NOUN || b.type == WordType::VERB)
            premiseWords.push_back(b.text);
    }
    for (const auto& b : hypothesis.getBlocks()) {
        if (b.type == WordType::NOUN || b.type == WordType::VERB)
            hypoWords.push_back(b.text);
    }

    int common = 0;
    for (const auto& pw : premiseWords) {
        if (std::find(hypoWords.begin(), hypoWords.end(), pw) != hypoWords.end())
            ++common;
    }

    if (premiseWords.empty() && hypoWords.empty()) return 0.5f;
    float base = static_cast<float>(common) /
                 std::max(premiseWords.size(), hypoWords.size());

    int diff = static_cast<int>(premise.getBlocks().size()) -
               static_cast<int>(hypothesis.getBlocks().size());
    int maxSize = static_cast<int>(std::max(premise.getBlocks().size(),
                                            hypothesis.getBlocks().size()));
    float lenRatio = (maxSize == 0) ? 1.0f
                     : 1.0f - static_cast<float>(std::abs(diff)) / static_cast<float>(maxSize);

    return std::min(0.95f, base * 0.7f + lenRatio * 0.3f);
}

// ========================================================================
// applyCreativityToText
// ========================================================================
std::string applyCreativityToText(const std::string& text, float creativity) {
    std::string result = text;
    ParsedPremise emptyPremise;
    advancedCreativeTransform(result, creativity, emptyPremise);
    return result;
}

} // namespace utils
