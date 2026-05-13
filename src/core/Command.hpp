/**
 * @file Command.hpp
 * @brief Command detection and decomposition into action, subject, and object.
 *
 * Supports Spanish and English via setLanguage(). Uses static verb lists for each language.
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef ADMIN850_COMMAND_HPP
#define ADMIN850_COMMAND_HPP

#include "../common/types.hpp"
#include "Word.hpp"
#include "Sentence.hpp"
#include <optional>
#include <string>
#include <vector>

/**
 * @class Command
 * @brief Represents a detected command with its action, subject, object, and success flag.
 */
class Command {
public:
    Command() : action_(CommandType::LEARN), success_(false) {}

    Command(CommandType action, std::string subject, std::string object)
        : action_(action), subject_(std::move(subject)), object_(std::move(object)), success_(false) {}

    void setSuccess(bool success) { success_ = success; }
    [[nodiscard]] CommandType       getAction()  const { return action_; }
    [[nodiscard]] const std::string& getSubject() const { return subject_; }
    [[nodiscard]] const std::string& getObject()  const { return object_; }
    [[nodiscard]] bool               isSuccess()  const { return success_; }

private:
    CommandType  action_;
    std::string  subject_;
    std::string  object_;
    bool         success_;
};

// --- Language configuration ---
void setCommandLanguage(const std::string& lang);

std::string getCommandLanguage();

// --- Detection functions ---

/**
 * @brief Detects the command type from a raw phrase.
 * @param phrase Input text.
 * @return The command type if recognised, otherwise std::nullopt.
 */
std::optional<CommandType> detectCommandFromPhrase(const std::string& phrase);

/** @brief Detects subjects (nouns after a preposition or similar) from a word vector. */
std::vector<std::string> detectSubjects(const std::vector<Word>& words);

/** @brief Detects subjects from a Sentence. */
std::vector<std::string> detectSubjects(const Sentence& sentence);

/** @brief Detects objects (nouns after the verb, excluding subjects) from a word vector. */
std::vector<std::string> detectObjects(const std::vector<Word>& words);

/** @brief Detects objects from a Sentence. */
std::vector<std::string> detectObjects(const Sentence& sentence);

#endif // ADMIN850_COMMAND_HPP
