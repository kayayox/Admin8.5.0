/**
 * @file SentenceRepository.hpp
 * @brief Repository for persisting Sentence entities into the semantic database.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_SENTENCE_REPOSITORY_HPP
#define ADMIN850_SENTENCE_REPOSITORY_HPP

#include "../core/Sentence.hpp"
#include <string>

class SentenceRepository {
public:
    static void setDatabasePath(const std::string& path);
    static std::string getDatabasePath();

    /// Creates tables (sentences, blocks) if they don't exist.
    static void initializeTables();

    /**
     * @brief Persists a sentence. Inserts if id == -1, updates otherwise.
     * @param sentence The sentence to save. Its id is assigned on insert.
     */
    static void save(Sentence& sentence);

    /// Loads a sentence by its ID.
    static Sentence loadById(int id);

    /// Loads a sentence by its key (word + type).
    static Sentence loadByKey(const std::string& keyWord, WordType keyType);

    /// Removes a sentence and its blocks.
    static bool remove(int id);

    static uint32_t getLastTime(int id);

    static bool updateTime(const Sentence& sentence);
    /// Loads all sentences.
    static std::vector<Sentence> loadAll();

    /**
     * @brief Finds sentences that contain any of the given words.
     * @param words List of words to search for.
     * @param limit Maximum number of sentences to return.
     */
    static std::vector<Sentence> findSentencesContainingWords(const std::vector<std::string>& words,
                                                               int limit = 100);

    /// Merges duplicate sentences (by normalized text) and updates references.
    static int mergeDuplicateSentences();

private:
    static std::string dbPath_;
    static int getOrCreateOracionId(const Sentence& sentence); // kept for compatibility
};

#endif // ADMIN850_SENTENCE_REPOSITORY_HPP
