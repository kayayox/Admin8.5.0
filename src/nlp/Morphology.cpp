/**
 * @file Morphology.cpp
 * @brief Implementation of language‑aware morphological analysis.
 *
 * Supports Spanish and English. The active language is set via setLanguage().
 * All closed‑class word lists and suffix rules are language‑specific.
 *
 * @author Soubhi Khayat Najjar
 * @date 2026
 */

#include "Morphology.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <vector>

namespace morphology {

    // -----------------------------------------------------------------------
    // Language state
    // -----------------------------------------------------------------------
    static std::string currentLanguage_ = "es";
    void setLanguage(const std::string& lang) { currentLanguage_ = lang; }
    std::string getLanguage() { return currentLanguage_; }

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------
    namespace {

        bool contains(const std::vector<std::string>& list, const std::string& word) {
            return std::find(list.begin(), list.end(), word) != list.end();
        }

        // Case‑insensitive search (used for English, which is lowercased)
        bool containsIgnoreCase(const std::vector<std::string>& list, const std::string& word) {
            std::string lower;
            lower.reserve(word.size());
            for (char c : word) lower.push_back(static_cast<char>(std::tolower(c)));
            return std::find(list.begin(), list.end(), lower) != list.end();
        }

        bool endsWithAny(const std::string& word, const std::vector<std::string>& suffixes) {
            for (const auto& suf : suffixes) {
                if (endsWith(word, suf)) return true;
            }
            return false;
        }

    } // anonymous namespace

    // ========================================================================
    // SPANISH DATA (unchanged logic, only enum names updated)
    // ========================================================================

    // Spanish negation words (lowercase)
    static const std::vector<std::string> es_negationWords = {
        "no", "nunca", "jamás", "tampoco", "nadie", "nada", "ninguno", "ninguna",
        "ningunos", "ningunas", "ningún", "ni", "sino", "ni siquiera", "nada de",
        "nadie más", "jamás de los jamases", "para nada", "en absoluto", "de ningún modo",
        "de ninguna manera", "no obstante", "sin embargo", "no solo", "sino también"
    };

