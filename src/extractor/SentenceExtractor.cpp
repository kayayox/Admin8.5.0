// SentenceExtractor.cpp
#include "SentenceExtractor.hpp"
#include "../common/types.hpp"
#include "../dialogue/ChunkCorrelator.hpp"
#include "../dialogue/ContextualCorrelator.hpp"
#include "../utils/Chunker.hpp"
#include "../utils/StringUtils.hpp"
#include "../core/Command.hpp"
#include "../db/WordRepository.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace StringUtils;

// ---------------------------------------------------------------------------
// Helper: extraer valor multi-palabra después de una palabra clave
// ---------------------------------------------------------------------------
static std::string extractValueAfterKeyword(const Sentence& sent, size_t keywordIndex) {
    const auto& blocks = sent.getBlocks();
    if (keywordIndex + 1 >= blocks.size()) return "";
    std::string value;
    for (size_t i = keywordIndex + 1; i < blocks.size(); ++i) {
        const Block& b = blocks[i];
        if (b.type == WordType::VERB || b.type == WordType::PREPOSITION || b.type == WordType::CONJUNCTION) {
            break;
        }
        if (b.type != WordType::UNDEFINED) {
            if (!value.empty()) value += " ";
            value += b.text;
        }
        if (!value.empty() && (value.back() == '.' || value.back() == '!' || value.back() == '?')) {
            value.pop_back();
            break;
        }
    }
    value = toLowerNoAccentsSafe(value);
    return value;
}

// ---------------------------------------------------------------------------
// Extracción por patrón de tipos
// ---------------------------------------------------------------------------
static std::string extractByTypePattern(const Sentence& sent, const std::vector<WordType>& pattern) {
    const auto& blocks = sent.getBlocks();
    if (pattern.empty() || blocks.size() < pattern.size()) return "";
    for (size_t i = 0; i + pattern.size() <= blocks.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < pattern.size(); ++j) {
            if (blocks[i + j].type != pattern[j]) { match = false; break; }
        }
        if (match) {
            std::string result;
            for (size_t j = 0; j < pattern.size(); ++j) {
                if (j > 0) result += " ";
                result += blocks[i + j].text;
            }
            return result;
        }
    }
    return "";
}

