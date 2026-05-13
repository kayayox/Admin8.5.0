/**
 * @file TemplateRepository.cpp
 * @brief Implementation of template persistence using SQLite.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "TemplateRepository.hpp"
#include "DatabaseManager.hpp"
#include <iostream>
#include <sstream>
#include <ctime>

std::string TemplateRepository::dbPath_;

void TemplateRepository::setDatabasePath(const std::string& path) {
    dbPath_ = path;
    DatabaseManager::instance().init(dbPath_);
}

void TemplateRepository::initializeTables() {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS response_templates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            language TEXT NOT NULL DEFAULT 'es',
            pattern_type INTEGER NOT NULL,
            template_text TEXT NOT NULL,
            slots TEXT,
            priority INTEGER DEFAULT 0,
            frequency INTEGER DEFAULT 1,
            context_keywords TEXT,
            last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "TemplateRepository: error creating table: " << err << std::endl;
        sqlite3_free(err);
    }
    const char* addLastUsed = "ALTER TABLE response_templates ADD COLUMN last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP";
    sqlite3_exec(db, addLastUsed, nullptr, nullptr, nullptr);
}

// Serialization helpers
static std::string serializeVector(const std::vector<std::string>& vec) {
    std::stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) ss << ',';
        ss << vec[i];
    }
    return ss.str();
}

static std::vector<std::string> deserializeVector(const std::string& str) {
    std::vector<std::string> res;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) res.push_back(item);
    }
    return res;
}

void TemplateRepository::save(ResponseTemplate& tmpl) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    if (tmpl.id <= 0) {
        const char* sql = R"(
            INSERT INTO response_templates (language, pattern_type, template_text, slots, priority, frequency, context_keywords, last_used)
            VALUES (?,?,?,?,?,?,?, CURRENT_TIMESTAMP)
        )";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, tmpl.language.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, static_cast<int>(tmpl.patternType));
        sqlite3_bind_text(stmt, 3, tmpl.templateText.c_str(), -1, SQLITE_STATIC);
        std::string slotsStr = serializeVector(tmpl.slots);
        sqlite3_bind_text(stmt, 4, slotsStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, tmpl.priority);
        sqlite3_bind_int(stmt, 6, tmpl.frequency_);
        std::string keywordsStr = serializeVector(tmpl.contextKeywords);
        sqlite3_bind_text(stmt, 7, keywordsStr.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            tmpl.id = static_cast<int>(sqlite3_last_insert_rowid(db));
        }
        sqlite3_finalize(stmt);
    } else {
        // Update: increment frequency and update last_used
        const char* sql = R"(
            UPDATE response_templates
            SET language=?, pattern_type=?, template_text=?, slots=?, priority=?, frequency=frequency+1, context_keywords=?, last_used=CURRENT_TIMESTAMP
            WHERE id=?
        )";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, tmpl.language.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, static_cast<int>(tmpl.patternType));
        sqlite3_bind_text(stmt, 3, tmpl.templateText.c_str(), -1, SQLITE_STATIC);
        std::string slotsStr = serializeVector(tmpl.slots);
        sqlite3_bind_text(stmt, 4, slotsStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, tmpl.priority);
        std::string keywordsStr = serializeVector(tmpl.contextKeywords);
        sqlite3_bind_text(stmt, 6, keywordsStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 7, tmpl.id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

std::optional<ResponseTemplate> TemplateRepository::loadById(int id) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return std::nullopt;

    const char* sql = "SELECT language, pattern_type, template_text, slots, priority, frequency, context_keywords, strftime('%s', last_used) FROM response_templates WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    ResponseTemplate tmpl;
    tmpl.id = id;
    tmpl.language    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    tmpl.patternType = static_cast<PatternType>(sqlite3_column_int(stmt, 1));
    tmpl.templateText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    tmpl.slots = deserializeVector(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    tmpl.priority = sqlite3_column_int(stmt, 4);
    tmpl.frequency_ = sqlite3_column_int(stmt, 5);
    tmpl.contextKeywords = deserializeVector(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
    tmpl.timestamp_ = static_cast<uint32_t>(sqlite3_column_int(stmt, 7));
    sqlite3_finalize(stmt);
    return tmpl;
}

std::vector<ResponseTemplate> TemplateRepository::loadAll() {
    std::vector<ResponseTemplate> result;
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return result;

    const char* sql = "SELECT id, language, pattern_type, template_text, slots, priority, frequency, context_keywords, strftime('%s', last_used) FROM response_templates";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResponseTemplate tmpl;
        tmpl.id = sqlite3_column_int(stmt, 0);
        tmpl.language = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tmpl.patternType = static_cast<PatternType>(sqlite3_column_int(stmt, 2));
        tmpl.templateText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        tmpl.slots = deserializeVector(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        tmpl.priority = sqlite3_column_int(stmt, 5);
        tmpl.frequency_ = sqlite3_column_int(stmt, 6);
        tmpl.contextKeywords = deserializeVector(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        tmpl.timestamp_ = static_cast<uint32_t>(sqlite3_column_int(stmt, 8));
        result.push_back(tmpl);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool TemplateRepository::remove(int id) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM response_templates WHERE id=?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

uint32_t TemplateRepository::getLastTime(int id) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db) return 0;
    const char* sql = "SELECT strftime('%s', last_used) FROM response_templates WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    uint32_t ts = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ts = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return ts;
}

bool TemplateRepository::updateTime(const ResponseTemplate& tmpl) {
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath_);
    if (!db || tmpl.id <= 0) return false;
    const char* sql = "UPDATE response_templates SET last_used = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, tmpl.id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

void TemplateRepository::loadDefaultIfEmpty() {
    auto all = loadAll();
    if (!all.empty()) return;

    TemplateMatcher defaultMatcher;
    defaultMatcher.loadDefaultTemplates();
    for (auto tmpl : defaultMatcher.getAll()) {
        save(tmpl);
    }
}
