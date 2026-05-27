// OutputFormatter.cpp
#include "OutputFormatter.hpp"
#include "../db/DatabaseManager.hpp"
#include <fstream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void OutputFormatter::saveToJSON(const std::unordered_map<std::string, std::string>& data,
                                 const std::string& path, const std::string& docName) {
    json j;
    j["document"] = docName;
    j["extracted_data"] = data;
    std::ofstream out(path);
    if (out) out << j.dump(4);
}

void OutputFormatter::saveToCSV(const std::unordered_map<std::string, std::string>& data,
                                const std::string& path) {
    std::ofstream out(path);
    if (!out) return;
    out << "Keyword,Value\n";
    for (const auto& [k, v] : data)
        out << "\"" << k << "\",\"" << v << "\"\n";
}

void OutputFormatter::saveToXML(const std::unordered_map<std::string, std::string>& data,
                                const std::string& path) {
    std::ofstream out(path);
    if (!out) return;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<extracted_data>\n";
    for (const auto& [k, v] : data)
        out << "  <field name=\"" << k << "\">" << v << "</field>\n";
    out << "</extracted_data>\n";
}

void OutputFormatter::saveToSQLite(const std::unordered_map<std::string, std::string>& data,
                                   const std::string& dbPath, const std::string& docName) {
    DatabaseManager::instance().init(dbPath);
    sqlite3* db = DatabaseManager::instance().getHandle(dbPath);
    if (!db) return;
    const char* createSQL = "CREATE TABLE IF NOT EXISTS extracted_data ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "document_name TEXT, keyword TEXT, value TEXT, "
                            "extraction_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";
    char* err = nullptr;
    sqlite3_exec(db, createSQL, nullptr, nullptr, &err);
    sqlite3_free(err);
    const char* insertSQL = "INSERT INTO extracted_data (document_name, keyword, value) VALUES (?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    for (const auto& [k, v] : data) {
        sqlite3_bind_text(stmt, 1, docName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, k.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, v.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
}
