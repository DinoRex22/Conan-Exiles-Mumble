// Copyright The Mumble Developers. All rights reserved. 
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at https://www.mumble.info/LICENSE.

// The following conditions apply exclusively to the code authored by Dino_Rex and do not affect or modify the copyright or licensing terms of the original Mumble code.

// MIT License

// The following conditions apply exclusively to the code written by Dino_Rex 
// and do not affect or modify the copyright 
// or licensing terms of the original Mumble code.

/*
Mozilla Public License Version 2.0 + Network Use Clause

Copyright(c) 2025 Dino_Rex

This software is licensed under the Mozilla Public License 2.0 (MPL - 2.0),
available at : https://www.mozilla.org/MPL/2.0/

Additional Network Use Clause :
If you modify this software and deploy or use it to provide a service accessible
to others over a network(including via web, API, or remote access),
you must make the complete corresponding source code of your modified version
publicly available under this same license(MPL - 2.0 + Network Use Clause),
including all changes and additions you made.

All other terms and conditions of the Mozilla Public License 2.0 remain unchanged.
*/

#include "MumblePlugin_v_1_0_x.h"
#include "plugin.h"
#include "resource.h"
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <process.h>
#include <wchar.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <locale.h>
#include <ctype.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Msimg32.lib")

#ifndef IDB_CHECKMARK
#define IDB_CHECKMARK 103
#endif

// ============================================================================
// DÉFINITIONS DES VARIABLES GLOBALES | GLOBAL VARIABLE DEFINITIONS
// ============================================================================

// Background image | Image de fond
HBITMAP hBackgroundBitmap = NULL;
HBITMAP hBackgroundAdvancedBitmap = NULL;
HBITMAP hBackgroundPresetsBitmap = NULL;
HBITMAP hBackgroundSavePresetBitmap = NULL;
HBITMAP hBackgroundRenamePresetBitmap = NULL;
static BOOL backgroundDrawn = FALSE;

// Plugin control variables | Variables de contrôle du plugin
BOOL enableGetPlayerCoordinates = TRUE;
BOOL TEMP = FALSE;

// Log control variables | Variables pour contrôler l'activation des logs
BOOL enableLogCoordinates = FALSE;
BOOL enableLogModFile = FALSE;
BOOL enableLogConfig = FALSE;
BOOL enableLogGeneral = FALSE;

// Mumble API interface | Interface API Mumble
struct MumbleAPI_v_1_0_x mumbleAPI;
mumble_plugin_id_t ownID;

// Audio distance variables | Variables de distance audio
double serverMaximumAudioDistance = 45.0;
BOOL maxAudioDistanceRetrieved = FALSE;
ULONGLONG lastMaxDistanceCheck = 0;

// Overlay border highlight variables | Variables de surbrillance de l'overlay
BOOL overlayBorderHighlight = FALSE;
mumble_userid_t overlayHighlightUserID = 0;
char overlaySpeakerText[128] = "";
CRITICAL_SECTION overlayTextLock;

// Channel management variables | Variables de gestion des canaux
mumble_channelid_t hubChannelID = -1;
mumble_channelid_t rootChannelID = -1;
mumble_channelid_t ingameChannelID = -1;
mumble_channelid_t lastTargetChannel = -1;
mumble_channelid_t lastValidChannel = -1;
BOOL channelManagementActive = FALSE;
BOOL enableAutomaticChannelChange = FALSE;
ULONGLONG lastChannelCheck = 0;

// Player position variables | Variables de position du joueur
float axe_x = 0.0f;
float axe_y = 0.0f;
float axe_z = 0.0f;
float avatarAxisX = 0.0f;
float avatarAxisY = 0.0f;
float avatarAxisZ = 0.0f;

// Adaptive mod system variables | Variables pour le système de Mod adaptatif
time_t lastFileCheck = 0;
time_t LastFileModification = 0;
int lastSeq = -1;
BOOL modDataValid = FALSE;
char modFilePath[MAX_PATH] = "";
BOOL coordinatesValid = FALSE;
struct ModFileData currentModData = { 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, FALSE };
ULONGLONG lastModDataTick = 0;

// Global variables for the interface | Variables globales pour l'interface
HWND hConfigDialog = NULL;
HWND hWhisperKeyEdit, hNormalKeyEdit, hShoutKeyEdit, hConfigKeyEdit;
HWND hWhisperButton, hNormalButton, hShoutButton, hConfigButton;
HWND hEnableDistanceMutingCheck, hEnableAutomaticChannelChangeCheck;
HWND hDistanceWhisperEdit, hDistanceNormalEdit, hDistanceShoutEdit;
HWND hSavedPathEdit, hSavedPathButton, hSavedPathBg;
HBITMAP hPathBoxBitmap = NULL;
HWND hCategoryPatch, hCategoryAdvanced;
HWND hEnableVoiceToggleCheck, hVoiceToggleKeyEdit, hVoiceToggleButton;
VoiceRangePreset voicePresets[MAX_VOICE_PRESETS];
int currentPresetIndex = -1;
HWND hCategoryPresets = NULL;
HWND hPresetLabels[MAX_VOICE_PRESETS] = { NULL };
HWND hPresetLoadButtons[MAX_VOICE_PRESETS] = { NULL };
HWND hPresetRenameButtons[MAX_VOICE_PRESETS] = { NULL };
HWND hPresetSaveDialog = NULL;
HWND hPresetRenameDialog = NULL;
char renameBuffer[PRESET_NAME_MAX_LENGTH] = "";
int renamePresetIndex = -1;
HWND hStatusMessage = NULL;
HWND hDistanceLimitMessage = NULL;
HFONT hFont = NULL, hFontBold = NULL, hFontLarge = NULL, hFontEmoji = NULL;
HFONT hPathFont = NULL;

// Interface message controls | Contrôles de messages de l'interface
HWND hDistanceWhisperMessage = NULL;
HWND hDistanceNormalMessage = NULL;
HWND hDistanceShoutMessage = NULL;
HWND hDistanceMutingMessage = NULL;
HWND hChannelSwitchingMessage = NULL;
HWND hPositionalAudioMessage = NULL;
HWND hDistanceServerLimitWhisper = NULL;
HWND hDistanceServerLimitNormal = NULL;
HWND hDistanceServerLimitShout = NULL;

// Interface state variables | Variables d'état de l'interface
int currentCategory = 1;
BOOL isCapturingKey = FALSE;
int captureKeyTarget = 0;
wchar_t savedPath[MAX_PATH] = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles\\ConanSandbox\\Saved";
wchar_t displayedPathText[MAX_PATH] = L"";
BOOL isUpdatingInterface = FALSE;
ULONGLONG lastInterfaceUpdate = 0;

// Voice toggle variables | Variables pour le toggle de voix
int voiceToggleKey = 84;
BOOL enableVoiceToggle = FALSE;
ULONGLONG lastVoiceTogglePress = 0;

// Key bindings | Raccourcis clavier
int whisperKey = 17;
int normalKey = 86;
int shoutKey = 16;
int configUIKey = 121;

// Key monitoring variables | Variables de surveillance des touches globales
BOOL isConfigDialogOpen = FALSE;
DWORD lastKeyPressTime = 0;
BOOL keyMonitorThreadRunning = FALSE;
HANDLE keyMonitorThread = NULL;
BOOL lastKeyState = FALSE;

// Connection and hub state variables | Variables pour détecter l'état de connexion et hub
BOOL isConnectedToServer = FALSE;
BOOL hubDescriptionAvailable = FALSE;
BOOL hubLimitsActive = FALSE;
ULONGLONG lastConnectionCheck = 0;

// Hub audio parameters | Paramètres audio du hub
double hubAudioMinDistance = 2.0;
double hubAudioMaxDistance = 50.0;
double hubAudioMaxVolume = 85.0;
double hubAudioBloom = 0.3;
double hubAudioFilterIntensity = 0.5;
BOOL hubForcePositionalAudio = FALSE;
ULONGLONG lastHubDescriptionCheck = 0;
char* lastHubDescriptionCache = NULL;

// Hub distance limits | Limites de distance du hub
double hubMinimumWhisper = 0.0;
double hubMaximumWhisper = 5.0;
double hubMinimumNormal = 5.0;
double hubMaximumNormal = 15.0;
double hubMinimumShout = 15.0;
double hubMaximumShout = 50.0;
BOOL hubForceDistanceBasedMuting = FALSE;
BOOL hubForceAutomaticChannelSwitching = FALSE;

// Voice system variables | Variables globales pour le système de voix
CompletePositionalData localVoiceData = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f, "" };
CompletePositionalData remotePlayersData[64];
size_t remotePlayerCount = 0;
ULONGLONG lastVoiceDataSent = 0;
ULONGLONG lastKeyCheck = 0;

// Voice distance settings | Paramètres de distance vocale
float distanceWhisper = 2.0f;
float distanceNormal = 15.0f;
float distanceShout = 50.0f;

// Voice features | Fonctionnalités vocales
BOOL enableDistanceMuting = FALSE;

// Overlay variables | Variables globales pour l'overlay
HWND hVoiceOverlay = NULL;
HWND hVoiceText = NULL;
BOOL enableVoiceOverlay = TRUE;
HFONT hOverlayFont = NULL;
BOOL overlayThreadRunning = FALSE;

// Mute system variables | Variables globales pour le système de mute
PlayerMuteState playerMuteStates[64];
size_t playerMuteStateCount = 0;
ULONGLONG lastDistanceCheck = 0;

// Refresh variables | Variables pour forcer un refresh complet des états de mute
BOOL forceGlobalMuteRefresh = FALSE;
ULONGLONG lastGlobalRefresh = 0;

// Automatic audio settings variables | Variables pour le système audio automatique
BOOL enableAutoAudioSettings = TRUE;
ULONGLONG lastAudioSettingsApply = 0;

// Adaptive system variables | Variables globales pour le système adaptatif
AdaptivePlayerData adaptivePlayerStates[64];
size_t adaptivePlayerCount = 0;
Vector3 localPlayerPosition = { 0.0f, 0.0f, 0.0f };

// Audio volume states | États de volume audio
AudioVolumeState audioVolumeStates[64];
size_t audioVolumeCount = 0;

// Low pass filter variables | Variables pour le filtre passe-bas
LowPassFilterState lowPassStates[64];
size_t lowPassStateCount = 0;

// Air diffusion option | Option pour activer la diffusion d'air
BOOL enableAirDiffusion = TRUE;

// ============================================================================
// MODULE 1 : UTILITAIRES DE BASE (appelés par TOUT le monde)
// ============================================================================
// Convert key codes to names | Conversion des codes de touches en noms
static const char* getKeyName(int vkCode) {
    switch (vkCode) {
    case 17: return "Ctrl";
    case 16: return "Shift";
    case 18: return "Alt";
    case 32: return "Space";
    case 13: return "Enter";
    case 27: return "Escape";
    case 9: return "Tab";
    case 8: return "Backspace";
    case 46: return "Delete";
    case 36: return "Home";
    case 35: return "End";
    case 33: return "Page Up";
    case 34: return "Page Down";
    case 45: return "Insert";
    case 20: return "Caps Lock";
    case 144: return "Num Lock";
    case 145: return "Scroll Lock";
    case 37: return "Left Arrow";
    case 38: return "Up Arrow";
    case 39: return "Right Arrow";
    case 40: return "Down Arrow";
    case 112: return "F1"; case 113: return "F2"; case 114: return "F3"; case 115: return "F4";
    case 116: return "F5"; case 117: return "F6"; case 118: return "F7"; case 119: return "F8";
    case 120: return "F9"; case 121: return "F10"; case 122: return "F11"; case 123: return "F12";
    case 65: return "A"; case 66: return "B"; case 67: return "C"; case 68: return "D";
    case 69: return "E"; case 70: return "F"; case 71: return "G"; case 72: return "H";
    case 73: return "I"; case 74: return "J"; case 75: return "K"; case 76: return "L";
    case 77: return "M"; case 78: return "N"; case 79: return "O"; case 80: return "P";
    case 81: return "Q"; case 82: return "R"; case 83: return "S"; case 84: return "T";
    case 85: return "U"; case 86: return "V"; case 87: return "W"; case 88: return "X";
    case 89: return "Y"; case 90: return "Z";
    case 48: return "0"; case 49: return "1"; case 50: return "2"; case 51: return "3";
    case 52: return "4"; case 53: return "5"; case 54: return "6"; case 55: return "7";
    case 56: return "8"; case 57: return "9";
    case 96: return "Num 0"; case 97: return "Num 1"; case 98: return "Num 2"; case 99: return "Num 3";
    case 100: return "Num 4"; case 101: return "Num 5"; case 102: return "Num 6"; case 103: return "Num 7";
    case 104: return "Num 8"; case 105: return "Num 9"; case 106: return "Num *"; case 107: return "Num +";
    case 109: return "Num -"; case 110: return "Num ."; case 111: return "Num /";
    case 186: return ";"; case 187: return "="; case 188: return ","; case 189: return "-";
    case 190: return "."; case 191: return "/"; case 192: return "`"; case 219: return "[";
    case 220: return "\\"; case 221: return "]"; case 222: return "'";
    case 1: return "Left Click"; case 2: return "Right Click"; case 4: return "Middle Click";
    case 5: return "X1 Mouse"; case 6: return "X2 Mouse";
    case 0: return "None";
    default: {
        static char buffer[16];
        snprintf(buffer, sizeof(buffer), "Key_%d", vkCode);
        return buffer;
    }
    }
}

// Display chat message | Afficher un message de chat
static void displayInChat(const char* message) {
    mumbleAPI.log(ownID, message);
}

// Get configuration folder path | Obtenir le chemin du dossier de configuration
wchar_t* getConfigFolderPath() {
    static wchar_t configPath[MAX_PATH];
    PWSTR documentsPath = NULL;

    // Get Documents folder | Obtenir le dossier Documents
    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Documents, 0, NULL, &documentsPath))) {
        // Create full path | Créer le chemin complet
        swprintf(configPath, MAX_PATH, L"%s\\Conan Exiles Mumble plugin", documentsPath);

        // Create folder if it doesn't exist | Créer le dossier s'il n'existe pas
        CreateDirectoryW(configPath, NULL);

        CoTaskMemFree(documentsPath);
        return configPath;
    }

    return NULL;
}

// Check if patch is already saved | Vérifier si le patch est déjà sauvegardé
int isPatchAlreadySaved() {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) {
        return 0;
    }

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    FILE* file = _wfopen(configFile, L"r");
    if (!file) {
        return 0;
    }
    wchar_t line[512];
    int found = 0;
    while (fgetws(line, 512, file)) {
        if (wcsncmp(line, L"SavedPath=", 10) == 0) {
            // Check if there's content after 'SavedPath=' | Vérifier qu'il y a du contenu après 'SavedPath='
            wchar_t* value = line + 10;
            while (*value == L' ' || *value == L'\t') value++;
            if (*value != L'\0' && *value != L'\n' && *value != L'\r') {
                found = 1;
                break;
            }
        }
    }
    fclose(file);
    return found;
}

// Calculate distance between two points | Calculer la distance entre deux points
static float calculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// Calculate 3D distance | Calculer la distance 3D entre deux points
static float calculateDistance3D(Vector3* a, Vector3* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// Count significant digits in value | Compter les chiffres significatifs dans une valeur
static int countSignificantDigits(float value) {
    if (value == 0.0f) return 1;

    int integerPart = (int)value;
    if (integerPart == 0) return 1;

    int digitCount = 0;
    while (integerPart > 0) {
        digitCount++;
        integerPart /= 10;
    }
    return digitCount;
}

// ============================================================================
// MODULE 2 : CONFIGURATION ET FICHIERS
// ============================================================================
static void loadVoiceDistancesFromConfig() {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    FILE* f = _wfopen(configFile, L"r");
    if (f) {
        wchar_t line[1024];
        while (fgetws(line, 1024, f)) {
            wchar_t* p = line;
            while (*p == L' ' || *p == L'\t') ++p;
            wchar_t* end = p + wcslen(p);
            while (end > p && (end[-1] == L'\r' || end[-1] == L'\n' || end[-1] == L' ' || end[-1] == L'\t'))
                *--end = L'\0';

            if (*p == L'#' || *p == L';' || *p == L'\0') continue;

            wchar_t* eq = wcschr(p, L'=');
            if (!eq) continue;
            *eq = L'\0';
            wchar_t* key = p;
            wchar_t* val = eq + 1;
            while (*val == L' ' || *val == L'\t') ++val;

            if (wcsncmp(key, L"DistanceWhisper", 15) == 0) {
                distanceWhisper = (float)_wtof(val);
            }
            else if (wcsncmp(key, L"DistanceNormal", 14) == 0) {
                distanceNormal = (float)_wtof(val);
            }
            else if (wcsncmp(key, L"DistanceShout", 13) == 0) {
                distanceShout = (float)_wtof(val);
            }
        }
        fclose(f);
    }

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Voice distances loaded from config - Whisper: %.1fm, Normal: %.1fm, Shout: %.1fm",
            distanceWhisper, distanceNormal, distanceShout);
        mumbleAPI.log(ownID, logMsg);
    }
}

static void readConfigurationSettings() {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    BOOL fileExists = (GetFileAttributesW(configFile) != INVALID_FILE_ATTRIBUTES);
    BOOL foundSavedPath = FALSE;
    BOOL foundEnableAutomaticChannelChange = FALSE;
    BOOL foundDistanceWhisper = FALSE;
    BOOL foundDistanceNormal = FALSE;
    BOOL foundDistanceShout = FALSE;

    wchar_t savedPathValue[MAX_PATH] = L"";

    if (fileExists) {
        FILE* f = _wfopen(configFile, L"r");
        if (f) {
            wchar_t line[1024];
            while (fgetws(line, 1024, f)) {
                wchar_t* p = line;
                while (*p == L' ' || *p == L'\t') ++p;
                wchar_t* end = p + wcslen(p);
                while (end > p && (end[-1] == L'\r' || end[-1] == L'\n' || end[-1] == L' ' || end[-1] == L'\t'))
                    *--end = L'\0';

                if (*p == L'#' || *p == L';' || *p == L'\0') continue;

                wchar_t* eq = wcschr(p, L'=');
                if (!eq) continue;
                *eq = L'\0';
                wchar_t* key = p;
                wchar_t* val = eq + 1;
                while (*val == L' ' || *val == L'\t') ++val;

                wchar_t valLower[16] = { 0 };
                int i = 0;
                for (; i < 15 && val[i]; ++i) valLower[i] = (wchar_t)towlower(val[i]);
                valLower[i] = 0;

                if (wcsncmp(key, L"SavedPath", 9) == 0) {
                    wcsncpy_s(savedPathValue, MAX_PATH, val, _TRUNCATE);
                    foundSavedPath = TRUE;
                }
                else if (wcsncmp(key, L"EnableAutomaticChannelChange", 28) == 0) {
                    enableAutomaticChannelChange = (wcscmp(valLower, L"true") == 0 || wcscmp(valLower, L"1") == 0);
                    foundEnableAutomaticChannelChange = TRUE;
                }
                else if (wcsncmp(key, L"WhisperKey", 10) == 0) {
                    whisperKey = _wtoi(val);
                }
                else if (wcsncmp(key, L"NormalKey", 9) == 0) {
                    normalKey = _wtoi(val);
                }
                else if (wcsncmp(key, L"ShoutKey", 8) == 0) {
                    shoutKey = _wtoi(val);
                }
                else if (wcsncmp(key, L"ConfigUIKey", 11) == 0) {
                    configUIKey = _wtoi(val);
                }
                else if (wcsncmp(key, L"EnableDistanceMuting", 20) == 0) {
                    enableDistanceMuting = (wcscmp(valLower, L"true") == 0 || wcscmp(valLower, L"1") == 0);
                }
                else if (wcsncmp(key, L"DistanceWhisper", 15) == 0) {
                    distanceWhisper = (float)_wtof(val);
                    foundDistanceWhisper = TRUE;
                }
                else if (wcsncmp(key, L"DistanceNormal", 14) == 0) {
                    distanceNormal = (float)_wtof(val);
                    foundDistanceNormal = TRUE;
                }
                else if (wcsncmp(key, L"DistanceShout", 13) == 0) {
                    distanceShout = (float)_wtof(val);
                    foundDistanceShout = TRUE;
                }
                else if (wcsncmp(key, L"VoiceToggleKey", 14) == 0) {
                    voiceToggleKey = _wtoi(val);
                }
                else if (wcsncmp(key, L"EnableVoiceToggle", 17) == 0) {
                    enableVoiceToggle = (wcscmp(valLower, L"true") == 0 || wcscmp(valLower, L"1") == 0);
                }
            }
            fclose(f);
        }
    }
    // If file doesn't exist or critical settings are missing, create or update it | Si le fichier n'existe pas ou si des paramètres critiques manquent, le créer ou le mettre à jour
    if (!fileExists || !foundSavedPath || !foundEnableAutomaticChannelChange) {
        FILE* f = _wfopen(configFile, L"w");
        if (f) {
            fwprintf(f, L"SavedPath=%s\n", foundSavedPath ? savedPathValue : L"");
            fwprintf(f, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
            fwprintf(f, L"WhisperKey=%d\n", whisperKey);
            fwprintf(f, L"NormalKey=%d\n", normalKey);
            fwprintf(f, L"ShoutKey=%d\n", shoutKey);
            fwprintf(f, L"ConfigUIKey=%d\n", configUIKey);
            fwprintf(f, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
            // CORRECTION: Écrire les distances SEULEMENT si elles ont été trouvées dans le fichier
            // Sinon, utiliser les valeurs par défaut
            fwprintf(f, L"DistanceWhisper=%.1f\n", foundDistanceWhisper ? distanceWhisper : 2.0f);
            fwprintf(f, L"DistanceNormal=%.1f\n", foundDistanceNormal ? distanceNormal : 10.0f);
            fwprintf(f, L"DistanceShout=%.1f\n", foundDistanceShout ? distanceShout : 15.0f);
            fwprintf(f, L"VoiceToggleKey=%d\n", voiceToggleKey);
            fwprintf(f, L"EnableVoiceToggle=%s\n", enableVoiceToggle ? L"true" : L"false");
            fclose(f);
        }
    }

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Configuration loaded - Whisper: %.1fm, Normal: %.1fm, Shout: %.1fm",
            distanceWhisper, distanceNormal, distanceShout);
        mumbleAPI.log(ownID, logMsg);
    }
}

// Helper function to save specific parameter | Fonction pour sauvegarder un paramètre spécifique
static void saveConfigurationChange(const char* key, const wchar_t* value) {
    // Call complete function to save everything at once | Appeler la fonction complète pour tout sauvegarder d'un coup
    saveVoiceSettings();

    if (TEMP) {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "SAVED CONFIG CHANGE: %s = %ls", key, value);
        mumbleAPI.log(ownID, logMsg);
    }
}

// Save voice settings and manage channel state | Sauvegarder les paramètres vocaux et gérer l'état des canaux
static void saveVoiceSettings() {
    // Get configuration folder path | Obtenir le chemin du dossier de configuration
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    // Store current voice distance before modifications | Stocker la distance vocale actuelle avant modifications
    float currentVoiceDistance = localVoiceData.voiceDistance;

    // Build configuration file path | Construire le chemin du fichier de configuration
    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    // Arrays to store configuration lines | Tableaux pour stocker les lignes de configuration
    wchar_t (*lines)[1024] = (wchar_t(*)[1024])calloc(100, sizeof(*lines));
    if (!lines) {
        // Allocation échouée, sortir proprement
        return;
    }
    int lineCount = 0;
    BOOL foundWhisper = FALSE, foundNormal = FALSE, foundShout = FALSE;
    BOOL foundDistanceMuting = FALSE, foundChannelChange = FALSE;

    // Read existing configuration file | Lire le fichier de configuration existant
    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, configFile, L"r");
    if (err == 0 && f) {
        while (fgetws(lines[lineCount], 1024, f) && lineCount < 99) {
            // Update whisper distance line | Mettre à jour la ligne de distance whisper
            if (wcsncmp(lines[lineCount], L"DistanceWhisper=", 16) == 0) {
                swprintf(lines[lineCount], 1024, L"DistanceWhisper=%.1f\n", distanceWhisper);
                foundWhisper = TRUE;
            }
            // Update normal distance line | Mettre à jour la ligne de distance normale
            else if (wcsncmp(lines[lineCount], L"DistanceNormal=", 15) == 0) {
                swprintf(lines[lineCount], 1024, L"DistanceNormal=%.1f\n", distanceNormal);
                foundNormal = TRUE;
            }
            // Update shout distance line | Mettre à jour la ligne de distance shout
            else if (wcsncmp(lines[lineCount], L"DistanceShout=", 14) == 0) {
                swprintf(lines[lineCount], 1024, L"DistanceShout=%.1f\n", distanceShout);
                foundShout = TRUE;
            }
            // Update distance muting setting | Mettre à jour le paramètre de muting par distance
            else if (wcsncmp(lines[lineCount], L"EnableDistanceMuting=", 21) == 0) {
                swprintf(lines[lineCount], 1024, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
                foundDistanceMuting = TRUE;
            }
            // Update automatic channel change setting | Mettre à jour le paramètre de changement automatique de canal
            else if (wcsncmp(lines[lineCount], L"EnableAutomaticChannelChange=", 29) == 0) {
                swprintf(lines[lineCount], 1024, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
                foundChannelChange = TRUE;
            }
            lineCount++;
        }
        fclose(f);
    }

    // Add missing configuration lines | Ajouter les lignes de configuration manquantes
    if (!foundWhisper && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"DistanceWhisper=%.1f\n", distanceWhisper);
    }
    if (!foundNormal && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"DistanceNormal=%.1f\n", distanceNormal);
    }
    if (!foundShout && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"DistanceShout=%.1f\n", distanceShout);
    }
    if (!foundDistanceMuting && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
    }
    if (!foundChannelChange && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
    }

    // Write updated configuration to file | Écrire la configuration mise à jour dans le fichier
    f = NULL;
    err = _wfopen_s(&f, configFile, L"w");
    if (err == 0 && f) {
        for (int i = 0; i < lineCount; i++) {
            fwprintf(f, L"%s", lines[i]);
        }
        fclose(f);

        // Preserve current voice mode based on closest distance match | Préserver le mode de voix actuel basé sur la correspondance de distance la plus proche
        if (fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceNormal) &&
            fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceShout)) {
            localVoiceData.voiceDistance = distanceWhisper;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Voice mode preserved: Whisper mode maintained after save");
            }
        }
        else if (fabsf(currentVoiceDistance - distanceShout) < fabsf(currentVoiceDistance - distanceNormal)) {
            localVoiceData.voiceDistance = distanceShout;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Voice mode preserved: Shout mode maintained after save");
            }
        }
        else {
            localVoiceData.voiceDistance = distanceNormal;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Voice mode preserved: Normal mode maintained after save");
            }
        }

        // Debug logging | Log de debug
        if (TEMP) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "Config saved - Voice mode preserved - Current distance: %.1f",
                localVoiceData.voiceDistance);
            mumbleAPI.log(ownID, logMsg);
        }
    }
}

// Initialize voice presets with default names | Initialiser les presets avec des noms par défaut
static void initializeVoicePresets(void) {
    for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
        snprintf(voicePresets[i].name, PRESET_NAME_MAX_LENGTH, "Save %d", i + 1);
        voicePresets[i].whisperDistance = 2.0f;
        voicePresets[i].normalDistance = 10.0f;
        voicePresets[i].shoutDistance = 15.0f;
        voicePresets[i].isUsed = FALSE;
    }

    if (enableLogConfig) {
        mumbleAPI.log(ownID, "Voice presets initialized with default values");
    }
}

// Save current voice ranges to preset | Sauvegarder les portées vocales actuelles dans un preset
static void saveVoicePreset(int presetIndex, const char* presetName) {
    if (presetIndex < 0 || presetIndex >= MAX_VOICE_PRESETS) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "ERROR: Invalid preset index for save");
        }
        return;
    }

    // Save current distances | Sauvegarder les distances actuelles
    voicePresets[presetIndex].whisperDistance = distanceWhisper;
    voicePresets[presetIndex].normalDistance = distanceNormal;
    voicePresets[presetIndex].shoutDistance = distanceShout;
    voicePresets[presetIndex].isUsed = TRUE;

    // Update name if provided | Mettre à jour le nom si fourni
    if (presetName && strlen(presetName) > 0) {
        strncpy_s(voicePresets[presetIndex].name, PRESET_NAME_MAX_LENGTH, presetName, _TRUNCATE);
    }

    currentPresetIndex = presetIndex;

    // Save to config file | Sauvegarder dans le fichier de configuration
    savePresetsToConfigFile();

    // Update interface labels | Mettre à jour les labels d'interface
    if (hConfigDialog && IsWindow(hConfigDialog)) {
        updatePresetLabels();
    }

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Voice preset saved: [%d] '%s' - Whisper:%.1f Normal:%.1f Shout:%.1f",
            presetIndex, voicePresets[presetIndex].name,
            distanceWhisper, distanceNormal, distanceShout);
        mumbleAPI.log(ownID, logMsg);
    }

    // Show confirmation | Afficher confirmation
    char confirmMsg[256];
    snprintf(confirmMsg, sizeof(confirmMsg),
        "✅ Preset saved: '%s'", voicePresets[presetIndex].name);
    displayInChat(confirmMsg);
}

// Load voice ranges from preset | Charger les portées vocales depuis un preset
static void loadVoicePreset(int presetIndex) {
    if (presetIndex < 0 || presetIndex >= MAX_VOICE_PRESETS) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "ERROR: Invalid preset index for load");
        }
        return;
    }

    if (!voicePresets[presetIndex].isUsed) {
        if (enableLogConfig) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "Preset %d is empty - nothing to load", presetIndex);
            mumbleAPI.log(ownID, logMsg);
        }

        MessageBoxW(hConfigDialog, L"This preset slot is empty.", L"Empty Preset", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Save current voice mode | Sauvegarder le mode vocal actuel
    float currentVoiceDistance = localVoiceData.voiceDistance;

    // Load distances from preset | Charger les distances depuis le preset
    distanceWhisper = voicePresets[presetIndex].whisperDistance;
    distanceNormal = voicePresets[presetIndex].normalDistance;
    distanceShout = voicePresets[presetIndex].shoutDistance;

    currentPresetIndex = presetIndex;

    // Update interface if open | Mettre à jour l'interface si ouverte
    if (hConfigDialog && IsWindow(hConfigDialog)) {
        isUpdatingInterface = TRUE;

        wchar_t whisperText[32], normalText[32], shoutText[32];
        swprintf(whisperText, 32, L"%.1f", distanceWhisper);
        swprintf(normalText, 32, L"%.1f", distanceNormal);
        swprintf(shoutText, 32, L"%.1f", distanceShout);

        if (hDistanceWhisperEdit) SetWindowTextW(hDistanceWhisperEdit, whisperText);
        if (hDistanceNormalEdit) SetWindowTextW(hDistanceNormalEdit, normalText);
        if (hDistanceShoutEdit) SetWindowTextW(hDistanceShoutEdit, shoutText);

        isUpdatingInterface = FALSE;

        updateDynamicInterface();
    }

    // Preserve voice mode | Préserver le mode vocal
    if (fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceNormal) &&
        fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceShout)) {
        localVoiceData.voiceDistance = distanceWhisper;
    }
    else if (fabsf(currentVoiceDistance - distanceShout) < fabsf(currentVoiceDistance - distanceNormal)) {
        localVoiceData.voiceDistance = distanceShout;
    }
    else {
        localVoiceData.voiceDistance = distanceNormal;
    }

    // Apply changes | Appliquer les changements
    saveVoiceSettings();
    applyDistanceToAllPlayers();

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Voice preset loaded: [%d] '%s' - Whisper:%.1f Normal:%.1f Shout:%.1f",
            presetIndex, voicePresets[presetIndex].name,
            distanceWhisper, distanceNormal, distanceShout);
        mumbleAPI.log(ownID, logMsg);
    }

    // Show confirmation message | Afficher message de confirmation
    char confirmMsg[256];
    snprintf(confirmMsg, sizeof(confirmMsg),
        "✅ Preset loaded: '%s'", voicePresets[presetIndex].name);
    displayInChat(confirmMsg);
}

// Rename voice preset | Renommer un preset vocal
static BOOL renameVoicePreset(int presetIndex, const char* newName) {
    if (presetIndex < 0 || presetIndex >= MAX_VOICE_PRESETS) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "ERROR: Invalid preset index for rename");
        }
        return FALSE;
    }

    if (!newName || strlen(newName) == 0) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "ERROR: Empty name provided for rename");
        }
        return FALSE;
    }

    // ✅ VÉRIFIER LA LONGUEUR (10 caractères max)
    if (strlen(newName) > 15) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "ERROR: Name too long (max 10 characters)");
        }
        return FALSE;
    }

    char oldName[PRESET_NAME_MAX_LENGTH];
    strncpy_s(oldName, PRESET_NAME_MAX_LENGTH, voicePresets[presetIndex].name, _TRUNCATE);

    // Update name | Mettre à jour le nom
    strncpy_s(voicePresets[presetIndex].name, PRESET_NAME_MAX_LENGTH, newName, _TRUNCATE);

    // Save to config | Sauvegarder dans la configuration
    savePresetsToConfigFile();

    // ✅ MISE À JOUR IMMÉDIATE DE L'INTERFACE (sans fermer/rouvrir)
    if (hConfigDialog && IsWindow(hConfigDialog)) {
        // Forcer la mise à jour des labels de presets
        updatePresetLabels();

        // Forcer le redessin de la zone des presets
        RECT presetArea;
        presetArea.left = 40;
        presetArea.top = 200;
        presetArea.right = 560;
        presetArea.bottom = 600;
        InvalidateRect(hConfigDialog, &presetArea, TRUE);
        UpdateWindow(hConfigDialog);
    }

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Preset renamed: [%d] '%s' -> '%s'",
            presetIndex, oldName, newName);
        mumbleAPI.log(ownID, logMsg);
    }

    return TRUE;
}

// Save presets to configuration file | Sauvegarder les presets dans le fichier de configuration
static void savePresetsToConfigFile(void) {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t presetFile[MAX_PATH];
    swprintf(presetFile, MAX_PATH, L"%s\\voice_presets.cfg", configFolder);

    FILE* file = NULL;
    errno_t err = _wfopen_s(&file, presetFile, L"w");
    if (err != 0 || !file) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "ERROR: Failed to create voice presets config file");
        }
        return;
    }

    // Write current preset index | Écrire l'index du preset actuel
    fwprintf(file, L"CurrentPreset=%d\n\n", currentPresetIndex);

    // Write all presets | Écrire tous les presets
    for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
        wchar_t wName[PRESET_NAME_MAX_LENGTH];
        size_t converted = 0;
        mbstowcs_s(&converted, wName, PRESET_NAME_MAX_LENGTH, voicePresets[i].name, _TRUNCATE);

        fwprintf(file, L"[Preset%d]\n", i);
        fwprintf(file, L"Name=%s\n", wName);
        fwprintf(file, L"Whisper=%.1f\n", voicePresets[i].whisperDistance);
        fwprintf(file, L"Normal=%.1f\n", voicePresets[i].normalDistance);
        fwprintf(file, L"Shout=%.1f\n", voicePresets[i].shoutDistance);
        fwprintf(file, L"IsUsed=%s\n\n", voicePresets[i].isUsed ? L"true" : L"false");
    }

    fclose(file);

    if (enableLogConfig) {
        mumbleAPI.log(ownID, "Voice presets saved to config file");
    }
}

// Load presets from configuration file | Charger les presets depuis le fichier de configuration
static void loadPresetsFromConfigFile(void) {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t presetFile[MAX_PATH];
    swprintf(presetFile, MAX_PATH, L"%s\\voice_presets.cfg", configFolder);

    FILE* file = NULL;
    errno_t err = _wfopen_s(&file, presetFile, L"r");
    if (err != 0 || !file) {
        // File doesn't exist, initialize with defaults | Fichier inexistant, initialiser avec valeurs par défaut
        initializeVoicePresets();
        return;
    }

    wchar_t line[512];
    int currentPreset = -1;

    while (fgetws(line, 512, file)) {
        // Remove trailing newline | Supprimer le retour à la ligne
        wchar_t* nl = wcschr(line, L'\n');
        if (nl) *nl = L'\0';
        wchar_t* cr = wcschr(line, L'\r');
        if (cr) *cr = L'\0';

        // Parse current preset index | Parser l'index du preset actuel
        if (wcsncmp(line, L"CurrentPreset=", 14) == 0) {
            currentPresetIndex = _wtoi(line + 14);
        }
        // Parse preset section | Parser la section preset
        else if (wcsncmp(line, L"[Preset", 7) == 0) {
            wchar_t* endBracket = wcschr(line, L']');
            if (endBracket) {
                *endBracket = L'\0';
                currentPreset = _wtoi(line + 7);
            }
        }
        // Parse preset properties | Parser les propriétés du preset
        else if (currentPreset >= 0 && currentPreset < MAX_VOICE_PRESETS) {
            if (wcsncmp(line, L"Name=", 5) == 0) {
                size_t converted = 0;
                wcstombs_s(&converted, voicePresets[currentPreset].name, PRESET_NAME_MAX_LENGTH, line + 5, _TRUNCATE);
            }
            else if (wcsncmp(line, L"Whisper=", 8) == 0) {
                voicePresets[currentPreset].whisperDistance = (float)_wtof(line + 8);
            }
            else if (wcsncmp(line, L"Normal=", 7) == 0) {
                voicePresets[currentPreset].normalDistance = (float)_wtof(line + 7);
            }
            else if (wcsncmp(line, L"Shout=", 6) == 0) {
                voicePresets[currentPreset].shoutDistance = (float)_wtof(line + 6);
            }
            else if (wcsncmp(line, L"IsUsed=", 7) == 0) {
                voicePresets[currentPreset].isUsed = (wcscmp(line + 7, L"true") == 0);
            }
        }
    }

    fclose(file);

    if (enableLogConfig) {
        mumbleAPI.log(ownID, "Voice presets loaded from config file");
    }
}

