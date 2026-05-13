/**
 * @file DialogueRepository.cpp
 * @brief Implementation of the dialogue repository using SQLite.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "DialogueRepository.hpp"
#include "DatabaseManager.hpp"
#include "SentenceRepository.hpp"   // for saving/loading sentences
#include <iostream>
#include <ctime>

std::string DialogueRepository::dbPath_;

void DialogueRepository::setDatabasePath(const std::string& path) {
    dbPath_ = path;
    DatabaseManager::instance().init(dbPath_);
    SentenceRepository::setDatabasePath(path);  // same database
}

std::string DialogueRepository::getDatabasePath() {
    return dbPath_;
}

void DialogueRepository::initializeTables() {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    const char* sql =
        "CREATE TABLE IF NOT EXISTS dialogs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "premise_id INTEGER, hypothesis_id INTEGER, pattern_type INTEGER,"
        "creativity REAL, frequency INTEGER DEFAULT 1,"
        "last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS feedback ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "word TEXT, proposed_type INTEGER, correct_type INTEGER,"
        "correct INTEGER, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "DialogueRepository: error creating tables: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    const char* addFreq = "ALTER TABLE dialogs ADD COLUMN frequency INTEGER DEFAULT 1";
    sqlite3_exec(db, addFreq, nullptr, nullptr, nullptr);
    const char* addLastUsed = "ALTER TABLE dialogs ADD COLUMN last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP";
    sqlite3_exec(db, addLastUsed, nullptr, nullptr, nullptr);
}

int DialogueRepository::getOrCreateSentenceId(const Sentence& sentence) {
    if (sentence.getId() > 0) return sentence.getId();
    Sentence copy = sentence;
    SentenceRepository::save(copy);
    return copy.getId();
}

void DialogueRepository::saveDialogue(const Sentence& premise, const Sentence& hypothesis,
                                      const Pattern& pattern, float creativity) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    int premId = getOrCreateSentenceId(premise);
    int hypId  = getOrCreateSentenceId(hypothesis);

    // Check if an identical dialogue already exists
    const char* sqlCheck =
        "SELECT id, frequency FROM dialogs WHERE premise_id = ? AND hypothesis_id = ? AND pattern_type = ? AND creativity = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlCheck, &stmt)) return;
    sqlite3_bind_int(stmt, 1, premId);
    sqlite3_bind_int(stmt, 2, hypId);
    sqlite3_bind_int(stmt, 3, static_cast<int>(pattern.type));
    sqlite3_bind_double(stmt, 4, creativity);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int oldFreq = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);

        // Update: increment frequency and update last_used
        const char* sqlUpd = "UPDATE dialogs SET frequency = ?, last_used = CURRENT_TIMESTAMP WHERE id = ?";
        if (!prepareSqlStatement(db, sqlUpd, &stmt)) return;
        sqlite3_bind_int(stmt, 1, oldFreq + 1);
        sqlite3_bind_int(stmt, 2, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    // Insert new dialogue with frequency=1 and current timestamp
    const char* sqlIns =
        "INSERT INTO dialogs (premise_id, hypothesis_id, pattern_type, creativity, frequency, last_used) VALUES (?,?,?,?,1,CURRENT_TIMESTAMP)";
    if (!prepareSqlStatement(db, sqlIns, &stmt)) return;
    sqlite3_bind_int(stmt, 1, premId);
    sqlite3_bind_int(stmt, 2, hypId);
    sqlite3_bind_int(stmt, 3, static_cast<int>(pattern.type));
    sqlite3_bind_double(stmt, 4, creativity);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void DialogueRepository::registerFeedback(const std::string& word,
                                          WordType proposedType,
                                          WordType correctType,
                                          bool correct) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    int success = correct ? 1 : 0;
    const char* sqlIns =
        "INSERT INTO feedback (word, proposed_type, correct_type, correct) VALUES (?,?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlIns, &stmt)) return;
    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(proposedType));
    sqlite3_bind_int(stmt, 3, static_cast<int>(correctType));
    sqlite3_bind_int(stmt, 4, success);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<std::string> DialogueRepository::buildCorpus() {
    std::vector<std::string> corpus;
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return corpus;

    const char* sql = "SELECT block_text FROM blocks ORDER BY sentence_id, position";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return corpus;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        corpus.emplace_back(word);
    }
    sqlite3_finalize(stmt);
    return corpus;
}

DialogueHistory DialogueRepository::loadHistory() {
    DialogueHistory history;
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return history;

    const char* sql = "SELECT premise_id, hypothesis_id, pattern_type, creativity, frequency, strftime('%s', last_used) FROM dialogs ORDER BY last_used";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return history;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int premId = sqlite3_column_int(stmt, 0);
        int hypId = sqlite3_column_int(stmt, 1);
        PatternType pType = static_cast<PatternType>(sqlite3_column_int(stmt, 2));
        float creativity = static_cast<float>(sqlite3_column_double(stmt, 3));
        uint32_t frequency_ = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
        uint32_t lastUsed_ = static_cast<uint32_t>(sqlite3_column_int(stmt, 5));

        Sentence premise = SentenceRepository::loadById(premId);
        Sentence hypothesis = SentenceRepository::loadById(hypId);
        Pattern pattern(patternFromSequence(premise.getTypeSequence()).sequence, pType);
        pattern.frequency_ = static_cast<float>(frequency_);
        pattern.timestamp_ = lastUsed_;

        Dialogue dia{premise, hypothesis, pattern, lastUsed_, frequency_, creativity};
        history.addDialogue(premise, hypothesis, pattern, creativity);
    }
    sqlite3_finalize(stmt);
    return history;
}

std::vector<Dialogue> DialogueRepository::loadHistoryContainingWords(const std::vector<std::string>& words,
                                                                     int limit) {
    std::vector<Dialogue> result;
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db || words.empty()) return result;

    // Build placeholders for IN clause
    std::string placeholders;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) placeholders += ",";
        placeholders += "?";
    }

    std::string sql =
        "SELECT DISTINCT d.id FROM dialogs d "
        "JOIN sentences s ON d.premise_id = s.id "
        "JOIN blocks b ON s.id = b.sentence_id "
        "WHERE b.block_text IN (" + placeholders + ") "
        "LIMIT ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return result;
    }

    for (size_t i = 0; i < words.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), words[i].c_str(), -1, SQLITE_STATIC);
    }
    sqlite3_bind_int(stmt, static_cast<int>(words.size() + 1), limit);

    std::vector<int> dialogueIds;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        dialogueIds.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    // Load each dialogue
    for (int did : dialogueIds) {
        const char* sqlDia = "SELECT premise_id, hypothesis_id, pattern_type, creativity, frequency, strftime('%s', last_used) FROM dialogs WHERE id = ?";
        sqlite3_stmt* stmt2 = nullptr;
        if (sqlite3_prepare_v2(db, sqlDia, -1, &stmt2, nullptr) != SQLITE_OK) {
            sqlite3_finalize(stmt2);
            continue;
        }
        sqlite3_bind_int(stmt2, 1, did);
        if (sqlite3_step(stmt2) == SQLITE_ROW) {
            int premId = sqlite3_column_int(stmt2, 0);
            int hypId = sqlite3_column_int(stmt2, 1);
            PatternType pType = static_cast<PatternType>(sqlite3_column_int(stmt2, 2));
            float creativity = static_cast<float>(sqlite3_column_double(stmt2, 3));
            uint32_t frequency_ = static_cast<uint32_t>(sqlite3_column_int(stmt2, 4));
            uint32_t lastUsed_ = static_cast<uint32_t>(sqlite3_column_int(stmt2, 5));

            Sentence premise = SentenceRepository::loadById(premId);
            Sentence hypothesis = SentenceRepository::loadById(hypId);
            Pattern pattern(patternFromSequence(premise.getTypeSequence()).sequence, pType);

            Dialogue dia{premise, hypothesis, pattern, lastUsed_, frequency_, creativity};
            result.push_back(dia);
        }
        sqlite3_finalize(stmt2);
    }
    return result;
}

int DialogueRepository::mergeDuplicateDialogues() {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return 0;

    const char* sqlFind =
        "SELECT id, premise_id, hypothesis_id, pattern_type, creativity FROM dialogs "
        "GROUP BY premise_id, hypothesis_id, pattern_type, creativity HAVING COUNT(*) > 1";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sqlFind, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return 0;
    }

    std::vector<int> toDelete;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int firstId = sqlite3_column_int(stmt, 0);
        int prem = sqlite3_column_int(stmt, 1);
        int hyp = sqlite3_column_int(stmt, 2);
        int pType = sqlite3_column_int(stmt, 3);
        double creativity = sqlite3_column_double(stmt, 4);

        const char* sqlDups = "SELECT id FROM dialogs WHERE premise_id=? AND hypothesis_id=? AND pattern_type=? AND creativity=? AND id != ?";
        sqlite3_stmt* stmtDup = nullptr;
        if (sqlite3_prepare_v2(db, sqlDups, -1, &stmtDup, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtDup, 1, prem);
            sqlite3_bind_int(stmtDup, 2, hyp);
            sqlite3_bind_int(stmtDup, 3, pType);
            sqlite3_bind_double(stmtDup, 4, creativity);
            sqlite3_bind_int(stmtDup, 5, firstId);
            while (sqlite3_step(stmtDup) == SQLITE_ROW) {
                toDelete.push_back(sqlite3_column_int(stmtDup, 0));
            }
            sqlite3_finalize(stmtDup);
        }
    }
    sqlite3_finalize(stmt);

    int removed = 0;
    const char* sqlDel = "DELETE FROM dialogs WHERE id = ?";
    for (int id : toDelete) {
        sqlite3_stmt* stmtDel = nullptr;
        if (sqlite3_prepare_v2(db, sqlDel, -1, &stmtDel, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtDel, 1, id);
            sqlite3_step(stmtDel);
            sqlite3_finalize(stmtDel);
            removed++;
        }
    }
    return removed;
}
