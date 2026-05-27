// KeyValueExtractor.cpp
#include "KeyValueExtractor.hpp"
#include "DocumentExtractor.hpp"
#include "SentenceExtractor.hpp"
#include "OutputFormatter.hpp"
#include "../api/NLPEngine.hpp"
#include "../db/DatabaseManager.hpp"
#include "../db/WordRepository.hpp"
#include "../db/SentenceRepository.hpp"
#include "../db/PatternRepository.hpp"
#include "../nlp/TagStats.hpp"
#include "../core/Command.hpp"
#include "../nlp/Morphology.hpp"
#include "../nlp/Tokenizer.hpp"
#include "../dialogue/ContextualCorrelator.hpp"
#include "../utils/LearningHelpers.hpp"
#include "../utils/StringConversions.hpp"
#include "../utils/StringUtils.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <unordered_set>

#define DEBUG_EXTRACTION

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
KeyValueExtractor::KeyValueExtractor(const std::string& semanticDbPath,
                                     const std::string& patternDbPath,
                                     const std::string& language) {
    DatabaseManager::instance().init(semanticDbPath);
    if (!patternDbPath.empty())
        DatabaseManager::instance().init(patternDbPath);

    WordRepository::setDatabasePath(semanticDbPath);
    SentenceRepository::setDatabasePath(semanticDbPath);
    PatternRepository::setDatabasePath(patternDbPath);
    TagStats::setDatabasePath(patternDbPath);

    WordRepository::initializeTables();
    SentenceRepository::initializeTables();
    PatternRepository::initializeTables();
    TagStats::initializeTables();

    morphology::setLanguage(language);
    TagStats::setLanguage(language);
    TagStats::loadDefaultFromStatic();

    if (!patternDbPath.empty()) {
        patternCorrW = std::make_unique<PatternCorrelator>(patternDbPath, "");
        patternCorrC = std::make_unique<PatternCorrelator>(patternDbPath, "_chunk");
        patternCorrL = std::make_unique<PatternCorrelator>(patternDbPath, "_letter");
        patternCorrS = std::make_unique<PatternCorrelator>(patternDbPath, "_syllable");
        contextualCorrelator_      = std::make_unique<ContextualCorrelator>(patternDbPath);
        chunkCorrelator_           = std::make_unique<ChunkCorrelator>(patternDbPath);
        letterCorrelator_          = std::make_unique<LetterCorrelator>(patternDbPath);
        syllableCorrelator_        = std::make_unique<SyllableCorrelator>(patternDbPath);
    }
}

KeyValueExtractor::~KeyValueExtractor() = default;

void KeyValueExtractor::setLanguage(const std::string& language) {
    morphology::setLanguage(language);
    TagStats::setLanguage(language);
    TagStats::loadDefaultFromStatic();
}

// ---------------------------------------------------------------------------
// Especificaciones
// ---------------------------------------------------------------------------
void KeyValueExtractor::addSpecification(const ExtractionSpec& spec) {
    specs_.push_back(spec);
}

void KeyValueExtractor::clearSpecifications() {
    specs_.clear();
}

