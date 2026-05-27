// DocumentExtractor.cpp
#include "DocumentExtractor.hpp"
#include "../extractor/PDFImageExtractor.hpp"
#include "../dialogue/LetterCorrelator.hpp"
#include "../utils/StringUtils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

bool DocumentExtractor::extractTextFromTXT(const std::string& txtPath, std::string& outText) {
    std::ifstream file(txtPath, std::ios::binary);
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    outText = buffer.str();
    outText.erase(std::remove(outText.begin(), outText.end(), '\0'), outText.end());
    return !outText.empty();
}

bool DocumentExtractor::extractTextFromPDF(const std::string& pdfPath, std::string& outText) {
    bool ok = PDFImageExtractor::extractTextFromPDF(pdfPath, outText);
    if (ok && !outText.empty()) {
        outText = correctOCR(outText, nullptr); // el caller deberá pasar su LetterCorrelator
    }
    return ok;
}

bool DocumentExtractor::extractTextFromImage(const std::string& imagePath, std::string& outText) {
    bool ok = PDFImageExtractor::extractTextFromImage(imagePath, outText);
    if (ok && !outText.empty()) {
        outText = correctOCR(outText, nullptr);
    }
    return ok;
}

std::string DocumentExtractor::correctOCR(const std::string& raw, LetterCorrelator* letterCorrelator) {
    if (!letterCorrelator) return raw;
    /*std::string corrected;
    for (size_t i = 0; i < raw.size(); ++i) {
        corrected += raw[i];
        std::vector<std::pair<WordPattern, double>> outcomes;
        if (letterCorrelator->GqueryNext(corrected, outcomes)) {
            char best = raw[i];
            double bestProb = 0.0;
            for (const auto& [pat, prob] : outcomes) {
                if (!pat.empty() && prob > bestProb) {
                    best = pat.begin()->first[0];
                    bestProb = prob;
                }
            }
            corrected += best;
        } else {
            corrected += raw[i];
        }
    }*/
    return raw;
}
