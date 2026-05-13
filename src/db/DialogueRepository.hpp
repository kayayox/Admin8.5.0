/**
 * @file DialogueRepository.hpp
 * @brief Repository for persisting dialogues and feedback in the semantic database.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_DIALOGUE_REPOSITORY_HPP
#define ADMIN850_DIALOGUE_REPOSITORY_HPP

#include "../core/Dialogue.hpp"
#include "../core/Pattern.hpp"
#include <string>
#include <vector>

// Forward declaration for Sentence (needed by SentenceRepository)
class Sentence;

class DialogueRepository {
public:
    static void setDatabasePath(const std::string& path);
    static std::string getDatabasePath();
    static void initializeTables();

    /**
     * @brief Persists a dialogue. Premise and hypothesis sentences are saved automatically
     *        if they don't have an ID yet.
     */
    static void saveDialogue(const Sentence& premise, const Sentence& hypothesis,
                             const Pattern& pattern, float creativity);

    /**
     * @brief Records feedback about a word classification.
     * @param word         The word text.
     * @param proposedType The type that was assigned by the system.
     * @param correctType  The correct type (ground truth).
     * @param correct      Whether the proposal matched the correct type.
     */
    static void registerFeedback(const std::string& word,
                                 WordType proposedType,
                                 WordType correctType,
                                 bool correct);

    /// Builds a corpus of all words stored in sentence blocks.
    static std::vector<std::string> buildCorpus();

    /// Loads the entire dialogue history.
    static DialogueHistory loadHistory();

    /**
     * @brief Loads dialogues whose premise contains any of the given words.
     * @param words List of words to search for.
     * @param limit Maximum number of dialogues to return.
     */
    static std::vector<Dialogue> loadHistoryContainingWords(const std::vector<std::string>& words,
                                                            int limit = 50);

    /// Merges duplicate dialogues (same premise, hypothesis, pattern type, creativity).
    static int mergeDuplicateDialogues();

private:
    static std::string dbPath_;
    static int getOrCreateSentenceId(const Sentence& sentence);
};

#endif // ADMIN850_DIALOGUE_REPOSITORY_HPP
