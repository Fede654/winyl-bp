# PLAN DE LIMPIEZA Y ORGANIZACIÓN DEL PROYECTO

## 📁 ANÁLISIS DE ARCHIVOS NO RASTREADOS

### ✅ **MANTENER** (Documentación de la Quest)
```
docs/
├── AUDIO_GEEK_REQUIREMENTS_SPECIFICATION.md ✅
├── BASS_ARCHITECTURE_REFERENCE.md ✅
├── BASS_MODERN_CAPABILITIES_EXPLORATION.md ✅
├── HISTORICAL_ISSUES_TECHNICAL_ANALYSIS.md ✅
├── PROJECT_QUEST_MANIFEST.md ✅ 
├── THE_DEVELOPER_NARRATIVE_ALEX_KRAS.md ✅
├── WASAPI_EXCLUSIVE_MODE.md ✅
└── EQUALIZER_IMPLEMENTATION_CHANGE.md ✅
```

### ✅ **MANTENER** (Herramientas y Datos de la Quest)
```
extract_developer_narrative.py ✅ (Script sistemático de análisis)
developer_narrative_analysis.json ✅ (4,874 comentarios extraídos)
equalizer_presets_professional.txt ✅ (Presets técnicos para audio geeks)
```

### 🔄 **EVALUAR Y REORGANIZAR**
```
PERFECT_PLAYER_IDEAS.md → Mover a docs/LEGACY_PERFECT_PLAYER_IDEAS.md
```

### ❌ **ELIMINAR** (Archivos temporales/inválidos)
```
nul ❌ (Archivo inválido de error de comando)
build_log_*.txt ❌ (70+ archivos de logs de build - solo mantener el más reciente)
```

---

## 🗂️ ESTRUCTURA OBJETIVO

### Raíz del Proyecto (Limpia)
```
winyl-bp/
├── README.md
├── CLAUDE.md  
├── LICENSE
├── Winyl.sln
├── build.bat
├── extract_developer_narrative.py
├── developer_narrative_analysis.json
├── equalizer_presets_professional.txt
├── docs/
├── Winyl/
└── .gitignore (actualizado)
```

### Carpeta docs/ Organizada
```
docs/
├── quest/
│   ├── PROJECT_QUEST_MANIFEST.md
│   ├── THE_DEVELOPER_NARRATIVE_ALEX_KRAS.md
│   ├── HISTORICAL_ISSUES_TECHNICAL_ANALYSIS.md
│   └── LEGACY_PERFECT_PLAYER_IDEAS.md
├── technical/
│   ├── BASS_ARCHITECTURE_REFERENCE.md
│   ├── BASS_MODERN_CAPABILITIES_EXPLORATION.md
│   ├── WASAPI_EXCLUSIVE_MODE.md
│   └── EQUALIZER_IMPLEMENTATION_CHANGE.md
├── specifications/
│   └── AUDIO_GEEK_REQUIREMENTS_SPECIFICATION.md
└── archive/
    └── winyl-modernization/ (documentación legacy)
```

---

## 🧹 ACCIONES DE LIMPIEZA

### 1. Eliminar Archivos Temporales
- ❌ `nul` (archivo de error)
- ❌ Mantener solo `build_log_latest.txt`, eliminar el resto
- ❌ Archivos .obj en build directories si existen

### 2. Reorganizar Documentación
- 🔄 Crear subcarpetas en docs/
- 🔄 Mover documentos según categoría
- 🔄 Actualizar links en README.md

### 3. Actualizar .gitignore
- ➕ Ignorar build logs excepto el último
- ➕ Ignorar archivos temporales del sistema
- ➕ Ignorar archivos de IDEs

---

## 📋 PLAN DE EJECUCIÓN

### Paso 1: Limpieza Inmediata
1. Eliminar archivo `nul`
2. Limpiar build logs (mantener solo el más reciente)
3. Verificar que no hay archivos temporales

### Paso 2: Reorganización Documental
1. Crear estructura de carpetas en docs/
2. Mover archivos según categorización
3. Actualizar README.md con nueva estructura

### Paso 3: Git Management
1. Stagear archivos que deben ser rastreados
2. Actualizar .gitignore con nuevas reglas
3. Commit de organización

---

## 🎯 RESULTADO ESPERADO

**Raíz Limpia**: Solo archivos esenciales y documentación organizada
**Documentación Estructurada**: Fácil navegación por tema/propósito  
**Git Limpio**: Solo archivos relevantes rastreados
**Build System**: Funcional sin archivos temporales

**Principio**: La organización debe reflejar la naturaleza arqueológica del proyecto - documentación de quest claramente separada de documentación técnica.