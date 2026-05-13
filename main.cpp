/**
 * @file main.cpp
 * @brief Interactive demo of the NLP engine (bilingual: Spanish/English).
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "src/api/NLPEngine.hpp"
#include "src/nlp/Tokenizer.hpp"          // splitIntoSentences
#include "src/core/Command.hpp"            // detectCommandFromPhrase
#include "src/utils/StringConversions.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <cctype>
#include <random>
#include <string>
#include <map>

#define MAX_ITER 16

// ----------------------------------------------------------------------------
// Bilingual text support
// ----------------------------------------------------------------------------
static std::string currentLang = "es";   // default Spanish

// Simple translation map for common strings
static std::string tr(const std::string& key) {
    if (currentLang == "en") {
        static const std::map<std::string, std::string> en = {
            {"yes_no", "(y/n)"},
            {"yes", "y"},
            {"no", "n"},
            // Menu
            {"menu_title", "=== MAIN MENU ==="},
            {"opt_learn_file", "1. Learn from file"},
            {"opt_learn_phrase", "2. Learn a phrase"},
            {"opt_process", "3. Process a sentence (classify + learn)"},
            {"opt_word_info", "4. Show word information"},
            {"opt_manual_correct", "5. Manually correct a word"},
            {"opt_reprocess", "6. Reprocess last sentence (after correction)"},
            {"opt_predict_loop", "7. Interactive prediction loop"},
            {"opt_generate", "8. Generate response to a premise"},
            {"opt_switch_lang", "9. Switch language (currently: "},
            {"opt_exit", "0. Exit"},
            // Common messages
            {"enter_option", "Option: "},
            {"invalid_option", "Invalid option."},
            {"exiting", "Exiting..."},
            {"file_name", "File name (e.g. corpus.txt): "},
            {"no_file", "No file specified."},
            {"phrase_to_learn", "Phrase to learn: "},
            {"command_detected", "Detected command ID: "},
            {"no_command", "No command detected."},
            {"learned_ok", "Phrase learned."},
            {"sentence_to_process", "Sentence to process: "},
            {"classification", "Classification:"},
            {"word", "Word: "},
            {"word_type", "  Type: "},
            {"confidence", " (confidence: "},
            {"meaning", "  Meaning: "},
            {"quantity", "  Quantity: "},
            {"tense", ", Tense: "},
            {"gender", ", Gender: "},
            {"person", ", Person: "},
            {"degree", ", Degree: "},
            {"related", "  Related:"},
            {"no_predictions", "  No predictions available."},
            {"predictions", "  Predictions:"},
            {"probability", " (probability: "},
            {"predict_mode", "--- PREDICTION MODE ---"},
            {"initial_phrase", "Initial phrase (at least 2 words): "},
            {"iteration", "=== Iteration "},
            {"low_confidence", "Low confidence. Options:"},
            {"choose_manual", "Choose number (1-"},
            {"or_manual", ") or 0 for manual: "},
            {"correct_word", "Correct word: "},
            {"is_prediction_correct", "Is the prediction '"},
            {"correct", "' correct?"},
            {"learned_sequence", "Learned: "},
            {"final_generated", "Final generated phrase: "},
            {"premise", "Premise (input sentence): "},
            {"generated_response", "Generated response: "},
            {"response_useful", "Was the last response correct or useful?"},
            {"feedback_recorded", "Feedback recorded."},
            {"switch_lang_prompt", "Enter language (es/en): "},
            {"lang_set", "Language set to "},
            {"invalid_lang", "Invalid language. Use 'es' or 'en'."},
            {"warning_file_open", "Warning: Unable to open file '"},
            {"learned_sentence", "Learned sentence "},
            {"total_learned", "Total "},
            {"sentences_learned_from", " sentences learned from '"},
            {"word_info", "Word information"},
            {"type_correct", "Correct type (Noun, Verb, Adjective, Adverb, Preposition, Conjunction, Article, Pronoun): "},
            {"correction_saved", "Correction saved. Use option 6 to reprocess last sentence."},
            {"reprocessed", "Last sentence reprocessed (statistics updated)."},
            {"no_predictions_break", "No predictions. Ending loop."},
            {"generating_phrase", "Generating phrase: "},
            {"premise_given", "Premise given: "},
            {"hypothesis_generated", "Hypothesis generated: "}
        };
        auto it = en.find(key);
        if (it != en.end()) return it->second;
        return key;  // fallback
    } else { // Spanish
        static const std::map<std::string, std::string> es = {
            {"yes_no", "(s/n)"},
            {"yes", "s"},
            {"no", "n"},
            {"menu_title", "=== MENÚ PRINCIPAL ==="},
            {"opt_learn_file", "1. Aprender de archivo"},
            {"opt_learn_phrase", "2. Aprender una frase"},
            {"opt_process", "3. Procesar una oración (clasificar + aprender)"},
            {"opt_word_info", "4. Mostrar información de una palabra"},
            {"opt_manual_correct", "5. Corregir manualmente una palabra"},
            {"opt_reprocess", "6. Reprocesar última oración (tras corrección)"},
            {"opt_predict_loop", "7. Predicción interactiva (bucle)"},
            {"opt_generate", "8. Generar respuesta a una premisa"},
            {"opt_switch_lang", "9. Cambiar idioma (actualmente: "},
            {"opt_exit", "0. Salir"},
            {"enter_option", "Opción: "},
            {"invalid_option", "Opción no válida."},
            {"exiting", "Saliendo..."},
            {"file_name", "Nombre del archivo (ej: corpus.txt): "},
            {"no_file", "No se especificó archivo."},
            {"phrase_to_learn", "Frase para aprender: "},
            {"command_detected", "Comando detectado: "},
            {"no_command", "Sin comando detectable."},
            {"learned_ok", "Frase aprendida."},
            {"sentence_to_process", "Oración a procesar: "},
            {"classification", "Clasificación:"},
            {"word", "Palabra: "},
            {"word_type", "  Tipo: "},
            {"confidence", " (confianza: "},
            {"meaning", "  Significado: "},
            {"quantity", "  Cantidad: "},
            {"tense", ", Tiempo: "},
            {"gender", ", Género: "},
            {"person", ", Persona: "},
            {"degree", ", Grado: "},
            {"related", "  Relacionadas:"},
            {"no_predictions", "  No hay predicciones disponibles."},
            {"predictions", "  Predicciones:"},
            {"probability", " (probabilidad: "},
            {"predict_mode", "--- MODO PREDICCIÓN ---"},
            {"initial_phrase", "Frase inicial (mínimo 2 palabras): "},
            {"iteration", "=== Iteración "},
            {"low_confidence", "Confianza baja. Opciones:"},
            {"choose_manual", "Elija número (1-"},
            {"or_manual", ") o 0 para ingresar manual: "},
            {"correct_word", "Palabra correcta: "},
            {"is_prediction_correct", "¿Es correcta la predicción '"},
            {"correct", "'?"},
            {"learned_sequence", "Aprendida: "},
            {"final_generated", "Frase final generada: "},
            {"premise", "Premisa (oración de entrada): "},
            {"generated_response", "Respuesta generada: "},
            {"response_useful", "¿La última respuesta fue correcta o útil?"},
            {"feedback_recorded", "Feedback registrado."},
            {"switch_lang_prompt", "Introduce idioma (es/en): "},
            {"lang_set", "Idioma configurado a "},
            {"invalid_lang", "Idioma no válido. Use 'es' o 'en'."},
            {"warning_file_open", "Advertencia: No se pudo abrir el archivo '"},
            {"learned_sentence", "Aprendida oración "},
            {"total_learned", "Total de "},
            {"sentences_learned_from", " oraciones aprendidas desde '"},
            {"word_info", "Información de palabra"},
            {"type_correct", "Tipo correcto (Sustantivo, Verbo, Adjetivo, Adverbio, Preposición, Conjunción, Artículo, Pronombre): "},
            {"correction_saved", "Corrección guardada. Use opción 6 para reprocesar la última oración."},
            {"reprocessed", "Última oración reprocesada (estadísticas actualizadas)."},
            {"no_predictions_break", "No hay predicciones. Fin del bucle."},
            {"generating_phrase", "Generando frase: "},
            {"premise_given", "Premisa dada: "},
            {"hypothesis_generated", "Hipótesis generada: "}
        };
        auto it = es.find(key);
        if (it != es.end()) return it->second;
        return key;
    }
}

bool askYesNo(const std::string& baseQuestion) {
    std::string answer;
    while (true) {
        std::cout << baseQuestion << " " << tr("yes_no") << ": ";
        std::getline(std::cin, answer);
        if (answer.empty()) continue;
        char first = std::tolower(answer[0]);
        char expectedYes = (currentLang == "en") ? 'y' : 's';
        char expectedNo  = (currentLang == "en") ? 'n' : 'n';
        if (first == expectedYes) return true;
        if (first == expectedNo) return false;
        std::cout << (currentLang == "en" ? "Invalid answer. Please answer 'y' or 'n'.\n"
                                          : "Respuesta no válida. Por favor responda 's' o 'n'.\n");
    }
}

// ----------------------------------------------------------------------------
// Helper functions (bilingual)
// ----------------------------------------------------------------------------
void printWordInfo(const WordInfo& info) {
    std::cout << tr("word") << info.word << "\n"
              << tr("word_type") << info.type << tr("confidence") << info.confidence << ")\n"
              << tr("meaning") << info.meaning << "\n"
              << tr("quantity") << info.quantity << tr("tense") << info.tense
              << tr("gender") << info.gender << tr("person") << info.person
              << tr("degree") << info.degree << "\n";
    if (!info.relatedWords.empty()) {
        std::cout << tr("related");
        for (const auto& rel : info.relatedWords)
            std::cout << " " << rel.first << "(" << rel.second << ")";
        std::cout << "\n";
    }
}

void printPredictions(const std::vector<Prediction>& preds) {
    if (preds.empty()) {
        std::cout << tr("no_predictions") << "\n";
        return;
    }
    std::cout << tr("predictions") << "\n";
    for (size_t i = 0; i < preds.size(); ++i) {
        std::cout << "    " << i + 1 << ". '" << preds[i].word
                  << "' " << tr("probability") << preds[i].probability << ")\n";
    }
}

bool learnFromFile(NLPEngine& engine, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << tr("warning_file_open") << filename << "'.\n";
        return false;
    }
    std::string content;
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    file.close();

    auto sentences = splitIntoSentences(content);
    int count = 0;
    for (auto& sent : sentences) {
        if (sent.empty()) continue;
        engine.learnText(sent);
        engine.processSentence(sent);
        count++;
        std::cout << tr("learned_sentence") << count << ": \"" << sent << "\"\n";
    }
    std::cout << tr("total_learned") << count << tr("sentences_learned_from") << filename << "'.\n";
    return true;
}

// ----------------------------------------------------------------------------
// Main program
// ----------------------------------------------------------------------------
int main() {
    NLPEngine engine;
    std::string semanticDb = "nlp_semantic.db";
    std::string patternDb  = "nlp_patterns.db";
    std::string temporalDb = ":memory:";

    if (!engine.initialize(semanticDb, patternDb, temporalDb)) {
        std::cerr << (currentLang == "en" ? "Fatal error: Could not initialise NLP engine.\n"
                                          : "Error fatal: No se pudo inicializar el motor NLP.\n");
        return 1;
    }
    std::cout << (currentLang == "en" ? "NLP engine initialised successfully.\n"
                                      : "Motor NLP inicializado correctamente.\n");

    // Set default language (Spanish)
    engine.setLanguage("es");
    currentLang = "es";

    int option = -1;
    do {
        std::cout << "\n" << tr("menu_title") << "\n"
                  << tr("opt_learn_file") << "\n"
                  << tr("opt_learn_phrase") << "\n"
                  << tr("opt_process") << "\n"
                  << tr("opt_word_info") << "\n"
                  << tr("opt_manual_correct") << "\n"
                  << tr("opt_reprocess") << "\n"
                  << tr("opt_predict_loop") << "\n"
                  << tr("opt_generate") << "\n"
                  << tr("opt_switch_lang") << currentLang << ")\n"
                  << tr("opt_exit") << "\n"
                  << tr("enter_option");
        std::cin >> option;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (option) {
            case 1: { // Learn from file
                std::cout << tr("file_name");
                std::string file;
                std::getline(std::cin, file);
                if (!file.empty()) learnFromFile(engine, file);
                else std::cout << tr("no_file") << "\n";
                break;
            }
            case 2: { // Learn a phrase
                std::cout << tr("phrase_to_learn");
                std::string phrase;
                std::getline(std::cin, phrase);
                if (!phrase.empty()) {
                    engine.learnText(phrase);
                    engine.processSentence(phrase);
                    auto cmd = detectCommandFromPhrase(phrase);
                    if (cmd.has_value())
                        std::cout << tr("command_detected") << static_cast<int>(cmd.value()) << std::endl;
                    else
                        std::cout << tr("no_command") << std::endl;
                    std::cout << tr("learned_ok") << "\n";
                }
                break;
            }
            case 3: { // Process sentence
                std::cout << tr("sentence_to_process");
                std::string sentence;
                std::getline(std::cin, sentence);
                if (!sentence.empty()) {
                    auto infos = engine.processSentence(sentence);
                    std::cout << tr("classification") << "\n";
                    for (const auto& info : infos) printWordInfo(info);
                }
                break;
            }
            case 4: { // Word info
                std::cout << tr("word");
                std::string word;
                std::getline(std::cin, word);
                if (!word.empty()) {
                    WordInfo info = engine.getWordInfo(word);
                    printWordInfo(info);
                }
                break;
            }
            case 5: { // Manually correct a word
                std::cout << tr("word");
                std::string word;
                std::getline(std::cin, word);
                std::cout << tr("type_correct");
                std::string type;
                std::getline(std::cin, type);
                if (!word.empty() && !type.empty()) {
                    engine.correctWord(word, type);
                    std::cout << tr("correction_saved") << "\n";
                }
                break;
            }
            case 6: { // Reprocess last sentence
                engine.reprocessLastSentence();
                std::cout << tr("reprocessed") << "\n";
                break;
            }
            case 7: { // Interactive prediction loop
                std::cout << tr("predict_mode") << "\n";
                std::cout << tr("initial_phrase");
                std::string input;
                std::getline(std::cin, input);
                std::string currentPhrase = input;
                for (int iter = 1; iter <= MAX_ITER; ++iter) {
                    std::cout << "\n" << tr("iteration") << iter << " ===\n";
                    bool type = false;
                    auto preds = engine.predictNext(currentPhrase, type);
                    printPredictions(preds);

                    if (preds.empty()) {
                        std::cout << tr("no_predictions_break") << "\n";
                        break;
                    }

                    std::string predicted = preds[0].word;
                    double bestProb = preds[0].probability;

                    if (bestProb < 0.5 && preds.size() > 1) {
                        std::cout << tr("low_confidence") << "\n";
                        for (size_t i = 0; i < preds.size() && i < 15; ++i)
                            std::cout << "   " << i+1 << ": " << preds[i].word << " (" << preds[i].probability << ")\n";
                        std::cout << tr("choose_manual") << std::min(15, (int)preds.size()) << tr("or_manual");
                        int choice;
                        std::cin >> choice;
                        std::cin.ignore();
                        if (choice >= 1 && choice <= (int)preds.size())
                            predicted = preds[choice-1].word;
                        else if (choice == 0) {
                            std::cout << tr("correct_word");
                            std::getline(std::cin, predicted);
                        }
                    } else {
                        if (preds.size() != 1) {
                            if (!askYesNo(tr("is_prediction_correct") + predicted + tr("correct"))) {
                                std::cout << tr("correct_word");
                                std::getline(std::cin, predicted);
                            }
                        }
                    }
                    std::cout<<type<<"\n";

                    // Learn the correct sequence
                    std::string correctedSentence;
                    if(!type){
                        correctedSentence = currentPhrase + " " + predicted;
                    }else{
                        correctedSentence = currentPhrase + predicted;
                    }
                    engine.processSentence(correctedSentence);
                    std::cout << tr("learned_sequence") << correctedSentence << "\n";
                    if(!type){
                        currentPhrase += " " + predicted;
                    }else{
                        currentPhrase += predicted;
                    }
                    WordInfo info = engine.getWordInfo(predicted);
                    printWordInfo(info);

                    // Optional correction of classification
                    /*if (info.confidence < 0.85f && askYesNo((currentLang == "en" ? "Correct classification of this word?" : "¿Corregir la clasificación de esta palabra?"))) {
                        std::cout << (currentLang == "en" ? "New type: " : "Nuevo tipo: ");
                        std::string newType;
                        std::getline(std::cin, newType);
                        engine.correctWord(predicted, newType);
                        engine.reprocessLastSentence();
                        WordInfo info2 = engine.getWordInfo(predicted);
                        std::cout << (currentLang == "en" ? "After correction:\n" : "Después de corrección:\n");
                        printWordInfo(info2);
                    }*/
                }
                std::cout << "\n" << tr("final_generated") << currentPhrase << "\n";
                break;
            }
            case 8: { // Generate response to a premise (with hypothesis generation)
                std::cout << tr("premise");
                std::string premise;
                std::getline(std::cin, premise);
                std::string currentPhrase = premise;
                if (!premise.empty()) {
                    // First, generate a hypothesis by extending the premise (like prediction loop)
                    for (int iter = 1; iter <= MAX_ITER; ++iter) {
                        bool type = false;
                        auto preds = engine.predictNext(currentPhrase, type);
                        if (preds.empty()) break;
                        std::string predicted = preds[0].word;
                        double bestProb = preds[0].probability;
                        if (bestProb < 0.5 && preds.size() > 1) {
                            // Random selection among the top predictions
                            static std::random_device rd;
                            static std::mt19937 gen(rd());
                            std::uniform_int_distribution<size_t> dist(0, preds.size() - 1);
                            predicted = preds[dist(gen)].word;
                        }
                        std::string correctedSentence;
                        if(!type){
                            correctedSentence = currentPhrase + " " + predicted;
                        }else{
                            correctedSentence = currentPhrase + predicted;
                        }
                        engine.processSentence(correctedSentence);
                        std::cout << tr("learned_sequence") << correctedSentence << "\n";
                        if(!type){
                            currentPhrase += " " + predicted;
                        }else{
                            currentPhrase += predicted;
                        }
                        // Optional: show progress
                        std::cout << "\r" << tr("generating_phrase") << currentPhrase << std::flush;
                    }
                    std::cout << "\n" << tr("premise_given") << premise << "\n";
                    std::cout << tr("hypothesis_generated") << currentPhrase << "\n";
                    std::string response = engine.generateResponse(premise);
                    std::cout << tr("generated_response") << response << "\n";
                }
                bool good = askYesNo(tr("response_useful"));
                engine.provideDialogueFeedback(good);
                std::cout << tr("feedback_recorded") << "\n";
                break;
            }
            case 9: { // Switch language
                std::cout << tr("switch_lang_prompt");
                std::string newLang;
                std::getline(std::cin, newLang);
                if (newLang == "es" || newLang == "en") {
                    engine.setLanguage(newLang);
                    currentLang = newLang;
                    std::cout << tr("lang_set") << currentLang << ".\n";
                } else {
                    std::cout << tr("invalid_lang") << "\n";
                }
                break;
            }
            case 0:
                std::cout << tr("exiting") << "\n";
                break;
            default:
                std::cout << tr("invalid_option") << "\n";
        }
    } while (option != 0);

    engine.shutdown();
    return 0;
}
