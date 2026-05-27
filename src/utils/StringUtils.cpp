// StringUtils.cpp
#include "StringUtils.hpp"
#include <regex>
#include <set>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <vector>

namespace StringUtils {

std::string toLowerNoAccentsSafe(const std::string& s) {
    if (s.empty()) return "";
    try {
        if (s.c_str() == nullptr) return "";
        std::string res = s;
        std::transform(res.begin(), res.end(), res.begin(), ::tolower);
        static const std::vector<std::pair<std::string, std::string>> accents = {
            {"á","a"}, {"é","e"}, {"í","i"}, {"ó","o"}, {"ú","u"}, {"ü","u"}, {"ñ","n"}
        };
        for (const auto& [acc, plain] : accents) {
            size_t pos = 0;
            while ((pos = res.find(acc, pos)) != std::string::npos) {
                res.replace(pos, acc.length(), plain);
                pos += plain.length();
            }
        }
        return res;
    } catch (...) {
        return "";
    }
}

bool isDateFormat(const std::string& token) {
    static const std::regex reIso(R"(^\d{4}-\d{1,2}-\d{1,2}$)");
    static const std::regex reEu(R"(^\d{1,2}/\d{1,2}/\d{4}$)");
    static const std::regex reIsoSlash(R"(^\d{4}/\d{1,2}/\d{1,2}$)");
    static const std::regex reEuDot(R"(^\d{1,2}\.\d{1,2}\.\d{4}$)");
    int y = 0, m = 0, d = 0;
    auto validMD = [](int month, int day) {
        return month >= 1 && month <= 12 && day >= 1 && day <= 31;
    };
    if (std::regex_match(token, reIso)) {
        std::sscanf(token.c_str(), "%d-%d-%d", &y, &m, &d);
        return validMD(m, d);
    }
    if (std::regex_match(token, reEu)) {
        std::sscanf(token.c_str(), "%d/%d/%d", &d, &m, &y);
        return validMD(m, d);
    }
    if (std::regex_match(token, reIsoSlash)) {
        std::sscanf(token.c_str(), "%d/%d/%d", &y, &m, &d);
        return validMD(m, d);
    }
    if (std::regex_match(token, reEuDot)) {
        std::sscanf(token.c_str(), "%d.%d.%d", &d, &m, &y);
        return validMD(m, d);
    }
    return false;
}

bool isNumber(const std::string& token) {
    char* end = nullptr;
    std::strtod(token.c_str(), &end);
    return (end == token.c_str() + token.size());
}

bool isEmail(const std::string& token) {
    static const std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(token, emailRegex);
}

bool isMoney(const std::string& token) {
    static const std::regex moneyRegex(R"((?:€|\$|USD|EUR)\s*\d+(?:\.\d{2})?|\d+(?:\.\d{2})?\s*(?:€|\$|USD|EUR))");
    return std::regex_match(token, moneyRegex);
}

bool isPhone(const std::string& token) {
    static const std::regex phoneRegex(R"(\+?[\d\s\-\(\)]{7,20})");
    return std::regex_match(token, phoneRegex);
}

bool validateValue(const std::string& value, TokenType type) {
    using namespace StringUtils;
    if (value.empty()) return false;
    switch (type) {
        case TokenType::DATE:   return isDateFormat(value);
        case TokenType::NUMBER: return isNumber(value);
        case TokenType::EMAIL:  return isEmail(value);
        case TokenType::MONEY:  return isMoney(value);
        case TokenType::PHONE:  return isPhone(value);
        default:                return true;
    }
}
/**
 * @brief Determines if a character is part of a word token.
 * Allows alphanumeric, apostrophe, hyphen, and common punctuation that may
 * appear inside words (but strips them later if needed). Disallows quotes,
 * colons, brackets, etc. UTF-8 continuation bytes (>= 0x80) are accepted.
 */
bool isWordChar(unsigned char c) {
    // Exclude certain punctuation that breaks words.
    if (c == '"' || c == ':' || c == ';' || c == '\\' ||
        c == '(' || c == ')' || c == '[' || c == ']' ||
        c == '{' || c == '}' || c == '<' || c == '>') {
        return false;
    }
    // UTF-8 multi-byte characters
    if (c >= 0x80) return true;
    return std::isalnum(c) || c == '\'' || c == '-' || c == '/' || c == '.' ||
           c == ',' || c == '?' || c == '!' || c == '$' || c == '@';
}

/**
 * @brief Trims punctuation characters from both ends of a string.
 */
void trimPunctuation(std::string& text) {
    static const std::string punct = ".,;:!?¡¿\"()[]{}<>";
    while (!text.empty() && punct.find(text.back()) != std::string::npos) {
        text.pop_back();
    }
    while (!text.empty() && punct.find(text.front()) != std::string::npos) {
        text.erase(0, 1);
    }
}
/**
 * @brief Converts a UTF-8 string to lowercase, handling Spanish accented vowels.
 * For non‑ASCII characters, only the two‑byte sequences starting with 0xC3 are
 * mapped; other multi‑byte sequences are copied unchanged.
 */
std::string toLowerUtf8(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            result.push_back(static_cast<char>(std::tolower(c)));
            ++i;
            continue;
        }
        // 2‑byte sequence (U+00C0 – U+00FF)
        if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
            unsigned char c2 = static_cast<unsigned char>(s[i + 1]);
            if (c == 0xC3) {
                switch (c2) {
                    case 0x80: result += "\xC3\xA0"; break; // À -> à
                    case 0x81: result += "\xC3\xA1"; break; // Á -> á
                    case 0x82: result += "\xC3\xA2"; break; // Â -> â
                    case 0x83: result += "\xC3\xA3"; break; // Ã -> ã
                    case 0x84: result += "\xC3\xA4"; break; // Ä -> ä
                    case 0x85: result += "\xC3\xA5"; break; // Å -> å
                    case 0x88: result += "\xC3\xA8"; break; // È -> è
                    case 0x89: result += "\xC3\xA9"; break; // É -> é
                    case 0x8A: result += "\xC3\xAA"; break; // Ê -> ê
                    case 0x8B: result += "\xC3\xAB"; break; // Ë -> ë
                    case 0x8C: result += "\xC3\xAC"; break; // Ì -> ì
                    case 0x8D: result += "\xC3\xAD"; break; // Í -> í
                    case 0x8E: result += "\xC3\xAE"; break; // Î -> î
                    case 0x8F: result += "\xC3\xAF"; break; // Ï -> ï
                    case 0x92: result += "\xC3\xB2"; break; // Ò -> ò
                    case 0x93: result += "\xC3\xB3"; break; // Ó -> ó
                    case 0x94: result += "\xC3\xB4"; break; // Ô -> ô
                    case 0x95: result += "\xC3\xB5"; break; // Õ -> õ
                    case 0x96: result += "\xC3\xB6"; break; // Ö -> ö
                    case 0x99: result += "\xC3\xB9"; break; // Ù -> ù
                    case 0x9A: result += "\xC3\xBA"; break; // Ú -> ú
                    case 0x9B: result += "\xC3\xBB"; break; // Û -> û
                    case 0x9C: result += "\xC3\xBC"; break; // Ü -> ü
                    case 0x91: result += "\xC3\xB1"; break; // Ñ -> ñ
                    default:   result += s.substr(i, 2); break;
                }
            } else {
                result += s.substr(i, 2);
            }
            i += 2;
            continue;
        }
        // 3‑byte sequence or other: copy as-is
        if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
            result += s.substr(i, 3);
            i += 3;
            continue;
        }
        // Fallback: copy the byte
        result += s[i];
        ++i;
    }
    return result;
}

