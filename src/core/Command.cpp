/**
 * @file Command.cpp
 * @brief Implementation of command detection for Spanish and English.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Command.hpp"
#include "../db/WordRepository.hpp"
#include "Word.hpp"
#include "../utils/LearningHelpers.hpp"
#include "../utils/StringUtils.hpp"
#include "../utils/Chunker.hpp"
#include "Sentence.hpp"
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
// Helper: Normalize a verb (lowercase + remove accents)
// ============================================================================
static std::string normalizeVerb(const std::string& token) {
    return StringUtils::toLowerNoAccentsSafe(token);
}

// ============================================================================
// Verb lists (Spanish & English)
// ============================================================================
namespace {

    // Spanish command verbs
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
        { CommandType::CLOSE,    {"cierra", "clausura"} },
        { CommandType::LEARN,    {"aprende", "estudia", "asimila", "memoriza", "aprehende"} },
        { CommandType::EXTRACT,  {"extrae", "saca", "obtén", "obten", "recupera", "extraer", "sacar", "recuperar", "retirar", "arrancar", "tomar", "coger", "agarrar"} },
        { CommandType::RETRIEVE, {"recupera", "rescata", "consigue", "obtén", "obten", "recuperar", "rescatar", "conseguir", "recobrar"} },
        { CommandType::QUERY,    {"consulta", "interroga", "pregunta", "indaga", "averigua", "investiga", "busca"} },
        { CommandType::FETCH,    {"trae", "obtén", "obten", "recupera", "busca", "traer", "buscar"} }
    };

    // English command verbs
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
        { CommandType::CLOSE,    {"close", "shut", "lock"} },
        { CommandType::LEARN,    {"learn", "study", "memorize", "assimilate"} },
        { CommandType::EXTRACT,  {"extract", "pull", "retrieve", "obtain", "get", "fetch", "take", "grab"} },
        { CommandType::RETRIEVE, {"retrieve", "fetch", "get", "obtain", "recover", "regain"} },
        { CommandType::QUERY,    {"query", "ask", "interrogate", "request", "inquire", "search"} },
        { CommandType::FETCH,    {"fetch", "retrieve", "get", "bring", "obtain"} }
    };

    // Create a normalized (lowercase + no accents) -> CommandType map
    std::unordered_map<std::string, CommandType> buildReverseVerbMap(const std::unordered_map<CommandType, std::vector<std::string>>& langMap) {
        std::unordered_map<std::string, CommandType> rev;
        for (const auto& [cmd, verbs] : langMap) {
            for (const auto& v : verbs) {
                std::string key = normalizeVerb(v);
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
        // Normalize: lowercase + remove accents
        std::string normToken = normalizeVerb(token);

        auto it = reverseMap.find(normToken);
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

// Extrae la frase nominal completa que sigue a un verbo de extracción.
// Utiliza el chunker para identificar el chunk nominal después del verbo.
std::string extractNounPhraseAfterVerb(const Sentence& sent, const std::string& commandVerb) {
    const auto& blocks = sent.getBlocks();
    if (blocks.empty()) return "";

    // Buscar la posición del verbo (o la primera palabra del comando)
    size_t verbPos = blocks.size();
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].type == WordType::VERB ||
            StringUtils::toLowerNoAccentsSafe(blocks[i].text) == commandVerb) {
            verbPos = i;
            break;
        }
    }
    if (verbPos == blocks.size() - 1 || verbPos >= blocks.size() - 1) return "";

    // Construir vector de palabras desde los bloques restantes
    std::vector<Word> words;
    for (size_t i = verbPos + 1; i < blocks.size(); ++i) {
        Word w(blocks[i].text);
        w.setType(blocks[i].type);
        words.push_back(w);
    }
    if (words.empty()) return "";

    // Obtener chunks a partir de las palabras
    std::vector<std::string> chunks = Chunker::chunk(words);
    if (chunks.empty()) return "";

    // El primer chunk después del verbo suele ser el objeto directo (frase nominal)
    // Pero puede haber varios chunks (ej. "número de factura" es un solo chunk nominal)
    // Devolvemos el primer chunk completo.
    std::string result = chunks[0];
    // Si el chunk contiene solo un artículo, mirar el siguiente chunk
    if (result.size() <= 3 && (result == "el" || result == "la" || result == "los" || result == "las" ||
                                result == "un" || result == "una" || result == "unos" || result == "unas")) {
        if (chunks.size() > 1) result = chunks[1];
    }
    return result;
}

std::string extractFieldNameFromCommand(const std::string& command, CommandType type) {
    if (command.empty()) return "";

    // Tokenizar y clasificar la oración
    std::vector<Word> words = createWordVector(command);
    Sentence sent(words);

    // Verificar que sea un comando de extracción
    if (type != CommandType::EXTRACT && type != CommandType::FETCH &&
        type != CommandType::RETRIEVE && type != CommandType::QUERY) {
        return "";
    }

    // Obtener la lista de verbos asociados a este tipo de comando
    const auto& verbMap = getCurrentVerbMap();
    auto it = verbMap.find(type);
    if (it == verbMap.end()) return "";
    const std::vector<std::string>& possibleVerbs = it->second;

    // Identificar cuál de esos verbos aparece realmente en la frase
    std::string commandVerb;
    std::istringstream iss(command);
    std::string token;
    while (iss >> token) {
        // Limpiar puntuación y normalizar
        token.erase(std::remove_if(token.begin(), token.end(),
                                   [](unsigned char c) { return std::ispunct(c); }),
                    token.end());
        if (token.empty()) continue;
        std::string normToken = normalizeVerb(token);
        // Verificar si este token está en la lista de verbos posibles
        for (const auto& verb : possibleVerbs) {
            if (normToken == normalizeVerb(verb)) {
                commandVerb = verb; // guardamos la forma original (normalizada sin acentos)
                break;
            }
        }
        if (!commandVerb.empty()) break;
    }

    if (commandVerb.empty()) return "";

    // Extraer la frase nominal después del verbo usando chunking
    std::string field = extractNounPhraseAfterVerb(sent, commandVerb);
    if (field.empty()) {
        // Fallback: intentar con detectObjects (original) pero mejorado
        auto objects = detectObjects(sent);
        if (!objects.empty()) {
            field = objects[0];
            for (size_t i = 1; i < objects.size(); ++i) field += " " + objects[i];
        }
    }
    if (field.empty()) {
        // Último recurso: toda la frase sin el primer verbo
        std::string full = sent.toString();
        size_t firstSpace = full.find(' ');
        if (firstSpace != std::string::npos) field = full.substr(firstSpace + 1);
        else field = full;
    }
    return field;
}
