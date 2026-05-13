/**
 * @file Command.cpp
 * @brief Implementation of command detection for Spanish and English.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Command.hpp"
#include "../db/WordRepository.hpp"
#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

// ============================================================================
// Internal state – language
// ============================================================================
static std::string currentLang_ = "es";

void setCommandLanguage(const std::string& lang) { currentLang_ = lang; }
std::string getCommandLanguage() { return currentLang_; }

// ============================================================================
// Verb lists (Spanish & English)
// ============================================================================
namespace {

    // Spanish command verbs (original)
    const std::unordered_map<CommandType, std::vector<std::string>> ES_COMMAND_VERBS = {
        { CommandType::DO,       {"hacer", "haz", "realiza", "ejecuta", "efectúa", "efectua"} },
        { CommandType::ANSWER,   {"responde", "contesta", "di", "contestame"} },
        { CommandType::ASK,      {"pregunta", "consulta", "interroga", "averigua"} },
        { CommandType::TASK,     {"asigna", "encarga", "encomienda", "manda", "delega"} },
        { CommandType::BID,      {"puja", "ofrece", "licita", "apuesta"} },
        { CommandType::GO,       {"ve", "anda", "navega", "dirígete", "dirigete", "accede", "desplázate", "desplazate"} },
        { CommandType::REPORT,   {"informa", "reporta", "notifica"} },
        { CommandType::MOVE,     {"mueve", "desplaza", "traslada"} },
        { CommandType::REPEAT,   {"repite", "vuélvelo", "vuelvelo", "repetir", "repite"} },
        { CommandType::FIND,     {"busca", "encuentra", "localiza", "halla"} },
        { CommandType::CREATE,   {"crea", "genera", "construye", "redacta", "elabora", "produce", "añade"} },
        { CommandType::DELETE,   {"elimina", "borra", "suprime", "quita", "deshaz", "tira", "destruye"} },
        { CommandType::UPDATE,   {"actualiza", "modifica", "edita", "cambia", "corrige"} },
        { CommandType::SEND,     {"envía", "envia", "manda", "transmite", "notifica", "empuja", "mensajea"} },
        { CommandType::CALL,     {"llama", "telefonea", "contacta", "videollama", "comunícate"} },
        { CommandType::PLAY,     {"reproduce", "toca", "play"} },
        { CommandType::PAUSE,    {"pausa", "suspende", "interrumpe"} },
        { CommandType::STOP,     {"para", "detén", "deten", "cancela", "termina", "aborta", "finaliza", "alto"} },
        { CommandType::START,    {"inicia", "comienza", "arranca", "lanza", "empieza", "ejecuta"} },
        { CommandType::SCHEDULE, {"programa", "agenda", "reserva", "planifica"} },
        { CommandType::SET,      {"configura", "ajusta", "establece", "activa", "desactiva", "regula"} },
        { CommandType::SHOW,     {"muestra", "enseña", "visualiza", "lista", "despliega"} },
        { CommandType::HELP,     {"ayuda", "asiste", "auxilio", "help"} },
        { CommandType::OPEN,     {"abre", "desbloquea"} },
        { CommandType::CLOSE,    {"cierra", "clausura"} }
    };

    // English command verbs (mirror of Spanish coverage)
    const std::unordered_map<CommandType, std::vector<std::string>> EN_COMMAND_VERBS = {
        { CommandType::DO,       {"do", "make", "execute", "perform", "run"} },
        { CommandType::ANSWER,   {"answer", "reply", "respond", "tell"} },
        { CommandType::ASK,      {"ask", "question", "query", "inquire"} },
        { CommandType::TASK,     {"assign", "delegate", "task", "commission"} },
        { CommandType::BID,      {"bid", "offer", "bet", "wager"} },
        { CommandType::GO,       {"go", "navigate", "browse", "head", "move", "travel", "visit"} },
        { CommandType::REPORT,   {"report", "inform", "notify", "brief"} },
        { CommandType::MOVE,     {"move", "shift", "transfer", "relocate"} },
        { CommandType::REPEAT,   {"repeat", "redo", "replay"} },
        { CommandType::FIND,     {"find", "search", "locate", "lookup", "discover"} },
        { CommandType::CREATE,   {"create", "make", "generate", "build", "write", "produce", "add"} },
        { CommandType::DELETE,   {"delete", "remove", "erase", "drop", "discard", "destroy", "eliminate"} },
        { CommandType::UPDATE,   {"update", "modify", "edit", "change", "correct", "revise"} },
        { CommandType::SEND,     {"send", "transmit", "forward", "push", "message"} },
        { CommandType::CALL,     {"call", "phone", "contact", "dial"} },
        { CommandType::PLAY,     {"play", "start", "run"} },
        { CommandType::PAUSE,    {"pause", "suspend", "interrupt", "hold"} },
        { CommandType::STOP,     {"stop", "halt", "cancel", "abort", "end", "terminate", "quit"} },
        { CommandType::START,    {"start", "begin", "launch", "initiate", "commence"} },
        { CommandType::SCHEDULE, {"schedule", "plan", "book", "reserve", "set"} },
        { CommandType::SET,      {"set", "configure", "adjust", "enable", "disable", "tune"} },
        { CommandType::SHOW,     {"show", "display", "list", "view", "reveal"} },
        { CommandType::HELP,     {"help", "assist", "support", "aid"} },
        { CommandType::OPEN,     {"open", "unlock"} },
        { CommandType::CLOSE,    {"close", "shut", "lock"} }
    };

    // Create a lowercase -> CommandType map for the current language
    std::unordered_map<std::string, CommandType> buildReverseVerbMap(const std::unordered_map<CommandType, std::vector<std::string>>& langMap) {
        std::unordered_map<std::string, CommandType> rev;
        for (const auto& [cmd, verbs] : langMap) {
            for (const auto& v : verbs) {
                std::string key = v;
                std::transform(key.begin(), key.end(), key.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                rev[key] = cmd;
            }
        }
        return rev;
    }

    const std::unordered_map<CommandType, std::vector<std::string>>& getCurrentVerbMap() {
        return (currentLang_ == "en") ? EN_COMMAND_VERBS : ES_COMMAND_VERBS;
    }

    const std::unordered_map<std::string, CommandType>& getCurrentReverseMap() {
        static std::unordered_map<std::string, CommandType> esReverse = buildReverseVerbMap(ES_COMMAND_VERBS);
        static std::unordered_map<std::string, CommandType> enReverse = buildReverseVerbMap(EN_COMMAND_VERBS);
        return (currentLang_ == "en") ? enReverse : esReverse;
    }

    // Helper: collect nouns after a preposition (or similar) – language‑agnostic
    std::vector<std::string> collectNounsAfter(const std::vector<Word>& words, bool stopAtVerb) {
        std::vector<std::string> result;
        bool trigger = false;
        for (const auto& w : words) {
            if (!trigger && w.getType() == WordType::PREPOSITION) {
                trigger = true;
                continue;
            }
            if (trigger) {
                if (w.getType() == WordType::NOUN || w.getType() == WordType::PRONOUN) {
                    result.push_back(w.getWord());
                } else if (stopAtVerb && w.getType() == WordType::VERB) {
                    break;
                }
            }
        }
        return result;
    }

    // Helper: collect nouns after a verb (objects)
    std::vector<std::string> collectNounsAfterVerb(const std::vector<Word>& words) {
        std::vector<std::string> result;
        bool verbSeen = false;
        bool artSeen = false;
        for (const auto& w : words) {
            if (!verbSeen && w.getType() == WordType::VERB) {
                verbSeen = true;
                continue;
            }
            if (w.getType() == WordType::ARTICLE) artSeen = true;
            if ((verbSeen || artSeen) && w.getType() == WordType::NOUN) {
                result.push_back(w.getWord());
            }
        }
        return result;
    }

} // anonymous namespace

// ============================================================================
// Public detection functions
// ============================================================================
std::optional<CommandType> detectCommandFromPhrase(const std::string& phrase) {
    if (phrase.empty()) return std::nullopt;

    std::istringstream iss(phrase);
    std::string token;
    const auto& reverseMap = getCurrentReverseMap();

    while (iss >> token) {
        // Remove punctuation
        token.erase(std::remove_if(token.begin(), token.end(),
                                   [](unsigned char c) { return std::ispunct(c); }),
                    token.end());
        if (token.empty()) continue;
        // Lowercase
        std::transform(token.begin(), token.end(), token.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        auto it = reverseMap.find(token);
        if (it != reverseMap.end()) {
            // Optional: verify that the word is indeed a verb in the database
            Word w;
            if (WordRepository::load(token, w)) {
                if (w.getType() == WordType::VERB)
                    return it->second;
            } else {
                // If the word is not in DB, still accept the command (heuristic)
                return it->second;
            }
        }
    }
    return std::nullopt;
}

std::vector<std::string> detectSubjects(const std::vector<Word>& words) {
    return collectNounsAfter(words, true);
}

std::vector<std::string> detectObjects(const std::vector<Word>& words) {
    auto objects = collectNounsAfterVerb(words);
    auto subjects = detectSubjects(words);
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [&](const std::string& obj) {
            return std::find(subjects.begin(), subjects.end(), obj) != subjects.end();
        }), objects.end());
    return objects;
}

std::vector<std::string> detectSubjects(const Sentence& sentence) {
    std::vector<std::string> result;
    bool trigger = false;
    Block key = sentence.getKey();
    if (key.type != WordType::VERB) result.push_back(key.text);
    for (const auto& b : sentence.getBlocks()) {
        if (!trigger && b.type == WordType::PREPOSITION) {
            trigger = true;
            continue;
        }
        if (trigger) {
            if (b.type == WordType::NOUN && b.text != key.text) {
                result.push_back(b.text);
            } else if (b.type == WordType::VERB) {
                break;
            }
        }
    }
    return result;
}

std::vector<std::string> detectObjects(const Sentence& sentence) {
    std::vector<std::string> result;
    bool verbSeen = false;
    bool artSeen = false;
    for (const auto& b : sentence.getBlocks()) {
        if (!verbSeen && b.type == WordType::VERB) {
            verbSeen = true;
            continue;
        }
        if (b.type == WordType::ARTICLE) artSeen = true;
        if ((verbSeen || artSeen) && b.type == WordType::NOUN) {
            result.push_back(b.text);
        }
    }
    auto subjects = detectSubjects(sentence);
    result.erase(std::remove_if(result.begin(), result.end(),
        [&](const std::string& obj) {
            return std::find(subjects.begin(), subjects.end(), obj) != subjects.end();
        }), result.end());
    return result;
}
