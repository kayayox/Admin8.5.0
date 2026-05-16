/**
 * @file SentenceRepository.cpp
 * @brief Implementation of Sentence persistence using SQLite.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "SentenceRepository.hpp"
#include "DatabaseManager.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <regex>
#include <ctime>

// ============================================================================
// Static members
// ============================================================================
std::string SentenceRepository::dbPath_;

void SentenceRepository::setDatabasePath(const std::string& path) {
    dbPath_ = path;
    DatabaseManager::instance().init(dbPath_);
}

std::string SentenceRepository::getDatabasePath() {
    return dbPath_;
}

void SentenceRepository::initializeTables() {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    const char* sql =
        "CREATE TABLE IF NOT EXISTS sentences ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "key_word TEXT, key_type INTEGER, frequency REAL,"
        "tense INTEGER, num_blocks INTEGER,"
        "last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS blocks ("
        "sentence_id INTEGER, position INTEGER, block_text TEXT, word_type INTEGER,"
        "FOREIGN KEY(sentence_id) REFERENCES sentences(id)"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SentenceRepository: error creating tables: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

static int findDuplicateSentenceId(const Sentence& sentence, sqlite3* db) {
    if (!db) return -1;

    // Obtener el texto completo normalizado de la oración a buscar
    std::string fullText = sentence.toString();
    std::transform(fullText.begin(), fullText.end(), fullText.begin(), ::tolower);
    // Limpiar espacios múltiples
    fullText = std::regex_replace(fullText, std::regex("\\s+"), " ");

    // Comparar con las oraciones existentes mediante concatenación de bloques
    const char* sql =
        "SELECT s.id "
        "FROM sentences s "
        "JOIN blocks b ON s.id = b.sentence_id "
        "GROUP BY s.id "
        "HAVING GROUP_CONCAT(b.block_text, ' ' ORDER BY b.position) = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_bind_text(stmt, 1, fullText.c_str(), -1, SQLITE_STATIC);
    int dupId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        dupId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return dupId;
}

void SentenceRepository::save(Sentence& sentence) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    int newId = sentence.getId();

    // --- Si es una nueva oración, comprobar duplicados ---
    if (newId <= 0) {
        int dupId = findDuplicateSentenceId(sentence, db);
        if (dupId != -1) {
            // Ya existe: actualizar frecuencia y timestamp de la existente
            const char* sqlUpd = "UPDATE sentences SET frequency = frequency + 1, last_used = CURRENT_TIMESTAMP WHERE id = ?";
            sqlite3_stmt* stmt = nullptr;
            if (prepareSqlStatement(db, sqlUpd, &stmt)) {
                sqlite3_bind_int(stmt, 1, dupId);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            sentence.setId(dupId);
            sentence.setFrequency(sentence.getFrequency() + 1.0f);
            sentence.setTimestamp(static_cast<uint32_t>(std::time(nullptr)));
            return; // No insertar duplicado
        }
    }

    // --- Si no es duplicado o es actualización de existente ---
    if (newId > 0) {
        // Actualizar oración existente (incrementar frecuencia en 1)
        const char* sqlUpd =
            "UPDATE sentences SET key_word=?, key_type=?, frequency=frequency+?, tense=?, num_blocks=?, last_used=CURRENT_TIMESTAMP WHERE id=?";
        sqlite3_stmt* stmt = nullptr;
        if (!prepareSqlStatement(db, sqlUpd, &stmt)) return;
        sqlite3_bind_text(stmt, 1, sentence.getKey().text.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, static_cast<int>(sentence.getKey().type));
        sqlite3_bind_double(stmt, 3, 1.0);   // CORREGIDO: incrementa en 1, no en sentence.getFrequency()
        sqlite3_bind_int(stmt, 4, static_cast<int>(sentence.getTense()));
        sqlite3_bind_int(stmt, 5, sentence.getNumBlocks());
        sqlite3_bind_int(stmt, 6, newId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Borrar bloques viejos
        const char* sqlDel = "DELETE FROM blocks WHERE sentence_id=?";
        if (!prepareSqlStatement(db, sqlDel, &stmt)) return;
        sqlite3_bind_int(stmt, 1, newId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        // Insertar nueva oración (sin duplicado)
        const char* sqlIns =
            "INSERT INTO sentences (key_word, key_type, frequency, tense, num_blocks, last_used) VALUES (?,?,?,?,?, CURRENT_TIMESTAMP)";
        sqlite3_stmt* stmt = nullptr;
        if (!prepareSqlStatement(db, sqlIns, &stmt)) return;
        sqlite3_bind_text(stmt, 1, sentence.getKey().text.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, static_cast<int>(sentence.getKey().type));
        sqlite3_bind_double(stmt, 3, sentence.getFrequency());
        sqlite3_bind_int(stmt, 4, static_cast<int>(sentence.getTense()));
        sqlite3_bind_int(stmt, 5, sentence.getNumBlocks());
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "SentenceRepository: error inserting sentence: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            return;
        }
        newId = static_cast<int>(sqlite3_last_insert_rowid(db));
        sqlite3_finalize(stmt);
        sentence.setId(newId);
    }

    // --- Insertar los bloques (común para ambos casos) ---
    const char* sqlBlock = "INSERT INTO blocks (sentence_id, position, block_text, word_type) VALUES (?,?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlBlock, &stmt)) return;
    const auto& blocks = sentence.getBlocks();
    for (size_t i = 0; i < blocks.size(); ++i) {
        sqlite3_bind_int(stmt, 1, newId);
        sqlite3_bind_int(stmt, 2, static_cast<int>(i));
        sqlite3_bind_text(stmt, 3, blocks[i].text.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, static_cast<int>(blocks[i].type));
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
}

Sentence SentenceRepository::loadById(int id) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    Sentence sentence;
    if (!db) return sentence;

    // Load header with last_used
    const char* sqlSentence =
        "SELECT key_word, key_type, frequency, tense, strftime('%s', last_used) FROM sentences WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlSentence, &stmt)) return sentence;
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return sentence;
    }
    std::string keyWord = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    WordType keyType = static_cast<WordType>(sqlite3_column_int(stmt, 1));
    float frequency = static_cast<float>(sqlite3_column_double(stmt, 2));
    Tense tense = static_cast<Tense>(sqlite3_column_int(stmt, 3));
    uint32_t lastUsed = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
    sqlite3_finalize(stmt);

    // Load blocks
    const char* sqlBlocks =
        "SELECT block_text, word_type FROM blocks WHERE sentence_id = ? ORDER BY position";
    if (!prepareSqlStatement(db, sqlBlocks, &stmt)) return sentence;
    sqlite3_bind_int(stmt, 1, id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        WordType type = static_cast<WordType>(sqlite3_column_int(stmt, 1));
        sentence.addBlock(text, type);
    }
    sqlite3_finalize(stmt);

    sentence.setId(id);
    sentence.setKey({keyWord, keyType});
    sentence.setFrequency(frequency);
    sentence.setTense(tense);
    sentence.setTimestamp(lastUsed);
    return sentence;
}

Sentence SentenceRepository::loadByKey(const std::string& keyWord, WordType keyType) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    Sentence sentence;
    if (!db) return sentence;

    const char* sql = "SELECT id FROM sentences WHERE key_word = ? AND key_type = ? LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return sentence;
    sqlite3_bind_text(stmt, 1, keyWord.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(keyType));
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return sentence;
    }
    int id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return loadById(id);
}

bool SentenceRepository::remove(int id) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return false;

    const char* sqlDelBlocks = "DELETE FROM blocks WHERE sentence_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sqlDelBlocks, &stmt)) return false;
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    const char* sqlDelSentence = "DELETE FROM sentences WHERE id = ?";
    if (!prepareSqlStatement(db, sqlDelSentence, &stmt)) return false;
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

uint32_t SentenceRepository::getLastTime(int id) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return 0;
    const char* sql = "SELECT strftime('%s', last_used) FROM sentences WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return 0;
    sqlite3_bind_int(stmt, 1, id);
    uint32_t ts = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ts = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return ts;
}

bool SentenceRepository::updateTime(const Sentence& sentence) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db || sentence.getId() <= 0) return false;
    const char* sql = "UPDATE sentences SET last_used = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return false;
    sqlite3_bind_int(stmt, 1, sentence.getId());
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int SentenceRepository::getOrCreateOracionId(const Sentence& sentence) {
    if (sentence.getId() > 0) return sentence.getId();
    Sentence copy = sentence;
    save(copy);
    return copy.getId();
}

std::vector<Sentence> SentenceRepository::loadAll() {
    std::vector<Sentence> result;
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return result;

    const char* sql = "SELECT id FROM sentences ORDER BY id";
    sqlite3_stmt* stmt = nullptr;
    if (!prepareSqlStatement(db, sql, &stmt)) return result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        result.push_back(loadById(id));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Sentence> SentenceRepository::findSentencesContainingWords(const std::vector<std::string>& words,
                                                                       int limit) {
    std::vector<Sentence> result;
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db || words.empty()) return result;

    std::string placeholders;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) placeholders += ",";
        placeholders += "?";
    }
    std::string sql =
        "SELECT DISTINCT s.id FROM sentences s JOIN blocks b ON s.id = b.sentence_id "
        "WHERE b.block_text IN (" + placeholders + ") LIMIT ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return result;
    }
    for (size_t i = 0; i < words.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), words[i].c_str(), -1, SQLITE_STATIC);
    }
    sqlite3_bind_int(stmt, static_cast<int>(words.size() + 1), limit);

    std::vector<int> ids;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    for (int id : ids) {
        result.push_back(loadById(id));
    }
    return result;
}

int SentenceRepository::mergeDuplicateSentences() {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return 0;

    // 1. Agrupar oraciones por texto normalizado (bloques concatenados)
    const char* sqlSelect =
        "SELECT s.id, GROUP_CONCAT(b.block_text, ' ' ORDER BY b.position) as full_text "
        "FROM sentences s "
        "JOIN blocks b ON s.id = b.sentence_id "
        "GROUP BY s.id";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sqlSelect, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return 0;
    }

    std::map<std::string, std::vector<int>> textToIds;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (!text) continue;
        std::string normalized(text);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        normalized = std::regex_replace(normalized, std::regex("\\s+"), " ");
        textToIds[normalized].push_back(id);
    }
    sqlite3_finalize(stmt);

    int removedCount = 0;
    for (const auto& pair : textToIds) {
        const auto& ids = pair.second;
        if (ids.size() <= 1) continue;

        // Elegir el ID más pequeño como canónico (para mantener referencias estables)
        int canonicalId = *std::min_element(ids.begin(), ids.end());

        for (int dupId : ids) {
            if (dupId == canonicalId) continue;

            // 2. Combinar frecuencias y last_used del duplicado hacia el canónico
            const char* sqlMerge =
                "UPDATE sentences SET "
                "frequency = frequency + (SELECT frequency FROM sentences WHERE id = ?), "
                "last_used = max(last_used, (SELECT last_used FROM sentences WHERE id = ?)) "
                "WHERE id = ?";
            sqlite3_stmt* stmtMerge = nullptr;
            if (sqlite3_prepare_v2(db, sqlMerge, -1, &stmtMerge, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmtMerge, 1, dupId);
                sqlite3_bind_int(stmtMerge, 2, dupId);
                sqlite3_bind_int(stmtMerge, 3, canonicalId);
                sqlite3_step(stmtMerge);
                sqlite3_finalize(stmtMerge);
            }

            // 3. Actualizar referencias en dialogs (si la tabla existe)
            const char* sqlUpdPrem = "UPDATE dialogs SET premise_id = ? WHERE premise_id = ?";
            sqlite3_stmt* stmtUp = nullptr;
            if (sqlite3_prepare_v2(db, sqlUpdPrem, -1, &stmtUp, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmtUp, 1, canonicalId);
                sqlite3_bind_int(stmtUp, 2, dupId);
                sqlite3_step(stmtUp);
                sqlite3_finalize(stmtUp);
            }
            const char* sqlUpdHyp = "UPDATE dialogs SET hypothesis_id = ? WHERE hypothesis_id = ?";
            if (sqlite3_prepare_v2(db, sqlUpdHyp, -1, &stmtUp, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmtUp, 1, canonicalId);
                sqlite3_bind_int(stmtUp, 2, dupId);
                sqlite3_step(stmtUp);
                sqlite3_finalize(stmtUp);
            }

            // 4. Eliminar bloques y la oración duplicada
            const char* sqlDelBlk = "DELETE FROM blocks WHERE sentence_id = ?";
            if (sqlite3_prepare_v2(db, sqlDelBlk, -1, &stmtUp, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmtUp, 1, dupId);
                sqlite3_step(stmtUp);
                sqlite3_finalize(stmtUp);
            }
            const char* sqlDelSen = "DELETE FROM sentences WHERE id = ?";
            if (sqlite3_prepare_v2(db, sqlDelSen, -1, &stmtUp, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmtUp, 1, dupId);
                sqlite3_step(stmtUp);
                sqlite3_finalize(stmtUp);
            }
            removedCount++;
        }
    }
    return removedCount;
}