// Write full Saved path to config file | Écriture du chemin complet Saved dans le fichier de configuration
static void writeFullConfiguration(const wchar_t* gameFolder, const wchar_t* distWhisper, const wchar_t* distNormal, const wchar_t* distShout) {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) {
        return;
    }

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    // Construire le chemin COMPLET pour l'enregistrement (avec \ConanSandbox\Saved)
    wchar_t savedPathFull[MAX_PATH];

    // Si displayedPathText contient le chemin, ajouter \ConanSandbox\Saved
    if (wcslen(displayedPathText) > 0) {
        // displayedPathText = C:\...\Conan Exiles (SANS \ConanSandbox\Saved)
        // Construire le chemin COMPLET = C:\...\Conan Exiles\ConanSandbox\Saved
        swprintf(savedPathFull, MAX_PATH, L"%s\\ConanSandbox\\Saved", displayedPathText);
    }
    // Sinon, construire depuis gameFolder (fallback)
    else if (gameFolder && wcslen(gameFolder) > 0) {
        swprintf(savedPathFull, MAX_PATH, L"%s\\ConanSandbox\\Saved", gameFolder);
    }
    else {
        // Valeur par défaut si rien n'est disponible
        wcscpy_s(savedPathFull, MAX_PATH, L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles\\ConanSandbox\\Saved");
    }

    // Save current voice mode before modifying distances | Sauvegarder le mode de voix actuel AVANT de modifier les distances
    float currentVoiceDistance = localVoiceData.voiceDistance;

    // CORRECTION: Convertir les valeurs d'entrée SANS appliquer de limites
    float whisperValue = (float)_wtof(distWhisper);
    float normalValue = (float)_wtof(distNormal);
    float shoutValue = (float)_wtof(distShout);

    // CORRECTION: Mettre à jour les variables globales avec les valeurs ORIGINALES de l'utilisateur
    distanceWhisper = whisperValue;
    distanceNormal = normalValue;
    distanceShout = shoutValue;

    // Écrire directement dans le fichier
    FILE* file = _wfopen(configFile, L"w");
    if (!file) {
        return;
    }

    fwprintf(file, L"SavedPath=%s\n", savedPathFull);
    fwprintf(file, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
    fwprintf(file, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
    fwprintf(file, L"WhisperKey=%d\n", whisperKey);
    fwprintf(file, L"NormalKey=%d\n", normalKey);
    fwprintf(file, L"ShoutKey=%d\n", shoutKey);
    fwprintf(file, L"ConfigUIKey=%d\n", configUIKey);
    // CORRECTION: Écrire les valeurs EXACTES entrées par l'utilisateur
    fwprintf(file, L"DistanceWhisper=%.1f\n", distanceWhisper);
    fwprintf(file, L"DistanceNormal=%.1f\n", distanceNormal);
    fwprintf(file, L"DistanceShout=%.1f\n", distanceShout);
    fwprintf(file, L"VoiceToggleKey=%d\n", voiceToggleKey);
    fwprintf(file, L"EnableVoiceToggle=%s\n", enableVoiceToggle ? L"true" : L"false");
    fclose(file);

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "✅ USER VALUES SAVED: Whisper=%.1f, Normal=%.1f, Shout=%.1f",
            distanceWhisper, distanceNormal, distanceShout);
        mumbleAPI.log(ownID, logMsg);
    }

    // Restore current voice mode instead of forcing Normal | Restaurer le mode de voix actuel au lieu de forcer Normal
    if (fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceNormal) &&
        fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceShout)) {
        localVoiceData.voiceDistance = distanceWhisper;
    }
    else if (fabsf(currentVoiceDistance - distanceShout) < fabsf(currentVoiceDistance - distanceNormal)) {
        localVoiceData.voiceDistance = distanceShout;
    }
    else {
        localVoiceData.voiceDistance = distanceNormal;
    }

    // Update mod file path | Mettre à jour le chemin du fichier mod
    size_t converted = 0;
    char modFilePathTemp[MAX_PATH] = "";
    wcstombs_s(&converted, modFilePathTemp, MAX_PATH, savedPathFull, _TRUNCATE);
    snprintf(modFilePath, MAX_PATH, "%s\\Pos.txt", modFilePathTemp);

    // Reinstall keyboard monitoring | Réinstaller la surveillance du clavier
    removeKeyMonitoring();
    installKeyMonitoring();
}

// ============================================================================
// MODULE 3 : VALIDATION ET LIMITES
// ============================================================================
// Check if distance limits should be applied | Fonction pour déterminer si les limites de distance doivent être appliquées
static BOOL shouldApplyDistanceLimits() {
    if (!isConnectedToServer) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Distance limits DISABLED: Not connected to server");
        }
        return FALSE;
    }

    if (rootChannelID == -1) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Distance limits DISABLED: No hub channel found");
        }
        return FALSE;
    }

    if (!hubDescriptionAvailable) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Distance limits DISABLED: Hub description not available");
        }
        return FALSE;
    }

    if (!hubForceDistanceBasedMuting) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Distance limits DISABLED: ForceDistanceBasedMuting = FALSE - user has full control");
        }
        return FALSE;
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Distance limits ENABLED: ForceDistanceBasedMuting = TRUE");
    }
    return TRUE;
}

// Check connection status | Fonction pour vérifier l'état de la connexion
static void checkConnectionStatus() {
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastConnectionCheck < 2000) return;
    lastConnectionCheck = currentTime;

    mumble_connection_t connection;
    mumble_error_t result = mumbleAPI.getActiveServerConnection(ownID, &connection);

    BOOL wasConnected = isConnectedToServer;
    isConnectedToServer = (result == MUMBLE_STATUS_OK);

    if (wasConnected != isConnectedToServer) {
        if (isConnectedToServer) {
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "CONNECTION: Connected to server - distance limits may be applied");
            }
        }
        else {
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "CONNECTION: Disconnected from server - distance limits DISABLED");
            }
            hubDescriptionAvailable = FALSE;
            hubLimitsActive = FALSE;
        }
    }
}

// Determine if value should be validated | Déterminer si une valeur doit être validée
static BOOL shouldValidateValue(float value, float minimum, float maximum, const char* modeName) {
    int digitCount = countSignificantDigits(value);

    int minDigits = countSignificantDigits(minimum);
    int maxDigits = countSignificantDigits(maximum);
    int requiredDigits = (maxDigits > minDigits) ? maxDigits : minDigits;

    if (maximum >= 10.0f) {
        requiredDigits = 2;
    }

    if (digitCount < requiredDigits) {
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg),
                "FILTER: %s value %.1f has %d digits, need %d digits - IGNORING",
                modeName, value, digitCount, requiredDigits);
            mumbleAPI.log(ownID, logMsg);
        }
        return FALSE;
    }

    if (enableLogGeneral) {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg),
            "FILTER: %s value %.1f has %d digits, need %d digits - VALIDATING",
            modeName, value, digitCount, requiredDigits);
        mumbleAPI.log(ownID, logMsg);
    }

    return TRUE;
}

// Validate and correct distance in real time | Fonction pour valider et corriger une distance en temps réel
static float validateDistanceValue(float value, float minimum, float maximum, const char* modeName) {
    if (!shouldApplyDistanceLimits()) {
        return value;
    }

    if (value < minimum) {
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "%s distance auto-corrected: %.1f -> %.1f (below minimum)",
                modeName, value, minimum);
            mumbleAPI.log(ownID, logMsg);
        }
        return minimum;
    }
    else if (value > maximum) {
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "%s distance auto-corrected: %.1f -> %.1f (above maximum)",
                modeName, value, maximum);
            mumbleAPI.log(ownID, logMsg);
        }
        return maximum;
    }
    return value;
}

static void validatePlayerDistances() {
    // Check if limits should be applied | Vérifier si les limites doivent être appliquées
    if (!shouldApplyDistanceLimits()) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Distance validation SKIPPED: ForceDistanceBasedMuting = FALSE - user has full control of distances");
        }
        hubLimitsActive = FALSE;

        // Ensure messages reflect user freedom | S'assurer que les messages reflètent la liberté de l'utilisateur
        if (hConfigDialog && IsWindow(hConfigDialog)) {
            updateDynamicInterface();
        }

        // Trigger immediate channel management check | Déclencher immédiatement une vérification de gestion des canaux
        if (enableAutomaticChannelChange && channelManagementActive) {
            lastChannelCheck = 0;
            if (TEMP) {
                mumbleAPI.log(ownID, "IMMEDIATE: Triggering channel management check");
            }
        }

        return;
    }

    hubLimitsActive = TRUE;

    BOOL distanceChanged = FALSE;
    float originalWhisper = distanceWhisper;
    float originalNormal = distanceNormal;
    float originalShout = distanceShout;

    // Validate whisper distance | Valider la distance whisper
    if (distanceWhisper < hubMinimumWhisper) {
        distanceWhisper = (float)hubMinimumWhisper;
        distanceChanged = TRUE;
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "Whisper distance corrected: %.1f -> %.1f (below minimum)",
                originalWhisper, distanceWhisper);
            mumbleAPI.log(ownID, logMsg);
        }
    }
    else if (distanceWhisper > hubMaximumWhisper) {
        distanceWhisper = (float)hubMaximumWhisper;
        distanceChanged = TRUE;
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "Whisper distance corrected: %.1f -> %.1f (above maximum)",
                originalWhisper, distanceWhisper);
            mumbleAPI.log(ownID, logMsg);
        }
    }

    // Validate normal distance | Valider la distance normale
    if (distanceNormal < hubMinimumNormal) {
        distanceNormal = (float)hubMinimumNormal;
        distanceChanged = TRUE;
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "Normal distance corrected: %.1f -> %.1f (below minimum)",
                originalNormal, distanceNormal);
            mumbleAPI.log(ownID, logMsg);
        }
    }
    else if (distanceNormal > hubMaximumNormal) {
        distanceNormal = (float)hubMaximumNormal;
        distanceChanged = TRUE;
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "Normal distance corrected: %.1f -> %.1f (above maximum)",
                originalNormal, distanceNormal);
            mumbleAPI.log(ownID, logMsg);
        }
    }

    // Validate shout distance | Valider la distance shout
    if (distanceShout < hubMinimumShout) {
        distanceShout = (float)hubMinimumShout;
        distanceChanged = TRUE;
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "Shout distance corrected: %.1f -> %.1f (below minimum)",
                originalShout, distanceShout);
            mumbleAPI.log(ownID, logMsg);
        }
    }
    else if (distanceShout > hubMaximumShout) {
        distanceShout = (float)hubMaximumShout;
        distanceChanged = TRUE;
        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "Shout distance corrected: %.1f -> %.1f (above maximum)",
                originalShout, distanceShout);
            mumbleAPI.log(ownID, logMsg);
        }
    }

    // Save and apply changes if necessary | Sauvegarder et appliquer les changements si nécessaire
    if (distanceChanged) {
        saveVoiceSettings();
        applyDistanceToAllPlayers();

        if (enableLogGeneral) {
            char summaryMsg[256];
            snprintf(summaryMsg, sizeof(summaryMsg),
                "Distance validation complete: Whisper=%.1f, Normal=%.1f, Shout=%.1f",
                distanceWhisper, distanceNormal, distanceShout);
            mumbleAPI.log(ownID, summaryMsg);
        }
    }
    else {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "All player distances are valid - no corrections needed");
        }
    }
}

// Retrieve server maximum audio distance | Récupérer la distance audio maximale du serveur
static void retrieveServerMaximumAudioDistance(BOOL forceUpdate) {
    ULONGLONG currentTime = GetTickCount64();

    if (!forceUpdate && (currentTime - lastMaxDistanceCheck < 5000)) return;
    lastMaxDistanceCheck = currentTime;

    mumble_error_t result = mumbleAPI.getMumbleSetting_double(ownID,
        MUMBLE_SK_AUDIO_OUTPUT_PA_MAXIMUM_DISTANCE, &serverMaximumAudioDistance);

    if (result == MUMBLE_STATUS_OK) {
        maxAudioDistanceRetrieved = TRUE;

        if (enableLogGeneral) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "Server Maximum Audio Distance retrieved: %.1f meters %s",
                serverMaximumAudioDistance,
                forceUpdate ? "(FORCED UPDATE)" : "");
            mumbleAPI.log(ownID, logMsg);
        }
    }
    else {
        serverMaximumAudioDistance = 30.0;
        maxAudioDistanceRetrieved = FALSE;

        if (enableLogGeneral) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "Failed to retrieve server max distance (Error: %d), using default: %.1f meters",
                result, serverMaximumAudioDistance);
            mumbleAPI.log(ownID, logMsg);
        }
    }
}

// Apply maximum distance limits | Appliquer les limites de distance maximale
static void applyMaximumDistanceLimits() {
    if (!maxAudioDistanceRetrieved) {
        retrieveServerMaximumAudioDistance(FALSE);
    }

    BOOL distanceChanged = FALSE;

    if (distanceWhisper > serverMaximumAudioDistance) {
        distanceWhisper = (float)serverMaximumAudioDistance;
        distanceChanged = TRUE;
    }

    if (distanceNormal > serverMaximumAudioDistance) {
        distanceNormal = (float)serverMaximumAudioDistance;
        distanceChanged = TRUE;
    }

    if (distanceShout > serverMaximumAudioDistance) {
        distanceShout = (float)serverMaximumAudioDistance;
        distanceChanged = TRUE;
    }

    if (distanceChanged) {
        saveVoiceSettings();
        applyDistanceToAllPlayers();
    }
}

// ============================================================================
// MODULE 4 : PARSING HUB
// ============================================================================

// Parse hub description to extract audio parameters | Parser la description du hub pour extraire les paramètres audio
static void parseHubDescription(const char* description) {
    if (!description || strlen(description) == 0) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Root description is empty, using default audio settings");
        }
        hubDescriptionAvailable = FALSE;
        return;
    }

    if (enableLogGeneral) {
        char logMsg[512];
        snprintf(logMsg, sizeof(logMsg), "Parsing hub description: %s", description);
        mumbleAPI.log(ownID, logMsg);
    }

    // Create copy and replace HTML tags | Créer une copie et remplacer les balises HTML
    size_t descLen = strlen(description);
    char* descCopy = (char*)malloc(descLen * 2 + 1);
    if (!descCopy) return;

    // Replace HTML line breaks | Remplacer les retours à la ligne HTML
    char* dest = descCopy;
    const char* src = description;

    while (*src) {
        if (strncmp(src, "<br/>", 5) == 0) {
            *dest++ = '\n';
            src += 5;
        }
        else if (strncmp(src, "<BR/>", 5) == 0) {
            *dest++ = '\n';
            src += 5;
        }
        else if (strncmp(src, "<br>", 4) == 0) {
            *dest++ = '\n';
            src += 4;
        }
        else if (strncmp(src, "<BR>", 4) == 0) {
            *dest++ = '\n';
            src += 4;
        }
        else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';

    if (enableLogGeneral) {
        char logMsg[512];
        snprintf(logMsg, sizeof(logMsg), "After HTML processing: %s", descCopy);
        mumbleAPI.log(ownID, logMsg);
    }

    // Parse line by line | Parser ligne par ligne
    char* context = NULL;
    char* line = strtok_s(descCopy, "\n\r", &context);

    while (line != NULL) {
        // Clean whitespace | Nettoyer les espaces
        while (*line == ' ' || *line == '\t') line++;

        // Clean trailing whitespace | Nettoyer les espaces à la fin
        char* end = line + strlen(line) - 1;
        while (end > line && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

        if (enableLogGeneral) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg), "Processing line: '%s'", line);
            mumbleAPI.log(ownID, logMsg);
        }

        // Audio parameters | Paramètres audio
        if (strncmp(line, "ForcePositionelleAudio", 22) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;

                if (strncmp(equal, "True", 4) == 0 || strncmp(equal, "true", 4) == 0 ||
                    strncmp(equal, "TRUE", 4) == 0 || strncmp(equal, "1", 1) == 0) {
                    hubForcePositionalAudio = TRUE;
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "Hub: ForcePositionalAudio = TRUE");
                    }
                }
                else {
                    hubForcePositionalAudio = FALSE;
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "Hub: ForcePositionalAudio = FALSE");
                    }
                }
            }
        }
        else if (strncmp(line, "AudioMinDistance", 16) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubAudioMinDistance = (float)strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: AudioMinDistance = %.1f", hubAudioMinDistance);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
        else if (strncmp(line, "AudioMaxDistance", 16) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubAudioMaxDistance = (float)strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: AudioMaxDistance = %.1f", hubAudioMaxDistance);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
        else if (strncmp(line, "AudioMaxVolume", 14) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubAudioMaxVolume = (float)strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: AudioMaxVolume = %.1f", hubAudioMaxVolume);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
        else if (strncmp(line, "AudioBloom", 10) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubAudioBloom = (float)strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: AudioBloom = %.1f", hubAudioBloom);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }

        // Parse AudioFilterIntensity | Parser AudioFilterIntensity
        else if (strncmp(line, "AudioFilterIntensity", 20) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubAudioFilterIntensity = (float)strtod(equal, NULL);

                // Limit between 0.0 and 1.0 | Limiter entre 0.0 et 1.0
                if (hubAudioFilterIntensity < 0.0) hubAudioFilterIntensity = 0.0;
                if (hubAudioFilterIntensity > 1.0) hubAudioFilterIntensity = 1.0;

                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: AudioFilterIntensity = %.2f", hubAudioFilterIntensity);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }

        // Distance-based muting parameters | Paramètres de muting basé sur la distance
        else if (strncmp(line, "ForceDistanceBasedMuting", 24) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;

                BOOL oldForceValue = hubForceDistanceBasedMuting;

                if (strncmp(equal, "True", 4) == 0 || strncmp(equal, "true", 4) == 0 ||
                    strncmp(equal, "TRUE", 4) == 0 || strncmp(equal, "1", 1) == 0) {
                    hubForceDistanceBasedMuting = TRUE;
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "Hub: ForceDistanceBasedMuting = TRUE");
                    }
                }
                else {
                    hubForceDistanceBasedMuting = FALSE;
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "Hub: ForceDistanceBasedMuting = FALSE - User choice restored");
                    }
                }

                // Detect state change | Détecter le changement d'état
                if (oldForceValue != hubForceDistanceBasedMuting) {
                    if (enableLogGeneral) {
                        char changeMsg[128];
                        snprintf(changeMsg, sizeof(changeMsg),
                            "Hub: ForceDistanceBasedMuting changed from %s to %s",
                            oldForceValue ? "TRUE" : "FALSE",
                            hubForceDistanceBasedMuting ? "TRUE" : "FALSE");
                        mumbleAPI.log(ownID, changeMsg);
                    }
                }
            }
        }

        // Automatic channel switching parameters | Paramètres de changement automatique de canal
        else if (strncmp(line, "ForceAutomaticChanelSwitching", 29) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;

                BOOL oldForceValue = hubForceAutomaticChannelSwitching;

                if (strncmp(equal, "True", 4) == 0 || strncmp(equal, "true", 4) == 0 ||
                    strncmp(equal, "TRUE", 4) == 0 || strncmp(equal, "1", 1) == 0) {
                    hubForceAutomaticChannelSwitching = TRUE;
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "Hub: ForceAutomaticChannelSwitching = TRUE");
                    }
                }
                else {
                    hubForceAutomaticChannelSwitching = FALSE;
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "Hub: ForceAutomaticChannelSwitching = FALSE - User choice restored");
                    }
                }

                // Detect state change | Détecter le changement d'état
                if (oldForceValue != hubForceAutomaticChannelSwitching) {
                    if (enableLogGeneral) {
                        char changeMsg[128];
                        snprintf(changeMsg, sizeof(changeMsg),
                            "Hub: ForceAutomaticChannelSwitching changed from %s to %s",
                            oldForceValue ? "TRUE" : "FALSE",
                            hubForceAutomaticChannelSwitching ? "TRUE" : "FALSE");
                        mumbleAPI.log(ownID, changeMsg);
                    }
                }
            }
        }
        // Whisper limits | Limites whisper
        else if (strncmp(line, "MinimumWisper", 13) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubMinimumWhisper = strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: MinimumWhisper = %.1f", hubMinimumWhisper);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
        else if (strncmp(line, "MaximumWisper", 13) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubMaximumWhisper = strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: MaximumWhisper = %.1f", hubMaximumWhisper);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }

        // Normal limits | Limites normales
        else if (strncmp(line, "MinimumNormal", 13) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubMinimumNormal = strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: MinimumNormal = %.1f", hubMinimumNormal);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
        else if (strncmp(line, "MaximumNormal", 13) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubMaximumNormal = strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: MaximumNormal = %.1f", hubMaximumNormal);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }

        // Shout limits | Limites shout
        else if (strncmp(line, "MinimumShout", 12) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubMinimumShout = strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: MinimumShout = %.1f", hubMinimumShout);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
        else if (strncmp(line, "MaximumShout", 12) == 0) {
            char* equal = strchr(line, '=');
            if (equal) {
                equal++;
                while (*equal == ' ' || *equal == '\t') equal++;
                hubMaximumShout = strtod(equal, NULL);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: MaximumShout = %.1f", hubMaximumShout);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }

        line = strtok_s(NULL, "\n\r", &context);
    }

    free(descCopy);

    // Mark hub description as available | Marquer la description du hub comme disponible
    hubDescriptionAvailable = TRUE;

    // Apply new values | Appliquer les nouvelles valeurs
    BOOL configChanged = FALSE;

    if (hubForcePositionalAudio) {
        if (!enableAutoAudioSettings) {
            enableAutoAudioSettings = TRUE;
            configChanged = TRUE;
        }
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Hub: ForcePositionalAudio = TRUE - Audio settings will be FORCED in Mumble");
        }
    }
    else {
        if (enableAutoAudioSettings) {
            enableAutoAudioSettings = FALSE;
            configChanged = TRUE;
        }
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Hub: ForcePositionalAudio = FALSE - User retains control of audio settings");
        }
    }

    // Force distance-based muting if requested by hub | Forcer le muting basé sur la distance si demandé par le hub
    if (hubForceDistanceBasedMuting) {
        if (!enableDistanceMuting) {
            enableDistanceMuting = TRUE;
            configChanged = TRUE;
        }
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Hub: Distance-based muting FORCED by server");
        }
    }
    else {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Hub: Distance-based muting not forced - user choice allowed");
        }
    }

    // Force automatic channel change if requested by hub | Forcer le changement automatique de canal si demandé par le hub
    if (hubForceAutomaticChannelSwitching) {
        if (!enableAutomaticChannelChange) {
            enableAutomaticChannelChange = TRUE;
            configChanged = TRUE;
        }
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Hub: Automatic channel switching FORCED by server");
        }
    }
    else {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Hub: Automatic channel switching not forced - user choice allowed");
        }
    }

    // Save immediately if changes were made | Sauvegarder immédiatement si des changements ont été faits
    if (configChanged) {
        saveVoiceSettings();
        if (TEMP) {
            mumbleAPI.log(ownID, "IMMEDIATE SAVE: Configuration changes applied and saved to disk");
        }
    }

    validatePlayerDistances();

    if (enableLogGeneral) {
        char logMsg[1024];
        snprintf(logMsg, sizeof(logMsg),
            "Hub settings applied:\n"
            "  ForceAudio=%s, MinDist=%.1f, MaxDist=%.1f, MaxVol=%.1f, Bloom=%.1f, FilterIntensity=%.2f\n"
            "  ForceMuting=%s\n"
            "  Whisper: %.1f-%.1f, Normal: %.1f-%.1f, Shout: %.1f-%.1f",
            enableAutoAudioSettings ? "TRUE" : "FALSE",
            hubAudioMinDistance, hubAudioMaxDistance, hubAudioMaxVolume, hubAudioBloom, hubAudioFilterIntensity,
            hubForceDistanceBasedMuting ? "TRUE" : "FALSE",
            hubMinimumWhisper, hubMaximumWhisper,
            hubMinimumNormal, hubMaximumNormal,
            hubMinimumShout, hubMaximumShout);
        mumbleAPI.log(ownID, logMsg);
    }
    if (hConfigDialog && IsWindow(hConfigDialog)) {
        updateDynamicInterface();
    }
}

// Read hub description | Lire la description du hub
static void readHubDescription() {
    // Robust read of root/hub channel description with synchronization retries
    // Lecture robuste de la description du canal root/hub avec retries pour la synchronisation
    if (rootChannelID == -1) {
        // Try to discover channels if not set | Essayer de découvrir les canaux si non configurés
        initializeChannelIDs();
        if (rootChannelID == -1 && enableLogGeneral) {
            mumbleAPI.log(ownID, "readHubDescription: rootChannelID not set - initializeChannelIDs attempted");
        }
    }

    ULONGLONG currentTime = GetTickCount64();
    // Check every 1 second (1000 ms) | Vérifier toutes les 1 seconde
    if (currentTime - lastHubDescriptionCheck < 1000) return;
    lastHubDescriptionCheck = currentTime;

    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        hubDescriptionAvailable = FALSE;
        if (enableLogGeneral) mumbleAPI.log(ownID, "readHubDescription: No active server connection");
        return;
    }

    // Wait for connection synchronization with short retries | Attendre la synchronisation avec des retries courts
    bool synchronized = false;
    mumble_error_t syncRes = mumbleAPI.isConnectionSynchronized(ownID, connection, &synchronized);
    int syncAttempts = 0;
    const int syncMaxAttempts = 10;
    while ((syncRes != MUMBLE_STATUS_OK || !synchronized) && syncAttempts < syncMaxAttempts) {
        if (enableLogGeneral) {
            char msg[128];
            snprintf(msg, sizeof(msg), "readHubDescription: waiting for synchronization (attempt %d/%d)", syncAttempts + 1, syncMaxAttempts);
            mumbleAPI.log(ownID, msg);
        }
        Sleep(200);
        syncRes = mumbleAPI.isConnectionSynchronized(ownID, connection, &synchronized);
        syncAttempts++;
    }

    if (!synchronized) {
        hubDescriptionAvailable = FALSE;
        if (enableLogGeneral) mumbleAPI.log(ownID, "readHubDescription: Connection not synchronized - will retry later");
        return;
    }

    // If rootChannelID unknown, attempt to find it now (case variants) | Si rootChannelID inconnu, tenter de le trouver (variantes de casse)
    if (rootChannelID == -1) {
        const char* rootNames[] = { "Root", "root", "ROOT" };
        for (size_t i = 0; i < sizeof(rootNames) / sizeof(rootNames[0]); ++i) {
            mumble_error_t fres = mumbleAPI.findChannelByName(ownID, connection, rootNames[i], &rootChannelID);
            if (fres == MUMBLE_STATUS_OK) {
                if (enableLogGeneral) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "readHubDescription: Found Root channel as '%s' (id=%d)", rootNames[i], rootChannelID);
                    mumbleAPI.log(ownID, msg);
                }
                break;
            }
        }
    }

    // Primary attempt to get description | Tentative primaire pour obtenir la description
    const char* description = NULL;
    mumble_error_t result = mumbleAPI.getChannelDescription(ownID, connection, rootChannelID, &description);

    // If unsynchronized blob or transient error, retry a few times | Si erreur transitoire, refaire quelques retries
    if (result != MUMBLE_STATUS_OK || description == NULL) {
        if (enableLogGeneral) {
            char msg[128];
            snprintf(msg, sizeof(msg), "readHubDescription: initial getChannelDescription returned %d - entering retry loop", result);
            mumbleAPI.log(ownID, msg);
        }

        const int maxTries = 8;
        int tries = 0;
        while (tries < maxTries) {
            Sleep(250);

            // Re-check sync quickly before retry | Re-vérifier la sync avant retry
            bool isSync = false;
            if (mumbleAPI.isConnectionSynchronized(ownID, connection, &isSync) != MUMBLE_STATUS_OK || !isSync) {
                if (enableLogGeneral) mumbleAPI.log(ownID, "readHubDescription: connection lost synchronization during retries");
                break;
            }

            // If rootChannelID was -1, try to discover again (case-insensitive) | Si rootChannelID non trouvé, tenter redécouverte
            if (rootChannelID == -1) {
                const char* rootNames[] = { "Root", "root", "ROOT" };
                for (size_t i = 0; i < sizeof(rootNames) / sizeof(rootNames[0]); ++i) {
                    mumble_error_t fres = mumbleAPI.findChannelByName(ownID, connection, rootNames[i], &rootChannelID);
                    if (fres == MUMBLE_STATUS_OK) {
                        if (enableLogGeneral) {
                            char msg[128];
                            snprintf(msg, sizeof(msg), "readHubDescription: discovered Root channel as '%s' (id=%d) during retries", rootNames[i], rootChannelID);
                            mumbleAPI.log(ownID, msg);
                        }
                        break;
                    }
                }
            }

            // Attempt to get description if we have an ID | Essayer d'obtenir la description si ID présent
            if (rootChannelID != -1) {
                if (description) { mumbleAPI.freeMemory(ownID, description); description = NULL; }
                result = mumbleAPI.getChannelDescription(ownID, connection, rootChannelID, &description);
                if (result == MUMBLE_STATUS_OK && description != NULL) break;
            }

            // Fallback: try hub channel if root still fails | Fallback : essayer le hub si root échoue
            if (rootChannelID == -1 && hubChannelID != -1) {
                if (description) { mumbleAPI.freeMemory(ownID, description); description = NULL; }
                result = mumbleAPI.getChannelDescription(ownID, connection, hubChannelID, &description);
                if (result == MUMBLE_STATUS_OK && description != NULL) {
                    if (enableLogGeneral) mumbleAPI.log(ownID, "readHubDescription: used hub channel description as fallback");
                    break;
                }
            }

            tries++;
        }
    }

    // If we succeeded, compare with cached copy and parse only on change
    if (result == MUMBLE_STATUS_OK && description != NULL) {
        // Create a local copy of returned description for safe storage
        size_t len = strlen(description) + 1;
        char* descCopy = (char*)malloc(len);
        if (descCopy) {
            memcpy(descCopy, description, len);
        }

        // Free the API-owned memory immediately
        mumbleAPI.freeMemory(ownID, description);
        description = NULL;

        // Compare with cached description
        if (descCopy) {
            if (lastHubDescriptionCache == NULL || strcmp(lastHubDescriptionCache, descCopy) != 0) {
                // Changed -> replace cache and apply
                if (lastHubDescriptionCache) {
                    free(lastHubDescriptionCache);
                    lastHubDescriptionCache = NULL;
                }
                lastHubDescriptionCache = descCopy;

                if (enableLogGeneral) mumbleAPI.log(ownID, "readHubDescription: description changed -> parsing and applying");
                parseHubDescription(lastHubDescriptionCache);
            }
            else {
                // Unchanged -> discard new copy
                free(descCopy);
                if (enableLogGeneral) mumbleAPI.log(ownID, "readHubDescription: description unchanged");
            }
            hubDescriptionAvailable = TRUE;
        }
        else {
            // Allocation failed; still attempt parse directly (best-effort)
            if (enableLogGeneral) mumbleAPI.log(ownID, "readHubDescription: malloc failed for desc copy - attempting parse on original (best-effort)");
            // Note: original description was already freed, so skip parsing to avoid UB
            hubDescriptionAvailable = FALSE;
        }
    }
    else {
        hubDescriptionAvailable = FALSE;
        if (enableLogGeneral) {
            char errorMsg[256];
            snprintf(errorMsg, sizeof(errorMsg),
                "readHubDescription: Failed to get hub description after retries, last error: %d - no distance limits",
                result);
            mumbleAPI.log(ownID, errorMsg);
        }
    }
}

static void hubDescriptionMonitorThread(void* arg) {
    Sleep(2000);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Hub description monitor: PERMANENT monitoring active");
    }

    while (enableGetPlayerCoordinates) {
        readHubDescription();
        Sleep(2000);
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Hub description monitor: Stopped");
    }
}

// ============================================================================
// MODULE 5 : GESTION DES CANAUX
// ============================================================================

// Initialize channel IDs | Initialiser les IDs des canaux
static void initializeChannelIDs() {
    // Robust channel ID initialization with synchronization wait and name variants
    // Initialisation robuste des IDs de canaux avec attente de synchronisation et variantes de nom
    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        if (enableLogGeneral) mumbleAPI.log(ownID, "initializeChannelIDs: No active connection");
        return;
    }

    // Wait briefly for synchronization | Attendre brièvement la synchronisation
    bool synchronized = false;
    mumble_error_t syncRes = mumbleAPI.isConnectionSynchronized(ownID, connection, &synchronized);
    int attempts = 0;
    const int maxAttempts = 8;
    while ((syncRes != MUMBLE_STATUS_OK || !synchronized) && attempts < maxAttempts) {
        if (enableLogGeneral) {
            char msg[128];
            snprintf(msg, sizeof(msg), "initializeChannelIDs: waiting for synchronization (attempt %d/%d)", attempts + 1, maxAttempts);
            mumbleAPI.log(ownID, msg);
        }
        Sleep(200);
        syncRes = mumbleAPI.isConnectionSynchronized(ownID, connection, &synchronized);
        attempts++;
    }

    if (!synchronized) {
        if (enableLogGeneral) mumbleAPI.log(ownID, "initializeChannelIDs: connection not synchronized - aborting channel lookups");
        return;
    }

    // Try canonical names and case variants | Essayer les noms canoniques et variantes de casse
    mumble_error_t res;

    // Hub channel variants | Variantes pour le hub
    const char* hubNames[] = { "hub", "Hub", "HUB" };
    hubChannelID = -1;
    for (size_t i = 0; i < sizeof(hubNames) / sizeof(hubNames[0]); ++i) {
        res = mumbleAPI.findChannelByName(ownID, connection, hubNames[i], &hubChannelID);
        if (res == MUMBLE_STATUS_OK) {
            if (enableLogGeneral) {
                char msg[128];
                snprintf(msg, sizeof(msg), "initializeChannelIDs: Found hub channel as '%s' (id=%d)", hubNames[i], hubChannelID);
                mumbleAPI.log(ownID, msg);
            }
            break;
        }
    }

    // Root channel variants | Variantes pour Root
    const char* rootNames[] = { "Root", "root", "ROOT" };
    rootChannelID = -1;
    for (size_t i = 0; i < sizeof(rootNames) / sizeof(rootNames[0]); ++i) {
        res = mumbleAPI.findChannelByName(ownID, connection, rootNames[i], &rootChannelID);
        if (res == MUMBLE_STATUS_OK) {
            if (enableLogGeneral) {
                char msg[128];
                snprintf(msg, sizeof(msg), "initializeChannelIDs: Found Root channel as '%s' (id=%d)", rootNames[i], rootChannelID);
                mumbleAPI.log(ownID, msg);
            }
            break;
        }
    }

    // In-game channel variants | Variantes pour ingame
    const char* ingameNames[] = { "ingame", "InGame", "Ingame" };
    ingameChannelID = -1;
    for (size_t i = 0; i < sizeof(ingameNames) / sizeof(ingameNames[0]); ++i) {
        res = mumbleAPI.findChannelByName(ownID, connection, ingameNames[i], &ingameChannelID);
        if (res == MUMBLE_STATUS_OK) {
            if (enableLogGeneral) {
                char msg[128];
                snprintf(msg, sizeof(msg), "initializeChannelIDs: Found ingame channel as '%s' (id=%d)", ingameNames[i], ingameChannelID);
                mumbleAPI.log(ownID, msg);
            }
            break;
        }
    }

    // Activate channel management only if required channels found | Activer la gestion seulement si canaux trouvés
    if (hubChannelID != -1 && ingameChannelID != -1) {
        channelManagementActive = TRUE;
        if (enableLogGeneral) mumbleAPI.log(ownID, "initializeChannelIDs: channelManagementActive = TRUE");
    }
    else {
        channelManagementActive = FALSE;
        if (enableLogGeneral) mumbleAPI.log(ownID, "initializeChannelIDs: channelManagementActive = FALSE (missing hub/ingame)");
    }
}

// Automatic channel management based on coordinates validity | Gestion automatique des canaux selon la validité des coordonnées
static void manageChannelBasedOnCoordinates() {
    // Skip if automatic channel switching is disabled | Ignorer si le changement automatique de canal est désactivé
    if (!enableAutomaticChannelChange) {
        if (TEMP) {
            mumbleAPI.log(ownID, "CHANNEL: manageChannelBasedOnCoordinates() SKIPPED - enableAutomaticChannelChange=FALSE");
        }

        // Deactivate channel management when disabled by user | Désactiver la gestion des canaux quand désactivé par l'utilisateur
        if (channelManagementActive) {
            channelManagementActive = FALSE;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "CHANNEL MANAGEMENT: Deactivated due to user preference");
            }
        }
        return;
    }

    // Skip if channel management system is not active | Ignorer si le système de gestion des canaux n'est pas actif
    if (!channelManagementActive) {
        if (TEMP) {
            mumbleAPI.log(ownID, "CHANNEL: manageChannelBasedOnCoordinates() SKIPPED - channelManagementActive=FALSE");
        }
        return;
    }

    // Rate limiting to prevent excessive channel operations | Limitation de fréquence pour éviter les opérations excessives
    ULONGLONG currentTick = GetTickCount64();
    if (currentTick - lastChannelCheck < 500) return;
    lastChannelCheck = currentTick;

    // Verify active server connection | Vérifier la connexion serveur active
    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        return;
    }

    // Ensure client is synchronized with server | Assurer que le client est synchronisé avec le serveur
    bool synchronized = false;
    if (mumbleAPI.isConnectionSynchronized(ownID, connection, &synchronized) != MUMBLE_STATUS_OK || !synchronized) {
        return;
    }

    // Get local user ID for channel operations | Obtenir l'ID utilisateur local pour les opérations de canal
    mumble_userid_t localUserID;
    if (mumbleAPI.getLocalUserID(ownID, connection, &localUserID) != MUMBLE_STATUS_OK) {
        return;
    }

    // Get current channel of the user | Obtenir le canal actuel de l'utilisateur
    mumble_channelid_t currentChannel;
    if (mumbleAPI.getChannelOfUser(ownID, connection, localUserID, &currentChannel) != MUMBLE_STATUS_OK) {
        return;
    }

    // Debug logging for channel management state | Log de debug pour l'état de gestion des canaux
    if (TEMP) {
        char stateMsg[256];
        snprintf(stateMsg, sizeof(stateMsg),
            "CHANNEL MANAGEMENT: Current=%d, Hub=%d, InGame=%d, CoordsValid=%s",
            currentChannel, hubChannelID, ingameChannelID, coordinatesValid ? "TRUE" : "FALSE");
        mumbleAPI.log(ownID, stateMsg);
    }

    // Move to appropriate channel based on coordinate validity | Déplacer vers le canal approprié selon la validité des coordonnées
    if (coordinatesValid) {
        if (currentChannel != ingameChannelID && ingameChannelID != -1) {
            mumbleAPI.requestUserMove(ownID, connection, localUserID, ingameChannelID, NULL);
            lastValidChannel = ingameChannelID;

            if (TEMP) {
                char moveMsg[128];
                snprintf(moveMsg, sizeof(moveMsg), "CHANNEL: Moved to InGame (%d) - coordinates valid", ingameChannelID);
                mumbleAPI.log(ownID, moveMsg);
            }
        }
    }
    else {
        if (currentChannel != hubChannelID && hubChannelID != -1) {
            mumbleAPI.requestUserMove(ownID, connection, localUserID, hubChannelID, NULL);
            lastValidChannel = hubChannelID;

            if (TEMP) {
                char moveMsg[128];
                snprintf(moveMsg, sizeof(moveMsg), "CHANNEL: Moved to Hub (%d) - coordinates invalid", hubChannelID);
                mumbleAPI.log(ownID, moveMsg);
            }
        }
    }
}

