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

// ============================================================================
// Internal helper for token -> Word creation
// ============================================================================
static void initWordFromToken(Word& word, const Token& token) {
    if (token.type != TokenType::WORD) {
        word.setType(token.type == TokenType::DATE ? WordType::DATE : WordType::NUMERAL);
        word.setConfidence(0.95f);
    }
    WordRepository::load(token.text,word);
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
