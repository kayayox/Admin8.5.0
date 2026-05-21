/**
 * @file ChunkCorrelator.hpp
 * @brief Correlation of chunk sequences (phrases) for higher‑level structure learning and prediction.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef CHUNK_CORRELATOR_HPP
#define CHUNK_CORRELATOR_HPP

#include "PatternCorrelator.hpp"
#include "../utils/Chunker.hpp"
#include <memory>
#include <vector>
#include <string>
#include <utility>

class ChunkCorrelator {
public:
    explicit ChunkCorrelator(const std::string& dbPath);

    void learnWithPreviousTwo(const std::vector<std::string>& chunks);
    void learnNextChunkDirect(const std::vector<std::string>& chunks);
    void learnFromClassifiedSentence(const std::vector<Word>& sentence);

    bool queryNext(const std::string& current,
                   const std::vector<std::string>& previousChunks,
                   std::vector<std::pair<WordPattern, double>>& outcomes);

    bool GqueryNext(const std::string& text,
                    std::vector<std::pair<WordPattern, double>>& outcomes);

    bool GqueryNext(const std::vector<Word>& text,
                    std::vector<std::pair<WordPattern, double>>& outcomes);

private:
    std::unique_ptr<PatternCorrelator> corr;  // uses "_chunk" suffix
    WordPattern makePattern(const std::vector<std::string>& chunks) const;
};

#endif