const std::set<std::string> kAbbreviations = {
    // ========== TÍTULOS Y TRATAMIENTOS (español) ==========
    "dr", "dra", "sr", "sra", "srta", "srl", "d", "dna", "da", "dña",
    "don", "doña", "sta", "sto", "san", "santa", "santo",
    "fray", "frai", "monseñor", "mons", "rvd", "rdo", "rda",
    "excmo", "excma", "ilm", "ilma", "v", "vd", "vds", "uds", "ud",
    "prof", "profra", "profesora", "lic", "licda", "lcd", "lcdg",
    "arq", "arqo", "ing", "mtr", "mtra", "maestra", "maestro",
    "cap", "capitán", "cnel", "coronel", "gral", "general",
    "alm", "almirante", "ten", "teniente", "sarg", "sargento",
    "pbro", "presbítero", "obispo", "arzobispo", "cardenal",

    // ========== TRATAMIENTOS Y TÍTULOS (inglés) ==========
    "mr", "mrs", "ms", "miss", "msr", "dr", "prof", "rev", "hon",
    "capt", "cpt", "col", "gen", "lt", "sgt", "cpl", "adm", "gov",
    "pres", "sen", "rep", "att", "atty", "esq", "jr", "sr", "phd",
    "md", "rn", "cpa", "co", "inc", "llc", "ltd", "corp",

    // ========== ABREVIATURAS LATINAS Y LOCUCIONES COMUNES ==========
    "ej", "p.ej", "por ej", "v.gr", "v.g", "i.e", "e.g", "etc",
    "et al", "ca", "circa", "aprox", "approx", "ibid", "op cit",
    "loc cit", "cf", "c.f", "vide", "supra", "infra", "passim",
    "sic", "q.e.d", "qed", "n.b", "nota bene", "p.s", "post scriptum",

    // ========== ABREVIATURAS DE DIRECCIONES, EMPRESAS, DOCUMENTOS ==========
    "av", "avda", "avenida", "c", "cl", "calle", "pl", "plza", "plaza",
    "pje", "pasaje", "cjón", "callejón", "blvr", "bulevar", "autov",
    "autopista", "km", "m", "cm", "mm", "hg", "kg", "g", "l", "ml",
    "s.l", "s.a", "s.r.l", "cía", "cia", "comp", "co", "ltd", "inc",
    "no", "núm", "num", "pág", "pag", "págs", "pags", "vol", "tomo",
    "cap", "caps", "ed", "eds", "trad", "trads", "comp", "comps",
    "coord", "coords", "dir", "dirs", "il", "ils", "fol", "fols",

    // ========== ABREVIATURAS TEMPORALES, FECHAS, HORAS ==========
    "a.c", "a.c.", "d.c", "d.c.", "aec", "ec", "ad", "a.d", "a.e.c",
    "ante mer", "post mer", "a.m", "a.m.", "p.m", "p.m.", "bce", "ce",
    "mes", "ene", "feb", "mar", "abr", "may", "jun", "jul", "ago",
    "sep", "sept", "oct", "nov", "dic", "lun", "mar", "mié", "jue",
    "vie", "sáb", "dom", "sem", "año", "sig", "sigl", "ss", "vv",

    // ========== ABREVIATURAS DE DIRECCIONES POSTALES / WEBS ==========
    "apto", "apdo", "depto", "edif", "ed", "piso", "esc", "puerta",
    "esq", "entlo", "principal", "pb", "bajos", "ático", "manz",
    "lote", "mzna", "parc", "pol", "urb", "urbanización", "rúa",
    "planta", "local", "oficina", "nave", "almacén", "depósito",

    // ========== ABREVIATURAS ACADÉMICAS Y CIENTÍFICAS ==========
    "fig", "figs", "tab", "tabs", "ec", "ecs", "eq", "eqs",
    "var", "vars", "ap", "aps", "anex", "ane", "apéndice",
    "ilustr", "gráf", "gráfico", "diag", "esq", "cuadro",
    "sec", "secs", "subsec", "cap", "caps", "ref", "refs",
    "bibliog", "bibl", "fuente", "fuentes", "nota", "notas",

    // ========== OTROS COMUNES ==========
    "c", "f", "j", "n", "p", "q", "rr", "ss", "vv", "w", "x", "y", "z",
    "tel", "telf", "fax", "móv", "cel", "email", "correo", "web", "url",
    "id", "ids", "nº", "número", "ref", "refs", "párr", "párrafo",
    "vf", "vta", "vuelta", "recto", "anv", "anverso", "rev", "reverso"
};

/**
 * @brief Checks whether a lowercase word is a known abbreviation
 *        (prevents sentence splitting after it).
 */
bool isAbbreviation(const std::string& word) {
    return StringUtils::kAbbreviations.find(word) != StringUtils::kAbbreviations.end();
}

} // namespace StringUtils