static void channelManagementThread(void* arg) {
    Sleep(3000);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Channel management thread: PERMANENT monitoring active");
    }

    while (enableGetPlayerCoordinates) {
        checkConnectionStatus();

        if (enableAutomaticChannelChange) {
            manageChannelBasedOnCoordinates();
        }

        Sleep(500);
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Channel management thread: Stopped");
    }
}

// ============================================================================
// MODULE 6 : AUDIO - CALCULS DE VOLUME
// ============================================================================

// Calculate volume multiplier by distance | Calculer le multiplicateur de volume selon la distance
static float calculateVolumeMultiplier(float distance, float maxDistance) {
    if (distance >= maxDistance) {
        return 0.01f;
    }

    if (distance <= 1.0f) {
        return 1.0f;
    }

    float minDistance = 1.0f;
    float normalizedDistance = (distance - minDistance) / (maxDistance - minDistance);
    normalizedDistance = fmaxf(0.0f, fminf(normalizedDistance, 1.0f));

    float volumeMultiplier = 1.0f / (1.0f + 2.0f * normalizedDistance * normalizedDistance);

    if (volumeMultiplier < 0.01f) {
        volumeMultiplier = 0.01f;
    }

    return volumeMultiplier;
}

static float calculateVolumeMultiplierWithHubSettings(float distance, float voiceDistance) {
    if (hubDescriptionAvailable && hubForcePositionalAudio) {
        float minDistance = (float)hubAudioMinDistance;
        float bloom = (float)hubAudioBloom;
        float maxVolumeFromServer = (float)hubAudioMaxVolume / 100.0f;

        if (distance >= voiceDistance) {
            return 0.0f;
        }

        if (distance <= minDistance) {
            return maxVolumeFromServer;
        }

        float effectiveRange = voiceDistance - minDistance;
        if (effectiveRange <= 0.0f) effectiveRange = 1.0f;

        float rel = (distance - minDistance) / effectiveRange;
        rel = fmaxf(0.0f, fminf(rel, 1.0f));

        float physicalBase = 1.0f + (distance / minDistance) * 1.5f;
        float physical = 1.0f / (physicalBase * physicalBase);

        float perceptualExponent = 1.8f + (bloom * 1.5f);
        float perceptual = powf(1.0f - rel, perceptualExponent);

        float rawVolume = physical * (1.0f - bloom) + perceptual * bloom;
        float volumeMultiplier = rawVolume * maxVolumeFromServer;

        volumeMultiplier = volumeMultiplier * (0.8f + 0.2f * sqrtf(volumeMultiplier));

        return fmaxf(0.0f, fminf(volumeMultiplier, maxVolumeFromServer));
    }
    else if (hubDescriptionAvailable && !hubForcePositionalAudio) {
        float minDistance = 0.0f;
        float maxDistance = voiceDistance;
        float bloom = 1.3f;

        float maxVolumeFromServer = (float)hubAudioMaxVolume / 100.0f;

        if (distance >= maxDistance) {
            return 0.0f;
        }

        if (distance <= 1.0f) {
            float baseVolumeRange = maxVolumeFromServer - 0.60f;
            if (baseVolumeRange < 0.0f) baseVolumeRange = 0.20f;

            float baseVolume = 0.60f + (distance * baseVolumeRange);
            return fminf(baseVolume, maxVolumeFromServer);
        }

        float rel = distance / maxDistance;
        rel = fmaxf(0.0f, fminf(rel, 1.0f));

        float exponent = 2.5f + (bloom * 1.2f);
        float volumeMultiplier = powf(1.0f - rel, exponent);

        volumeMultiplier *= maxVolumeFromServer;
        volumeMultiplier = volumeMultiplier * (0.85f + 0.15f * sqrtf(volumeMultiplier));

        return fmaxf(0.0f, fminf(volumeMultiplier, maxVolumeFromServer));
    }
    else {
        return (float)calculateVolumeMultiplier(distance, voiceDistance);
    }
}


// Apply smoothing to avoid abrupt volume changes | Appliquer un lissage pour éviter les sauts brusques de volume
static float smoothVolume(float current, float target) {
    float difference = fabsf(target - current);

    if (difference < 0.01f) {
        return current * 0.99f + target * 0.01f;
    }
    else if (difference < 0.03f) {
        return current * 0.97f + target * 0.03f;
    }
    else if (difference < 0.08f) {
        return current * 0.93f + target * 0.07f;
    }
    else if (difference < 0.15f) {
        return current * 0.85f + target * 0.15f;
    }
    else {
        return current * 0.75f + target * 0.25f;
    }
}

// ============================================================================
// MODULE 7 : AUDIO - FILTRES
// ============================================================================

// Find or create low pass filter state | Fonction pour trouver ou créer l'état du filtre d'un utilisateur
static LowPassFilterState* findOrCreateLowPassState(mumble_userid_t userID) {
    for (size_t i = 0; i < lowPassStateCount; i++) {
        if (lowPassStates[i].userID == userID) {
            lowPassStates[i].lastUpdate = GetTickCount64();
            return &lowPassStates[i];
        }
    }

    if (lowPassStateCount < 64) {
        LowPassFilterState* newState = &lowPassStates[lowPassStateCount];
        newState->userID = userID;
        newState->lastSampleLeft = 0.0f;
        newState->lastSampleRight = 0.0f;
        newState->lastCutoff = -1.0f;
        newState->lastAlpha = 0.0f;
        newState->lastUpdate = GetTickCount64();
        newState->isInitialized = FALSE;
        lowPassStateCount++;
        return newState;
    }

    return NULL;
}

// Apply low pass filter | Fonction pour appliquer le filtre passe-bas
static void applyLowPassFilter(float* samples, uint32_t sampleCount, uint16_t channelCount,
    float cutoffHz, uint32_t sampleRate, LowPassFilterState* filterState) {
    if (!samples || !filterState || sampleCount == 0 || sampleRate == 0) return;

    // Recalculate alpha only if cutoff changed | Recalculer alpha seulement si cutoff a changé
    if (cutoffHz != filterState->lastCutoff) {
        const float dt = 1.0f / (float)sampleRate;
        const float RC = 1.0f / (cutoffHz * 6.28318530718f);
        filterState->lastAlpha = dt / (RC + dt);
        filterState->lastCutoff = cutoffHz;
    }

    const float alpha = filterState->lastAlpha;

    if (channelCount == 1) {
        // Mono processing | Traitement mono
        for (uint32_t i = 0; i < sampleCount; i++) {
            if (!filterState->isInitialized) {
                filterState->lastSampleLeft = samples[i];
                filterState->isInitialized = TRUE;
            }

            filterState->lastSampleLeft += alpha * (samples[i] - filterState->lastSampleLeft);
            float secondPass = filterState->lastSampleLeft;
            filterState->lastSampleLeft += alpha * 0.7f * (secondPass - filterState->lastSampleLeft);
            samples[i] = filterState->lastSampleLeft;
        }
    }
    else if (channelCount == 2) {
        // Stereo processing | Traitement stéréo
        for (uint32_t sample = 0; sample < sampleCount; sample++) {
            uint32_t leftIdx = sample * 2;
            uint32_t rightIdx = sample * 2 + 1;

            if (!filterState->isInitialized) {
                filterState->lastSampleLeft = samples[leftIdx];
                filterState->lastSampleRight = samples[rightIdx];
                filterState->isInitialized = TRUE;
            }

            filterState->lastSampleLeft += alpha * (samples[leftIdx] - filterState->lastSampleLeft);
            float leftSecond = filterState->lastSampleLeft;
            filterState->lastSampleLeft += alpha * 0.7f * (leftSecond - filterState->lastSampleLeft);
            samples[leftIdx] = filterState->lastSampleLeft;

            filterState->lastSampleRight += alpha * (samples[rightIdx] - filterState->lastSampleRight);
            float rightSecond = filterState->lastSampleRight;
            filterState->lastSampleRight += alpha * 0.7f * (rightSecond - filterState->lastSampleRight);
            samples[rightIdx] = filterState->lastSampleRight;
        }
    }
}

// Apply air diffusion effect | Fonction pour appliquer l'effet de diffusion d'air
static void applyAirDiffusion(float* samples, uint32_t sampleCount, uint16_t channelCount, float intensity) {
    if (!samples || intensity <= 0.01f || sampleCount < 2) return;

    const float diffusionFactor = intensity * 0.05f;

    if (channelCount == 1) {
        for (uint32_t i = 1; i < sampleCount; i++) {
            float average = (samples[i - 1] + samples[i]) * 0.5f;
            samples[i] = samples[i] * (1.0f - diffusionFactor) + average * diffusionFactor;
        }
    }
    else if (channelCount == 2) {
        for (uint32_t sample = 1; sample < sampleCount; sample++) {
            uint32_t leftIdx = sample * 2;
            uint32_t rightIdx = sample * 2 + 1;
            uint32_t prevLeftIdx = (sample - 1) * 2;
            uint32_t prevRightIdx = (sample - 1) * 2 + 1;

            float avgLeft = (samples[prevLeftIdx] + samples[leftIdx]) * 0.5f;
            float avgRight = (samples[prevRightIdx] + samples[rightIdx]) * 0.5f;

            samples[leftIdx] = samples[leftIdx] * (1.0f - diffusionFactor) + avgLeft * diffusionFactor;
            samples[rightIdx] = samples[rightIdx] * (1.0f - diffusionFactor) + avgRight * diffusionFactor;
        }
    }
}

// Cleanup low pass states | Fonction pour nettoyer les états de filtre passe-bas
static void cleanupLowPassStates() {
    lowPassStateCount = 0;
    memset(lowPassStates, 0, sizeof(lowPassStates));

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Low-pass filter states cleaned up");
    }
}

// ============================================================================
// MODULE 8 : AUDIO - ÉTATS DES JOUEURS
// ============================================================================

// Find or create audio volume state | Fonction pour trouver ou créer l'état audio d'un utilisateur
static AudioVolumeState* findOrCreateAudioVolumeState(mumble_userid_t userID) {
    for (size_t i = 0; i < audioVolumeCount; i++) {
        if (audioVolumeStates[i].userID == userID) {
            return &audioVolumeStates[i];
        }
    }

    if (audioVolumeCount < 64) {
        AudioVolumeState* newState = &audioVolumeStates[audioVolumeCount];
        newState->userID = userID;
        newState->targetVolume = 1.0f;
        newState->currentVolume = 1.0f;
        newState->leftVolume = 1.0f;
        newState->rightVolume = 1.0f;
        newState->lastUpdate = GetTickCount64();
        newState->isValid = true;
        audioVolumeCount++;
        return newState;
    }

    return NULL;
}

// Update user adaptive volume | Fonction pour mettre à jour le volume cible d'un utilisateur
static void setUserAdaptiveVolume(mumble_userid_t userID, float targetVolume) {
    AudioVolumeState* audioState = findOrCreateAudioVolumeState(userID);
    if (audioState) {
        float currentVolume = audioState->targetVolume;
        float volumeDiff = fabsf(targetVolume - currentVolume);

        if (volumeDiff > 0.3f) {
            if (targetVolume > currentVolume) {
                targetVolume = currentVolume + 0.3f;
            }
            else {
                targetVolume = currentVolume - 0.3f;
            }
        }

        audioState->targetVolume = targetVolume;
        audioState->lastUpdate = GetTickCount64();

        if (audioState->targetVolume < 0.0f) audioState->targetVolume = 0.0f;
        if (audioState->targetVolume > 1.0f) audioState->targetVolume = 1.0f;
    }
}


// Update volume with spatialization | Mettre à jour le volume avec spatialisation
static void setUserAdaptiveVolumeWithSpatial(mumble_userid_t userID, float baseVolume, float leftVol, float rightVol) {
    AudioVolumeState* audioState = findOrCreateAudioVolumeState(userID);
    if (audioState) {
        float volumeDiff = fabsf(baseVolume - audioState->targetVolume);
        if (volumeDiff > 0.3f) {
            if (baseVolume > audioState->targetVolume) {
                baseVolume = audioState->targetVolume + 0.3f;
            }
            else {
                baseVolume = audioState->targetVolume - 0.3f;
            }
        }

        audioState->targetVolume = baseVolume;
        audioState->leftVolume = leftVol;
        audioState->rightVolume = rightVol;
        audioState->lastUpdate = GetTickCount64();

        if (audioState->targetVolume < 0.0f) audioState->targetVolume = 0.0f;
        if (audioState->targetVolume > 1.0f) audioState->targetVolume = 1.0f;
        if (audioState->leftVolume < 0.0f) audioState->leftVolume = 0.0f;
        if (audioState->leftVolume > 1.0f) audioState->leftVolume = 1.0f;
        if (audioState->rightVolume < 0.0f) audioState->rightVolume = 0.0f;
        if (audioState->rightVolume > 1.0f) audioState->rightVolume = 1.0f;
    }
}

// Cleanup audio volume states | Fonction pour nettoyer les états de volume audio
static void cleanupAudioVolumeStates() {
    audioVolumeCount = 0;
    memset(audioVolumeStates, 0, sizeof(audioVolumeStates));

    cleanupLowPassStates();

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Audio volume states and optimized low-pass filters cleaned up");
    }
}

// ============================================================================
// MODULE 9 : AUDIO - SYSTÈME ADAPTATIF
// ============================================================================


// Find or create adaptive player state | Trouver ou créer l'état adaptatif d'un joueur
static AdaptivePlayerData* findOrCreateAdaptivePlayerState(mumble_userid_t userID, const char* playerName) {
    for (size_t i = 0; i < adaptivePlayerCount; i++) {
        if (adaptivePlayerStates[i].userID == userID) {
            return &adaptivePlayerStates[i];
        }
    }

    if (adaptivePlayerCount < 64) {
        AdaptivePlayerData* newState = &adaptivePlayerStates[adaptivePlayerCount];
        newState->userID = userID;
        strncpy_s(newState->playerName, sizeof(newState->playerName), playerName, _TRUNCATE);
        newState->currentVolume = 1.0f;
        newState->isValid = false;
        newState->lastVolumeUpdate = 0;
        adaptivePlayerCount++;
        return newState;
    }

    return NULL;
}

// Process adaptive volume data with hub parameters | Fonction principale de traitement du volume adaptatif avec paramètres hub
static void processAdaptiveVolumeData(const CompletePositionalData* receivedData, mumble_userid_t senderID) {
    if (!receivedData || !enableDistanceMuting) return;

    ULONGLONG currentTime = GetTickCount64();

    localPlayerPosition.x = localVoiceData.x;
    localPlayerPosition.y = localVoiceData.y;
    localPlayerPosition.z = localVoiceData.z;

    Vector3 remotePosition = { receivedData->x, receivedData->y, receivedData->z };
    float distance = calculateDistance3D(&localPlayerPosition, &remotePosition);

    float targetVolume = calculateVolumeMultiplierWithHubSettings(distance, receivedData->voiceDistance);

    setUserAdaptiveVolume(senderID, targetVolume);

    AdaptivePlayerData* adaptiveState = findOrCreateAdaptivePlayerState(senderID, receivedData->playerName);
    if (adaptiveState) {
        adaptiveState->currentVolume = targetVolume;
        adaptiveState->position = remotePosition;
        adaptiveState->voiceDistance = receivedData->voiceDistance;
        adaptiveState->isValid = true;
        adaptiveState->lastVolumeUpdate = currentTime;
    }
}

// Cleanup adaptive player states | Fonction de nettoyage pour le système adaptatif
static void cleanupAdaptivePlayerStates() {
    adaptivePlayerCount = 0;
    memset(adaptivePlayerStates, 0, sizeof(adaptivePlayerStates));
}

// Get adaptive player volume | Fonction pour obtenir le volume d'un joueur
static float getAdaptivePlayerVolume(mumble_userid_t userID) {
    for (size_t i = 0; i < adaptivePlayerCount; i++) {
        if (adaptivePlayerStates[i].userID == userID && adaptivePlayerStates[i].isValid) {
            return adaptivePlayerStates[i].currentVolume;
        }
    }
    return 0.0f;
}

// Check if player is audible | Fonction pour vérifier si un joueur est audible
static bool isPlayerAdaptivelyAudible(mumble_userid_t userID) {
    return getAdaptivePlayerVolume(userID) > MIN_AUDIBLE_VOLUME;
}

// Get distance to player | Fonction pour obtenir la distance à un joueur
static float getAdaptiveDistanceToPlayer(mumble_userid_t userID) {
    for (size_t i = 0; i < adaptivePlayerCount; i++) {
        if (adaptivePlayerStates[i].userID == userID && adaptivePlayerStates[i].isValid) {
            return calculateDistance3D(&localPlayerPosition, &adaptivePlayerStates[i].position);
        }
    }
    return -1.0f;
}

// ============================================================================
// MODULE 10 : AUDIO - APPLICATION DES DISTANCES
// ============================================================================
// Apply distance changes to all connected players | Appliquer les changements de distance à tous les joueurs connectés
static void applyDistanceToAllPlayers() {
    if (!enableDistanceMuting) return;

    localPlayerPosition.x = localVoiceData.x;
    localPlayerPosition.y = localVoiceData.y;
    localPlayerPosition.z = localVoiceData.z;

    // Recalculate all audio volumes | Recalculer tous les volumes audio
    for (size_t i = 0; i < adaptivePlayerCount; i++) {
        AdaptivePlayerData* adaptiveState = &adaptivePlayerStates[i];

        if (adaptiveState->isValid) {
            float distance = calculateDistance3D(&localPlayerPosition, &adaptiveState->position);
            float maxHearingDistance = adaptiveState->voiceDistance;

            float targetVolume = calculateVolumeMultiplier(distance, maxHearingDistance);

            // Apply via audio system instead of mute | Appliquer via le système audio au lieu du mute
            setUserAdaptiveVolume(adaptiveState->userID, targetVolume);

            adaptiveState->currentVolume = targetVolume;

            if (enableLogGeneral) {
                static ULONGLONG lastInstantLog = 0;
                ULONGLONG currentTime = GetTickCount64();
                if (currentTime - lastInstantLog > 10000) {
                    char logMsg[256];
                    snprintf(logMsg, sizeof(logMsg),
                        "Instant volume update: Player %s: Distance=%.1fm, Volume=%.0f%%",
                        adaptiveState->playerName, distance, targetVolume * 100.0f);
                    mumbleAPI.log(ownID, logMsg);
                    lastInstantLog = currentTime;
                }
            }
        }
    }
}  // ⭐ Appelée par Module 3 ET Module 12

// ============================================================================
// MODULE 11 : CALLBACK AUDIO MUMBLE
// ============================================================================

PLUGIN_EXPORT bool PLUGIN_CALLING_CONVENTION mumble_onAudioSourceFetched(
    float* outputPCM,
    uint32_t sampleCount,
    uint16_t channelCount,
    uint32_t sampleRate,
    bool isSpeech,
    mumble_userid_t userID) {

    if (!isSpeech || !enableDistanceMuting) {
        return false;
    }

    AudioVolumeState* audioState = findOrCreateAudioVolumeState(userID);
    if (!audioState) {
        return false;
    }

    // Volume smoothing | Lissage du volume
    float volumeDifference = audioState->targetVolume - audioState->currentVolume;
    float smoothingFactor = 0.25f;
    audioState->currentVolume += volumeDifference * smoothingFactor;

    // Get distance for filter | Récupérer la distance pour le filtre
    float distance = 0.0f;
    float voiceDistance = 15.0f;
    bool hasDistanceData = false;

    for (size_t i = 0; i < adaptivePlayerCount; i++) {
        if (adaptivePlayerStates[i].userID == userID && adaptivePlayerStates[i].isValid) {
            Vector3 localPos = { localPlayerPosition.x, localPlayerPosition.y, localPlayerPosition.z };
            distance = (float)calculateDistance3D(&localPos, &adaptivePlayerStates[i].position);
            voiceDistance = adaptivePlayerStates[i].voiceDistance;
            hasDistanceData = true;
            break;
        }
    }

    // Silence if volume too low | Silence si volume trop faible
    if (audioState->currentVolume < 0.02f) {
        for (uint32_t i = 0; i < sampleCount * channelCount; i++) {
            outputPCM[i] = 0.0f;
        }
        return true;
    }

    // Apply stereo/mono processing | Application stéréo/mono
    if (channelCount == 2) {
        float leftVolume = audioState->leftVolume;
        float rightVolume = audioState->rightVolume;

        static float lastLeftVolume[64] = { 0 };
        static float lastRightVolume[64] = { 0 };

        int userIndex = userID % 64;
        float spatialSmoothFactor = 0.2f;

        leftVolume = lastLeftVolume[userIndex] * (1.0f - spatialSmoothFactor) + leftVolume * spatialSmoothFactor;
        rightVolume = lastRightVolume[userIndex] * (1.0f - spatialSmoothFactor) + rightVolume * spatialSmoothFactor;

        lastLeftVolume[userIndex] = leftVolume;
        lastRightVolume[userIndex] = rightVolume;

        for (uint32_t sample = 0; sample < sampleCount; sample++) {
            uint32_t leftIndex = sample * 2;
            uint32_t rightIndex = sample * 2 + 1;

            outputPCM[leftIndex] *= leftVolume;
            outputPCM[rightIndex] *= rightVolume;
        }
    }
    else {
        float finalVolume = audioState->currentVolume;
        for (uint32_t i = 0; i < sampleCount; i++) {
            outputPCM[i] *= finalVolume;
        }
    }

    // Smart filtering with AudioFilterIntensity | Nouveau filtre intelligent avec AudioFilterIntensity
    if (!hasDistanceData) {
        distance = 5.0f;
        voiceDistance = 35.0f;
    }

    // Filter with curve controlled by AudioFilterIntensity | Filtre avec courbe contrôlée par AudioFilterIntensity
    if (distance >= 0.0f && voiceDistance > 0.0f && hubAudioFilterIntensity > 0.01f) {
        float minDistance = (float)hubAudioMinDistance;

        float protectedZone = minDistance * 0.5f;
        if (protectedZone < 1.0f) protectedZone = 1.0f;

        float filterStartDistance = protectedZone;
        float filterDistance = distance - filterStartDistance;

        // No filter in protected zone | Pas de filtre dans la zone protégée
        if (distance <= filterStartDistance) {
            if (enableLogGeneral) {
                static ULONGLONG lastClearLog = 0;
                ULONGLONG currentTime = GetTickCount64();
                if (currentTime - lastClearLog > 3000) {
                    char logMsg[256];
                    snprintf(logMsg, sizeof(logMsg),
                        "🔊 CLEAR AUDIO: User=%u Dist=%.1fm (Protected zone ≤%.1fm) - No filter applied",
                        userID, distance, filterStartDistance);
                    mumbleAPI.log(ownID, logMsg);
                    lastClearLog = currentTime;
                }
            }
        }
        else {
            // Apply filter with controlled curve | Appliquer le filtre avec courbe contrôlée
            float effectiveFilterRange = voiceDistance - filterStartDistance;
            float filterRel = filterDistance / effectiveFilterRange;
            if (filterRel > 1.0f) filterRel = 1.0f;
            if (filterRel < 0.0f) filterRel = 0.0f;

            float baseFilterStrength = filterRel;

            float filterStrength;
            if (hubAudioFilterIntensity <= 0.5f) {
                float softness = (float)(0.5f - hubAudioFilterIntensity) * 2.0f;
                filterStrength = powf(baseFilterStrength, 2.0f + softness * 2.0f);
            }
            else {
                float aggressiveness = (float)(hubAudioFilterIntensity - 0.5f) * 2.0f;
                filterStrength = powf(baseFilterStrength, 0.5f + aggressiveness * 0.3f);
            }

            filterStrength *= (float)hubAudioFilterIntensity;

            float maxCutoff = 18000.0f;
            float minCutoff = 200.0f;

            float cutoffHz = maxCutoff - (filterStrength * (maxCutoff - minCutoff));

            LowPassFilterState* filterState = findOrCreateLowPassState(userID);
            if (filterState) {
                applyLowPassFilter(outputPCM, sampleCount, channelCount, cutoffHz, sampleRate, filterState);

                if (enableAirDiffusion && filterStrength >= 0.3f) {
                    float diffusionIntensity = (float)(((float)filterStrength - 0.3f) / 0.7f * (float)hubAudioFilterIntensity);
                    if (diffusionIntensity > 1.0f) diffusionIntensity = 1.0f;
                    applyAirDiffusion(outputPCM, sampleCount, channelCount, diffusionIntensity);
                }

                if (enableLogGeneral) {
                    static ULONGLONG lastFilterLog = 0;
                    ULONGLONG currentTime = GetTickCount64();
                    if (currentTime - lastFilterLog > 3000) {
                        char logMsg[256];
                        snprintf(logMsg, sizeof(logMsg),
                            "🎛️ SMART FILTER: User=%u Dist=%.1fm FilterStr=%.2f Cutoff=%.0fHz Intensity=%.2f",
                            userID, distance, filterStrength, cutoffHz, hubAudioFilterIntensity);
                        mumbleAPI.log(ownID, logMsg);
                        lastFilterLog = currentTime;
                    }
                }
            }
        }
    }

    return true;
}
// ============================================================================
// MODULE 12 : SYSTÈME DE VOIX ET MODES
// ============================================================================

// Get voice distance for mode | Obtenir la distance de voix selon le mode
static float getVoiceDistanceForMode(uint8_t voiceMode) {
    switch (voiceMode) {
    case 0: return distanceWhisper;
    case 1: return distanceNormal;
    case 2: return distanceShout;
    default: return distanceNormal;
    }
}

// Get local player name | Obtenir le nom du joueur local
static void getLocalPlayerName() {
    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) == MUMBLE_STATUS_OK) {
        mumble_userid_t localUserID;
        if (mumbleAPI.getLocalUserID(ownID, connection, &localUserID) == MUMBLE_STATUS_OK) {
            const char* userName = NULL;
            if (mumbleAPI.getUserName(ownID, connection, localUserID, &userName) == MUMBLE_STATUS_OK) {
                if (userName) {
                    strncpy_s(localVoiceData.playerName, sizeof(localVoiceData.playerName), userName, _TRUNCATE);
                    mumbleAPI.freeMemory(ownID, userName);
                }
            }
        }
    }
}

// Voice mode cycling function | Fonction pour cycliser entre les modes de voix
static void cycleVoiceMode() {
    if (!enableVoiceToggle) return;

    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastVoiceTogglePress < 300) return;
    lastVoiceTogglePress = currentTime;

    uint8_t currentVoiceMode = 1;

    if (fabsf(localVoiceData.voiceDistance - distanceWhisper) < 0.5f) {
        currentVoiceMode = 0;
    }
    else if (fabsf(localVoiceData.voiceDistance - distanceShout) < 0.5f) {
        currentVoiceMode = 2;
    }
    else {
        currentVoiceMode = 1;
    }

    uint8_t newMode;
    switch (currentVoiceMode) {
    case 1: newMode = 2; break;
    case 2: newMode = 0; break;
    case 0: newMode = 1; break;
    default: newMode = 1; break;
    }

    localVoiceData.voiceDistance = getVoiceDistanceForMode(newMode);

    lastVoiceDataSent = 0;
    applyDistanceToAllPlayers();

    updateVoiceOverlay();

    char modeNames[][10] = { "Whisper", "Normal", "Shout" };
    char chatMessage[128];
    snprintf(chatMessage, sizeof(chatMessage),
        "🔄 Voice mode switched to: %s - Distance: %.1f meters",
        modeNames[newMode], localVoiceData.voiceDistance);
    displayInChat(chatMessage);

    if (enableLogGeneral) {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "TOGGLE: Voice mode cycled to %s (Distance: %.1fm)",
            modeNames[newMode], localVoiceData.voiceDistance);
        mumbleAPI.log(ownID, logMsg);
    }
}

static void updateVoiceMode() {
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastKeyCheck < 40) return;
    lastKeyCheck = currentTime;

    // CORRECTION: Déterminer le mode actuel depuis la distance réelle
    // Determine current mode from actual distance | Déterminer le mode actuel depuis la distance réelle
    uint8_t lastVoiceMode = 1; // Default Normal
    if (fabsf(localVoiceData.voiceDistance - distanceWhisper) < 0.5f) {
        lastVoiceMode = 0;
    }
    else if (fabsf(localVoiceData.voiceDistance - distanceShout) < 0.5f) {
        lastVoiceMode = 2;
    }
    else {
        lastVoiceMode = 1;
    }

    uint8_t newVoiceMode = lastVoiceMode;

    if (enableVoiceToggle) {
        if (GetAsyncKeyState(voiceToggleKey) & 0x8000) {
            if (currentTime - lastVoiceTogglePress >= 200) {
                lastVoiceTogglePress = currentTime;

                switch (lastVoiceMode) {
                case 1: newVoiceMode = 2; break; // Normal → Shout
                case 2: newVoiceMode = 0; break; // Shout → Whisper  
                case 0: newVoiceMode = 1; break; // Whisper → Normal
                default: newVoiceMode = 1; break;
                }

                localVoiceData.voiceDistance = getVoiceDistanceForMode(newVoiceMode);

                lastVoiceDataSent = 0;
                applyDistanceToAllPlayers();

                // CORRECTION: Force overlay update immediately | Forcer la mise à jour immédiate de l'overlay
                updateVoiceOverlay();

                char modeNames[][10] = { "Whisper", "Normal", "Shout" };
                char chatMessage[128];
                snprintf(chatMessage, sizeof(chatMessage),
                    "🔄 Voice mode toggled to: %s - Distance: %.1f meters",
                    modeNames[newVoiceMode], localVoiceData.voiceDistance);
                displayInChat(chatMessage);
            }
        }
        return;
    }
    else {
        // Detection with priority and state memory | Détection avec priorité et mémorisation de l'état
        static BOOL lastWhisperPressed = FALSE;
        static BOOL lastShoutPressed = FALSE;
        static BOOL lastNormalPressed = FALSE;

        BOOL whisperPressed = (GetAsyncKeyState(whisperKey) & 0x8000) != 0;
        BOOL shoutPressed = (GetAsyncKeyState(shoutKey) & 0x8000) != 0;
        BOOL normalPressed = (GetAsyncKeyState(normalKey) & 0x8000) != 0;

        // Detect NEW presses (rising edge) | Détecter les NOUVELLES pressions (front montant)
        if (whisperPressed && !lastWhisperPressed) {
            newVoiceMode = 0; // Whisper
        }
        else if (shoutPressed && !lastShoutPressed) {
            newVoiceMode = 2; // Shout
        }
        else if (normalPressed && !lastNormalPressed) {
            newVoiceMode = 1; // Normal
        }

        // Remember key states | Mémoriser l'état des touches
        lastWhisperPressed = whisperPressed;
        lastShoutPressed = shoutPressed;
        lastNormalPressed = normalPressed;

        if (newVoiceMode != lastVoiceMode) {
            localVoiceData.voiceDistance = getVoiceDistanceForMode(newVoiceMode);

            lastVoiceDataSent = 0;
            applyDistanceToAllPlayers();

            // CORRECTION: Force overlay update immediately | Forcer la mise à jour immédiate de l'overlay
            updateVoiceOverlay();

            char modeNames[][10] = { "Whisper", "Normal", "Shout" };
            char chatMessage[128];
            snprintf(chatMessage, sizeof(chatMessage),
                "Voice mode: %s - Distance: %.1f meters",
                modeNames[newVoiceMode],
                localVoiceData.voiceDistance);

            displayInChat(chatMessage);
        }
    }
}

static void calculateLocalPositionalData(CompletePositionalData* localData) {
    if (!localData) return;

    // Position in meters (correct conversion) | Position en mètres (conversion correcte)
    localData->x = axe_x / 100.0f;
    localData->y = axe_y / 100.0f;
    localData->z = axe_z / 100.0f;

    // Complete player direction | Direction complète du joueur
    localData->dirX = avatarAxisX;
    localData->dirY = avatarAxisY;
    localData->dirZ = avatarAxisZ;

    // Vertical axis (up vector) for complete orientation | Axe vertical (up vector) pour orientation complète
    localData->axisX = 0.0f;
    localData->axisY = 1.0f;
    localData->axisZ = 0.0f;

    // Voice distance according to current mode | Distance de voix selon le mode actuel
    localData->voiceDistance = localVoiceData.voiceDistance;

    // Player name | Nom du joueur
    if (strlen(localVoiceData.playerName) > 0) {
        strncpy_s(localData->playerName, sizeof(localData->playerName),
            localVoiceData.playerName, _TRUNCATE);
    }
    else {
        strcpy_s(localData->playerName, sizeof(localData->playerName), "Player");
    }

    // Debug logging of sent data | Log des données envoyées
    if (enableLogGeneral) {
        static ULONGLONG lastSendDebug = 0;
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastSendDebug > 3000) {
            char debugMsg[512];
            snprintf(debugMsg, sizeof(debugMsg),
                "📤 SENDING COMPLETE DATA: Pos(%.1f,%.1f,%.1f) Dir(%.3f,%.3f,%.3f) Axis(%.1f,%.1f,%.1f) VoiceDist=%.1f Name='%s'",
                localData->x, localData->y, localData->z,
                localData->dirX, localData->dirY, localData->dirZ,
                localData->axisX, localData->axisY, localData->axisZ,
                localData->voiceDistance, localData->playerName);
            mumbleAPI.log(ownID, debugMsg);
            lastSendDebug = currentTime;
        }
    }
}


// Send complete positional data at 15ms intervals | Envoi des données positionnelles complètes à 15ms
static void sendCompletePositionalData() {
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastVoiceDataSent < 15) return;
    lastVoiceDataSent = currentTime;

    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        return;
    }

    CompletePositionalData localData;
    calculateLocalPositionalData(&localData);

    // Critical logging of sent data | Log critique des données envoyées
    if (enableLogGeneral) {
        static ULONGLONG lastSendDebugLog = 0;
        if (currentTime - lastSendDebugLog > 2000) {
            char logMsg[512];
            snprintf(logMsg, sizeof(logMsg),
                "📤 SENDING RAW DATA: Pos(%.6f,%.6f,%.6f) Dir(%.6f,%.6f,%.6f) VoiceDist=%.6f Name='%s' LocalRaw(%.3f,%.3f,%.3f)",
                localData.x, localData.y, localData.z, localData.dirX, localData.dirY, localData.dirZ,
                localData.voiceDistance, localData.playerName, axe_x, axe_y, axe_z);
            mumbleAPI.log(ownID, logMsg);
            lastSendDebugLog = currentTime;
        }
    }

    mumble_userid_t* allUsers = NULL;
    size_t userCount = 0;

    if (mumbleAPI.getAllUsers(ownID, connection, &allUsers, &userCount) == MUMBLE_STATUS_OK) {
        if (allUsers && userCount > 0) {
            mumble_error_t result = mumbleAPI.sendData(ownID, connection, allUsers, userCount,
                (const uint8_t*)&localData, sizeof(CompletePositionalData),
                "ConanExiles_CompletePositional");

            mumbleAPI.freeMemory(ownID, allUsers);
        }
    }
}

