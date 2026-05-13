/**
 * @file TagStats.hpp
 * @brief Dynamic n-gram transition statistics (unigram, bigram, trigram) for tag refinement.
 *
 * Data is persisted in the pattern database and can be updated continuously.
 * Supports multiple languages; the active language is set via setLanguage().
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_TAG_STATS_HPP
#define ADMIN850_TAG_STATS_HPP

#include "../common/types.hpp"
#include <string>
#include <vector>
#include <utility>

namespace TagStats {

    /// Configure the pattern database path. Must be called before any other operation.
    void setDatabasePath(const std::string& path);

    /// Creates the necessary tables (with language column) if they do not exist.
    void initializeTables();

    /**
     * @brief Sets the current language for subsequent updates and queries.
     * @param lang Language code (e.g. "es", "en"). Defaults to "es".
     */
    void setLanguage(const std::string& lang);

    /// Returns the current language code.
    std::string getLanguage();

    // --- Counter updates ---
    void updateUnigram(WordType prev, WordType curr, int inc = 1);
    void updateBigram(WordType prev2, WordType prev, WordType curr, int inc = 1);
    void updateTrigram(WordType prev, WordType curr, WordType next, int inc = 1);

    // --- Prediction queries (use the currently active language) ---
    std::vector<std::pair<WordType, float>> getUnigramProbs(WordType prev);
    std::vector<std::pair<WordType, float>> getBigramProbs(WordType prev2, WordType prev);
    std::vector<std::pair<WordType, float>> getTrigramProbs(WordType prev, WordType next);

    /**
     * @brief Loads default n-gram data for all supported languages if the tables are empty.
     *
     * Currently loads Spanish ("es") and English ("en") default transition probabilities.
     */
    void loadDefaultFromStatic();

} // namespace TagStats

#endif // ADMIN850_TAG_STATS_HPP
