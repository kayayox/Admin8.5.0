// main.cpp
#include "../extractor/KeyValueExtractor.hpp"
#include "../extractor/OutputFormatter.hpp"
#include "../dialogue/PatternCorrelator.hpp"
#include "../dialogue/ContextualCorrelator.hpp"
#include "../nlp/Morphology.hpp"
#include "../nlp/Tokenizer.hpp"
#include "../core/Command.hpp"
#include "../utils/LearningHelpers.hpp"
#include "../utils/StringConversions.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cctype>
#include <algorithm>
#include <vector>
#include <chrono>
#include <ctime>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================================
// Traducción simple (español ↔ inglés)
// ============================================================================
static std::string _(const std::string& es, const std::string& en) {
    return (getCommandLanguage() == "en") ? en : es;
}

// ============================================================================
// Utilidades de string (trim)
// ============================================================================
static inline std::string& ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return s;
}

static inline std::string& rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

static inline std::string& trim(std::string& s) {
    return ltrim(rtrim(s));
}

static inline std::string trim(const std::string& s) {
    std::string copy = s;
    return trim(copy);
}

// ============================================================================
// Impresión y formato (bilingüe)
// ============================================================================
void printHeader(const std::string& titleEs, const std::string& titleEn) {
    std::string title = _(titleEs, titleEn);
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ " << std::left << std::setw(60) << title << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
}

void printTable(const std::unordered_map<std::string, std::string>& data) {
    if (data.empty()) {
        std::cout << _("  (sin datos extraídos)\n", "  (no data extracted)\n");
        return;
    }
    std::cout << std::left << std::setw(30) << _("CAMPO", "FIELD") << _("VALOR", "VALUE") << "\n";
    std::cout << std::string(70, '-') << "\n";
    for (const auto& [key, value] : data) {
        std::cout << std::setw(30) << key << value << "\n";
    }
}

void printHelp() {
    if (getCommandLanguage() == "en") {
        std::cout << R"(
COMMANDS:

  EXTRACTION:
    extract <field>                - Extract a field from a document (e.g. 'extract the due date')
    extract-multi                  - Extract multiple fields using loaded specifications

  SPECIFICATION MANAGEMENT:
    list specs                     - Show active specifications
    add spec                       - Add a specification manually (interactive)
    remove spec <index>            - Remove a specification by index
    load specs <file.json>         - Load specifications from JSON
    save specs <file.json>         - Save current specifications
    clear specs                    - Delete all specifications

  FEEDBACK & LEARNING:
    feedback <field> <yes/no>      - Record manual feedback for the last field
    learn                          - Show learning statistics (from memory)

  OTHER:
    help                           - Show this help
    change lang "es"/"en"          - Change language
    exit / quit                    - Exit the program
)";
    } else {
        std::cout << R"(
COMANDOS DISPONIBLES:

  EXTRACCIÓN:
    extrae <campo>                 - Extrae un campo de un documento (ej: 'extrae la fecha de vencimiento')
    extrae-multi                   - Extrae múltiples campos usando especificaciones cargadas

  GESTIÓN DE ESPECIFICACIONES:
    list specs                     - Muestra las especificaciones activas
    add spec                       - Añade una especificación manualmente (interactivo)
    remove spec <índice>           - Elimina una especificación por índice
    load specs <archivo.json>      - Carga especificaciones desde JSON
    save specs <archivo.json>      - Guarda especificaciones actuales
    clear specs                    - Borra todas las especificaciones

  FEEDBACK Y APRENDIZAJE:
    feedback <campo> <sí/no>       - Registra feedback manual (sí/no) para el último campo
    aprender                       - Muestra estadísticas de aprendizaje (desde memoria)

  OTROS:
    help                           - Muestra esta ayuda
    change lang "es"/"en"          - Cambia el idioma
    exit / salir                   - Termina el programa
)";
    }
}

