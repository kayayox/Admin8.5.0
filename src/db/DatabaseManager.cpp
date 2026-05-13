/**
 * @file DatabaseManager.cpp
 * @brief Implementation of the multi‑database SQLite manager.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "DatabaseManager.hpp"
#include "../db/SentenceRepository.hpp"
#include "../db/DialogueRepository.hpp"
#include "../dialogue/PatternCorrelator.hpp"
#include <iostream>

// ---------------------------------------------------------------------------
// Utility function (declared in header, defined here)
// ---------------------------------------------------------------------------
bool prepareSqlStatement(sqlite3* db, const char* sql, sqlite3_stmt** stmt) {
    int rc = sqlite3_prepare_v2(db, sql, -1, stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "DatabaseManager: SQL prepare error: " << sqlite3_errmsg(db) << "\n"
                  << "SQL: " << sql << std::endl;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Connection management
// ---------------------------------------------------------------------------
bool DatabaseManager::init(const std::string& dbPath) {
    // Already open?
    if (connections_.find(dbPath) != connections_.end()) {
        return true;
    }

    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "DatabaseManager: error opening " << dbPath << ": "
                  << sqlite3_errmsg(db) << std::endl;
        if (db) sqlite3_close(db);
        return false;
    }

    // Performance pragmas
    const char* pragmas = "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, pragmas, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "DatabaseManager: warning setting pragmas: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    connections_[dbPath] = db;
    return true;
}

void DatabaseManager::close(const std::string& dbPath) {
    auto it = connections_.find(dbPath);
    if (it != connections_.end()) {
        sqlite3_close(it->second);
        connections_.erase(it);
    }
}

void DatabaseManager::closeAll() {
    for (auto& pair : connections_) {
        sqlite3_close(pair.second);
    }
    connections_.clear();
}

sqlite3* DatabaseManager::getHandle(const std::string& dbPath) const {
    auto it = connections_.find(dbPath);
    return (it != connections_.end()) ? it->second : nullptr;
}

bool DatabaseManager::prepareStatement(const std::string& dbPath,
                                       const char* sql,
                                       sqlite3_stmt** stmt) const {
    sqlite3* db = getHandle(dbPath);
    if (!db) return false;
    return prepareSqlStatement(db, sql, stmt);
}

// ---------------------------------------------------------------------------
// Cleanup operations
// ---------------------------------------------------------------------------
int DatabaseManager::cleanSemanticDatabase(const std::string& dbPath) {
    instance().init(dbPath);
    SentenceRepository::setDatabasePath(dbPath);
    DialogueRepository::setDatabasePath(dbPath);

    int removedSentences = SentenceRepository::mergeDuplicateSentences();
    int removedDialogues = DialogueRepository::mergeDuplicateDialogues();
    return removedSentences + removedDialogues;
}

int DatabaseManager::cleanPatternDatabase(const std::string& dbPath) {
    instance().init(dbPath);
    // Purge low‑weight correlations in both normal and chunk tables
    {
        PatternCorrelator corr(dbPath, "");
        corr.purgeLowWeightCorrelations(0.01);
    }
    {
        PatternCorrelator corr(dbPath, "_chunk");
        corr.purgeLowWeightCorrelations(0.01);
    }
    // Compact the database
    sqlite3* db = instance().getHandle(dbPath);
    if (db) {
        sqlite3_exec(db, "VACUUM", nullptr, nullptr, nullptr);
    }
    return 0;
}

void DatabaseManager::cleanAll(const std::string& semanticPath,
                               const std::string& patternPath) {
    cleanSemanticDatabase(semanticPath);
    cleanPatternDatabase(patternPath);
}