// ---------------------------------------------------------------------------
// Extracción por palabras clave (sin aprendizaje, solo búsqueda)
// ---------------------------------------------------------------------------
static std::string extractByKeywords(const Sentence& sent, const std::vector<std::string>& keywords,
                                     bool useSynonyms) {
    const auto& blocks = sent.getBlocks();
    if (blocks.empty()) return "";

    // Función para normalizar
    auto norm = [](const std::string& s) { return toLowerNoAccentsSafe(s); };

    // 1. Buscar frases multi-palabra exactas
    for (const auto& kw : keywords) {
        if (kw.find(' ') != std::string::npos) {
            std::vector<std::string> kwTokens;
            std::istringstream iss(kw);
            std::string tok;
            while (iss >> tok) kwTokens.push_back(norm(tok));
            if (kwTokens.empty()) continue;
            for (size_t i = 0; i + kwTokens.size() <= blocks.size(); ++i) {
                bool match = true;
                for (size_t j = 0; j < kwTokens.size(); ++j) {
                    if (norm(blocks[i+j].text) != kwTokens[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    return extractValueAfterKeyword(sent, i + kwTokens.size() - 1);
                }
            }
        }
    }

    // 2. Buscar palabras individuales (exactas o sinónimos)
    for (const auto& kw : keywords) {
        std::string kwNorm = norm(kw);
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (norm(blocks[i].text) == kwNorm) {
                return extractValueAfterKeyword(sent, i);
            }
        }
        if (useSynonyms) {
            // Buscar sinónimos en WordRepository
            Word w;
            if (WordRepository::load(kw, w)) {
                for (const auto& [syn, weight] : w.getRelated()) {
                    if (weight < 0.3) continue;
                    std::string synNorm = norm(syn);
                    for (size_t i = 0; i < blocks.size(); ++i) {
                        if (norm(blocks[i].text) == synNorm) {
                            return extractValueAfterKeyword(sent, i);
                        }
                    }
                }
            }
        }
    }
    return "";
}

// ---------------------------------------------------------------------------
// Extracción por rol semántico
// ---------------------------------------------------------------------------
static std::string extractBySemanticRole(const Sentence& sent, SemanticRole role) {
    const auto& blocks = sent.getBlocks();
    switch (role) {
        case SemanticRole::SUBJECT: {
            for (size_t i = 0; i < blocks.size(); ++i) {
                if (blocks[i].type == WordType::NOUN || blocks[i].type == WordType::PRONOUN) {
                    if (i > 0 && blocks[i-1].type == WordType::PREPOSITION) continue;
                    return blocks[i].text;
                }
            }
            return "";
        }
        case SemanticRole::OBJECT: {
            bool afterVerb = false;
            for (size_t i = 0; i < blocks.size(); ++i) {
                if (!afterVerb && blocks[i].type == WordType::VERB) {
                    afterVerb = true;
                    continue;
                }
                if (afterVerb && (blocks[i].type == WordType::NOUN || blocks[i].type == WordType::PRONOUN)) {
                    return blocks[i].text;
                }
            }
            return "";
        }
        case SemanticRole::VERB: {
            for (const auto& b : blocks)
                if (b.type == WordType::VERB) return b.text;
            return "";
        }
        case SemanticRole::TIME: {
            for (const auto& b : blocks)
                if (b.type == WordType::DATE || isDateFormat(b.text)) return b.text;
            return "";
        }
        case SemanticRole::AMOUNT: {
            for (size_t i = 0; i < blocks.size(); ++i) {
                if (blocks[i].type == WordType::NUMERAL && i+1 < blocks.size() && isMoney(blocks[i+1].text))
                    return blocks[i].text + " " + blocks[i+1].text;
                if (isMoney(blocks[i].text)) {
                    if (i+1 < blocks.size() && blocks[i+1].type == WordType::NUMERAL)
                        return blocks[i].text + " " + blocks[i+1].text;
                    return blocks[i].text;
                }
            }
            return "";
        }
        default: return "";
    }
}

// ---------------------------------------------------------------------------
// Método principal de extracción (sin aprendizaje)
// ---------------------------------------------------------------------------
std::string SentenceExtractor::extractFromSentence(const Sentence& sentence,
                                                   const ExtractionSpec& spec,
                                                   ChunkCorrelator* /*chunkCorrelator*/,
                                                   ContextualCorrelator* /*contextualCorrelator*/) {
    std::string value;

    if (!spec.typePattern.empty()) {
        value = extractByTypePattern(sentence, spec.typePattern);
        if (!value.empty() && validateValue(value, spec.expectedType)) return value;
    }

    if (!spec.keywords.empty()) {
        value = extractByKeywords(sentence, spec.keywords, spec.useSynonyms);
        if (!value.empty() && validateValue(value, spec.expectedType)) return value;
    }

    if (spec.role != SemanticRole::NONE) {
        value = extractBySemanticRole(sentence, spec.role);
        if (!value.empty() && validateValue(value, spec.expectedType)) return value;
    }

    return "";
}

// ---------------------------------------------------------------------------
// Inferencia de tipo por contexto (sin cambios relevantes)
// ---------------------------------------------------------------------------
TokenType SentenceExtractor::inferTypeFromContext(const std::string& fieldName,
                                                  const Sentence& sent,
                                                  ChunkCorrelator* chunkCorrelator) {
    if (chunkCorrelator) {
        std::vector<Word> words;
        for (const auto& blk : sent.getBlocks()) {
            Word w(blk.text);
            w.setType(blk.type);
            words.push_back(w);
        }
        std::vector<std::string> chunks = Chunker::chunk(words);
        for (size_t i = 0; i < chunks.size(); ++i) {
            if (chunks[i].find(fieldName) != std::string::npos && i+1 < chunks.size()) {
                std::vector<std::pair<WordPattern, double>> outcomes;
                std::string prev = (i > 0) ? chunks[i-1] : "__START__";
                // Solo consulta, sin aprendizaje
                if (chunkCorrelator->queryNext(chunks[i], {prev}, outcomes)) {
                    for (const auto& [pat, prob] : outcomes) {
                        for (const auto& [tag, w] : pat) {
                            if (tag == "DATE") return TokenType::DATE;
                            if (tag == "NUMBER") return TokenType::NUMBER;
                            if (tag == "MONEY") return TokenType::MONEY;
                            if (tag == "EMAIL") return TokenType::EMAIL;
                            if (tag == "PHONE") return TokenType::PHONE;
                        }
                    }
                }
                break;
            }
        }
    }
    std::string lower = toLowerNoAccentsSafe(fieldName);
    if (lower.find("fecha") != std::string::npos || lower.find("date") != std::string::npos) return TokenType::DATE;
    if (lower.find("precio") != std::string::npos || lower.find("monto") != std::string::npos) return TokenType::MONEY;
    if (lower.find("email") != std::string::npos) return TokenType::EMAIL;
    if (lower.find("tel") != std::string::npos || lower.find("phone") != std::string::npos) return TokenType::PHONE;
    if (lower.find("num") != std::string::npos || lower.find("cant") != std::string::npos) return TokenType::NUMBER;
    return TokenType::WORD;
}
