/**
 * @file PatternSequenceCorrelator.hpp
 * @brief Convenience layer over PatternCorrelator for wordType-level context (up to five preceding Types).
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef PATTERNSEQUENCECORRELATOR_HPP_INCLUDED
#define PATTERNSEQUENCECORRELATOR_HPP_INCLUDED

#include "PatternCorrelator.hpp"
#include "../common/types.hpp"
#include "../utils/PatternUtils.hpp"
#include <memory>
#include <string>
#include <vector>

class PatternSequenceCorrelator {
public:
    explicit PatternSequenceCorrelator(const std::string& dbPath);

    void learnWithPrevious(const std::vector<WordType>& pattern);
    void learnNextWordDirect(const std::vector<WordType>& pattern);

    bool queryNext(const std::string& current,
                   const std::vector<std::string>& previousWords,
                   std::vector<std::pair<WordPattern, double>>& outcomes);

    bool GqueryNext(const std::string& phrase,
                   std::vector<std::pair<WordPattern, double>>& outcomes);

private:
    std::unique_ptr<PatternCorrelator> corr;  // uses default suffix ""
    WordPattern makePattern(const std::vector<std::string>& words) const;
};

#endif // PATTERNSEQUENCECORRELATOR_HPP_INCLUDED
