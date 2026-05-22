/**
 * @file types.hpp
 * @brief Shared enumeration types for the entire NLP engine.
 * @author Soubhi Khayat Najjar
 * @date 2026
 * @note Part of the Admin8.5.0 NLP engine.
 */

#pragma once

#include <cstdint>

// ============================================================================
// Word-related enums
// ============================================================================

/** @brief Part-of-speech / lexical category. */
enum class WordType : uint8_t {
    PRONOUN,        ///< Pronoun
    ARTICLE,        ///< Article
    ADJECTIVE,      ///< Adjective
    NOUN,           ///< Noun
    VERB,           ///< Verb
    INTERROGATIVE,  ///< Interrogative
    ADVERB,         ///< Adverb
    SENSORY,        ///< Sensory word (onomatopoeia, interjection)
    PREPOSITION,    ///< Preposition
    RELATIVE,       ///< Relative
    NUMERAL,        ///< Numeral
    CONJUNCTION,    ///< Conjunction
    CONTENT,        ///< Unclassified textual content
    QUANTIFIER,     ///< Quantifier
    DEMONSTRATIVE,  ///< Demonstrative
    DATE,           ///< Date / temporal expression
    EMAIL,
    MONEY,
    PHONE,
    UNDEFINED       ///< Unspecified or unknown type
};

/** @brief Grammatical number. */
enum class Quantity : uint8_t {
    SINGULAR,   ///< Singular
    PLURAL,     ///< Plural
    NONE        ///< Not applicable
};

/** @brief Verb tense. */
enum class Tense : uint8_t {
    PAST,           ///< Past
    PRESENT,        ///< Present
    FUTURE,         ///< Future
    UNDETERMINED    ///< Indeterminate / not applicable
};

/** @brief Grammatical gender. */
enum class Gender : uint8_t {
    MASCULINE,  ///< Masculine
    FEMININE,   ///< Feminine
    NEUTER      ///< Neuter / not applicable
};

/** @brief Degree (for adjectives, adverbs, etc.). */
enum class Degree : uint8_t {
    COMPARATIVE,    ///< Comparative degree
    SUPERLATIVE,    ///< Superlative degree
    POSITIVE,       ///< Positive degree
    INTENSIVE,      ///< Intensive (adverb)
    INTERROGATIVE,  ///< Interrogative degree
    NEGATIVE,       ///< Negative (adverb)
    RELATIVE,       ///< Relative
    QUANTITATIVE,   ///< Quantitative
    NONE            ///< Not applicable
};

/** @brief Grammatical person. */
enum class Person : uint8_t {
    FIRST,   ///< First person
    SECOND,  ///< Second person
    THIRD,   ///< Third person
    NONE     ///< Not applicable
};

// ============================================================================
// Pattern / Token types
// ============================================================================

/** @brief Pattern classification for sentence structures. */
enum class PatternType : uint8_t {
    SIMPLE_AFFIRMATIVE,
    COMPOUND_AFFIRMATIVE,
    SIMPLE_NEGATIVE,
    COMPOUND_NEGATIVE,
    SIMPLE_INTERROGATIVE,
    COMPOUND_INTERROGATIVE,
    MIXED,
    SENTENCES
};

/** @brief Token type produced by the lexical analyzer. */
enum class TokenType : uint8_t {
    WORD,
    NUMBER,
    DATE,
    EMAIL,
    MONEY,
    PHONE
};

// ============================================================================
// Command & state types
// ============================================================================

/** @brief Command types for the task/action system. */
enum class CommandType {
    DO, ANSWER, ASK, TASK, BID, GO, REPORT, MOVE, REPEAT, FIND,
    CREATE, DELETE, UPDATE, SEND, CALL, PLAY, PAUSE, STOP, START,
    SCHEDULE, SET, SHOW, HELP, OPEN, CLOSE, LEARN,
    EXTRACT, RETRIEVE, QUERY, FETCH
};

/** @brief Parsing state machine states. */
enum class State {
    START,
    NOMINAL,
    VERBAL,
    PREPOSITIONAL,
    CONJUNCTION
};