    // Spanish affirmation words (lowercase)
    static const std::vector<std::string> es_affirmationWords = {
        "sí", "claro", "ciertamente", "efectivamente", "vale", "ok", "de acuerdo",
        "afirmativo", "correcto", "exacto", "justo", "así es", "por supuesto",
        "desde luego", "sin duda", "seguro", "cierto", "verdad", "bueno", "bien",
        "perfecto", "entendido", "conforme", "ya"
    };
    static const std::vector<std::string> es_commonNouns = {
        "casa", "perro", "gato", "hombre", "mujer", "niño", "niña", "padre", "madre", "hermano",
        "hermana", "amigo", "amiga", "trabajo", "escuela", "ciudad", "país", "mundo", "vida", "día",
        "noche", "tiempo", "año", "mes", "mar", "semana", "hora", "minuto", "persona", "familia", "empresa",
        "agua", "aire", "fuego", "tierra", "sol", "luna", "estrella", "cielo", "árbol", "flor",
        "pájaro", "pez", "caballo", "vaca", "cerdo", "oveja", "pato", "gallina", "conejo", "ratón",
        "elefante", "tigre", "león", "oso", "lobo", "zorro", "serpiente", "insecto", "mosca", "abeja",
        "cabeza", "cara", "ojo", "oreja", "nariz", "boca", "diente", "lengua", "cuello", "espalda",
        "brazo", "mano", "dedo", "uña", "pierna", "pie", "rodilla", "corazón", "sangre", "hueso",
        "comida", "bebida", "pan", "queso", "leche", "huevo", "carne", "arroz", "pasta", "fruta",
        "manzana", "pera", "plátano", "naranja", "uva", "fresa", "cereza", "tomate", "patata", "cebolla",
        "mesa", "silla", "cama", "puerta", "ventana", "techo", "pared", "cocina", "baño", "habitación",
        "reloj", "teléfono", "televisor", "ordenador", "coche", "bicicleta", "tren", "avión", "barco", "autobús",
        "calle", "carretera", "camino", "puente", "río", "montaña", "bosque", "playa", "isla", "continente",
        "dinero", "precio", "compras", "tienda", "mercado", "banco", "hospital", "biblioteca", "iglesia", "museo",
        "arte", "música", "canción", "libro", "película", "historia", "idioma", "palabra", "frase", "página",
        "amor", "alegría", "miedo", "dolor", "paz", "guerra", "problema", "solución", "sueño", "recuerdo"
    };
    static const std::vector<std::string> es_commonAdjectives = {
        "bueno", "buena", "malo", "mala", "grande", "pequeño", "pequeña", "nuevo", "nueva", "viejo",
        "vieja", "joven", "alto", "alta", "bajo", "baja", "feliz", "triste", "inteligente", "amable",
        "rojo", "roja", "azul", "verde", "amarillo", "amarilla", "blanco", "blanca", "negro", "negra",
        "gris", "marrón", "naranja", "rosa", "violeta", "oscuro", "oscura", "claro", "clara",
        "largo", "larga", "corto", "corta", "estrecho", "estrecha", "ancho", "ancha", "gordo", "gorda",
        "delgado", "delgada", "redondo", "redonda", "cuadrado", "cuadrada", "recto", "recta", "curvo", "curva",
        "bonito", "bonita", "feo", "fea", "guapo", "guapa", "hermoso", "hermosa", "lindo", "linda",
        "rico", "rica", "pobre", "dulce", "salado", "salada", "amargo", "amarga", "ácido", "ácida",
        "caliente", "frío", "fría", "tibio", "tibia", "seco", "seca", "mojado", "mojada", "húmedo", "húmeda",
        "rápido", "rápida", "lento", "lenta", "fuerte", "débil", "duro", "dura", "blando", "blanda",
        "fácil", "difícil", "sencillo", "sencilla", "complicado", "complicada", "posible", "imposible",
        "importante", "necesario", "necesaria", "especial", "diferente", "igual", "común", "raro", "rara",
        "alegre", "enojado", "enojada", "cansado", "cansada", "enfermo", "enferma", "sano", "sana",
        "valiente", "cobarde", "simpático", "simpática", "antipático", "antipática", "generoso", "generosa", "egoísta",
        "trabajador", "trabajadora", "perezoso", "perezosa", "honesto", "honesta", "mentiroso", "mentirosa",
        "limpio", "limpia", "sucio", "sucia", "ordenado", "ordenada", "desordenado", "desordenada",
        "abierto", "abierta", "cerrado", "cerrada", "lleno", "llena", "vacío", "vacía"
    };
    static const std::vector<std::string> es_irregularVerbs = {
        "ser", "ir", "haber", "estar", "extrae", "extraer", "tener", "hacer", "poder", "decir", "ver", "dar",
        "saber", "querer", "llegar", "pasar", "deber", "poner", "parecer", "quedar", "creer",
        "venir", "salir", "valer", "caber", "caer", "traer", "oír", "oler",
        "andar", "conducir", "traducir", "conocer", "reconocer", "agradecer", "ofrecer", "pertenecer",
        "nacer", "obedecer", "enriquecer", "envejecer", "oscurecer", "permanecer", "establecer",
        "jugar", "pensar", "entender", "empezar", "comenzar", "perder", "preferir", "sentir",
        "dormir", "morir", "pedir", "servir", "repetir", "seguir", "conseguir", "perseguir",
        "vestir", "rendir", "elegir", "corregir", "medir", "reñir", "teñir", "freír", "reír", "sonreír",
        "mover", "llover", "volver", "resolver", "devolver", "envolver", "conmover", "promover",
        "soler", "doler", "satisfacer", "deshacer", "rehacer",
        "incluir", "construir", "destruir", "huir", "sustituir", "distinguir", "extinguir",
        "erguir", "adquirir", "inquirir",
        "roer", "raer", "yacer", "placer", "cocer", "torcer", "vencer", "mecer"
    };
    static const std::vector<std::string> es_commonAdverbs = {
        "aquí", "allí", "ahí", "cerca", "lejos", "despacio", "rápido", "bien", "mal", "mucho",
        "poco", "nunca", "siempre", "también", "tampoco", "solo", "solamente", "inclusive",
        "adelante", "atrás", "arriba", "abajo", "dentro", "fuera", "encima", "debajo",
        "enfrente", "detrás", "alrededor", "dondequiera", "rapidamente",
        "antes", "después", "luego", "pronto", "tarde", "temprano", "ayer", "hoy", "mañana",
        "anoche", "entonces", "todavía", "ya", "aún", "incluso", "jamás", "frecuentemente",
        "a menudo", "a veces", "raramente", "recién", "antiguamente", "últimamente", "mientras",
        "deprisa", "lentamente", "fácilmente", "difícilmente", "cuidadosamente", "especialmente",
        "así", "tal", "cómo", "dónde", "cuándo", "cuánto",
        "mejor", "peor", "regular", "más", "menos", "casi", "aproximadamente", "exactamente",
        "quizá", "quizás", "acaso", "seguramente", "probablemente", "efectivamente",
        "sí", "no", "ciertamente", "cierto", "además", "asimismo"
    };
    static const std::vector<std::string> es_demonstratives = {
        "este", "esta", "estos", "estas", "esto",
        "ese", "esa", "esos", "esas", "eso",
        "aquel", "aquella", "aquellos", "aquellas", "aquello"
    };
    static const std::vector<std::string> es_numerals = {
        "uno", "dos", "tres", "cuatro", "cinco", "seis", "siete", "ocho", "nueve", "diez",
        "once", "doce", "trece", "catorce", "quince", "veinte", "cien", "mil", "primer", "tercer",
        "veintiuno", "veintidós", "veintitrés", "veinticuatro", "veinticinco",
        "veintiséis", "veintisiete", "veintiocho", "veintinueve",
        "treinta", "cuarenta", "cincuenta", "sesenta", "setenta", "ochenta", "noventa",
        "ciento", "doscientos", "trescientos", "cuatrocientos", "quinientos",
        "seiscientos", "setecientos", "ochocientos", "novecientos",
        "millón",
        "primero", "segundo", "tercero", "cuarto", "quinto", "sexto",
        "séptimo", "octavo", "noveno", "décimo",
        "undécimo", "duodécimo", "vigésimo", "trigésimo", "cuadragésimo",
        "quincuagésimo", "sexagésimo", "septuagésimo", "octogésimo", "nonagésimo"
    };
    static const std::vector<std::string> es_relatives = {
        "que", "quien", "quienes", "cuyo", "cuya", "cuyos", "cuyas",
        "el cual", "la cual", "lo cual",
        "los cuales", "las cuales",
        "cuanto", "cuanta", "cuantos", "cuantas"
    };
    static const std::vector<std::string> es_quantifiers = {
        "mucho", "mucha", "muchos", "muchas", "poco", "poca", "pocos", "pocas",
        "varios", "varias", "todo", "toda", "todos", "todas", "algo", "nada",
        "bastante", "demasiado", "demasiada", "demasiados", "demasiadas",
        "más", "menos", "tanto", "tanta", "tantos", "tantas", "alguno", "alguna",
        "algunos", "algunas", "ningún", "ninguno", "ninguna", "cualquier", "cualquiera",
        "ambos", "ambas", "cada", "sendos", "sendas", "diversos", "diversas", "distintos",
        "suficiente", "suficientes", "alguien", "nadie",
        "cada", "cualquiera", "quienquiera", "uno", "una",
        "unos", "unas", "numeroso", "numerosa", "numerosos", "numerosas",
        "escaso", "escasa", "escasos", "escasas", "distintas"
    };
    static const std::vector<std::string> es_prepositions = {
        "a", "al", "del", "ante", "bajo", "con", "de", "en",
        "para", "por", "sin", "sobre", "tras", "durante", "mediante",
        "desde", "hasta", "hacia", "según", "contra", "entre",
        "excepto", "salvo", "so", "cabe", "versus", "vía", "pro"
    };
    static const std::vector<std::string> es_conjunctions = {
        "y", "e", "o", "u",
        "pero", "sino", "mas",
        "porque", "pues", "conque", "luego", "así",
        "si", "aunque", "ni", "que",
        "como", "cuando", "mientras", "siquiera"
    };
    static const std::vector<std::string> es_articles = {
        "el", "la", "los", "las", "lo", "un", "una", "unos", "unas"
    };
    static const std::vector<std::string> es_pronouns = {
        "yo", "tú", "vos", "él", "ella", "ello", "nosotros", "nosotras",
        "vosotros", "vosotras", "ellos", "ellas", "usted", "ustedes",
        "me", "te", "se", "lo", "la", "le", "nos", "os"
    };
    static const std::vector<std::string> es_possessives = {
        "mío", "mía", "míos", "mías", "tuyo", "tuya", "tuyos", "tuyas",
        "suyo", "suya", "suyos", "suyas", "nuestro", "nuestra", "nuestros", "nuestras",
        "vuestro", "vuestra", "vuestros", "vuestras"
    };
    static const std::vector<std::string> es_interrogatives = {
        "qué", "cuál", "cuáles", "cómo", "cuándo", "dónde", "adónde",
        "por qué", "para qué", "cuánto", "cuánta", "cuántos", "cuántas", "cuán",
        "quién", "quiénes"
    };

