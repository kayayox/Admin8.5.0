/**
 * @file TemplateRepository.hpp
 * @brief Persistence for response templates in the semantic database,
 *        with language support.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef TEMPLATE_REPOSITORY_HPP
#define TEMPLATE_REPOSITORY_HPP

#include "../utils/ResponseTemplates.hpp"
#include <optional>
#include <string>
#include <vector>

class TemplateRepository {
public:
    static void setDatabasePath(const std::string& path);
    static void initializeTables();

    static void save(ResponseTemplate& tmpl);
    static std::optional<ResponseTemplate> loadById(int id);
    static std::vector<ResponseTemplate> loadAll();
    static bool remove(int id);

    static uint32_t getLastTime(int id);

    static bool updateTime(const ResponseTemplate& tmpl);

    /// Loads default bilingual templates if the database is empty.
    static void loadDefaultIfEmpty();

private:
    static std::string dbPath_;
};

#endif // TEMPLATE_REPOSITORY_HPP
