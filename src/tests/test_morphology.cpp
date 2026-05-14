/**
 * @file test_morphology.cpp
 * @brief Pruebas unitarias para Morphology (análisis morfológico, español/inglés).
 */

#include <gtest/gtest.h>
#include "../nlp/Morphology.hpp"

class MorphologyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Por defecto español, cada prueba puede cambiarlo
        morphology::setLanguage("es");
    }
};

// -----------------------------------------------------------------------------
// Español
// -----------------------------------------------------------------------------

TEST_F(MorphologyTest, SpanishGender) {
    EXPECT_EQ(morphology::detectGender("casa"), Gender::FEMININE);
    EXPECT_EQ(morphology::detectGender("perro"), Gender::MASCULINE);
    EXPECT_EQ(morphology::detectGender("mapa"), Gender::MASCULINE);   // excepción
    EXPECT_EQ(morphology::detectGender("mano"), Gender::FEMININE);   // excepción
}

TEST_F(MorphologyTest, SpanishPlural) {
    EXPECT_TRUE(morphology::isPlural("casas"));
    EXPECT_FALSE(morphology::isPlural("casa"));
    EXPECT_FALSE(morphology::isPlural("lunes"));   // excepción
    EXPECT_FALSE(morphology::isPlural("crisis"));  // excepción
}

TEST_F(MorphologyTest, SpanishTense) {
    EXPECT_EQ(morphology::detectTense("caminó"), Tense::PAST);
    EXPECT_EQ(morphology::detectTense("cantará"), Tense::FUTURE);
    EXPECT_EQ(morphology::detectTense("hablo"), Tense::PRESENT);
}

TEST_F(MorphologyTest, SpanishPerson) {
    EXPECT_EQ(morphology::detectPerson("hablo"), Person::FIRST);
    EXPECT_EQ(morphology::detectPerson("hablas"), Person::SECOND);
    EXPECT_EQ(morphology::detectPerson("habla"), Person::THIRD);
    EXPECT_EQ(morphology::detectPerson("hablamos"), Person::FIRST);
    EXPECT_EQ(morphology::detectPerson("hablan"), Person::THIRD);
}

TEST_F(MorphologyTest, SpanishAdjectiveDegree) {
    EXPECT_EQ(morphology::detectAdjectiveDegree("bonito"), Degree::POSITIVE);
    EXPECT_EQ(morphology::detectAdjectiveDegree("bonitísimo"), Degree::SUPERLATIVE);
    EXPECT_EQ(morphology::detectAdjectiveDegree("mejor"), Degree::COMPARATIVE);
}

TEST_F(MorphologyTest, SpanishCommonWord) {
    WordType tag;
    float conf;
    EXPECT_TRUE(morphology::isCommonWord("casa", tag, conf));
    EXPECT_EQ(tag, WordType::NOUN);
    EXPECT_GE(conf, 0.9f);

    EXPECT_TRUE(morphology::isCommonWord("el", tag, conf));
    EXPECT_EQ(tag, WordType::ARTICLE);
}

TEST_F(MorphologyTest, SpanishGuessInitialTag) {
    EXPECT_EQ(morphology::guessInitialTag("corriendo"), WordType::VERB);
    EXPECT_EQ(morphology::guessInitialTag("rapidamente"), WordType::ADVERB);
    EXPECT_EQ(morphology::guessInitialTag("bonito"), WordType::ADJECTIVE);
    EXPECT_EQ(morphology::guessInitialTag("casa"), WordType::NOUN);
    EXPECT_EQ(morphology::guessInitialTag("xyz123"), WordType::UNDEFINED);
}

// -----------------------------------------------------------------------------
// Inglés
// -----------------------------------------------------------------------------

TEST_F(MorphologyTest, EnglishGender) {
    morphology::setLanguage("en");
    EXPECT_EQ(morphology::detectGender("house"), Gender::NEUTER);
    EXPECT_EQ(morphology::detectGender("man"), Gender::NEUTER);
}

TEST_F(MorphologyTest, EnglishPlural) {
    morphology::setLanguage("en");
    EXPECT_TRUE(morphology::isPlural("cars"));
    EXPECT_FALSE(morphology::isPlural("car"));
}

TEST_F(MorphologyTest, EnglishTense) {
    morphology::setLanguage("en");
    EXPECT_EQ(morphology::detectTense("walked"), Tense::PAST);
    EXPECT_EQ(morphology::detectTense("walk"), Tense::PRESENT);
    EXPECT_EQ(morphology::detectTense("will walk"), Tense::PRESENT); // no detectable
}

TEST_F(MorphologyTest, EnglishPerson) {
    morphology::setLanguage("en");
    EXPECT_EQ(morphology::detectPerson("runs"), Person::THIRD);
    EXPECT_EQ(morphology::detectPerson("run"), Person::NONE);
}

TEST_F(MorphologyTest, EnglishAdjectiveDegree) {
    morphology::setLanguage("en");
    EXPECT_EQ(morphology::detectAdjectiveDegree("fast"), Degree::POSITIVE);
    EXPECT_EQ(morphology::detectAdjectiveDegree("faster"), Degree::COMPARATIVE);
    EXPECT_EQ(morphology::detectAdjectiveDegree("fastest"), Degree::SUPERLATIVE);
}

TEST_F(MorphologyTest, EnglishCommonWord) {
    morphology::setLanguage("en");
    WordType tag;
    float conf;
    EXPECT_TRUE(morphology::isCommonWord("house", tag, conf));
    EXPECT_EQ(tag, WordType::NOUN);
    EXPECT_GE(conf, 0.9f);

    EXPECT_TRUE(morphology::isCommonWord("the", tag, conf));
    EXPECT_EQ(tag, WordType::ARTICLE);
}

TEST_F(MorphologyTest, EnglishGuessInitialTag) {
    morphology::setLanguage("en");
    EXPECT_EQ(morphology::guessInitialTag("quickly"), WordType::ADVERB);
    EXPECT_EQ(morphology::guessInitialTag("running"), WordType::VERB);
    EXPECT_EQ(morphology::guessInitialTag("beautiful"), WordType::ADJECTIVE);
    EXPECT_EQ(morphology::guessInitialTag("car"), WordType::NOUN);
}