bool KeyValueExtractor::loadSpecificationsFromJSON(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) return false;
    json j;
    try { file >> j; } catch (...) { return false; }

    specs_.clear();
    if (j.contains("specifications") && j["specifications"].is_array()) {
        for (const auto& item : j["specifications"]) {
            ExtractionSpec spec;
            spec.fieldName = item.value("fieldName", "");
            if (item.contains("typePattern") && item["typePattern"].is_array())
                for (int v : item["typePattern"])
                    spec.typePattern.push_back(static_cast<WordType>(v));
            if (item.contains("keywords") && item["keywords"].is_array())
                for (const auto& kw : item["keywords"])
                    spec.keywords.push_back(kw.get<std::string>());
            spec.useSynonyms = item.value("useSynonyms", true);
            std::string roleStr = item.value("role", "NONE");
            if (roleStr == "SUBJECT") spec.role = SemanticRole::SUBJECT;
            else if (roleStr == "OBJECT") spec.role = SemanticRole::OBJECT;
            else if (roleStr == "VERB") spec.role = SemanticRole::VERB;
            else if (roleStr == "TIME") spec.role = SemanticRole::TIME;
            else if (roleStr == "AMOUNT") spec.role = SemanticRole::AMOUNT;
            else spec.role = SemanticRole::NONE;

            std::string typeStr = item.value("expectedType", "TEXT");
            if (typeStr == "DATE") spec.expectedType = TokenType::DATE;
            else if (typeStr == "NUMBER") spec.expectedType = TokenType::NUMBER;
            else if (typeStr == "EMAIL") spec.expectedType = TokenType::EMAIL;
            else if (typeStr == "MONEY") spec.expectedType = TokenType::MONEY;
            else if (typeStr == "PHONE") spec.expectedType = TokenType::PHONE;
            else spec.expectedType = TokenType::WORD;

            spec.required = item.value("required", false);
            spec.contextTrigger = item.value("contextTrigger", "");

            spec.multiple = item.value("multiple", false);

            specs_.push_back(spec);
        }
    }
    return true;
}

bool KeyValueExtractor::saveSpecificationsToJSON(const std::string& jsonPath) const {
    json j;
    j["specifications"] = json::array();
    for (const auto& spec : specs_) {
        json item;
        item["fieldName"] = spec.fieldName;
        if (!spec.typePattern.empty()) {
            json arr = json::array();
            for (auto t : spec.typePattern) arr.push_back(static_cast<int>(t));
            item["typePattern"] = arr;
        }
        if (!spec.keywords.empty()) item["keywords"] = spec.keywords;
        item["useSynonyms"] = spec.useSynonyms;
        std::string roleStr;
        switch (spec.role) {
            case SemanticRole::SUBJECT: roleStr = "SUBJECT"; break;
            case SemanticRole::OBJECT:  roleStr = "OBJECT"; break;
            case SemanticRole::VERB:    roleStr = "VERB"; break;
            case SemanticRole::TIME:    roleStr = "TIME"; break;
            case SemanticRole::AMOUNT:  roleStr = "AMOUNT"; break;
            default: roleStr = "NONE";
        }
        item["role"] = roleStr;
        std::string typeStr;
        switch (spec.expectedType) {
            case TokenType::DATE:   typeStr = "DATE"; break;
            case TokenType::NUMBER: typeStr = "NUMBER"; break;
            case TokenType::EMAIL:  typeStr = "EMAIL"; break;
            case TokenType::MONEY:  typeStr = "MONEY"; break;
            case TokenType::PHONE:  typeStr = "PHONE"; break;
            default: typeStr = "TEXT";
        }
        item["expectedType"] = typeStr;
        item["required"] = spec.required;
        item["contextTrigger"] = spec.contextTrigger;

        item["multiple"] = spec.multiple;

        j["specifications"].push_back(item);
    }
    std::ofstream file(jsonPath);
    if (!file) return false;
    file << j.dump(4);
    return true;
}

// ---------------------------------------------------------------------------
// parseCommand
// ---------------------------------------------------------------------------
bool KeyValueExtractor::parseCommand(const std::string& userInput, ExtractionRequest& outRequest) {
    outRequest.originalCommand = userInput;

    auto type = detectCommandFromPhrase(userInput);
    if(type.has_value()){
        outRequest.commandType = type.value();
    }else{
        errors_.push_back("No se reconoce un comando válido en: " + userInput);
        return false;
    }
    if (outRequest.commandType != CommandType::EXTRACT &&
        outRequest.commandType != CommandType::FETCH &&
        outRequest.commandType != CommandType::RETRIEVE &&
        outRequest.commandType != CommandType::QUERY) {
        errors_.push_back("El comando no es una solicitud de extracción de datos.");
        return false;
    }
    outRequest.fieldName = extractFieldNameFromCommand(userInput, outRequest.commandType);
    outRequest.expectedType = SentenceExtractor::inferTypeFromContext(outRequest.fieldName, Sentence(), chunkCorrelator_.get());

    outRequest.keywords = generateSynonymsFor(outRequest.fieldName);
    std::istringstream iss(outRequest.fieldName);
    std::string word;
    while (iss >> word) outRequest.keywords.push_back(word);
    std::sort(outRequest.keywords.begin(), outRequest.keywords.end());
    outRequest.keywords.erase(std::unique(outRequest.keywords.begin(), outRequest.keywords.end()), outRequest.keywords.end());

    outRequest.feedbackRequested = true;
    return true;
}

