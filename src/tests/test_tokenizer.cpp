/**
 * @file test_tokenizer.cpp
 * @brief Pruebas unitarias para Tokenizer (tokenización y segmentación de oraciones).
 */

#include <gtest/gtest.h>
#include "../nlp/Tokenizer.hpp"
#include "../utils/StringUtils.hpp"

// -----------------------------------------------------------------------------
// Pruebas de tokenización (tokenize)
// -----------------------------------------------------------------------------

TEST(TokenizerTest, TokenizeSimpleWords) {
    auto tokens = tokenize("Hola mundo");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].text, "hola");
    EXPECT_EQ(tokens[0].type, TokenType::WORD);
    EXPECT_EQ(tokens[1].text, "mundo");
    EXPECT_EQ(tokens[1].type, TokenType::WORD);
}

TEST(TokenizerTest, TokenizeNumbers) {
    auto tokens = tokenize("El número 123 y 45.67");
    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[2].text, "123");
    EXPECT_EQ(tokens[2].type, TokenType::NUMBER);
    EXPECT_EQ(tokens[3].text, "y");
    EXPECT_EQ(tokens[3].type, TokenType::WORD);
}

TEST(TokenizerTest, TokenizeDates) {
    auto tokens = tokenize("Hoy es 2026-05-14 y mañana 14/05/2026");
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[2].type, TokenType::DATE);
    EXPECT_EQ(tokens[5].type, TokenType::DATE);
}

TEST(TokenizerTest, TokenizePunctuationAround) {
    auto tokens = tokenize("¡Hola! ¿Cómo estás?");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].text, "hola");
    EXPECT_EQ(tokens[1].text, "cómo");
    EXPECT_EQ(tokens[2].text, "estás");
}

TEST(TokenizerTest, TokenizeApostropheAndHyphen) {
    auto tokens = tokenize("don't bienestar");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].text, "don't");
    EXPECT_EQ(tokens[1].text, "bienestar");
}

TEST(TokenizerTest, LowercaseUtf8) {
    auto tokens = tokenize("Árbol NIÑO ÉL");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].text, "árbol");
    EXPECT_EQ(tokens[1].text, "niño");
    EXPECT_EQ(tokens[2].text, "él");
}

TEST(TokenizerTest, EmptyInput) {
    auto tokens = tokenize("");
    EXPECT_TRUE(tokens.empty());
}

// -----------------------------------------------------------------------------
// Pruebas de segmentación de oraciones (splitIntoSentences)
// -----------------------------------------------------------------------------

TEST(SentenceSplitTest, BasicPeriodSplit) {
    auto sentences = splitIntoSentences("Hola. Mundo.");
    ASSERT_EQ(sentences.size(), 2);
    EXPECT_EQ(sentences[0], "Hola.");
    EXPECT_EQ(sentences[1], "Mundo.");
}

TEST(SentenceSplitTest, AbbreviationNoSplit) {
    auto sentences = splitIntoSentences("El Dr. Pérez llegó. Él saludo.");
    // No debe cortar después de "Dr."
    ASSERT_EQ(sentences.size(), 1);
    EXPECT_EQ(sentences[0], "El Dr. Pérez llegó. Él saludo.");
}

TEST(SentenceSplitTest, QuestionExclamation) {
    auto sentences = splitIntoSentences("¿Cómo estás? Bien.");
    ASSERT_EQ(sentences.size(), 2);
    EXPECT_EQ(sentences[0], "¿Cómo estás?");
    EXPECT_EQ(sentences[1], "Bien.");
}

TEST(SentenceSplitTest, NewlineAsSentenceEnd) {
    auto sentences = splitIntoSentences("Primera línea.\nSegunda línea.");
    ASSERT_EQ(sentences.size(), 2);
    std::string s0 = sentences[0];
    s0.erase(std::remove(s0.begin(), s0.end(), '\n'), s0.end());
    EXPECT_EQ(s0, "Primera línea.");
    std::string s1 = sentences[1];
    s1.erase(std::remove(s1.begin(), s1.end(), '\n'), s1.end());
    EXPECT_EQ(s1, "Segunda línea.");
}

TEST(SentenceSplitTest, MultipleSpacesAfterPeriod) {
    auto sentences = splitIntoSentences("Fin.   Siguiente");
    ASSERT_EQ(sentences.size(), 2);
    EXPECT_EQ(sentences[0], "Fin.");
    EXPECT_EQ(sentences[1], "Siguiente");
}

TEST(SentenceSplitTest, NoPeriodAtEnd) {
    auto sentences = splitIntoSentences("Esta oración no tiene punto final");
    ASSERT_EQ(sentences.size(), 1);
    EXPECT_EQ(sentences[0], "Esta oración no tiene punto final");
}
