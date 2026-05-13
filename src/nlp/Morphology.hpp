/**
 * @file Morphology.hpp
 * @brief Language-aware morphological analysis: gender, number, tense,
 *        person, degree, and initial tag guessing via suffix and dictionary.
 *
 * Supports Spanish and English; the active language is set via setLanguage().
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_MORPHOLOGY_HPP
#define ADMIN850_MORPHOLOGY_HPP

#include "../common/types.hpp"
#include <string>

namespace morphology {

    // --- Language configuration ---

    /// Set the current processing language (e.g. "es", "en"). Default is "es".
    void setLanguage(const std::string& lang);
    /// Returns the current language code.
    std::string getLanguage();

    // --- Morphological properties ---

    bool   isPlural(const std::string& word);
    Gender detectGender(const std::string& word);
    Tense  detectTense(const std::string& word);
    Person detectPerson(const std::string& word);
    Degree detectAdjectiveDegree(const std::string& word);

    // --- Tag suggestion & validation ---

    WordType guessInitialTag(const std::string& word);
    float    getSuffixProb(const std::string& word, WordType tag);
    float    validateTag(const std::string& word, WordType tag);

    // --- Dictionary lookup ---
    bool     isCommonWord(const std::string& word, WordType& outTag, float& outConf);

    // --- Closed‑class word detectors ---
    bool isArticle(const std::string& word);
    bool isPreposition(const std::string& word);
    bool isConjunction(const std::string& word);
    bool isInterrogative(const std::string& word);
    bool isDemonstrative(const std::string& word);
    bool isNumeral(const std::string& word);
    bool isRelativePronoun(const std::string& word);
    bool isQuantifier(const std::string& word);

    // --- Utility ---
    bool endsWith(const std::string& word, const std::string& suffix);

} // namespace morphology

#endif // ADMIN850_MORPHOLOGY_HPP