static void calculateLocalPositionalAudio(const CompletePositionalData* remoteData, mumble_userid_t userID) {
    if (!remoteData || !enableDistanceMuting) return;

    // Local position in meters | Position locale en mètres
    Vector3 localPos = {
        axe_x / 100.0f,
        axe_y / 100.0f,
        axe_z / 100.0f
    };

    // Remote player position in meters | Position du joueur distant en mètres
    Vector3 remotePos = { remoteData->x, remoteData->y, remoteData->z };

    // Distance calculation | Calcul de distance
    float dx = remotePos.x - localPos.x;
    float dy = remotePos.y - localPos.y;
    float dz = remotePos.z - localPos.z;
    float distanceInMeters = sqrtf(dx * dx + dy * dy + dz * dz);

    // TRUE stereo spatialization | Spatialisation stéréo vraie
    float toRemoteX = remotePos.x - localPos.x;
    float toRemoteY = remotePos.y - localPos.y;
    float toRemoteZ = remotePos.z - localPos.z;

    float toRemoteLen = sqrtf(toRemoteX * toRemoteX + toRemoteY * toRemoteY + toRemoteZ * toRemoteZ);
    if (toRemoteLen > 1e-6f) {
        toRemoteX /= toRemoteLen;
        toRemoteY /= toRemoteLen;
        toRemoteZ /= toRemoteLen;
    }

    float localDirX = avatarAxisX;
    float localDirY = avatarAxisY;
    float localDirZ = avatarAxisZ;

    float frontBack = localDirX * toRemoteX + localDirY * toRemoteY + localDirZ * toRemoteZ;

    // Invert calculation to fix left/right | Inverser le calcul pour corriger gauche/droite
    float leftRight = (localDirX * toRemoteZ - localDirZ * toRemoteX);

    // Gradual transition instead of abrupt on/off | Transition graduelle au lieu de on/off brutal
    float leftVolume = 1.0f;
    float rightVolume = 1.0f;

    // Progressive stereo panning settings | Réglages du panoramique stéréo progressif
    const float MIN_PAN_THRESHOLD = 0.05f;  // Central zone (equal sound) | Zone centrale (son égal)
    const float MAX_PAN_INTENSITY = 0.85f;  // Maximum pan intensity (85% instead of 70%) | Intensité maximale du panoramique (85% au lieu de 70%)
    const float PAN_CURVE = 1.5f;           // Transition curve (1.5 = soft, 2.0 = normal, 3.0 = aggressive) | Courbe de transition (1.5 = doux, 2.0 = normal, 3.0 = agressif)

    // Calculate panning with gradual transition | Calcul du panoramique avec transition graduelle
    if (fabsf(leftRight) > MIN_PAN_THRESHOLD) {
        float panAmount = fabsf(leftRight);
        panAmount = fminf(panAmount, 1.0f);

        float smoothPan = powf(panAmount, 1.0f / PAN_CURVE);
        float attenuation = smoothPan * MAX_PAN_INTENSITY;

        // CORRECTION: Inverser l'application des volumes (pas le calcul)
        if (leftRight > MIN_PAN_THRESHOLD) {
            // AVANT: rightVolume plein, leftVolume réduit
            // APRÈS: leftVolume plein, rightVolume réduit
            leftVolume = 1.0f;
            rightVolume = 1.0f - attenuation;
            if (rightVolume < 0.15f) rightVolume = 0.15f;
        }
        else if (leftRight < -MIN_PAN_THRESHOLD) {
            // AVANT: leftVolume plein, rightVolume réduit
            // APRÈS: rightVolume plein, leftVolume réduit
            rightVolume = 1.0f;
            leftVolume = 1.0f - attenuation;
            if (leftVolume < 0.15f) leftVolume = 0.15f;
        }
    }

    // Rear effect - Gradual transition also | Effet arrière - Transition graduelle aussi
    if (frontBack < -0.2f) {
        float backFactor = fabsf(frontBack + 0.2f) / 0.8f; // Normalize 0.0 to 1.0 | Normaliser 0.0 à 1.0
        backFactor = fminf(backFactor, 1.0f);

        // Smooth transition for rear effect | Transition douce pour l'effet arrière
        float backAttenuation = 0.85f - (backFactor * 0.35f); // From 0.85 to 0.50 | De 0.85 à 0.50
        leftVolume *= backAttenuation;
        rightVolume *= backAttenuation;
    }

    // Main volume calculation by distance | Calcul du volume principal selon la distance
    float voiceVolume = calculateVolumeMultiplierWithHubSettings(distanceInMeters, remoteData->voiceDistance);
    float finalLeftVolume = leftVolume * voiceVolume;
    float finalRightVolume = rightVolume * voiceVolume;
    float avgVolume = (finalLeftVolume + finalRightVolume) * 0.5f;

    // Apply TRUE stereo volumes | Appliquer les VRAIS volumes stéréo
    setUserAdaptiveVolumeWithSpatial(userID, avgVolume, finalLeftVolume, finalRightVolume);

    // Debug logging | Log pour debug
    if (enableLogGeneral) {
        static ULONGLONG lastResultLog = 0;
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastResultLog > 3000) {
            char resultMsg[256];
            snprintf(resultMsg, sizeof(resultMsg),
                "🔊 FIXED STEREO: Player='%s' Dist=%.1fm Pan=%.2f L=%.0f%% R=%.0f%% (CORRECTED+SMOOTH)",
                remoteData->playerName, distanceInMeters, leftRight,
                finalLeftVolume * 100.0f, finalRightVolume * 100.0f);
            mumbleAPI.log(ownID, resultMsg);
            lastResultLog = currentTime;
        }
    }
}

// Process received voice data with maximum reactivity | Traitement des données de voix reçues avec réactivité maximale
static void processReceivedVoiceData(const CompletePositionalData* receivedData, mumble_userid_t senderID) {
    processAdaptiveVolumeData(receivedData, senderID);
}

// ============================================================================
// MODULE 13 : OVERLAY VOCAL
// ============================================================================

static void setOverlayHighlightState(mumble_userid_t userID, mumble_connection_t connection, BOOL highlight) {
    // Ne pas modifier de texte, ne rien envoyer au serveur — uniquement UI locale
    if (highlight) {
        overlayBorderHighlight = TRUE;
        overlayHighlightUserID = userID;
    }
    else {
        if (overlayBorderHighlight && overlayHighlightUserID == userID) {
            overlayBorderHighlight = FALSE;
            overlayHighlightUserID = 0;
        }
    }

    if (hVoiceOverlay && IsWindow(hVoiceOverlay)) {
        InvalidateRect(hVoiceOverlay, NULL, TRUE);
        UpdateWindow(hVoiceOverlay);
    }
}

// Get current voice mode text | Obtenir le texte du mode de voix actuel
static const char* getCurrentVoiceModeText() {
    if (localVoiceData.voiceDistance <= distanceWhisper + 0.5f) {
        return "WHISPER";
    }
    else if (localVoiceData.voiceDistance <= distanceNormal + 0.5f) {
        return "NORMAL";
    }
    else {
        return "SHOUT";
    }
}

// Voice overlay window procedure | Procédure de fenêtre pour l'overlay vocal
static LRESULT CALLBACK VoiceOverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        InitializeCriticalSection(&overlayTextLock);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Fond noir transparent
        HBRUSH hClearBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rect, hClearBrush);
        DeleteObject(hClearBrush);

        // Bordure subtile interne
        HPEN hSubtlePen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hSubtlePen);
        HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hNullBrush);
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hSubtlePen);

        // Si surbrillance active, dessiner une bordure blanche épaisse autour
        if (overlayBorderHighlight) {
            HPEN hWhitePen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
            HPEN hPrev = (HPEN)SelectObject(hdc, hWhitePen);
            Rectangle(hdc, rect.left + 1, rect.top + 1, rect.right - 1, rect.bottom - 1);
            SelectObject(hdc, hPrev);
            DeleteObject(hWhitePen);
        }

        // Texte: si overlaySpeakerText non vide, l'afficher ; sinon afficher le mode local (comportement existant)
        char textBuffer[128] = { 0 };
        EnterCriticalSection(&overlayTextLock);
        if (overlaySpeakerText[0] != '\0') {
            strncpy_s(textBuffer, sizeof(textBuffer), overlaySpeakerText, _TRUNCATE);
        }
        LeaveCriticalSection(&overlayTextLock);

        const char* modeText = textBuffer;
        if (modeText[0] == '\0') {
            modeText = getCurrentVoiceModeText();
        }

        // Texte et ombre
        SetBkMode(hdc, TRANSPARENT);
        HFONT hSafeFont = CreateFontA(
            17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial"
        );
        HFONT hOldFont = (HFONT)SelectObject(hdc, hSafeFont);

        SIZE textSize;
        GetTextExtentPoint32A(hdc, modeText, (int)strlen(modeText), &textSize);
        int textX = (rect.right - rect.left - textSize.cx) / 2;
        int textY = (rect.bottom - rect.top - textSize.cy) / 2;
        if (textX < 0) textX = 2;
        if (textY < 0) textY = 2;

        SetTextColor(hdc, RGB(0, 0, 0));
        TextOutA(hdc, textX + 1, textY + 1, modeText, (int)strlen(modeText));
        SetTextColor(hdc, RGB(140, 140, 140));
        TextOutA(hdc, textX, textY, modeText, (int)strlen(modeText));

        SelectObject(hdc, hOldFont);
        DeleteObject(hSafeFont);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        DeleteCriticalSection(&overlayTextLock);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// Create voice overlay | Créer l'overlay vocal
static void createVoiceOverlay() {
    if (hVoiceOverlay != NULL) return;
    if (!enableVoiceOverlay) return;

    // Register window class | Enregistrer la classe de fenêtre
    const wchar_t OVERLAY_CLASS_NAME[] = L"VoiceOverlayClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = VoiceOverlayProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = OVERLAY_CLASS_NAME;
    wc.hbrBackground = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    UnregisterClassW(OVERLAY_CLASS_NAME, wc.hInstance);
    if (RegisterClassW(&wc) == 0) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "ERROR: Failed to register overlay window class");
        }
        return;
    }

    // Get screen dimensions | Obtenir les dimensions de l'écran
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Overlay dimensions | Dimensions de l'overlay
    int overlayWidth = 130;
    int overlayHeight = 32;

    // Position at bottom right | Position en bas à droite
    int posX = screenWidth - overlayWidth - 25;
    int posY = screenHeight - (screenHeight * 2 / 100) - overlayHeight;

    // Create with extended styles for fullscreen compatibility | Créer avec styles étendus pour compatibilité plein écran
    hVoiceOverlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        OVERLAY_CLASS_NAME,
        L"",
        WS_POPUP,
        posX, posY, overlayWidth, overlayHeight,
        NULL, NULL, wc.hInstance, NULL
    );

    if (hVoiceOverlay) {
        // Set transparency | Définir la transparence
        SetLayeredWindowAttributes(hVoiceOverlay, RGB(0, 0, 0), 100, LWA_ALPHA);

        // Force maximum z-order for fullscreen compatibility | Forcer l'ordre z maximum pour compatibilité plein écran
        SetWindowPos(hVoiceOverlay, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

        // Additional fullscreen compatibility measures | Mesures supplémentaires pour compatibilité plein écran
        LONG_PTR exStyle = GetWindowLongPtrW(hVoiceOverlay, GWL_EXSTYLE);
        exStyle |= WS_EX_TOPMOST;
        SetWindowLongPtrW(hVoiceOverlay, GWL_EXSTYLE, exStyle);

        // Show window | Afficher la fenêtre
        ShowWindow(hVoiceOverlay, SW_SHOWNOACTIVATE);
        UpdateWindow(hVoiceOverlay);

        // Force immediate redraw | Forcer le redessin immédiat
        InvalidateRect(hVoiceOverlay, NULL, TRUE);

        if (enableLogGeneral) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Voice overlay created with FULLSCREEN COMPATIBILITY at (%d, %d)",
                posX, posY);
            mumbleAPI.log(ownID, msg);
        }
    }
    else {
        DWORD error = GetLastError();
        if (enableLogGeneral) {
            char errorMsg[128];
            snprintf(errorMsg, sizeof(errorMsg), "ERROR: Failed to create overlay window. Error: %lu", error);
            mumbleAPI.log(ownID, errorMsg);
        }
    }
}

// Add fullscreen overlay refresh function | Ajouter fonction de rafraîchissement pour plein écran
static void refreshOverlayForFullscreen() {
    if (!hVoiceOverlay || !IsWindow(hVoiceOverlay)) return;

    // Force overlay to stay on top of fullscreen applications | Forcer l'overlay à rester au-dessus des applications plein écran
    SetWindowPos(hVoiceOverlay, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

    // Refresh window properties | Rafraîchir les propriétés de la fenêtre
    LONG_PTR exStyle = GetWindowLongPtrW(hVoiceOverlay, GWL_EXSTYLE);
    exStyle |= WS_EX_TOPMOST;
    SetWindowLongPtrW(hVoiceOverlay, GWL_EXSTYLE, exStyle);

    ShowWindow(hVoiceOverlay, SW_SHOWNOACTIVATE);
    InvalidateRect(hVoiceOverlay, NULL, TRUE);
    UpdateWindow(hVoiceOverlay);
}

// Reposition voice overlay | Repositionner l'overlay vocal
static void repositionVoiceOverlay() {
    if (!hVoiceOverlay) return;

    // Get new screen dimensions | Obtenir les nouvelles dimensions d'écran
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Overlay dimensions | Dimensions de l'overlay
    int overlayWidth = 120;
    int overlayHeight = 35;

    // New lower position | Nouvelle position plus basse
    int posX = screenWidth - overlayWidth - 20;
    int posY = screenHeight - (screenHeight * 2 / 100) - overlayHeight;

    // Reposition | Repositionner
    SetWindowPos(hVoiceOverlay, HWND_TOPMOST, posX, posY, 0, 0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

    if (enableLogGeneral) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Voice overlay repositioned to LOWER position (%d, %d) for screen %dx%d",
            posX, posY, screenWidth, screenHeight);
        mumbleAPI.log(ownID, msg);
    }
}

// Update voice overlay display | Mettre à jour l'affichage de l'overlay
static void updateVoiceOverlay() {
    if (hVoiceOverlay && enableVoiceOverlay && IsWindow(hVoiceOverlay)) {
        // Force complete window redraw | Forcer le redessin complet de la fenêtre
        InvalidateRect(hVoiceOverlay, NULL, TRUE);
        UpdateWindow(hVoiceOverlay);

        // Ensure overlay stays on top | S'assurer que l'overlay reste au premier plan
        SetWindowPos(hVoiceOverlay, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

// Destroy voice overlay | Détruire l'overlay vocal
static void destroyVoiceOverlay() {
    if (hVoiceOverlay) {
        DestroyWindow(hVoiceOverlay);
        hVoiceOverlay = NULL;

        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Voice overlay destroyed");
        }
    }

    if (hOverlayFont) {
        DeleteObject(hOverlayFont);
        hOverlayFont = NULL;
    }
}

// Resolution monitor thread | Thread pour surveiller les changements de résolution
static void overlayMonitorThread(void* arg) {
    overlayThreadRunning = TRUE;

    int lastScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int lastScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Voice overlay monitor thread started with fullscreen support");
    }

    while (overlayThreadRunning && enableGetPlayerCoordinates) {
        int currentScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        int currentScreenHeight = GetSystemMetrics(SM_CYSCREEN);

        if (currentScreenWidth != lastScreenWidth || currentScreenHeight != lastScreenHeight) {
            if (enableLogGeneral) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Screen resolution changed: %dx%d -> %dx%d",
                    lastScreenWidth, lastScreenHeight, currentScreenWidth, currentScreenHeight);
                mumbleAPI.log(ownID, msg);
            }

            repositionVoiceOverlay();
            lastScreenWidth = currentScreenWidth;
            lastScreenHeight = currentScreenHeight;
        }

        static int refreshCounter = 0;
        refreshCounter++;
        if (refreshCounter >= 5) {
            refreshOverlayForFullscreen();
            refreshCounter = 0;
        }

        Sleep(2000);
    }

    overlayThreadRunning = FALSE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Voice overlay monitor thread stopped");
    }
}

// ============================================================================
// MODULE 14 : INTERFACE UTILISATEUR
// ============================================================================

// Force window to foreground without affecting mouse | Forcer la fenêtre au premier plan sans affecter la souris
static void forceWindowToForegroundNoMouse(HWND hwnd) {
    ShowWindow(hwnd, SW_SHOW);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);

    // Force activation without mouse manipulation | Forcer l'activation sans manipulation de la souris
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);

    if (currentThreadId != foregroundThreadId) {
        AttachThreadInput(currentThreadId, foregroundThreadId, TRUE);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        AttachThreadInput(currentThreadId, foregroundThreadId, FALSE);
    }
    else {
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
    }

    BringWindowToTop(hwnd);
}

// Show/hide controls based on category | Afficher/masquer les contrôles selon la catégorie
static void ShowCategoryControls(int category) {
    currentCategory = category;

    // ✅ DÉSACTIVER LE REDESSIN PENDANT LA TRANSITION
    SendMessage(hConfigDialog, WM_SETREDRAW, FALSE, 0);

    // Get all control handles | Récupérer tous les handles de contrôles
    HWND hExplanation1 = GetDlgItem(hConfigDialog, 401);
    HWND hExplanation2 = GetDlgItem(hConfigDialog, 402);
    HWND hExplanation3 = GetDlgItem(hConfigDialog, 403);
    HWND hPathLabel = GetDlgItem(hConfigDialog, 404);

    HWND hPluginLabel = GetDlgItem(hConfigDialog, 501);
    HWND hKeyLabel = GetDlgItem(hConfigDialog, 502);
    HWND hWhisperLabel = GetDlgItem(hConfigDialog, 503);
    HWND hNormalLabel = GetDlgItem(hConfigDialog, 504);
    HWND hShoutLabel = GetDlgItem(hConfigDialog, 505);
    HWND hConfigLabel = GetDlgItem(hConfigDialog, 506);
    HWND hConfigExplain = GetDlgItem(hConfigDialog, 507);
    HWND hDistanceLabel = GetDlgItem(hConfigDialog, 508);
    HWND hDistanceWhisperLabel = GetDlgItem(hConfigDialog, 509);
    HWND hDistanceNormalLabel = GetDlgItem(hConfigDialog, 510);
    HWND hDistanceShoutLabel = GetDlgItem(hConfigDialog, 511);
    HWND hToggleLabel = GetDlgItem(hConfigDialog, 514);
    HWND hDistanceMutingLabel = GetDlgItem(hConfigDialog, 2001);
    HWND hChannelSwitchingLabel = GetDlgItem(hConfigDialog, 2002);
    HWND hVoiceToggleLabel = GetDlgItem(hConfigDialog, 2003);

    // Preset category controls | Contrôles de la catégorie presets
    HWND hPresetTitle = GetDlgItem(hConfigDialog, 800);
    HWND hPresetInstructions = GetDlgItem(hConfigDialog, 801);

    // ✅ RÉCUPÉRER TOUS LES BOUTONS UNE SEULE FOIS
    HWND hSaveConfigButton = GetDlgItem(hConfigDialog, 1);       // Save Configuration (Patch + Advanced)
    HWND hSaveVoiceRangeButton = GetDlgItem(hConfigDialog, 11); // Save Voice Range (Advanced uniquement)
    HWND hCancelButton = GetDlgItem(hConfigDialog, 2);          // Cancel (jamais affiché)

    if (category == 1) { // ========== PATCH CONFIGURATION ==========
        // Afficher catégorie 1
        if (hSavedPathBg) ShowWindow(hSavedPathBg, SW_SHOW);
        if (hSavedPathEdit) ShowWindow(hSavedPathEdit, SW_SHOW);
        if (hSavedPathButton) ShowWindow(hSavedPathButton, SW_SHOW);

        // Masquer catégories 2 et 3 (batch)
        HWND hideControls[] = {
            hPluginLabel, hKeyLabel, hWhisperLabel, hNormalLabel, hShoutLabel,
            hConfigLabel, hConfigExplain, hDistanceLabel, hDistanceWhisperLabel,
            hDistanceNormalLabel, hDistanceShoutLabel, hToggleLabel,
            hWhisperKeyEdit, hWhisperButton, hNormalKeyEdit, hNormalButton,
            hShoutKeyEdit, hShoutButton, hConfigKeyEdit, hConfigButton,
            hEnableDistanceMutingCheck, hEnableAutomaticChannelChangeCheck,
            hEnableVoiceToggleCheck, hVoiceToggleKeyEdit, hVoiceToggleButton,
            hDistanceWhisperEdit, hDistanceNormalEdit, hDistanceShoutEdit,
            hDistanceMutingLabel, hChannelSwitchingLabel, hVoiceToggleLabel,
            hDistanceWhisperMessage, hDistanceNormalMessage, hDistanceShoutMessage,
            hDistanceMutingMessage, hChannelSwitchingMessage, hPositionalAudioMessage,
            hPresetTitle, hPresetInstructions
        };

        for (int i = 0; i < sizeof(hideControls) / sizeof(HWND); i++) {
            if (hideControls[i]) ShowWindow(hideControls[i], SW_HIDE);
        }

        // Masquer presets
        for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
            if (hPresetLabels[i]) ShowWindow(hPresetLabels[i], SW_HIDE);
            if (hPresetLoadButtons[i]) ShowWindow(hPresetLoadButtons[i], SW_HIDE);
            if (hPresetRenameButtons[i]) ShowWindow(hPresetRenameButtons[i], SW_HIDE);
        }

        // Boutons
        if (hSaveConfigButton) {
            ShowWindow(hSaveConfigButton, SW_SHOW);
            // S'assurer que le bouton est au-dessus après redraw
            SetWindowPos(hSaveConfigButton, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        if (hSaveVoiceRangeButton) {
            ShowWindow(hSaveVoiceRangeButton, SW_HIDE);
        }
        if (hCancelButton) ShowWindow(hCancelButton, SW_HIDE);
    }
    else if (category == 2) { // ========== ADVANCED OPTIONS ==========
        if (hSavedPathBg) ShowWindow(hSavedPathBg, SW_HIDE);
        // Hide patch controls | Masquer contrôles patch
        if (hExplanation1) ShowWindow(hExplanation1, SW_HIDE);
        if (hExplanation2) ShowWindow(hExplanation2, SW_HIDE);
        if (hExplanation3) ShowWindow(hExplanation3, SW_HIDE);
        if (hPathLabel) ShowWindow(hPathLabel, SW_HIDE);
        if (hSavedPathEdit) ShowWindow(hSavedPathEdit, SW_HIDE);
        if (hSavedPathButton) ShowWindow(hSavedPathButton, SW_HIDE);

        // Show advanced options | Afficher options avancées
        if (hPluginLabel) ShowWindow(hPluginLabel, SW_SHOW);
        if (hKeyLabel) ShowWindow(hKeyLabel, SW_SHOW);
        if (hWhisperLabel) ShowWindow(hWhisperLabel, SW_SHOW);
        if (hNormalLabel) ShowWindow(hNormalLabel, SW_SHOW);
        if (hShoutLabel) ShowWindow(hShoutLabel, SW_SHOW);
        if (hConfigLabel) ShowWindow(hConfigLabel, SW_SHOW);
        if (hConfigExplain) ShowWindow(hConfigExplain, SW_SHOW);
        if (hDistanceLabel) ShowWindow(hDistanceLabel, SW_SHOW);
        if (hDistanceWhisperLabel) ShowWindow(hDistanceWhisperLabel, SW_SHOW);
        if (hDistanceNormalLabel) ShowWindow(hDistanceNormalLabel, SW_SHOW);
        if (hDistanceShoutLabel) ShowWindow(hDistanceShoutLabel, SW_SHOW);
        if (hWhisperKeyEdit) ShowWindow(hWhisperKeyEdit, SW_SHOW);
        if (hWhisperButton) ShowWindow(hWhisperButton, SW_SHOW);
        if (hNormalKeyEdit) ShowWindow(hNormalKeyEdit, SW_SHOW);
        if (hNormalButton) ShowWindow(hNormalButton, SW_SHOW);
        if (hShoutKeyEdit) ShowWindow(hShoutKeyEdit, SW_SHOW);
        if (hShoutButton) ShowWindow(hShoutButton, SW_SHOW);
        if (hConfigKeyEdit) ShowWindow(hConfigKeyEdit, SW_SHOW);
        if (hConfigButton) ShowWindow(hConfigButton, SW_SHOW);
        if (hEnableDistanceMutingCheck) ShowWindow(hEnableDistanceMutingCheck, SW_SHOW);
        if (hEnableAutomaticChannelChangeCheck) ShowWindow(hEnableAutomaticChannelChangeCheck, SW_SHOW);
        if (hDistanceWhisperEdit) ShowWindow(hDistanceWhisperEdit, SW_SHOW);
        if (hDistanceNormalEdit) ShowWindow(hDistanceNormalEdit, SW_SHOW);
        if (hDistanceShoutEdit) ShowWindow(hDistanceShoutEdit, SW_SHOW);
        if (hEnableVoiceToggleCheck) ShowWindow(hEnableVoiceToggleCheck, SW_SHOW);
        if (hVoiceToggleKeyEdit) ShowWindow(hVoiceToggleKeyEdit, SW_SHOW);
        if (hVoiceToggleButton) ShowWindow(hVoiceToggleButton, SW_SHOW);
        if (hToggleLabel) ShowWindow(hToggleLabel, SW_SHOW);
        if (hDistanceMutingLabel) ShowWindow(hDistanceMutingLabel, SW_SHOW);
        if (hChannelSwitchingLabel) ShowWindow(hChannelSwitchingLabel, SW_SHOW);
        if (hVoiceToggleLabel) ShowWindow(hVoiceToggleLabel, SW_SHOW);

        // Show messages | Afficher messages
        if (hDistanceWhisperMessage) ShowWindow(hDistanceWhisperMessage, SW_SHOW);
        if (hDistanceNormalMessage) ShowWindow(hDistanceNormalMessage, SW_SHOW);
        if (hDistanceShoutMessage) ShowWindow(hDistanceShoutMessage, SW_SHOW);
        if (hDistanceMutingMessage) ShowWindow(hDistanceMutingMessage, SW_SHOW);
        if (hChannelSwitchingMessage) ShowWindow(hChannelSwitchingMessage, SW_SHOW);
        if (hPositionalAudioMessage) ShowWindow(hPositionalAudioMessage, SW_SHOW);

        updateDynamicInterface();

        // Hide preset controls | Masquer contrôles presets
        if (hPresetTitle) ShowWindow(hPresetTitle, SW_HIDE);
        if (hPresetInstructions) ShowWindow(hPresetInstructions, SW_HIDE);
        for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
            if (hPresetLabels[i]) ShowWindow(hPresetLabels[i], SW_HIDE);
            if (hPresetLoadButtons[i]) ShowWindow(hPresetLoadButtons[i], SW_HIDE);
            if (hPresetRenameButtons[i]) ShowWindow(hPresetRenameButtons[i], SW_HIDE);
        }

        // ✅ BOUTONS POUR CATÉGORIE 2 : Save Voice Range + Save Configuration (PAS Cancel)
        if (hSaveConfigButton) {
            ShowWindow(hSaveConfigButton, SW_SHOW);
            SetWindowPos(hSaveConfigButton, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        if (hSaveVoiceRangeButton) {
            ShowWindow(hSaveVoiceRangeButton, SW_SHOW);
            // Forcer Z-order au-dessus (résout "partiellement visible / coupé")
            SetWindowPos(hSaveVoiceRangeButton, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        if (hCancelButton) ShowWindow(hCancelButton, SW_SHOW);
    }
    else if (category == 3) { // ========== VOICE RANGE PRESETS ==========
        // Hide patch controls | Masquer contrôles patch
        if (hSavedPathBg) ShowWindow(hSavedPathBg, SW_HIDE);
        if (hExplanation1) ShowWindow(hExplanation1, SW_HIDE);
        if (hExplanation2) ShowWindow(hExplanation2, SW_HIDE);
        if (hExplanation3) ShowWindow(hExplanation3, SW_HIDE);
        if (hPathLabel) ShowWindow(hPathLabel, SW_HIDE);
        if (hSavedPathEdit) ShowWindow(hSavedPathEdit, SW_HIDE);
        if (hSavedPathButton) ShowWindow(hSavedPathButton, SW_HIDE);
        if (hDistanceMutingLabel) ShowWindow(hDistanceMutingLabel, SW_HIDE);
        if (hChannelSwitchingLabel) ShowWindow(hChannelSwitchingLabel, SW_HIDE);
        if (hVoiceToggleLabel) ShowWindow(hVoiceToggleLabel, SW_HIDE);

        // Hide advanced options | Masquer options avancées
        if (hPluginLabel) ShowWindow(hPluginLabel, SW_HIDE);
        if (hKeyLabel) ShowWindow(hKeyLabel, SW_HIDE);
        if (hWhisperLabel) ShowWindow(hWhisperLabel, SW_HIDE);
        if (hNormalLabel) ShowWindow(hNormalLabel, SW_HIDE);
        if (hShoutLabel) ShowWindow(hShoutLabel, SW_HIDE);
        if (hConfigLabel) ShowWindow(hConfigLabel, SW_HIDE);
        if (hConfigExplain) ShowWindow(hConfigExplain, SW_HIDE);
        if (hDistanceLabel) ShowWindow(hDistanceLabel, SW_HIDE);
        if (hDistanceWhisperLabel) ShowWindow(hDistanceWhisperLabel, SW_HIDE);
        if (hDistanceNormalLabel) ShowWindow(hDistanceNormalLabel, SW_HIDE);
        if (hDistanceShoutLabel) ShowWindow(hDistanceShoutLabel, SW_HIDE);
        if (hWhisperKeyEdit) ShowWindow(hWhisperKeyEdit, SW_HIDE);
        if (hWhisperButton) ShowWindow(hWhisperButton, SW_HIDE);
        if (hNormalKeyEdit) ShowWindow(hNormalKeyEdit, SW_HIDE);
        if (hNormalButton) ShowWindow(hNormalButton, SW_HIDE);
        if (hShoutKeyEdit) ShowWindow(hShoutKeyEdit, SW_HIDE);
        if (hShoutButton) ShowWindow(hShoutButton, SW_HIDE);
        if (hConfigKeyEdit) ShowWindow(hConfigKeyEdit, SW_HIDE);
        if (hConfigButton) ShowWindow(hConfigButton, SW_HIDE);
        if (hEnableDistanceMutingCheck) ShowWindow(hEnableDistanceMutingCheck, SW_HIDE);
        if (hEnableAutomaticChannelChangeCheck) ShowWindow(hEnableAutomaticChannelChangeCheck, SW_HIDE);
        if (hDistanceWhisperEdit) ShowWindow(hDistanceWhisperEdit, SW_HIDE);
        if (hDistanceNormalEdit) ShowWindow(hDistanceNormalEdit, SW_HIDE);
        if (hDistanceShoutEdit) ShowWindow(hDistanceShoutEdit, SW_HIDE);
        if (hEnableVoiceToggleCheck) ShowWindow(hEnableVoiceToggleCheck, SW_HIDE);
        if (hVoiceToggleKeyEdit) ShowWindow(hVoiceToggleKeyEdit, SW_HIDE);
        if (hVoiceToggleButton) ShowWindow(hVoiceToggleButton, SW_HIDE);
        if (hToggleLabel) ShowWindow(hToggleLabel, SW_HIDE);

        // Hide messages | Masquer messages
        if (hDistanceWhisperMessage) {
            SetWindowTextW(hDistanceWhisperMessage, L"");
            ShowWindow(hDistanceWhisperMessage, SW_HIDE);
        }
        if (hDistanceNormalMessage) {
            SetWindowTextW(hDistanceNormalMessage, L"");
            ShowWindow(hDistanceNormalMessage, SW_HIDE);
        }
        if (hDistanceShoutMessage) {
            SetWindowTextW(hDistanceShoutMessage, L"");
            ShowWindow(hDistanceShoutMessage, SW_HIDE);
        }
        if (hDistanceMutingMessage) {
            SetWindowTextW(hDistanceMutingMessage, L"");
            ShowWindow(hDistanceMutingMessage, SW_HIDE);
        }
        if (hChannelSwitchingMessage) {
            SetWindowTextW(hChannelSwitchingMessage, L"");
            ShowWindow(hChannelSwitchingMessage, SW_HIDE);
        }
        if (hPositionalAudioMessage) {
            SetWindowTextW(hPositionalAudioMessage, L"");
            ShowWindow(hPositionalAudioMessage, SW_HIDE);
        }

        // Show preset controls | Afficher contrôles presets
        if (hPresetTitle) ShowWindow(hPresetTitle, SW_SHOW);
        if (hPresetInstructions) ShowWindow(hPresetInstructions, SW_SHOW);
        for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
            if (hPresetLabels[i]) ShowWindow(hPresetLabels[i], SW_SHOW);
            if (hPresetLoadButtons[i]) ShowWindow(hPresetLoadButtons[i], SW_SHOW);
            if (hPresetRenameButtons[i]) ShowWindow(hPresetRenameButtons[i], SW_SHOW);
        }

        // ✅ BOUTONS POUR CATÉGORIE 3 : TOUT MASQUÉ (pas de boutons en bas)
        if (hSaveConfigButton) ShowWindow(hSaveConfigButton, SW_HIDE);
        if (hSaveVoiceRangeButton) ShowWindow(hSaveVoiceRangeButton, SW_HIDE);
        if (hCancelButton) ShowWindow(hCancelButton, SW_HIDE);

        updatePresetLabels();
    }

    SendMessage(hConfigDialog, WM_SETREDRAW, TRUE, 0);

    // Remplacer InvalidateRect + UpdateWindow par un RedrawWindow complet
    // RDW_ERASE forcera l'appel de WM_ERASEBKGND (qui dessine l'image seulement si currentCategory==1)
    // RDW_ALLCHILDREN assure que les enfants sont rafraîchis proprement — évite les "carrés blancs" résiduels.
    RedrawWindow(hConfigDialog, NULL, NULL,
        RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    // Mettre à jour l'état des boutons de catégorie
    if (hCategoryPatch && hCategoryAdvanced && hCategoryPresets) {
        SendMessage(hCategoryPatch, BM_SETSTATE, (category == 1) ? TRUE : FALSE, 0);
        SendMessage(hCategoryAdvanced, BM_SETSTATE, (category == 2) ? TRUE : FALSE, 0);
        SendMessage(hCategoryPresets, BM_SETSTATE, (category == 3) ? TRUE : FALSE, 0);
    }
}

// Apply font to control | Appliquer la police à un contrôle
static void ApplyFontToControl(HWND control, HFONT font) {
    if (font && control) {
        SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
    }
}

// Modern folder browser | Fonction pour parcourir les dossiers (moderne)
static void browseSavedPath(HWND hwnd) {
    // ✅ 1. Initialiser COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(hwnd, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // ✅ 2. Créer le dialogue de sélection de fichier
    IFileOpenDialog* pFileOpen = NULL;
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (void**)&pFileOpen);

    if (SUCCEEDED(hr)) {
        // ✅ 3. Configurer pour sélectionner des DOSSIERS (pas des fichiers)
        DWORD dwOptions;
        hr = pFileOpen->lpVtbl->GetOptions(pFileOpen, &dwOptions);
        if (SUCCEEDED(hr)) {
            hr = pFileOpen->lpVtbl->SetOptions(pFileOpen, dwOptions | FOS_PICKFOLDERS);
        }

        // ✅ 4. Définir le titre du dialogue
        if (SUCCEEDED(hr)) {
            hr = pFileOpen->lpVtbl->SetTitle(pFileOpen, L"Select your Conan Exiles game folder");
        }

        // ✅ 5. Afficher le dialogue
        if (SUCCEEDED(hr)) {
            hr = pFileOpen->lpVtbl->Show(pFileOpen, hwnd);

            // ✅ 6. Récupérer le chemin sélectionné SI l'utilisateur a cliqué OK
            if (SUCCEEDED(hr)) {
                IShellItem* pItem = NULL;
                hr = pFileOpen->lpVtbl->GetResult(pFileOpen, &pItem);

                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath = NULL;
                    hr = pItem->lpVtbl->GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                    // ✅ 7. Mettre à jour le contrôle ET forcer le redessin
                    if (SUCCEEDED(hr) && pszFilePath) {
                        // ✅ Mettre à jour la VARIABLE GLOBALE (pas un contrôle EDIT)
                        wcscpy_s(displayedPathText, MAX_PATH, pszFilePath);

                        // ✅ Forcer le redessin de l'image pour afficher le nouveau texte
                        if (hSavedPathBg && IsWindow(hSavedPathBg)) {
                            InvalidateRect(hSavedPathBg, NULL, TRUE);
                            UpdateWindow(hSavedPathBg);
                        }

                        CoTaskMemFree(pszFilePath);
                    }

                    pItem->lpVtbl->Release(pItem);
                }
            }
        }

        pFileOpen->lpVtbl->Release(pFileOpen);
    }

    // ✅ 8. Libérer COM
    CoUninitialize();
}

// Modern folder browser | Explorateur de dossier moderne
static void browseFolderModern(HWND hwnd) {
    IFileDialog* pfd = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void**)&pfd);
    if (SUCCEEDED(hr)) {
        DWORD options;
        pfd->lpVtbl->GetOptions(pfd, &options);
        pfd->lpVtbl->SetOptions(pfd, options | FOS_PICKFOLDERS);
        pfd->lpVtbl->SetTitle(pfd, L"Select the Conan Exiles folder");
        hr = pfd->lpVtbl->Show(pfd, hwnd);
        if (SUCCEEDED(hr)) {
            IShellItem* psi;
            hr = pfd->lpVtbl->GetResult(pfd, &psi);
            if (SUCCEEDED(hr)) {
                wchar_t* path = NULL;
                hr = psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH, &path);
                if (SUCCEEDED(hr) && path) {
                    SetWindowTextW(hSavedPathEdit, path);
                    CoTaskMemFree(path);
                }
                psi->lpVtbl->Release(psi);
            }
        }
        pfd->lpVtbl->Release(pfd);
    }
}

