/**
 * @file LetterCorrelator.cpp
 * @brief Implementation of the letter correlator.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "LetterCorrelator.hpp"
#include <sstream>
#include <cctype>

LetterCorrelator::LetterCorrelator(const std::string& dbPath)
    : corr(std::make_unique<PatternCorrelator>(dbPath, "_letter")) {}

WordPattern LetterCorrelator::makePattern(const std::vector<std::string>& letters) const {
    WordPattern pat;
    for (const auto& ch : letters) {
        pat[ch] = 1.0f;
    }
    return pat;
}

void LetterCorrelator::learnWithPrevious(const std::string& text) {
    // Convert text into a vector of single-character strings
    std::vector<std::string> letters;
    for (char c : text) {
        letters.push_back(std::string(1, c));
        if(c == ' ' || c == '\n' || c == '.' || c == ';') break;
    }

    for (size_t i = 0; i < letters.size(); ++i) {
        const std::string& current = letters[i];
        std::vector<std::string> prevLetters;
        if (i >= 4) {
            prevLetters.push_back(letters[i-4]);
            prevLetters.push_back(letters[i-3]);
            prevLetters.push_back(letters[i-2]);
            prevLetters.push_back(letters[i-1]);
        } else if (i >= 3) {
            prevLetters.push_back(letters[i-3]);
            prevLetters.push_back(letters[i-2]);
            prevLetters.push_back(letters[i-1]);
        } else if (i >= 2) {
            prevLetters.push_back(letters[i-2]);
            prevLetters.push_back(letters[i-1]);
        } else if (i == 1) {
            prevLetters.push_back(letters[i-1]);
        }
        if (prevLetters.empty()) continue;

        WordPattern prevPat = makePattern(prevLetters);
        if (i + 1 < letters.size()) {
            WordPattern nextPat = makePattern({letters[i+1]});
            corr->record(current, prevPat, nextPat, 1.0f);
        }
    }
}

void LetterCorrelator::learnNextLetterDirect(const std::string& text) {
    std::vector<std::string> letters;
    for (char c : text) {
        letters.push_back(std::string(1, c));
    }

    for (size_t i = 0; i + 1 < letters.size(); ++i) {
        const std::string& current = letters[i];
        WordPattern prevPat = {{"__NO_CONTEXT__", 1.0f}};
        WordPattern nextPat = makePattern({letters[i+1]});
        corr->record(current, prevPat, nextPat, 1.0f);
    }
}

void LetterCorrelator::learnFromText(const std::string& text) {
    learnWithPrevious(text);
}

bool LetterCorrelator::queryNext(const std::string& current,
                                 const std::vector<std::string>& previousLetters,
                                 std::vector<std::pair<WordPattern, double>>& outcomes) {
    WordPattern prevPat = makePattern(previousLetters);
    return corr->query(current, prevPat, outcomes);
}

bool LetterCorrelator::GqueryNext(const std::string& word,
                                            std::vector<std::pair<WordPattern, double>>& outcomes) {
    std::vector<std::string> letters;
    for (char c : word) {
        letters.push_back(std::string(1, c));
    }

    const std::string& current = letters.back();
    std::vector<std::string> prevLetters;
    if (letters.size() >= 5) {
        prevLetters.push_back(letters[letters.size()-5]);
        prevLetters.push_back(letters[letters.size()-4]);
        prevLetters.push_back(letters[letters.size()-3]);
        prevLetters.push_back(letters[letters.size()-2]);
    } else if (letters.size() >= 4) {
        prevLetters.push_back(letters[letters.size()-4]);
        prevLetters.push_back(letters[letters.size()-3]);
        prevLetters.push_back(letters[letters.size()-2]);
    }else if (letters.size() >= 3) {
        prevLetters.push_back(letters[letters.size()-3]);
        prevLetters.push_back(letters[letters.size()-2]);
    }else if (letters.size() == 2) {
        prevLetters.push_back(letters[letters.size()-2]);
    }
    if (prevLetters.empty()) {
        prevLetters = {"__NO_CONTEXT__"};
    }

    WordPattern prevPat = makePattern(prevLetters);
    return corr->query(current, prevPat, outcomes);
}
