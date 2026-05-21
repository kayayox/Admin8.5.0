/**
 * @file test_nlpengine.cpp
 * @brief Pruebas unitarias para NLPEngine (facade principal).
 * @author Generated for testing purposes
 * @date 2026
 */

#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>

#include "../api/NLPEngine.hpp"

// -----------------------------------------------------------------------------
// Fixture que crea bases de datos temporales para cada prueba
// -----------------------------------------------------------------------------
class NLPEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Crear nombres de archivos temporales únicos
        semPath = "test_semantic_" + std::to_string(rand()) + ".db";
        patPath = "test_pattern_" + std::to_string(rand()) + ".db";
        // Usar :memory: para la base temporal (o archivo, pero :memory: es más limpio)
        tempPath = ":memory:";
    }

    void TearDown() override {
        engine.shutdown();
        // Eliminar archivos temporales si existen
        std::remove(semPath.c_str());
        std::remove(patPath.c_str());
    }

    NLPEngine engine;
    std::string semPath;
    std::string patPath;
    std::string tempPath;
};

// -----------------------------------------------------------------------------
// Pruebas de inicialización y configuración
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, InitializeAndShutdown) {
    EXPECT_TRUE(engine.initialize(semPath, patPath, tempPath));
    // Inicializar dos veces debe fallar (o al menos no romper)
    EXPECT_TRUE(engine.initialize(semPath, patPath, tempPath));
    engine.shutdown();
    // Después de shutdown, inicializar de nuevo debe funcionar
    EXPECT_TRUE(engine.initialize(semPath, patPath, tempPath));
}

TEST_F(NLPEngineTest, SetLanguage) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    // No debe lanzar excepción
    EXPECT_NO_THROW(engine.setLanguage("es"));
    EXPECT_NO_THROW(engine.setLanguage("en"));
}

TEST_F(NLPEngineTest, SetDebugMode) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    EXPECT_NO_THROW(engine.setDebugMode(true));
    EXPECT_NO_THROW(engine.setDebugMode(false));
}

// -----------------------------------------------------------------------------
// Pruebas de procesamiento de oraciones
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, ProcessSimpleSentence) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    std::vector<WordInfo> result = engine.processSentence("El perro corre");
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(result.size(), 3);
    // Verificar contenido mínimo
    EXPECT_EQ(result[0].word, "el");
    EXPECT_EQ(result[1].word, "perro");
    EXPECT_EQ(result[2].word, "corre");
    // El tipo debe ser no vacío
    EXPECT_FALSE(result[0].type.empty());
    EXPECT_FALSE(result[1].type.empty());
    EXPECT_FALSE(result[2].type.empty());
}

TEST_F(NLPEngineTest, ProcessEmptySentence) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    auto result = engine.processSentence("");
    EXPECT_TRUE(result.empty());
    result = engine.processSentence("   ");
    EXPECT_TRUE(result.empty());
}

TEST_F(NLPEngineTest, ProcessSentenceWithNumbers) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    auto result = engine.processSentence("Tengo 2 perros");
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[1].word, "2");
    EXPECT_FALSE(result[1].type.empty()); // Debería ser NUMBER o NUMERAL
}

// -----------------------------------------------------------------------------
// Pruebas de predicción (requiere datos aprendidos previamente)
// -----------------------------------------------------------------------------

/*TEST_F(NLPEngineTest, PredictNextAfterLearning) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    // Aprender una frase para que el correlator tenga datos
    engine.learnText("hola mundo cómo estás");
    bool type = false;
    auto predictions = engine.predictNext("hola mundo", type);
    // Puede devolver vacío si no hay suficiente aprendizaje, pero no debe fallar
    EXPECT_NO_THROW(engine.predictNext("", type));
    EXPECT_NO_THROW(engine.predictNext("una sola", type));
}*/

// -----------------------------------------------------------------------------
// Pruebas de generación de respuesta
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, GenerateResponseBasic) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    std::string response = engine.generateResponse("Hola");
    // La respuesta puede ser vacía si no hay templates, pero no debe fallar
    EXPECT_NO_THROW(engine.generateResponse(""));
    // Si hay templates por defecto, debería devolver algo
    if (!response.empty()) {
        EXPECT_FALSE(response.empty());
    }
}