// Preset rename dialog procedure | Procédure de dialogue de renommage de preset
static LRESULT CALLBACK PresetRenameDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hPresetRenameDialog = hwnd;

        // Load rename dialog background from resources | Charger le fond du dialogue de renommage depuis les ressources
        HMODULE hModuleRename = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)PresetRenameDialogProc, &hModuleRename);

        hBackgroundRenamePresetBitmap = (HBITMAP)LoadImageW(
            hModuleRename,
            MAKEINTRESOURCEW(IDB_Background_Rename_Preset),
            IMAGE_BITMAP,
            0, 0,
            LR_CREATEDIBSECTION);

        if (!hBackgroundRenamePresetBitmap && enableLogGeneral) {
            mumbleAPI.log(ownID, "WARNING: Rename dialog background (IDB_Background_Rename_Preset) not loaded");
        }

        // Current name label | Label du nom actuel
        wchar_t currentText[128];
        swprintf(currentText, 128, L"%S", voicePresets[renamePresetIndex].name);
        HWND hCurrentLabel = CreateWindowW(L"STATIC", currentText,
            WS_VISIBLE | WS_CHILD | SS_LEFT | SS_OWNERDRAW,
            92, 37, 260, 25, hwnd, (HMENU)1500, NULL, NULL);
        ApplyFontToControl(hCurrentLabel, hFont);

        // Get dialog client area for centering | Obtenir la zone cliente du dialogue pour centrage
        RECT dlgRect;
        GetClientRect(hwnd, &dlgRect);
        int dlgWidth = dlgRect.right - dlgRect.left;
        int dlgHeight = dlgRect.bottom - dlgRect.top;

        // Input edit control dimensions | Dimensions du contrôle d'édition
        int inputWidth = 260;
        int inputHeight = 30;
        int inputX = (dlgWidth - inputWidth) / 2;
        int inputY = 90;

        HWND hInputEdit = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            inputX, inputY, inputWidth, inputHeight, hwnd, (HMENU)1001, NULL, NULL);

        // ✅ LIMITER À 10 CARACTÈRES MAXIMUM
        SendMessage(hInputEdit, EM_LIMITTEXT, 15, 0);

        SetWindowTextA(hInputEdit, voicePresets[renamePresetIndex].name);
        SetFocus(hInputEdit);
        SendMessage(hInputEdit, EM_SETSEL, 0, -1);

        // OK and Cancel buttons dimensions | Dimensions des boutons OK et Annuler
        int btnWidth = 80;
        int btnHeight = 35;
        int btnGap = 10;
        int totalBtnWidth = (btnWidth * 2) + btnGap;
        int btnStartX = (dlgWidth - totalBtnWidth) / 2;
        int btnY = inputY + inputHeight + 30;

        // Get real dimensions of IDB_OK_Box_01 | Récupérer les dimensions réelles de IDB_OK_Box_01
        HMODULE hModuleOkBtn = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)PresetRenameDialogProc, &hModuleOkBtn);

        if (!hModuleOkBtn) {
            // Fallback: Get module handle from DLL | Repli: Obtenir le handle du module depuis la DLL
            hModuleOkBtn = GetModuleHandleW(NULL);
        }

        HBITMAP hOkBoxTemp = (HBITMAP)LoadImageW(hModuleOkBtn, MAKEINTRESOURCEW(IDB_OK_Box_01),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        if (!hOkBoxTemp && enableLogGeneral) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "ERROR: Failed to load IDB_OK_Box_01 - HMODULE=0x%p", hModuleOkBtn);
            mumbleAPI.log(ownID, logMsg);
        }

        int okBoxWidth = 80;   // Valeur par défaut
        int okBoxHeight = 35;  // Valeur par défaut

        if (hOkBoxTemp) {
            BITMAP bmOk;
            GetObject(hOkBoxTemp, sizeof(BITMAP), &bmOk);
            okBoxWidth = bmOk.bmWidth;
            okBoxHeight = bmOk.bmHeight;
            DeleteObject(hOkBoxTemp);

            if (enableLogGeneral) {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg),
                    "IDB_OK_Box_01 size: %dx%d pixels", okBoxWidth, okBoxHeight);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        // Recalculate button positions with real image dimensions | Recalculer les positions avec les vraies dimensions
        int okBtnGap = 10;
        int totalOkBtnWidth = (okBoxWidth * 2) + okBtnGap;
        int okBtnStartX = (dlgWidth - totalOkBtnWidth) / 2;
        int okBtnY = inputY + inputHeight + 30;

        // OK button with real image size | Bouton OK à taille réelle
        HWND hOkButton = CreateWindowW(L"BUTTON", L"OK",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
            okBtnStartX, okBtnY, okBoxWidth, okBoxHeight, hwnd, (HMENU)1002, NULL, NULL);
        ApplyFontToControl(hOkButton, hFont);

        // Cancel button with real image size | Bouton Cancel à taille réelle
        HWND hCancelButton = CreateWindowW(L"BUTTON", L"Cancel",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            okBtnStartX + okBoxWidth + okBtnGap, okBtnY, okBoxWidth, okBoxHeight, hwnd, (HMENU)1003, NULL, NULL);
        ApplyFontToControl(hCancelButton, hFont);

        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rect;
        GetClientRect(hwnd, &rect);

        // Draw dialog background bitmap if available | Dessiner le bitmap de fond si disponible
        if (hBackgroundRenamePresetBitmap) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBackgroundRenamePresetBitmap);

            BITMAP bm;
            GetObject(hBackgroundRenamePresetBitmap, sizeof(BITMAP), &bm);

            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc,
                0, 0, rect.right - rect.left, rect.bottom - rect.top,
                hdcMem,
                0, 0, bm.bmWidth, bm.bmHeight,
                SRCCOPY);

            SelectObject(hdcMem, hOld);
            DeleteDC(hdcMem);
        }
        else {
            // Fallback: solid background | Repli : fond uni
            HBRUSH hBrush = CreateSolidBrush(RGB(248, 249, 250));
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);
        }

        return 1;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;

        // Draw OK and Cancel buttons with bitmap | Dessiner les boutons OK et Cancel avec bitmap
        if (lpDIS->CtlID == 1002 || lpDIS->CtlID == 1003) {
            HDC hdc = lpDIS->hDC;
            RECT rect = lpDIS->rcItem;

            // Load bitmap resource | Charger la ressource bitmap
            HMODULE hModule = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)PresetRenameDialogProc, &hModule);

            if (!hModule) hModule = GetModuleHandleW(NULL);

            HBITMAP hBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_OK_Box_01),
                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

            if (hBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

                BITMAP bm;
                GetObject(hBitmap, sizeof(BITMAP), &bm);

                // Draw bitmap at real size (no stretch) | Dessiner le bitmap à taille réelle (pas de stretch)
                BitBlt(hdc,
                    rect.left,
                    rect.top,
                    bm.bmWidth,
                    bm.bmHeight,
                    hdcMem,
                    0, 0,
                    SRCCOPY);

                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
                DeleteObject(hBitmap);

                // Draw text centered on image | Dessiner le texte centré sur l'image
                wchar_t text[32] = L"";
                GetWindowTextW(lpDIS->hwndItem, text, 32);

                HFONT hTextFont = hFont ? hFont : CreateFontW(
                    16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hTextFont);

                RECT textRect;
                textRect.left = rect.left;
                textRect.top = rect.top;
                textRect.right = rect.left + bm.bmWidth;
                textRect.bottom = rect.top + bm.bmHeight;

                SetBkMode(hdc, TRANSPARENT);

                // Shadow text | Texte ombre
                SetTextColor(hdc, RGB(0, 0, 0));
                RECT shadowRect = textRect;
                OffsetRect(&shadowRect, 1, 1);
                DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Main text in white | Texte principal en blanc
                SetTextColor(hdc, RGB(255, 255, 255));
                DrawTextW(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, hOldFont);
                if (!hFont) DeleteObject(hTextFont);
            }

            return TRUE;
        }

        // Draw current label with white text and shadow | Dessiner le label actuel avec texte blanc et ombre
        if (lpDIS->CtlID == 1500) {
            HDC hdc = lpDIS->hDC;
            RECT rect = lpDIS->rcItem;

            SetBkMode(hdc, TRANSPARENT);

            wchar_t text[128] = L"";
            GetWindowTextW(lpDIS->hwndItem, text, 128);

            HFONT hTextFont = hFont ? hFont : CreateFontW(
                16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hTextFont);

            // Shadow text | Texte ombre
            SetTextColor(hdc, RGB(0, 0, 0));
            RECT shadowRect = rect;
            OffsetRect(&shadowRect, 1, 1);
            DrawTextW(hdc, text, -1, &shadowRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // Main text in white | Texte principal en blanc
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, text, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
            if (!hFont) DeleteObject(hTextFont);

            return TRUE;
        }

        // Draw button bitmaps | Dessiner les bitmaps des boutons
        if (lpDIS->CtlType == ODT_BUTTON) {
            DrawButtonWithBitmap(lpDIS);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1002: { // OK
            HWND hEdit = GetDlgItem(hwnd, 1001);
            char newName[PRESET_NAME_MAX_LENGTH];
            GetWindowTextA(hEdit, newName, PRESET_NAME_MAX_LENGTH);

            if (strlen(newName) > 0) {
                renameVoicePreset(renamePresetIndex, newName);
            }

            DestroyWindow(hwnd);
            return 0;
        }
        case 1003: // Cancel
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(33, 37, 41));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }



    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetBkMode(hdcEdit, TRANSPARENT);
        SetTextColor(hdcEdit, RGB(33, 37, 41));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_DESTROY:
        hPresetRenameDialog = NULL;
        // Free background bitmap for rename dialog | Libérer le bitmap de fond du dialogue de renommage
        if (hBackgroundRenamePresetBitmap) {
            DeleteObject(hBackgroundRenamePresetBitmap);
            hBackgroundRenamePresetBitmap = NULL;
        }
        // Réinitialiser le flag pour permettre le redessin à la prochaine ouverture
        backgroundDrawn = FALSE;
        return 0;


    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Preset save dialog procedure | Procédure de dialogue de sauvegarde de preset
static LRESULT CALLBACK PresetSaveDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hPresetSaveDialog = hwnd;

        // Load save-presets dialog background from resources | Charger le fond du dialogue de sauvegarde depuis les ressources
        HMODULE hModule = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)PresetSaveDialogProc, &hModule);

        hBackgroundSavePresetBitmap = (HBITMAP)LoadImageW(
            hModule,
            MAKEINTRESOURCEW(IDB_Background_Save_Voice_Range_Presets),
            IMAGE_BITMAP,
            0, 0,
            LR_CREATEDIBSECTION);

        if (!hBackgroundSavePresetBitmap && enableLogGeneral) {
            mumbleAPI.log(ownID, "WARNING: Save presets dialog background (IDB_Background_Save_Voice_Range_Presets) not loaded");
        }

        // Create 10 preset buttons using preset box image size | Créer 10 boutons de preset en utilisant la taille de l'image de preset
        int yPos = 90;

        // Get HMODULE and real dimensions of IDB_Preset_Box_01 | Obtenir HMODULE et dimensions réelles de IDB_Preset_Box_01
        HMODULE hPresetModule = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)PresetSaveDialogProc, &hPresetModule);

        HBITMAP hPresetBoxTemp = (HBITMAP)LoadImageW(hPresetModule, MAKEINTRESOURCEW(IDB_Preset_Box_01),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        int presetBoxWidth = 400;  // fallback width | largeur de repli
        int presetBoxHeight = 35;  // fallback height | hauteur de repli

        if (hPresetBoxTemp) {
            BITMAP bmPreset;
            GetObject(hPresetBoxTemp, sizeof(BITMAP), &bmPreset);
            presetBoxWidth = bmPreset.bmWidth;
            presetBoxHeight = bmPreset.bmHeight;
            DeleteObject(hPresetBoxTemp);

            if (enableLogGeneral) {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg),
                    "IDB_Preset_Box_01 size: %dx%d pixels", presetBoxWidth, presetBoxHeight);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
            wchar_t buttonText[128];
            if (voicePresets[i].isUsed) {
                wchar_t wName[PRESET_NAME_MAX_LENGTH];
                size_t converted = 0;
                mbstowcs_s(&converted, wName, PRESET_NAME_MAX_LENGTH, voicePresets[i].name, _TRUNCATE);
                swprintf(buttonText, 128, L"[%d] %s (%.1f / %.1f / %.1f)",
                    i + 1, wName,
                    voicePresets[i].whisperDistance,
                    voicePresets[i].normalDistance,
                    voicePresets[i].shoutDistance);
            }
            else {
                swprintf(buttonText, 128, L"[%d] Empty Slot", i + 1);
            }

            // Calculate centered X for preset button | Calculer la position X centrée pour le bouton preset
            RECT dlgRect;
            GetClientRect(hwnd, &dlgRect);
            int clientWidth = dlgRect.right - dlgRect.left;
            int presetX = (clientWidth - presetBoxWidth) / 2;
            if (presetX < 10) presetX = 10; // keep small left margin if image wider than dialog | garder une petite marge si l'image est plus large que la fenêtre

            HWND hPresetButton = CreateWindowW(L"BUTTON", buttonText,
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
                presetX, yPos, presetBoxWidth, presetBoxHeight, hwnd, (HMENU)(700 + i), NULL, NULL);
            ApplyFontToControl(hPresetButton, hFont);

            // vertical spacing = image height + 5px gap | espacement vertical = hauteur image + 5px
            yPos += presetBoxHeight + 5;
        }

        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rect;
        GetClientRect(hwnd, &rect);

        // Draw dialog background bitmap if available | Dessiner le bitmap de fond du dialogue si disponible
        if (hBackgroundSavePresetBitmap) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBackgroundSavePresetBitmap);

            BITMAP bm;
            GetObject(hBackgroundSavePresetBitmap, sizeof(BITMAP), &bm);

            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc,
                0, 0, rect.right - rect.left, rect.bottom - rect.top,
                hdcMem,
                0, 0, bm.bmWidth, bm.bmHeight,
                SRCCOPY);

            SelectObject(hdcMem, hOld);
            DeleteDC(hdcMem);
        }
        else {
            // Fallback: solid background | Repli : fond uni
            HBRUSH hBrush = CreateSolidBrush(RGB(248, 249, 250));
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);
        }

        return 1;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;

        // Dessiner le bitmap pour TOUS les boutons
        if (lpDIS->CtlType == ODT_BUTTON) {
            DrawButtonWithBitmap(lpDIS);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        int buttonId = LOWORD(wParam);

        // Handle preset selection | Gérer la sélection de preset
        if (buttonId >= 700 && buttonId < 700 + MAX_VOICE_PRESETS) {
            int presetIndex = buttonId - 700;
            saveVoicePreset(presetIndex, NULL);
            DestroyWindow(hwnd);
            return 0;
        }
        // Handle cancel | Gérer l'annulation
        else if (buttonId == 999) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(33, 37, 41));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_DESTROY:
        hPresetSaveDialog = NULL;

        // Free background bitmap for save dialog | Libérer le bitmap de fond du dialogue de sauvegarde
        if (hBackgroundSavePresetBitmap) {
            DeleteObject(hBackgroundSavePresetBitmap);
            hBackgroundSavePresetBitmap = NULL;
        }

        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Show preset save dialog | Afficher le dialogue de sauvegarde de preset
static void showPresetSaveDialog(void) {
    if (hPresetSaveDialog && IsWindow(hPresetSaveDialog)) {
        SetForegroundWindow(hPresetSaveDialog);
        return;
    }

    const wchar_t PRESET_DIALOG_CLASS[] = L"PresetSaveDialogClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = PresetSaveDialogProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = PRESET_DIALOG_CLASS;
    wc.hbrBackground = CreateSolidBrush(RGB(248, 249, 250));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    UnregisterClassW(PRESET_DIALOG_CLASS, wc.hInstance);
    RegisterClassW(&wc);

    // ✅ CORRECTION : Centrer au-dessus de l'interface principale
    int dialogWidth = 440;
    int dialogHeight = 580;
    int dialogX, dialogY;

    if (hConfigDialog && IsWindow(hConfigDialog)) {
        // Get parent window position and size | Obtenir position et taille de la fenêtre parente
        RECT parentRect;
        GetWindowRect(hConfigDialog, &parentRect);

        int parentWidth = parentRect.right - parentRect.left;
        int parentHeight = parentRect.bottom - parentRect.top;
        int parentX = parentRect.left;
        int parentY = parentRect.top;

        dialogX = parentX + (parentWidth - dialogWidth) / 2;
        dialogY = parentY + (parentHeight - dialogHeight) / 2;

        if (enableLogGeneral) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "Save dialog: Parent at (%d,%d), Dialog at (%d,%d)",
                parentX, parentY, dialogX, dialogY);
            mumbleAPI.log(ownID, logMsg);
        }
    }
    else {
        // Fallback to screen center if parent not available | Centrer sur l'écran si parent indisponible
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        dialogX = (screenWidth - dialogWidth) / 2;
        dialogY = (screenHeight - dialogHeight) / 2;
    }

    hPresetSaveDialog = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        PRESET_DIALOG_CLASS,
        L"Save Voice Range Preset",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        dialogX, dialogY, dialogWidth, dialogHeight,
        hConfigDialog, NULL, wc.hInstance, NULL);

    if (hPresetSaveDialog) {
        SetLayeredWindowAttributes(hPresetSaveDialog, 0, 250, LWA_ALPHA);
        ShowWindow(hPresetSaveDialog, SW_SHOW);
        UpdateWindow(hPresetSaveDialog);
    }
}

// Update preset labels in category 3 | Mettre à jour les labels de preset dans la catégorie 3
static void updatePresetLabels(void) {
    if (!hConfigDialog || !IsWindow(hConfigDialog)) return;
    if (currentCategory != 3) return;

    for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
        if (hPresetLabels[i] && IsWindow(hPresetLabels[i])) {
            wchar_t labelText[256];
            wchar_t wName[PRESET_NAME_MAX_LENGTH];
            size_t converted = 0;
            mbstowcs_s(&converted, wName, PRESET_NAME_MAX_LENGTH, voicePresets[i].name, _TRUNCATE);

            if (voicePresets[i].isUsed) {
                // ✅ FORMAT SANS CROCHETS
                swprintf(labelText, 256, L"%d %s - W:%.1f N:%.1f S:%.1f m",
                    i + 1, wName,
                    voicePresets[i].whisperDistance,
                    voicePresets[i].normalDistance,
                    voicePresets[i].shoutDistance);
            }
            else {
                swprintf(labelText, 256, L"%d %s (Empty)", i + 1, wName);
            }

            SetWindowTextW(hPresetLabels[i], labelText);
        }

        if (hPresetLoadButtons[i] && IsWindow(hPresetLoadButtons[i])) {
            EnableWindow(hPresetLoadButtons[i], voicePresets[i].isUsed);
        }
    }
}

static LRESULT CALLBACK PresetLabelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Fond transparent
        SetBkMode(hdc, TRANSPARENT);

        // Récupérer le texte du label
        wchar_t text[256];
        GetWindowTextW(hwnd, text, 256);

        // Charger l'icône checkmark
        HMODULE hModule = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)PresetLabelProc, &hModule);

        HICON hCheckIcon = (HICON)LoadImageW(hModule, MAKEINTRESOURCEW(IDI_CHECKMARK),
            IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

        if (hCheckIcon) {
            // Dessiner l'icône à gauche
            int iconX = 2;
            int iconY = (rect.bottom - 16) / 2;
            DrawIconEx(hdc, iconX, iconY, hCheckIcon, 16, 16, 0, NULL, DI_NORMAL);
            DestroyIcon(hCheckIcon);
        }

        // Dessiner le texte décalé de 20px à droite
        RECT textRect = rect;
        textRect.left += 20;

        // Ombre noire | Black shadow
        SetTextColor(hdc, RGB(0, 0, 0));
        RECT shadowRect = textRect;
        OffsetRect(&shadowRect, 1, 1);
        DrawTextW(hdc, text, -1, &shadowRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Texte principal en blanc | Main text in white
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextW(hdc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, PresetLabelProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Create presets category controls | Créer les contrôles de la catégorie presets
static void createPresetsCategory(void) {
    // Create preset rows | Créer les lignes de preset
        // Get module handle for loading resources | Obtenir le handle du module pour charger les ressources
    HMODULE hModule = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)createPresetsCategory, &hModule);

    int presetYPositions[MAX_VOICE_PRESETS] = {
            215,    // Preset 1
            257,    // Preset 2
            298,    // Preset 3
            340,    // Preset 4
            380,    // Preset 5
            423,    // Preset 6
            465,    // Preset 7
            505,    // Preset 8
            547,    // Preset 9
            587     // Preset 10
    };

    // Manual Y positions for Load and Rename buttons | Positions Y manuelles pour les boutons Load et Rename
    int buttonYPositions[MAX_VOICE_PRESETS] = {
            217,    // Preset 1
            259,    // Preset 2
            300,    // Preset 3
            342,    // Preset 4
            382,    // Preset 5
            426,    // Preset 6
            467,    // Preset 7
            507,    // Preset 8
            549,    // Preset 9
            589     // Preset 10
    };

    for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
        int yPos = presetYPositions[i];
        int buttonY = buttonYPositions[i];

        // Preset name label | Label du nom du preset - SANS EMOJI
        wchar_t labelText[256];
        wchar_t wName[PRESET_NAME_MAX_LENGTH];
        size_t converted = 0;
        mbstowcs_s(&converted, wName, PRESET_NAME_MAX_LENGTH, voicePresets[i].name, _TRUNCATE);

        if (voicePresets[i].isUsed) {
            // ✅ FORMAT CORRIGÉ : Pas d'emoji microphone
            swprintf(labelText, 256, L"[%d] %s - W:%.1f N:%.1f S:%.1f m",
                i + 1, wName,
                voicePresets[i].whisperDistance,
                voicePresets[i].normalDistance,
                voicePresets[i].shoutDistance);
        }
        else {
            swprintf(labelText, 256, L"[%d] %s (Empty)", i + 1, wName);
        }

        hPresetLabels[i] = CreateWindowW(L"STATIC", labelText,
            WS_CHILD | SS_LEFT | SS_OWNERDRAW,
            40, yPos + 5, 300, 25, hConfigDialog, (HMENU)(850 + i), NULL, NULL);
        ApplyFontToControl(hPresetLabels[i], hFont);

        // ✅ Activer le custom draw
        SetWindowSubclass(hPresetLabels[i], PresetLabelProc, 850 + i, 0);

        // Get real dimensions of IDB_Load_Box_01 | Récupérer les dimensions réelles de IDB_Load_Box_01
        HBITMAP hLoadBoxTemp = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Load_Box_01),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        int loadBoxWidth = 80;   // Valeur par défaut
        int loadBoxHeight = 30;  // Valeur par défaut

        if (hLoadBoxTemp) {
            BITMAP bmLoad;
            GetObject(hLoadBoxTemp, sizeof(BITMAP), &bmLoad);
            loadBoxWidth = bmLoad.bmWidth;
            loadBoxHeight = bmLoad.bmHeight;
            DeleteObject(hLoadBoxTemp);

            if (enableLogGeneral) {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg),
                    "IDB_Load_Box_01 size: %dx%d pixels", loadBoxWidth, loadBoxHeight);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        // Load button with Load Box image size | Bouton charger avec la taille de l'image Load Boxf
        hPresetLoadButtons[i] = CreateWindowW(L"BUTTON", L"Load",
            WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            370, buttonY, loadBoxWidth, loadBoxHeight, hConfigDialog, (HMENU)(900 + i), NULL, NULL);
        ApplyFontToControl(hPresetLoadButtons[i], hFont);
        EnableWindow(hPresetLoadButtons[i], voicePresets[i].isUsed);

        // Get real dimensions of IDB_Rename_Box_01 | Récupérer les dimensions réelles de IDB_Rename_Box_01
        HBITMAP hRenameBoxTemp = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Rename_Box_01),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        int renameBoxWidth = 100;  // Valeur par défaut
        int renameBoxHeight = 30;  // Valeur par défaut

        if (hRenameBoxTemp) {
            BITMAP bmRename;
            GetObject(hRenameBoxTemp, sizeof(BITMAP), &bmRename);
            renameBoxWidth = bmRename.bmWidth;
            renameBoxHeight = bmRename.bmHeight;
            DeleteObject(hRenameBoxTemp);

            if (enableLogGeneral) {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg),
                    "IDB_Rename_Box_01 size: %dx%d pixels", renameBoxWidth, renameBoxHeight);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        // Rename button with Rename Box image size | Bouton renommer avec la taille de l'image Rename Box
        hPresetRenameButtons[i] = CreateWindowW(L"BUTTON", L"Rename",
            WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            465, buttonY, renameBoxWidth, renameBoxHeight, hConfigDialog, (HMENU)(950 + i), NULL, NULL);
    }
}

// Check if Saved folder exists in game folder | Vérifier que le dossier Saved existe dans le dossier du jeu
static int savedExistsInFolder(const wchar_t* folderPath) {
    wchar_t savedPath[MAX_PATH];
    swprintf(savedPath, MAX_PATH, L"%s\\ConanSandbox\\Saved", folderPath);
    DWORD attribs = GetFileAttributesW(savedPath);
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

// Key capture processing | Traitement de la capture de touches
static void processKeyCapture() {
    if (!isCapturingKey) return;

    for (int vk = 1; vk < 256; vk++) {
        if (vk == 27) continue;
        if (GetAsyncKeyState(vk) & 0x8000) {
            switch (captureKeyTarget) {
            case 1: whisperKey = vk; if (hWhisperKeyEdit) SetWindowTextA(hWhisperKeyEdit, getKeyName(vk)); break;
            case 2: normalKey = vk; if (hNormalKeyEdit) SetWindowTextA(hNormalKeyEdit, getKeyName(vk)); break;
            case 3: shoutKey = vk; if (hShoutKeyEdit) SetWindowTextA(hShoutKeyEdit, getKeyName(vk)); break;
            case 4: configUIKey = vk; if (hConfigKeyEdit) SetWindowTextA(hConfigKeyEdit, getKeyName(vk)); break;
            case 5: voiceToggleKey = vk; if (hVoiceToggleKeyEdit) SetWindowTextA(hVoiceToggleKeyEdit, getKeyName(vk)); break;
            }
            isCapturingKey = FALSE;
            captureKeyTarget = 0;
            if (hWhisperButton) EnableWindow(hWhisperButton, TRUE);
            if (hNormalButton) EnableWindow(hNormalButton, TRUE);
            if (hShoutButton) EnableWindow(hShoutButton, TRUE);
            if (hConfigButton) EnableWindow(hConfigButton, TRUE);
            Sleep(200);
            break;
        }
    }
}

static int calculateButtonWidth(const wchar_t* text, HFONT font) {
    HDC hdc = GetDC(NULL);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    SIZE textSize;
    GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &textSize);

    SelectObject(hdc, oldFont);
    ReleaseDC(NULL, hdc);

    // Largeur du texte + 10 pixels de marge totale (5px de chaque côté) + padding Windows
    return textSize.cx + 10 + 20; // 20px = padding interne du bouton Windows
}

// Load background bitmap from resources | Charger l'image de fond depuis les ressources
static HBITMAP LoadBackgroundFromResource(int resourceID) {
    HMODULE hModule = NULL;
    // Obtenir le HMODULE de la DLL à partir d'une adresse interne (fonction dans cette DLL)
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)LoadBackgroundFromResource, &hModule)) {
        DWORD err = GetLastError();
        if (enableLogGeneral) {
            char errorMsg[128];
            snprintf(errorMsg, sizeof(errorMsg),
                "ERROR: GetModuleHandleExW failed (Error: %lu)", err);
            mumbleAPI.log(ownID, errorMsg);
        }
        return NULL;
    }

    // Charger le bitmap depuis les ressources de la DLL (Unicode)
    HBITMAP hBitmap = (HBITMAP)LoadImageW(
        hModule,
        MAKEINTRESOURCEW(resourceID),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION
    );

    if (!hBitmap) {
        DWORD error = GetLastError();
        if (enableLogGeneral) {
            char errorMsg[192];
            snprintf(errorMsg, sizeof(errorMsg),
                "ERROR: Failed to load background bitmap (ID=%d) from DLL resources (Error: %lu)",
                resourceID, error);
            mumbleAPI.log(ownID, errorMsg);
        }
        return NULL;
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Background bitmap loaded successfully from DLL resources");
    }

    return hBitmap;
}

