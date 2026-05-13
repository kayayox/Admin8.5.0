/**
 * @file SyllableCorrelator.cpp
 * @brief Implementation of the syllable correlator.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "SyllableCorrelator.hpp"
#include <sstream>
#include <cctype>
#include <algorithm>

SyllableCorrelator::SyllableCorrelator(const std::string& dbPath)
    : corr(std::make_unique<PatternCorrelator>(dbPath, "_syllable")) {}

WordPattern SyllableCorrelator::makePattern(const std::vector<std::string>& syllables) const {
    WordPattern pat;
    for (const auto& s : syllables) {
        pat[s] = 1.0f;
    }
    return pat;
}

// Basic English syllabification: each syllable contains one vowel sound.
// This simple algorithm processes a word and splits at positions where a vowel
// is followed by a consonant cluster and then another vowel (VCV pattern).
// It returns lowercase syllables.
std::vector<std::string> SyllableCorrelator::splitWordIntoSyllables(const std::string& word) {
    if (word.empty()) return {};
    auto isVowel = [](char c) {
        c = tolower(c);
        return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
    };

    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::vector<std::string> syllables;
    size_t start = 0;
    size_t len = lower.size();

    for (size_t i = 0; i < len; ++i) {
        // Look for a vowel that is followed by a consonant and then another vowel
        if (isVowel(lower[i]) && i + 2 < len && !isVowel(lower[i+1]) && isVowel(lower[i+2])) {
            // Split after the consonant (i+1)
            syllables.push_back(lower.substr(start, i + 2 - start));
            start = i + 2;
            i = start - 1; // continue after split
        }
    }
    if (start < len) {
        syllables.push_back(lower.substr(start));
    }
    // If no split was performed, the whole word is one syllable
    if (syllables.empty()) {
        syllables.push_back(lower);
    }
    return syllables;
}

std::vector<std::string> SyllableCorrelator::tokenizeIntoSyllables(const std::string& text) {
    std::vector<std::string> tokens;
    std::string currentWord;
    for (char ch : text) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!currentWord.empty()) {
                auto sylls = splitWordIntoSyllables(currentWord);
                tokens.insert(tokens.end(), sylls.begin(), sylls.end());
                currentWord.clear();
            }
            // Add space token to separate words
            tokens.push_back("__SPACE__");
        } else {
            currentWord.push_back(ch);
        }
    }
    // Last word
    if (!currentWord.empty()) {
        auto sylls = splitWordIntoSyllables(currentWord);
        tokens.insert(tokens.end(), sylls.begin(), sylls.end());
    } else if (!tokens.empty() && tokens.back() == "__SPACE__") {
        // Remove trailing space token if present
        tokens.pop_back();
    }
    return tokens;
}

void SyllableCorrelator::learnWithPreviousTwo(const std::string& text) {
    std::vector<std::string> syllables = tokenizeIntoSyllables(text);

    for (size_t i = 0; i < syllables.size(); ++i) {
        const std::string& current = syllables[i];
        std::vector<std::string> prevSyllables;
        if (i >= 2) {
            prevSyllables.push_back(syllables[i-2]);
            prevSyllables.push_back(syllables[i-1]);
        } else if (i == 1) {
            prevSyllables.push_back(syllables[i-1]);
        }
        if (prevSyllables.empty()) continue;

        WordPattern prevPat = makePattern(prevSyllables);
        if (i + 1 < syllables.size()) {
            WordPattern nextPat = makePattern({syllables[i+1]});
            corr->record(current, prevPat, nextPat, 1.0f);
        }
    }
}

void SyllableCorrelator::learnNextSyllableDirect(const std::string& text) {
    std::vector<std::string> syllables = tokenizeIntoSyllables(text);
    for (size_t i = 0; i + 1 < syllables.size(); ++i) {
        const std::string& current = syllables[i];
        WordPattern prevPat = {{"__NO_CONTEXT__", 1.0f}};
        WordPattern nextPat = makePattern({syllables[i+1]});
        corr->record(current, prevPat, nextPat, 1.0f);
    }
}

void SyllableCorrelator::learnFromText(const std::string& text) {
    learnWithPreviousTwo(text);
}

bool SyllableCorrelator::queryNext(const std::string& current,
                                   const std::vector<std::string>& previousSyllables,
                                   std::vector<std::pair<WordPattern, double>>& outcomes) {
    WordPattern prevPat = makePattern(previousSyllables);
    return corr->query(current, prevPat, outcomes);
}

bool SyllableCorrelator::queryNextWithOnePrev(const std::string& current,
                                              const std::string& prev,
                                              std::vector<std::pair<WordPattern, double>>& outcomes) {
    return queryNext(current, {prev}, outcomes);
}

bool SyllableCorrelator::queryNextWithTwoPrev(const std::string& current,
                                              const std::string& prev1,
                                              const std::string& prev2,
                                              std::vector<std::pair<WordPattern, double>>& outcomes) {
    return queryNext(current, {prev2, prev1}, outcomes);
}
