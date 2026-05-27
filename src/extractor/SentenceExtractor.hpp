// SentenceExtractor.hpp
#ifndef SENTENCE_EXTRACTOR_HPP
#define SENTENCE_EXTRACTOR_HPP

#include <string>
#include <vector>
#include "../core/Sentence.hpp"
#include "KeyValueExtractor.hpp"

class ChunkCorrelator;
class ContextualCorrelator;

class SentenceExtractor {
public:
    static std::string extractFromSentence(const Sentence& sentence,
                                           const ExtractionSpec& spec,
                                           ChunkCorrelator* chunkCorrelator = nullptr,
                                           ContextualCorrelator* contextualCorrelator = nullptr);

    static TokenType inferTypeFromContext(const std::string& fieldName,
                                          const Sentence& sent,
                                          ChunkCorrelator* chunkCorrelator);
};

#endif
