/**
 * @file PatternRepository.cpp
 * @brief Implementation of pattern persistence using SQLite.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "PatternRepository.hpp"
#include "DatabaseManager.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <ctime>

// ============================================================================
// Serialization helpers
// ============================================================================
static std::string serializeSequence(const std::vector<WordType>& seq) {
    std::stringstream ss;
    for (size_t i = 0; i < seq.size(); ++i) {
        if (i > 0) ss << ",";
        ss << static_cast<int>(seq[i]);
    }
    return ss.str();
}

static std::vector<WordType> deserializeSequence(const std::string& str) {
    std::vector<WordType> res;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
            res.push_back(static_cast<WordType>(std::stoi(item)));
        }
    }
    return res;
}

// ============================================================================
// Static members
// ============================================================================
static std::string dbPath_;

void PatternRepository::setDatabasePath(const std::string& path) {
    dbPath_ = path;
    DatabaseManager::instance().init(dbPath_);
}

std::string PatternRepository::getDatabasePath() {
    return dbPath_;
}

void PatternRepository::initializeTables() {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    char* errMsg = nullptr;

    const char* createSql =
        "CREATE TABLE IF NOT EXISTS patterns ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "pattern_type INTEGER NOT NULL,"
        "sequence TEXT NOT NULL,"
        "frequency REAL DEFAULT 1.0,"
        "last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";

    if (sqlite3_exec(db, createSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "PatternRepository: error creating table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return;
    }
    const char* addFreq = "ALTER TABLE patterns ADD COLUMN frequency REAL DEFAULT 1.0";
    sqlite3_exec(db, addFreq, nullptr, nullptr, nullptr);
    const char* addLastUsed = "ALTER TABLE patterns ADD COLUMN last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP";
    sqlite3_exec(db, addLastUsed, nullptr, nullptr, nullptr);

    const char* indexSql =
        "CREATE INDEX IF NOT EXISTS idx_pattern_type_sequence "
        "ON patterns(pattern_type, sequence);";

    if (sqlite3_exec(db, indexSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "PatternRepository: warning creating index: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void PatternRepository::save(const Pattern& pattern) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    std::string seqStr = serializeSequence(pattern.sequence);
    sqlite3_stmt* stmt = nullptr;

    const char* sqlCheck = "SELECT id, frequency FROM patterns WHERE pattern_type = ? AND sequence = ?";
    if (!prepareSqlStatement(db, sqlCheck, &stmt)) return;
    sqlite3_bind_int(stmt, 1, static_cast<int>(pattern.type));
    sqlite3_bind_text(stmt, 2, seqStr.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        float oldFreq = static_cast<float>(sqlite3_column_double(stmt, 1));
        sqlite3_finalize(stmt);

        const char* sqlUpd = "UPDATE patterns SET frequency = ?, last_used = CURRENT_TIMESTAMP WHERE id = ?";
        if (!prepareSqlStatement(db, sqlUpd, &stmt)) return;
        sqlite3_bind_double(stmt, 1, oldFreq + pattern.frequency_);
        sqlite3_bind_int(stmt, 2, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        sqlite3_finalize(stmt);
        const char* sqlIns = "INSERT INTO patterns (pattern_type, sequence, frequency, last_used) VALUES (?,?,?, CURRENT_TIMESTAMP)";
        if (!prepareSqlStatement(db, sqlIns, &stmt)) return;
        sqlite3_bind_int(stmt, 1, static_cast<int>(pattern.type));
        sqlite3_bind_text(stmt, 2, seqStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, pattern.frequency_);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

std::optional<Pattern> PatternRepository::findExactMatch(const std::vector<WordType>& sequence,
                                                          float& outSimiliarity) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) {
        outSimiliarity = 0.0f;
        return std::nullopt;
    }

    std::string targetSeq = serializeSequence(sequence);
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT pattern_type, sequence, frequency, strftime('%s', last_used) FROM patterns WHERE sequence = ? AND pattern_type = ?";
    if (!prepareSqlStatement(db, sql, &stmt)) {
        outSimiliarity = 0.0f;
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, targetSeq.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(classifySentencePattern(sequence)));

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outSimiliarity = 1.0f;
        PatternType pType = static_cast<PatternType>(sqlite3_column_int(stmt, 0));
        const char* seqStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::vector<WordType> seq = deserializeSequence(seqStr);
        float freq = static_cast<float>(sqlite3_column_double(stmt, 2));
        uint32_t ts = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
        Pattern pat(seq, pType);
        pat.frequency_ = freq;
        pat.timestamp_ = ts;
        sqlite3_finalize(stmt);
        return pat;
    }
    sqlite3_finalize(stmt);
    outSimiliarity = 0.0f;
    return std::nullopt;
}

std::vector<Pattern> PatternRepository::loadAll() {
    std::vector<Pattern> result;
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return result;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT pattern_type, sequence, frequency, strftime('%s', last_used) FROM patterns";
    if (!prepareSqlStatement(db, sql, &stmt)) return result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PatternType pType = static_cast<PatternType>(sqlite3_column_int(stmt, 0));
        const char* seqStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::vector<WordType> seq = deserializeSequence(seqStr);
        float freq = static_cast<float>(sqlite3_column_double(stmt, 2));
        uint32_t ts = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
        Pattern pat(seq, pType);
        pat.frequency_ = freq;
        pat.timestamp_ = ts;
        result.push_back(pat);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool PatternRepository::remove(const Pattern& pattern) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return false;
    std::string seqStr = serializeSequence(pattern.sequence);
    const char* sql = "DELETE FROM patterns WHERE pattern_type = ? AND sequence = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return false;
    sqlite3_bind_int(stmt, 1, static_cast<int>(pattern.type));
    sqlite3_bind_text(stmt, 2, seqStr.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

uint32_t PatternRepository::getLastTime(const Pattern& pattern) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return 0;
    std::string seqStr = serializeSequence(pattern.sequence);
    const char* sql = "SELECT strftime('%s', last_used) FROM patterns WHERE pattern_type = ? AND sequence = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return 0;
    sqlite3_bind_int(stmt, 1, static_cast<int>(pattern.type));
    sqlite3_bind_text(stmt, 2, seqStr.c_str(), -1, SQLITE_STATIC);
    uint32_t ts = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ts = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return ts;
}

bool PatternRepository::updateTime(const Pattern& pattern) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return false;
    std::string seqStr = serializeSequence(pattern.sequence);
    const char* sql = "UPDATE patterns SET last_used = CURRENT_TIMESTAMP WHERE pattern_type = ? AND sequence = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return false;
    sqlite3_bind_int(stmt, 1, static_cast<int>(pattern.type));
    sqlite3_bind_text(stmt, 2, seqStr.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}
