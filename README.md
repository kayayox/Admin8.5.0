```
# NLPEngine – Natural Language Processing Engine

![CI status](https://github.com/kayayox/Admin8.5.0/actions/workflows/ci.yml/badge.svg)

A modular C++17 engine for tokenization, morphological analysis, part-of-speech tagging, pattern learning, contextual prediction, and dialogue generation. Supports Spanish and English.

## Features

- **Text processing**: Tokenization, sentence splitting, POS tagging (morphology + context).
- **Machine learning**: N‑gram tag statistics (unigram, bigram, trigram) with smoothing.
- **Pattern correlation**: Learn word/chunk/letter/syllable trigrams stored in SQLite.
- **Dialogue generation**: Hypothesis generation using templates, inference rules, or correlators.
- **Persistence**: All data (words, sentences, dialogues, patterns) stored in SQLite databases.
- **Multilingual**: Spanish and English (easily extensible).

## Requirements

- C++17 compiler (GCC 7+, Clang 6+, MSVC 2019+)
- CMake 3.14+
- SQLite3 (optional – automatically downloaded if not found on system)

## Build Instructions

```bash
# Clone the repository
git clone https://github.com/your-repo/nlpengine.git
cd nlpengine

# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# (Optional) Install
cmake --install . --prefix /usr/local
```

CMake options:

- `-DBUILD_EXECUTABLE=ON`   (builds the demo executable, default ON)
- `-DBUILD_STATIC_LIB=OFF`  (build static library)
- `-DBUILD_SHARED_LIB=OFF`  (build shared library)
- `-DFORCE_DOWNLOAD_SQLITE3=OFF` (force download SQLite3 even if system has it)

## Quick Usage (C++ API)

```cpp
#include "NLPEngine.hpp"

int main() {
    NLPEngine engine;
    if (!engine.initialize("semantic.db", "patterns.db")) {
        return 1;
    }

    // Set language (es or en)
    engine.setLanguage("en");

    // Process a sentence
    auto words = engine.processSentence("The cat sits on the mat.");

    // Generate a response
    std::string response = engine.generateResponse("What is your name?");
    std::cout << response << std::endl;

    return 0;
}
```

## Project Structure

```
src/
├── api/           – Public facade (NLPEngine)
├── core/          – Domain entities (Word, Sentence, Pattern, Dialogue…)
├── db/            – SQLite repositories
├── dialogue/      – Correlators, chunking, slot filling
├── nlp/           – Tokenizer, morphology, classifier, tag stats
└── utils/         – Helpers, string conversions, templates
```

## Databases

The engine uses two SQLite databases:

- **semantic.db**: words, sentences, dialogues, feedback.
- **patterns.db**: tag n‑grams, pattern correlations (with optional suffix for chunks/letters/syllables).

All tables are created automatically.

## Language Support

- **Spanish** and **English** are built-in.
- Set language with `engine.setLanguage("es")` or `"en"`.
- Morphology, tag statistics, and response templates adapt accordingly.

## Limitations & Future Work

- Dialogue generation.
- Inference rules are hardcoded; external JSON configuration will be added.
- Word repository does not separate by language – for homographs, use separate databases or table suffix.

## License

MIT License

---

# NLPEngine – Motor de Procesamiento de Lenguaje Natural

Motor modular en C++17 para tokenización, análisis morfológico, etiquetado gramatical, aprendizaje de patrones, predicción contextual y generación de diálogos. Soporta español e inglés.

## Características

- **Procesamiento de texto**: tokenización, segmentación de oraciones, etiquetado POS (morfología + contexto).
- **Aprendizaje automático**: estadísticas n‑gram (unigrama, bigrama, trigrama) con suavizado.
- **Correlación de patrones**: aprende trigramas de palabras, fragmentos, letras o sílabas almacenados en SQLite.
- **Generación de diálogo**: genera hipótesis usando plantillas, reglas de inferencia o correladores.
- **Persistencia**: todos los datos (palabras, oraciones, diálogos, patrones) se guardan en bases SQLite.
- **Multilingüe**: español e inglés (fácilmente extensible).

## Requisitos

- Compilador C++17 (GCC 7+, Clang 6+, MSVC 2019+)
- CMake 3.14+
- SQLite3 (opcional – se descarga automáticamente si no está en el sistema)

## Compilación

```bash
# Clonar el repositorio
git clone https://github.com/tu-repo/nlpengine.git
cd nlpengine

# Configurar y compilar
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# (Opcional) Instalar
cmake --install . --prefix /usr/local
```

Opciones de CMake:

- `-DBUILD_EXECUTABLE=ON`   (compila el ejecutable de demostración, activado por defecto)
- `-DBUILD_STATIC_LIB=OFF`  (compila biblioteca estática)
- `-DBUILD_SHARED_LIB=OFF`  (compila biblioteca dinámica)
- `-DFORCE_DOWNLOAD_SQLITE3=OFF` (forzar descarga de SQLite3 aunque el sistema lo tenga)

## Uso rápido (API C++)

```cpp
#include "NLPEngine.hpp"

int main() {
    NLPEngine engine;
    if (!engine.initialize("semantic.db", "patterns.db")) {
        return 1;
    }

    // Cambiar idioma (es o en)
    engine.setLanguage("es");

    // Procesar una oración
    auto words = engine.processSentence("El gato está sobre la alfombra.");

    // Generar respuesta
    std::string respuesta = engine.generateResponse("¿Cómo te llamas?");
    std::cout << respuesta << std::endl;

    return 0;
}
```

## Estructura del proyecto

```
src/
├── api/          – Fachada pública (NLPEngine)
├── core/         – Entidades del dominio (Word, Sentence, Pattern, Dialogue…)
├── db/           – Repositorios SQLite
├── dialogue/     – Correladores, fragmentación, llenado de slots
├── nlp/          – Tokenizador, morfología, clasificador, estadísticas de etiquetas
└── utils/        – Utilidades, conversiones de cadenas, plantillas
```

## Bases de datos

El motor usa dos bases de datos SQLite:

- **semantic.db**: palabras, oraciones, diálogos, retroalimentación.
- **patterns.db**: n‑gramas de etiquetas, correlaciones de patrones (con sufijos opcionales para fragmentos/letras/sílabas).

Todas las tablas se crean automáticamente.

## Soporte de idiomas

- **Español** e **inglés** incluidos.
- Cambiar idioma con `engine.setLanguage("es")` o `"en"`.
- Morfología, estadísticas de etiquetas y plantillas de respuesta se adaptan automáticamente.

## Limitaciones y trabajo futuro

- La generación de diálogo aún es rudimentaria.
- Las reglas de inferencia están fijas en el código; se añadirá configuración externa (JSON).
- El repositorio de palabras no separa por idioma – para palabras homógrafas se recomienda usar bases de datos separadas o sufijo en tablas.

## Licencia

MIT License
```
