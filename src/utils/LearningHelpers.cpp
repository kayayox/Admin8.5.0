/**
 * @file LearningHelpers.cpp
 * @brief Implementation of learning helper utilities.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "LearningHelpers.hpp"
#include "../nlp/Tokenizer.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <ctime>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstdint>

// ============================================================================
// Internal helper for token -> Word creation
// ============================================================================
static void initWordFromToken(Word& word, const Token& token) {
    if (token.type != TokenType::WORD) {
        word.setType(token.type == TokenType::DATE ? WordType::DATE : WordType::NUMERAL);
        word.setConfidence(0.95f);
    }
}

// ============================================================================
// Public interface
// ============================================================================
bool createWordVector(std::vector<Word>& words, const std::string& input) {
    std::vector<Token> tokens = tokenize(input);
    if (tokens.empty()) return false;

    for (size_t i = 0; i < tokens.size(); ) {
        // Process up to 4 tokens at a time (identical to original but cleaner)
        size_t batchSize = std::min<size_t>(4, tokens.size() - i);
        for (size_t j = 0; j < batchSize; ++j) {
            Word w(tokens[i + j].text);
            initWordFromToken(w, tokens[i + j]);
            words.push_back(w);
        }
        i += batchSize;
    }
    return true;
}

std::vector<Word> createWordVector(const std::string& input) {
    std::vector<Word> words;
    createWordVector(words, input);
    return words;
}

void learnContextual(ContextualCorrelator& ctx, const std::string& text) {
    if (!text.empty()) ctx.learnWithPreviousTwo(text);
}

void learnDirect(ContextualCorrelator& ctx, const std::string& text) {
    if (!text.empty()) ctx.learnNextWordDirect(text);
}

void learnTextWithContext(ContextualCorrelator& ctx, PatternCorrelator& corr, const std::string& text) {
    if (text.empty()) return;
    corr.learnFromText(text, 1);
    ctx.learnWithPreviousTwo(text);
    ctx.learnNextWordDirect(text);
}

void learnLetterCorrelations(LetterCorrelator& lttCorr, const std::string& text) {
    if (text.empty()) return;
    std::vector<Token> toks = tokenize(text);
    for( auto t : toks){
        std::string word = t.text;
        word.push_back(' ');
        lttCorr.learnWithPreviousTwo(word);
        lttCorr.learnNextLetterDirect(word);
    }
}

void learnSyllableCorrelations(SyllableCorrelator& sllCorr, const std::string& text) {
    if (text.empty()) return;
    sllCorr.learnWithPreviousTwo(text);
    sllCorr.learnNextSyllableDirect(text);
}

void clearInputBuffer() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void trimString(std::string& line) {
    line.erase(line.begin(), std::find_if(line.begin(), line.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    line.erase(std::find_if(line.rbegin(), line.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
}

std::string formatTimestamp(uint32_t ts) {
    std::time_t t = ts;
    std::tm tm_buf;
    localtime_r(&t, &tm_buf);  // POSIX (Linux, macOS)
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buffer);
}

std::vector<int> getcomparetime(uint32_t ts) {
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm tm_buf;

    // localtime_r es POSIX (thread-safe). En Windows usar localtime_s.
    localtime_r(&t, &tm_buf);

    // Extraer los campos de fecha/hora
    int year  = tm_buf.tm_year + 1900;  // años desde 1900
    int month = tm_buf.tm_mon + 1;      // meses: 0-11 → 1-12
    int day   = tm_buf.tm_mday;
    int hour  = tm_buf.tm_hour;
    int min   = tm_buf.tm_min;
    int sec   = tm_buf.tm_sec;

    // Devolver vector con los 6 valores
    return {year, month, day, hour, min, sec};
}
