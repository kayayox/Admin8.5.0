/**
 * @file PatternRepository.hpp
 * @brief Repository for persisting grammatical patterns (Pattern) into the pattern database.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_PATTERN_REPOSITORY_HPP
#define ADMIN850_PATTERN_REPOSITORY_HPP

#include "../core/Pattern.hpp"
#include <iostream>
#include <optional>
#include <vector>

class PatternRepository {
public:
    static void setDatabasePath(const std::string& path);
    static std::string getDatabasePath();
    static void initializeTables();

    /**
     * @brief Saves a pattern (increments frequency if it already exists).
     * @param pattern The pattern to persist.
     */
    static void save(const Pattern& pattern);

    /**
     * @brief Finds a pattern matching exactly the given word-type sequence.
     * @param sequence    The word-type sequence to look for.
     * @param outSimiliarity Output: 1.0 if exact match, 0.0 otherwise.
     * @return The matching pattern if found, std::nullopt otherwise.
     */
    static std::optional<Pattern> findExactMatch(const std::vector<WordType>& sequence,
                                                  float& outSimiliarity);

    /// Loads all patterns from the database.
    static std::vector<Pattern> loadAll();

    /// Removes a pattern.
    static bool remove(const Pattern& pattern);

    static uint32_t getLastTime(const Pattern& pattern);

    static bool updateTime(const Pattern& pattern);
};

#endif // ADMIN850_PATTERN_REPOSITORY_HPP