// ============================================================================
// Funciones interactivas (bilingües)
// ============================================================================
void addSpecificationInteractively(KeyValueExtractor& extractor) {
    ExtractionSpec spec;
    std::cout << _("Nombre del campo: ", "Field name: ");
    std::getline(std::cin, spec.fieldName);
    if (spec.fieldName.empty()) {
        std::cout << _("Cancelado.\n", "Cancelled.\n");
        return;
    }

    std::cout << _("Tipo esperado (TEXT/DATE/NUMBER/EMAIL/MONEY/PHONE): ", "Expected type (TEXT/DATE/NUMBER/EMAIL/MONEY/PHONE): ");
    std::string typeStr;
    std::getline(std::cin, typeStr);
    std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::toupper);
    if (typeStr == "DATE") spec.expectedType = TokenType::DATE;
    else if (typeStr == "NUMBER") spec.expectedType = TokenType::NUMBER;
    else if (typeStr == "EMAIL") spec.expectedType = TokenType::EMAIL;
    else if (typeStr == "MONEY") spec.expectedType = TokenType::MONEY;
    else if (typeStr == "PHONE") spec.expectedType = TokenType::PHONE;
    else spec.expectedType = TokenType::WORD;

    std::cout << _("Palabras clave (separadas por coma): ", "Keywords (comma separated): ");
    std::string kwLine;
    std::getline(std::cin, kwLine);
    std::stringstream ss(kwLine);
    std::string kw;
    while (std::getline(ss, kw, ',')) {
        kw = trim(kw);
        if (!kw.empty()) spec.keywords.push_back(kw);
    }

    std::cout << _("¿Requerido? (s/n): ", "Required? (y/n): ");
    char req;
    std::cin >> req;
    std::cin.ignore();
    spec.required = (req == 's' || req == 'S' || req == 'y' || req == 'Y');

    std::cout << _("¿Múltiple? (s/n): ", "Multiple? (y/n): ");
    char mult;
    std::cin >> mult;
    std::cin.ignore();
    spec.multiple = (mult == 's' || mult == 'S' || mult == 'y' || mult == 'Y');

    std::cout << _("Rol semántico (SUBJECT/OBJECT/VERB/TIME/AMOUNT/NONE): ", "Semantic role (SUBJECT/OBJECT/VERB/TIME/AMOUNT/NONE): ");
    std::string roleStr;
    std::getline(std::cin, roleStr);
    std::transform(roleStr.begin(), roleStr.end(), roleStr.begin(), ::toupper);
    if (roleStr == "SUBJECT") spec.role = SemanticRole::SUBJECT;
    else if (roleStr == "OBJECT") spec.role = SemanticRole::OBJECT;
    else if (roleStr == "VERB") spec.role = SemanticRole::VERB;
    else if (roleStr == "TIME") spec.role = SemanticRole::TIME;
    else if (roleStr == "AMOUNT") spec.role = SemanticRole::AMOUNT;
    else spec.role = SemanticRole::NONE;

    extractor.addSpecification(spec);
    std::cout << _("✓ Especificación añadida.\n", "✓ Specification added.\n");
}

void showMemoryStats(KeyValueExtractor& /*extractor*/) {
    if (getCommandLanguage() == "en") {
        std::cout << "\nLEARNING STATUS:\n";
        std::cout << "  - The system learns automatically when you provide feedback (yes/no).\n";
        std::cout << "  - Trigger phrases and synonyms are accumulated in the database.\n";
        std::cout << "  - To see concrete results, inspect the pattern database.\n";
        std::cout << "  - You can force new feedback with the 'feedback' command.\n";
    } else {
        std::cout << "\nESTADO DEL APRENDIZAJE:\n";
        std::cout << "  - El sistema aprende automáticamente cuando proporcionas feedback (sí/no).\n";
        std::cout << "  - Las frases disparadoras y sinónimos se acumulan en la base de datos.\n";
        std::cout << "  - Para ver resultados concretos, inspecciona la base de patrones.\n";
        std::cout << "  - Puedes forzar nuevo feedback con el comando 'feedback'.\n";
    }
}