    static const std::vector<std::string> es_nounSuffixes = {
        "ción", "sión", "xión", "dad", "tad", "eza", "ez",
        "ancia", "encia", "icia", "icie", "ismo", "aje", "ambre", "umbre",
        "tud", "dura", "anza", "ncia", "tor", "sor", "dor",
        "ista", "ería","ada", "miento", "mento", "tura", "or", "ud",
        "-ito", "ita", "illo", "illa", "ico", "ica",
        "uelo", "uela","-ón", "ona", "azo", "aza", "ote", "ota"
    };
    static const std::vector<std::string> es_verbSuffixes = {
        "ar", "er", "ir", "ando", "iendo", "yendo",
        "ado", "ido", "aba", "abas", "ábamos", "aban",
        "ía", "ías", "íamos", "ían", "are", "ere", "ire",
        "aria", "ería", "iría", "aste", "iste", "asteis", "isteis",
        "aron", "ieron", "ó", "ió", "as", "a", "amos", "áis", "an",
        "es", "e", "emos", "éis", "en", "é", "aste", "ó", "amos",
        "asteis", "aron","í", "iste", "ió", "imos", "isteis", "ieron",
        "abais", "aban", "íais", "ían", "aré", "eré", "iré",
        "arás", "erás", "irás", "ará", "erá", "irá", "aremos", "eremos", "iremos",
        "aréis", "eréis", "iréis", "arán", "erán", "irán", "e", "es", "e",
        "emos", "éis", "en", "a", "as", "a", "amos", "áis", "an", "ra",
        "ras", "ra", "ramos", "rais", "ran", "se", "ses", "se", "semos", "seis",
        "sen","are", "ares", "are", "áremos", "areis", "aren"
    };
    static const std::vector<std::string> es_adjectiveSuffixes = {
        "oso", "osa", "ivo", "iva", "ble", "nte", "ante", "ente",
        "al", "il","ario", "aria", "ero", "era", "dora", "dero", "dera",
        "ativo", "itiva", "esco", "esca", "uno", "una", "izo", "iza",
        "torio", "toria", "ano", "ana", "és", "esa", "ino", "ina", "ense",
        "eño", "eña", "iento", "ienta", "udo", "uda", "iente", "ico", "ica",
        "ístico", "ística", "ático", "ática", "able", "ible", "ón", "ona"
    };
    static const std::vector<std::string> es_adverbSuffixes = { "mente" };

    // ========================================================================
    // ENGLISH DATA (extensive coverage)
    // ========================================================================

    // English negation words (lowercase)
    static const std::vector<std::string> en_negationWords = {
        "no", "not", "never", "neither", "nor", "none", "nothing", "nowhere",
        "nobody", "no one", "nowise", "nay", "nix", "nah", "nope", "non",
        "cannot", "can't", "won't", "don't", "doesn't", "didn't", "isn't", "aren't",
        "wasn't", "weren't", "haven't", "hasn't", "hadn't", "couldn't", "shouldn't",
        "wouldn't", "mightn't", "mustn't", "needn't", "daren't", "shan't"
    };

    // English affirmation words (lowercase)
    static const std::vector<std::string> en_affirmationWords = {
        "yes", "yeah", "yep", "yup", "yea", "sure", "ok", "okay", "alright",
        "affirmative", "true", "correct", "indeed", "certainly", "definitely",
        "absolutely", "precisely", "exactly", "right", "roger", "aye", "uh-huh"
    };

