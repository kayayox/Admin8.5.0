/**
 * @file Chunker.cpp
 * @brief Implementation of grammatical chunking.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Chunker.hpp"
#include "../common/types.hpp"
#include <sstream>

namespace {

    bool isNominal(WordType t) {
        return t == WordType::ARTICLE || t == WordType::NOUN || t == WordType::ADJECTIVE ||
               t == WordType::NUMERAL  || t == WordType::PRONOUN || t == WordType::DEMONSTRATIVE ||
               t == WordType::QUANTIFIER || t == WordType::RELATIVE;
    }

    bool isVerbal(WordType t) {
        return t == WordType::VERB || t == WordType::ADVERB;
    }

    bool isPreposition(WordType t) {
        return t == WordType::PREPOSITION;
    }

    bool isConjunction(WordType t) {
        return t == WordType::CONJUNCTION;
    }

} // anonymous namespace

std::vector<std::string> Chunker::chunk(const std::vector<Word>& words) {
    if (words.empty()) return {};

    std::vector<std::string> chunks;
    State state = State::START;
    std::string currentChunk;

    for (size_t i = 0; i < words.size(); ++i) {
        const Word& w = words[i];
        const std::string& token = w.getWord();
        WordType tag = w.getType();

        State newState = state;
        bool finalizeChunk = false;

        switch (state) {
            case State::START:
                if (isNominal(tag)) newState = State::NOMINAL;
                else if (isVerbal(tag)) newState = State::VERBAL;
                else if (isPreposition(tag)) newState = State::PREPOSITIONAL;
                else if (isConjunction(tag)) newState = State::CONJUNCTION;
                break;

            case State::NOMINAL:
                if (isVerbal(tag) || isPreposition(tag) || isConjunction(tag)) {
                    finalizeChunk = true;
                    if (isVerbal(tag)) newState = State::VERBAL;
                    else if (isPreposition(tag)) newState = State::PREPOSITIONAL;
                    else if (isConjunction(tag)) newState = State::CONJUNCTION;
                } else if (isNominal(tag)) {
                    newState = State::NOMINAL; // continue accumulating
                } else {
                    newState = State::NOMINAL; // fallback for unhandled tags
                }
                break;

            case State::VERBAL:
                if (isPreposition(tag) || isConjunction(tag) || (isNominal(tag) && tag != WordType::PRONOUN)) {
                    finalizeChunk = true;
                    if (isPreposition(tag)) newState = State::PREPOSITIONAL;
                    else if (isConjunction(tag)) newState = State::CONJUNCTION;
                    else if (isNominal(tag)) newState = State::NOMINAL;
                } else if (isVerbal(tag) || tag == WordType::PRONOUN) {
                    newState = State::VERBAL; // clitic pronouns stick to verb
                } else {
                    newState = State::VERBAL;
                }
                break;

            case State::PREPOSITIONAL:
                if (isVerbal(tag) && tag == WordType::VERB) {
                    finalizeChunk = true;
                    newState = State::VERBAL;
                } else {
                    newState = State::PREPOSITIONAL; // keep accumulating
                }
                break;

            case State::CONJUNCTION:
                finalizeChunk = true;
                if (isNominal(tag)) newState = State::NOMINAL;
                else if (isVerbal(tag)) newState = State::VERBAL;
                else if (isPreposition(tag)) newState = State::PREPOSITIONAL;
                else newState = State::NOMINAL;
                break;
        }

        if (finalizeChunk && !currentChunk.empty()) {
            chunks.push_back(currentChunk);
            currentChunk.clear();
        }

        if (currentChunk.empty()) currentChunk = token;
        else currentChunk += " " + token;

        state = newState;
    }

    if (!currentChunk.empty()) chunks.push_back(currentChunk);

    return chunks;
}
