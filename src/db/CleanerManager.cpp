#include "CleanerManager.hpp"
#include "DatabaseManager.hpp"
#include "../dialogue/PatternCorrelator.hpp"
#include <sqlite3.h>
#include <iostream>
#include <ctime>

// ---------------------------------------------------------------------------
// Definición de miembros estáticos
// ---------------------------------------------------------------------------
std::string Cleaner::semanticDbPath_;
std::string Cleaner::patternDbPath_;
Config Cleaner::config_ = {0.01f, 1.0f, 0}; // valores por defecto

// ---------------------------------------------------------------------------
// Configuración
// ---------------------------------------------------------------------------
void Cleaner::setDatabasePaths(const std::string& semanticPath,
                               const std::string& patternPath,
                               const std::string& templatePath) {
    semanticDbPath_ = semanticPath;
    patternDbPath_ = patternPath;
    DatabaseManager::instance().init(semanticPath);
    DatabaseManager::instance().init(patternPath);
    WordRepository::setDatabasePath(semanticPath);
    SentenceRepository::setDatabasePath(semanticPath);
    DialogueRepository::setDatabasePath(semanticPath);
    TemplateRepository::setDatabasePath(semanticPath);
    PatternRepository::setDatabasePath(patternPath);
}

void Cleaner::setConfig(const Config& cfg) {
    config_ = cfg;
}

// ---------------------------------------------------------------------------
// Limpieza semántica: palabras, oraciones, diálogos, plantillas
// ---------------------------------------------------------------------------
void Cleaner::cleanSemanticDatabase() {
    if (semanticDbPath_.empty()) return;
    cleanWords();
    cleanSentences();
    cleanDialogues();
    cleanTemplates();
}

void Cleaner::cleanWords() {
    sqlite3* db = DatabaseManager::instance().getHandle(semanticDbPath_);
    if (!db) return;

    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    std::string sql = "DELETE FROM words WHERE frequency < ?";
    if (config_.maxTimeago > 0) {
        sql += " OR (strftime('%s', last_used) < ?)";
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, config_.minFrecuency);
    if (config_.maxTimeago > 0) {
        uint32_t cutoff = now - config_.maxTimeago;
        sqlite3_bind_int(stmt, 2, static_cast<int>(cutoff));
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Eliminar palabras huérfanas de related_words (por FK CASCADE ya debería ocurrir)
}

void Cleaner::cleanSentences() {
    sqlite3* db = DatabaseManager::instance().getHandle(semanticDbPath_);
    if (!db) return;

    // Eliminar oraciones con frecuencia baja o muy antiguas
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    std::string sql = "SELECT id FROM sentences WHERE frequency < ?";
    if (config_.maxTimeago > 0) {
        sql += " OR (strftime('%s', last_used) < ?)";
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, config_.minFrecuency);
    if (config_.maxTimeago > 0) {
        uint32_t cutoff = now - config_.maxTimeago;
        sqlite3_bind_int(stmt, 2, static_cast<int>(cutoff));
    }

    std::vector<int> toDelete;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        toDelete.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    for (int id : toDelete) {
        SentenceRepository::remove(id);
    }
}

void Cleaner::cleanDialogues() {
    sqlite3* db = DatabaseManager::instance().getHandle(semanticDbPath_);
    if (!db) return;

    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    std::string sql = "DELETE FROM dialogs WHERE frequency < ?";
    if (config_.maxTimeago > 0) {
        sql += " OR (strftime('%s', last_used) < ?)";
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, config_.minFrecuency);
    if (config_.maxTimeago > 0) {
        uint32_t cutoff = now - config_.maxTimeago;
        sqlite3_bind_int(stmt, 2, static_cast<int>(cutoff));
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Opcional: eliminar diálogos cuyas premisa/hipótesis ya no existan
    const char* orphanSql = "DELETE FROM dialogs WHERE premise_id NOT IN (SELECT id FROM sentences) OR hypothesis_id NOT IN (SELECT id FROM sentences)";
    sqlite3_exec(db, orphanSql, nullptr, nullptr, nullptr);
}

void Cleaner::cleanTemplates() {
    sqlite3* db = DatabaseManager::instance().getHandle(semanticDbPath_);
    if (!db) return;

    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    std::string sql = "SELECT id FROM response_templates WHERE frequency < ?";
    if (config_.maxTimeago > 0) {
        sql += " OR (strftime('%s', last_used) < ?)";
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, config_.minFrecuency);
    if (config_.maxTimeago > 0) {
        uint32_t cutoff = now - config_.maxTimeago;
        sqlite3_bind_int(stmt, 2, static_cast<int>(cutoff));
    }

    std::vector<int> toDelete;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        toDelete.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    for (int id : toDelete) {
        TemplateRepository::remove(id);
    }
}

// ---------------------------------------------------------------------------
// Limpieza de base de patrones (PatternRepository + PatternCorrelator)
// ---------------------------------------------------------------------------
void Cleaner::cleanPatternDatabase() {
    if (patternDbPath_.empty()) return;
    cleanPatterns();
    cleanPatternCorrelations();
}

void Cleaner::cleanPatterns() {
    sqlite3* db = DatabaseManager::instance().getHandle(patternDbPath_);
    if (!db) return;

    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    std::string sql = "DELETE FROM patterns WHERE frequency < ?";
    if (config_.maxTimeago > 0) {
        sql += " OR (strftime('%s', last_used) < ?)";
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, config_.minFrecuency);
    if (config_.maxTimeago > 0) {
        uint32_t cutoff = now - config_.maxTimeago;
        sqlite3_bind_int(stmt, 2, static_cast<int>(cutoff));
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Cleaner::cleanPatternCorrelations() {
    // Usar PatternCorrelator para purgar correlaciones de bajo peso
    // Se asume que existen dos tablas: pattern_correlations y pattern_correlations_chunk
    PatternCorrelator corrNorm(patternDbPath_, "");
    corrNorm.purgeLowWeightCorrelations(config_.minRelevant);

    PatternCorrelator corrChunk(patternDbPath_, "_chunk");
    corrChunk.purgeLowWeightCorrelations(config_.minRelevant);
}

// ---------------------------------------------------------------------------
// Limpieza completa
// ---------------------------------------------------------------------------
void Cleaner::cleanAll() {
    cleanSemanticDatabase();
    cleanPatternDatabase();

    // Compactar ambas bases después de eliminaciones masivas
    sqlite3* dbSem = DatabaseManager::instance().getHandle(semanticDbPath_);
    if (dbSem) sqlite3_exec(dbSem, "VACUUM", nullptr, nullptr, nullptr);

    sqlite3* dbPat = DatabaseManager::instance().getHandle(patternDbPath_);
    if (dbPat) sqlite3_exec(dbPat, "VACUUM", nullptr, nullptr, nullptr);
}