    // Common nouns (lowercased)
    static const std::vector<std::string> en_commonNouns = {
        "house","dog","cat","man","woman","boy","girl","father","mother","brother",
        "sister","friend","job","school","city","country","world","life","day",
        "night","time","year","month","sea","week","hour","minute","person","family","company",
        "water","air","fire","earth","sun","moon","star","sky","tree","flower",
        "bird","fish","horse","cow","pig","sheep","duck","chicken","rabbit","mouse",
        "elephant","tiger","lion","bear","wolf","fox","snake","insect","fly","bee",
        "head","face","eye","ear","nose","mouth","tooth","tongue","neck","back",
        "arm","hand","finger","nail","leg","foot","knee","heart","blood","bone",
        "food","drink","bread","cheese","milk","egg","meat","rice","pasta","fruit",
        "apple","pear","banana","orange","grape","strawberry","cherry","tomato","potato","onion",
        "table","chair","bed","door","window","ceiling","wall","kitchen","bathroom","room",
        "clock","telephone","television","computer","car","bicycle","train","plane","boat","bus",
        "street","road","path","bridge","river","mountain","forest","beach","island","continent",
        "money","price","shopping","shop","market","bank","hospital","library","church","museum",
        "art","music","song","book","film","story","language","word","sentence","page",
        "love","joy","fear","pain","peace","war","problem","solution","dream","memory",
        "phone","internet","video","game","idea","experience","opportunity","question","answer",
        "decision","history","photo","email","message","noise","light","color","name","place",
        "task","event","action","result","example","study","research","health","power","control"
    };

    // Common adjectives (lowercased)
    static const std::vector<std::string> en_commonAdjectives = {
        "good","bad","big","small","new","old","young","high","low","happy","sad",
        "intelligent","kind","red","blue","green","yellow","white","black","grey","brown",
        "orange","pink","purple","dark","light","long","short","narrow","wide","fat","thin",
        "round","square","straight","curved","beautiful","ugly","pretty","handsome","rich","poor",
        "sweet","salty","bitter","sour","hot","cold","warm","dry","wet","humid",
        "fast","slow","strong","weak","hard","soft","easy","difficult","simple","complicated",
        "possible","impossible","important","necessary","special","different","same","common","rare",
        "angry","tired","sick","healthy","brave","cowardly","nice","mean","generous","selfish",
        "hardworking","lazy","honest","dishonest","clean","dirty","tidy","messy","open","closed",
        "full","empty","exciting","boring","interesting","terrible","wonderful","comfortable",
        "uncomfortable","similar","various","nervous","calm","safe","dangerous","modern","traditional",
        "expensive","cheap","correct","wrong","real","false","main","entire","free","busy","ready"
    };

    // Common verbs (base form, lowercased)
    static const std::vector<std::string> en_commonVerbs = {
        "be","have","do","say","go","get","make","know","think","take","see","come",
        "want","look","use","find","give","tell","work","call","try","ask","need",
        "feel","become","leave","put","mean","keep","let","begin","seem","help","show",
        "hear","play","run","move","live","believe","bring","happen","write","sit","stand",
        "lose","pay","meet","include","continue","set","learn","change","lead","understand",
        "watch","follow","stop","create","speak","read","allow","add","spend","grow","open",
        "walk","win","offer","remember","love","consider","appear","buy","wait","serve",
        "die","send","expect","build","stay","fall","cut","reach","kill","remain",
        "suggest","raise","pass","sell","require","report","decide","pull","break","receive",
        "agree","hit","wish","choose","cost","drive","eat","sleep","fight","teach",
        "sing","dance","cook","clean","fix","paint","draw","fly","swim","climb",
        "explain","discuss","improve","reduce","increase","provide","accept","refuse","enjoy","hate",
        "prefer","manage","check","share","compare","prepare","record","realize","imagine","pretend"
    };

    // Irregular verbs (subset of commonVerbs) – useful for special handling
    // Note: this list is a subset; no duplicates with commonVerbs, just a reference.
    static const std::vector<std::string> en_irregularVerbs = {
        "be","have","do","say","go","get","make","know","think","take","see","come",
        "give","tell","find","feel","become","leave","put","mean","keep","let","begin",
        "seem","hear","write","sit","stand","lose","pay","meet","set","lead","understand",
        "speak","read","grow","win","buy","build","fall","cut","choose","drive","eat",
        "fight","teach","sing","swim","throw","wake","wear","break","steal","freeze"
    };

    // Common adverbs (lowercased) – duplicates removed, expanded
    static const std::vector<std::string> en_commonAdverbs = {
        "here","there","near","far","slowly","quickly","well","badly","much","little",
        "never","always","also","neither","only","just","even","forward","backward",
        "up","down","inside","outside","above","below","ahead","behind","around","anywhere",
        "before","after","then","soon","late","early","yesterday","today","tomorrow",
        "tonight","still","yet","already","ever","frequently","often","sometimes",
        "rarely","recently","lately","while","hastily","easily","difficultly","carefully",
        "especially","thus","so","how","where","when","why","better","worse","more",
        "less","almost","approximately","exactly","perhaps","maybe","surely","probably",
        "definitely","yes","no","certainly","indeed","too","as","well",
        "extremely","quite","rather","somehow","anyway","otherwise","abruptly","accidentally",
        "basically","briefly","constantly","directly","entirely","finally","generally"
    };

    // Demonstratives
    static const std::vector<std::string> en_demonstratives = {
        "this","that","these","those"
    };

    // Numerals (common words)
    static const std::vector<std::string> en_numerals = {
        "one","two","three","four","five","six","seven","eight","nine","ten",
        "eleven","twelve","thirteen","fourteen","fifteen","twenty","hundred","thousand","million",
        "first","second","third","fourth","fifth","sixth","seventh","eighth","ninth","tenth"
    };

