/**
 * @file Classifier.hpp
 * @brief Word classifier that combines morphology and context to assign
 *        part-of-speech tags and grammatical attributes to a sentence.
 * @author Soubhi Khayat Najjar
 * @date 2026
 *
 * @note The classifier modifies Word objects in place. It uses morphological
 *       rules, contextual refinement, and updates transition statistics when
 *       the whole sentence reaches high confidence.
 */

#ifndef ADMIN850_CLASSIFIER_HPP
#define ADMIN850_CLASSIFIER_HPP

#include "../core/Word.hpp"
#include <vector>

/**
 * @class Classifier
 * @brief Encapsulates the classification pipeline for a sequence of words.
 */
class Classifier {
public:
    Classifier();

    /**
     * @brief Classifies all words in a sentence, assigning types and attributes.
     * @param words The sentence as a mutable vector of Word objects.
     */
    void classifySentence(std::vector<Word>& words);

    /**
     * @brief Adjusts a word's confidence based on external feedback.
     * @param word The word to update.
     * @param wasCorrect True if the previous tag was correct, false otherwise.
     */
    void updateConfidence(Word& word, bool wasCorrect);

private:
    /**
     * @brief Updates the morphological attributes of a word after tagging.
     * @param word The word to modify.
     * @param tag The assigned WordType.
     */
    void updateMorphAttributes(Word& word, WordType tag);
};

#endif // ADMIN850_CLASSIFIER_HPP
