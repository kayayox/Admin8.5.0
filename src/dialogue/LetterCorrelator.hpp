/**
 * @file LetterCorrelator.hpp
 * @brief Correlator for letter sequences (character‑level), with and without context.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef LETTER_CORRELATOR_HPP
#define LETTER_CORRELATOR_HPP

#include "PatternCorrelator.hpp"
#include <memory>
#include <string>
#include <vector>
#include <utility>

class LetterCorrelator {
public:
    explicit LetterCorrelator(const std::string& dbPath);

    // Learning from a text: splits into letters (including spaces)
    void learnWithPrevious(const std::string& text);
    void learnNextLetterDirect(const std::string& text);
    void learnFromText(const std::string& text);  // alias for learnWithPreviousTwo

    // Query methods
    bool queryNext(const std::string& current,
                   const std::vector<std::string>& previousLetters,
                   std::vector<std::pair<WordPattern, double>>& outcomes);

    bool GqueryNext(const std::string& word,
                              std::vector<std::pair<WordPattern, double>>& outcomes);


private:
    std::unique_ptr<PatternCorrelator> corr;  // uses "_letter" suffix
    WordPattern makePattern(const std::vector<std::string>& letters) const;
};

#endif // LETTER_CORRELATOR_HPP
