/**
 * @file StringConversions.hpp
 * @brief Conversion of enumeration types to human‑readable strings.
 *
 * Used for debugging, logging, and user interfaces. All strings are in English;
 * localisation can be added later if needed.
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_STRING_CONVERSIONS_HPP
#define ADMIN850_STRING_CONVERSIONS_HPP

#include "../common/types.hpp"
#include <string>

/** @brief Converts WordType to its English name (e.g. NOUN -> "Noun"). */
std::string wordTypeToString(WordType type);

/** @brief Converts Tense to its English name (e.g. PAST -> "Past"). */
std::string tenseToString(Tense tense);

/** @brief Converts Gender to its English name (e.g. MASCULINE -> "Masculine"). */
std::string genderToString(Gender gender);

/** @brief Converts Person to its English name (e.g. FIRST -> "First"). */
std::string personToString(Person person);

/** @brief Converts Degree to its English name (e.g. COMPARATIVE -> "Comparative"). */
std::string degreeToString(Degree degree);

/** @brief Converts Quantity to its English name (e.g. SINGULAR -> "Singular"). */
std::string quantityToString(Quantity quantity);

/** @brief Converts PatternType to its English name (e.g. SIMPLE_AFFIRMATIVE -> "Simple Affirmative"). */
std::string patternTypeToString(PatternType type);

#endif // ADMIN850_STRING_CONVERSIONS_HPP
