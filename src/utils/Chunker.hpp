/**
 * @file Chunker.hpp
 * @brief Segmentation of word sequences into chunks (noun, verb, prepositional phrases, etc.)
 *        for pattern learning.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef CHUNKER_HPP
#define CHUNKER_HPP

#include "../core/Word.hpp"
#include <vector>
#include <string>

class Chunker {
public:
    /**
     * @brief Converts a sequence of classified words into a sequence of chunks.
     * @param words Classified words.
     * @return Vector of chunk strings.
     */
    static std::vector<std::string> chunk(const std::vector<Word>& words);
};

#endif // CHUNKER_HPP
