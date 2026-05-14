/**
 * @file test_tagstats.cpp
 * @brief Pruebas unitarias para TagStats (estadísticas de n-gramas con SQLite).
 */

#include <gtest/gtest.h>
#include "../nlp/TagStats.hpp"
#include "../db/DatabaseManager.hpp"

class TagStatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Usar base de datos en memoria para aislamiento
        TagStats::setDatabasePath(":memory:");
        TagStats::initializeTables();
        TagStats::setLanguage("es");
    }
};

// -----------------------------------------------------------------------------
// Actualización y consulta de unigramas
// -----------------------------------------------------------------------------

TEST_F(TagStatsTest, UpdateAndRetrieveUnigram) {
    TagStats::updateUnigram(WordType::ARTICLE, WordType::NOUN, 10);
    TagStats::updateUnigram(WordType::ARTICLE, WordType::ADJECTIVE, 5);

    auto probs = TagStats::getUnigramProbs(WordType::ARTICLE);
    ASSERT_GE(probs.size(), 2);

    float nounProb = 0.0f, adjProb = 0.0f;
    for (const auto& p : probs) {
        if (p.first == WordType::NOUN) nounProb = p.second;
        if (p.first == WordType::ADJECTIVE) adjProb = p.second;
    }
    EXPECT_GT(nounProb, adjProb);
    EXPECT_NEAR(nounProb, 10.0f / (10+5+ 0.001f*10), 0.01f); // con smoothing
}

TEST_F(TagStatsTest, UnigramSmoothing) {
    // Sin datos previos, debe devolver probabilidades suavizadas > 0
    auto probs = TagStats::getUnigramProbs(WordType::VERB);
    EXPECT_FALSE(probs.empty());
    for (const auto& p : probs) {
        EXPECT_GT(p.second, 0.0f);
    }
}

// -----------------------------------------------------------------------------
// Bigramas y trigramas
// -----------------------------------------------------------------------------

TEST_F(TagStatsTest, UpdateAndRetrieveBigram) {
    TagStats::updateBigram(WordType::PREPOSITION, WordType::ARTICLE, WordType::NOUN, 20);
    auto probs = TagStats::getBigramProbs(WordType::PREPOSITION, WordType::ARTICLE);
    bool found = false;
    for (const auto& p : probs) {
        if (p.first == WordType::NOUN) {
            EXPECT_GT(p.second, 0.0f);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(TagStatsTest, UpdateAndRetrieveTrigram) {
    TagStats::updateTrigram(WordType::DEMONSTRATIVE, WordType::ADJECTIVE, WordType::NOUN, 30);
    auto probs = TagStats::getTrigramProbs(WordType::DEMONSTRATIVE, WordType::NOUN);
    bool found = false;
    for (const auto& p : probs) {
        if (p.first == WordType::ADJECTIVE) {
            EXPECT_GT(p.second, 0.0f);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// -----------------------------------------------------------------------------
// Cambio de idioma
// -----------------------------------------------------------------------------

TEST_F(TagStatsTest, LanguageSeparation) {
    TagStats::setLanguage("es");
    TagStats::updateUnigram(WordType::ARTICLE, WordType::NOUN, 100);

    TagStats::setLanguage("en");
    auto probs_en = TagStats::getUnigramProbs(WordType::ARTICLE);
    bool found_es_data = false;
    for (const auto& p : probs_en) {
        if (p.first == WordType::NOUN && p.second > 0.5f) {
            found_es_data = true;
            break;
        }
    }
    EXPECT_FALSE(found_es_data) << "Los datos en español no deben aparecer en inglés";
}

// -----------------------------------------------------------------------------
// Carga de datos por defecto
// -----------------------------------------------------------------------------

TEST_F(TagStatsTest, LoadDefaultFromStatic) {
    // Limpiar (no hay método público, pero podemos recargar en BD vacía)
    // Como ya tenemos una BD limpia, loadDefault debe insertar datos.
    TagStats::loadDefaultFromStatic();

    // Verificar que existen algunas entradas para español
    auto probs = TagStats::getUnigramProbs(WordType::ARTICLE);
    bool hasNoun = false, hasAdjective = false;
    for (const auto& p : probs) {
        if (p.first == WordType::NOUN) hasNoun = true;
        if (p.first == WordType::ADJECTIVE) hasAdjective = true;
    }
    EXPECT_TRUE(hasNoun);
    EXPECT_TRUE(hasAdjective);
}
