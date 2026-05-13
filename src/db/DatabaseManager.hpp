/**
 * @file DatabaseManager.hpp
 * @brief Singleton manager for multiple SQLite database connections.
 * @author Soubhi Khayat Najjar
 * @date 2026
 *
 * @note Each database path gets its own connection handle. Used for semantic,
 *       pattern, and future temporal databases.
 */

#ifndef ADMIN850_DATABASE_MANAGER_HPP
#define ADMIN850_DATABASE_MANAGER_HPP

#include <sqlite3.h>
#include <string>
#include <unordered_map>

/**
 * @class DatabaseManager
 * @brief Maintains a map of file path → sqlite3* connections.
 *
 * Non-copyable singleton. Provides convenience methods to open, close,
 * and retrieve handles, plus database cleanup operations.
 */
class DatabaseManager {
public:
    /// Returns the singleton instance.
    static DatabaseManager& instance();

    /**
     * @brief Opens or retrieves an already open connection for the given path.
     * @param dbPath File path to the SQLite database.
     * @return true if the connection was successfully opened or already existed.
     */
    bool init(const std::string& dbPath);

    /// Closes the connection associated with the given path.
    void close(const std::string& dbPath);

    /// Closes all open connections.
    void closeAll();

    /**
     * @brief Returns the raw sqlite3 handle for a path, or nullptr if not open.
     */
    sqlite3* getHandle(const std::string& dbPath) const;

    /**
     * @brief Prepares an SQL statement on the connection identified by dbPath.
     * @param dbPath Database identifier.
     * @param sql    SQL string to prepare.
     * @param stmt   Output: prepared statement handle.
     * @return true on success.
     */
    bool prepareStatement(const std::string& dbPath,
                          const char* sql,
                          sqlite3_stmt** stmt) const;

    // --- Cleanup routines ---
    static int  cleanSemanticDatabase(const std::string& dbPath);
    static int  cleanPatternDatabase(const std::string& dbPath);
    static void cleanAll(const std::string& semanticPath,
                         const std::string& patternPath);

    // Non-copyable
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

private:
    DatabaseManager() = default;
    ~DatabaseManager() { closeAll(); }

    std::unordered_map<std::string, sqlite3*> connections_;
};

/**
 * @brief Utility function that prepares an SQL statement with error logging.
 * @param db   SQLite database handle.
 * @param sql  SQL string.
 * @param stmt Output: prepared statement.
 * @return true on success.
 *
 * Defined in DatabaseManager.cpp to avoid <iostream> dependency in the header.
 */
bool prepareSqlStatement(sqlite3* db, const char* sql, sqlite3_stmt** stmt);

#endif // ADMIN850_DATABASE_MANAGER_HPP