void registerManualFeedback(KeyValueExtractor& extractor, const std::string& fieldName,
                            const std::string& correctValue, NLPProcessor* nlp = nullptr) {
    ExtractionRequest req;
    req.fieldName = fieldName;
    req.expectedType = TokenType::WORD;
    req.originalCommand = (getCommandLanguage() == "en" ? "extract " : "extrae ") + fieldName;

    if (nlp) {
        req.keywords.push_back(fieldName);
    }

    if (!correctValue.empty()) {
        req.correctedValue = correctValue;
        extractor.provideFeedback(req, false);
        std::cout << _("✓ Feedback registrado: '", "✓ Feedback recorded: '") << fieldName
                  << _("' se corrigió a '", "' was corrected to '") << correctValue << "'\n";
    } else {
        extractor.provideFeedback(req, true);
        std::cout << _("✓ Feedback registrado: '", "✓ Feedback recorded: '") << fieldName
                  << _("' fue correcto.\n", "' was correct.\n");
    }
}

void showReq(const ExtractionRequest& req){
    std::cout << _("Tipo de token esperado: ", "Expected token type: ") << tokenTypeToString(req.expectedType) << "\n";
    std::cout << _("Tipo de comando dado: ", "Command type given: ") << commandTypeToString(req.commandType) << "\n";
    std::cout << _("Tipo de campo esperado: ", "Expected field: ") << req.fieldName << "\n";
    std::cout << _("Palabras clave: ", "Keywords: ") << "\n";
    for(auto w : req.keywords) std::cout << w << ",";
    std::cout << "\n";
}

// ============================================================================
// Extracción principal
// ============================================================================
void performExtraction(KeyValueExtractor& extractor, const std::string& userCommand) {
    ExtractionRequest req;
    if (!extractor.parseCommand(userCommand, req)) {
        std::cout << _("No entendí el comando. Usa 'extrae <campo>'.\n", "I didn't understand the command. Use 'extract <field>'.\n");
        return;
    }

    if (req.fieldName.empty()) {
        std::cout << _("No se pudo determinar el campo a extraer.\n", "Could not determine the field to extract.\n");
        return;
    }

    if (req.documentPath.empty()) {
        std::cout << _("Ruta del documento (txt, pdf, imagen): ", "Document path (txt, pdf, image): ");
        std::getline(std::cin, req.documentPath);
    }
    if (req.outputFormat.empty()) {
        std::cout << _("Formato de salida (json, csv, xml, sqlite): ", "Output format (json, csv, xml, sqlite): ");
        std::getline(std::cin, req.outputFormat);
        if (req.outputFormat.empty()) req.outputFormat = "json";
    }
    if (req.outputPath.empty()) {
        std::string ext = (req.outputFormat == "sqlite") ? ".db" : "." + req.outputFormat;
        req.outputPath = "extraccion_" + formatTimestamp(std::chrono::system_clock::now().time_since_epoch().count()) + ext;
    }

    std::unordered_map<std::string, std::string> result;
    std::vector<std::string> errors;
    extractor.clearErrors();
    bool success = extractor.executeExtraction(req, result, errors);

    if (success) {
        printHeader("RESULTADO DE EXTRACCIÓN", "EXTRACTION RESULT");
        printTable(result);

        std::string docName = req.documentPath;
        size_t slash = docName.find_last_of("/\\");
        if (slash != std::string::npos) docName = docName.substr(slash + 1);

        if (req.outputFormat == "json") {
            OutputFormatter::saveToJSON(result, req.outputPath, docName);
            std::cout << _("Guardado JSON en: ", "JSON saved to: ") << req.outputPath << "\n";
        } else if (req.outputFormat == "csv") {
            OutputFormatter::saveToCSV(result, req.outputPath);
            std::cout << _("Guardado CSV en: ", "CSV saved to: ") << req.outputPath << "\n";
        } else if (req.outputFormat == "xml") {
            OutputFormatter::saveToXML(result, req.outputPath);
            std::cout << _("Guardado XML en: ", "XML saved to: ") << req.outputPath << "\n";
        } else if (req.outputFormat == "sqlite") {
            OutputFormatter::saveToSQLite(result, req.outputPath, docName);
            std::cout << _("Guardado SQLite en: ", "SQLite saved to: ") << req.outputPath << "\n";
        } else {
            std::cout << _("Formato '", "Format '") << req.outputFormat << _("' no soportado.\n", "' not supported.\n");
        }

        std::cout << _("\n¿Es correcta la extracción? (s/n): ", "\nIs the extraction correct? (y/n): ");
        char fb;
        std::cin >> fb;
        std::cin.ignore();
        if (fb == 's' || fb == 'S' || fb == 'y' || fb == 'Y') {
            extractor.provideFeedback(req, true);
            std::cout << _("¡Gracias! El sistema ha aprendido de esta extracción.\n", "Thank you! The system has learned from this extraction.\n");
        } else {
            std::cout << _("Introduce el valor correcto para '", "Enter the correct value for '")
                      << req.fieldName << "': ";
            std::string correct;
            std::getline(std::cin, correct);
            req.correctedValue = correct;
            extractor.provideFeedback(req, false);
            std::cout << _("Corrección registrada. El sistema mejorará en el futuro.\n", "Correction recorded. The system will improve in the future.\n");
        }
    } else {
        std::cout << _("Error en la extracción:\n", "Extraction error:\n");
        for (const auto& e : errors)
            std::cout << "  - " << e << "\n";
    }
}

