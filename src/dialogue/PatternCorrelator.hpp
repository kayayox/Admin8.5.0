/**
 * @file PatternCorrelator.hpp
 * @brief Correlator that stores trigram relationships (current word, previous pattern, next pattern)
 *        in its own SQLite database (patterns.db). Patterns are maps word→weight.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef PATTERN_CORRELATOR_HPP
#define PATTERN_CORRELATOR_HPP

#include "../utils/PatternUtils.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <cstdint>

class PatternCorrelator {
public:
    /**
     * @brief Constructs a PatternCorrelator.
     * @param dbPath Path to the SQLite database file.
     * @param tableSuffix Suffix for table names (e.g. "_chunk" for chunk correlations).
     */
    PatternCorrelator(const std::string& dbPath, const std::string& tableSuffix = "");
    ~PatternCorrelator();

    /**
     * @brief Records a single correlation between a current word and its context.
     * @param current      The current word.
     * @param prevPattern  Pattern of preceding context (word→weight).
     * @param nextPattern  Pattern of following context.
     * @param weight       Importance weight (default 1.0f).
     */
    void record(const std::string& current, const WordPattern& prevPattern,
                const WordPattern& nextPattern, float weight = 1.0f);

    /**
     * @brief Queries the most probable next patterns for a given current word and previous context.
     * @param current      The current word.
     * @param prevPattern  The preceding context pattern.
     * @param outcomes     Output vector of (pattern, probability) pairs, sorted by probability.
     * @return True if any outcomes were found.
     */
    bool query(const std::string& current, const WordPattern& prevPattern,
               std::vector<std::pair<WordPattern, double>>& outcomes);

    /**
     * @brief Learns correlations from raw text (trigrams). Assumes windowSize=1.
     * @param text       Input text.
     * @param windowSize Must be 1 (default).
     */
    void learnFromText(const std::string& text, size_t windowSize = 1);

    /**
     * @brief Removes correlations whose total weight is below a threshold.
     * @param minWeight Minimum total weight to keep (default 0.01).
     */
    void purgeLowWeightCorrelations(double minWeight = 0.01);

private:
    sqlite3* db;
    std::string tableSuffix;

    // Table names with suffix
    std::string getWordsTable() const { return "words" + tableSuffix; }
    std::string getPatternsTable() const { return "correlation_patterns" + tableSuffix;}
    std::string getCorrelationsTable() const { return "correlations" + tableSuffix; }

    // Caches
    std::map<std::string, int> wordToId;
    std::map<int, std::string> idToWord;
    std::map<std::string, int> patternSerializedToId;
    std::map<int, WordPattern> idToPattern;

    // Prepared statements
    sqlite3_stmt* stmtGetWordId = nullptr;
    sqlite3_stmt* stmtInsertWord = nullptr;
    sqlite3_stmt* stmtGetPatternId = nullptr;
    sqlite3_stmt* stmtInsertPattern = nullptr;
    sqlite3_stmt* stmtFindCorrelation = nullptr;
    sqlite3_stmt* stmtUpdateCorrelation = nullptr;
    sqlite3_stmt* stmtInsertCorrelation = nullptr;
    sqlite3_stmt* stmtGetTotalWeight = nullptr;
    sqlite3_stmt* stmtGetNextPatterns = nullptr;

    int getWordId(const std::string& word);
    int getPatternId(const WordPattern& pattern);
    uint32_t timestamp_;                ///< Last acces
    void updateCorrelation(int currWordId, int prevPatternId, int nextPatternId, float weight);
    void prepareStatements();  // builds SQL statements using the correct table names
};

#endif // PATTERN_CORRELATOR_HPP