static void DrawButtonWithBitmap(LPDRAWITEMSTRUCT lpDIS) {
    HDC hdc = lpDIS->hDC;
    RECT rect = lpDIS->rcItem;

    int ctrlId = lpDIS->CtlID;
    int bitmapResource = IDB_Main_Button_01; // Initialize with default value | Initialiser avec valeur par défaut

    HMODULE hModule = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)DrawButtonWithBitmap, &hModule);

    // Use main image for category and bottom buttons | Utiliser l'image principale pour les boutons de catégorie et du bas
    if (ctrlId == 301 || ctrlId == 302 || ctrlId == 303 || ctrlId == 1 || ctrlId == 11 || ctrlId == 2) {
        bitmapResource = IDB_Main_Button_01;
    }
    // Use preset box image for preset save buttons (IDs 700-709) | Utiliser l'image de preset pour les boutons de sauvegarde de preset (IDs 700-709)
    else if (ctrlId >= 700 && ctrlId < 700 + MAX_VOICE_PRESETS) {
        bitmapResource = IDB_Preset_Box_01;
    }
    // Use dedicated image for Browse button | Utiliser l'image dédiée pour le bouton Browse (ID 105)
    else if (ctrlId == 105) {
        bitmapResource = IDB_Browse_Button;
    }

    // Charger le bitmap (chaque appel charge une instance locale afin d'éviter conflits de HBITMAP partagés)
    HBITMAP hButtonBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(bitmapResource),
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    if (hButtonBitmap) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hButtonBitmap);

        BITMAP bm;
        GetObject(hButtonBitmap, sizeof(BITMAP), &bm);

        int btnWidth = rect.right - rect.left;
        int btnHeight = rect.bottom - rect.top;

        // Draw main/button images and preset box images at real size | Dessiner les images principales et les boîtes de preset à taille réelle
        if (ctrlId == 301 || ctrlId == 302 || ctrlId == 303 || ctrlId == 1 || ctrlId == 11 || ctrlId == 2 ||
            (ctrlId >= 700 && ctrlId < 700 + MAX_VOICE_PRESETS)) {
            // Draw bitmap at natural size (no stretch) | Dessiner le bitmap à taille naturelle (sans étirement)
            BitBlt(hdc,
                rect.left,
                rect.top,
                bm.bmWidth,
                bm.bmHeight,
                hdcMem,
                0, 0,
                SRCCOPY);

            // Centrer le texte sur l'image (texte récupéré depuis le bouton si disponible)
            wchar_t overlayText[128] = L"";
            GetWindowTextW(lpDIS->hwndItem, overlayText, (int)(sizeof(overlayText) / sizeof(wchar_t)));
            if (wcslen(overlayText) == 0) {
                // Valeurs par défaut si aucun texte (fallback lisible)
                if (ctrlId == 301) wcscpy_s(overlayText, sizeof(overlayText) / sizeof(wchar_t), L"Patch Configuration");
                else if (ctrlId == 302) wcscpy_s(overlayText, sizeof(overlayText) / sizeof(wchar_t), L"Advanced Options");
                else if (ctrlId == 303) wcscpy_s(overlayText, sizeof(overlayText) / sizeof(wchar_t), L"Voice Presets");
                else if (ctrlId == 1)   wcscpy_s(overlayText, sizeof(overlayText) / sizeof(wchar_t), L"Save Configuration");
                else if (ctrlId == 11)  wcscpy_s(overlayText, sizeof(overlayText) / sizeof(wchar_t), L"Save Voice Range");
                else if (ctrlId == 2)   wcscpy_s(overlayText, sizeof(overlayText) / sizeof(wchar_t), L"Cancel");
            }

            HFONT hTextFont = NULL;
            HFONT hOldFont = NULL;
            if (hFontBold) {
                hTextFont = hFontBold;
                hOldFont = (HFONT)SelectObject(hdc, hTextFont);
            }
            else {
                hTextFont = CreateFontW(
                    16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                hOldFont = (HFONT)SelectObject(hdc, hTextFont);
            }

            RECT textRect;
            textRect.left = rect.left;
            textRect.top = rect.top;
            textRect.right = rect.left + bm.bmWidth;
            textRect.bottom = rect.top + bm.bmHeight;

            // Si le bouton réel est plus grand que le bitmap, étendre le rect de texte pour centrer proprement
            if ((textRect.right - textRect.left) < btnWidth) textRect.right = rect.right;
            if ((textRect.bottom - textRect.top) < btnHeight) textRect.bottom = rect.bottom;

            SetBkMode(hdc, TRANSPARENT);
            // Ombre
            SetTextColor(hdc, RGB(0, 0, 0));
            RECT shadowRect = textRect;
            OffsetRect(&shadowRect, 1, 1);
            DrawTextW(hdc, overlayText, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Texte principal
            SetTextColor(hdc, RGB(240, 240, 240));
            DrawTextW(hdc, overlayText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
            if (!hFontBold && hTextFont) {
                DeleteObject(hTextFont);
            }
        }
        else if (ctrlId == 105) {
            // Bouton Browse : dessiner le bitmap à taille réelle, sans stretch (identique logique)
            BitBlt(hdc,
                rect.left,
                rect.top,
                bm.bmWidth,
                bm.bmHeight,
                hdcMem,
                0, 0,
                SRCCOPY);

            wchar_t overlayText[128] = L"";
            GetWindowTextW(lpDIS->hwndItem, overlayText, (int)(sizeof(overlayText) / sizeof(wchar_t)));
            if (wcslen(overlayText) == 0) wcscpy_s(overlayText, sizeof(overlayText) / sizeof(wchar_t), L"Browse");

            HFONT hTextFont = NULL;
            HFONT hOldFont = NULL;
            if (hFontBold) {
                hTextFont = hFontBold;
                hOldFont = (HFONT)SelectObject(hdc, hTextFont);
            }
            else if (hFont) {
                hTextFont = hFont;
                hOldFont = (HFONT)SelectObject(hdc, hTextFont);
            }
            else {
                hTextFont = CreateFontW(
                    16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                hOldFont = (HFONT)SelectObject(hdc, hTextFont);
            }

            RECT textRect;
            textRect.left = rect.left;
            textRect.top = rect.top;
            textRect.right = rect.left + bm.bmWidth;
            textRect.bottom = rect.top + bm.bmHeight;

            int btnWidthLocal = rect.right - rect.left;
            int btnHeightLocal = rect.bottom - rect.top;
            if ((textRect.right - textRect.left) < btnWidthLocal) textRect.right = rect.right;
            if ((textRect.bottom - textRect.top) < btnHeightLocal) textRect.bottom = rect.bottom;

            SetBkMode(hdc, TRANSPARENT);

            SetBkMode(hdc, TRANSPARENT);
            // Ombre
            SetTextColor(hdc, RGB(0, 0, 0));
            RECT shadowRect = textRect;
            OffsetRect(&shadowRect, 1, 1);
            DrawTextW(hdc, overlayText, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Texte principal en blanc
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, overlayText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
            if (!hFontBold && hTextFont) {
                DeleteObject(hTextFont);
            }
        }

        SelectObject(hdcMem, hOldBitmap);
        DeleteDC(hdcMem);
        DeleteObject(hButtonBitmap);
    }
    else {
        // Fallback : fond gris si l'image ne charge pas
        HBRUSH hBrush = CreateSolidBrush(RGB(220, 220, 220));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
    }

    // Do not redraw default text for main, bottom and preset save buttons | Ne pas redessiner le texte par défaut pour les boutons principaux, du bas et de sauvegarde des presets
    if (ctrlId != 301 && ctrlId != 302 && ctrlId != 303 && ctrlId != 105 && ctrlId != 1 && ctrlId != 11 && ctrlId != 2
        && !(ctrlId >= 700 && ctrlId < 700 + MAX_VOICE_PRESETS)) {

        wchar_t text[256];
        GetWindowTextW(lpDIS->hwndItem, text, 256);
        SetBkMode(hdc, TRANSPARENT);

        if (lpDIS->itemState & ODS_SELECTED) {
            rect.left += 2;
            rect.top += 2;
        }

        // Shadow text | Texte ombre
        SetTextColor(hdc, RGB(0, 0, 0));
        RECT shadowRect = rect;
        OffsetRect(&shadowRect, 1, 1);
        DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Main text in white | Texte principal en blanc
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

}

// Subclass procedure pour rendre les labels cliquables
static LRESULT CALLBACK CheckboxLabelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg) {
    case WM_LBUTTONDOWN: {
        // Cliquer sur le label = cocher/décocher la checkbox associée
        HWND hCheckbox = (HWND)dwRefData;
        if (hCheckbox && IsWindow(hCheckbox)) {
            LRESULT checkState = SendMessage(hCheckbox, BM_GETCHECK, 0, 0);
            SendMessage(hCheckbox, BM_SETCHECK, (checkState == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED, 0);

            // Envoyer notification au parent (simule un clic sur la checkbox)
            HWND hParent = GetParent(hCheckbox);
            if (hParent) {
                SendMessage(hParent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hCheckbox), BN_CLICKED), (LPARAM)hCheckbox);
            }
        }
        return 0;
    }

    case WM_SETCURSOR:
        // Afficher le curseur "main" (pointeur) au survol
        SetCursor(LoadCursor(NULL, IDC_HAND));
        return TRUE;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, CheckboxLabelProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Main window procedure | Procédure de la fenêtre principale
LRESULT CALLBACK ConfigDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND control;

    switch (msg) {
    case WM_CREATE:
        hConfigDialog = hwnd;

        // ✅ 1) CRÉER LES POLICES UNE SEULE FOIS
        hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        hFontBold = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        hFontLarge = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        hFontEmoji = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");

        hBackgroundBitmap = LoadBackgroundFromResource(IDB_BACKGROUND);

        hBackgroundAdvancedBitmap = LoadBackgroundFromResource(IDB_Background_Plugin_Settings);
        if (!hBackgroundAdvancedBitmap && enableLogGeneral) {
            mumbleAPI.log(ownID, "WARNING: Advanced background image (IDB_Background_Plugin_Settings) not loaded");
        }

        hBackgroundPresetsBitmap = LoadBackgroundFromResource(IDB_Background_Voice_Presets);

        // Disable double buffering to reduce flickering | Désactiver le double buffering pour réduire les scintillements
        SetWindowLongPtr(hwnd, GWL_EXSTYLE,
            GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_COMPOSITED);

        // Get real dimensions of IDB_Main_Button_01 image | Récupérer les dimensions réelles de l'image IDB_Main_Button_01
        HMODULE hModuleBtn = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)ConfigDialogProc, &hModuleBtn);

        HBITMAP hTempBitmap = (HBITMAP)LoadImageW(hModuleBtn, MAKEINTRESOURCEW(IDB_Main_Button_01),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        int imageWidth = 170;  // Valeur par défaut au cas où l'image ne charge pas
        int imageHeight = 40;  // Valeur par défaut

        if (hTempBitmap) {
            BITMAP bm;
            GetObject(hTempBitmap, sizeof(BITMAP), &bm);
            imageWidth = bm.bmWidth;   // ✅ Largeur RÉELLE de l'image
            imageHeight = bm.bmHeight; // ✅ Hauteur RÉELLE de l'image
            DeleteObject(hTempBitmap);

            if (enableLogGeneral) {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg),
                    "IDB_Main_Button_01 size: %dx%d pixels", imageWidth, imageHeight);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        {
            // Get actual window width | Récupérer la largeur réelle de la fenêtre
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int windowWidth = clientRect.right - clientRect.left;

            const int numButtons = 3;
            const float edgeToGapRatio = 2.0f; // Ratio between edge gap and internal gap | Ratio entre l'écart des bords et l'écart interne

            int totalButtonWidth = imageWidth * numButtons;
            int availableSpace = windowWidth - totalButtonWidth;

            // Check if there is enough space | Vérifier s'il y a assez d'espace
            if (availableSpace < 0) {
                if (enableLogGeneral) {
                    char errorMsg[128];
                    snprintf(errorMsg, sizeof(errorMsg),
                        "ERROR: Not enough space for buttons (window: %d, buttons: %d)",
                        windowWidth, totalButtonWidth);
                    mumbleAPI.log(ownID, errorMsg);
                }
                availableSpace = 20; // Valeur de secours
            }

            float edgeGap = (float)availableSpace * edgeToGapRatio / (2.0f * edgeToGapRatio + 2.0f);
            float internalGap = edgeGap / edgeToGapRatio;

            // Positions calculées dynamiquement
            int patchX = (int)edgeGap;
            int advX = patchX + imageWidth + (int)internalGap;
            int presetsX = advX + imageWidth + (int)internalGap;

            hCategoryPatch = CreateWindowW(L"BUTTON", L"Patch Configuration",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
                patchX - 2, 68, imageWidth, imageHeight,
                hwnd, (HMENU)301, NULL, NULL);

            hCategoryAdvanced = CreateWindowW(L"BUTTON", L"Advanced Options",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
                advX, 68, imageWidth, imageHeight, hwnd, (HMENU)302, NULL, NULL);
            ApplyFontToControl(hCategoryAdvanced, hFontBold);

            hCategoryPresets = CreateWindowW(L"BUTTON", L"Voice Presets",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
                presetsX + 2, 68, imageWidth, imageHeight, hwnd, (HMENU)303, NULL, NULL);
            ApplyFontToControl(hCategoryPresets, hFontBold);
        }

        // Create background image first | Créer d'abord l'image de fond
        hSavedPathBg = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_OWNERDRAW,
            40, 270, 380, 35, hwnd, (HMENU)1100, NULL, NULL);

        // Load current values from configuration | Charger les valeurs actuelles depuis la configuration
        wchar_t gamePathFromConfig[MAX_PATH] = L"";
        wchar_t savedPathFromConfig[MAX_PATH] = L""; // ✅ Pour stocker le chemin COMPLET du fichier de config

        // Read path from config file | Lire le chemin depuis le fichier de config
        wchar_t* configFolder = getConfigFolderPath();
        if (configFolder) {
            wchar_t configFile[MAX_PATH];
            swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);
            FILE* f = _wfopen(configFile, L"r");
            if (f) {
                wchar_t line[512];
                while (fgetws(line, 512, f)) {
                    if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                        wchar_t* pathStart = line + 10;
                        // Remove trailing newlines | Supprimer les retours à la ligne
                        wchar_t* nl = wcschr(pathStart, L'\n');
                        if (nl) *nl = L'\0';
                        wchar_t* cr = wcschr(pathStart, L'\r');
                        if (cr) *cr = L'\0';

                        // ✅ CORRECTION : Stocker le chemin COMPLET depuis la config
                        wcscpy_s(savedPathFromConfig, MAX_PATH, pathStart);

                        // ✅ Extraire le répertoire parent UNIQUEMENT pour l'affichage
                        wcscpy_s(displayedPathText, MAX_PATH, pathStart);
                        wchar_t* conanSandbox = wcsstr(displayedPathText, L"\\ConanSandbox\\Saved");
                        if (conanSandbox) {
                            *conanSandbox = L'\0'; // Tronquer pour affichage
                        }
                        break;
                    }
                }
                fclose(f);
            }
        }

        // ✅ Si aucun chemin dans la config, utiliser les valeurs par défaut
        if (wcslen(displayedPathText) == 0) {
            wcscpy_s(displayedPathText, MAX_PATH, L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles");
            wcscpy_s(savedPathFromConfig, MAX_PATH, L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles\\ConanSandbox\\Saved");
        }

        if (hSavedPathBg && IsWindow(hSavedPathBg)) {
            InvalidateRect(hSavedPathBg, NULL, TRUE);
            UpdateWindow(hSavedPathBg);
        }

        HMODULE hModule = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)ConfigDialogProc, &hModule);


        hPathBoxBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Path_Box),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        int pathBoxWidth = 380;
        int pathBoxHeight = 35;
        if (hPathBoxBitmap) {
            BITMAP bmPath;
            GetObject(hPathBoxBitmap, sizeof(BITMAP), &bmPath);
            pathBoxWidth = bmPath.bmWidth;
            pathBoxHeight = bmPath.bmHeight;
            SendMessage(hSavedPathBg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hPathBoxBitmap);
        }

        HBITMAP hBrowseTemp = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Browse_Button),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        int browseBtnWidth = 100;
        int browseBtnHeight = 35;
        if (hBrowseTemp) {
            BITMAP bmBrowse;
            GetObject(hBrowseTemp, sizeof(BITMAP), &bmBrowse);
            browseBtnWidth = bmBrowse.bmWidth;
            browseBtnHeight = bmBrowse.bmHeight;
            DeleteObject(hBrowseTemp);
        }

        int pathX = 40;
        int pathY = 270;
        int browseGap = 10;
        int browseX = pathX + pathBoxWidth + browseGap;
        int browseY = pathY + ((pathBoxHeight - browseBtnHeight) / 2);

        hSavedPathButton = CreateWindowW(L"BUTTON", L"Browse",
            WS_CHILD | BS_OWNERDRAW,
            browseX, browseY, browseBtnWidth, browseBtnHeight, hwnd, (HMENU)105, NULL, NULL);
        ApplyFontToControl(hSavedPathButton, hFontBold);

        // ========== CATÉGORIE 2 : ADVANCED OPTIONS ==========

        // === CHECKBOX 1 : Distance-based muting ===
        hEnableDistanceMutingCheck = CreateWindowW(L"BUTTON", L"",
            WS_CHILD | BS_AUTOCHECKBOX,  // ✅ WS_CHILD uniquement (pas WS_VISIBLE)
            60, 145, 20, 20, hwnd, (HMENU)201, NULL, NULL);

        HMODULE hModuleIcon = GetModuleHandleW(NULL);
        HICON hCheckIcon = (HICON)LoadImageW(hModuleIcon, MAKEINTRESOURCEW(IDI_CHECKMARK),
            IMAGE_ICON, 13, 13, LR_DEFAULTCOLOR);
        if (hCheckIcon) {
            SendMessage(hEnableDistanceMutingCheck, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hCheckIcon);
        }

        HWND hDistanceMutingLabel = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT | SS_NOTIFY,  // ✅ ENLEVER WS_VISIBLE
            85, 147, 295, 20, hwnd, (HMENU)2001, NULL, NULL);
        ApplyFontToControl(hDistanceMutingLabel, hFont);

        SetWindowSubclass(hDistanceMutingLabel, CheckboxLabelProc, 201, (DWORD_PTR)hEnableDistanceMutingCheck);


        // === CHECKBOX 2 : Automatic channel switching ===
        hEnableAutomaticChannelChangeCheck = CreateWindowW(L"BUTTON", L"",
            WS_CHILD | BS_AUTOCHECKBOX,  // ✅ WS_CHILD uniquement
            60, 165, 20, 20, hwnd, (HMENU)203, NULL, NULL);

        if (hCheckIcon) {
            SendMessage(hEnableAutomaticChannelChangeCheck, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hCheckIcon);
        }

        HWND hChannelSwitchingLabel = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT | SS_NOTIFY,  // ✅ ENLEVER WS_VISIBLE
            85, 167, 375, 20, hwnd, (HMENU)2002, NULL, NULL);
        ApplyFontToControl(hChannelSwitchingLabel, hFont);

        SetWindowSubclass(hChannelSwitchingLabel, CheckboxLabelProc, 203, (DWORD_PTR)hEnableAutomaticChannelChangeCheck);


        // === CHECKBOX 3 : Voice toggle ===
        hEnableVoiceToggleCheck = CreateWindowW(L"BUTTON", L"",
            WS_CHILD | BS_AUTOCHECKBOX,  // ✅ WS_CHILD uniquement
            60, 185, 20, 20, hwnd, (HMENU)204, NULL, NULL);

        if (hCheckIcon) {
            SendMessage(hEnableVoiceToggleCheck, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hCheckIcon);
        }

        HWND hVoiceToggleLabel = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT | SS_NOTIFY,  // ✅ ENLEVER WS_VISIBLE
            85, 187, 375, 20, hwnd, (HMENU)2003, NULL, NULL);
        ApplyFontToControl(hVoiceToggleLabel, hFont);

        SetWindowSubclass(hVoiceToggleLabel, CheckboxLabelProc, 204, (DWORD_PTR)hEnableVoiceToggleCheck);

        // Get real dimensions of IDB_Key_Box_01 | Récupérer les dimensions réelles de IDB_Key_Box_01
        HMODULE hModuleKey = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)ConfigDialogProc, &hModuleKey);

        HBITMAP hKeyBoxTemp = (HBITMAP)LoadImageW(hModuleKey, MAKEINTRESOURCEW(IDB_Key_Box_01),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        int keyBoxWidth = 110;  // Valeur par défaut
        int keyBoxHeight = 30;  // Valeur par défaut

        if (hKeyBoxTemp) {
            BITMAP bmKey;
            GetObject(hKeyBoxTemp, sizeof(BITMAP), &bmKey);
            keyBoxWidth = bmKey.bmWidth;
            keyBoxHeight = bmKey.bmHeight;
            DeleteObject(hKeyBoxTemp);

            if (enableLogGeneral) {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg),
                    "IDB_Key_Box_01 size: %dx%d pixels", keyBoxWidth, keyBoxHeight);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        // Create all key edit and button controls with Patch Configuration style | Créer tous les contrôles avec le style de Patch Configuration
        hWhisperKeyEdit = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_OWNERDRAW,
            200, 303, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)2001, NULL, NULL);
        ApplyFontToControl(hWhisperKeyEdit, hFont);

        hWhisperButton = CreateWindowW(L"BUTTON", L"Set Key",
            WS_CHILD | BS_OWNERDRAW,
            200 + keyBoxWidth + 10, 303, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)101, NULL, NULL);
        ApplyFontToControl(hWhisperButton, hFontBold);

        hNormalKeyEdit = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_OWNERDRAW,
            200, 338, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)2002, NULL, NULL);
        ApplyFontToControl(hNormalKeyEdit, hFont);

        hNormalButton = CreateWindowW(L"BUTTON", L"Set Key",
            WS_CHILD | BS_OWNERDRAW,
            200 + keyBoxWidth + 10, 338, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)102, NULL, NULL);
        ApplyFontToControl(hNormalButton, hFontBold);

        hShoutKeyEdit = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_OWNERDRAW,
            200, 373, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)2003, NULL, NULL);
        ApplyFontToControl(hShoutKeyEdit, hFont);

        hShoutButton = CreateWindowW(L"BUTTON", L"Set Key",
            WS_CHILD | BS_OWNERDRAW,
            200 + keyBoxWidth + 10, 373, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)103, NULL, NULL);
        ApplyFontToControl(hShoutButton, hFontBold);

        hConfigKeyEdit = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_OWNERDRAW,
            200, 408, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)2004, NULL, NULL);
        ApplyFontToControl(hConfigKeyEdit, hFont);

        hConfigButton = CreateWindowW(L"BUTTON", L"Set Key",
            WS_CHILD | BS_OWNERDRAW,
            200 + keyBoxWidth + 10, 408, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)104, NULL, NULL);
        ApplyFontToControl(hConfigButton, hFontBold);

        hVoiceToggleKeyEdit = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_OWNERDRAW,
            200, 485, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)2005, NULL, NULL);
        ApplyFontToControl(hVoiceToggleKeyEdit, hFont);

        hVoiceToggleButton = CreateWindowW(L"BUTTON", L"Set Key",
            WS_CHILD | BS_OWNERDRAW,
            200 + keyBoxWidth + 10, 485, keyBoxWidth, keyBoxHeight, hwnd, (HMENU)106, NULL, NULL);
        ApplyFontToControl(hVoiceToggleButton, hFontBold);

        hDistanceWhisperEdit = CreateWindowW(L"EDIT", L"2.0",
            WS_CHILD | WS_BORDER | ES_CENTER,
            155, 550, 60, 28, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hDistanceWhisperEdit, hFont);

        hDistanceNormalEdit = CreateWindowW(L"EDIT", L"15.0",
            WS_CHILD | WS_BORDER | ES_CENTER,
            315, 550, 60, 28, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hDistanceNormalEdit, hFont);

        hDistanceShoutEdit = CreateWindowW(L"EDIT", L"50.0",
            WS_CHILD | WS_BORDER | ES_CENTER,
            465, 550, 60, 28, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hDistanceShoutEdit, hFont);

        createPresetsCategory();

        hDistanceWhisperMessage = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT,
            60, 585, 480, 20, hwnd, (HMENU)520, NULL, NULL);
        ApplyFontToControl(hDistanceWhisperMessage, hFont);

        hDistanceNormalMessage = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT,
            60, 600, 480, 20, hwnd, (HMENU)521, NULL, NULL);
        ApplyFontToControl(hDistanceNormalMessage, hFont);

        hDistanceShoutMessage = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT,
            60, 615, 480, 20, hwnd, (HMENU)522, NULL, NULL);
        ApplyFontToControl(hDistanceShoutMessage, hFont);

        hDistanceMutingMessage = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT,
            60, 210, 460, 18, hwnd, (HMENU)523, NULL, NULL);
        ApplyFontToControl(hDistanceMutingMessage, hFont);

        hChannelSwitchingMessage = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT,
            60, 230, 460, 18, hwnd, (HMENU)524, NULL, NULL);
        ApplyFontToControl(hChannelSwitchingMessage, hFont);

        hPositionalAudioMessage = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT,
            60, 250, 460, 18, hwnd, (HMENU)525, NULL, NULL);
        ApplyFontToControl(hPositionalAudioMessage, hFont);

        const wchar_t* saveConfigText = L"Save Configuration";
        const wchar_t* saveVoiceRangeText = L"Save Voice Range";
        const wchar_t* cancelText = L"Cancel";

        int bottomBtnWidth = imageWidth;
        int bottomBtnHeight = imageHeight;

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;

        const int gap = 9;
        const int btnY = 654;

        // Centrer les trois boutons en utilisant la largeur réelle de l'image
        int totalWidth = bottomBtnWidth * 3 + gap * 2;
        int startX = (clientWidth - totalWidth) / 2;
        if (startX < 10) startX = 10;

        int saveVoiceRangeX = startX;
        int saveConfigX = startX + bottomBtnWidth + gap;
        int cancelX = startX + (bottomBtnWidth + gap) * 2;

        // Créer les boutons avec BS_OWNERDRAW — DrawButtonWithBitmap dessinera l'image à taille réelle
        control = CreateWindowW(L"BUTTON", saveVoiceRangeText,
            WS_CHILD | BS_OWNERDRAW,  // Caché par défaut
            saveVoiceRangeX, btnY, bottomBtnWidth, bottomBtnHeight, hwnd, (HMENU)11, NULL, NULL);
        ApplyFontToControl(control, hFont);

        control = CreateWindowW(L"BUTTON", saveConfigText,
            WS_CHILD | BS_OWNERDRAW,
            saveConfigX, btnY, bottomBtnWidth, bottomBtnHeight, hwnd, (HMENU)1, NULL, NULL);
        ApplyFontToControl(control, hFont);

        control = CreateWindowW(L"BUTTON", cancelText,
            WS_CHILD | BS_OWNERDRAW,
            cancelX, btnY, bottomBtnWidth, bottomBtnHeight, hwnd, (HMENU)2, NULL, NULL);
        ApplyFontToControl(control, hFont);

        // Status message (toujours visible)
        hStatusMessage = CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            40, 740, 520, 25, hwnd, (HMENU)600, NULL, NULL);
        ApplyFontToControl(hStatusMessage, hFont);

        // ✅ 8) CHARGER LES VALEURS (une seule fois)
        loadVoiceDistancesFromConfig();

        ShowCategoryControls(1);

        // Set actual values | Définir les valeurs réelles
        if (wcslen(gamePathFromConfig) > 0) {
            SetWindowTextW(hSavedPathEdit, gamePathFromConfig);
        }
        else {
            SetWindowTextW(hSavedPathEdit, L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles");
        }

        SetWindowTextA(hWhisperKeyEdit, getKeyName(whisperKey));
        SetWindowTextA(hNormalKeyEdit, getKeyName(normalKey));
        SetWindowTextA(hShoutKeyEdit, getKeyName(shoutKey));
        SetWindowTextA(hConfigKeyEdit, getKeyName(configUIKey));
        SetWindowTextA(hVoiceToggleKeyEdit, getKeyName(voiceToggleKey));
        CheckDlgButton(hwnd, 201, enableDistanceMuting ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, 203, enableAutomaticChannelChange ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, 204, enableVoiceToggle ? BST_CHECKED : BST_UNCHECKED);

        // CORRECTION: Utiliser les valeurs CHARGÉES depuis le fichier de configuration
        // au lieu des valeurs par défaut
        wchar_t whisperText[32], normalText[32], shoutText[32];
        swprintf(whisperText, 32, L"%.1f", distanceWhisper);
        swprintf(normalText, 32, L"%.1f", distanceNormal);
        swprintf(shoutText, 32, L"%.1f", distanceShout);

        SetWindowTextW(hDistanceWhisperEdit, whisperText);
        SetWindowTextW(hDistanceNormalEdit, normalText);
        SetWindowTextW(hDistanceShoutEdit, shoutText);

        if (enableLogConfig) {
            char debugMsg[256];
            snprintf(debugMsg, sizeof(debugMsg),
                "WM_CREATE: Distances set in fields - Whisper: %.1f, Normal: %.1f, Shout: %.1f",
                distanceWhisper, distanceNormal, distanceShout);
            mumbleAPI.log(ownID, debugMsg);
        }

        // Create preset category controls | Créer les contrôles de la catégorie presets
        createPresetsCategory();

        // Load presets from config | Charger les presets depuis la configuration
        loadPresetsFromConfigFile();

        ShowCategoryControls(1);

        break;

case WM_ERASEBKGND: {
    HDC hdc = (HDC)wParam;
    RECT rect;
    GetClientRect(hwnd, &rect);

    int winW = rect.right - rect.left;
    int winH = rect.bottom - rect.top;

    // CATÉGORIE 1 : Afficher BACKGROUND.bmp | Category 1: Display BACKGROUND.bmp
    if (currentCategory == 1 && hBackgroundBitmap) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBackgroundBitmap);

        BITMAP bm;
        GetObject(hBackgroundBitmap, sizeof(BITMAP), &bm);

        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc,
            0, 0, winW, winH,
            hdcMem,
            0, 0, bm.bmWidth, bm.bmHeight,
            SRCCOPY);

        SelectObject(hdcMem, hOldBitmap);
        DeleteDC(hdcMem);
    }
    // CATÉGORIE 2 : Afficher BACKGROUND_Plugin_Settings | Category 2: Display BACKGROUND_Plugin_Settings
    else if (currentCategory == 2 && hBackgroundAdvancedBitmap) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBackgroundAdvancedBitmap);

        BITMAP bm;
        GetObject(hBackgroundAdvancedBitmap, sizeof(BITMAP), &bm);

        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc,
            0, 0, winW, winH,
            hdcMem,
            0, 0, bm.bmWidth, bm.bmHeight,
            SRCCOPY);

        SelectObject(hdcMem, hOldBitmap);
        DeleteDC(hdcMem);
    }
    // CATÉGORIE 3 : Afficher Background_Voice_Presets | Category 3: Display Background_Voice_Presets
    else if (currentCategory == 3 && hBackgroundPresetsBitmap) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBackgroundPresetsBitmap);

        BITMAP bm;
        GetObject(hBackgroundPresetsBitmap, sizeof(BITMAP), &bm);

        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc,
            0, 0, winW, winH,
            hdcMem,
            0, 0, bm.bmWidth, bm.bmHeight,
            SRCCOPY);

        SelectObject(hdcMem, hOldBitmap);
        DeleteDC(hdcMem);
    }
    else {
        // Fallback : Fond uni pour toutes les catégories sans image | Fallback: Solid background for all categories without image
        HBRUSH hBrush = CreateSolidBrush(RGB(248, 249, 250));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
    }

    return 1;
}

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        int controlId = GetDlgCtrlID(hwndStatic);

        SetBkMode(hdcStatic, TRANSPARENT);

        wchar_t messageText[300];
        GetWindowTextW(hwndStatic, messageText, 300);

        // ========== TEXTES D'INFORMATION (IDs 401, 402, 403) EN BLEU ==========
        if (controlId >= 401 && controlId <= 403) {
            SetTextColor(hdcStatic, RGB(0, 0, 0)); // Noir
        }
        // ========== MESSAGES DE VALIDATION DE RANGE (BLEU) ==========
        // IDs 520-522 = Messages whisper/normal/shout au bas de l'interface
        else if (controlId >= 520 && controlId <= 522) {
            // Si le message contient "Valid range" ou "Free range" → BLEU
            if (wcsstr(messageText, L"Valid range") || wcsstr(messageText, L"Free range")) {
                SetTextColor(hdcStatic, RGB(59, 130, 246)); // Bleu vif
            }
            // Si le message contient "Auto-corrected" → ROUGE
            else if (wcsstr(messageText, L"Auto-corrected")) {
                SetTextColor(hdcStatic, RGB(220, 38, 38)); // Rouge
            }
            else {
                SetTextColor(hdcStatic, RGB(107, 114, 128)); // Gris par défaut
            }
        }
        // ========== MESSAGES DE STATUT (IDs 523-525) ==========
        // Distance Muting, Channel Switching, Positional Audio
        else if (controlId >= 523 && controlId <= 525) {
            // ✅ CORRECTION : Vérifier "INFO" EN PREMIER (avant LOCKED/FORCED)
            if (wcsstr(messageText, L"INFO:")) {
                SetTextColor(hdcStatic, RGB(59, 130, 246)); // BLEU pour INFO
            }
            // Si le message contient "LOCKED" ou "FORCED" → ROUGE FONCÉ
            else if (wcsstr(messageText, L"LOCKED") || wcsstr(messageText, L"FORCED") ||
                wcsstr(messageText, L"ACTIVE")) {
                SetTextColor(hdcStatic, RGB(139, 0, 0)); // Rouge foncé (DarkRed)
            }
            // Si le message contient "OK" ou "Enabled" → VERT
            else if (wcsstr(messageText, L"OK") || wcsstr(messageText, L"Enabled")) {
                SetTextColor(hdcStatic, RGB(34, 197, 94)); // Vert
            }
            else {
                SetTextColor(hdcStatic, RGB(107, 114, 128)); // Gris
            }
        }
        // ========== MESSAGE DE STATUT EN BAS (ID 600) ==========
        else if (controlId == 600) {
            // Erreurs → ROUGE
            if (wcsstr(messageText, L"\u26A0") || wcsstr(messageText, L"Error") ||
                wcsstr(messageText, L"does not exist")) {
                SetTextColor(hdcStatic, RGB(220, 53, 69)); // Rouge vif
            }
            // Succès → VERT
            else if (wcsstr(messageText, L"\u2705") || wcsstr(messageText, L"\u2699") ||
                wcsstr(messageText, L"success")) {
                SetTextColor(hdcStatic, RGB(40, 167, 69)); // Vert
            }
            else {
                SetTextColor(hdcStatic, RGB(108, 117, 125)); // Gris
            }
        }
        // ========== AUTRES TEXTES (COULEUR PAR DÉFAUT) ==========
        else {
            SetTextColor(hdcStatic, RGB(33, 37, 41)); // Gris très foncé
        }

        // ✅ Retourner NULL_BRUSH pour fond transparent
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 301: ShowCategoryControls(1); break;
        case 302:
            ShowCategoryControls(2);
            updateConsolidatedDistanceMessages();
            break;
        case 303: ShowCategoryControls(3); break;
        case 105: browseSavedPath(hwnd); break;

        case 101:
            isCapturingKey = TRUE; captureKeyTarget = 1;
            EnableWindow(hWhisperButton, FALSE); EnableWindow(hNormalButton, FALSE);
            EnableWindow(hShoutButton, FALSE); EnableWindow(hConfigButton, FALSE);
            SetWindowTextA(hWhisperKeyEdit, "Press key..."); break;

        case 102:
            isCapturingKey = TRUE; captureKeyTarget = 2;
            EnableWindow(hWhisperButton, FALSE); EnableWindow(hNormalButton, FALSE);
            EnableWindow(hShoutButton, FALSE); EnableWindow(hConfigButton, FALSE);
            SetWindowTextA(hNormalKeyEdit, "Press key..."); break;

        case 103:
            isCapturingKey = TRUE; captureKeyTarget = 3;
            EnableWindow(hWhisperButton, FALSE); EnableWindow(hNormalButton, FALSE);
            EnableWindow(hShoutButton, FALSE); EnableWindow(hConfigButton, FALSE);
            SetWindowTextA(hShoutKeyEdit, "Press key..."); break;

        case 104:
            isCapturingKey = TRUE; captureKeyTarget = 4;
            EnableWindow(hWhisperButton, FALSE); EnableWindow(hNormalButton, FALSE);
            EnableWindow(hShoutButton, FALSE); EnableWindow(hConfigButton, FALSE);
            SetWindowTextA(hConfigKeyEdit, "Press key..."); break;

        case 106:
            isCapturingKey = TRUE; captureKeyTarget = 5;
            EnableWindow(hWhisperButton, FALSE); EnableWindow(hNormalButton, FALSE);
            EnableWindow(hShoutButton, FALSE); EnableWindow(hConfigButton, FALSE);
            EnableWindow(hVoiceToggleButton, FALSE);
            SetWindowTextA(hVoiceToggleKeyEdit, "Press key..."); break;

        case 201: // Distance Muting checkbox
            if (HIWORD(wParam) == BN_CLICKED) {
                if (hubForceDistanceBasedMuting) {
                    // Serveur force -> garder coché
                    CheckDlgButton(hwnd, 201, BST_CHECKED);
                    enableDistanceMuting = TRUE;
                    showStatusMessage(L"Cannot disable: enforced by server", TRUE);
                    MessageBeep(MB_ICONWARNING);
                }
                else {
                    // Toggle normal géré par Windows
                    enableDistanceMuting = (IsDlgButtonChecked(hwnd, 201) == BST_CHECKED);
                    updateDynamicInterface();
                }
            }
            break;

        case 203: // Automatic Channel Change checkbox
            if (HIWORD(wParam) == BN_CLICKED) {
                if (hubForceAutomaticChannelSwitching) {
                    // Serveur force -> garder coché
                    CheckDlgButton(hwnd, 203, BST_CHECKED);
                    enableAutomaticChannelChange = TRUE;
                    showStatusMessage(L"Cannot disable: enforced by server", TRUE);
                    MessageBeep(MB_ICONWARNING);
                }
                else {
                    // Toggle normal géré par Windows
                    enableAutomaticChannelChange = (IsDlgButtonChecked(hwnd, 203) == BST_CHECKED);
                    updateDynamicInterface();
                }
            }
            break;

        case 204: // Voice Toggle checkbox
            if (HIWORD(wParam) == BN_CLICKED) {
                // Pas de verrou serveur - toggle normal
                enableVoiceToggle = (IsDlgButtonChecked(hwnd, 204) == BST_CHECKED);
            }
            break;

        case 1: { // Save Configuration (Patch + Advanced)
            if (currentCategory == 1) {
                // === CATÉGORIE 1 : PATCH CONFIGURATION ===

                // ✅ 1) Vérifier que displayedPathText contient un chemin valide
                if (wcslen(displayedPathText) == 0) {
                    showStatusMessage(L"⚠ Error: No game path specified", TRUE);
                    MessageBoxW(hwnd,
                        L"Please select your Conan Exiles game folder using the Browse button.",
                        L"Missing Path", MB_OK | MB_ICONWARNING);
                    break;
                }

                showStatusMessage(L"Validating game folder...", FALSE);
                UpdateWindow(hwnd);

                // ✅ 2) Vérifier UNIQUEMENT que le dossier ConanSandbox\Saved existe (pas Pos.txt)
                wchar_t savedFolderPath[MAX_PATH];
                wcscpy_s(savedFolderPath, MAX_PATH, displayedPathText);

                // Si le chemin ne se termine pas par \Saved, l'ajouter
                if (!wcsstr(savedFolderPath, L"\\ConanSandbox\\Saved")) {
                    wcscat_s(savedFolderPath, MAX_PATH, L"\\ConanSandbox\\Saved");
                }

                DWORD savedAttribs = GetFileAttributesW(savedFolderPath);
                if (savedAttribs == INVALID_FILE_ATTRIBUTES || !(savedAttribs & FILE_ATTRIBUTE_DIRECTORY)) {
                    showStatusMessage(L"⚠ Error: ConanSandbox\\Saved folder not found", TRUE);

                    wchar_t errorMsg[512];
                    swprintf(errorMsg, 512,
                        L"The folder 'ConanSandbox\\Saved' does not exist in:\n%s\n\n"
                        L"Please verify:\n"
                        L"1. This is your Conan Exiles game folder\n",
                        displayedPathText);

                    MessageBoxW(hwnd, errorMsg, L"Folder Not Found", MB_OK | MB_ICONERROR);
                    break;
                }

                // ✅ 3) Toutes les vérifications passées → SAUVEGARDER
                showStatusMessage(L"Saving patch configuration...", FALSE);
                UpdateWindow(hwnd);

                wchar_t distWhisper[32], distNormal[32], distShout[32];
                swprintf(distWhisper, 32, L"%.1f", distanceWhisper);
                swprintf(distNormal, 32, L"%.1f", distanceNormal);
                swprintf(distShout, 32, L"%.1f", distanceShout);

                // Extraire le dossier du jeu (sans ConanSandbox\Saved) pour writeFullConfiguration
                wchar_t gameFolder[MAX_PATH];
                wcscpy_s(gameFolder, MAX_PATH, displayedPathText);
                wchar_t* conanSandbox = wcsstr(gameFolder, L"\\ConanSandbox\\Saved");
                if (conanSandbox) {
                    *conanSandbox = L'\0';
                }

                BOOL wasAlreadySaved = isPatchAlreadySaved();

                writeFullConfiguration(gameFolder, distWhisper, distNormal, distShout);

                if (!wasAlreadySaved) {
                    showStatusMessage(L"✅ Patch configuration saved successfully!", FALSE);
                }
                else {
                    showStatusMessage(L"✅ Patch configuration updated successfully!", FALSE);
                }

                if (enableLogConfig) {
                    char logMsg[512];
                    size_t converted = 0;
                    char savedPathUtf8[MAX_PATH];
                    wcstombs_s(&converted, savedPathUtf8, MAX_PATH, savedFolderPath, _TRUNCATE);

                    snprintf(logMsg, sizeof(logMsg),
                        "✅ SECURITY PASSED: Saved folder verified at: %s",
                        savedPathUtf8);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
            else if (currentCategory == 2) {
                // === CATÉGORIE 2 : ADVANCED OPTIONS ===
                // ✅ CORRECTION : Récupérer TOUTES les valeurs de l'interface (comme Patch Config)

                // Récupérer les touches clavier depuis l'interface
                // (Les valeurs sont déjà dans whisperKey, normalKey, shoutKey, configUIKey, voiceToggleKey
                // car elles sont mises à jour lors de la capture de touche)

                // Récupérer les checkboxes
                enableDistanceMuting = (IsDlgButtonChecked(hwnd, 201) == BST_CHECKED);
                enableAutomaticChannelChange = (IsDlgButtonChecked(hwnd, 203) == BST_CHECKED);
                enableVoiceToggle = (IsDlgButtonChecked(hwnd, 204) == BST_CHECKED);

                // Récupérer les distances
                wchar_t distWhisper[32], distNormal[32], distShout[32];
                GetWindowTextW(hDistanceWhisperEdit, distWhisper, 32);
                GetWindowTextW(hDistanceNormalEdit, distNormal, 32);
                GetWindowTextW(hDistanceShoutEdit, distShout, 32);

                // Convertir les distances
                distanceWhisper = (float)_wtof(distWhisper);
                distanceNormal = (float)_wtof(distNormal);
                distanceShout = (float)_wtof(distShout);

                // ✅ SAUVEGARDER AVEC writeFullConfiguration (comme Patch Config)
                // pour inclure TOUTES les touches clavier
                wchar_t gameFolder[MAX_PATH] = L"";

                // Récupérer le chemin du jeu depuis le fichier de config
                wchar_t* configFolder = getConfigFolderPath();
                if (configFolder) {
                    wchar_t configFile[MAX_PATH];
                    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);
                    FILE* f = _wfopen(configFile, L"r");
                    if (f) {
                        wchar_t line[512];
                        while (fgetws(line, 512, f)) {
                            if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                                wchar_t* pathStart = line + 10;
                                wchar_t* nl = wcschr(pathStart, L'\n');
                                if (nl) *nl = L'\0';
                                wchar_t* cr = wcschr(pathStart, L'\r');
                                if (cr) *cr = L'\0';

                                // Extraire le dossier parent (sans ConanSandbox\Saved)
                                wcscpy_s(gameFolder, MAX_PATH, pathStart);
                                wchar_t* conanSandbox = wcsstr(gameFolder, L"\\ConanSandbox\\Saved");
                                if (conanSandbox) {
                                    *conanSandbox = L'\0';
                                }
                                break;
                            }
                        }
                        fclose(f);
                    }
                }

                writeFullConfiguration(gameFolder, distWhisper, distNormal, distShout);

                // Restaurer le mode vocal
                float currentVoiceDistance = localVoiceData.voiceDistance;
                if (fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceNormal) &&
                    fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceShout)) {
                    localVoiceData.voiceDistance = distanceWhisper;
                }
                else if (fabsf(currentVoiceDistance - distanceShout) < fabsf(currentVoiceDistance - distanceNormal)) {
                    localVoiceData.voiceDistance = distanceShout;
                }
                else {
                    localVoiceData.voiceDistance = distanceNormal;
                }

                // Appliquer les changements
                applyDistanceToAllPlayers();

                showStatusMessage(L"Advanced options saved successfully!", FALSE);

                if (enableLogConfig) {
                    char logMsg[512];
                    snprintf(logMsg, sizeof(logMsg),
                        "✅ ADVANCED OPTIONS SAVED: WhisperKey=%d NormalKey=%d ShoutKey=%d ConfigKey=%d VoiceToggleKey=%d Whisper=%.1f Normal=%.1f Shout=%.1f Muting=%s AutoChannel=%s VoiceToggle=%s",
                        whisperKey, normalKey, shoutKey, configUIKey, voiceToggleKey,
                        distanceWhisper, distanceNormal, distanceShout,
                        enableDistanceMuting ? "true" : "false",
                        enableAutomaticChannelChange ? "true" : "false",
                        enableVoiceToggle ? "true" : "false");
                    mumbleAPI.log(ownID, logMsg);
                }
            }
            break;
        }

        case 11: { // Save Voice Range (Advanced Options)
            showPresetSaveDialog();
            break;
        }

        case 12: { // Save Configuration (Advanced Options - save ALL to plugin.cfg)
            // Save current voice mode before modifying | Sauvegarder le mode vocal actuel
            float currentVoiceDistance = localVoiceData.voiceDistance;

            // Get values from interface | Récupérer les valeurs de l'interface
            enableDistanceMuting = (IsDlgButtonChecked(hwnd, 201) == BST_CHECKED);
            enableAutomaticChannelChange = (IsDlgButtonChecked(hwnd, 203) == BST_CHECKED);
            enableVoiceToggle = (IsDlgButtonChecked(hwnd, 204) == BST_CHECKED);

            wchar_t distWhisper[32], distNormal[32], distShout[32];
            GetWindowTextW(hDistanceWhisperEdit, distWhisper, 32);
            GetWindowTextW(hDistanceNormalEdit, distNormal, 32);
            GetWindowTextW(hDistanceShoutEdit, distShout, 32);

            // Convert distances | Convertir les distances
            float whisperValue = (float)_wtof(distWhisper);
            float normalValue = (float)_wtof(distNormal);
            float shoutValue = (float)_wtof(distShout);

            // Update global distances | Mettre à jour les distances globales
            distanceWhisper = whisperValue;
            distanceNormal = normalValue;
            distanceShout = shoutValue;

            // Save everything using saveVoiceSettings() | Tout sauvegarder avec saveVoiceSettings()
            saveVoiceSettings();

            // Restore voice mode | Restaurer le mode vocal
            if (fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceNormal) &&
                fabsf(currentVoiceDistance - distanceWhisper) < fabsf(currentVoiceDistance - distanceShout)) {
                localVoiceData.voiceDistance = distanceWhisper;
            }
            else if (fabsf(currentVoiceDistance - distanceShout) < fabsf(currentVoiceDistance - distanceNormal)) {
                localVoiceData.voiceDistance = distanceShout;
            }
            else {
                localVoiceData.voiceDistance = distanceNormal;
            }

            // Apply changes | Appliquer les changements
            applyDistanceToAllPlayers();

            showStatusMessage(L"Advanced options saved successfully!", FALSE);

            if (enableLogConfig) {
                char logMsg[256];
                snprintf(logMsg, sizeof(logMsg),
                    "Advanced options saved: Whisper=%.1f Normal=%.1f Shout=%.1f Muting=%s AutoChannel=%s VoiceToggle=%s",
                    distanceWhisper, distanceNormal, distanceShout,
                    enableDistanceMuting ? "true" : "false",
                    enableAutomaticChannelChange ? "true" : "false",
                    enableVoiceToggle ? "true" : "false");
                mumbleAPI.log(ownID, logMsg);
            }
            break;
        }

        case 2: DestroyWindow(hwnd); break;

            // CORRECTION CRITIQUE: Gérer les boutons LOAD et RENAME **EN DEHORS** de EN_CHANGE
        default:
            // Handle preset load buttons | Gérer boutons load
            if (LOWORD(wParam) >= 900 && LOWORD(wParam) < 900 + MAX_VOICE_PRESETS) {
                int presetIndex = LOWORD(wParam) - 900;
                loadVoicePreset(presetIndex);
                break;
            }
            // Handle preset rename buttons | Gérer boutons rename
            else if (LOWORD(wParam) >= 950 && LOWORD(wParam) < 950 + MAX_VOICE_PRESETS) {
                int presetIndex = LOWORD(wParam) - 950;
                renamePresetIndex = presetIndex;

                if (!hPresetRenameDialog || !IsWindow(hPresetRenameDialog)) {
                    const wchar_t RENAME_DIALOG_CLASS[] = L"PresetRenameDialogClass";
                    WNDCLASSW wc = { 0 };
                    wc.lpfnWndProc = PresetRenameDialogProc;
                    wc.hInstance = GetModuleHandleW(NULL);
                    wc.lpszClassName = RENAME_DIALOG_CLASS;
                    wc.hbrBackground = CreateSolidBrush(RGB(248, 249, 250));
                    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

                    UnregisterClassW(RENAME_DIALOG_CLASS, wc.hInstance);
                    RegisterClassW(&wc);

                    // ✅ CORRECTION : Centrer au-dessus de l'interface principale
                    int dialogWidth = 300;
                    int dialogHeight = 240;
                    int dialogX, dialogY;

                    if (hwnd && IsWindow(hwnd)) {
                        // Get parent window position and size | Obtenir position et taille de la fenêtre parente
                        RECT parentRect;
                        GetWindowRect(hwnd, &parentRect);

                        int parentWidth = parentRect.right - parentRect.left;
                        int parentHeight = parentRect.bottom - parentRect.top;
                        int parentX = parentRect.left;
                        int parentY = parentRect.top;
                        dialogX = parentX + (parentWidth - dialogWidth) / 2;
                        dialogY = parentY + (parentHeight - dialogHeight) / 2;

                        if (enableLogGeneral) {
                            char logMsg[256];
                            snprintf(logMsg, sizeof(logMsg),
                                "Rename dialog: Parent at (%d,%d), Dialog at (%d,%d)",
                                parentX, parentY, dialogX, dialogY);
                            mumbleAPI.log(ownID, logMsg);
                        }
                    }
                    else {
                        // Fallback to screen center if parent not available | Centrer sur l'écran si parent indisponible
                        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                        dialogX = (screenWidth - dialogWidth) / 2;
                        dialogY = (screenHeight - dialogHeight) / 2;
                    }

                    hPresetRenameDialog = CreateWindowExW(
                        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                        RENAME_DIALOG_CLASS,
                        L"Rename Preset",
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                        dialogX, dialogY, dialogWidth, dialogHeight,
                        hwnd, NULL, wc.hInstance, NULL);

                    if (hPresetRenameDialog) {
                        SetLayeredWindowAttributes(hPresetRenameDialog, 0, 250, LWA_ALPHA);
                        ShowWindow(hPresetRenameDialog, SW_SHOW);
                        UpdateWindow(hPresetRenameDialog);
                    }
                }
                break;
            }
            // Handle distance field changes | Gestion des changements dans les champs de distance
            else if (HIWORD(wParam) == EN_CHANGE) {
                HWND hEditControl = (HWND)lParam;
                if (hEditControl == hDistanceWhisperEdit) {
                    handleDistanceEditChange(1);
                }
                else if (hEditControl == hDistanceNormalEdit) {
                    handleDistanceEditChange(2);
                }
                else if (hEditControl == hDistanceShoutEdit) {
                    handleDistanceEditChange(3);
                }
            }
            break;
        }
        break;

    case WM_TIMER:
        if (wParam == 1) {
            // Timer for key capture | Timer pour capture des touches
            processKeyCapture();
        }
        else if (wParam == 2) {
            // Timer to clear status message | Timer pour effacer le message de statut
            clearStatusMessage();
            KillTimer(hwnd, 2);
        }

        break;

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
        int ctrlId = lpDIS->CtlID;

        if (ctrlId >= 2001 && ctrlId <= 2005) {
            HDC hdc = lpDIS->hDC;
            RECT rect = lpDIS->rcItem;

            // Load bitmap resource | Charger la ressource bitmap
            HMODULE hModule = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)ConfigDialogProc, &hModule);

            HBITMAP hBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Key_Box_01),
                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

            if (hBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

                int width = rect.right - rect.left;
                int height = rect.bottom - rect.top;

                StretchBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, width, height, SRCCOPY);

                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
                DeleteObject(hBitmap);
            }

            // Draw text with Patch Configuration style | Dessiner le texte avec le style de Patch Configuration
            HWND hCtrl = lpDIS->hwndItem;
            wchar_t text[256] = L"";
            GetWindowTextW(hCtrl, text, 256);

            if (wcslen(text) > 0) {
                HFONT hTextFont = NULL;
                HFONT hOldFont = NULL;
                if (hFont) {
                    hTextFont = hFont;
                    hOldFont = (HFONT)SelectObject(hdc, hTextFont);
                }
                else {
                    hTextFont = CreateFontW(
                        18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                    hOldFont = (HFONT)SelectObject(hdc, hTextFont);
                }

                SetBkMode(hdc, TRANSPARENT);

                // Shadow text | Texte ombre
                SetTextColor(hdc, RGB(0, 0, 0));
                RECT shadowRect = rect;
                OffsetRect(&shadowRect, 1, 1);
                DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Main text | Texte principal
                SetTextColor(hdc, RGB(240, 240, 240));
                DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, hOldFont);
                if (!hFontBold && hTextFont) {
                    DeleteObject(hTextFont);
                }
            }

            return TRUE;
        }

        // Draw Rename buttons with Rename Box image | Dessiner les boutons Rename avec l'image Rename Box
        if (ctrlId >= 950 && ctrlId < 950 + MAX_VOICE_PRESETS) {
            HDC hdc = lpDIS->hDC;
            RECT rect = lpDIS->rcItem;

            // Load bitmap resource | Charger la ressource bitmap
            HMODULE hModule = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)ConfigDialogProc, &hModule);

            HBITMAP hBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Rename_Box_01),
                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

            if (hBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

                BITMAP bm;
                GetObject(hBitmap, sizeof(BITMAP), &bm);

                // Draw bitmap at real size (no stretch) | Dessiner le bitmap à taille réelle (pas de stretch)
                BitBlt(hdc,
                    rect.left,
                    rect.top,
                    bm.bmWidth,
                    bm.bmHeight,
                    hdcMem,
                    0, 0,
                    SRCCOPY);

                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
                DeleteObject(hBitmap);

                // Draw text centered on image | Dessiner le texte centré sur l'image
                wchar_t text[32] = L"Rename";
                if (wcslen(text) > 0) {
                    HFONT hTextFont = hFont ? hFont : CreateFontW(
                        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hTextFont);

                    RECT textRect;
                    textRect.left = rect.left;
                    textRect.top = rect.top;
                    textRect.right = rect.left + bm.bmWidth;
                    textRect.bottom = rect.top + bm.bmHeight;

                    SetBkMode(hdc, TRANSPARENT);

                    // Shadow text | Texte ombre
                    SetTextColor(hdc, RGB(0, 0, 0));
                    RECT shadowRect = textRect;
                    OffsetRect(&shadowRect, 1, 1);
                    DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    // Main text | Texte principal
                    SetTextColor(hdc, RGB(240, 240, 240));
                    DrawTextW(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(hdc, hOldFont);
                    if (!hFont) DeleteObject(hTextFont);
                }
            }

            return TRUE;
        }

        // Draw Load buttons with Load Box image | Dessiner les boutons Load avec l'image Load Box
        if (ctrlId >= 900 && ctrlId < 900 + MAX_VOICE_PRESETS) {
            HDC hdc = lpDIS->hDC;
            RECT rect = lpDIS->rcItem;

            // Load bitmap resource | Charger la ressource bitmap
            HMODULE hModule = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)ConfigDialogProc, &hModule);

            HBITMAP hBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Load_Box_01),
                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

            if (hBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

                BITMAP bm;
                GetObject(hBitmap, sizeof(BITMAP), &bm);

                // Draw bitmap at real size (no stretch) | Dessiner le bitmap à taille réelle (pas de stretch)
                BitBlt(hdc,
                    rect.left,
                    rect.top,
                    bm.bmWidth,
                    bm.bmHeight,
                    hdcMem,
                    0, 0,
                    SRCCOPY);

                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
                DeleteObject(hBitmap);

                // Draw text centered on image | Dessiner le texte centré sur l'image
                wchar_t text[32] = L"Load";
                if (wcslen(text) > 0) {
                    HFONT hTextFont = hFont ? hFont : CreateFontW(
                        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hTextFont);

                    RECT textRect;
                    textRect.left = rect.left;
                    textRect.top = rect.top;
                    textRect.right = rect.left + bm.bmWidth;
                    textRect.bottom = rect.top + bm.bmHeight;

                    SetBkMode(hdc, TRANSPARENT);

                    // Shadow text | Texte ombre
                    SetTextColor(hdc, RGB(0, 0, 0));
                    RECT shadowRect = textRect;
                    OffsetRect(&shadowRect, 1, 1);
                    DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    // Main text | Texte principal
                    SetTextColor(hdc, RGB(240, 240, 240));
                    DrawTextW(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(hdc, hOldFont);
                    if (!hFont) DeleteObject(hTextFont);
                }
            }

            return TRUE;
        }

        // Draw Set Key buttons with Patch Configuration style | Dessiner les boutons Set Key avec le style de Patch Configuration
        if (ctrlId == 101 || ctrlId == 102 || ctrlId == 103 || ctrlId == 104 || ctrlId == 106) {
            HDC hdc = lpDIS->hDC;
            RECT rect = lpDIS->rcItem;

            // Load bitmap resource | Charger la ressource bitmap
            HMODULE hModule = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)ConfigDialogProc, &hModule);

            HBITMAP hBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Key_Box_01),
                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

            if (hBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

                BITMAP bm;
                GetObject(hBitmap, sizeof(BITMAP), &bm);

                // Draw bitmap at real size (no stretch) | Dessiner le bitmap à taille réelle (pas de stretch)
                BitBlt(hdc,
                    rect.left,
                    rect.top,
                    bm.bmWidth,
                    bm.bmHeight,
                    hdcMem,
                    0, 0,
                    SRCCOPY);

                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
                DeleteObject(hBitmap);

                // Draw text with Patch Configuration style | Dessiner le texte avec le style de Patch Configuration
                wchar_t text[32] = L"";
                GetWindowTextW(lpDIS->hwndItem, text, 32);

                if (wcslen(text) > 0) {
                    HFONT hTextFont = NULL;
                    HFONT hOldFont = NULL;
                    if (hFont) {
                        hTextFont = hFont;
                        hOldFont = (HFONT)SelectObject(hdc, hTextFont);
                    }
                    else {
                        hTextFont = CreateFontW(
                            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                        hOldFont = (HFONT)SelectObject(hdc, hTextFont);
                    }

                    RECT textRect;
                    textRect.left = rect.left;
                    textRect.top = rect.top;
                    textRect.right = rect.left + bm.bmWidth;
                    textRect.bottom = rect.top + bm.bmHeight;

                    SetBkMode(hdc, TRANSPARENT);

                    // Shadow text | Texte ombre
                    SetTextColor(hdc, RGB(0, 0, 0));
                    RECT shadowRect = textRect;
                    OffsetRect(&shadowRect, 1, 1);
                    DrawTextW(hdc, text, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    // Main text | Texte principal
                    SetTextColor(hdc, RGB(240, 240, 240));
                    DrawTextW(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(hdc, hOldFont);
                    if (!hFontBold && hTextFont) {
                        DeleteObject(hTextFont);
                    }
                }
            }

            return TRUE;
        }

        // ✅ Dessiner l'image Path_Box AVEC TEXTE par-dessus
        if (ctrlId == 1100) {
            HDC hdc = lpDIS->hDC;
            RECT rect = lpDIS->rcItem;

            // Charger le bitmap
            HMODULE hModule = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)ConfigDialogProc, &hModule);

            HBITMAP hBitmap = (HBITMAP)LoadImageW(hModule, MAKEINTRESOURCEW(IDB_Path_Box),
                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

            int bmWidth = rect.right - rect.left;
            int bmHeight = rect.bottom - rect.top;

            if (hBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

                BITMAP bm;
                GetObject(hBitmap, sizeof(BITMAP), &bm);

                // utiliser les dimensions réelles du bitmap
                bmWidth = bm.bmWidth;
                bmHeight = bm.bmHeight;

                BitBlt(hdc,
                    rect.left, rect.top,
                    bm.bmWidth, bm.bmHeight,
                    hdcMem,
                    0, 0,
                    SRCCOPY);

                SelectObject(hdcMem, hOldBitmap);
                DeleteDC(hdcMem);
                DeleteObject(hBitmap);
            }

            if (wcslen(displayedPathText) > 0) {
                // Sélection de la police (préférer hPathFont)
                HFONT hTextFont = NULL;
                HFONT hOldFont = NULL;
                BOOL createdLocalFont = FALSE;
                if (hPathFont) {
                    hTextFont = hPathFont;
                    hOldFont = (HFONT)SelectObject(hdc, hTextFont);
                }
                else if (hFont) {
                    hTextFont = hFont;
                    hOldFont = (HFONT)SelectObject(hdc, hTextFont);
                }
                else {
                    hTextFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                    hOldFont = (HFONT)SelectObject(hdc, hTextFont);
                    createdLocalFont = TRUE;
                }

                SetBkMode(hdc, TRANSPARENT);

                // rectangle image exact
                RECT imageRect;
                imageRect.left = rect.left;
                imageRect.top = rect.top;
                imageRect.right = rect.left + bmWidth;
                imageRect.bottom = rect.top + bmHeight;

                const int horizMargin = 8;
                int availWidth = bmWidth - horizMargin * 2;
                if (availWidth < 1) availWidth = 1;

                // mesurer le texte actuel avec la police sélectionnée
                SIZE textSize = { 0, 0 };
                GetTextExtentPoint32W(hdc, displayedPathText, (int)wcslen(displayedPathText), &textSize);

                if (textSize.cx <= availWidth) {
                    // texte tient → on calcule une position exacte (pixel-perfect)
                    int textX = imageRect.left + (bmWidth - textSize.cx) / 2;
                    int textY = imageRect.top + (bmHeight - textSize.cy) / 2;

                    // Ombre
                    SetTextColor(hdc, RGB(0, 0, 0));
                    TextOutW(hdc, textX + 1, textY + 1, displayedPathText, (int)wcslen(displayedPathText));

                    // Texte principal (blanc)
                    SetTextColor(hdc, RGB(255, 255, 255));
                    TextOutW(hdc, textX, textY, displayedPathText, (int)wcslen(displayedPathText));
                }
                else {
                    // trop long → utiliser DrawText avec DT_END_ELLIPSIS et centrer verticalement
                    RECT dtRect = imageRect;
                    dtRect.left += horizMargin;
                    dtRect.right -= horizMargin;

                    // Calcule la hauteur du texte (DT_CALCRECT) pour centrer verticalement
                    RECT calcRect = dtRect;
                    DrawTextW(hdc, displayedPathText, -1, &calcRect, DT_SINGLELINE | DT_CALCRECT | DT_END_ELLIPSIS);

                    int textH = calcRect.bottom - calcRect.top;
                    if (textH <= 0) textH = (bmHeight / 2);

                    // Positionner verticalement au centre
                    int top = imageRect.top + (bmHeight - textH) / 2;
                    dtRect.top = top;
                    dtRect.bottom = top + textH;

                    // Ombre
                    RECT shadowRect = dtRect;
                    OffsetRect(&shadowRect, 1, 1);
                    SetTextColor(hdc, RGB(0, 0, 0));
                    DrawTextW(hdc, displayedPathText, -1, &shadowRect,
                        DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

                    // Texte principal (blanc)
                    SetTextColor(hdc, RGB(255, 255, 255));
                    DrawTextW(hdc, displayedPathText, -1, &dtRect,
                        DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
                }

                // restaurer police et nettoyer si nécessaire
                SelectObject(hdc, hOldFont);
                if (createdLocalFont && hTextFont) {
                    DeleteObject(hTextFont);
                }
            }

            return TRUE;
        }

        // Dessiner les boutons (code existant)
        if (lpDIS->CtlType == ODT_BUTTON) {
            int ctrlId = lpDIS->CtlID;
            if (ctrlId != 201 && ctrlId != 203 && ctrlId != 204) {
                DrawButtonWithBitmap(lpDIS);
                return TRUE;
            }
        }
        break;
    }

    case WM_DESTROY:
        if (hFont) DeleteObject(hFont);
        if (hFontBold) DeleteObject(hFontBold);
        if (hFontLarge) DeleteObject(hFontLarge);
        if (hFontEmoji) DeleteObject(hFontEmoji);
        if (hPathFont) { DeleteObject(hPathFont); hPathFont = NULL; }

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}

// Show configuration interface centered on screen | Afficher l'interface de configuration centrée à l'écran
static int showConfigInterface() {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Function started");
    }

    readConfigurationSettings();
    retrieveServerMaximumAudioDistance(TRUE);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Configuration settings read");
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        if (enableLogGeneral) {
            char errorMsg[128];
            snprintf(errorMsg, sizeof(errorMsg), "showConfigInterface: COM initialization failed with HRESULT: 0x%08X", hr);
            mumbleAPI.log(ownID, errorMsg);
        }
        MessageBoxW(NULL, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        isConfigDialogOpen = FALSE;
        return -1;
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: COM initialized successfully");
    }

    const wchar_t CONFIG_CLASS_NAME[] = L"ModernConfigClass";

    // Charger l'image de fond depuis la ressource AVANT d'enregistrer la classe
    if (!hBackgroundBitmap) {
        hBackgroundBitmap = LoadBackgroundFromResource(IDB_BACKGROUND);
        if (hBackgroundBitmap && enableLogGeneral) {
            mumbleAPI.log(ownID, "Background bitmap loaded for class background");
        }
    }

    // ✅ IMPORTANT : Utiliser NULL_BRUSH pour empêcher Windows de peindre le fond
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ConfigDialogProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = CONFIG_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    UnregisterClassW(CONFIG_CLASS_NAME, wc.hInstance);

    ATOM classAtom = RegisterClassW(&wc);
    if (classAtom == 0) {
        DWORD error = GetLastError();
        if (enableLogGeneral) {
            char errorMsg[128];
            snprintf(errorMsg, sizeof(errorMsg), "showConfigInterface: RegisterClassW failed with error: %lu", error);
            mumbleAPI.log(ownID, errorMsg);
        }
        CoUninitialize();
        isConfigDialogOpen = FALSE;
        return -1;
    }

    // Get screen dimensions without affecting mouse | Obtenir les dimensions de l'écran sans affecter la souris
    RECT desktopRect;
    GetWindowRect(GetDesktopWindow(), &desktopRect);

    int windowWidth = 600;
    int windowHeight = 780;

    // Center on main screen | Centrer sur l'écran principal
    int windowX = (desktopRect.right - desktopRect.left - windowWidth) / 2;
    int windowY = (desktopRect.bottom - desktopRect.top - windowHeight) / 2;

    if (windowX < 10) windowX = 10;
    if (windowY < 10) windowY = 10;

    if (enableLogGeneral) {
        char posMsg[256];
        snprintf(posMsg, sizeof(posMsg), "showConfigInterface: Positioning window at screen center - Window: (%d,%d)",
            windowX, windowY);
        mumbleAPI.log(ownID, posMsg);
    }

    hConfigDialog = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CONFIG_CLASS_NAME,
        L"\U0001F3AE Plugin Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX, windowY, windowWidth, windowHeight,
        NULL, NULL, wc.hInstance, NULL);

    if (!hConfigDialog) {
        DWORD error = GetLastError();
        if (enableLogGeneral) {
            char errorMsg[128];
            snprintf(errorMsg, sizeof(errorMsg), "showConfigInterface: CreateWindowExW failed with error: %lu", error);
            mumbleAPI.log(ownID, errorMsg);
        }
        CoUninitialize();
        isConfigDialogOpen = FALSE;
        return -1;
    }

    if (enableLogGeneral) {
        char msg[128];
        snprintf(msg, sizeof(msg), "showConfigInterface: Window created successfully, hWnd = 0x%p", hConfigDialog);
        mumbleAPI.log(ownID, msg);
    }

    SetLayeredWindowAttributes(hConfigDialog, 0, 255, LWA_ALPHA);

    InvalidateRect(hConfigDialog, NULL, TRUE);
    RedrawWindow(hConfigDialog, NULL, NULL,
        RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    UpdateWindow(hConfigDialog);

    // Force window to foreground without affecting mouse | Forcer la fenêtre au premier plan sans affecter la souris
    forceWindowToForegroundNoMouse(hConfigDialog);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Window positioned at screen center and forced to foreground");
    }

    SetTimer(hConfigDialog, 1, 50, NULL);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Timer set, entering message loop");
    }

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Message loop exited");
    }

    CoUninitialize();
    isConfigDialogOpen = FALSE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Function completed successfully");
    }

    return 0;
}

