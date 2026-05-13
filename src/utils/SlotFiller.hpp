/**
 * @file SlotFiller.hpp
 * @brief Fills template placeholders with concrete words using contextual correlation
 *        and morphological prediction.
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#ifndef SLOT_FILLER_HPP
#define SLOT_FILLER_HPP

#include "../common/types.hpp"
#include "../dialogue/ContextualCorrelator.hpp"
#include <string>
#include <vector>
#include <unordered_map>

class SlotFiller {
public:
    explicit SlotFiller(ContextualCorrelator& correlator);

    /**
     * @brief Predicts a word for a given slot, expected type, and previous context.
     * @param slotName       Name of the slot (e.g. "SUBJECT", "VERB", "NOUN").
     * @param expectedType   Expected WordType.
     * @param prevTagContext Sequence of preceding WordType tags.
     * @param prevWordContext Sequence of preceding word strings.
     * @return The predicted word (or a generic fallback).
     */
    std::string predictForSlot(const std::string& slotName,
                               WordType expectedType,
                               const std::vector<WordType>& prevTagContext,
                               const std::vector<std::string>& prevWordContext);

    /// Overload that infers expected type from slot name.
    std::string predictForSlot(const std::string& slotName,
                               const std::vector<WordType>& prevTagContext,
                               const std::vector<std::string>& prevWordContext);

    /**
     * @brief Fills multiple slots iteratively, building context as it goes.
     * @param slots              Slot names in order.
     * @param slotTypes          Optional explicit types for slots.
     * @param initialTagContext  Initial sequence of word types.
     * @param initialWordContext Initial sequence of words.
     * @return Map from slot name to filled word.
     */
    std::unordered_map<std::string, std::string> fillSlots(
        const std::vector<std::string>& slots,
        const std::unordered_map<std::string, WordType>& slotTypes,
        const std::vector<WordType>& initialTagContext,
        const std::vector<std::string>& initialWordContext);

    /// Maps a slot role name to a WordType (e.g. "SUBJECT" -> NOUN).
    static WordType inferTypeFromSlotName(const std::string& slotName);

    /// Sets the semantic premise context (subject, verb, object).
    void setPremiseContext(const std::string& subject, const std::string& verb, const std::string& object);

    /// Clears premise context.
    void clearPremiseContext();

private:
    ContextualCorrelator& ctxCorr;
    std::string premiseSubject_;
    std::string premiseVerb_;
    std::string premiseObject_;
};

#endif // SLOT_FILLER_HPP
