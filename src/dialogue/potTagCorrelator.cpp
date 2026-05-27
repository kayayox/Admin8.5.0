/**
 * @file PatternSequenceCorrelator.cpp
 * @brief Implementation of the sequence correlator.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "postTagCorrelator.hpp"
#include "../utils/StringConversions.hpp"
#include <sstream>

PostTagCorrelator::PostTagCorrelator(const std::string& dbPath)
    : corr(std::make_unique<PatternCorrelator>(dbPath, "_WordType")) {}

WordPattern PostTagCorrelator::makePattern(const std::vector<std::string>& patterns) const {
    WordPattern pat;
    for (const auto& p : patterns) {
        pat[p] = 1.0f;
    }
    return pat;
}

void PostTagCorrelator::learnWithPrevious(const std::vector<WordType>& pattern) {
    if(pattern.empty()) return;
    std::vector<std::string> words;
    for(auto p : pattern){
        words.push_back(wordTypeToString(p));
    }

    for (size_t i = 0; i < words.size(); ++i) {
        const std::string& current = words[i];
        std::vector<std::string> prevWords;
        if (i >= 4) {
            prevWords.push_back(words[i-4]);
            prevWords.push_back(words[i-3]);
            prevWords.push_back(words[i-2]);
            prevWords.push_back(words[i-1]);
        } else if (i >= 3) {
            prevWords.push_back(words[i-3]);
            prevWords.push_back(words[i-2]);
            prevWords.push_back(words[i-1]);
        }else if (i >= 2) {
            prevWords.push_back(words[i-2]);
            prevWords.push_back(words[i-1]);
        } else if (i == 1) {
            prevWords.push_back(words[i-1]);
        }
        if (prevWords.empty()) continue;

        WordPattern prevPat = makePattern(prevWords);
        if (i + 1 < words.size()) {
            WordPattern nextPat = makePattern({words[i+1]});
            corr->record(current, prevPat, nextPat, 1.0f);
        }
    }
}

void PostTagCorrelator::learnNextWordDirect(const std::vector<WordType>& pattern) {
    if(pattern.empty()) return;
    std::vector<std::string> words;
    for(auto p : pattern){
        words.push_back(wordTypeToString(p));
    }
    for (size_t i = 0; i + 1 < words.size(); ++i) {
        const std::string& current = words[i];
        WordPattern prevPat = {{"__NO_CONTEXT__", 1.0f}};
        WordPattern nextPat = makePattern({words[i+1]});
        corr->record(current, prevPat, nextPat, 1.0f);
    }
}

bool PostTagCorrelator::queryNext(const std::string& current,
                                     const std::vector<std::string>& previousWords,
                                     std::vector<std::pair<WordPattern, double>>& outcomes) {
    WordPattern prevPat = makePattern(previousWords);
    return corr->query(current, prevPat, outcomes);
}

bool PostTagCorrelator::GqueryNext(const std::string& phrase, std::vector<std::pair<WordPattern, double>>& outcomes){
    std::vector<std::string> words;
    std::stringstream ss(phrase);
    std::string w;
    while (ss >> w) words.push_back(w);
    const std::string& current = words.back();
    std::vector<std::string> prevWords;
    if (words.size() >= 5) {
        prevWords.push_back(words[words.size()-5]);
        prevWords.push_back(words[words.size()-4]);
        prevWords.push_back(words[words.size()-3]);
        prevWords.push_back(words[words.size()-2]);
    } else if (words.size() >= 4) {
        prevWords.push_back(words[words.size()-4]);
        prevWords.push_back(words[words.size()-3]);
        prevWords.push_back(words[words.size()-2]);
    }else if (words.size() >= 3) {
        prevWords.push_back(words[words.size()-3]);
        prevWords.push_back(words[words.size()-2]);
    }else if (words.size() >= 2) {
        prevWords.push_back(words[words.size()-2]);
    }
    if (prevWords.empty()) {
        prevWords = {"__NO_CONTEXT__"};
    }

    WordPattern prevPat = makePattern(prevWords);
    return corr->query(current, prevPat, outcomes);
}
