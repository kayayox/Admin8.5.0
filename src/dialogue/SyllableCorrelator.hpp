/**
 * @file SyllableCorrelator.hpp
 * @brief Correlator for syllable sequences, including word boundaries as special tokens.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef SYLLABLE_CORRELATOR_HPP
#define SYLLABLE_CORRELATOR_HPP

#include "PatternCorrelator.hpp"
#include <memory>
#include <string>
#include <vector>
#include <utility>

class SyllableCorrelator {
public:
    explicit SyllableCorrelator(const std::string& dbPath);

    // Learning: splits text into syllables (space becomes "__SPACE__")
    void learnWithPreviousTwo(const std::string& text);
    void learnNextSyllableDirect(const std::string& text);
    void learnFromText(const std::string& text);  // alias for learnWithPreviousTwo

    // Query methods
    bool queryNext(const std::string& current,
                   const std::vector<std::string>& previousSyllables,
                   std::vector<std::pair<WordPattern, double>>& outcomes);

    bool GqueryNext(const std::string& syllables,
                    std::vector<std::pair<WordPattern, double>>& outcomes);

private:
    std::unique_ptr<PatternCorrelator> corr;  // uses "_syllable" suffix
    WordPattern makePattern(const std::vector<std::string>& syllables) const;

    // Helper to split a word into syllables (basic English syllabification)
    static std::vector<std::string> splitWordIntoSyllables(const std::string& word);
    // Helper to tokenize full text into syllable tokens (including space markers)
    static std::vector<std::string> tokenizeIntoSyllables(const std::string& text);
};

#endif // SYLLABLE_CORRELATOR_HPP
