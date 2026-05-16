/**
 * @file WordRepository.cpp
 * @brief Implementation of Word persistence using SQLite.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "WordRepository.hpp"
#include "DatabaseManager.hpp"
#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <ctime>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>

/**
 * @brief Retrieves the internal ID of a word, optionally creating a new record.
 * Ahora también inicializa frequency y last_used al crear.
 */
static int getWordId(sqlite3* db, const std::string& wordText, bool createIfMissing = true) {
    // Look up existing ID
    const char* sqlSelect = "SELECT id FROM words WHERE word = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlSelect, &stmt)) return -1;
    sqlite3_bind_text(stmt, 1, wordText.c_str(), -1, SQLITE_STATIC);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    // Insert a minimal row if needed
    if (id == -1 && createIfMissing) {
        const char* sqlInsert = "INSERT INTO words (word, frequency, last_used) VALUES (?, 1, CURRENT_TIMESTAMP)";
        if (!prepareSqlStatement(db, sqlInsert, &stmt)) return -1;
        sqlite3_bind_text(stmt, 1, wordText.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "WordRepository: error inserting word placeholder: "
                      << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            return -1;
        }
        id = static_cast<int>(sqlite3_last_insert_rowid(db));
        sqlite3_finalize(stmt);
    }
    return id;
}

// ============================================================================
// Static members
// ============================================================================
std::string WordRepository::dbPath_;

// ============================================================================
// Public interface
// ============================================================================
void WordRepository::setDatabasePath(const std::string& path) {
    dbPath_ = path;
    DatabaseManager::instance().init(dbPath_);
}

std::string WordRepository::getDatabasePath() {
    return dbPath_;
}

void WordRepository::initializeTables() {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    const char* sql =
        "CREATE TABLE IF NOT EXISTS words ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "word TEXT UNIQUE NOT NULL,"
        "meaning TEXT,"
        "type INTEGER,"
        "quantity INTEGER,"
        "tense INTEGER,"
        "gender INTEGER,"
        "degree INTEGER,"
        "person INTEGER,"
        "confidence REAL,"
        "frequency INTEGER DEFAULT 1,"
        "last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS related_words ("
        "word_id INTEGER, related_word_id INTEGER, weight REAL,"
        "FOREIGN KEY(word_id) REFERENCES words(id) ON DELETE CASCADE,"
        "FOREIGN KEY(related_word_id) REFERENCES words(id) ON DELETE CASCADE,"
        "PRIMARY KEY (word_id, related_word_id)"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "WordRepository: error creating tables: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void WordRepository::save(const Word& word) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) {
        std::cerr << "WordRepository::save: semantic database not initialized" << std::endl;
        return;
    }

    // 1. Upsert main word data, ahora incrementando frequency y actualizando last_used
    const char* sqlUpsert =
        "INSERT INTO words (word, meaning, type, quantity, tense, gender, degree, person, confidence, frequency, last_used) "
        "VALUES (?,?,?,?,?,?,?,?,?,?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(word) DO UPDATE SET "
        "meaning=excluded.meaning, type=excluded.type, quantity=excluded.quantity, "
        "tense=excluded.tense, gender=excluded.gender, degree=excluded.degree, "
        "person=excluded.person, confidence=excluded.confidence, "
        "frequency = frequency + 1, last_used = CURRENT_TIMESTAMP";

    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlUpsert, &stmt)) return;

    sqlite3_bind_text(stmt, 1, word.getWord().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, word.getMeaning().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(word.getType()));
    sqlite3_bind_int(stmt, 4, static_cast<int>(word.getQuantity()));
    sqlite3_bind_int(stmt, 5, static_cast<int>(word.getTense()));
    sqlite3_bind_int(stmt, 6, static_cast<int>(word.getGender()));
    sqlite3_bind_int(stmt, 7, static_cast<int>(word.getDegree()));
    sqlite3_bind_int(stmt, 8, static_cast<int>(word.getPerson()));
    sqlite3_bind_double(stmt, 9, word.getConfidence());
    sqlite3_bind_int(stmt, 10, word.getFrequency());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "WordRepository::save: error upserting word: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    // 2. Obtain the internal word ID
    int wordId = getWordId(db, word.getWord(), false);
    if (wordId == -1) {
        std::cerr << "WordRepository::save: could not retrieve ID for " << word.getWord() << std::endl;
        return;
    }

    // 3. Merge related words from the entity and existing DB records (sin cambios)
    std::map<std::string, double> mergedRels;
    for (const auto& [relWord, weight] : word.getRelated()) {
        mergedRels[relWord] = std::max(mergedRels[relWord], weight);
    }
    const char* sqlLoadRels =
        "SELECT w2.word, rw.weight FROM related_words rw "
        "JOIN words w2 ON rw.related_word_id = w2.id "
        "WHERE rw.word_id = ?";
    sqlite3_stmt* stmtLoad = nullptr;
    if (prepareSqlStatement(db, sqlLoadRels, &stmtLoad)) {
        sqlite3_bind_int(stmtLoad, 1, wordId);
        while (sqlite3_step(stmtLoad) == SQLITE_ROW) {
            std::string relWord = reinterpret_cast<const char*>(sqlite3_column_text(stmtLoad, 0));
            double weight = sqlite3_column_double(stmtLoad, 1);
            mergedRels[relWord] = std::max(mergedRels[relWord], weight);
        }
        sqlite3_finalize(stmtLoad);
    }
    std::vector<std::pair<std::string, double>> topRels(mergedRels.begin(), mergedRels.end());
    std::sort(topRels.begin(), topRels.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    if (topRels.size() > 15) topRels.resize(15);

    const char* sqlDeleteRels = "DELETE FROM related_words WHERE word_id = ?";
    sqlite3_stmt* stmtDel = nullptr;
    if (prepareSqlStatement(db, sqlDeleteRels, &stmtDel)) {
        sqlite3_bind_int(stmtDel, 1, wordId);
        sqlite3_step(stmtDel);
        sqlite3_finalize(stmtDel);
    }
    const char* sqlInsertRel = "INSERT INTO related_words (word_id, related_word_id, weight) VALUES (?, ?, ?)";
    for (const auto& [relWord, weight] : topRels) {
        int relId = getWordId(db, relWord, true);
        if (relId == -1) continue;
        sqlite3_stmt* stmtIns = nullptr;
        if (!prepareSqlStatement(db, sqlInsertRel, &stmtIns)) continue;
        sqlite3_bind_int(stmtIns, 1, wordId);
        sqlite3_bind_int(stmtIns, 2, relId);
        sqlite3_bind_double(stmtIns, 3, weight);
        sqlite3_step(stmtIns);
        sqlite3_finalize(stmtIns);
    }
}