// ---------------------------------------------------------------------------
// executeExtraction - corregido para multi-campo
// ---------------------------------------------------------------------------
bool KeyValueExtractor::executeExtraction(ExtractionRequest& request,
                                          std::unordered_map<std::string, std::string>& result,
                                          std::vector<std::string>& errors) {
    errors.clear();
    bool multiMode = request.fieldName.empty() || request.fieldName == "multiple";

    // Guardar especificaciones originales solo si vamos a modificarlas (modo single)
    std::vector<ExtractionSpec> originalSpecs;
    if (!multiMode) {
        originalSpecs = specs_;
    }

    if (request.documentPath.empty()) {
        errors.push_back("No se especificó el documento.");
        return false;
    }

    std::string textContent;
    std::string ext = request.documentPath.substr(request.documentPath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    bool ok = false;
    if (ext == "txt") ok = DocumentExtractor::extractTextFromTXT(request.documentPath, textContent);
    else if (ext == "pdf") ok = DocumentExtractor::extractTextFromPDF(request.documentPath, textContent);
    else if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "tiff")
        ok = DocumentExtractor::extractTextFromImage(request.documentPath, textContent);
    else ok = DocumentExtractor::extractTextFromTXT(request.documentPath, textContent);

    if (!ok || textContent.empty()) {
        errors.push_back("No se pudo extraer texto del documento o el documento está vacío.");
        return false;
    }

    if (letterCorrelator_ && !textContent.empty()) {
        textContent = DocumentExtractor::correctOCR(textContent, letterCorrelator_.get());
    }
    // Configurar specs para la extracción
    if (!multiMode) {
        specs_.clear();
        ExtractionSpec tempSpec;
        tempSpec.fieldName = request.fieldName;
        tempSpec.keywords = request.keywords;
        tempSpec.useSynonyms = true;
        tempSpec.expectedType = request.expectedType;
        tempSpec.required = true;
        // Por defecto en modo simple, no queremos múltiple a menos que se indique
        tempSpec.multiple = false;
        specs_.push_back(tempSpec);
    }
    // Si es multiMode, specs_ ya contiene las especificaciones cargadas

    auto extracted = extractFromText(textContent);

    // Restaurar specs originales si era modo single
    if (!multiMode) {
        specs_ = originalSpecs;
    }

    if (extracted.empty() && !multiMode) {
        errors.push_back("No se encontró el campo '" + request.fieldName + "' en el documento.");
        return false;
    }

    result = extracted;
    request.extractionSuccess = true;
    return true;
}