// Path selection dialog thread | Thread pour la boîte de dialogue de sélection de chemin
void showPathSelectionDialogThread(void* arg) {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showPathSelectionDialogThread: Thread started");
    }

    int result = showConfigInterface();

    if (enableLogGeneral) {
        char msg[128];
        snprintf(msg, sizeof(msg), "showPathSelectionDialogThread: Thread finished with result: %d", result);
        mumbleAPI.log(ownID, msg);
    }
}


// ============================================================================
// MODULE 15 : INTERFACE - MESSAGES
// ============================================================================

// Update distance message in real time | Fonction pour mettre à jour les messages de distance en temps réel
static void updateDistanceMessage(HWND hMessageControl, float currentValue, float minimum, float maximum, const char* modeName) {
    if (!hMessageControl || !IsWindow(hMessageControl)) return;

    SetWindowTextW(hMessageControl, L"");
    InvalidateRect(hMessageControl, NULL, TRUE);

    wchar_t message[256] = L"";

    if (!shouldApplyDistanceLimits()) {
        swprintf(message, 256, L"INFO: %S distance: %.1f (No server limits - free range)", modeName, currentValue);
    }
    else if (currentValue < minimum) {
        swprintf(message, 256, L"WARNING: %S: %.1f too low (min: %.1f) - auto-corrected", modeName, currentValue, minimum);
    }
    else if (currentValue > maximum) {
        swprintf(message, 256, L"WARNING: %S: %.1f too high (max: %.1f) - auto-corrected", modeName, currentValue, maximum);
    }

    SetWindowTextW(hMessageControl, message);
    ShowWindow(hMessageControl, SW_SHOW);
    UpdateWindow(hMessageControl);
}

// Update distance muting message | Fonction pour mettre à jour le message de muting
static void updateDistanceMutingMessage() {
    if (!hDistanceMutingMessage || !IsWindow(hDistanceMutingMessage)) return;

    SetWindowTextW(hDistanceMutingMessage, L"");

    wchar_t message[256] = L"";

    if (!shouldApplyDistanceLimits()) {
        if (enableDistanceMuting) {
            swprintf(message, 256, L"INFO: Distance-based muting: Enabled (No server restrictions)");
        }
        else {
            swprintf(message, 256, L"INFO: Distance-based muting: Disabled (No server restrictions)");
        }
    }
    else if (hubForceDistanceBasedMuting) {
        if (enableDistanceMuting) {
            swprintf(message, 256, L"LOCKED: Distance-based muting: FORCED by server (cannot disable)");
        }
        else {
            swprintf(message, 256, L"LOCKED: Distance-based muting: FORCED by server - enabling automatically");
        }
    }
    else {
        if (enableDistanceMuting) {
            swprintf(message, 256, L"OK: Distance-based muting: Enabled (user choice)");
        }
        else {
            swprintf(message, 256, L"INFO: Distance-based muting: Disabled (user choice)");
        }
    }

    InvalidateRect(hDistanceMutingMessage, NULL, TRUE);
    SetWindowTextW(hDistanceMutingMessage, message);
    ShowWindow(hDistanceMutingMessage, SW_SHOW);
    UpdateWindow(hDistanceMutingMessage);
}

static void updatePositionalAudioMessage() {
    if (!hPositionalAudioMessage || !IsWindow(hPositionalAudioMessage)) return;

    SetWindowTextW(hPositionalAudioMessage, L"");
    InvalidateRect(hPositionalAudioMessage, NULL, TRUE);

    wchar_t message[400] = L"";

    if (!shouldApplyDistanceLimits()) {
        if (enableAutoAudioSettings) {
            swprintf(message, 400, L"INFO: Positional audio: Enabled (No server restrictions)");
        }
        else {
            swprintf(message, 400, L"INFO: Positional audio: Disabled (No server restrictions)");
        }
    }
    else if (hubForcePositionalAudio) {
        if (enableAutoAudioSettings) {
            swprintf(message, 400,
                L"ACTIVE: Positional audio FORCED - MinDist=%.1f MaxDist=%.1f MaxVol=%.0f%% Bloom=%.1f FilterIntensity=%.2f",
                hubAudioMinDistance, hubAudioMaxDistance, hubAudioMaxVolume, hubAudioBloom, hubAudioFilterIntensity);
        }
        else {
            swprintf(message, 400, L"LOCKED: Positional audio: FORCED by server - enabling automatically");
        }
    }
    else {
        if (enableAutoAudioSettings) {
            swprintf(message, 400,
                L"OK: Positional audio enabled - MinDist=%.1f MaxDist=%.1f MaxVol=%.0f%% Bloom=%.1f FilterIntensity=%.2f",
                hubAudioMinDistance, hubAudioMaxDistance, hubAudioMaxVolume, hubAudioBloom, hubAudioFilterIntensity);
        }
        else {
            swprintf(message, 400, L"INFO: Positional audio: Disabled (user choice)");
        }
    }

    SetWindowTextW(hPositionalAudioMessage, message);
    ShowWindow(hPositionalAudioMessage, SW_SHOW);
    UpdateWindow(hPositionalAudioMessage);
}

static void updateChannelSwitchingMessage() {
    if (!hChannelSwitchingMessage || !IsWindow(hChannelSwitchingMessage)) return;

    SetWindowTextW(hChannelSwitchingMessage, L"");

    wchar_t message[256] = L"";

    if (!shouldApplyDistanceLimits()) {
        if (enableAutomaticChannelChange) {
            swprintf(message, 256, L"INFO: Automatic channel switching: Enabled (No server restrictions)");
        }
        else {
            swprintf(message, 256, L"INFO: Automatic channel switching: Disabled (No server restrictions)");
        }
    }
    else if (hubForceAutomaticChannelSwitching) {
        if (enableAutomaticChannelChange) {
            swprintf(message, 256, L"LOCKED: Automatic channel switching: FORCED by server (cannot disable)");
        }
        else {
            swprintf(message, 256, L"LOCKED: Automatic channel switching: FORCED by server - enabling automatically");
        }
    }
    else {
        if (enableAutomaticChannelChange) {
            swprintf(message, 256, L"OK: Automatic channel switching: Enabled (user choice)");
        }
        else {
            swprintf(message, 256, L"INFO: Automatic channel switching: Disabled (user choice)");
        }
    }

    InvalidateRect(hChannelSwitchingMessage, NULL, TRUE);
    SetWindowTextW(hChannelSwitchingMessage, message);
    ShowWindow(hChannelSwitchingMessage, SW_SHOW);
    UpdateWindow(hChannelSwitchingMessage);
}


// Create consolidated message for each mode | Fonction pour créer un message consolidé unique pour chaque mode
static void updateConsolidatedDistanceMessages() {
    if (isUpdatingInterface) return;
    if (!hConfigDialog || !IsWindow(hConfigDialog)) return;

    BOOL limitsActive = shouldApplyDistanceLimits();

    // Whisper consolidated message | Message whisper consolidé
    if (hDistanceWhisperMessage && IsWindow(hDistanceWhisperMessage)) {
        SetWindowTextW(hDistanceWhisperMessage, L"");
        InvalidateRect(hDistanceWhisperMessage, NULL, TRUE);

        wchar_t whisperMsg[300] = L"";

        if (!limitsActive) {
            swprintf(whisperMsg, 300, L"Whisper: %.1f meters (Free range - no server limits)", distanceWhisper);
        }
        else {
            if (distanceWhisper < hubMinimumWhisper) {
                swprintf(whisperMsg, 300, L"Whisper: %.1f→%.1f meters (Auto-corrected: below minimum %.1f)",
                    distanceWhisper, (float)hubMinimumWhisper, (float)hubMinimumWhisper);
            }
            else if (distanceWhisper > hubMaximumWhisper) {
                swprintf(whisperMsg, 300, L"Whisper: %.1f→%.1f meters (Auto-corrected: above maximum %.1f)",
                    distanceWhisper, (float)hubMaximumWhisper, (float)hubMaximumWhisper);
            }
            else {
                swprintf(whisperMsg, 300, L"Whisper: %.1f meters (Valid range: %.1f-%.1f)",
                    distanceWhisper, (float)hubMinimumWhisper, (float)hubMaximumWhisper);
            }
        }

        SetWindowTextW(hDistanceWhisperMessage, whisperMsg);
        ShowWindow(hDistanceWhisperMessage, SW_SHOW);
        UpdateWindow(hDistanceWhisperMessage);
    }

    // Normal consolidated message | Message normal consolidé
    if (hDistanceNormalMessage && IsWindow(hDistanceNormalMessage)) {
        SetWindowTextW(hDistanceNormalMessage, L"");
        InvalidateRect(hDistanceNormalMessage, NULL, TRUE);

        wchar_t normalMsg[300] = L"";

        if (!limitsActive) {
            swprintf(normalMsg, 300, L" Normal: %.1f meters (Free range - no server limits)", distanceNormal);
        }
        else {
            if (distanceNormal < hubMinimumNormal) {
                swprintf(normalMsg, 300, L"Normal: %.1f→%.1f meters (Auto-corrected: below minimum %.1f)",
                    distanceNormal, (float)hubMinimumNormal, (float)hubMinimumNormal);
            }
            else if (distanceNormal > hubMaximumNormal) {
                swprintf(normalMsg, 300, L"Normal: %.1f→%.1f meters (Auto-corrected: above maximum %.1f)",
                    distanceNormal, (float)hubMaximumNormal, (float)hubMaximumNormal);
            }
            else {
                swprintf(normalMsg, 300, L"Normal: %.1f meters (Valid range: %.1f-%.1f)",
                    distanceNormal, (float)hubMinimumNormal, (float)hubMaximumNormal);
            }
        }

        SetWindowTextW(hDistanceNormalMessage, normalMsg);
        ShowWindow(hDistanceNormalMessage, SW_SHOW);
        UpdateWindow(hDistanceNormalMessage);
    }

    // Shout consolidated message | Message shout consolidé
    if (hDistanceShoutMessage && IsWindow(hDistanceShoutMessage)) {
        SetWindowTextW(hDistanceShoutMessage, L"");
        InvalidateRect(hDistanceShoutMessage, NULL, TRUE);

        wchar_t shoutMsg[300] = L"";

        if (!limitsActive) {
            swprintf(shoutMsg, 300, L" Shout: %.1f meters (Free range - no server limits)", distanceShout);
        }
        else {
            if (distanceShout < hubMinimumShout) {
                swprintf(shoutMsg, 300, L"Shout: %.1f→%.1f meters (Auto-corrected: below minimum %.1f)",
                    distanceShout, (float)hubMinimumShout, (float)hubMinimumShout);
            }
            else if (distanceShout > hubMaximumShout) {
                swprintf(shoutMsg, 300, L"Shout: %.1f→%.1f meters (Auto-corrected: above maximum %.1f)",
                    distanceShout, (float)hubMaximumShout, (float)hubMaximumShout);
            }
            else {
                swprintf(shoutMsg, 300, L"Shout: %.1f meters (Valid range: %.1f-%.1f)",
                    distanceShout, (float)hubMinimumShout, (float)hubMaximumShout);
            }
        }

        SetWindowTextW(hDistanceShoutMessage, shoutMsg);
        ShowWindow(hDistanceShoutMessage, SW_SHOW);
        UpdateWindow(hDistanceShoutMessage);
    }
}

// Display server limits separately | Fonction pour afficher les limites serveur distinctement
static void updateServerLimitMessages() {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "DEBUG: updateServerLimitMessages() FUNCTION CALLED SUCCESSFULLY!");
    }

    if (isUpdatingInterface) return;
    if (!hConfigDialog || !IsWindow(hConfigDialog)) return;

    BOOL limitsActive = shouldApplyDistanceLimits();

    if (enableLogGeneral) {
        char debugMsg[128];
        snprintf(debugMsg, sizeof(debugMsg), "DEBUG: updateServerLimitMessages - limitsActive = %s",
            limitsActive ? "TRUE" : "FALSE");
        mumbleAPI.log(ownID, debugMsg);
    }
}

// Display status message in interface | Afficher un message de statut dans l'interface
static void showStatusMessage(const wchar_t* message, BOOL isError) {
    if (hStatusMessage) {
        SetWindowTextW(hStatusMessage, message);

        // Change color based on message type | Changer la couleur selon le type de message
        if (isError) {
            SendMessage(hStatusMessage, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hStatusMessage), (LPARAM)hStatusMessage);
        }
        else {
            SendMessage(hStatusMessage, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hStatusMessage), (LPARAM)hStatusMessage);
        }

        SetTimer(hConfigDialog, 2, 5000, NULL);
    }
}

// Clear status message | Effacer le message de statut
static void clearStatusMessage() {
    if (hStatusMessage) {
        SetWindowTextW(hStatusMessage, L"");
    }
}

// Generate dynamic distance limit message | Générer un message dynamique sur les limites de distance
static void showDynamicDistanceLimitMessage() {
    if (!maxAudioDistanceRetrieved) {
        retrieveServerMaximumAudioDistance(FALSE);
    }

    char dynamicMsg[1024];
    snprintf(dynamicMsg, sizeof(dynamicMsg),
        "Voice distance information:\n"
        "Server Maximum Audio Distance: %.1f meters\n"
        "Current Settings:\n"
        "  • Whisper: %.1f meters %s\n"
        "  • Normal: %.1f meters %s\n"
        "  • Shout: %.1f meters %s\n"
        "Note: Distances are automatically limited by server settings. "
        "Each server may have different maximum distances.",
        serverMaximumAudioDistance,
        distanceWhisper, (distanceWhisper == serverMaximumAudioDistance) ? "(LIMITED)" : "",
        distanceNormal, (distanceNormal == serverMaximumAudioDistance) ? "(LIMITED)" : "",
        distanceShout, (distanceShout == serverMaximumAudioDistance) ? "(LIMITED)" : "");

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, dynamicMsg);
    }
}

// ============================================================================
// MODULE 16 : INTERFACE - GESTION DYNAMIQUE
// ============================================================================

