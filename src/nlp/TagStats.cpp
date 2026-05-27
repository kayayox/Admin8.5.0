/**
 * @file TagStats.cpp
 * @brief Implementation of language‑aware n‑gram tag statistics.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "TagStats.hpp"
#include "../db/DatabaseManager.hpp"
#include <sqlite3.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>

namespace TagStats {

    // ========================================================================
    // Internal state
    // ========================================================================
    static std::string patternDbPath_;
    static std::string currentLanguage_ = "es";   // Spanish by default

    // Number of WordType tags (assuming UNDEFINED is the last one)
    constexpr int kNumTags = static_cast<int>(WordType::UNDEFINED) + 1;
    constexpr float kSmoothingK = 0.001f;

    // ========================================================================
    // Database helpers
    // ========================================================================
    namespace {

        sqlite3* dbHandle() {
            return DatabaseManager::instance().getHandle(patternDbPath_);
        }

        /**
         * @brief Generic INSERT OR REPLACE for update counts.
         * Works for any table that has a 'language' column and a 'count' column.
         * The table's primary key columns are assumed to be (language, ...).
         */
        void upsertCount(const char* table,
                 const std::vector<int>& tagKeys,
                 const std::string& lang,
                 int inc = 1) {
            sqlite3* db = dbHandle();
            if (!db) return;

            // Determinar nombres de columnas según la tabla
            std::vector<std::string> colNames;
            if (std::string(table) == "tag_unigrams") {
                colNames = {"prev", "curr"};
            } else if (std::string(table) == "tag_bigrams") {
                colNames = {"prev2", "prev", "curr"};
            } else if (std::string(table) == "tag_trigrams") {
                colNames = {"prev", "curr", "next"};
            } else {
                std::cerr << "TagStats: unknown table " << table << std::endl;
                return;
            }

            // Validar que tagKeys.size() coincida
            if (tagKeys.size() != colNames.size()) {
                std::cerr << "TagStats: key count mismatch for table " << table << std::endl;
                return;
            }

            // Construir SQL: INSERT INTO table (language, col1, col2, ..., count)
            std::string sql = "INSERT INTO " + std::string(table) + " (language";
            for (const auto& col : colNames) {
                sql += ", " + col;
            }
            sql += ", count) VALUES (?";
            for (size_t i = 0; i < colNames.size() + 1; ++i) sql += ",?";
            sql += ") ON CONFLICT(language";
            for (const auto& col : colNames) {
                sql += ", " + col;
            }
            sql += ") DO UPDATE SET count = count + excluded.count";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                std::cerr << "TagStats: SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
                return;
            }
            int bindIdx = 1;
            sqlite3_bind_text(stmt, bindIdx++, lang.c_str(), -1, SQLITE_STATIC);
            for (int k : tagKeys) sqlite3_bind_int(stmt, bindIdx++, k);
            sqlite3_bind_int(stmt, bindIdx++, inc);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        /**
         * @brief Retrieves probabilities for a given context from a named table.
         * @param table  Table name (tag_unigrams, tag_bigrams, tag_trigrams).
         * @param keys   Integer keys for the WHERE clause (prev, prev2, etc.).
         * @param lang   Language filter.
         * @return Sorted list of (tag, probability) pairs.
         */
        std::vector<std::pair<WordType, float>> getProbs(const std::string& table,
                                                         const std::vector<int>& keys,
                                                         const std::string& lang) {
            std::vector<std::pair<WordType, float>> result;
            sqlite3* db = dbHandle();
            if (!db) return result;

            std::string sql = "SELECT ";
            if (table == "tag_unigrams") sql += "curr, count FROM tag_unigrams WHERE language = ? AND prev = ?";
            else if (table == "tag_bigrams") sql += "curr, count FROM tag_bigrams WHERE language = ? AND prev2 = ? AND prev = ?";
            else sql += "curr, count FROM tag_trigrams WHERE language = ? AND prev = ? AND next = ?";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                std::cerr << "TagStats: SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
                return result;
            }
            int bindIdx = 1;
            sqlite3_bind_text(stmt, bindIdx++, lang.c_str(), -1, SQLITE_STATIC);
            for (int k : keys) sqlite3_bind_int(stmt, bindIdx++, k);

            int total = 0;
            std::map<int, int> counts;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int tag = sqlite3_column_int(stmt, 0);
                int cnt = sqlite3_column_int(stmt, 1);
                counts[tag] = cnt;
                total += cnt;
            }
            sqlite3_finalize(stmt);

            // Add smoothing
            for (int t = 0; t < kNumTags; ++t) {
                int cnt = counts[t];
                float prob = (cnt + kSmoothingK) / (total + kSmoothingK * kNumTags);
                if (prob > 0.0f) {
                    result.emplace_back(static_cast<WordType>(t), prob);
                }
            }
            std::sort(result.begin(), result.end(),
                      [](const auto& a, const auto& b) { return a.second > b.second; });
            return result;
        }

    } // anonymous namespace

    // ========================================================================
    // Public API
    // ========================================================================
    void setDatabasePath(const std::string& path) {
        patternDbPath_ = path;
        DatabaseManager::instance().init(patternDbPath_);
    }

    void initializeTables() {
        sqlite3* db = dbHandle();
        if (!db) return;

        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS tag_unigrams (
                language TEXT NOT NULL,
                prev INTEGER NOT NULL,
                curr INTEGER NOT NULL,
                count INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (language, prev, curr)
            );
            CREATE TABLE IF NOT EXISTS tag_bigrams (
                language TEXT NOT NULL,
                prev2 INTEGER NOT NULL,
                prev INTEGER NOT NULL,
                curr INTEGER NOT NULL,
                count INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (language, prev2, prev, curr)
            );
            CREATE TABLE IF NOT EXISTS tag_trigrams (
                language TEXT NOT NULL,
                prev INTEGER NOT NULL,
                curr INTEGER NOT NULL,
                next INTEGER NOT NULL,
                count INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (language, prev, curr, next)
            );
        )";
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "TagStats: error creating tables: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }

    void setLanguage(const std::string& lang) {
        currentLanguage_ = lang;
    }

    std::string getLanguage() {
        return currentLanguage_;
    }

    // Updaters
    void updateUnigram(WordType prev, WordType curr, int inc) {
        upsertCount("tag_unigrams", {static_cast<int>(prev), static_cast<int>(curr)},
                     currentLanguage_, inc);
    }

    void updateBigram(WordType prev2, WordType prev, WordType curr, int inc) {
        upsertCount("tag_bigrams", {static_cast<int>(prev2), static_cast<int>(prev), static_cast<int>(curr)},
                     currentLanguage_, inc);
    }

    void updateTrigram(WordType prev, WordType curr, WordType next, int inc) {
        upsertCount("tag_trigrams", {static_cast<int>(prev), static_cast<int>(curr), static_cast<int>(next)},
                     currentLanguage_, inc);
    }

    // Queries
    std::vector<std::pair<WordType, float>> getUnigramProbs(WordType prev) {
        return getProbs("tag_unigrams", {static_cast<int>(prev)}, currentLanguage_);
    }

    std::vector<std::pair<WordType, float>> getBigramProbs(WordType prev2, WordType prev) {
        return getProbs("tag_bigrams", {static_cast<int>(prev2), static_cast<int>(prev)}, currentLanguage_);
    }

    std::vector<std::pair<WordType, float>> getTrigramProbs(WordType prev, WordType next) {
        return getProbs("tag_trigrams", {static_cast<int>(prev), static_cast<int>(next)}, currentLanguage_);
    }

    // ========================================================================
    // Default data loading
    // ========================================================================

    // ------------------------------------------------------------------------
    // Spanish default n-grams
    // ------------------------------------------------------------------------
    static void insertSpanishDefaults() {
        // Unigrams
        auto unigram = [](WordType prev, WordType curr, int c) {
            updateUnigram(prev, curr, c);
        };
        unigram(WordType::ARTICLE,         WordType::ADJECTIVE,     80);
        unigram(WordType::ARTICLE,         WordType::NOUN,          88);
        unigram(WordType::ARTICLE,         WordType::NUMERAL,       85);
        unigram(WordType::ARTICLE,         WordType::PRONOUN,       75);
        unigram(WordType::PREPOSITION,     WordType::NOUN,          45);
        unigram(WordType::PREPOSITION,     WordType::PRONOUN,       85);
        unigram(WordType::NOUN,            WordType::VERB,          45);
        unigram(WordType::NOUN,            WordType::ADJECTIVE,     70);
        unigram(WordType::VERB,            WordType::ADVERB,        40);
        unigram(WordType::VERB,            WordType::PRONOUN,       70);
        unigram(WordType::VERB,            WordType::NEGATION,     15);
        unigram(WordType::ADVERB,          WordType::ADJECTIVE,     65);
        unigram(WordType::ADVERB,          WordType::VERB,          75);
        unigram(WordType::ADVERB,          WordType::NEGATION,     10);
        unigram(WordType::PRONOUN,         WordType::VERB,          80);
        unigram(WordType::CONJUNCTION,     WordType::PRONOUN,       90);
        unigram(WordType::CONJUNCTION,     WordType::NEGATION,     25);
        unigram(WordType::CONJUNCTION,     WordType::AFIRMATION,   20);
        unigram(WordType::NUMERAL,         WordType::NOUN,          85);
        unigram(WordType::NUMERAL,         WordType::ADJECTIVE,     70);
        unigram(WordType::ADJECTIVE,       WordType::NOUN,          60);
        unigram(WordType::RELATIVE,        WordType::VERB,          50);
        unigram(WordType::DEMONSTRATIVE,   WordType::NOUN,          80);
        unigram(WordType::NEGATION,        WordType::VERB,         90);
        unigram(WordType::NEGATION,        WordType::ADJECTIVE,    25);
        unigram(WordType::NEGATION,        WordType::ADVERB,       20);
        unigram(WordType::NEGATION,        WordType::PRONOUN,       5);
        unigram(WordType::AFIRMATION,      WordType::VERB,         80);
        unigram(WordType::AFIRMATION,      WordType::ADJECTIVE,    30);
        unigram(WordType::AFIRMATION,      WordType::ADVERB,       40);
        unigram(WordType::AFIRMATION,      WordType::NOUN,         10);

        // Bigrams
        auto bigram = [](WordType prev2, WordType prev, WordType curr, int c) {
            updateBigram(prev2, prev, curr, c);
        };
        bigram(WordType::ARTICLE, WordType::NOUN,      WordType::ADJECTIVE, 75);
        bigram(WordType::ARTICLE, WordType::NOUN,      WordType::NUMERAL,   15);
        bigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::NOUN,      88);
        bigram(WordType::PREPOSITION, WordType::ARTICLE, WordType::NOUN,    75);
        bigram(WordType::PREPOSITION, WordType::NOUN,  WordType::VERB,      45);
        bigram(WordType::VERB,       WordType::ADVERB, WordType::ADJECTIVE, 45);
        bigram(WordType::VERB,       WordType::PRONOUN,WordType::VERB,      70);
        bigram(WordType::NOUN,       WordType::VERB,   WordType::ADVERB,    50);
        bigram(WordType::NOUN,       WordType::ADJECTIVE, WordType::VERB,   60);
        bigram(WordType::NOUN,         WordType::NEGATION,      WordType::VERB,      60);
        bigram(WordType::VERB,         WordType::NEGATION,      WordType::VERB,      20);
        bigram(WordType::VERB,         WordType::AFIRMATION,    WordType::VERB,      30);
        bigram(WordType::CONJUNCTION,  WordType::NEGATION,      WordType::ADJECTIVE, 40);
        bigram(WordType::ADVERB,     WordType::VERB,   WordType::ADJECTIVE, 65);
        bigram(WordType::PRONOUN,    WordType::VERB,   WordType::ADVERB,    50);
        bigram(WordType::CONJUNCTION, WordType::PRONOUN, WordType::VERB,    80);
        bigram(WordType::NUMERAL,    WordType::NOUN,   WordType::ADJECTIVE, 65);
        bigram(WordType::INTERROGATIVE, WordType::VERB, WordType::ADVERB,   60);
        bigram(WordType::UNDEFINED,    WordType::NEGATION,      WordType::VERB,      85);
        bigram(WordType::UNDEFINED,    WordType::AFIRMATION,    WordType::VERB,      75);
        bigram(WordType::PRONOUN,      WordType::NEGATION,      WordType::VERB,      70);

        // Trigrams
        auto trigram = [](WordType prev, WordType curr, WordType next, int c) {
            updateTrigram(prev, curr, next, c);
        };
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::ADJECTIVE, 75);
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::NUMERAL,   15);
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::PRONOUN,    5);
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::DEMONSTRATIVE, 2);
        trigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::NOUN,      88);
        trigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::NUMERAL,    6);
        trigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::ADVERB,     3);
        trigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::CONJUNCTION,2);
        trigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::PRONOUN,    1);
        trigram(WordType::ARTICLE, WordType::VERB,      WordType::NOUN,      80);
        trigram(WordType::ARTICLE, WordType::VERB,      WordType::PRONOUN,   12);
        trigram(WordType::ARTICLE, WordType::VERB,      WordType::ADJECTIVE,  5);
        trigram(WordType::ARTICLE, WordType::VERB,      WordType::NUMERAL,    3);
        trigram(WordType::ARTICLE, WordType::PRONOUN,   WordType::NOUN,      70);
        trigram(WordType::ARTICLE, WordType::PRONOUN,   WordType::ADJECTIVE, 20);
        trigram(WordType::ARTICLE, WordType::PRONOUN,   WordType::NUMERAL,   10);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::ARTICLE,   75);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::NUMERAL,   10);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::PRONOUN,    8);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::DEMONSTRATIVE,5);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::ADJECTIVE,  2);
        trigram(WordType::PREPOSITION, WordType::VERB,  WordType::ARTICLE,   50);
        trigram(WordType::PREPOSITION, WordType::VERB,  WordType::PRONOUN,   25);
        trigram(WordType::PREPOSITION, WordType::VERB,  WordType::NOUN,      15);
        trigram(WordType::PREPOSITION, WordType::VERB,  WordType::DEMONSTRATIVE,5);
        trigram(WordType::PREPOSITION, WordType::VERB,  WordType::NUMERAL,    5);
        trigram(WordType::PREPOSITION, WordType::PRONOUN, WordType::VERB,    70);
        trigram(WordType::PREPOSITION, WordType::PRONOUN, WordType::PREPOSITION,15);
        trigram(WordType::PREPOSITION, WordType::PRONOUN, WordType::ADVERB,  10);
        trigram(WordType::PREPOSITION, WordType::PRONOUN, WordType::CONJUNCTION,5);
        trigram(WordType::VERB, WordType::VERB,        WordType::ADVERB,     50);
        trigram(WordType::VERB, WordType::VERB,        WordType::CONJUNCTION,25);
        trigram(WordType::VERB, WordType::VERB,        WordType::PRONOUN,    15);
        trigram(WordType::VERB, WordType::VERB,        WordType::PREPOSITION,10);
        trigram(WordType::VERB, WordType::ADJECTIVE,   WordType::ADVERB,     55);
        trigram(WordType::VERB, WordType::ADJECTIVE,   WordType::VERB,       20);
        trigram(WordType::VERB, WordType::ADJECTIVE,   WordType::PRONOUN,    15);
        trigram(WordType::VERB, WordType::ADJECTIVE,   WordType::PREPOSITION,10);
        trigram(WordType::VERB, WordType::ADVERB,      WordType::ADJECTIVE,  45);
        trigram(WordType::VERB, WordType::ADVERB,      WordType::VERB,       25);
        trigram(WordType::VERB, WordType::ADVERB,      WordType::PREPOSITION,15);
        trigram(WordType::VERB, WordType::ADVERB,      WordType::PRONOUN,    10);
        trigram(WordType::VERB, WordType::ADVERB,      WordType::CONJUNCTION, 5);
        trigram(WordType::VERB, WordType::NOUN,        WordType::ARTICLE,    50);
        trigram(WordType::VERB, WordType::NOUN,        WordType::PREPOSITION,25);
        trigram(WordType::VERB, WordType::NOUN,        WordType::PRONOUN,    15);
        trigram(WordType::VERB, WordType::NOUN,        WordType::ADVERB,     10);
        trigram(WordType::NOUN, WordType::VERB,        WordType::ADJECTIVE,  55);
        trigram(WordType::NOUN, WordType::VERB,        WordType::PREPOSITION,25);
        trigram(WordType::NOUN, WordType::VERB,        WordType::PRONOUN,    10);
        trigram(WordType::NOUN, WordType::VERB,        WordType::CONJUNCTION, 7);
        trigram(WordType::NOUN, WordType::VERB,        WordType::ADVERB,      3);
        trigram(WordType::NOUN, WordType::ADJECTIVE,   WordType::VERB,       60);
        trigram(WordType::NOUN, WordType::ADJECTIVE,   WordType::PREPOSITION,30);
        trigram(WordType::NOUN, WordType::ADJECTIVE,   WordType::CONJUNCTION,15);
        trigram(WordType::NOUN, WordType::ADJECTIVE,   WordType::ADVERB,      5);
        trigram(WordType::ADVERB, WordType::ADVERB,    WordType::CONJUNCTION,45);
        trigram(WordType::ADVERB, WordType::ADVERB,    WordType::ADJECTIVE,  30);
        trigram(WordType::ADVERB, WordType::ADVERB,    WordType::VERB,       15);
        trigram(WordType::ADVERB, WordType::ADVERB,    WordType::PREPOSITION,10);
        trigram(WordType::ADVERB, WordType::VERB,      WordType::ADJECTIVE,  65);
        trigram(WordType::ADVERB, WordType::VERB,      WordType::ADVERB,     15);
        trigram(WordType::ADVERB, WordType::VERB,      WordType::PREPOSITION,10);
        trigram(WordType::ADVERB, WordType::VERB,      WordType::CONJUNCTION,10);
        trigram(WordType::ADVERB, WordType::ADJECTIVE, WordType::NOUN,       60);
        trigram(WordType::ADVERB, WordType::ADJECTIVE, WordType::VERB,       25);
        trigram(WordType::ADVERB, WordType::ADJECTIVE, WordType::ADVERB,     10);
        trigram(WordType::ADVERB, WordType::ADJECTIVE, WordType::PREPOSITION, 5);
        trigram(WordType::PRONOUN, WordType::PRONOUN,  WordType::VERB,       85);
        trigram(WordType::PRONOUN, WordType::PRONOUN,  WordType::ADVERB,      8);
        trigram(WordType::PRONOUN, WordType::PRONOUN,  WordType::CONJUNCTION, 5);
        trigram(WordType::PRONOUN, WordType::PRONOUN,  WordType::PREPOSITION, 2);
        trigram(WordType::PRONOUN, WordType::VERB,     WordType::ADVERB,     50);
        trigram(WordType::PRONOUN, WordType::VERB,     WordType::PREPOSITION,25);
        trigram(WordType::PRONOUN, WordType::VERB,     WordType::CONJUNCTION,15);
        trigram(WordType::PRONOUN, WordType::VERB,     WordType::PRONOUN,    10);
        trigram(WordType::CONJUNCTION, WordType::NOUN, WordType::VERB,       60);
        trigram(WordType::CONJUNCTION, WordType::NOUN, WordType::ADJECTIVE,  20);
        trigram(WordType::CONJUNCTION, WordType::NOUN, WordType::PREPOSITION,10);
        trigram(WordType::CONJUNCTION, WordType::NOUN, WordType::ADVERB,     10);
        trigram(WordType::ADJECTIVE, WordType::NOUN,   WordType::ADVERB,     55);
        trigram(WordType::ADJECTIVE, WordType::NOUN,   WordType::QUANTIFIER, 20);
        trigram(WordType::ADJECTIVE, WordType::NOUN,   WordType::VERB,       15);
        trigram(WordType::ADJECTIVE, WordType::NOUN,   WordType::CONJUNCTION,10);
        trigram(WordType::ADJECTIVE, WordType::VERB,   WordType::ADVERB,     60);
        trigram(WordType::ADJECTIVE, WordType::VERB,   WordType::PREPOSITION,20);
        trigram(WordType::ADJECTIVE, WordType::VERB,   WordType::CONJUNCTION,15);
        trigram(WordType::ADJECTIVE, WordType::VERB,   WordType::PRONOUN,     5);
        trigram(WordType::NUMERAL, WordType::NOUN,     WordType::ADJECTIVE,  65);
        trigram(WordType::NUMERAL, WordType::NOUN,     WordType::ARTICLE,    20);
        trigram(WordType::NUMERAL, WordType::NOUN,     WordType::NUMERAL,    10);
        trigram(WordType::NUMERAL, WordType::NOUN,     WordType::PREPOSITION, 5);
        trigram(WordType::NUMERAL, WordType::ADJECTIVE,WordType::NOUN,       80);
        trigram(WordType::NUMERAL, WordType::ADJECTIVE,WordType::NUMERAL,    10);
        trigram(WordType::NUMERAL, WordType::ADJECTIVE,WordType::PREPOSITION,10);
        trigram(WordType::INTERROGATIVE, WordType::NOUN,WordType::ADJECTIVE, 75);
        trigram(WordType::INTERROGATIVE, WordType::NOUN,WordType::NUMERAL,   15);
        trigram(WordType::INTERROGATIVE, WordType::NOUN,WordType::VERB,      10);
        trigram(WordType::INTERROGATIVE, WordType::VERB,WordType::ADVERB,    60);
        trigram(WordType::INTERROGATIVE, WordType::VERB,WordType::NOUN,      30);
        trigram(WordType::INTERROGATIVE, WordType::VERB,WordType::ADJECTIVE, 10);
        trigram(WordType::UNDEFINED,       WordType::NEGATION,      WordType::VERB,      85);
        trigram(WordType::UNDEFINED,       WordType::AFIRMATION,    WordType::VERB,      75);
        trigram(WordType::PRONOUN,     WordType::NEGATION,      WordType::VERB,      70);
        trigram(WordType::NOUN,        WordType::NEGATION,      WordType::VERB,      60);
        trigram(WordType::NEGATION,    WordType::VERB,          WordType::ADVERB,    30);
        trigram(WordType::NEGATION,    WordType::VERB,          WordType::NOUN,      25);
        trigram(WordType::NEGATION,    WordType::VERB,          WordType::PRONOUN,   10);
        trigram(WordType::AFIRMATION,  WordType::VERB,          WordType::ADVERB,    40);
        trigram(WordType::AFIRMATION,  WordType::VERB,          WordType::NOUN,      20);
        trigram(WordType::NEGATION,    WordType::ADJECTIVE,     WordType::CONJUNCTION, 15);
    }

    // ------------------------------------------------------------------------
    // English default n-grams (based on typical English POS sequences)
    // REVISED & EXPANDED
    // ------------------------------------------------------------------------
    static void insertEnglishDefaults() {
        // Unigrams (prev -> curr)
        auto unigram = [](WordType prev, WordType curr, int c) {
            updateUnigram(prev, curr, c);
        };
        unigram(WordType::ARTICLE,        WordType::ADJECTIVE,    75);
        unigram(WordType::ARTICLE,        WordType::NOUN,         90);
        // unigram(WordType::ARTICLE,     WordType::ADVERB,      10); // REMOVED - improbable
        unigram(WordType::PREPOSITION,    WordType::ARTICLE,      60);
        unigram(WordType::PREPOSITION,    WordType::NOUN,         35);
        unigram(WordType::PREPOSITION,    WordType::PRONOUN,      25);
        unigram(WordType::NOUN,           WordType::VERB,         55);
        unigram(WordType::NOUN,           WordType::PREPOSITION,  25);
        unigram(WordType::NOUN,           WordType::CONJUNCTION,  15);
        unigram(WordType::VERB,           WordType::ADVERB,       30);
        unigram(WordType::VERB,           WordType::PREPOSITION,  30);
        unigram(WordType::VERB,           WordType::NOUN,         35);
        unigram(WordType::VERB,           WordType::PRONOUN,      45);
        unigram(WordType::VERB,           WordType::ARTICLE,      20);
        unigram(WordType::ADJECTIVE,      WordType::NOUN,         80);
        unigram(WordType::ADJECTIVE,      WordType::CONJUNCTION,  10);
        unigram(WordType::ADJECTIVE,      WordType::ADVERB,        5);
        unigram(WordType::ADVERB,         WordType::ADJECTIVE,    50);
        unigram(WordType::ADVERB,         WordType::VERB,         30);
        unigram(WordType::ADVERB,         WordType::ADVERB,       20);
        unigram(WordType::PRONOUN,        WordType::VERB,         70);
        unigram(WordType::PRONOUN,        WordType::ADJECTIVE,     3);
        unigram(WordType::CONJUNCTION,    WordType::PRONOUN,      40);
        unigram(WordType::CONJUNCTION,    WordType::NOUN,         30);
        unigram(WordType::CONJUNCTION,    WordType::VERB,         20);
        unigram(WordType::NUMERAL,        WordType::NOUN,         85);
        unigram(WordType::NUMERAL,        WordType::ADJECTIVE,    10);
        unigram(WordType::DEMONSTRATIVE,  WordType::NOUN,         80);
        unigram(WordType::DEMONSTRATIVE,  WordType::ADJECTIVE,    15);
        unigram(WordType::INTERROGATIVE,  WordType::VERB,         60);
        unigram(WordType::INTERROGATIVE,  WordType::NOUN,         25);
        unigram(WordType::NEGATION,    WordType::VERB,         85);
        unigram(WordType::NEGATION,    WordType::ADJECTIVE,    30);
        unigram(WordType::NEGATION,    WordType::ADVERB,       25);
        unigram(WordType::AFIRMATION,  WordType::VERB,         75);
        unigram(WordType::AFIRMATION,  WordType::ADJECTIVE,    20);
        unigram(WordType::AFIRMATION,  WordType::ADVERB,       35);
        unigram(WordType::AFIRMATION,  WordType::NOUN,          8);
        unigram(WordType::VERB,        WordType::NEGATION,     12);
        unigram(WordType::ADVERB,      WordType::NEGATION,      8);
        unigram(WordType::CONJUNCTION, WordType::NEGATION,     20);
        unigram(WordType::CONJUNCTION, WordType::AFIRMATION,   15);
        // Bigrams (prev2, prev, curr)
        auto bigram = [](WordType prev2, WordType prev, WordType curr, int c) {
            updateBigram(prev2, prev, curr, c);
        };
        bigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::NOUN,      85);
        bigram(WordType::ARTICLE, WordType::NOUN,      WordType::VERB,      35);
        bigram(WordType::ARTICLE, WordType::NOUN,      WordType::PREPOSITION,30);
        bigram(WordType::PREPOSITION, WordType::ARTICLE, WordType::NOUN,    70);
        bigram(WordType::PREPOSITION, WordType::NOUN,  WordType::VERB,      30);
        bigram(WordType::NOUN, WordType::VERB,         WordType::ADVERB,    25);
        bigram(WordType::NOUN, WordType::VERB,         WordType::PREPOSITION,25);
        bigram(WordType::NOUN, WordType::VERB,         WordType::NOUN,      30);
        bigram(WordType::NOUN, WordType::PREPOSITION,  WordType::ARTICLE,   50);
        bigram(WordType::NOUN, WordType::PREPOSITION,  WordType::NOUN,      25);
        bigram(WordType::VERB, WordType::ADVERB,       WordType::ADJECTIVE, 40);
        bigram(WordType::VERB, WordType::ADVERB,       WordType::VERB,      30);
        bigram(WordType::VERB, WordType::PREPOSITION,  WordType::ARTICLE,   50);
        bigram(WordType::VERB, WordType::PREPOSITION,  WordType::NOUN,      30);
        bigram(WordType::VERB, WordType::PREPOSITION,  WordType::PRONOUN,   20);
        bigram(WordType::VERB, WordType::PRONOUN,      WordType::VERB,      15);
        bigram(WordType::ADJECTIVE, WordType::NOUN,    WordType::VERB,      45);
        bigram(WordType::ADJECTIVE, WordType::NOUN,    WordType::PREPOSITION,30);
        bigram(WordType::ADVERB, WordType::ADJECTIVE,  WordType::NOUN,      70);
        bigram(WordType::ADVERB, WordType::VERB,       WordType::ADVERB,    30);
        bigram(WordType::ADVERB, WordType::VERB,       WordType::PREPOSITION,20);
        bigram(WordType::PRONOUN, WordType::VERB,      WordType::ADVERB,    40);
        bigram(WordType::PRONOUN, WordType::VERB,      WordType::NOUN,      30);
        bigram(WordType::PRONOUN, WordType::VERB,      WordType::PREPOSITION,30);
        bigram(WordType::CONJUNCTION, WordType::PRONOUN, WordType::VERB,    70);
        bigram(WordType::DEMONSTRATIVE, WordType::NOUN, WordType::VERB,     50);
        bigram(WordType::INTERROGATIVE, WordType::VERB, WordType::PRONOUN,  35);
        bigram(WordType::NUMERAL, WordType::ADJECTIVE, WordType::NOUN,      40);
        bigram(WordType::UNDEFINED,        WordType::NEGATION,      WordType::VERB,      80);
        bigram(WordType::UNDEFINED,        WordType::AFIRMATION,    WordType::VERB,      70);
        bigram(WordType::PRONOUN,      WordType::NEGATION,      WordType::VERB,      75);
        bigram(WordType::NOUN,         WordType::NEGATION,      WordType::VERB,      55);
        bigram(WordType::VERB,         WordType::NEGATION,      WordType::VERB,      15);
        bigram(WordType::VERB,         WordType::AFIRMATION,    WordType::VERB,      25);
        bigram(WordType::CONJUNCTION,  WordType::NEGATION,      WordType::ADJECTIVE, 35);

        // Trigrams (prev, curr, next)
        auto trigram = [](WordType prev, WordType curr, WordType next, int c) {
            updateTrigram(prev, curr, next, c);
        };
        trigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::NOUN,      85);
        trigram(WordType::ARTICLE, WordType::ADJECTIVE, WordType::ADVERB,     5);
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::VERB,      40);
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::PREPOSITION,35);
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::CONJUNCTION,15);
        trigram(WordType::ARTICLE, WordType::NOUN,      WordType::ADVERB,    10);
        trigram(WordType::PREPOSITION, WordType::ARTICLE, WordType::NOUN,    75);
        trigram(WordType::PREPOSITION, WordType::ARTICLE, WordType::ADJECTIVE,40);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::VERB,      25);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::PREPOSITION,20);
        trigram(WordType::PREPOSITION, WordType::NOUN,  WordType::CONJUNCTION,15);
        trigram(WordType::NOUN, WordType::PREPOSITION,  WordType::ARTICLE,   30);
        trigram(WordType::NOUN, WordType::VERB,         WordType::ADVERB,    25);
        trigram(WordType::NOUN, WordType::VERB,         WordType::PREPOSITION,25);
        trigram(WordType::NOUN, WordType::VERB,         WordType::NOUN,      30);
        trigram(WordType::NOUN, WordType::VERB,         WordType::PRONOUN,   10);
        trigram(WordType::VERB, WordType::ADVERB,       WordType::ADJECTIVE, 35);
        trigram(WordType::VERB, WordType::ADVERB,       WordType::VERB,      30);
        trigram(WordType::VERB, WordType::ADVERB,       WordType::PREPOSITION,20);
        trigram(WordType::VERB, WordType::PREPOSITION,  WordType::ARTICLE,   60);
        trigram(WordType::VERB, WordType::PREPOSITION,  WordType::NOUN,      30);
        trigram(WordType::VERB, WordType::NOUN,         WordType::ADVERB,    25);
        trigram(WordType::VERB, WordType::NOUN,         WordType::VERB,      25);
        trigram(WordType::VERB, WordType::NOUN,         WordType::PREPOSITION,25);
        trigram(WordType::VERB, WordType::PRONOUN,      WordType::ADVERB,    15);
        trigram(WordType::PRONOUN, WordType::VERB,      WordType::PRONOUN,   25);
        trigram(WordType::PRONOUN, WordType::VERB,      WordType::ADVERB,    40);
        trigram(WordType::PRONOUN, WordType::VERB,      WordType::NOUN,      25);
        trigram(WordType::PRONOUN, WordType::VERB,      WordType::PREPOSITION,20);
        trigram(WordType::ADJECTIVE, WordType::NOUN,    WordType::VERB,      45);
        trigram(WordType::ADJECTIVE, WordType::NOUN,    WordType::PREPOSITION,25);
        trigram(WordType::ADJECTIVE, WordType::NOUN,    WordType::CONJUNCTION,20);
        trigram(WordType::ADVERB, WordType::ADJECTIVE,  WordType::NOUN,      70);
        trigram(WordType::ADVERB, WordType::VERB,       WordType::ADVERB,    30);
        trigram(WordType::ADVERB, WordType::VERB,       WordType::ADJECTIVE, 30);
        trigram(WordType::CONJUNCTION, WordType::PRONOUN, WordType::VERB,    70);
        trigram(WordType::CONJUNCTION, WordType::NOUN,  WordType::VERB,      40);
        trigram(WordType::CONJUNCTION, WordType::VERB,  WordType::ADVERB,    20);
        trigram(WordType::DEMONSTRATIVE, WordType::NOUN, WordType::VERB,     45);
        trigram(WordType::DEMONSTRATIVE, WordType::NOUN, WordType::PREPOSITION,25);
        trigram(WordType::INTERROGATIVE, WordType::VERB, WordType::PRONOUN,  40);
        trigram(WordType::INTERROGATIVE, WordType::VERB, WordType::NOUN,     30);
        trigram(WordType::INTERROGATIVE, WordType::VERB, WordType::ADVERB,   20);
        trigram(WordType::UNDEFINED,       WordType::NEGATION,      WordType::VERB,      80);
        trigram(WordType::UNDEFINED,       WordType::AFIRMATION,    WordType::VERB,      70);
        trigram(WordType::PRONOUN,     WordType::NEGATION,      WordType::VERB,      75);
        trigram(WordType::NOUN,        WordType::NEGATION,      WordType::VERB,      55);
        trigram(WordType::NEGATION,    WordType::VERB,          WordType::ADVERB,    40);
        trigram(WordType::NEGATION,    WordType::VERB,          WordType::NOUN,      30);
        trigram(WordType::NEGATION,    WordType::VERB,          WordType::PRONOUN,   15);
        trigram(WordType::AFIRMATION,  WordType::VERB,          WordType::ADVERB,    45);
        trigram(WordType::AFIRMATION,  WordType::VERB,          WordType::NOUN,      25);
        trigram(WordType::NEGATION,    WordType::ADJECTIVE,     WordType::CONJUNCTION, 20);
    }

    void loadDefaultFromStatic() {
        sqlite3* db = dbHandle();
        if (!db) return;

        // Only load if tables are empty (check tag_unigrams)
        sqlite3_stmt* check = nullptr;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tag_unigrams", -1, &check, nullptr);
        int count = 0;
        if (sqlite3_step(check) == SQLITE_ROW) count = sqlite3_column_int(check, 0);
        sqlite3_finalize(check);
        if (count > 0) return;

        // Temporarily switch to Spanish to insert default data
        std::string savedLang = currentLanguage_;
        setLanguage("es");
        insertSpanishDefaults();
        setLanguage("en");
        insertEnglishDefaults();
        setLanguage(savedLang);
    }

} // namespace TagStats