    // Relative pronouns
    static const std::vector<std::string> en_relatives = {
        "who","whom","whose","which","that"
    };

    // Quantifiers
    static const std::vector<std::string> en_quantifiers = {
        "much","many","little","few","some","any","none","all","both","each",
        "every","several","enough","plenty","more","most","less","least","a lot of","lots of",
        "a bit of","a couple of"
    };

    // Prepositions
    static const std::vector<std::string> en_prepositions = {
        "about","above","across","after","against","along","among","around","at","before",
        "behind","below","beneath","beside","between","beyond","by","down","during","except",
        "for","from","in","inside","into","near","of","off","on","onto","out","outside",
        "over","past","since","through","to","toward","under","until","up","upon","with","within","without",
        "amid","amongst","mid","via"
    };

    // Conjunctions
    static const std::vector<std::string> en_conjunctions = {
        "and","or","but","nor","yet","so","for","because","although","though","since","while",
        "if","when","where","whereas","unless","until","after","before","once","whether",
        "wherever","whoever","provided"
    };

    // Articles
    static const std::vector<std::string> en_articles = {
        "a","an","the"
    };

    // Pronouns (personal, possessive, reflexive implied)
    static const std::vector<std::string> en_pronouns = {
        "i","you","he","she","it","we","they","me","him","her","us","them",
        "my","your","his","her","its","our","their","mine","yours","hers","ours","theirs"
    };

    // Possessives (subset of pronouns, kept for clarity)
    static const std::vector<std::string> en_possessives = {
        "my","your","his","her","its","our","their","mine","yours","hers","ours","theirs"
    };

    // Interrogatives
    static const std::vector<std::string> en_interrogatives = {
        "what","which","who","whom","whose","where","when","why","how"
    };

    // ========================================================================
    // SUFFIX LISTS (morphological)
    // ========================================================================

    // Noun-building suffixes (derivational)
    static const std::vector<std::string> en_nounSuffixes = {
        "tion","sion","ness","ment","ity","ance","ence","er","or","ist",
        "ism","ship","hood","dom","age","al","ure","cy","ty","th","tude","logy","graphy","metry",
        "ion","ation","ition"  // variants
    };

    // Verb-building suffixes (derivational + common inflectional, marked)
    static const std::vector<std::string> en_verbSuffixes = {
        "ate","ify","ize","ise","en",      // derivational
        "ed","ing","es","s","ies"          // inflectional (optional, keep if needed)
    };

    // Adjective-building suffixes
    static const std::vector<std::string> en_adjectiveSuffixes = {
        "ous","ious","ive","ative","able","ible","al","ial","an","ian","ary","ic",
        "ish","like","less","ful","y","ly","ent","ant","some","worthy",
        "ual","tic","fic"  // extended
    };

    // Adverb-building suffixes
    static const std::vector<std::string> en_adverbSuffixes = {
        "ly","ily","ally","wards","wise"
    };

