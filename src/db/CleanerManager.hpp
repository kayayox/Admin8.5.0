#ifndef CLEANERMANAGER_HPP_INCLUDED
#define CLEANERMANAGER_HPP_INCLUDED

#include "WordRepository.hpp"
#include "SentenceRepository.hpp"
#include "PatternRepository.hpp"
#include "DialogueRepository.hpp"
#include "TemplateRepository.hpp"
#include <cstdint>

struct Config {
    float minRelevant;      // Peso mínimo para correlaciones (PatternCorrelator)
    float minFrecuency;     // Frecuencia mínima para conservar elementos
    uint32_t maxTimeago;    // Segundos máximos desde last_used (0 = sin límite)
};

class Cleaner {
public:
    static void setDatabasePaths(const std::string& semanticPath,
                                 const std::string& patternPath,
                                 const std::string& templatePath); // opcional: misma semanticPath

    static void setConfig(const Config& cfg);

    // Limpieza completa de todas las bases
    static void cleanAll();

    // Limpieza específica por base
    static void cleanSemanticDatabase();
    static void cleanPatternDatabase();

private:
    static std::string semanticDbPath_;
    static std::string patternDbPath_;
    static Config config_;

    // Helpers internos
    static void cleanWords();
    static void cleanSentences();
    static void cleanDialogues();
    static void cleanTemplates();
    static void cleanPatterns();
    static void cleanPatternCorrelations();
};

#endif // CLEANERMANAGER_HPP_INCLUDED
