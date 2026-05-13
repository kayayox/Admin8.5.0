/**
 * @file WordRepository.hpp
 * @brief Repository for persisting Word entities into the semantic database.
 * @author Soubhi Khayat Najjar
 * @date 2026
 * @note Relies on DatabaseManager and the Word entity. All methods are static.
 */

#ifndef ADMIN850_WORD_REPOSITORY_HPP
#define ADMIN850_WORD_REPOSITORY_HPP

#include "../core/Word.hpp"
#include <string>

/**
 * @class WordRepository
 * @brief Provides static methods for CRUD operations on Word objects.
 */
class WordRepository {
public:
    /// Configures the path for the semantic database. Must be called before any other operation.
    static void setDatabasePath(const std::string& path);
    static std::string getDatabasePath();

    /// Creates the necessary tables if they do not exist.
    static void initializeTables();

    /**
     * @brief Persists a Word (insert or update).
     * @param word The word to save.
     */
    static void save(const Word& word);

    /**
     * @brief Loads a Word by its text form.
     * @param wordText The word string to search for.
     * @param outWord  Output parameter filled if found.
     * @return True if the word existed, false otherwise.
     */
    static bool load(const std::string& wordText, Word& outWord);

    /// Checks whether a word exists in the database.
    static bool exists(const std::string& wordText);

    /// Removes a word from the database.
    static bool remove(const std::string& wordText);

    static uint32_t getLastTime(const std::string word);

    static bool updateTime(const Word word);
    /**
     * @brief Retrieves related words through recursive traversal.
     * @param word      Starting word.
     * @param maxDepth  Maximum recursion depth.
     * @param maxTotal  Maximum number of related words to collect.
     * @return Vector of related word strings.
     */
    static std::vector<std::string> getRelatedWordsRecursive(const std::string& word,
                                                             int maxDepth = 2,
                                                             int maxTotal = 50);

private:
    static std::string dbPath_;
    static int getOrCreateWordId(const Word& word); // auxiliary – not implemented in shown code, kept for compatibility
};

#endif // ADMIN850_WORD_REPOSITORY_HPP
