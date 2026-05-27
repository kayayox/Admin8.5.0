// KeyValueExtractor.hpp
#ifndef ADMIN850_KEYVALUE_EXTRACTOR_HPP
#define ADMIN850_KEYVALUE_EXTRACTOR_HPP

#include "../dialogue/ContextualCorrelator.hpp"
#include "../dialogue/ChunkCorrelator.hpp"
#include "../dialogue/LetterCorrelator.hpp"
#include "../dialogue/SyllableCorrelator.hpp"
#include "../core/Word.hpp"
#include "../core/Sentence.hpp"
#include "../common/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

struct ExtractionSpec {
    std::string fieldName;
    std::vector<WordType> typePattern;
    std::vector<std::string> keywords;
    bool useSynonyms = true;

    SemanticRole role = SemanticRole::NONE;

    TokenType expectedType = TokenType::WORD;
    bool required = false;
    std::string contextTrigger;
    bool multiple = true;   // extraer todas las ocurrencias
};

struct ExtractionRequest {
    std::string fieldName;
    TokenType expectedType = TokenType::WORD;
    std::vector<std::string> keywords;
    std::string documentPath;
    std::string outputFormat;
    std::string outputPath;
    bool feedbackRequested = true;
    CommandType commandType = CommandType::EXTRACT;
    std::string originalCommand;

    bool extractionSuccess = false;
    std::string extractedValue;
    std::string correctedValue;

    ExtractionRequest() = default;
    ExtractionRequest(const std::string& name, TokenType type = TokenType::WORD)
        : fieldName(name), expectedType(type) {}
};

class NLPProcessor;

class KeyValueExtractor {
public:
    KeyValueExtractor(const std::string& semanticDbPath,
                      const std::string& patternDbPath = "",
                      const std::string& language = "es");
    ~KeyValueExtractor();

    void setLanguage(const std::string& language);
    bool parseCommand(const std::string& userInput, ExtractionRequest& outRequest);
    bool executeExtraction(ExtractionRequest& request,
                           std::unordered_map<std::string, std::string>& result,
                           std::vector<std::string>& errors);

    void addSpecification(const ExtractionSpec& spec);
    void clearSpecifications();
    bool loadSpecificationsFromJSON(const std::string& jsonPath);
    bool saveSpecificationsToJSON(const std::string& jsonPath) const;
    const std::vector<ExtractionSpec>& getSpecifications() const { return specs_; }
    std::vector<std::string> generateSynonymsFor(const std::string& phrase);
    void provideFeedback(const ExtractionRequest& request, bool wasCorrect);

    std::vector<std::string> getExtractionErrors() const { return errors_; }
    void clearErrors() { errors_.clear(); }

private:
    std::vector<ExtractionSpec> specs_;
    mutable std::vector<std::string> errors_;
    std::unique_ptr<PatternCorrelator> patternCorrW;
    std::unique_ptr<PatternCorrelator> patternCorrC;
    std::unique_ptr<PatternCorrelator> patternCorrL;
    std::unique_ptr<PatternCorrelator> patternCorrS;
    std::unique_ptr<ContextualCorrelator> contextualCorrelator_;
    std::unique_ptr<ChunkCorrelator> chunkCorrelator_;
    std::unique_ptr<LetterCorrelator> letterCorrelator_;
    std::unique_ptr<SyllableCorrelator> syllableCorrelator_;

    std::unordered_map<std::string, std::string> extractFromText(const std::string& text);
    bool sentenceContainsTrigger(const Sentence& sent, const std::string& trigger) const;
};

#endif
