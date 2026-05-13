/**
 * @file StringConversions.cpp
 * @brief Implementation of enumeration‑to‑string conversions.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "StringConversions.hpp"

std::string wordTypeToString(WordType type) {
    static const char* names[] = {
        "Pronoun",          // PRONOUN
        "Article",          // ARTICLE
        "Adjective",        // ADJECTIVE
        "Noun",             // NOUN
        "Verb",             // VERB
        "Interrogative",    // INTERROGATIVE
        "Adverb",           // ADVERB
        "Sensory",          // SENSORY
        "Preposition",      // PREPOSITION
        "Relative",         // RELATIVE
        "Numeral",          // NUMERAL
        "Conjunction",      // CONJUNCTION
        "Content",          // CONTENT
        "Quantifier",       // QUANTIFIER
        "Demonstrative",    // DEMONSTRATIVE
        "Date",             // DATE
        "Undefined"         // UNDEFINED
    };
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= static_cast<int>(sizeof(names)/sizeof(names[0]))) return "Unknown";
    return names[idx];
}

std::string tenseToString(Tense tense) {
    static const char* names[] = {
        "Past",             // PAST
        "Present",          // PRESENT
        "Future",           // FUTURE
        "Undetermined"      // UNDETERMINED
    };
    int idx = static_cast<int>(tense);
    if (idx < 0 || idx >= 4) return "Unknown";
    return names[idx];
}

std::string genderToString(Gender gender) {
    static const char* names[] = {
        "Masculine",        // MASCULINE
        "Feminine",         // FEMININE
        "Neuter"            // NEUTER
    };
    int idx = static_cast<int>(gender);
    if (idx < 0 || idx > 2) return "Unknown";
    return names[idx];
}

std::string personToString(Person person) {
    static const char* names[] = {
        "First",            // FIRST
        "Second",           // SECOND
        "Third",            // THIRD
        "None"              // NONE
    };
    int idx = static_cast<int>(person);
    if (idx < 0 || idx > 3) return "Unknown";
    return names[idx];
}

std::string degreeToString(Degree degree) {
    static const char* names[] = {
        "Comparative",      // COMPARATIVE
        "Superlative",      // SUPERLATIVE
        "Positive",         // POSITIVE
        "Intensive",        // INTENSIVE
        "Interrogative",    // INTERROGATIVE (degree)
        "Negative",         // NEGATIVE
        "Relative",         // RELATIVE
        "Quantitative",     // QUANTITATIVE
        "None"              // NONE
    };
    int idx = static_cast<int>(degree);
    if (idx < 0 || idx > 8) return "Unknown";
    return names[idx];
}

std::string quantityToString(Quantity quantity) {
    static const char* names[] = {
        "Singular",         // SINGULAR
        "Plural",           // PLURAL
        "None"              // NONE
    };
    int idx = static_cast<int>(quantity);
    if (idx < 0 || idx > 2) return "Unknown";
    return names[idx];
}

std::string patternTypeToString(PatternType type) {
    static const char* names[] = {
        "Simple Affirmative",       // SIMPLE_AFFIRMATIVE
        "Compound Affirmative",     // COMPOUND_AFFIRMATIVE
        "Simple Negative",          // SIMPLE_NEGATIVE
        "Compound Negative",        // COMPOUND_NEGATIVE
        "Simple Interrogative",     // SIMPLE_INTERROGATIVE
        "Compound Interrogative",   // COMPOUND_INTERROGATIVE
        "Mixed",                    // MIXED
        "Sentences"                 // SENTENCES
    };
    int idx = static_cast<int>(type);
    if (idx < 0 || idx > 7) return "Unknown";
    return names[idx];
}