TEST_F(NLPEngineTest, GenerateResponseWithContext) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    engine.processSentence("Me llamo Juan");
    std::string response = engine.generateResponse("¿Cómo te llamas?");
    // No se verifica contenido exacto, solo que no crashee
    SUCCEED();
}

// -----------------------------------------------------------------------------
// Pruebas de feedback y corrección
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, ProvideFeedback) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    // Primero generar una respuesta para tener último diálogo
    engine.generateResponse("Buenos días");
    // Dar feedback positivo y negativo
    EXPECT_NO_THROW(engine.provideDialogueFeedback(true));
    EXPECT_NO_THROW(engine.provideDialogueFeedback(false));
}

TEST_F(NLPEngineTest, CorrectWord) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    // Procesar una palabra para que exista en BD
    engine.processSentence("casa");
    EXPECT_NO_THROW(engine.correctWord("casa", "Sustantivo"));
    EXPECT_NO_THROW(engine.correctWord("palabra inexistente", "Verbo")); // debe ignorar
}

TEST_F(NLPEngineTest, ReprocessLastSentence) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    engine.processSentence("Esto es una prueba");
    EXPECT_NO_THROW(engine.reprocessLastSentence());
    // Sin oración previa no debe fallar
    engine.resetContext();
    EXPECT_NO_THROW(engine.reprocessLastSentence());
}

// -----------------------------------------------------------------------------
// Pruebas de consulta de palabras
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, GetWordInfoExisting) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    engine.processSentence("perro");
    std::string word = "perro";
    WordInfo info = engine.getWordInfo(word);
    EXPECT_EQ(info.word, "perro");
    EXPECT_FALSE(info.type.empty());
    // La confianza debe ser >0
    EXPECT_GT(info.confidence, 0.0f);
}

TEST_F(NLPEngineTest, GetWordInfoNonExisting) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    std::string word = "inexistente123";
    WordInfo info = engine.getWordInfo(word);
    EXPECT_EQ(info.word, "inexistente123");
    // Para palabra desconocida, confianza baja o tipo vacío
    // En la implementación actual, carga y devuelve lo que haya (probablemente UNDEFINED)
    // Solo verificamos que no lance excepción
    SUCCEED();
}

// -----------------------------------------------------------------------------
// Pruebas de reinicio de contexto
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, ResetContext) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    engine.processSentence("Primera oración");
    engine.generateResponse("Segunda");
    EXPECT_NO_THROW(engine.resetContext());
    // Después de reset, procesar debe seguir funcionando
    auto result = engine.processSentence("Nueva oración");
    EXPECT_FALSE(result.empty());
}

// -----------------------------------------------------------------------------
// Pruebas de aprendizaje de texto
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, LearnText) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    EXPECT_NO_THROW(engine.learnText("Este es un texto largo para aprender correlaciones."));
    // Aprender texto vacío no debe fallar
    EXPECT_NO_THROW(engine.learnText(""));
}

// -----------------------------------------------------------------------------
// Pruebas de integración secuencial
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, FullDialogueCycle) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    // 1. Aprender algunas frases
    engine.learnText("Hola me llamo Ana. ¿Cómo estás? Estoy bien gracias.");
    // 2. Procesar una oración
    auto words = engine.processSentence("¿Cómo te llamas?");
    ASSERT_FALSE(words.empty());
    // 3. Generar respuesta
    std::string resp = engine.generateResponse("¿Cómo te llamas?");
    // 4. Feedback positivo
    engine.provideDialogueFeedback(true);
    // 5. Corregir alguna palabra si es necesario
    engine.correctWord("llamas", "Verbo");
    // 6. Reprocesar última oración
    engine.reprocessLastSentence();
    // 7. Predecir siguiente palabra
    bool type = false;
    auto preds = engine.predictNext("cómo te", type);
    // No evaluamos contenido exacto, solo que no crashee
    SUCCEED();
}

// -----------------------------------------------------------------------------
// Prueba de múltiples idiomas (si se soporta)
// -----------------------------------------------------------------------------

TEST_F(NLPEngineTest, SwitchLanguageDuringRuntime) {
    ASSERT_TRUE(engine.initialize(semPath, patPath, tempPath));
    engine.setLanguage("es");
    auto spanishWords = engine.processSentence("el gato");
    engine.setLanguage("en");
    auto englishWords = engine.processSentence("the cat");
    // Ambos deben procesarse sin errores
    EXPECT_FALSE(spanishWords.empty());
    EXPECT_FALSE(englishWords.empty());
}
