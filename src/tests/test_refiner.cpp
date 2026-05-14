/**
 * @file test_refiner.cpp
 * @brief Pruebas unitarias para Refiner.
 */

#include <gtest/gtest.h>
#include "../nlp/Refiner.hpp"
#include "../nlp/TagStats.hpp"

class RefinerTest : public ::testing::Test {
protected:
    void SetUp() override {
        TagStats::setDatabasePath(":memory:");
        TagStats::initializeTables();
        TagStats::setLanguage("es");
        // Insertar datos controlados
        TagStats::updateTrigram(WordType::NOUN, WordType::VERB, WordType::ADVERB, 50);
        TagStats::updateTrigram(WordType::NOUN, WordType::VERB, WordType::NOUN, 10);
        TagStats::updateBigram(WordType::ARTICLE, WordType::NOUN, WordType::VERB, 30);
        TagStats::updateUnigram(WordType::PREPOSITION, WordType::NOUN, 20);
    }
};

TEST_F(RefinerTest, HighConfidenceKept) {
    auto result = refineTag(WordType::UNDEFINED, WordType::UNDEFINED,
                            WordType::VERB, WordType::UNDEFINED, 0.9f);
    EXPECT_EQ(result.tag, WordType::VERB);
    EXPECT_FLOAT_EQ(result.confidence, 0.9f);
}

TEST_F(RefinerTest, FallbackToBigramWhenTrigramEmpty) {
    auto result = refineTag(WordType::ARTICLE, WordType::NOUN,
                            WordType::UNDEFINED, WordType::ADJECTIVE, 0.0f);
    EXPECT_EQ(result.tag, WordType::VERB);
}

TEST_F(RefinerTest, FallbackToUnigram) {
    auto result = refineTag(WordType::UNDEFINED, WordType::PREPOSITION,
                            WordType::UNDEFINED, WordType::UNDEFINED, 0.0f);
    EXPECT_EQ(result.tag, WordType::NOUN);
}
