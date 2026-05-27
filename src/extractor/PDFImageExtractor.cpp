/**
 * @file PDFImageExtractor.cpp
 * @brief Implementación usando comandos externos (pdftotext, tesseract).
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "PDFImageExtractor.hpp"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>

namespace {
    bool execCommand(const std::string& cmd, std::string& output) {
        char buffer[128];
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return false;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        pclose(pipe);
        return !output.empty();
    }
}

bool PDFImageExtractor::extractTextFromPDF(const std::string& pdfPath, std::string& outText) {
    std::string tmpTxt = pdfPath + ".tmp.txt";
    std::string cmd = "pdftotext \"" + pdfPath + "\" \"" + tmpTxt + "\"";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Error ejecutando pdftotext. ¿Instalado? (sudo apt install poppler-utils)" << std::endl;
        return false;
    }
    std::ifstream file(tmpTxt);
    if (!file) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    outText = content;
    std::remove(tmpTxt.c_str());
    return true;
}

bool PDFImageExtractor::extractTextFromImage(const std::string& imagePath, std::string& outText) {
    std::string tmpTxt = imagePath + ".txt";
    std::string cmd = "tesseract \"" + imagePath + "\" \"" + tmpTxt.substr(0, tmpTxt.find_last_of('.')) + "\"";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Error ejecutando tesseract. ¿Instalado? (sudo apt install tesseract-ocr)" << std::endl;
        return false;
    }
    std::ifstream file(tmpTxt);
    if (!file) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    outText = content;
    std::remove(tmpTxt.c_str());
    return true;
}
