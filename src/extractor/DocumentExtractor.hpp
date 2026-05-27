// DocumentExtractor.hpp
#ifndef DOCUMENT_EXTRACTOR_HPP
#define DOCUMENT_EXTRACTOR_HPP

#include <string>
#include <memory>

class LetterCorrelator;

class DocumentExtractor {
public:
    static bool extractTextFromTXT(const std::string& txtPath, std::string& outText);
    static bool extractTextFromPDF(const std::string& pdfPath, std::string& outText);
    static bool extractTextFromImage(const std::string& imagePath, std::string& outText);
    static std::string correctOCR(const std::string& raw, LetterCorrelator* letterCorrelator = nullptr);
};

#endif
