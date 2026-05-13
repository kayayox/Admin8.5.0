/**
 * @file Word.cpp
 * @brief Implementation of the Word entity.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Word.hpp"
#include <algorithm>
#include <sstream>
#include <ctime>

namespace {

/// Token used when no preceding context word exists.
const std::string kNoContextToken = "__NO_CONTEXT__";

// ============================================================================
// Descriptor tables for generateStructuredMeaning()
// ============================================================================

/**
 * @brief Helper structure to describe how a WordType maps to its textual parts.
 */
struct TypeDescriptor {
    WordType type;
    const char* baseLabel;          ///< e.g. "a noun"
    bool usesGender   = false;
    bool usesQuantity = false;
    bool usesTense    = false;
    bool usesDegree   = false;
    bool usesPerson   = false;
    const char* suffix = "";        ///< appended after all morphological details
};

constexpr TypeDescriptor kTypeDescriptors[] = {
    { WordType::NOUN,           "a noun",                                       true,  true,  false, false, false, " that designates an entity." },
    { WordType::VERB,           "a verb",                                       false, false, true,  false, true,  " that expresses an action, state, or process." },
    { WordType::ADJECTIVE,      "a qualifying adjective",                       false, false, false, true,  false, " that modifies a noun." },
    { WordType::ADVERB,         "an adverb",                                    false, false, false, true,  false, " that modifies a verb, adjective, or other adverb." },
    { WordType::PREPOSITION,    "a preposition that establishes dependency relationships between words." },
    { WordType::CONJUNCTION,    "a conjunction that joins clauses or terms." },
    { WordType::PRONOUN,        "a pronoun",                                    false, false, false, false, true,  " that substitutes a noun." },
    { WordType::ARTICLE,        "an article",                                   true,  true,  false, false, false, " that determines a noun." },
    { WordType::NUMERAL,        "a numeral that indicates quantity or order." },
    { WordType::DEMONSTRATIVE,  "a demonstrative that indicates relative distance." },
    { WordType::QUANTIFIER,     "a quantifier that expresses an imprecise amount." },
    { WordType::RELATIVE,       "a relative that introduces a subordinate clause." },
    { WordType::INTERROGATIVE,  "an interrogative used in direct or indirect questions." },
    { WordType::SENSORY,        "a sensory word (onomatopoeia or interjection)." },
    { WordType::CONTENT,        "textual content without specific classification." },
    { WordType::DATE,           "a date or temporal expression." },
    { WordType::UNDEFINED,      "a word of unspecified type." }
};

/// Gender labels  (index = Gender enum value)
constexpr const char* kGenderLabels[] = { " masculine", " feminine", "" }; // MASCULINE, FEMININE, NEUTER

/// Quantity labels
constexpr const char* kQuantityLabels[] = { " in singular", " in plural", "" }; // SINGULAR, PLURAL, NONE

/// Tense labels
constexpr const char* kTenseLabels[] = { " in past", " in present", " in future", "" }; // PAST, PRESENT, FUTURE, UNDETERMINED

/// Degree labels (generic)
constexpr const char* kDegreeLabels[] = { " in comparative degree", " in superlative degree", " in positive degree",
                                          " of intensity", " interrogative", " of negation", " relative", " quantitative", "" };

/// Person labels
constexpr const char* kPersonLabels[] = { " of first person", " of second person", " of third person", "" }; // FIRST, SECOND, THIRD, NONE

/**
 * @brief Appends gender and quantity details for nominal types (noun, article, etc.).
 */
void appendGenderQuantity(std::ostringstream& oss, Gender gender, Quantity quantity) {
    if (gender != Gender::NEUTER) {
        oss << kGenderLabels[static_cast<int>(gender)];
    }
    if (quantity != Quantity::NONE) {
        oss << kQuantityLabels[static_cast<int>(quantity)];
    }
}

/**
 * @brief Appends tense and person details for verbal types.
 */
void appendTensePerson(std::ostringstream& oss, Tense tense, Person person) {
    if (tense != Tense::UNDETERMINED) {
        oss << kTenseLabels[static_cast<int>(tense)];
    }
    if (person != Person::NONE) {
        oss << kPersonLabels[static_cast<int>(person)];
    }
}

