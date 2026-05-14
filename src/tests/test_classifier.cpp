/**
 * @file test_classifier.cpp
 * @brief Pruebas unitarias para Classifier (clasificación completa de oraciones).
 */

#include <gtest/gtest.h>
#include "../nlp/Classifier.hpp"
#include "../nlp/Morphology.hpp"
#include "../nlp/TagStats.hpp"
#include "../core/Word.hpp"
#include "../db/WordRepository.hpp"

// -----------------------------------------------------------------------------
// Fixture que configura bases de datos en memoria
// -----------------------------------------------------------------------------
class ClassifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configurar WordRepository en memoria (si existe el método)
        // WordRepository::setDatabasePath(":memory:");
        WordRepository::initializeTables();

        // Configurar TagStats en memoria
        TagStats::setDatabasePath(":memory:");
        TagStats::initializeTables();
        TagStats::setLanguage("es");
        morphology::setLanguage("es");

        TagStats::loadDefaultFromStatic();
    }
};

// -----------------------------------------------------------------------------
// Pruebas
// -----------------------------------------------------------------------------

TEST_F(ClassifierTest, SimpleSpanishSentence) {
    Classifier classifier;
    std::vector<Word> words;
    words.emplace_back("el");
    words.emplace_back("perro");
    words.emplace_back("corre");

    classifier.classifySentence(words);

    ASSERT_EQ(words.size(), 3);

    EXPECT_EQ(words[0].getType(), WordType::ARTICLE);
    EXPECT_GE(words[0].getConfidence(), 0.8f);

    EXPECT_EQ(words[1].getType(), WordType::NOUN);
    EXPECT_EQ(words[1].getGender(), Gender::MASCULINE);
    EXPECT_EQ(words[1].getQuantity(), Quantity::SINGULAR);
    EXPECT_GE(words[1].getConfidence(), 0.8f);

    EXPECT_EQ(words[2].getType(), WordType::VERB);
    EXPECT_EQ(words[2].getTense(), Tense::PRESENT);
    EXPECT_EQ(words[2].getPerson(), Person::THIRD);
    EXPECT_GE(words[2].getConfidence(), 0.8f);
}

TEST_F(ClassifierTest, WordWithLowInitialConfidenceGetsRefined) {
    Classifier classifier;
    std::vector<Word> words;
    words.emplace_back("El");
    words.emplace_back("banco");
    words.emplace_back("cierra");

    classifier.classifySentence(words);

    EXPECT_EQ(words[1].getType(), WordType::NOUN);
    EXPECT_GT(words[1].getConfidence(), 0.5f);
}

TEST_F(ClassifierTest, UnknownWordBecomesUndefined) {
    Classifier classifier;
    std::vector<Word> words;
    words.emplace_back("xyz");
    classifier.classifySentence(words);
    EXPECT_LT(words[0].getConfidence(), 0.3f);
}

TEST_F(ClassifierTest, UpdateConfidenceCorrect) {
    Classifier classifier;
    Word word("casa");
    word.setConfidence(0.5f);
    classifier.updateConfidence(word, true);
    EXPECT_GT(word.getConfidence(), 0.5f);
    EXPECT_LE(word.getConfidence(), 0.99f);
    classifier.updateConfidence(word, false);
    EXPECT_LE(word.getConfidence(), 0.52f);
    EXPECT_GE(word.getConfidence(), 0.1f);
}
