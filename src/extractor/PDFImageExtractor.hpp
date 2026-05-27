/**
 * @file PDFImageExtractor.hpp
 * @brief Extracción de texto desde PDF e imágenes usando herramientas externas.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef PDF_IMAGE_EXTRACTOR_HPP
#define PDF_IMAGE_EXTRACTOR_HPP

#include <string>

namespace PDFImageExtractor {
    bool extractTextFromPDF(const std::string& pdfPath, std::string& outText);
    bool extractTextFromImage(const std::string& imagePath, std::string& outText);
}

#endif
