/**
 * @file ChunkCorrelator.cpp
 * @brief Implementation of the chunk correlator, including learning with previous context and queries.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "ChunkCorrelator.hpp"
#include "../common/types.hpp"
#include "../utils/LearningHelpers.hpp"

ChunkCorrelator::ChunkCorrelator(const std::string& dbPath)
    : corr(std::make_unique<PatternCorrelator>(dbPath, "_chunk")) {}

WordPattern ChunkCorrelator::makePattern(const std::vector<std::string>& chunks) const {
    WordPattern pat;
    for (const auto& ch : chunks) {
        pat[ch] = 1.0f;
    }
    return pat;
}

void ChunkCorrelator::learnWithPreviousTwo(const std::vector<std::string>& chunks) {
    for (size_t i = 0; i < chunks.size(); ++i) {
        const std::string& current = chunks[i];
        std::vector<std::string> prevChunks;
        if (i >= 2) {
            prevChunks.push_back(chunks[i-2]);
            prevChunks.push_back(chunks[i-1]);
        } else if (i == 1) {
            prevChunks.push_back(chunks[i-1]);
        } else {
            continue;
        }

        WordPattern prevPat = makePattern(prevChunks);
        if (i + 1 < chunks.size()) {
            WordPattern nextPat = makePattern({chunks[i+1]});
            corr->record(current, prevPat, nextPat, 1.0f);
        }
    }
}

void ChunkCorrelator::learnNextChunkDirect(const std::vector<std::string>& chunks) {
    for (size_t i = 0; i + 1 < chunks.size(); ++i) {
        const std::string& current = chunks[i];
        WordPattern prevPat = makePattern({"__NO_CONTEXT__"});
        WordPattern nextPat = makePattern({chunks[i+1]});
        corr->record(current, prevPat, nextPat, 1.0f);
    }
}

void ChunkCorrelator::learnFromClassifiedSentence(const std::vector<Word>& sentence) {
    std::vector<std::string> chunks = Chunker::chunk(sentence);
    learnWithPreviousTwo(chunks);
}

bool ChunkCorrelator::queryNext(const std::string& current,
                                const std::vector<std::string>& previousChunks,
                                std::vector<std::pair<WordPattern, double>>& outcomes) {
    WordPattern prevPat = makePattern(previousChunks);
    return corr->query(current, prevPat, outcomes);
}

bool ChunkCorrelator::GqueryNext(const std::string& text,
                                std::vector<std::pair<WordPattern, double>>& outcomes) {
    std::vector<Word> words = createWordVector(text);
    std::vector<std::string> chunks = Chunker::chunk(words);
    const std::string& current = chunks.back();
    std::vector<std::string> prevChunks;
    if (chunks.size() >= 3) {
        prevChunks.push_back(chunks[chunks.size()-3]);
        prevChunks.push_back(chunks[chunks.size()-2]);
    } else if (chunks.size() >= 2) {
        prevChunks.push_back(chunks[chunks.size()-2]);
    }

    WordPattern prevPat = makePattern(prevChunks);
    return queryNext(current, prevChunks, outcomes);
}

bool ChunkCorrelator::GqueryNext(const std::vector<Word>& text,
                                std::vector<std::pair<WordPattern, double>>& outcomes) {
    std::vector<std::string> chunks = Chunker::chunk(text);
    if (chunks.empty()) return false;
    const std::string& current = chunks.back();
    std::vector<std::string> prevChunks;
    if (chunks.size() >= 3) {
        prevChunks.push_back(chunks[chunks.size()-3]);
        prevChunks.push_back(chunks[chunks.size()-2]);
    } else if (chunks.size() >= 2) {
        prevChunks.push_back(chunks[chunks.size()-2]);
    } else if (chunks.size() == 1) {
        prevChunks.push_back(chunks[chunks.size()-1]);
    }
    return queryNext(current, prevChunks, outcomes);
}
