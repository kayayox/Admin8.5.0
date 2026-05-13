/**
 * @file Word.hpp
 * @brief Domain entity representing a word with its grammatical and semantic attributes.
 * @author Soubhi Khayat Najjar
 * @date 2026
 * @note Persistence operations are delegated to WordRepository (persistence layer).
 *       This class has no database dependencies.
 */

#ifndef ADMIN850_WORD_HPP
#define ADMIN850_WORD_HPP

#include "../common/types.hpp"
#include "../dialogue/PatternCorrelator.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

/**
 * @class Word
 * @brief Models a lexical unit with part-of-speech tagging, morphological features,
 *        and a confidence score. It can learn semantically related words from a
 *        PatternCorrelator and generate a structured meaning description.
 *
 * All setters are public because fields may need adjustment while confidence < 0.75.
 * Future versions may enforce immutability based on confidence thresholds.
 */
class Word {
public:
    /// Default constructor (creates an empty word).
    Word() = default;

    /**
     * @brief Constructs a Word with a given lexical form.
     * @param word The word string (should not be empty for valid usage).
     */
    explicit Word(const std::string& word);

    // --- Getters ---
    [[nodiscard]] const std::string& getWord()        const noexcept { return word_; }
    [[nodiscard]] const std::string& getMeaning()     const noexcept { return meaning_; }
    [[nodiscard]] WordType           getType()        const noexcept { return type_; }
    [[nodiscard]] Quantity           getQuantity()    const noexcept { return quantity_; }
    [[nodiscard]] Tense              getTense()       const noexcept { return tense_; }
    [[nodiscard]] Gender             getGender()      const noexcept { return gender_; }
    [[nodiscard]] Degree             getDegree()      const noexcept { return degree_; }
    [[nodiscard]] Person             getPerson()      const noexcept { return person_; }
    [[nodiscard]] float              getConfidence()  const noexcept { return confidence_; }
    [[nodiscard]] uint32_t           getTime()        const noexcept { return timestamp_; }
    [[nodiscard]] uint32_t           getFrequency()   const noexcept { return frequency_; }

    /**
     * @brief Returns a copy of the related words list to preserve encapsulation.
     * @return Vector of (word, weight) pairs.
     */
    [[nodiscard]] std::vector<std::pair<std::string, double>> getRelated() const {
        return relatedWords_;
    }

    // --- Setters (modify entity in memory; no automatic persistence) ---
    void setWord(const std::string& w)       { word_ = w; }
    void setMeaning(const std::string& meaning);
    void setType(WordType type)              { type_ = type; }
    void setQuantity(Quantity qty)           { quantity_ = qty; }
    void setTense(Tense tense)               { tense_ = tense; }
    void setGender(Gender gender)            { gender_ = gender; }
    void setDegree(Degree degree)            { degree_ = degree; }
    void setPerson(Person person)            { person_ = person; }
    void setConfidence(float confidence);
    void setTimestamp(uint32_t ts)           { timestamp_ = ts; }
    void setFrequency(uint32_t freq)         { frequency_ = freq; }
    void incrementFrequency()                { ++frequency_; }

    // --- Related words management ---
    void addRelated(const std::string& relatedWord, double weight);
    void clearRelated() noexcept { relatedWords_.clear(); }

    /**
     * @brief Learns related words using a trained correlator that stores
     *        (currentWord, previousPattern) → nextPattern associations.
     * @param correlator The pattern correlator (non‑const because it may update internal caches).
     * @param contextWords List of words in the current context (used to find the previous word).
     * @param minConfidence Minimum confidence threshold to accept a relation.
     */
    void learnRelationsFromCorrelator(PatternCorrelator& correlator,
                                      const std::vector<std::string>& contextWords,
                                      double minConfidence = 0.3);

    /**
     * @brief Builds a human‑readable structured meaning from the current grammatical attributes.
     * @note This overwrites the current meaning string. Call it explicitly after attribute changes.
     */
    void generateStructuredMeaning();

private:
    std::string word_{};
    std::string meaning_{};
    WordType    type_{WordType::UNDEFINED};
    Quantity    quantity_{Quantity::NONE};
    Tense       tense_{Tense::UNDETERMINED};
    Gender      gender_{Gender::NEUTER};
    Degree      degree_{Degree::NONE};
    Person      person_{Person::NONE};
    float       confidence_{0.0f};

    uint32_t timestamp_ = 0;                ///< Last access (Unix timestamp)
    uint32_t frequency_ = 1;                ///< Counter of how many times the word has been used/accessed

    /// List of semantically related words with their association weights.
    std::vector<std::pair<std::string, double>> relatedWords_;
};

#endif // ADMIN850_WORD_HPP
