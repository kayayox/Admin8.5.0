/**
 * @file ResponseTemplates.hpp
 * @brief Template‑based response generation structures.
 *
 * Templates contain placeholders (e.g., {NOUN}, {VERB}) that are filled by the
 * SlotFiller. They are language‑specific and can be automatically created when
 * a new PatternType sequence is encountered.
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef RESPONSE_TEMPLATES_HPP
#define RESPONSE_TEMPLATES_HPP

#include "../core/Pattern.hpp"   // for PatternType
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

/**
 * @struct ResponseTemplate
 * @brief A response template with placeholders and metadata.
 *
 * Placeholders use the form {ROLE} where ROLE is one of:
 * SUBJECT, OBJECT, VERB, ADJECTIVE, NOUN, PRONOUN, ARTICLE, etc.
 * They are extracted automatically from the template text.
 */
struct ResponseTemplate {
    int id = -1;
    std::string language;                       ///< e.g. "es", "en"
    PatternType patternType = PatternType::SENTENCES;
    std::vector<std::string> slots;             ///< placeholders extracted from templateText
    std::string templateText;                   ///< raw template with {PLACEHOLDER}s
    int         priority = 0;                   ///< manually assigned or learned priority
    int         frequency_ = 1;                  ///< usage frequency (for learning)
    std::vector<std::string> contextKeywords;   ///< optional trigger words
    uint32_t timestamp_ = 0;                    ///< Last access (moved to end, with default)

    /// Extracts all placeholders of the form {NAME} from a template string.
    static std::vector<std::string> extractSlots(const std::string& tmpl);
};

/**
 * @class TemplateMatcher
 * @brief Manages a collection of templates, matching them by pattern type and
 *        keywords, and creating new ones when no match exists.
 */
class TemplateMatcher {
public:
    /// Registers a template (in memory).
    void registerTemplate(const ResponseTemplate& tmpl);

    /**
     * @brief Finds the best template for a given pattern type, language and keywords.
     * @param lang         The current language.
     * @param patternType  The input sentence pattern type.
     * @param keywords     Relevant words from the premise.
     * @return Pointer to the best matching template, or nullptr if none.
     */
    const ResponseTemplate* matchTemplate(const std::string& lang,
                                          PatternType patternType,
                                          const std::vector<std::string>& keywords) const;

    /**
     * @brief Creates a generic template from a word‑type sequence and registers it.
     * @param lang       Language tag.
     * @param patternType Pattern classification.
     * @param typeSequence Sequence of WordType of the sentence (used to build slots).
     * @param keywords   Context words.
     * @return The newly created template.
     */
    ResponseTemplate createAndRegisterTemplate(const std::string& lang,
                                               PatternType patternType,
                                               const std::vector<WordType>& typeSequence,
                                               const std::vector<std::string>& keywords);

    /// Replaces placeholders with concrete values.
    std::string fillTemplate(const ResponseTemplate& tmpl,
                             const std::unordered_map<std::string, std::string>& slotValues) const;

    /// Loads a minimal set of default bilingual templates.
    void loadDefaultTemplates();

    /// Returns all templates (const access).
    const std::vector<ResponseTemplate>& getAll() const { return templates_; }

    /// Clears in‑memory templates.
    void clear() { templates_.clear(); }

private:
    std::vector<ResponseTemplate> templates_;
};

#endif // RESPONSE_TEMPLATES_HPP