// Handle distance edit changes with smart filtering | Gérer les changements d'édition de distance avec filtrage intelligent
static void handleDistanceEditChange(int editId) {
    if (isUpdatingInterface) return;

    HWND hEdit = NULL;
    float* targetDistance = NULL;
    const char* modeName = "";

    switch (editId) {
    case 1: // Whisper
        hEdit = hDistanceWhisperEdit;
        targetDistance = &distanceWhisper;
        modeName = "Whisper";
        break;
    case 2: // Normal
        hEdit = hDistanceNormalEdit;
        targetDistance = &distanceNormal;
        modeName = "Normal";
        break;
    case 3: // Shout
        hEdit = hDistanceShoutEdit;
        targetDistance = &distanceShout;
        modeName = "Shout";
        break;
    default:
        return;
    }

    if (!hEdit || !targetDistance) return;

    wchar_t text[32];
    GetWindowTextW(hEdit, text, 32);
    float newValue = (float)_wtof(text);

    // Validate value with filtering | Valider la valeur avec filtrage
    if (newValue > 0) {
        BOOL valueChanged = FALSE;
        float correctedValue = newValue;

        // Apply digit filter if server limits are active | Appliquer le filtre de chiffres si les limites serveur sont actives
        if (shouldApplyDistanceLimits()) {
            float minimum, maximum;

            switch (editId) {
            case 1: // Whisper
                minimum = (float)hubMinimumWhisper;
                maximum = (float)hubMaximumWhisper;
                break;
            case 2: // Normal
                minimum = (float)hubMinimumNormal;
                maximum = (float)hubMaximumNormal;
                break;
            case 3: // Shout
                minimum = (float)hubMinimumShout;
                maximum = (float)hubMaximumShout;
                break;
            default:
                return;
            }

            // Check if we have enough digits before validating | Vérifier si on a assez de chiffres avant de valider
            if (!shouldValidateValue(newValue, minimum, maximum, modeName)) {
                return;
            }

            correctedValue = validateDistanceValue(newValue, minimum, maximum, modeName);

            // Update field if value was corrected | Mettre à jour le champ si la valeur a été corrigée
            if (correctedValue != newValue) {
                isUpdatingInterface = TRUE;

                wchar_t correctedText[32];
                swprintf(correctedText, 32, L"%.1f", correctedValue);
                SetWindowTextW(hEdit, correctedText);

                SendMessage(hEdit, EM_SETSEL, wcslen(correctedText), wcslen(correctedText));

                isUpdatingInterface = FALSE;
                valueChanged = TRUE;

                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "CORRECTED: %s %.1f -> %.1f (server limits)",
                        modeName, newValue, correctedValue);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
        else {
            // No server limits - accept values with at least 1 digit | Pas de limites serveur - accepter les valeurs avec au moins 1 chiffre
            if (countSignificantDigits(newValue) < 1) {
                return;
            }
        }

        // Apply corrected or original value | Appliquer la valeur corrigée ou originale
        if (correctedValue != *targetDistance) {
            *targetDistance = correctedValue;
            valueChanged = TRUE;

            if (enableLogGeneral) {
                char changeMsg[128];
                snprintf(changeMsg, sizeof(changeMsg), "Distance changed: %s = %.1f", modeName, correctedValue);
                mumbleAPI.log(ownID, changeMsg);
            }
        }

        // Save and update interface if necessary | Sauvegarder et mettre à jour l'interface si nécessaire
        if (valueChanged) {
            saveVoiceSettings();
            updateDynamicInterface();
            forceInterfaceRefresh();
        }
    }
}


// Update dynamic interface | Mettre à jour l'interface dynamique
static void updateDynamicInterface() {
    if (isUpdatingInterface) return;
    if (!hConfigDialog || !IsWindow(hConfigDialog)) return;

    if (currentCategory != 2) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "updateDynamicInterface: Not in category 2 - skipping update");
        }
        return;
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "DEBUG: updateDynamicInterface() CALLED!");
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "DEBUG: updateDynamicInterface() CALLED!");
    }

    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastInterfaceUpdate < 100) return;
    lastInterfaceUpdate = currentTime;

    isUpdatingInterface = TRUE;

    // Force distance-based muting if required | Forcer le muting basé sur la distance si nécessaire
    if (hubForceDistanceBasedMuting && !enableDistanceMuting) {
        enableDistanceMuting = TRUE;
        if (hEnableDistanceMutingCheck) {
            CheckDlgButton(hConfigDialog, 201, BST_CHECKED);
        }
    }

    // Update distance muting checkbox state | Mettre à jour l'état de la checkbox de muting
    if (hEnableDistanceMutingCheck) {
        BOOL shouldDisable = hubDescriptionAvailable && hubForceDistanceBasedMuting;
        EnableWindow(hEnableDistanceMutingCheck, !shouldDisable);

        if (enableLogGeneral) {
            char debugMsg[256];
            snprintf(debugMsg, sizeof(debugMsg),
                "Distance muting checkbox: hubDescriptionAvailable=%s, hubForceDistanceBasedMuting=%s, shouldDisable=%s",
                hubDescriptionAvailable ? "TRUE" : "FALSE",
                hubForceDistanceBasedMuting ? "TRUE" : "FALSE",
                shouldDisable ? "TRUE" : "FALSE");
            mumbleAPI.log(ownID, debugMsg);
        }
    }

    // Force automatic channel change if required | Forcer le changement automatique de canal si nécessaire
    if (hubForceAutomaticChannelSwitching && !enableAutomaticChannelChange) {
        enableAutomaticChannelChange = TRUE;
        if (hEnableAutomaticChannelChangeCheck) {
            CheckDlgButton(hConfigDialog, 203, BST_CHECKED);
        }
    }

    // Update channel switching checkbox state | Mettre à jour l'état de la checkbox de changement de canal
    if (hEnableAutomaticChannelChangeCheck) {
        BOOL shouldDisable = hubDescriptionAvailable && hubForceAutomaticChannelSwitching;
        EnableWindow(hEnableAutomaticChannelChangeCheck, !shouldDisable);

        if (enableLogGeneral) {
            char debugMsg[256];
            snprintf(debugMsg, sizeof(debugMsg),
                "Channel switching checkbox: hubDescriptionAvailable=%s, hubForceAutomaticChannelSwitching=%s, shouldDisable=%s",
                hubDescriptionAvailable ? "TRUE" : "FALSE",
                hubForceAutomaticChannelSwitching ? "TRUE" : "FALSE",
                shouldDisable ? "TRUE" : "FALSE");
            mumbleAPI.log(ownID, debugMsg);
        }
    }

    // Force positional audio if required | Forcer l'audio positionnel si nécessaire
    if (hubForcePositionalAudio && !enableAutoAudioSettings) {
        enableAutoAudioSettings = TRUE;
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Hub: Positional audio FORCED by server - enabling automatically");
        }
    }

    updateConsolidatedDistanceMessages();

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "DEBUG: About to call updateServerLimitMessages()");
    }

    updateServerLimitMessages();
    updateDistanceMutingMessage();
    updateChannelSwitchingMessage();
    updatePositionalAudioMessage();

    float newWhisper = distanceWhisper;
    float newNormal = distanceNormal;
    float newShout = distanceShout;

    // Apply server limits if distance-based muting is forced | Appliquer les limites serveur si le muting basé sur la distance est forcé
    if (shouldApplyDistanceLimits()) {
        newWhisper = validateDistanceValue(distanceWhisper, (float)hubMinimumWhisper, (float)hubMaximumWhisper, "Whisper");
        newNormal = validateDistanceValue(distanceNormal, (float)hubMinimumNormal, (float)hubMaximumNormal, "Normal");
        newShout = validateDistanceValue(distanceShout, (float)hubMinimumShout, (float)hubMaximumShout, "Shout");
    }
    else {
        // User has full control over distances | L'utilisateur a le contrôle total sur les distances
        if (enableLogGeneral) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "USER FREEDOM: Using and SAVING user-defined distances - Whisper: %.1f, Normal: %.1f, Shout: %.1f (no server limits)",
                distanceWhisper, distanceNormal, distanceShout);
            mumbleAPI.log(ownID, logMsg);
        }
    }

    BOOL distanceChanged = FALSE;

    if (newWhisper != distanceWhisper) {
        distanceWhisper = newWhisper;
        distanceChanged = TRUE;
        if (hDistanceWhisperEdit) {
            wchar_t whisperText[32];
            swprintf(whisperText, 32, L"%.1f", distanceWhisper);
            SetWindowTextW(hDistanceWhisperEdit, whisperText);
        }
    }

    if (newNormal != distanceNormal) {
        distanceNormal = newNormal;
        distanceChanged = TRUE;
        if (hDistanceNormalEdit) {
            wchar_t normalText[32];
            swprintf(normalText, 32, L"%.1f", distanceNormal);
            SetWindowTextW(hDistanceNormalEdit, normalText);
        }
    }

    if (newShout != distanceShout) {
        distanceShout = newShout;
        distanceChanged = TRUE;
        if (hDistanceShoutEdit) {
            wchar_t shoutText[32];
            swprintf(shoutText, 32, L"%.1f", distanceShout);
            SetWindowTextW(hDistanceShoutEdit, shoutText);
        }
    }

    // Always save distance changes | Toujours sauvegarder les changements de distance
    if (distanceChanged) {
        saveVoiceSettings();
        applyDistanceToAllPlayers();

        if (enableLogGeneral) {
            char saveMsg[256];
            snprintf(saveMsg, sizeof(saveMsg),
                "Distances saved to config: Whisper=%.1f, Normal=%.1f, Shout=%.1f (ForceDistanceBasedMuting=%s)",
                distanceWhisper, distanceNormal, distanceShout,
                hubForceDistanceBasedMuting ? "TRUE" : "FALSE");
            mumbleAPI.log(ownID, saveMsg);
        }
    }

    // Force redraw of all messages | Forcer le redessin de tous les messages
    if (hDistanceMutingMessage) {
        InvalidateRect(hDistanceMutingMessage, NULL, TRUE);
        UpdateWindow(hDistanceMutingMessage);
    }
    if (hChannelSwitchingMessage) {
        InvalidateRect(hChannelSwitchingMessage, NULL, TRUE);
        UpdateWindow(hChannelSwitchingMessage);
    }
    if (hPositionalAudioMessage) {
        InvalidateRect(hPositionalAudioMessage, NULL, TRUE);
        UpdateWindow(hPositionalAudioMessage);
    }

    isUpdatingInterface = FALSE;
}

static void forceInterfaceRefresh() {
    if (!hConfigDialog || !IsWindow(hConfigDialog)) return;
    if (currentCategory != 2) return;

    // Force immediate message updates | Forcer la mise à jour immédiate des messages
    updateConsolidatedDistanceMessages();
    updateDistanceMutingMessage();
    updateChannelSwitchingMessage();
    updatePositionalAudioMessage();

    // Force redraw of all messages | Forcer le redessin de tous les messages
    if (hDistanceWhisperMessage) {
        InvalidateRect(hDistanceWhisperMessage, NULL, TRUE);
        UpdateWindow(hDistanceWhisperMessage);
    }
    if (hDistanceNormalMessage) {
        InvalidateRect(hDistanceNormalMessage, NULL, TRUE);
        UpdateWindow(hDistanceNormalMessage);
    }
    if (hDistanceShoutMessage) {
        InvalidateRect(hDistanceShoutMessage, NULL, TRUE);
        UpdateWindow(hDistanceShoutMessage);
    }
}

// ============================================================================
// MODULE 17 : SURVEILLANCE DES TOUCHES
// ============================================================================

// Key monitoring thread | Thread de surveillance des touches
static void keyMonitorThreadFunction(void* arg) {
    keyMonitorThreadRunning = TRUE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Key monitor thread: Started with ultra-reactive detection");
    }

    while (keyMonitorThreadRunning) {
        BOOL currentKeyState = (GetAsyncKeyState(configUIKey) & 0x8000) != 0;

        // Rising edge detection | Détection de front montant
        if (currentKeyState && !lastKeyState) {
            if (!isConfigDialogOpen) {
                isConfigDialogOpen = TRUE;

                if (enableLogGeneral) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "KEY INSTANT-DETECTED! %s (VK:%d) - opening interface immediately...",
                        getKeyName(configUIKey), configUIKey);
                    mumbleAPI.log(ownID, msg);
                }

                _beginthread(showPathSelectionDialogThread, 0, NULL);
            }
        }

        lastKeyState = currentKeyState;
        Sleep(50);
    }

    keyMonitorThreadRunning = FALSE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Key monitor thread: Stopped");
    }
}


// Start monitoring thread | Démarrer le thread de surveillance
static void startKeyMonitorThread() {
    if (!keyMonitorThreadRunning) {
        keyMonitorThread = (HANDLE)_beginthread(keyMonitorThreadFunction, 0, NULL);

        if (enableLogGeneral) {
            char msg[128];
            snprintf(msg, sizeof(msg), "Key monitor thread started for key: %s (VK:%d)",
                getKeyName(configUIKey), configUIKey);
            mumbleAPI.log(ownID, msg);
        }
    }
}


// Stop monitoring thread | Arrêter le thread de surveillance
static void stopKeyMonitorThread() {
    keyMonitorThreadRunning = FALSE;
    if (keyMonitorThread != NULL) {
        Sleep(500);
        keyMonitorThread = NULL;

        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Key monitor thread stopped");
        }
    }
}

// Install key monitoring | Installer la surveillance des touches
static void installKeyMonitoring() {
    if (enableLogGeneral) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Installing key monitoring for: %s (VK:%d)",
            getKeyName(configUIKey), configUIKey);
        mumbleAPI.log(ownID, msg);
    }

    startKeyMonitorThread();
}

// Remove key monitoring | Supprimer la surveillance des touches
static void removeKeyMonitoring() {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Removing key monitoring");
    }

    stopKeyMonitorThread();
}

// ============================================================================
// MODULE 18 : FICHIER MOD
// ============================================================================

// Check if mod file is active | Vérifier si le fichier mod est actif
static BOOL checkModFileActive() {
    // Verify file existence | Vérifier l'existence du fichier
    DWORD attributes = GetFileAttributesA(modFilePath);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }

    // Get file information | Obtenir les informations du fichier
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(modFilePath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    FindClose(hFind);

    ULARGE_INTEGER ull = { 0 };
    ull.LowPart = findFileData.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = findFileData.ftLastWriteTime.dwHighDateTime;
    time_t fileTime = (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);

    // File is active if modified within 5 seconds | Le fichier est actif s'il a été modifié dans les 5 secondes
    if (time(NULL) - fileTime <= 5) {
        return TRUE;
    }

    return FALSE;
}

// Read mod file data safely | Lire les données du fichier mod de manière sécurisée
static BOOL readModFileData(struct ModFileData* data) {
    if (!data) return FALSE;

    FILE* file = fopen(modFilePath, "rb");
    if (!file) {
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"ERREUR: Fichier introuvable. Re-vérifiez le chemin.");
        }
        return FALSE;
    }

    char buffer[256];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);

    if (bytesRead <= 0) {
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"ERREUR: Impossible de lire le contenu du fichier.");
        }
        return FALSE;
    }
    buffer[bytesRead] = '\0';

    if (enableLogModFile) {
        char logBuffer[300];
        snprintf(logBuffer, sizeof(logBuffer), u8"DEBUG: Contenu du fichier lu: %s", buffer);
        mumbleAPI.log(ownID, logBuffer);
    }

    char* endptr;
    data->valid = FALSE;

    // Universal format parsing | Parsing du format universel
    char* seq_ptr = strstr(buffer, "SEQ=");
    if (seq_ptr) {
        seq_ptr += 4;
        data->seq = (int)strtod(seq_ptr, &endptr);
        if (enableLogModFile) {
            char logBuffer[100];
            snprintf(logBuffer, sizeof(logBuffer), u8"DEBUG: SEQ lu: %d", data->seq);
            mumbleAPI.log(ownID, logBuffer);
        }
    }
    else {
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"ERREUR DE PARSING: 'SEQ=' non trouvé.");
        }
        return FALSE;
    }

    // Parse X coordinate | Parser la coordonnée X
    char* x_ptr = strstr(buffer, "X=");
    if (x_ptr) {
        x_ptr += 2;
        data->x = (float)strtod(x_ptr, &endptr);
        if (enableLogModFile) {
            char logBuffer[100];
            snprintf(logBuffer, sizeof(logBuffer), u8"DEBUG: X lu: %f", data->x);
            mumbleAPI.log(ownID, logBuffer);
        }
    }
    else {
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"ERREUR DE PARSING: 'X=' non trouvé.");
        }
        return FALSE;
    }

    // Parse Y coordinate | Parser la coordonnée Y
    char* y_ptr = strstr(buffer, "Y=");
    if (y_ptr) {
        y_ptr += 2;
        data->y = (float)strtod(y_ptr, &endptr);
        if (enableLogModFile) {
            char logBuffer[100];
            snprintf(logBuffer, sizeof(logBuffer), u8"DEBUG: Y lu: %f", data->y);
            mumbleAPI.log(ownID, logBuffer);
        }
    }
    else {
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"ERREUR DE PARSING: 'Y=' non trouvé.");
        }
        return FALSE;
    }

    // Parse Z coordinate | Parser la coordonnée Z
    char* z_ptr = strstr(buffer, "Z=");
    if (z_ptr) {
        z_ptr += 2;
        data->z = (float)strtod(z_ptr, &endptr);
        if (enableLogModFile) {
            char logBuffer[100];
            snprintf(logBuffer, sizeof(logBuffer), u8"DEBUG: Z lu: %f", data->z);
            mumbleAPI.log(ownID, logBuffer);
        }
    }
    else {
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"ERREUR DE PARSING: 'Z=' non trouvé.");
        }
        return FALSE;
    }

    // Parse YAW rotation | Parser la rotation YAW
    char* yaw_ptr = strstr(buffer, "YAW=");
    if (yaw_ptr) {
        yaw_ptr += 4;
        data->yaw = (float)strtod(yaw_ptr, &endptr);
        if (enableLogModFile) {
            char logBuffer[100];
            snprintf(logBuffer, sizeof(logBuffer), u8"DEBUG: YAW lu: %f", data->yaw);
            mumbleAPI.log(ownID, logBuffer);
        }
    }
    else {
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"ERREUR DE PARSING: 'YAW=' non trouvé.");
        }
        return FALSE;
    }

    // Parse YAWY rotation | Parser la rotation YAWY
    char* yawy_ptr = strstr(buffer, "YAWY=");
    if (yawy_ptr) {
        yawy_ptr += 5;
        data->yawY = (float)strtod(yawy_ptr, &endptr);
        if (enableLogModFile) {
            char logBuffer[100];
            snprintf(logBuffer, sizeof(logBuffer), u8"DEBUG: YAWY lu: %f", data->yawY);
            mumbleAPI.log(ownID, logBuffer);
        }
    }
    else {
        data->yawY = 0.0f;
        if (enableLogModFile) {
            mumbleAPI.log(ownID, u8"INFO DE PARSING: 'YAWY=' non trouvé. Utilisation de la valeur par défaut 0.0.");
        }
    }

    data->valid = TRUE;
    if (enableLogModFile) {
        mumbleAPI.log(ownID, u8"SUCCÈS: Toutes les données ont été lues avec le format universel.");
    }
    return TRUE;
}

static void modFileWatcherThread(void* arg) {

    while (enableGetPlayerCoordinates) {
        BOOL isModActive = checkModFileActive();
        BOOL newModDataRead = FALSE;

        if (isModActive) {
            struct ModFileData newData;
            if (readModFileData(&newData)) {
                if (newData.seq != lastSeq) {
                    currentModData = newData;
                    lastSeq = newData.seq;
                    newModDataRead = TRUE;
                }
            }
        }

        ULONGLONG currentTick = GetTickCount64();

        if (newModDataRead) {
            axe_x = currentModData.x;
            axe_y = currentModData.y;
            axe_z = currentModData.z;

            float yawRad = currentModData.yaw * 3.14159265f / 180.0f;
            avatarAxisX = -(float)cos(yawRad);
            avatarAxisY = sinf(-currentModData.yawY * 3.14159265f / 180.0f);
            avatarAxisZ = -(float)sin(yawRad);

            coordinatesValid = TRUE;
            lastModDataTick = currentTick;
        }
        else if (!isModActive) {
            coordinatesValid = FALSE;
        }

        Sleep(20);
    }
}

// ============================================================================
// MODULE 19 : THREADS SYSTÈME
// ============================================================================


// High-frequency voice system thread | Thread du système de voix à haute fréquence
static void voiceSystemThread(void* arg) {
    Sleep(2000);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "HIGH-FREQUENCY Voice system thread: Now active at 40ms intervals");
    }

    while (enableGetPlayerCoordinates) {
        if (strlen(localVoiceData.playerName) == 0) {
            getLocalPlayerName();
        }

        updateVoiceMode();

        if (enableLogGeneral) {
            static ULONGLONG lastDebugTime = 0;
            ULONGLONG currentTime = GetTickCount64();
            if (currentTime - lastDebugTime > 20000) {
                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg),
                    "HIGH-FREQ Voice system status - coordinatesValid: %s, playerCount: %zu",
                    coordinatesValid ? "TRUE" : "FALSE",
                    remotePlayerCount);
                mumbleAPI.log(ownID, debugMsg);
                lastDebugTime = currentTime;
            }
        }

        if (coordinatesValid && enableDistanceMuting) {
            sendCompletePositionalData();
        }

        Sleep(40);
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "HIGH-FREQUENCY Voice system thread: Stopped");
    }
}

// Force complete initialization | Forcer l'initialisation complète
static void forceCompleteInitialization() {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Force complete initialization starting...");
    }

    // Ensure all systems are ready | S'assurer que tous les systèmes sont prêts
    readConfigurationSettings();

    // Check if modFilePath is configured | Vérifier si modFilePath est configuré
    if (strlen(modFilePath) == 0) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "modFilePath empty - attempting auto-configuration");
        }

        wchar_t* configFolder = getConfigFolderPath();
        if (configFolder) {
            wchar_t configFile[MAX_PATH];
            swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);
            FILE* file = _wfopen(configFile, L"r");
            if (file) {
                wchar_t line[512];
                while (fgetws(line, 512, file)) {
                    if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                        wchar_t* pathStart = line + 10;
                        wchar_t* nl = wcschr(pathStart, L'\n');
                        if (nl) *nl = L'\0';
                        wchar_t* cr = wcschr(pathStart, L'\r');
                        if (cr) *cr = L'\0';

                        // Convert to modFilePath | Convertir en chemin modFilePath
                        size_t converted = 0;
                        wcstombs_s(&converted, modFilePath, MAX_PATH, pathStart, _TRUNCATE);
                        strcat_s(modFilePath, MAX_PATH, "\\Pos.txt");

                        if (enableLogGeneral) {
                            char msg[300];
                            snprintf(msg, sizeof(msg), "Auto-configured modFilePath: %s", modFilePath);
                            mumbleAPI.log(ownID, msg);
                        }
                        break;
                    }
                }
                fclose(file);
            }
        }
    }

    if (enableLogGeneral) {
        char msg[300];
        snprintf(msg, sizeof(msg), "Configuration status - modFilePath: %s, enableDistanceMuting: %s",
            modFilePath, enableDistanceMuting ? "TRUE" : "FALSE");
        mumbleAPI.log(ownID, msg);
    }
}

// ============================================================================
// MODULE 20 : CLEANUP
// ============================================================================

// Cleanup player mute states | Nettoyer les états de mute des joueurs
static void cleanupPlayerMuteStates() {
    playerMuteStateCount = 0;
    memset(playerMuteStates, 0, sizeof(playerMuteStates));
    lastDistanceCheck = 0;
}

// ============================================================================
// MODULE 21 : CALLBACKS MUMBLE (ordre chronologique)
// ============================================================================

PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onUserTalkingStateChanged(mumble_connection_t connection,
    mumble_userid_t userID,
    mumble_talking_state_t talkingState) {
    // Récupérer l'ID de l'utilisateur local
    mumble_userid_t localUserID;
    if (mumbleAPI.getLocalUserID(ownID, connection, &localUserID) != MUMBLE_STATUS_OK) {
        return; // Impossible de déterminer l'utilisateur local
    }

    // Vérifier si c'est l'utilisateur local qui parle
    if (userID != localUserID) {
        return; // Ignorer les autres utilisateurs
    }

    // Activer/désactiver la surbrillance uniquement pour soi-même
    if ((int)talkingState != 0) {
        setOverlayHighlightState(userID, connection, TRUE);
    }
    else {
        setOverlayHighlightState(userID, connection, FALSE);
    }
}


// Plugin initialization function | Fonction d'initialisation du plugin
mumble_error_t mumble_init(mumble_plugin_id_t pluginID) {
    hVoiceOverlay = NULL;
    hOverlayFont = NULL;
    overlayThreadRunning = FALSE;
    ownID = pluginID;
    setlocale(LC_NUMERIC, "C");

    lastAudioSettingsApply = 0;

    // Initialization | Initialisation
    lastFileCheck = 0;
    lastSeq = -1;
    modDataValid = FALSE;

    // Voice system initialization | Initialisation du système de voix
    memset(&localVoiceData, 0, sizeof(CompletePositionalData));
    localVoiceData.voiceDistance = distanceNormal;
    remotePlayerCount = 0;
    lastVoiceDataSent = 0;
    lastKeyCheck = 0;

    // Load voice distances from config | Charger les distances de voix depuis la configuration
    loadVoiceDistancesFromConfig();
    // Initialize voice presets | Initialiser les presets vocaux
    initializeVoicePresets();
    loadPresetsFromConfigFile();

    // Read and activate configuration | Lecture et activation automatique de la configuration
    readConfigurationSettings();

    // Auto-build modFilePath | Construire automatiquement le chemin modFilePath
    wchar_t* configFolder = getConfigFolderPath();
    if (configFolder) {
        wchar_t configFile[MAX_PATH];
        swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);
        FILE* file = _wfopen(configFile, L"r");
        if (file) {
            wchar_t line[512];
            while (fgetws(line, 512, file)) {
                if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                    wchar_t* pathStart = line + 10;
                    // Clean line breaks | Nettoyer les retours à la ligne
                    wchar_t* nl = wcschr(pathStart, L'\n');
                    if (nl) *nl = L'\0';
                    wchar_t* cr = wcschr(pathStart, L'\r');
                    if (cr) *cr = L'\0';

                    // Convert to modFilePath | Convertir en chemin modFilePath
                    size_t converted = 0;
                    wcstombs_s(&converted, modFilePath, MAX_PATH, pathStart, _TRUNCATE);
                    strcat_s(modFilePath, MAX_PATH, "\\Pos.txt");

                    if (enableLogGeneral) {
                        char msg[300];
                        snprintf(msg, sizeof(msg), "Auto-configured modFilePath: %s", modFilePath);
                        mumbleAPI.log(ownID, msg);
                    }
                    break;
                }
            }
            fclose(file);
        }
    }

    if (!isPatchAlreadySaved()) {
        _beginthread(showPathSelectionDialogThread, 0, NULL);
    }

    // Read saved path and convert to modFilePath | Lire le chemin sauvegardé et convertir en modFilePath
    wchar_t savedPath[MAX_PATH] = L"";
    wchar_t displayedPathText[MAX_PATH] = L"";
    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);
    FILE* file = _wfopen(configFile, L"r");
    if (file) {
        wchar_t line[512];
        while (fgetws(line, 512, file)) {
            if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                wcscpy_s(savedPath, MAX_PATH, line + 10);
                break;
            }
        }
        fclose(file);
    }

    installKeyMonitoring();

    if (enableLogGeneral) {
        char keyMsg[128];
        snprintf(keyMsg, sizeof(keyMsg), "Config UI key set to: %s (VK:%d)",
            getKeyName(configUIKey), configUIKey);
        mumbleAPI.log(ownID, keyMsg);
    }

    if (enableLogConfig) {
        char debugMsg[512];
        snprintf(debugMsg, sizeof(debugMsg), "Chemin configuré pour Pos.txt: %s", modFilePath);
        mumbleAPI.log(ownID, debugMsg);
    }

    // Force complete initialization | Forcer l'initialisation complète
    forceCompleteInitialization();

    // Start threads | Démarrer les threads
    _beginthread(modFileWatcherThread, 0, NULL);
    _beginthread(voiceSystemThread, 0, NULL);

    // CORRECTION: Start PERMANENT monitoring threads | Démarrer les threads de surveillance PERMANENTS
    _beginthread(channelManagementThread, 0, NULL);
    _beginthread(hubDescriptionMonitorThread, 0, NULL);

    // Create voice overlay if enabled | Créer l'overlay vocal si activé
    if (enableVoiceOverlay && enableDistanceMuting) {
        createVoiceOverlay();
        _beginthread(overlayMonitorThread, 0, NULL);
    }

    return MUMBLE_STATUS_OK;
}

// Register Mumble API functions | Enregistrer les fonctions de l'API Mumble
void mumble_registerAPIFunctions(void* apiStruct) {
    mumbleAPI = MUMBLE_API_CAST(apiStruct);
}

// Get plugin name | Obtenir le nom du plugin
struct MumbleStringWrapper mumble_getName() {
    static const char* name = u8"Conan_exiles";

    struct MumbleStringWrapper wrapper = { 0 };
    wrapper.data = name;
    wrapper.size = strlen(name);
    wrapper.needsReleasing = false;

    return wrapper;
}

// Get plugin version | Obtenir la version du plugin
mumble_version_t mumble_getVersion() {
    mumble_version_t version = { 0 };
    version.major = 5;
    version.minor = 4;
    version.patch = 3;

    return version;
}

// Get plugin author information | Obtenir les informations de l'auteur du plugin
struct MumbleStringWrapper mumble_getAuthor() {
    static const char* author = u8"Creator's Discord : Dino_Rex";

    struct MumbleStringWrapper wrapper = { 0 };
    wrapper.data = author;
    wrapper.size = strlen(author);
    wrapper.needsReleasing = false;

    return wrapper;
}

// Get plugin description | Obtenir la description du plugin
struct MumbleStringWrapper mumble_getDescription() {
    static const char* description = u8"By installing, you agree to the license terms available at: https://github.com/DinoRex22/Conan-Exiles-Mumble/blob/main/LICENSE | Discord: https://discord.gg/tFBbQzmDaZ";

    struct MumbleStringWrapper wrapper = { 0 };
    wrapper.data = description;
    wrapper.size = strlen(description);
    wrapper.needsReleasing = false;

    return wrapper;
}

uint32_t mumble_getFeatures() {
    return MUMBLE_FEATURE_AUDIO;
}

// Get API version | Obtenir la version de l'API
mumble_version_t mumble_getAPIVersion() {
    return MUMBLE_PLUGIN_API_VERSION;
}


// Return the plugin ID | Retourner l'ID du plugin
static mumble_plugin_id_t mumble_getPluginID() {
    return ownID;
}

PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerConnected(mumble_connection_t connection) {
    isConnectedToServer = TRUE;
    hubDescriptionAvailable = FALSE;
    hubLimitsActive = FALSE;

    channelManagementActive = FALSE;
    hubChannelID = -1;
    ingameChannelID = -1;
    lastTargetChannel = -1;
    lastValidChannel = -1;

    cleanupAudioVolumeStates();

    cleanupAdaptivePlayerStates();
    localPlayerPosition.x = localPlayerPosition.y = localPlayerPosition.z = 0.0f;
    cleanupPlayerMuteStates();
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    remotePlayerCount = 0;
    lastDistanceCheck = 0;

    maxAudioDistanceRetrieved = FALSE;
    retrieveServerMaximumAudioDistance(FALSE);
    applyMaximumDistanceLimits();

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "REAL adaptive volume system initialized - smooth real-time volume control active");
    }

    if (enableVoiceOverlay && enableDistanceMuting && hVoiceOverlay == NULL) {
        createVoiceOverlay();
    }
}

// Called when the client has finished synchronizing with the server | Appelé quand le client a fini de se synchroniser avec le serveur
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerSynchronized(mumble_connection_t connection) {
    if (TEMP) {
        mumbleAPI.log(ownID, "=== SERVER SYNCHRONIZED - APPLYING HUB SETTINGS IMMEDIATELY ===");
    }

    initializeChannelIDs();

    Sleep(1000);
    readHubDescription();

    if (hubDescriptionAvailable) {
        if (hubForceDistanceBasedMuting && !enableDistanceMuting) {
            enableDistanceMuting = TRUE;
            saveConfigurationChange("EnableDistanceMuting", L"true");
            if (TEMP) {
                mumbleAPI.log(ownID, "IMMEDIATE: Distance-based muting FORCED by server and SAVED");
            }
        }

        if (hubForceAutomaticChannelSwitching && !enableAutomaticChannelChange) {
            enableAutomaticChannelChange = TRUE;
            saveConfigurationChange("EnableAutomaticChannelChange", L"true");
            if (TEMP) {
                mumbleAPI.log(ownID, "IMMEDIATE: Automatic channel switching FORCED by server and SAVED");
            }
        }

        if (hubForcePositionalAudio && !enableAutoAudioSettings) {
            enableAutoAudioSettings = TRUE;
            if (TEMP) {
                mumbleAPI.log(ownID, "IMMEDIATE: Positional audio FORCED by server");
            }
        }

        validatePlayerDistances();

        if (hConfigDialog && IsWindow(hConfigDialog)) {
            CheckDlgButton(hConfigDialog, 201, enableDistanceMuting ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hConfigDialog, 203, enableAutomaticChannelChange ? BST_CHECKED : BST_UNCHECKED);
            updateDynamicInterface();
        }

        if (enableAutomaticChannelChange && channelManagementActive) {
            lastChannelCheck = 0;
            Sleep(500);
            manageChannelBasedOnCoordinates();
        }

        if (TEMP) {
            char applyMsg[512];
            snprintf(applyMsg, sizeof(applyMsg),
                "IMMEDIATE APPLICATION COMPLETE:\n"
                "  • AutoChannelChange: %s\n"
                "  • DistanceMuting: %s\n"
                "  • PositionalAudio: %s",
                enableAutomaticChannelChange ? "ENABLED" : "disabled",
                enableDistanceMuting ? "ENABLED" : "disabled",
                enableAutoAudioSettings ? "ENABLED" : "disabled");
            mumbleAPI.log(ownID, applyMsg);
        }
    }
}

// Receive complete positional data | Réception des données positionnelles complètes
PLUGIN_EXPORT bool PLUGIN_CALLING_CONVENTION mumble_onReceiveData(mumble_connection_t connection,
    mumble_userid_t sender,
    const uint8_t* data,
    size_t dataLength,
    const char* dataID) {

    // Detailed reception logging | Log détaillé de réception
    if (enableLogGeneral) {
        static ULONGLONG lastReceiveLog = 0;
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastReceiveLog > 1000) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "📦 RECEIVE: ID='%s' Size=%zu From=%u Time=%llu",
                dataID ? dataID : "NULL", dataLength, sender, currentTime);
            mumbleAPI.log(ownID, logMsg);
            lastReceiveLog = currentTime;
        }
    }

    if (dataID && strcmp(dataID, "ConanExiles_CompletePositional") == 0) {
        if (dataLength == sizeof(CompletePositionalData)) {
            const CompletePositionalData* receivedData = (const CompletePositionalData*)data;

            // Critical raw data logging | Log critique des données reçues brutes
            if (enableLogGeneral) {
                static ULONGLONG lastDataLog = 0;
                ULONGLONG currentTime = GetTickCount64();
                if (currentTime - lastDataLog > 1000) {
                    char logMsg[512];
                    snprintf(logMsg, sizeof(logMsg),
                        "📥 RAW DATA RECEIVED: Player='%s' Pos(%.6f,%.6f,%.6f) Dir(%.6f,%.6f,%.6f) VoiceDist=%.6f",
                        receivedData->playerName, receivedData->x, receivedData->y, receivedData->z,
                        receivedData->dirX, receivedData->dirY, receivedData->dirZ, receivedData->voiceDistance);
                    mumbleAPI.log(ownID, logMsg);
                    lastDataLog = currentTime;
                }
            }

            calculateLocalPositionalAudio(receivedData, sender);
            return true;
        }
        else {
            if (enableLogGeneral) {
                char errorMsg[256];
                snprintf(errorMsg, sizeof(errorMsg),
                    "❌ SIZE MISMATCH: Expected %zu bytes, got %zu bytes from user %u",
                    sizeof(CompletePositionalData), dataLength, sender);
                mumbleAPI.log(ownID, errorMsg);
            }
        }
    }

    return false;
}

// Called whenever any user enters a channel | Appelé quand un utilisateur entre dans un canal
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onChannelEntered(mumble_connection_t connection,
    mumble_userid_t userID,
    mumble_channelid_t previousChannelID,
    mumble_channelid_t newChannelID) {
    if (!enableAutomaticChannelChange) return;
    if (!channelManagementActive) return;

    mumble_userid_t localUserID;
    if (mumbleAPI.getLocalUserID(ownID, connection, &localUserID) != MUMBLE_STATUS_OK) {
        return;
    }

    if (userID == localUserID) {
        if (!coordinatesValid) {
            if (newChannelID != hubChannelID && hubChannelID != -1) {
                Sleep(50);
                mumbleAPI.requestUserMove(ownID, connection, localUserID, hubChannelID, NULL);
            }
        }
    }
}

// Channel exit cleanup | Nettoyage lors de la sortie d'un canal
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onChannelExited(mumble_connection_t connection,
    mumble_userid_t userID,
    mumble_channelid_t channelID) {

    if (userID != 0) {
        for (size_t i = 0; i < playerMuteStateCount; i++) {
            if (playerMuteStates[i].userID == userID) {
                for (size_t j = i; j < playerMuteStateCount - 1; j++) {
                    playerMuteStates[j] = playerMuteStates[j + 1];
                }
                playerMuteStateCount--;

                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Cleaned mute state for user %u who left channel", userID);
                    mumbleAPI.log(ownID, logMsg);
                }
                break;
            }
        }

        for (size_t i = 0; i < remotePlayerCount; i++) {
            // Note: periodic cleanup handled separately | Note: nettoyage périodique géré séparément
        }
    }
}

// Complete cleanup on disconnection | Nettoyage complet à la déconnexion
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerDisconnected(mumble_connection_t connection) {
    isConnectedToServer = FALSE;
    hubDescriptionAvailable = FALSE;
    hubLimitsActive = FALSE;

    channelManagementActive = FALSE;
    lastValidChannel = -1;

    // Clean audio system AND filters | Nettoyer le système audio ET les filtres
    cleanupAudioVolumeStates();

    cleanupAdaptivePlayerStates();
    cleanupPlayerMuteStates();
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    remotePlayerCount = 0;
    memset(localVoiceData.playerName, 0, sizeof(localVoiceData.playerName));
    lastVoiceDataSent = 0;
    lastDistanceCheck = 0;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "REAL adaptive volume system reset with low-pass filtering - audio processing restored");
    }
    destroyVoiceOverlay();
}

void mumble_shutdown() {
    overlayThreadRunning = FALSE;
    removeKeyMonitoring();

    // Voice system cleanup
    remotePlayerCount = 0;
    memset(remotePlayersData, 0, sizeof(CompletePositionalData));
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    cleanupPlayerMuteStates();

    if (lastHubDescriptionCache) {
        free(lastHubDescriptionCache);
        lastHubDescriptionCache = NULL;
        if (enableLogGeneral) mumbleAPI.log(ownID, "Freed cached hub description");
    }

    if (hBackgroundBitmap) {
        DeleteObject(hBackgroundBitmap);
        hBackgroundBitmap = NULL;
    }

    // Libérer le background avancé si présent
    if (hBackgroundAdvancedBitmap) {
        DeleteObject(hBackgroundAdvancedBitmap);
        hBackgroundAdvancedBitmap = NULL;
    }

    if (hBackgroundPresetsBitmap) {
        DeleteObject(hBackgroundPresetsBitmap);
        hBackgroundPresetsBitmap = NULL;
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Background bitmaps freed");
    }

    if (hPathBoxBitmap) {
        DeleteObject(hPathBoxBitmap);
        hPathBoxBitmap = NULL;
    }

}

// Release Mumble resource | Libérer une ressource Mumble
void mumble_releaseResource(const void* pointer) {
    mumbleAPI.log(ownID, u8"Called mumble_releaseResource but expected that this never gets called -> Aborting");
    abort();
}