void performMultiFieldExtraction(KeyValueExtractor& extractor) {
    const auto& specs = extractor.getSpecifications();
    if (specs.empty()) {
        std::cout << _("No hay especificaciones cargadas. Usa 'load specs' o 'add spec' primero.\n",
                      "No specifications loaded. Use 'load specs' or 'add spec' first.\n");
        return;
    }

    ExtractionRequest req;
    req.fieldName = "multiple";
    req.commandType = CommandType::EXTRACT;
    req.feedbackRequested = true;

    std::cout << _("Ruta del documento: ", "Document path: ");
    std::getline(std::cin, req.documentPath);
    std::cout << _("Formato de salida (json, csv, xml, sqlite): ", "Output format (json, csv, xml, sqlite): ");
    std::getline(std::cin, req.outputFormat);
    if (req.outputFormat.empty()) req.outputFormat = "json";
    req.outputPath = "multi_extraccion." + (req.outputFormat == "sqlite" ? "db" : req.outputFormat);

    std::unordered_map<std::string, std::string> result;
    std::vector<std::string> errors;
    extractor.clearErrors();
    bool success = extractor.executeExtraction(req, result, errors);

    if (success) {
        printHeader("EXTRACCIÓN MULTI-CAMPO", "MULTI-FIELD EXTRACTION");
        printTable(result);

        std::string docName = req.documentPath;
        size_t slash = docName.find_last_of("/\\");
        if (slash != std::string::npos) docName = docName.substr(slash + 1);

        if (req.outputFormat == "json") {
            OutputFormatter::saveToJSON(result, req.outputPath, docName);
        } else if (req.outputFormat == "csv") {
            OutputFormatter::saveToCSV(result, req.outputPath);
        } else if (req.outputFormat == "xml") {
            OutputFormatter::saveToXML(result, req.outputPath);
        } else if (req.outputFormat == "sqlite") {
            OutputFormatter::saveToSQLite(result, req.outputPath, docName);
        } else {
            std::cout << _("Formato no soportado.\n", "Format not supported.\n");
            return;
        }
        std::cout << _("Guardado en ", "Saved to ") << req.outputPath << "\n";

        std::cout << _("\n¿Los datos extraídos son correctos? (s/n): ", "\nAre the extracted data correct? (y/n): ");
        char fb;
        std::cin >> fb;
        std::cin.ignore();
        if (fb == 's' || fb == 'S' || fb == 'y' || fb == 'Y') {
            std::cout << _("Feedback positivo registrado (mejora el aprendizaje).\n", "Positive feedback recorded (improves learning).\n");
        } else {
            std::cout << _("Puedes corregir campos individualmente con el comando 'feedback'.\n", "You can correct individual fields with the 'feedback' command.\n");
        }
    } else {
        std::cout << _("Errores:\n", "Errors:\n");
        for (const auto& e : errors) std::cout << "  - " << e << "\n";
    }
}