bool WordRepository::load(const std::string& wordText, Word& outWord) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return false;

    const char* sqlMain =
        "SELECT meaning, type, quantity, tense, gender, degree, person, confidence, frequency, last_used "
        "FROM words WHERE word = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlMain, &stmt)) return false;
    sqlite3_bind_text(stmt, 1, wordText.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }

    outWord.setWord(wordText);
    const char* meaning = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    outWord.setMeaning(meaning ? meaning : "");
    outWord.setType(static_cast<WordType>(sqlite3_column_int(stmt, 1)));
    outWord.setQuantity(static_cast<Quantity>(sqlite3_column_int(stmt, 2)));
    outWord.setTense(static_cast<Tense>(sqlite3_column_int(stmt, 3)));
    outWord.setGender(static_cast<Gender>(sqlite3_column_int(stmt, 4)));
    outWord.setDegree(static_cast<Degree>(sqlite3_column_int(stmt, 5)));
    outWord.setPerson(static_cast<Person>(sqlite3_column_int(stmt, 6)));
    outWord.setConfidence(static_cast<float>(sqlite3_column_double(stmt, 7)));
    uint32_t freq = static_cast<uint32_t>(sqlite3_column_int(stmt, 8));
    outWord.setFrequency(freq);
    sqlite3_finalize(stmt);

    // Load related words
    outWord.clearRelated();
    int wordId = getWordId(db, wordText, false);
    if (wordId == -1) return true;
    const char* sqlRels =
        "SELECT w2.word, rw.weight FROM related_words rw "
        "JOIN words w2 ON rw.related_word_id = w2.id "
        "WHERE rw.word_id = ?";
    if (!prepareSqlStatement(db, sqlRels, &stmt)) return true;
    sqlite3_bind_int(stmt, 1, wordId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* relText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (relText) {
            outWord.addRelated(std::string(relText), sqlite3_column_double(stmt, 1));
        }
    }
    sqlite3_finalize(stmt);
    return true;
}

bool WordRepository::exists(const std::string& wordText) {
    Word dummy;
    return load(wordText, dummy);
}

bool WordRepository::remove(const std::string& wordText) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return false;
    const char* sql = "DELETE FROM words WHERE word = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return false;
    sqlite3_bind_text(stmt, 1, wordText.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

uint32_t WordRepository::getLastTime(const std::string wordText) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return 0;
    const char* sql = "SELECT strftime('%s', last_used) FROM words WHERE word = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return 0;
    sqlite3_bind_text(stmt, 1, wordText.c_str(), -1, SQLITE_STATIC);
    uint32_t ts = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ts = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return ts;
}

