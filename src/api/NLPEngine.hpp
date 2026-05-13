/**
 * @file NLPEngine.hpp
 * @brief Main facade of the NLP engine for interactive applications.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_NLP_ENGINE_HPP
#define ADMIN850_NLP_ENGINE_HPP

#include <string>
#include <vector>
#include <memory>

// ============================================================================
// Public data structures
// ============================================================================

struct WordInfo {
    std::string word;
    std::string type;                     // "Noun", "Verb", etc.
    float       confidence = 0.0f;        // 0..1
    std::string meaning;
    std::string quantity;                 // "Singular", "Plural", ""
    std::string tense;                    // "Past", "Present", "Future", ""
    std::string gender;                   // "Masculine", "Feminine", "Neuter", ""
    std::string person;                   // "First", "Second", "Third", ""
    std::string degree;                   // "Positive", "Superlative", ...
    std::vector<std::pair<std::string, double>> relatedWords;
};

struct Prediction {
    std::string word;
    double      probability = 0.0;
};

// ============================================================================
// NLP Engine facade class
// ============================================================================

class NLPEngine {
public:
    NLPEngine();
    ~NLPEngine();

    /**
     * @brief Initialises the engine with paths for the three databases.
     * @param semanticDbPath  Path to the semantic database (words, sentences, dialogs).
     * @param patternDbPath   Path to the pattern/statistics database.
     * @param temporalDbPath  Optional path for temporary data; defaults to in‑memory.
     * @return true on success.
     */
    bool initialize(const std::string& semanticDbPath,
                    const std::string& patternDbPath,
                    const std::string& temporalDbPath = "");

    /// Shuts down all connections and releases resources.
    void shutdown();

    /// Enables or disables debug output.
    void setDebugMode(bool enable);

    /**
     * @brief Sets the active language for morphological analysis and tag statistics.
     * @param lang Language code ("es" or "en").
     */
    void setLanguage(const std::string& lang);

    // --- Learning ---

    /**
     * @brief Learns from raw text (trains n‑grams and contextual correlations).
     * @param text Input text (multiple sentences allowed).
     */
    void learnText(const std::string& text);

    // --- Sentence processing ---

    /**
     * @brief Fully processes a sentence: tokenises, classifies, learns patterns,
     *        stores in DB, and updates conversational context.
     * @param sentence The input sentence.
     * @return Detailed information for each word.
     */
    std::vector<WordInfo> processSentence(const std::string& sentence);

    // --- Prediction ---

    /**
     * @brief Predicts the next word(s) given a context string.
     * @param currentWords The preceding words (at least one).
     * @return Ranked list of predictions.
     */
    std::vector<Prediction> predictNext(const std::string& currentWords, bool& type);

    // --- Response generation ---

    /**
     * @brief Generates a hypothesis (response) for a given premise.
     * @param premise The input sentence.
     * @return Generated response text.
     */
    std::string generateResponse(const std::string& premise);

    // --- Feedback & context ---

    /**
     * @brief Provides feedback on the last dialogue interaction.
     * @param positive true if the response was correct/helpful.
     */
    void provideDialogueFeedback(bool positive);

    /**
     * @brief Clears the conversational context (recent words, last sentences).
     */
    void resetContext();

    // --- Word lookup ---

        /**
     * @brief Manually corrects the classification of a word.
     * @param word The word to correct.
     * @param correctType The correct type as string (e.g., "Noun", "Verb").
     */
    void correctWord(const std::string& word, const std::string& correctType);

    /**
     * @brief Re‑processes the last sentence, updating patterns and context.
     */
    void reprocessLastSentence();

    /**
     * @brief Retrieves stored information about a single word.
     * @param word The word to look up (mutated to remove punctuation).
     * @return WordInfo if found, otherwise empty.
     */
    WordInfo getWordInfo(std::string& word);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // ADMIN850_NLP_ENGINE_HPP