// ---------------------------------------------------------------------------
// extractFromText - MODIFICADO para soportar extracción múltiple
// ---------------------------------------------------------------------------
std::unordered_map<std::string, std::string> KeyValueExtractor::extractFromText(const std::string& text) {
    std::unordered_map<std::string, std::string> result;
    if (specs_.empty()) {
        errors_.push_back("No hay especificaciones de extracción cargadas");
        return result;
    }

    std::vector<Sentence> sentences;
    std::vector<std::string> textsXtracted = splitIntoSentences(text);
    for(auto tx : textsXtracted){
        std::vector<Word> words = createWordVector(tx);
        Sentence sent(words);
        sentences.push_back(sent);
    }
    if (sentences.empty()) {
        errors_.push_back("No se pudo extraer ninguna oración del texto");
        return result;
    }

#ifdef DEBUG_EXTRACTION
    std::cerr << "[DEBUG] Procesando " << specs_.size() << " especificaciones sobre " << sentences.size() << " oraciones\n";
#endif

    for (const auto& spec : specs_) {
        std::vector<std::string> allValues;  // Acumula todos los valores encontrados

        auto addValue = [&](const std::string& val) {
            if (!val.empty()) allValues.push_back(val);
        };

        // Si hay contextTrigger, filtrar oraciones que lo contengan
        if (!spec.contextTrigger.empty()) {
            for (const auto& sent : sentences) {
                if (sentenceContainsTrigger(sent, spec.contextTrigger)) {
                    std::string val = SentenceExtractor::extractFromSentence(sent, spec,
                                                                             chunkCorrelator_.get(),
                                                                             contextualCorrelator_.get());
                    addValue(val);
                    if (!spec.multiple && !val.empty()) break;  // Si no es múltiple, salir tras el primero
                }
            }
        } else {
            for (const auto& sent : sentences) {
                std::string val = SentenceExtractor::extractFromSentence(sent, spec,
                                                                         chunkCorrelator_.get(),
                                                                         contextualCorrelator_.get());
                addValue(val);
                if (!spec.multiple && !val.empty()) break;
            }
        }

        // Construir el valor final según el flag multiple
        std::string finalValue;
        if (!allValues.empty()) {
            if (spec.multiple) {
                // Unir todos los valores con separador ", "
                for (size_t i = 0; i < allValues.size(); ++i) {
                    if (i > 0) finalValue += ", ";
                    finalValue += allValues[i];
                }
            } else {
                finalValue = allValues[0];
            }
        }

#ifdef DEBUG_EXTRACTION
        std::cerr << "[DEBUG] Spec: " << spec.fieldName
                  << " -> valores encontrados: " << allValues.size()
                  << " -> resultado: '" << finalValue << "'\n";
#endif

        if (finalValue.empty() && spec.required) {
            errors_.push_back("Campo requerido no encontrado: " + spec.fieldName);
        } else if (!finalValue.empty()) {
            result[spec.fieldName] = finalValue;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// sentenceContainsTrigger
// ---------------------------------------------------------------------------
bool KeyValueExtractor::sentenceContainsTrigger(const Sentence& sent, const std::string& trigger) const {
    if (trigger.empty()) return true;
    std::string sentText = StringUtils::toLowerNoAccentsSafe(sent.toString());
    std::string trigLower = StringUtils::toLowerNoAccentsSafe(trigger);
    return sentText.find(trigLower) != std::string::npos;
}

void KeyValueExtractor::provideFeedback(const ExtractionRequest& request, bool wasCorrect) {
    if (request.fieldName.empty()) return;
    Word fieldWord(request.fieldName);
    WordRepository::load(request.fieldName, fieldWord);

    if (wasCorrect) {
        fieldWord.incrementFrequency();
        WordRepository::save(fieldWord);
    } else {
        if (request.correctedValue.empty()) return;
        fieldWord.addRelated(request.correctedValue, 0.8);
        WordRepository::save(fieldWord);

        Word correctWord(request.correctedValue);
        WordRepository::load(request.correctedValue, correctWord);
        correctWord.addRelated(request.fieldName, 0.8);
        WordRepository::save(correctWord);
    }
}

// ---------------------------------------------------------------------------
// Helper: generar sinónimos usando WordRepository
// ---------------------------------------------------------------------------
std::vector<std::string> KeyValueExtractor::generateSynonymsFor(const std::string& phrase) {
    std::unordered_set<std::string> synSet;
    std::vector<std::string> words;
    std::istringstream iss(phrase);
    std::string w;
    while (iss >> w) words.push_back(w);
    for (const auto& word : words) {
        synSet.insert(word);
        Word wordObj;
        if (WordRepository::load(word, wordObj)) {
            for (const auto& [rel, weight] : wordObj.getRelated()) {
                if (weight > 0.3) synSet.insert(rel);
            }
        }
    }
    if (phrase.find(' ') != std::string::npos) synSet.insert(phrase);
    std::vector<std::string> result(synSet.begin(), synSet.end());
    std::sort(result.begin(), result.end());
    return result;
}
