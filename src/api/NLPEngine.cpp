/**
 * @file NLPEngine.cpp
 * @brief Implementation of the NLP engine facade.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "NLPEngine.hpp"

// Core
#include "../core/Word.hpp"
#include "../core/Sentence.hpp"
#include "../core/Pattern.hpp"
#include "../core/Command.hpp"
#include "../core/Dialogue.hpp"
#include "../core/InferenceEngine.hpp"

// Database
#include "../db/DatabaseManager.hpp"
#include "../db/WordRepository.hpp"
#include "../db/SentenceRepository.hpp"
#include "../db/PatternRepository.hpp"
#include "../db/DialogueRepository.hpp"
#include "../db/TemplateRepository.hpp"

// NLP
#include "../nlp/Tokenizer.hpp"
#include "../nlp/Morphology.hpp"
#include "../nlp/TagStats.hpp"
#include "../nlp/Classifier.hpp"
#include "../nlp/Refiner.hpp"

// Dialogue
#include "../dialogue/PatternCorrelator.hpp"
#include "../dialogue/ContextualCorrelator.hpp"
#include "../dialogue/ChunkCorrelator.hpp"
#include "../dialogue/LetterCorrelator.hpp"
#include "../dialogue/SyllableCorrelator.hpp"

// Utils
#include "../utils/StringConversions.hpp"
#include "../utils/LearningHelpers.hpp"
#include "../utils/ResponseTemplates.hpp"
#include "../utils/SlotFiller.hpp"
#include "../utils/SentenceUtils.hpp"

// STL
#include <deque>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

// ============================================================================
// Internal implementation (Pimpl)
// ============================================================================

class NLPEngine::Impl {
public:
    bool initialized = false;
    bool debugMode = false;

    std::deque<std::string> contextWords;          // up to 15 recent words
    std::string lastProcessedSentenceText;
    Sentence lastProcessedSentence;

    // Correlators and dialogue helpers
    std::unique_ptr<PatternCorrelator> patternCorrW;
    std::unique_ptr<PatternCorrelator> patternCorrC;
    std::unique_ptr<PatternCorrelator> patternCorrL;
    std::unique_ptr<PatternCorrelator> patternCorrS;
    std::unique_ptr<ContextualCorrelator> ctxCorr;
    std::unique_ptr<ChunkCorrelator> chcCorr;
    std::unique_ptr<LetterCorrelator> lttCorr;
    std::unique_ptr<SyllableCorrelator> sllCorr;
    std::unique_ptr<TemplateMatcher> templateMatcher;
    std::unique_ptr<SlotFiller> slotFiller;
    Classifier classifier;

    DialogueContext dialogueContext;

    // Database paths
    std::string semanticDbPath;
    std::string patternDbPath;
    std::string temporalDbPath;

    // Last interaction for feedback
    std::string lastPremiseText;
    std::string lastResponseText;

    // Current language (default Spanish)
    std::string currentLanguage = "es";

    // ----------------------------------------------------------------
    // Initialisation
    // ----------------------------------------------------------------
    bool initialize(const std::string& semPath, const std::string& patPath, const std::string& tempPath) {
        semanticDbPath = semPath;
        patternDbPath  = patPath;
        temporalDbPath = tempPath.empty() ? ":memory:" : tempPath;

        // 1. Open databases
        if (!DatabaseManager::instance().init(semanticDbPath)) {
            if (debugMode) std::cerr << "[ERROR] Cannot open semantic DB: " << semanticDbPath << std::endl;
            return false;
        }
        if (!DatabaseManager::instance().init(patternDbPath)) {
            if (debugMode) std::cerr << "[ERROR] Cannot open pattern DB: " << patternDbPath << std::endl;
            return false;
        }
        if (!DatabaseManager::instance().init(temporalDbPath)) {
            if (debugMode) std::cerr << "[ERROR] Cannot open temporal DB: " << temporalDbPath << std::endl;
            return false;
        }

        // 2. Configure repositories
        WordRepository::setDatabasePath(semanticDbPath);
        SentenceRepository::setDatabasePath(semanticDbPath);
        DialogueRepository::setDatabasePath(semanticDbPath);
        PatternRepository::setDatabasePath(patternDbPath);
        TagStats::setDatabasePath(patternDbPath);
        TemplateRepository::setDatabasePath(patternDbPath);

        // 3. Create tables
        WordRepository::initializeTables();
        SentenceRepository::initializeTables();
        TagStats::initializeTables();
        DialogueRepository::initializeTables();
        TemplateRepository::initializeTables();

        // 4. Load default data
        TagStats::loadDefaultFromStatic();
        TemplateRepository::loadDefaultIfEmpty();

        // 5. Initialise correlators
        try {
            patternCorrW = std::make_unique<PatternCorrelator>(patternDbPath, "");
            patternCorrC = std::make_unique<PatternCorrelator>(patternDbPath, "_chunk");
            patternCorrL = std::make_unique<PatternCorrelator>(patternDbPath, "_letter");
            patternCorrS = std::make_unique<PatternCorrelator>(patternDbPath, "_syllable");
            ctxCorr      = std::make_unique<ContextualCorrelator>(patternDbPath);
            chcCorr      = std::make_unique<ChunkCorrelator>(patternDbPath);
            lttCorr      = std::make_unique<LetterCorrelator>(patternDbPath);
            sllCorr      = std::make_unique<SyllableCorrelator>(patternDbPath);
        } catch (const std::exception& e) {
            if (debugMode) std::cerr << "[ERROR] Correlators: " << e.what() << std::endl;
            return false;
        }
        PatternRepository::initializeTables();

        // 6. Templates and slot filler
        templateMatcher = std::make_unique<TemplateMatcher>();
        templateMatcher->loadDefaultTemplates();
        slotFiller = std::make_unique<SlotFiller>(*ctxCorr);

        // 7. Dialogue context
        dialogueContext.patternCorr     = patternCorrW.get();
        dialogueContext.ctxCorr        = ctxCorr.get();
        dialogueContext.chcCorr        = chcCorr.get();
        dialogueContext.templateMatcher = templateMatcher.get();
        dialogueContext.slotFiller      = slotFiller.get();
        loadDefaultInferenceRules(dialogueContext);

        // Set initial language
        setLanguage(currentLanguage);

        initialized = true;
        return true;
    }

    void shutdown() {
        if (initialized) {
            patternCorrW.reset();
            patternCorrC.reset();
            patternCorrL.reset();
            patternCorrS.reset();
            ctxCorr.reset();
            chcCorr.reset();
            lttCorr.reset();
            sllCorr.reset();
            templateMatcher.reset();
            slotFiller.reset();
            DatabaseManager::instance().closeAll();
            initialized = false;
        }
    }

    void setDebugMode(bool enable) { debugMode = enable; }

    void setLanguage(const std::string& lang) {
        currentLanguage = lang;
        morphology::setLanguage(lang);
        TagStats::setLanguage(lang);
        // also propagate to command detection etc.
        setCommandLanguage(lang);
    }

    // ----------------------------------------------------------------
    // Sentence processing
    // ----------------------------------------------------------------
    std::vector<WordInfo> processSentence(const std::string& sentence) {
        if (!initialized || sentence.empty()) return {};

        // 1. Tokenise and create initial word list
        std::vector<Word> words = createWordVector(sentence);
        if (words.empty()) return {};
        // 2. Classify
        bool classific = true;
        for(auto w : words){
            if(w.getConfidence() < 0.8f)classific = false;
        }
        if(!classific)classifier.classifySentence(words);

        // 3. Learn contextual and chunk correlations
        learnTextWithContext(*ctxCorr, *patternCorrW, sentence);
        learnLetterCorrelations(*lttCorr, sentence);
        learnSyllableCorrelations(*sllCorr, sentence);
        chcCorr->learnFromClassifiedSentence(words);
        std::vector<std::string> chunks = Chunker::chunk(words);
        chcCorr->learnNextChunkDirect(chunks);

        // 4. Build Sentence, save to DB
        Sentence sent(words);
        Pattern p = patternFromSequence(sent.getTypeSequence());
        PatternRepository::save(p);
        SentenceRepository::save(sent);
        lastProcessedSentence = sent;
        lastProcessedSentenceText = sentence;

        // 5. Learn word relations using the correlator
        std::vector<std::string> wordStrings;
        for (const auto& w : words) wordStrings.push_back(w.getWord());
        for (auto& w : words) {
            w.learnRelationsFromCorrelator(*dialogueContext.patternCorr, wordStrings);
            WordRepository::save(w);
        }

        // 6. Update internal context (max 15 words)
        for (const auto& w : words) {
            contextWords.push_back(w.getWord());
            if (contextWords.size() > 15) contextWords.pop_front();
        }

        // 7. Convert to WordInfo for output
        std::vector<WordInfo> result;
        for (const auto& w : words) {
            result.push_back(wordToInfo(w));
        }
        return result;
    }

    // ----------------------------------------------------------------
    // Next word prediction
    // ----------------------------------------------------------------

    bool predictNextLetter(const std::string& currentContext,
                std::vector<std::pair<WordPattern, double>>& outcomes) {
        if (currentContext.empty()) return {};

        // Split context into individual characters (including spaces)
        std::vector<std::string> letters;
        for (char c : currentContext) {
            letters.push_back(std::string(1, c));
        }
        bool found = false;

        if (letters.size() >= 2) {
            // Use two previous letters as context
            std::string curr = letters.back();
            std::string prev1 = letters[letters.size()-2];
            std::string prev2 = (letters.size() >= 3) ? letters[letters.size()-3] : "";
            if (!prev2.empty())
                found = lttCorr->queryNextWithTwoPrev(curr, prev1, prev2, outcomes);
            else
                found = lttCorr->queryNextWithOnePrev(curr, prev1, outcomes);
        } else if (letters.size() == 1) {
            found = lttCorr->queryNext(letters.back(), {"__NO_CONTEXT__"}, outcomes);
        }
        return found;
    }

    bool predictNextSyllable(const std::string& currentContext,
                    std::vector<std::pair<WordPattern, double>>& outcomes) {
        if (currentContext.empty()) return {};

        // Tokenize context by spaces (each token is a syllable)
        std::vector<std::string> syllables;
        std::stringstream ss(currentContext);
        std::string token;
        while (ss >> token) syllables.push_back(token);

        bool found = false;

        if (syllables.size() >= 2) {
            std::string curr = syllables.back();
            std::string prev1 = syllables[syllables.size()-2];
            std::string prev2 = (syllables.size() >= 3) ? syllables[syllables.size()-3] : "";
            if (!prev2.empty())
                found = sllCorr->queryNextWithTwoPrev(curr, prev1, prev2, outcomes);
            else
                found = sllCorr->queryNextWithOnePrev(curr, prev1, outcomes);
        } else if (syllables.size() == 1) {
            found = sllCorr->queryNext(syllables.back(), {"__NO_CONTEXT__"}, outcomes);
        }
        return found;
    }
    std::vector<Prediction> predictNext(const std::string& currentWords,bool& type) {
        if (!initialized) return {};

        std::vector<Word> words;
        std::vector<std::string> wordsCorr;
        std::stringstream ss(currentWords);
        std::string w;
        while (ss >> w) {
            wordsCorr.push_back(w);
            Word wo(w);
            WordRepository::load(w, wo);
            words.push_back(wo);
        }

        std::vector<std::string> chunks = Chunker::chunk(words);
        std::vector<std::pair<WordPattern, double>> outcomes;
        bool hasPrediction = false;

        if (chunks.size() >= 3) {
            hasPrediction = chcCorr->queryNextWithTwoPrev(
                chunks.back(), chunks[chunks.size()-2], chunks[chunks.size()-3], outcomes);
        } else if (chunks.size() == 2) {
            hasPrediction = chcCorr->queryNextWithOnePrev(chunks.back(), chunks[0], outcomes);
        } else if (chunks.size() == 1) {
            hasPrediction = chcCorr->queryNext(chunks.back(), {"__NO_CONTEXT__"}, outcomes);
        }

        // Fallback to word-level context
        if (!hasPrediction || outcomes.empty()) {
            std::string curr;
            std::vector<std::string> prev;
            if (wordsCorr.size() > 1) {
                curr = wordsCorr.back();
                prev = {wordsCorr[wordsCorr.size()-2]};
            } else if (wordsCorr.size() == 1) {
                curr = wordsCorr.back();
                prev = {"__NO_CONTEXT__"};
            }
            if (!curr.empty()) {
                outcomes.clear();
                hasPrediction = ctxCorr->queryNext(curr, prev, outcomes);
            }
        }
        if ((!hasPrediction || outcomes.empty()) && type == false){
            hasPrediction = predictNextSyllable(currentWords, outcomes);
            if(hasPrediction) type = true;
        }

        if ((!hasPrediction || outcomes.empty()) && type == false){
            hasPrediction = predictNextLetter(currentWords, outcomes);
            if(hasPrediction) type = true;
        }

        std::vector<Prediction> preds;
        for (const auto& p : outcomes) {
            if (!p.first.empty()) {
                Prediction pred;
                pred.word        = p.first.begin()->first;
                pred.probability = p.second;
                preds.push_back(pred);
            }
        }
        return preds;
    }

    // ----------------------------------------------------------------
    // Response generation
    // ----------------------------------------------------------------
    std::string generateResponse(const std::string& premiseText) {
        if (!initialized || premiseText.empty()) return "";

        std::vector<Word> words = createWordVector(premiseText);
        if (words.empty()) return "";
        classifier.classifySentence(words);
        Sentence premiseSent(words);
        SentenceRepository::save(premiseSent);

        Pattern p;
        p.sequence = premiseSent.getTypeSequence();
        p.type     = classifySentencePattern(p.sequence);

        DialogueHistory history = DialogueRepository::loadHistory();
        float creativity = history.getThresholdCreativity();

        std::srand(static_cast<unsigned>(std::time(nullptr)));
        float randomFactor = (std::rand() % 100) / 100.0f * 0.05f;  // 0..0.05
        creativity = std::min(0.95f, creativity + randomFactor - 0.025f);

        Sentence hypothesis = generateHypothesis(premiseSent, dialogueContext, &p,
                                                 words.back().getWord(), creativity);
        std::string responseText = hypothesis.toString();

        SentenceRepository::save(hypothesis);
        DialogueRepository::saveDialogue(premiseSent, hypothesis, p, creativity);

        lastPremiseText = premiseText;
        lastResponseText = responseText;
        return responseText;
    }

    // ----------------------------------------------------------------
    // Feedback
    // ----------------------------------------------------------------
    void provideDialogueFeedback(bool positive) {
        if (!initialized || lastPremiseText.empty() || lastResponseText.empty()) return;

        Sentence premSent = utils::buildSentenceFromText(lastPremiseText);
        Sentence respSent = utils::buildSentenceFromText(lastResponseText);

        if (premSent.getId() <= 0) SentenceRepository::save(premSent);
        if (respSent.getId() <= 0) SentenceRepository::save(respSent);

        Pattern p = patternFromSequence(premSent.getTypeSequence());
        float creativity = utils::computeCreativity(premSent, respSent, p);
        DialogueRepository::saveDialogue(premSent, respSent, p, creativity);

        if (positive) {
            for (const auto& block : respSent.getBlocks()) {
                // Register feedback that the assigned type was correct
                DialogueRepository::registerFeedback(block.text, block.type, block.type, true);
            }
        }
        // On negative feedback we could trigger reprocessing; for now, just log.
    }

    // ------------------------------------------------------------------------
    // Corrección y reprocesamiento
    // ------------------------------------------------------------------------
    void correctWord(const std::string& word, const std::string& correctType) {
        if (!initialized) return;
        Word w(word);
        if (!WordRepository::load(word, w)) return;

        WordType newType = WordType::UNDEFINED;
        if (correctType == "Sustantivo") newType = WordType::NOUN;
        else if (correctType == "Verbo") newType = WordType::VERB;
        else if (correctType == "Adjetivo") newType = WordType::ADJECTIVE;
        else if (correctType == "Adverbio") newType = WordType::ADVERB;
        else if (correctType == "Preposición") newType = WordType::PREPOSITION;
        else if (correctType == "Conjunción") newType = WordType::CONJUNCTION;
        else if (correctType == "Artículo") newType = WordType::ARTICLE;
        else if (correctType == "Pronombre") newType = WordType::PRONOUN;

        if (newType != WordType::UNDEFINED) {
            w.setType(newType);
            w.setConfidence(0.95f);
            w.setMeaning("Corregido por usuario a " + correctType);
            WordRepository::save(w);
        } else if (debugMode) {
            std::cerr << "[WARN] Tipo incorrecto para corrección: " << correctType << std::endl;
        }
    }

    void reprocessLastSentence() {
        if (!initialized || lastProcessedSentenceText.empty()) return;
        processSentence(lastProcessedSentenceText);
    }
    // ----------------------------------------------------------------
    // Context management
    // ----------------------------------------------------------------
    void resetContext() {
        contextWords.clear();
        lastProcessedSentenceText.clear();
        lastProcessedSentence = Sentence();
        lastPremiseText.clear();
        lastResponseText.clear();
    }

    // ----------------------------------------------------------------
    // Word look‑up
    // ----------------------------------------------------------------
    WordInfo getWordInfo(std::string& word) {
        if (!initialized) return {};
        // Remove trailing punctuation (simple)
        while (!word.empty() && std::ispunct(static_cast<unsigned char>(word.back()))) {
            word.pop_back();
        }
        Word w(word);
        WordRepository::load(word, w);
        return wordToInfo(w);
    }

private:
    WordInfo wordToInfo(const Word& w) {
        WordInfo info;
        info.word       = w.getWord();
        info.type       = wordTypeToString(w.getType());
        info.confidence = w.getConfidence();
        info.meaning    = w.getMeaning();
        info.quantity   = quantityToString(w.getQuantity());
        info.tense      = tenseToString(w.getTense());
        info.gender     = genderToString(w.getGender());
        info.person     = personToString(w.getPerson());
        info.degree     = degreeToString(w.getDegree());
        info.relatedWords = w.getRelated();   // copy of vector<pair<string,double>>
        return info;
    }
};

// ============================================================================
// Public Facade
// ============================================================================

NLPEngine::NLPEngine() : pImpl(std::make_unique<Impl>()) {}
NLPEngine::~NLPEngine() { shutdown(); }

bool NLPEngine::initialize(const std::string& semPath,
                           const std::string& patPath,
                           const std::string& tempPath) {
    return pImpl->initialize(semPath, patPath, tempPath);
}

void NLPEngine::shutdown() { pImpl->shutdown(); }
void NLPEngine::setDebugMode(bool enable) { pImpl->setDebugMode(enable); }
void NLPEngine::setLanguage(const std::string& lang) { pImpl->setLanguage(lang); }
void NLPEngine::learnText(const std::string& text) {
    if (!pImpl->initialized) return;
    learnTextWithContext(*pImpl->ctxCorr, *pImpl->patternCorrW, text);
}

std::vector<WordInfo> NLPEngine::processSentence(const std::string& sentence) {
    return pImpl->processSentence(sentence);
}

std::vector<Prediction> NLPEngine::predictNext(const std::string& currentWord, bool& type) {
    return pImpl->predictNext(currentWord, type);
}

std::string NLPEngine::generateResponse(const std::string& premise) {
    return pImpl->generateResponse(premise);
}

void NLPEngine::provideDialogueFeedback(bool positive) {
    pImpl->provideDialogueFeedback(positive);
}

void NLPEngine::correctWord(const std::string& word, const std::string& correctType) {
    pImpl->correctWord(word, correctType);
}

void NLPEngine::reprocessLastSentence() {
    pImpl->reprocessLastSentence();
}

void NLPEngine::resetContext() {
    pImpl->resetContext();
}

WordInfo NLPEngine::getWordInfo(std::string& word) {
    return pImpl->getWordInfo(word);
}
