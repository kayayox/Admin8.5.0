/**
 * @file Sentence.hpp
 * @brief Domain entity representing a sentence as a sequence of blocks
 *        (words/tokens) with their grammatical types.
 * @author Soubhi Khayat Najjar
 * @date 2026
 * @note Replaces the old Oracion class. No persistence dependencies.
 *       Construction from a vector<Word> requires the full definition of Word
 *       and is implemented in Sentence.cpp.
 */

#ifndef ADMIN850_SENTENCE_HPP
#define ADMIN850_SENTENCE_HPP

#include "../common/types.hpp"
#include <string>
#include <vector>
#include <cstdint>

// Forward declaration to avoid circular includes
class Word;

/**
 * @struct Block
 * @brief A single token within a sentence: its text and word type.
 */
struct Block {
    std::string text{};                     ///< The token text.
    WordType    type = WordType::UNDEFINED; ///< Part-of-speech or token type.
};

/**
 * @class Sentence
 * @brief Models a sentence as an ordered collection of Blocks, with an
 *        identified key block (main noun or verb), tense, and frequency.
 */
class Sentence {
public:
    Sentence() = default;

    /**
     * @brief Constructs a Sentence from a vector of fully classified Words.
     * @param words The words forming the sentence.
     */
    explicit Sentence(const std::vector<Word>& words);

    // --- Getters ---
    [[nodiscard]] int                   getId()        const noexcept { return id_; }
    [[nodiscard]] const std::vector<Block>& getBlocks() const noexcept { return blocks_; }
    [[nodiscard]] Tense                 getTense()     const noexcept { return tense_; }
    [[nodiscard]] const Block&          getKey()       const noexcept { return key_; }
    [[nodiscard]] float                 getFrequency() const noexcept { return frequency_; }
    [[nodiscard]] int                   getNumBlocks() const noexcept { return static_cast<int>(blocks_.size()); }
    [[nodiscard]] uint32_t              getTimestamp() const noexcept { return timestam_; }

    /**
     * @brief Returns the sequence of word types for this sentence.
     * @return Vector of WordType in the order they appear.
     */
    [[nodiscard]] std::vector<WordType> getTypeSequence() const;

    /**
     * @brief Reconstructs the sentence as a plain text string.
     * @return Space-separated word tokens.
     */
    [[nodiscard]] std::string toString() const;

    // --- Setters (in-memory modifications only) ---
    void setId(int id)             { id_ = id; }
    void setKey(const Block& key)  { key_ = key; }
    void setFrequency(float freq)  { frequency_ = freq; }
    void setTense(Tense tense)     { tense_ = tense; }
    void setTimestamp(uint32_t ts) { timestam_ = ts; }
    void incrementFrequency()      { frequency_ += 1.0f; }

    // --- Modifiers for hypothesis generation ---
    void addBlock(const std::string& text, WordType type);
    void insertBlockAtStart(const std::string& text, WordType type);
    void insertNegation();                            ///< Inserts "no" after the first verb.
    void replaceNoun(const std::string& newWord);     ///< Replaces the first noun.

private:
    int                id_ = -1;
    std::vector<Block> blocks_;
    Tense              tense_ = Tense::UNDETERMINED;
    Block              key_;              ///< Key block (main noun or verb).
    float              frequency_ = 1.0f;
    uint32_t           timestam_ = 0;     ///< Last access timestamp (Unix seconds)
};

#endif // ADMIN850_SENTENCE_HPP
