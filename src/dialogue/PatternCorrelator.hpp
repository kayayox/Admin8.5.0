/**
 * @file PatternCorrelator.hpp
 * @brief Correlator that stores trigram relationships in its own SQLite database.
 *        For suffix "_CellType" adds last_used timestamp to patterns table.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef PATTERN_CORRELATOR_HPP
#define PATTERN_CORRELATOR_HPP

#include "../utils/PatternUtils.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <ctime>

class PatternCorrelator {
public:
    PatternCorrelator(const std::string& dbPath, const std::string& tableSuffix = "");
    ~PatternCorrelator();

    void record(const std::string& current, const WordPattern& prevPattern,
                const WordPattern& nextPattern, float weight = 1.0f);

    bool query(const std::string& current, const WordPattern& prevPattern,
               std::vector<std::pair<WordPattern, double>>& outcomes);

    void learnFromText(const std::string& text, size_t windowSize = 1);
    void purgeLowWeightCorrelations(double minWeight = 0.01);

    /**
     * @brief Devuelve el último timestamp previo registrado (solo útil si tableSuffix == "_CellType").
     * @return Timestamp Unix anterior a la última actualización, o 0 si no aplica.
     */
    int64_t getPreviousPatternTimestamp() const { return previousPatternTimestamp; }

    /**
     * @brief Elimina patrones que no se han usado desde hace más de 'daysOld' días.
     *        Solo tiene efecto si el sufijo de tabla es "_CellType".
     *        Requiere que las claves foráneas tengan ON DELETE CASCADE.
     * @param daysOld Antigüedad en días (por defecto 30).
     */
    void cleanOldPatterns(int daysOld = 30);

private:
    sqlite3* db;
    std::string tableSuffix;

    std::string getWordsTable() const { return "words" + tableSuffix; }
    std::string getPatternsTable() const { return "correlation_patterns" + tableSuffix; }
    std::string getCorrelationsTable() const { return "correlations" + tableSuffix; }
    bool hasTimestampColumn() const { return tableSuffix == "_CellType"; }

    // Caches
    std::map<std::string, int> wordToId;
    std::map<int, std::string> idToWord;
    std::map<std::string, int> patternSerializedToId;
    std::map<int, WordPattern> idToPattern;

    int64_t previousPatternTimestamp = 0;  // Almacena el timestamp previo al update (solo _CellType)

    // Prepared statements
    sqlite3_stmt* stmtGetWordId = nullptr;
    sqlite3_stmt* stmtInsertWord = nullptr;
    sqlite3_stmt* stmtGetPatternId = nullptr;
    sqlite3_stmt* stmtInsertPattern = nullptr;
    sqlite3_stmt* stmtFindCorrelation = nullptr;
    sqlite3_stmt* stmtUpdateCorrelation = nullptr;
    sqlite3_stmt* stmtInsertCorrelation = nullptr;
    sqlite3_stmt* stmtGetTotalWeight = nullptr;
    sqlite3_stmt* stmtGetNextPatterns = nullptr;
    sqlite3_stmt* stmtUpdatePatternTimestamp = nullptr;

    int getWordId(const std::string& word);
    int getPatternId(const WordPattern& pattern);
    void updateCorrelation(int currWordId, int prevPatternId, int nextPatternId, float weight);
    void prepareStatements();
};

#endif // PATTERN_CORRELATOR_HPP
