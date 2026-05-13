/**
 * @file Sentence.cpp
 * @brief Implementation of the Sentence entity.
 * @author Soubhi Khayat Najjar
 * @date 2026
 * @note The constructor from vector<Word> requires the full Word definition,
 *       included here.
 */

#include "Sentence.hpp"
#include "Word.hpp"
#include <algorithm>
#include <ctime>

Sentence::Sentence(const std::vector<Word>& words) {
    blocks_.reserve(words.size());
    for (const auto& word : words) {
        Block block;
        block.text = word.getWord();
        block.type = word.getType();
        blocks_.push_back(block);

        // Key selection logic:
        // - The first verb that appears while tense is still undetermined sets the key.
        // - If a noun appears later, it overrides the key (noun has priority).
        if (word.getType() == WordType::VERB && tense_ == Tense::UNDETERMINED) {
            tense_ = word.getTense();
            key_ = block;
            frequency_ = 1.0f;
        }
        if (word.getType() == WordType::NOUN) {
            key_ = block;
            frequency_ = 1.0f;
        }
    }

    // If no noun was found, fall back to the first verb as key.
    if (key_.type != WordType::NOUN) {
        auto it = std::find_if(blocks_.begin(), blocks_.end(),
                               [](const Block& b) { return b.type == WordType::VERB; });
        if (it != blocks_.end()) {
            key_ = *it;
            frequency_ = 1.2f;
        }
    }
    timestam_ = static_cast<uint32_t>(std::time(nullptr));
}

std::vector<WordType> Sentence::getTypeSequence() const {
    std::vector<WordType> typeSeq;
    typeSeq.reserve(blocks_.size());
    for (const auto& block : blocks_) {
        typeSeq.push_back(block.type);
    }
    return typeSeq;
}

void Sentence::addBlock(const std::string& text, WordType type) {
    blocks_.push_back({text, type});
}

void Sentence::insertBlockAtStart(const std::string& text, WordType type) {
    blocks_.insert(blocks_.begin(), {text, type});
}

void Sentence::insertNegation() {
    // Insert "no" as an adverb right after the first verb found.
    for (size_t i = 0; i < blocks_.size(); ++i) {
        if (blocks_[i].type == WordType::VERB) {
            blocks_.insert(blocks_.begin() + i + 1, {"no", WordType::ADVERB});
            break;
        }
    }
}

void Sentence::replaceNoun(const std::string& newWord) {
    for (auto& block : blocks_) {
        if (block.type == WordType::NOUN) {
            block.text = newWord;
            break;
        }
    }
}

std::string Sentence::toString() const {
    std::string result;
    for (const auto& block : blocks_) {
        if (!result.empty()) result += ' ';
        result += block.text;
    }
    return result;
}