/**
 * @brief Appends degree details for adjectives and adverbs.
 */
void appendDegree(std::ostringstream& oss, Degree degree) {
    if (degree != Degree::NONE) {
        oss << kDegreeLabels[static_cast<int>(degree)];
    }
}

} // anonymous namespace

// ============================================================================
// Constructor
// ============================================================================

Word::Word(const std::string& word) : word_(word) {
    timestamp_ = static_cast<uint32_t>(std::time(nullptr));
    frequency_ = 1;
}

// ============================================================================
// Setters
// ============================================================================

void Word::setMeaning(const std::string& meaning) {
    meaning_ = meaning;
}

void Word::setConfidence(float confidence) {
    confidence_ = confidence;
}

// ============================================================================
// Related words
// ============================================================================

void Word::addRelated(const std::string& relatedWord, double weight) {
    relatedWords_.emplace_back(relatedWord, weight);
}

// ============================================================================
// learnRelationsFromCorrelator
// ============================================================================

void Word::learnRelationsFromCorrelator(PatternCorrelator& correlator,
                                        const std::vector<std::string>& contextWords,
                                        double minConfidence) {
    // Build previous pattern with the word that precedes 'word_' in contextWords
    WordPattern prevPattern;
    std::string prevWord = kNoContextToken;

    auto it = std::find(contextWords.begin(), contextWords.end(), word_);
    if (it != contextWords.end() && it != contextWords.begin()) {
        prevWord = *(it - 1);
    }
    prevPattern[prevWord] = 1.0f;

    std::vector<std::pair<WordPattern, double>> outcomes;
    if (correlator.query(word_, prevPattern, outcomes)) {
        relatedWords_.clear();
        for (const auto& [pattern, confidence] : outcomes) {
            if (confidence >= minConfidence && !pattern.empty()) {
                // Each pattern is a map of one word with its probability.
                for (const auto& [relatedWord, weight] : pattern) {
                    if (relatedWord != word_) {
                        relatedWords_.emplace_back(relatedWord, confidence * weight);
                    }
                }
            }
        }
    }
}

// ============================================================================
// generateStructuredMeaning
// ============================================================================

void Word::generateStructuredMeaning() {
    std::ostringstream oss;
    oss << "The word \"" << word_ << "\" is ";

    // Find descriptor for the current type
    const TypeDescriptor* descriptor = nullptr;
    for (const auto& desc : kTypeDescriptors) {
        if (desc.type == type_) {
            descriptor = &desc;
            break;
        }
    }

    if (!descriptor) {
        // Fallback for unknown types (should not happen if enum is complete)
        oss << "a word of unspecified type.";
        meaning_ = oss.str();
        return;
    }

    // Handle special cases with custom behavior
    if (type_ == WordType::ADVERB) {
        oss << "an adverb";
        if (degree_ == Degree::INTENSIVE) {
            oss << " of intensity";
        } else if (degree_ == Degree::NEGATIVE) {
            oss << " of negation";
        } else if (degree_ == Degree::RELATIVE) {
            oss << " relative";
        } else if (degree_ != Degree::NONE) {
            oss << kDegreeLabels[static_cast<int>(degree_)];
        }
        oss << " that modifies a verb, adjective, or other adverb.";
    } else {
        oss << descriptor->baseLabel;

        if (descriptor->usesGender || descriptor->usesQuantity) {
            appendGenderQuantity(oss, gender_, quantity_);
        }
        if (descriptor->usesTense || descriptor->usesPerson) {
            appendTensePerson(oss, tense_, person_);
        }
        if (descriptor->usesDegree) {
            appendDegree(oss, degree_);
        }
        oss << descriptor->suffix;
    }

    // Append related words if present
    if (!relatedWords_.empty()) {
        oss << " Furthermore, it is related to: ";
        size_t count = 0;
        for (const auto& [relWord, w] : relatedWords_) {
            if (count++ > 0) oss << ", ";
            oss << relWord;
            if (count >= 5) {
                oss << ", etc.";
                break;
            }
        }
        oss << '.';
    }

    meaning_ = oss.str();
}
