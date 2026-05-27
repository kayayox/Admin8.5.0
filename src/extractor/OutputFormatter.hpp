// OutputFormatter.hpp
#ifndef OUTPUT_FORMATTER_HPP
#define OUTPUT_FORMATTER_HPP

#include <string>
#include <unordered_map>

class OutputFormatter {
public:
    static void saveToJSON(const std::unordered_map<std::string, std::string>& data,
                           const std::string& path, const std::string& docName);
    static void saveToCSV(const std::unordered_map<std::string, std::string>& data,
                          const std::string& path);
    static void saveToXML(const std::unordered_map<std::string, std::string>& data,
                          const std::string& path);
    static void saveToSQLite(const std::unordered_map<std::string, std::string>& data,
                             const std::string& dbPath, const std::string& docName);
};

#endif