// ============================================================================
// Extractor main
// ============================================================================
int extractor(std::string semanticDb, std::string patternDb, std::string language) {
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     EXTRACTOR INTELIGENTE CLAVE-VALOR v2.1 - MODO COMPLETO   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    std::cout << "\n" << _("BD semántica: ", "Semantic DB: ") << semanticDb << "\n";
    std::cout << _("BD patrones: ", "Pattern DB: ") << patternDb << "\n";
    std::cout << _("Idioma: ", "Language: ") << language << "\n";

    KeyValueExtractor extractor(semanticDb, patternDb, language);
    PatternCorrelator patternCorr(patternDb);
    ContextualCorrelator contextualCorrelator(patternDb);

    if (std::ifstream("src/utils/rules1.json").good()) {
        if (extractor.loadSpecificationsFromJSON("src/utils/rules1.json"))
            std::cout << _("Especificaciones cargadas desde src/utils/rules1.json\n", "Specifications loaded from config/rules.json\n");
    }
    printHelp();

    bool running = true;
    std::string line;

    while (running) {
        std::cout << "\n> ";
        std::getline(std::cin, line);
        if (line.empty()) continue;
        std::string cmd = line;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        // Comandos de salida
        if (cmd == "exit" || cmd == "salir" || cmd == "quit") {
            running = false;
        }
        else if (cmd == "help") {
            printHelp();
        }
        else if (cmd == "list specs") {
            const auto& specs = extractor.getSpecifications();
            if (specs.empty()) {
                std::cout << _("  No hay especificaciones.\n", "  No specifications.\n");
            } else {
                std::cout << _("Especificaciones activas (", "Active specifications (") << specs.size() << "):\n";
                for (size_t i = 0; i < specs.size(); ++i) {
                    std::cout << "  [" << i << "] " << specs[i].fieldName;
                    if (specs[i].required) std::cout << _(" (requerido)", " (required)");
                    if (specs[i].multiple) std::cout << _(" (múltiple)", " (multiple)");
                    std::cout << "\n";
                }
            }
        }
        else if (cmd == "add spec") {
            addSpecificationInteractively(extractor);
        }
        else if (cmd.rfind("remove spec ", 0) == 0) {
            std::string idxStr = cmd.substr(12);
            int idx = std::stoi(idxStr);
            const auto& specs = extractor.getSpecifications();
            if (idx >= 0 && idx < (int)specs.size()) {
                extractor.clearSpecifications();
                for (int i = 0; i < (int)specs.size(); ++i) {
                    if (i != idx) extractor.addSpecification(specs[i]);
                }
                std::cout << _("Especificación eliminada.\n", "Specification removed.\n");
            } else {
                std::cout << _("Índice inválido.\n", "Invalid index.\n");
            }
        }
        else if (cmd.rfind("load specs ", 0) == 0) {
            std::string path = line.substr(11);
            if (extractor.loadSpecificationsFromJSON(path))
                std::cout << _("Cargadas desde ", "Loaded from ") << path << "\n";
            else
                std::cout << _("Error al cargar JSON.\n", "Error loading JSON.\n");
        }
        else if (cmd.rfind("save specs ", 0) == 0) {
            std::string path = line.substr(11);
            if (extractor.saveSpecificationsToJSON(path))
                std::cout << _("Guardadas en ", "Saved to ") << path << "\n";
            else
                std::cout << _("Error al guardar.\n", "Error saving.\n");
        }
        else if (cmd == "clear specs") {
            extractor.clearSpecifications();
            std::cout << _("Especificaciones eliminadas.\n", "Specifications cleared.\n");
        }
        else if (cmd == "aprender" || cmd == "learn") {
            showMemoryStats(extractor);
        }
        else if (cmd.rfind("feedback ", 0) == 0) {
            std::string rest = line.substr(9);
            std::stringstream ss(rest);
            std::string field, action;
            ss >> field >> action;
            if (field.empty()) {
                std::cout << _("Uso: feedback <campo> [si|no] [valor_corregido]\n", "Usage: feedback <field> [yes|no] [corrected_value]\n");
                continue;
            }
            if (action.empty() || action == "si" || action == "s" || action == "yes" || action == "y") {
                registerManualFeedback(extractor, field, "", nullptr);
            } else if (action == "no" || action == "n") {
                std::string correctVal;
                std::getline(ss, correctVal);
                correctVal = trim(correctVal);
                if (correctVal.empty()) {
                    std::cout << _("Proporciona el valor correcto: ", "Provide the correct value: ");
                    std::getline(std::cin, correctVal);
                }
                registerManualFeedback(extractor, field, correctVal, nullptr);
            } else {
                std::cout << _("Acción no reconocida. Usa 'si' o 'no'.\n", "Action not recognized. Use 'yes' or 'no'.\n");
            }
        }
        else if (cmd.rfind("change lang ", 0) == 0) {
            std::string lang = cmd.substr(12);
            if (lang == "es" || lang == "en") {
                extractor.setLanguage(lang);
                setCommandLanguage(lang);
                std::cout << _("Idioma cambiado a español.\n", "Language changed to English.\n");
            } else {
                std::cout << _("Idioma no soportado. Usa 'es' o 'en'.\n", "Language not supported. Use 'es' or 'en'.\n");
            }
        }
        else if (cmd == "extrae-multi" || cmd == "extract-multi") {
            performMultiFieldExtraction(extractor);
        }
        // Comandos de extracción: español e inglés
        else if (cmd.rfind("extrae ", 0) == 0 || cmd.rfind("obten ", 0) == 0 ||
                 cmd.rfind("saca ", 0) == 0 || cmd.rfind("recupera ", 0) == 0 ||
                 cmd.rfind("rescata ", 0) == 0 || cmd.rfind("consigue ", 0) == 0 ||
                 cmd.rfind("consulta ", 0) == 0 || cmd.rfind("trae ", 0) == 0 ||
                 cmd.rfind("busca ", 0) == 0 ||
                 cmd.rfind("extract ", 0) == 0 || cmd.rfind("get ", 0) == 0 ||
                 cmd.rfind("retrieve ", 0) == 0 || cmd.rfind("fetch ", 0) == 0 ||
                 cmd.rfind("query ", 0) == 0 || cmd.rfind("find ", 0) == 0 ||
                 cmd.rfind("obtain ", 0) == 0 || cmd.rfind("pull ", 0) == 0 ||
                 cmd.rfind("bring ", 0) == 0 || cmd.rfind("search ", 0) == 0 ||
                 cmd.rfind("recover ", 0) == 0) {
            performExtraction(extractor, line);
        }
        else {
            std::cout << _("Comando no reconocido. Escribe 'help' para ayuda.\n", "Command not recognized. Type 'help' for help.\n");
        }
    }

    std::cout << _("\n¡Hasta luego!\n", "\nGoodbye!\n");
    return 0;
}