bool WordRepository::updateTime(const Word word) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return false;
    const char* sql = "UPDATE words SET last_used = CURRENT_TIMESTAMP WHERE word = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return false;
    sqlite3_bind_text(stmt, 1, word.getWord().c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int WordRepository::getOrCreateWordId(const Word& word) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return -1;
    return getWordId(db, word.getWord(), true);
}

std::vector<std::string> WordRepository::getRelatedWordsRecursive(const std::string& word,
                                                                  int maxDepth,
                                                                  int maxTotal) {
    std::vector<std::string> result;
    std::unordered_set<std::string> visited;
    std::queue<std::pair<std::string, int>> queue;
    queue.push({word, 0});
    visited.insert(word);

    while (!queue.empty() && result.size() < static_cast<size_t>(maxTotal)) {
        auto [current, depth] = queue.front();
        queue.pop();
        if (depth > 0) result.push_back(current);
        if (depth >= maxDepth) continue;

        Word w;
        if (load(current, w)) {
            for (const auto& [rel, conf] : w.getRelated()) {
                if (visited.find(rel) == visited.end() && conf > 0.3) {
                    visited.insert(rel);
                    queue.push({rel, depth + 1});
                }
            }
        }
    }
    return result;
}

std::vector<Word> WordRepository::getHighConfidenceWords(float minConfidence, int limit) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) {
        std::cerr << "WordRepository::getHighConfidenceWords: database not initialized" << std::endl;
        return {};
    }

    // 1. Build SQL query
    std::string sql =
        "SELECT id, word, meaning, type, quantity, tense, gender, degree, person, confidence, frequency, last_used "
        "FROM words WHERE confidence >= ? ";
    if (limit > 0) {
        sql += "LIMIT ?";
    }

    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql.c_str(), &stmt)) {
        std::cerr << "WordRepository::getHighConfidenceWords: prepare failed" << std::endl;
        return {};
    }

    sqlite3_bind_double(stmt, 1, minConfidence);
    if (limit > 0) {
        sqlite3_bind_int(stmt, 2, limit);
    }

    std::vector<Word> words;
    std::vector<int> wordIds;
    std::unordered_map<int, size_t> idToIndex; // id -> position in words vector

    // 2. Fetch main word data
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* wordText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* meaning = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        Word w;
        w.setWord(wordText ? wordText : "");
        w.setMeaning(meaning ? meaning : "");
        w.setType(static_cast<WordType>(sqlite3_column_int(stmt, 3)));
        w.setQuantity(static_cast<Quantity>(sqlite3_column_int(stmt, 4)));
        w.setTense(static_cast<Tense>(sqlite3_column_int(stmt, 5)));
        w.setGender(static_cast<Gender>(sqlite3_column_int(stmt, 6)));
        w.setDegree(static_cast<Degree>(sqlite3_column_int(stmt, 7)));
        w.setPerson(static_cast<Person>(sqlite3_column_int(stmt, 8)));
        w.setConfidence(static_cast<float>(sqlite3_column_double(stmt, 9)));
        w.setFrequency(static_cast<uint32_t>(sqlite3_column_int(stmt, 10)));
        w.setTimestamp(getLastTime(wordText));

        wordIds.push_back(id);
        idToIndex[id] = words.size();
        words.push_back(std::move(w));
    }
    sqlite3_finalize(stmt);

    if (words.empty()) {
        return words;
    }

    // 3. Batch load related words for all retrieved word IDs
    std::stringstream idsStr;
    for (size_t i = 0; i < wordIds.size(); ++i) {
        if (i != 0) idsStr << ",";
        idsStr << wordIds[i];
    }

    std::string sqlRels =
        "SELECT rw.word_id, w2.word, rw.weight "
        "FROM related_words rw "
        "JOIN words w2 ON rw.related_word_id = w2.id "
        "WHERE rw.word_id IN (" + idsStr.str() + ")";

    sqlite3_stmt* stmtRels = nullptr;
    if (prepareSqlStatement(db, sqlRels.c_str(), &stmtRels)) {
        while (sqlite3_step(stmtRels) == SQLITE_ROW) {
            int wordId = sqlite3_column_int(stmtRels, 0);
            const char* relWord = reinterpret_cast<const char*>(sqlite3_column_text(stmtRels, 1));
            double weight = sqlite3_column_double(stmtRels, 2);

            auto it = idToIndex.find(wordId);
            if (it != idToIndex.end() && relWord) {
                words[it->second].addRelated(std::string(relWord), weight);
            }
        }
        sqlite3_finalize(stmtRels);
    } else {
        std::cerr << "WordRepository::getHighConfidenceWords: warning, could not load relations" << std::endl;
    }

    return words;
}