    // ========================================================================
    // Public utility: endsWith (unchanged)
    // ========================================================================
    bool endsWith(const std::string& word, const std::string& suffix) {
        if (suffix.size() > word.size()) return false;
        return word.compare(word.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    // ========================================================================
    // Language‑dispatched functions
    // ========================================================================

    bool isPlural(const std::string& word) {
        if (currentLanguage_ == "en") {
            if (word.size() < 2) return false;
            // Simple: ends with 's' but not 'ss' (like "bus")
            if (endsWith(word, "s") && !endsWith(word, "ss")) return true;
            return false;
        } else {
            // Spanish original logic
            static const std::vector<std::string> es_exceptions = {
                "crisis","martes","paraguas","lunes","miércoles","jueves","viernes","sábado","domingo","tórax","fórceps","virus","atlas","mes","país","cactus"
            };
            if (contains(es_exceptions, word)) return false;
            if (word.size() < 2) return false;
            return word.back() == 's';
        }
    }

    Gender detectGender(const std::string& word) {
        if (currentLanguage_ == "en") {
            return Gender::NEUTER; // English has no grammatical gender
        }
        // Spanish logic
        static const std::vector<std::string> ex_fem = {
            "mapa","día","problema","sistema","idioma","clima","programa","tema"
        };
        static const std::vector<std::string> ex_masc = {
            "mano","radio","moto","foto","modelo","imagen"
        };
        if (contains(ex_fem, word)) return Gender::MASCULINE;
        if (contains(ex_masc, word)) return Gender::FEMININE;
        static const std::vector<std::string> term_fem = {
            "a","ción","sión","dad","tad","umbre","eza","iz","triz"
        };
        static const std::vector<std::string> term_masc = {
            "o","l","n","r","s","ma","ta","pa","aje"
        };
        if (endsWithAny(word, term_fem)) return Gender::FEMININE;
        if (endsWithAny(word, term_masc)) return Gender::MASCULINE;
        return Gender::NEUTER;
    }

    Tense detectTense(const std::string& word) {
        if (currentLanguage_ == "en") {
            // Past: ends with "ed" (but not "eed" like "need" – we ignore edge cases)
            if (endsWith(word, "ed") && word.size() > 2) return Tense::PAST;
            // Future not detectable from suffix alone; return PRESENT
            return Tense::PRESENT;
        } else {
            // Spanish original
            if (endsWith(word, "ó") || endsWith(word, "ió") || endsWith(word, "aba") ||
                endsWith(word, "ía") || endsWith(word, "aste") || endsWith(word, "iste") ||
                endsWith(word, "aron") || endsWith(word, "ieron") || endsWith(word, "ado") ||
                endsWith(word, "ido")) return Tense::PAST;
            if (endsWith(word, "aré") || endsWith(word, "eré") || endsWith(word, "iré") ||
                endsWith(word, "ará") || endsWith(word, "erá") || endsWith(word, "irá"))
                return Tense::FUTURE;
            return Tense::PRESENT;
        }
    }

    Person detectPerson(const std::string& word) {
        if (currentLanguage_ == "en") {
            // 3rd singular present: -s, -es, -ies (but not after vowel like "does" we ignore)
            if (endsWith(word, "s") && !endsWith(word, "ss") && word.size() > 2)
                return Person::THIRD;
            return Person::NONE;
        } else {
            // Spanish original
            if (endsWith(word, "o") && !endsWith(word, "mos") && !endsWith(word, "ís") && !endsWith(word, "n"))
                return Person::FIRST;
            if (endsWith(word, "as") || endsWith(word, "es") || endsWith(word, "ís"))
                return Person::SECOND;
            if (endsWith(word, "a") || endsWith(word, "e"))
                return Person::THIRD;
            if (endsWith(word, "mos")) return Person::FIRST;
            if (endsWith(word, "n")) return Person::THIRD;
            return Person::NONE;
        }
    }

    Degree detectAdjectiveDegree(const std::string& word) {
        if (currentLanguage_ == "en") {
            if (endsWith(word, "est")) return Degree::SUPERLATIVE;
            if (endsWith(word, "er")) return Degree::COMPARATIVE;
            return Degree::POSITIVE;
        } else {
            // Spanish
            if (endsWith(word, "ísimo") || endsWith(word, "érrimo") ||
                endsWith(word, "ísima") || endsWith(word, "érrima"))
                return Degree::SUPERLATIVE;
            if (endsWith(word, "or") && word.size() > 3)
                return Degree::COMPARATIVE;
            return Degree::POSITIVE;
        }
    }
    bool detectLanguage(const std::string text) {
        int en = 0;
        int es = 0;
        if (containsIgnoreCase(en_articles, text)) en ++;
        if (containsIgnoreCase(en_prepositions, text)) en ++;
        if (containsIgnoreCase(en_conjunctions, text)) en ++;
        if (containsIgnoreCase(es_articles, text)) es ++;
        if (containsIgnoreCase(es_prepositions, text)) es ++;
        if (containsIgnoreCase(es_conjunctions, text)) es ++;
        (es > en) ? setLanguage("es") : setLanguage("en");
        return (es > en);
    }
    // ------------------------------------------------------------------------
    // Dictionary lookup
    // ------------------------------------------------------------------------
    bool isCommonWord(const std::string& word, WordType& outTag, float& outConf) {
        if (currentLanguage_ == "en") {
            // English closed‑class words have very high confidence
            if (containsIgnoreCase(en_articles, word))       { outTag = WordType::ARTICLE;       outConf = 0.99f; return true; }
            if (containsIgnoreCase(en_prepositions, word))    { outTag = WordType::PREPOSITION;   outConf = 0.98f; return true; }
            if (containsIgnoreCase(en_conjunctions, word))    { outTag = WordType::CONJUNCTION;   outConf = 0.97f; return true; }
            if (containsIgnoreCase(en_interrogatives, word))  { outTag = WordType::INTERROGATIVE; outConf = 0.96f; return true; }
            if (containsIgnoreCase(en_pronouns, word))        { outTag = WordType::PRONOUN;       outConf = 0.97f; return true; }
            if (containsIgnoreCase(en_possessives, word))     { outTag = WordType::PRONOUN;       outConf = 0.5f; return true; }
            // Detect negation words (high confidence)
            if (containsIgnoreCase(en_negationWords, word))    { outTag = WordType::NEGATION; outConf = 0.99f; return true; }
            if (containsIgnoreCase(en_affirmationWords, word)) { outTag = WordType::AFIRMATION; outConf = 0.80f; return true; }
            // Open class with high confidence
            if (containsIgnoreCase(en_commonNouns, word))      { outTag = WordType::NOUN;      outConf = 0.95f; return true; }
            if (containsIgnoreCase(en_commonAdjectives, word)) { outTag = WordType::ADJECTIVE; outConf = 0.95f; return true; }
            if (containsIgnoreCase(en_irregularVerbs, word) ||
                containsIgnoreCase(en_commonVerbs, word))      { outTag = WordType::VERB;      outConf = 0.95f; return true; }
            if (containsIgnoreCase(en_commonAdverbs, word))    { outTag = WordType::ADVERB;    outConf = 0.95f; return true; }
            if (containsIgnoreCase(en_demonstratives, word))   { outTag = WordType::DEMONSTRATIVE; outConf = 0.95f; return true; }
            if (containsIgnoreCase(en_numerals, word))         { outTag = WordType::NUMERAL;      outConf = 0.95f; return true; }
            if (containsIgnoreCase(en_relatives, word))        { outTag = WordType::RELATIVE;     outConf = 0.95f; return true; }
            if (containsIgnoreCase(en_quantifiers, word))      { outTag = WordType::QUANTIFIER;   outConf = 0.95f; return true; }
            return false;
        } else {
            // Spanish original
            if (contains(es_negationWords, word))       { outTag = WordType::NEGATION; outConf = 0.99f; return true; }
            if (contains(es_affirmationWords, word))    { outTag = WordType::AFIRMATION; outConf = 0.80f; return true; }
            if (contains(es_commonNouns, word))         { outTag = WordType::NOUN; outConf = 0.95f; return true; }
            if (contains(es_commonAdjectives, word))    { outTag = WordType::ADJECTIVE; outConf = 0.95f; return true; }
            if (contains(es_irregularVerbs, word))      { outTag = WordType::VERB; outConf = 0.95f; return true; }
            if (contains(es_commonAdverbs, word))       { outTag = WordType::ADVERB; outConf = 0.95f; return true; }
            if (contains(es_demonstratives, word))      { outTag = WordType::DEMONSTRATIVE; outConf = 0.95f; return true; }
            if (contains(es_numerals, word))            { outTag = WordType::NUMERAL; outConf = 0.95f; return true; }
            if (contains(es_relatives, word))           { outTag = WordType::RELATIVE; outConf = 0.95f; return true; }
            if (contains(es_quantifiers, word))         { outTag = WordType::QUANTIFIER; outConf = 0.95f; return true; }
            if (contains(es_articles, word))            { outTag = WordType::ARTICLE; outConf = 0.99f; return true; }
            if (contains(es_prepositions, word))        { outTag = WordType::PREPOSITION; outConf = 0.98f; return true; }
            if (contains(es_conjunctions, word))        { outTag = WordType::CONJUNCTION; outConf = 0.97f; return true; }
            if (contains(es_interrogatives, word))      { outTag = WordType::INTERROGATIVE; outConf = 0.96f; return true; }
            if (contains(es_pronouns, word))            { outTag = WordType::PRONOUN; outConf = 0.97f; return true; }
            if (contains(es_possessives, word))         { outTag = WordType::PRONOUN; outConf = 0.5f; return true; }
            return false;
        }
    }

    // ------------------------------------------------------------------------
    // Tag validation (morphological plausibility)
    // ------------------------------------------------------------------------
    float validateTag(const std::string& word, WordType tag) {
        if (currentLanguage_ == "en") {
            switch (tag) {
                case WordType::NOUN:
                    return (endsWithAny(word, en_nounSuffixes) || isPlural(word)) ? 0.70f : 0.15f;
                case WordType::VERB:
                    return (endsWithAny(word, en_verbSuffixes) ||
                            containsIgnoreCase(en_irregularVerbs, word) ||
                            containsIgnoreCase(en_commonVerbs, word)) ? 0.75f : 0.15f;
                case WordType::ADJECTIVE:
                    return endsWithAny(word, en_adjectiveSuffixes) ? 0.70f : 0.15f;
                case WordType::ADVERB:
                    return endsWithAny(word, en_adverbSuffixes) ||
                           containsIgnoreCase(en_commonAdverbs, word) ? 0.80f : 0.10f;
                case WordType::INTERROGATIVE:
                    return containsIgnoreCase(en_interrogatives, word) ? 0.96f : 0.0f;
                case WordType::DEMONSTRATIVE:
                    return containsIgnoreCase(en_demonstratives, word) ? 0.90f : 0.10f;
                case WordType::NUMERAL:
                    return containsIgnoreCase(en_numerals, word) ||
                           std::regex_match(word, std::regex("\\d+")) ? 0.85f : 0.10f;
                case WordType::RELATIVE:
                    return containsIgnoreCase(en_relatives, word) ? 0.90f : 0.10f;
                case WordType::QUANTIFIER:
                    return containsIgnoreCase(en_quantifiers, word) ? 0.80f : 0.10f;
                default: return 0.0f;
            }
        } else {
            // Spanish original
            switch (tag) {
                case WordType::NOUN:
                    return endsWithAny(word, es_nounSuffixes) || isPlural(word) ? 0.70f : 0.15f;
                case WordType::VERB:
                    return endsWithAny(word, es_verbSuffixes) ||
                           contains(es_irregularVerbs, word) ? 0.75f : 0.15f;
                case WordType::ADJECTIVE:
                    return endsWithAny(word, es_adjectiveSuffixes) ? 0.70f : 0.15f;
                case WordType::ADVERB:
                    return endsWithAny(word, es_adverbSuffixes) ||
                           contains(es_commonAdverbs, word) ? 0.80f : 0.10f;
                case WordType::INTERROGATIVE:
                    return contains(es_interrogatives, word) ? 0.96f : 0.0f;
                case WordType::DEMONSTRATIVE:
                    return contains(es_demonstratives, word) ? 0.90f : 0.10f;
                case WordType::NUMERAL:
                    return contains(es_numerals, word) ||
                           std::regex_match(word, std::regex("\\d+")) ? 0.85f : 0.10f;
                case WordType::RELATIVE:
                    return contains(es_relatives, word) ? 0.90f : 0.10f;
                case WordType::QUANTIFIER:
                    return contains(es_quantifiers, word) ? 0.80f : 0.10f;
                default: return 0.0f;
            }
        }
    }

    // ------------------------------------------------------------------------
    // Initial tag guess
    // ------------------------------------------------------------------------
    WordType guessInitialTag(const std::string& word) {
        if (currentLanguage_ == "en") {
            if (containsIgnoreCase(en_articles, word))        return WordType::ARTICLE;
            if (containsIgnoreCase(en_prepositions, word))    return WordType::PREPOSITION;
            if (containsIgnoreCase(en_conjunctions, word))    return WordType::CONJUNCTION;
            if (containsIgnoreCase(en_interrogatives, word))  return WordType::INTERROGATIVE;
            WordType commTag; float dummy;
            if (isCommonWord(word, commTag, dummy)) return commTag;
            if (endsWithAny(word, en_verbSuffixes))      return WordType::VERB;
            if (endsWithAny(word, en_adjectiveSuffixes)) return WordType::ADJECTIVE;
            if (endsWithAny(word, en_adverbSuffixes))    return WordType::ADVERB;
            if (endsWithAny(word, en_nounSuffixes))      return WordType::NOUN;
            return WordType::UNDEFINED;
        } else {
            // Spanish original
            if (contains(es_articles, word))        return WordType::ARTICLE;
            if (contains(es_prepositions, word))    return WordType::PREPOSITION;
            if (contains(es_conjunctions, word))    return WordType::CONJUNCTION;
            if (contains(es_interrogatives, word))  return WordType::INTERROGATIVE;
            WordType commTag; float dummy;
            if (isCommonWord(word, commTag, dummy)) return commTag;
            if (endsWithAny(word, es_verbSuffixes))      return WordType::VERB;
            if (endsWithAny(word, es_adjectiveSuffixes)) return WordType::ADJECTIVE;
            if (endsWithAny(word, es_adverbSuffixes))    return WordType::ADVERB;
            if (endsWithAny(word, es_nounSuffixes))      return WordType::NOUN;
            return WordType::UNDEFINED;
        }
    }

    // ------------------------------------------------------------------------
    // Suffix probability
    // ------------------------------------------------------------------------
    float getSuffixProb(const std::string& word, WordType tag) {
        if (currentLanguage_ == "en") {
            if (tag == WordType::ARTICLE      && containsIgnoreCase(en_articles, word))       return 0.99f;
            if (tag == WordType::PREPOSITION  && containsIgnoreCase(en_prepositions, word))    return 0.98f;
            if (tag == WordType::CONJUNCTION  && containsIgnoreCase(en_conjunctions, word))    return 0.97f;
            if (tag == WordType::INTERROGATIVE && containsIgnoreCase(en_interrogatives, word)) return 0.96f;
            WordType commTag; float commonConf;
            if (isCommonWord(word, commTag, commonConf) && commTag == tag) return commonConf;
            float base = (word.size() < 3) ? 0.01f : 0.05f;
            switch (tag) {
                case WordType::NOUN:    return endsWithAny(word, en_nounSuffixes)      ? 0.35f : base;
                case WordType::VERB:    return endsWithAny(word, en_verbSuffixes)      ? 0.40f : base;
                case WordType::ADJECTIVE: return endsWithAny(word, en_adjectiveSuffixes) ? 0.35f : base;
                case WordType::ADVERB:  return endsWithAny(word, en_adverbSuffixes)    ? 0.70f : base;
                case WordType::DEMONSTRATIVE: return containsIgnoreCase(en_demonstratives, word) ? 0.90f : base;
                case WordType::NUMERAL:      return containsIgnoreCase(en_numerals, word) ? 0.85f : base;
                case WordType::RELATIVE:     return containsIgnoreCase(en_relatives, word) ? 0.85f : base;
                case WordType::QUANTIFIER:   return containsIgnoreCase(en_quantifiers, word) ? 0.80f : base;
                default: return base;
            }
        } else {
            if (tag == WordType::ARTICLE      && contains(es_articles, word))       return 0.99f;
            if (tag == WordType::PREPOSITION  && contains(es_prepositions, word))    return 0.98f;
            if (tag == WordType::CONJUNCTION  && contains(es_conjunctions, word))    return 0.97f;
            if (tag == WordType::INTERROGATIVE && contains(es_interrogatives, word)) return 0.96f;
            WordType commTag; float commonConf;
            if (isCommonWord(word, commTag, commonConf) && commTag == tag) return commonConf;
            float base = (word.size() < 3) ? 0.01f : 0.05f;
            switch (tag) {
                case WordType::NOUN:    return endsWithAny(word, es_nounSuffixes)      ? 0.35f : base;
                case WordType::VERB:    return endsWithAny(word, es_verbSuffixes)      ? 0.40f : base;
                case WordType::ADJECTIVE: return endsWithAny(word, es_adjectiveSuffixes) ? 0.35f : base;
                case WordType::ADVERB:  return endsWithAny(word, es_adverbSuffixes)    ? 0.70f : base;
                case WordType::DEMONSTRATIVE: return contains(es_demonstratives, word) ? 0.90f : base;
                case WordType::NUMERAL:      return contains(es_numerals, word) ? 0.85f : base;
                case WordType::RELATIVE:     return contains(es_relatives, word) ? 0.85f : base;
                case WordType::QUANTIFIER:   return contains(es_quantifiers, word) ? 0.80f : base;
                default: return base;
            }
        }
    }

    // ========================================================================
    // Closed‑class detectors (language‑aware)
    // ========================================================================
    bool isArticle(const std::string& word) {
        if (currentLanguage_ == "en") return containsIgnoreCase(en_articles, word);
        return contains(es_articles, word);
    }

    bool isPreposition(const std::string& word) {
        if (currentLanguage_ == "en") return containsIgnoreCase(en_prepositions, word);
        return contains(es_prepositions, word);
    }

    bool isConjunction(const std::string& word) {
        if (currentLanguage_ == "en") return containsIgnoreCase(en_conjunctions, word);
        return contains(es_conjunctions, word);
    }

    bool isInterrogative(const std::string& word) {
        if (currentLanguage_ == "en") return containsIgnoreCase(en_interrogatives, word);
        return contains(es_interrogatives, word);
    }

    bool isDemonstrative(const std::string& word) {
        if (currentLanguage_ == "en") return containsIgnoreCase(en_demonstratives, word);
        return contains(es_demonstratives, word);
    }

    bool isNumeral(const std::string& word) {
        if (currentLanguage_ == "en") {
            return containsIgnoreCase(en_numerals, word) ||
                   std::regex_match(word, std::regex("\\d+"));
        }
        return contains(es_numerals, word) || std::regex_match(word, std::regex("\\d+"));
    }

    bool isRelativePronoun(const std::string& word) {
        if (currentLanguage_ == "en") return containsIgnoreCase(en_relatives, word);
        return contains(es_relatives, word);
    }

    bool isQuantifier(const std::string& word) {
        if (currentLanguage_ == "en") return containsIgnoreCase(en_quantifiers, word);
        return contains(es_quantifiers, word);
    }

} // namespace morphology
