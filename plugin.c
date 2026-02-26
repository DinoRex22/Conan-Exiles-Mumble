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
BOOL enableAutomaticPatchFind = FALSE;
HWND hAutomaticPatchFindCheck = NULL;

// F9 coordinate broadcast variables | Variables pour la diffusion des coordonnées en F9
BOOL f9CoordinateBroadcastActive = FALSE;
ULONGLONG lastCoordinateBroadcast = 0;

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
uint8_t currentVoiceMode = 1; // 0: Whisper, 1: Normal, 2: Shout | 0: Murmure, 1: Normal, 2: Cri
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

// Thread stop flags | Flags d'arrêt des threads
BOOL modFileWatcherRunning = FALSE;
BOOL voiceSystemRunning = FALSE;
BOOL channelManagementRunning = FALSE;
BOOL hubDescriptionMonitorRunning = FALSE;

// Thread handles for cleanup | Handles de threads pour nettoyage
HANDLE modFileWatcherThreadHandle = NULL;
HANDLE voiceSystemThreadHandle = NULL;
HANDLE channelManagementThreadHandle = NULL;
HANDLE hubDescriptionMonitorThreadHandle = NULL;
HANDLE overlayMonitorThreadHandle = NULL;

// Connection and hub state variables | Variables pour détecter l'état de connexion et hub
BOOL isConnectedToServer = FALSE;
BOOL hubDescriptionAvailable = FALSE;
BOOL hubLimitsActive = FALSE;
ULONGLONG lastConnectionCheck = 0;

// First connection default settings tracking | Suivi des paramètres par défaut à la première connexion
char serverConfigHash[256] = "";
BOOL hasAppliedDefaultSettings = FALSE;

// Default suggested settings on first connection | Paramètres par défaut suggérés à la première connexion
BOOL enableDefaultSettingsOnFirstConnection = TRUE;
int defaultWhisperKey = 17;           // Ctrl | Contrôle
int defaultNormalKey = 86;            // V
int defaultShoutKey = 16;             // Shift | Maj
int defaultVoiceToggleKey = 84;       // T | T
float defaultDistanceWhisper = 2.0f;
float defaultDistanceNormal = 15.0f;
float defaultDistanceShout = 50.0f;

// Hub audio parameters | Paramètres audio du hub
double hubAudioMinDistance = 2.0;
double hubAudioMaxDistance = 50.0;
double hubAudioMaxVolume = 85.0;
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

// Race system structures | Structures pour le système de races
#define MAX_RACES 32
#define MAX_STEAMIDS_PER_RACE 100

typedef struct {
    char name[64];
    uint64_t steamIDs[MAX_STEAMIDS_PER_RACE];
    size_t steamIDCount;
    double minimumWhisper;
    double maximumWhisper;
    double minimumNormal;
    double maximumNormal;
    double minimumShout;
    double maximumShout;
    float listenAddDistance;
    BOOL isActive;
} Race;

// Steam ID variable | Variable du Steam ID
uint64_t steamID = 0;

// Race system variables | Variables pour le système de races
Race races[MAX_RACES];
size_t raceCount = 0;
int currentPlayerRaceIndex = -1;
float currentListenAddDistance = 0.0f;

// Zone system variables | Variables pour le système de zones
#define MAX_ZONES 32
typedef struct {
    char name[64];
    float x1, z1, x2, z2, x3, z3, x4, z4;
    float groundY;
    float topY;
    double audioMinDistance;
    double audioMaxDistance;
    double audioMaxVolume;
    float whisperDist;
    float normalDist;
    float shoutDist;
    BOOL isSoundproof;
} Zone;

Zone zones[MAX_ZONES];
size_t zoneCount = 0;
int currentZoneIndex = -1;

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

// Forward declarations | Déclarations forward
static float getVoiceDistanceForMode(uint8_t voiceMode);
static BOOL findConanExilesAutomatic(wchar_t* outPath, size_t pathSize);
static BOOL parseSteamLibraryFolders(const wchar_t* vdfPath, wchar_t* outConanPath, size_t pathSize);

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

// Display hub parameters confirmation in chat | Afficher la confirmation des paramètres du hub dans le chat
static void displayHubParametersConfirmation(BOOL globalSuccess, BOOL racesSuccess, BOOL playerInRace, BOOL zonesSuccess) {
    char confirmMsg[1024] = "";

    // Build confirmation message | Construire le message de confirmation
    strcat_s(confirmMsg, sizeof(confirmMsg), "Root Parameters: ");

    // Global parameters status | Statut des paramètres globaux
    strcat_s(confirmMsg, sizeof(confirmMsg), globalSuccess ? "GLOBAL✓ " : "GLOBAL✗ ");

    // Races status | Statut des races
    if (racesSuccess) {
        char raceMsg[128];
        if (playerInRace && currentPlayerRaceIndex != -1) {
            snprintf(raceMsg, sizeof(raceMsg), "RACES(%zu)✓ [YOUR RACE: %s] ",
                raceCount, races[currentPlayerRaceIndex].name);
        }
        else if (raceCount > 0) {
            snprintf(raceMsg, sizeof(raceMsg), "RACES(%zu)✓ [NOT IN RACE] ", raceCount);
        }
        else {
            snprintf(raceMsg, sizeof(raceMsg), "RACES(0)✓ ");
        }
        strcat_s(confirmMsg, sizeof(confirmMsg), raceMsg);
    }
    else {
        strcat_s(confirmMsg, sizeof(confirmMsg), "RACES✗ ");
    }

    // Zones status | Statut des zones
    if (zonesSuccess) {
        char zoneMsg[64];
        snprintf(zoneMsg, sizeof(zoneMsg), "ZONES(%zu)✓", zoneCount);
        strcat_s(confirmMsg, sizeof(confirmMsg), zoneMsg);
    }
    else {
        strcat_s(confirmMsg, sizeof(confirmMsg), "ZONES✗");
    }

    displayInChat(confirmMsg);
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

// Get server hash for tracking default settings | Obtenir le hash du serveur pour le suivi des paramètres par défaut
static BOOL getServerHashForTracking(mumble_connection_t connection, char* outHash, size_t hashSize) {
    if (!outHash || hashSize < 65) {
        if (outHash && hashSize > 0) outHash[0] = '\0';
        return FALSE;
    }

    const char* serverHash = NULL;
    mumble_error_t result = mumbleAPI.getServerHash(ownID, connection, &serverHash);

    if (result != MUMBLE_STATUS_OK || !serverHash) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Failed to retrieve server hash from Mumble API");
        }
        outHash[0] = '\0';
        return FALSE;
    }

    // Copy server hash to output buffer | Copier le hash du serveur au buffer de sortie
    strncpy_s(outHash, hashSize, serverHash, _TRUNCATE);

    // Free Mumble API memory | Libérer la mémoire de l'API Mumble
    mumbleAPI.freeMemory(ownID, serverHash);

    if (enableLogGeneral) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Server hash retrieved: %s", outHash);
        mumbleAPI.log(ownID, logMsg);
    }

    return TRUE;
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
    BOOL foundAutomaticPatchFind = FALSE;

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
                else if (wcsncmp(key, L"AutomaticPatchFind", 18) == 0) {
                    enableAutomaticPatchFind = (wcscmp(valLower, L"true") == 0 || wcscmp(valLower, L"1") == 0);
                    foundAutomaticPatchFind = TRUE;
                }

            }
            fclose(f);
        }
    }
    // If file doesn't exist or critical settings are missing, create or update it | Si le fichier n'existe pas ou si des paramètres critiques manquent, le créer ou le mettre à jour
    if (!fileExists || !foundSavedPath || !foundEnableAutomaticChannelChange || !foundAutomaticPatchFind) {
        FILE* f = _wfopen(configFile, L"w");
        if (f) {
            fwprintf(f, L"SavedPath=%s\n", foundSavedPath ? savedPathValue : L"");
            fwprintf(f, L"AutomaticSavedPath=\n");
            fwprintf(f, L"AutomaticPatchFind=%s\n", TRUE ? L"true" : L"false");
            fwprintf(f, L"AutomaticSavedPath=\n");
            fwprintf(f, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
            fwprintf(f, L"WhisperKey=%d\n", whisperKey);
            fwprintf(f, L"NormalKey=%d\n", normalKey);
            fwprintf(f, L"ShoutKey=%d\n", shoutKey);
            fwprintf(f, L"ConfigUIKey=%d\n", configUIKey);
            fwprintf(f, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
            fwprintf(f, L"DistanceWhisper=%.1f\n", foundDistanceWhisper ? distanceWhisper : 2.0f);
            fwprintf(f, L"DistanceNormal=%.1f\n", foundDistanceNormal ? distanceNormal : 10.0f);
            fwprintf(f, L"DistanceShout=%.1f\n", foundDistanceShout ? distanceShout : 15.0f);
            fwprintf(f, L"VoiceToggleKey=%d\n", voiceToggleKey);
            fwprintf(f, L"EnableVoiceToggle=%s\n", enableVoiceToggle ? L"true" : L"false");
            fclose(f);
        }
        // ✅ CORRECTION CRITIQUE : Définir enableAutomaticPatchFind à TRUE lors de la création initiale
        enableAutomaticPatchFind = TRUE;
    }

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Configuration loaded - Whisper: %.1fm, Normal: %.1fm, Shout: %.1fm, AutomaticPatchFind: %s",
            distanceWhisper, distanceNormal, distanceShout, enableAutomaticPatchFind ? "TRUE" : "FALSE");
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
        BOOL hasAutomaticSavedPath = FALSE;
        BOOL hasAutomaticPatchFind = FALSE;

        for (int i = 0; i < lineCount; i++) {
            fwprintf(f, L"%s", lines[i]);
            if (wcsncmp(lines[i], L"AutomaticSavedPath=", 19) == 0) {
                hasAutomaticSavedPath = TRUE;
            }
            if (wcsncmp(lines[i], L"AutomaticPatchFind=", 19) == 0) {
                hasAutomaticPatchFind = TRUE;
            }
        }

        fclose(f);

        // Update active distance based on absolute truth | Mettre à jour la distance active selon la vérité absolue
        localVoiceData.voiceDistance = getVoiceDistanceForMode(currentVoiceMode);

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
        voicePresets[i].whisperKey = 17;      // Ctrl | Contrôle
        voicePresets[i].normalKey = 86;       // V
        voicePresets[i].shoutKey = 16;        // Shift | Maj
        voicePresets[i].voiceToggleKey = 84;  // T
        voicePresets[i].isUsed = FALSE;
    }

    if (enableLogConfig) {
        mumbleAPI.log(ownID, "Voice presets initialized with default values and keyboard shortcuts");
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

    // Save current distances and keyboard shortcuts | Sauvegarder les distances actuelles et les touches clavier
    voicePresets[presetIndex].whisperDistance = distanceWhisper;
    voicePresets[presetIndex].normalDistance = distanceNormal;
    voicePresets[presetIndex].shoutDistance = distanceShout;
    voicePresets[presetIndex].whisperKey = whisperKey;
    voicePresets[presetIndex].normalKey = normalKey;
    voicePresets[presetIndex].shoutKey = shoutKey;
    voicePresets[presetIndex].voiceToggleKey = voiceToggleKey;
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

    // Load distances and keyboard shortcuts from preset | Charger les distances et les touches clavier depuis le preset
    distanceWhisper = voicePresets[presetIndex].whisperDistance;
    distanceNormal = voicePresets[presetIndex].normalDistance;
    distanceShout = voicePresets[presetIndex].shoutDistance;
    whisperKey = voicePresets[presetIndex].whisperKey;
    normalKey = voicePresets[presetIndex].normalKey;
    shoutKey = voicePresets[presetIndex].shoutKey;
    voiceToggleKey = voicePresets[presetIndex].voiceToggleKey;

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

        // Update keyboard shortcut displays in interface | Mettre à jour l'affichage des touches clavier dans l'interface
        if (hWhisperKeyEdit) SetWindowTextA(hWhisperKeyEdit, getKeyName(whisperKey));
        if (hNormalKeyEdit) SetWindowTextA(hNormalKeyEdit, getKeyName(normalKey));
        if (hShoutKeyEdit) SetWindowTextA(hShoutKeyEdit, getKeyName(shoutKey));
        if (hVoiceToggleKeyEdit) SetWindowTextA(hVoiceToggleKeyEdit, getKeyName(voiceToggleKey));

        isUpdatingInterface = FALSE;

        updateDynamicInterface();
    }

    // Update active distance based on absolute truth | Mettre à jour la distance active selon la vérité absolue
    localVoiceData.voiceDistance = getVoiceDistanceForMode(currentVoiceMode);

    wchar_t gameFolder[MAX_PATH] = L"";
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

    wchar_t distWhisper[32], distNormal[32], distShout[32];
    swprintf(distWhisper, 32, L"%.1f", distanceWhisper);
    swprintf(distNormal, 32, L"%.1f", distanceNormal);
    swprintf(distShout, 32, L"%.1f", distanceShout);

    writeFullConfiguration(gameFolder, distWhisper, distNormal, distShout);

    // Apply changes | Appliquer les changements
    applyDistanceToAllPlayers();

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Voice preset loaded and saved: [%d] '%s' - Whisper:%.1f Normal:%.1f Shout:%.1f Keys: W=%d N=%d S=%d T=%d",
            presetIndex, voicePresets[presetIndex].name,
            distanceWhisper, distanceNormal, distanceShout,
            whisperKey, normalKey, shoutKey, voiceToggleKey);
        mumbleAPI.log(ownID, logMsg);
    }

    // Show confirmation message | Afficher message de confirmation
    char confirmMsg[256];
    snprintf(confirmMsg, sizeof(confirmMsg),
        "✅ Preset loaded and saved: '%s'", voicePresets[presetIndex].name);
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
        fwprintf(file, L"WhisperKey=%d\n", voicePresets[i].whisperKey);
        fwprintf(file, L"NormalKey=%d\n", voicePresets[i].normalKey);
        fwprintf(file, L"ShoutKey=%d\n", voicePresets[i].shoutKey);
        fwprintf(file, L"VoiceToggleKey=%d\n", voicePresets[i].voiceToggleKey);
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
            else if (wcsncmp(line, L"WhisperKey=", 11) == 0) {
                voicePresets[currentPreset].whisperKey = _wtoi(line + 11);
            }
            else if (wcsncmp(line, L"NormalKey=", 10) == 0) {
                voicePresets[currentPreset].normalKey = _wtoi(line + 10);
            }
            else if (wcsncmp(line, L"ShoutKey=", 9) == 0) {
                voicePresets[currentPreset].shoutKey = _wtoi(line + 9);
            }
            else if (wcsncmp(line, L"VoiceToggleKey=", 15) == 0) {
                voicePresets[currentPreset].voiceToggleKey = _wtoi(line + 15);
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

    // Read existing SavedPath and AutomaticSavedPath before overwriting | Lire le SavedPath et AutomaticSavedPath existants avant écrasement
    wchar_t existingSavedPath[MAX_PATH] = L"";
    wchar_t existingAutoPath[MAX_PATH] = L"";

    FILE* fRead = _wfopen(configFile, L"r");
    if (fRead) {
        wchar_t line[512];
        while (fgetws(line, 512, fRead)) {
            if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                wchar_t* pathStart = line + 10;
                wchar_t* nl = wcschr(pathStart, L'\n');
                if (nl) *nl = L'\0';
                wchar_t* cr = wcschr(pathStart, L'\r');
                if (cr) *cr = L'\0';
                wcscpy_s(existingSavedPath, MAX_PATH, pathStart);
            }
            else if (wcsncmp(line, L"AutomaticSavedPath=", 19) == 0) {
                wchar_t* pathStart = line + 19;
                wchar_t* nl = wcschr(pathStart, L'\n');
                if (nl) *nl = L'\0';
                wchar_t* cr = wcschr(pathStart, L'\r');
                if (cr) *cr = L'\0';
                wcscpy_s(existingAutoPath, MAX_PATH, pathStart);
            }
        }
        fclose(fRead);
    }

    // Write configuration file | Écrire le fichier de configuration
    FILE* file = _wfopen(configFile, L"w");
    if (!file) {
        return;
    }

    // Logic for SavedPath | Logique pour SavedPath
    if (enableAutomaticPatchFind) {
        // Automatic mode: preserve manual path in SavedPath, write to AutomaticSavedPath | Mode automatique : préserver le chemin manuel dans SavedPath, écrire dans AutomaticSavedPath
        if (wcslen(existingSavedPath) > 0) {
            fwprintf(file, L"SavedPath=%s\n", existingSavedPath);
        }
        else {
            fwprintf(file, L"SavedPath=\n");
        }
        fwprintf(file, L"AutomaticSavedPath=%s\n", savedPathFull);
    }
    else {
        // Manual mode: write to SavedPath, preserve automatic path in AutomaticSavedPath | Mode manuel : écrire dans SavedPath, préserver le chemin automatique dans AutomaticSavedPath
        fwprintf(file, L"SavedPath=%s\n", savedPathFull);
        if (wcslen(existingAutoPath) > 0) {
            fwprintf(file, L"AutomaticSavedPath=%s\n", existingAutoPath);
        }
        else {
            fwprintf(file, L"AutomaticSavedPath=\n");
        }
    }

    fwprintf(file, L"AutomaticPatchFind=%s\n", enableAutomaticPatchFind ? L"true" : L"false");
    fwprintf(file, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
    fwprintf(file, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
    fwprintf(file, L"WhisperKey=%d\n", whisperKey);
    fwprintf(file, L"NormalKey=%d\n", normalKey);
    fwprintf(file, L"ShoutKey=%d\n", shoutKey);
    fwprintf(file, L"ConfigUIKey=%d\n", configUIKey);
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
    // Override limits if player is in a zone | Surcharger les limites si le joueur est dans une zone
    // Only flag limits as active and update UI if in a zone | Marquer les limites comme actives et MAJ interface si en zone
    if (currentZoneIndex != -1) {
        hubLimitsActive = TRUE;
        applyDistanceToAllPlayers(); // Recalculate based on zone values via getVoiceDistanceForMode | Recalculer via getVoiceDistanceForMode
        if (hConfigDialog && IsWindow(hConfigDialog)) updateDynamicInterface();
        return; // Global variables remain unchanged (stay in RAM as user settings) | Les variables globales restent inchangées (restent en RAM comme réglages utilisateur)
    }

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

// Helper function to determine if a point is inside a polygon (2D point-in-polygon) | Fonction helper pour déterminer si un point est dans un polygone
static BOOL isPointInPolygon(float px, float pz, float x1, float z1, float x2, float z2, float x3, float z3, float x4, float z4) {
    // Use ray casting algorithm for quadrilateral | Utiliser l'algorithme de ray casting pour quadrilatéral
    // Cross product method to check if point is on the same side of all edges | Méthode du produit croisé pour vérifier si le point est du même côté de tous les bords

    // Define the 4 vertices of the polygon | Définir les 4 sommets du polygone
    float vertices[4][2] = {
        {x1, z1}, {x2, z2}, {x3, z3}, {x4, z4}
    };

    // Check if point is inside the polygon using cross product method | Vérifier si le point est dans le polygone
    int sign = 0;
    for (int i = 0; i < 4; i++) {
        float x_a = vertices[i][0];
        float z_a = vertices[i][1];
        float x_b = vertices[(i + 1) % 4][0];
        float z_b = vertices[(i + 1) % 4][1];

        // Calculate cross product | Calculer le produit croisé
        float cross = (x_b - x_a) * (pz - z_a) - (z_b - z_a) * (px - x_a);

        if (cross != 0.0f) {
            int current_sign = (cross > 0.0f) ? 1 : -1;
            if (sign == 0) {
                sign = current_sign;
            }
            else if (sign != current_sign) {
                return FALSE; // Point is outside polygon | Point est hors du polygone
            }
        }
    }

    return TRUE; // Point is inside polygon | Point est dans le polygone
}

// Check if a player is inside a specific zone (3D quadrilateral) | Vérifier si un joueur est dans une zone spécifique (quadrilatéral 3D)
static int getPlayerZone(float playerX, float playerY, float playerZ) {
    for (size_t i = 0; i < zoneCount; i++) {
        // Check if point is inside the 4-coordinate polygon | Vérifier si le point est dans le polygone à 4 coordonnées
        if (isPointInPolygon(playerX, playerZ,
            zones[i].x1, zones[i].z1,
            zones[i].x2, zones[i].z2,
            zones[i].x3, zones[i].z3,
            zones[i].x4, zones[i].z4)) {

            // Check Y boundary (vertical cube limits) | Vérifier la limite Y (limites verticales du cube)
            float minY = zones[i].groundY < zones[i].topY ? zones[i].groundY : zones[i].topY;
            float maxY = zones[i].groundY > zones[i].topY ? zones[i].groundY : zones[i].topY;

            if (playerY >= minY && playerY <= maxY) {
                return (int)i; // Player is in 3D zone cube | Joueur dans le cube 3D de la zone
            }
        }
    }
    return -1; // Player not in any zone | Joueur hors de toute zone
}

// ============================================================================
// MODULE 4 : PARSING HUB
// ============================================================================

// Apply default settings on first connection to server | Appliquer les paramètres par défaut à la première connexion au serveur
static void applyDefaultSettingsIfNeeded(const char* description, mumble_connection_t connection) {
    if (!enableDefaultSettingsOnFirstConnection || !description) {
        return;
    }

    // Get current server hash | Obtenir le hash actuel du serveur
    char currentHash[256] = "";
    if (!getServerHashForTracking(connection, currentHash, sizeof(currentHash))) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Cannot apply default settings: failed to get server hash");
        }
        return;
    }

    // Load stored settings | Charger les paramètres stockés
    loadDefaultSettingsFromConfig();

    // Check if config has changed or first time | Vérifier si la config a changé ou première fois
    if (strcmp(currentHash, serverConfigHash) != 0 || !hasAppliedDefaultSettings) {
        if (enableLogGeneral) {
            char logMsg[512];
            snprintf(logMsg, sizeof(logMsg),
                "FIRST CONNECTION DETECTED:\n"
                "  Old hash: %s\n"
                "  New hash: %s\n"
                "  Applying default settings...",
                serverConfigHash, currentHash);
            mumbleAPI.log(ownID, logMsg);
        }

        // Store new hash | Stocker le nouveau hash
        strcpy_s(serverConfigHash, sizeof(serverConfigHash), currentHash);
        hasAppliedDefaultSettings = TRUE;

        // Apply suggested default settings | Appliquer les paramètres par défaut suggérés
        whisperKey = defaultWhisperKey;
        normalKey = defaultNormalKey;
        shoutKey = defaultShoutKey;
        voiceToggleKey = defaultVoiceToggleKey;
        distanceWhisper = defaultDistanceWhisper;
        distanceNormal = defaultDistanceNormal;
        distanceShout = defaultDistanceShout;

        if (enableLogGeneral) {
            char applyMsg[512];
            snprintf(applyMsg, sizeof(applyMsg),
                "APPLYING DEFAULT SETTINGS:\n"
                "  Keys: Whisper=%d(%s), Normal=%d(%s), Shout=%d(%s), VoiceToggle=%d(%s)\n"
                "  Distances: Whisper=%.1f, Normal=%.1f, Shout=%.1f",
                whisperKey, getKeyName(whisperKey),
                normalKey, getKeyName(normalKey),
                shoutKey, getKeyName(shoutKey),
                voiceToggleKey, getKeyName(voiceToggleKey),
                distanceWhisper, distanceNormal, distanceShout);
            mumbleAPI.log(ownID, applyMsg);
        }

        // ✅ CORRECTION CRITIQUE : Utiliser writeFullConfiguration pour sauvegarder TOUTES les touches
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

        // Préparer les distances pour writeFullConfiguration
        wchar_t distWhisper[32], distNormal[32], distShout[32];
        swprintf(distWhisper, 32, L"%.1f", distanceWhisper);
        swprintf(distNormal, 32, L"%.1f", distanceNormal);
        swprintf(distShout, 32, L"%.1f", distanceShout);

        // ✅ UTILISER writeFullConfiguration (comme Save Configuration)
        writeFullConfiguration(gameFolder, distWhisper, distNormal, distShout);

        // Save default settings with hash | Sauvegarder les paramètres par défaut avec le hash
        saveDefaultSettingsToConfig();

        // Update UI if open | Mettre à jour l'interface si ouverte
        if (hConfigDialog && IsWindow(hConfigDialog)) {
            isUpdatingInterface = TRUE;

            wchar_t whisperText[32], normalText[32], shoutText[32];
            swprintf(whisperText, 32, L"%.1f", distanceWhisper);
            swprintf(normalText, 32, L"%.1f", distanceNormal);
            swprintf(shoutText, 32, L"%.1f", distanceShout);

            if (hDistanceWhisperEdit) SetWindowTextW(hDistanceWhisperEdit, whisperText);
            if (hDistanceNormalEdit) SetWindowTextW(hDistanceNormalEdit, normalText);
            if (hDistanceShoutEdit) SetWindowTextW(hDistanceShoutEdit, shoutText);

            SetWindowTextA(hWhisperKeyEdit, getKeyName(whisperKey));
            SetWindowTextA(hNormalKeyEdit, getKeyName(normalKey));
            SetWindowTextA(hShoutKeyEdit, getKeyName(shoutKey));
            SetWindowTextA(hVoiceToggleKeyEdit, getKeyName(voiceToggleKey));

            CheckDlgButton(hConfigDialog, 204, enableVoiceToggle ? BST_CHECKED : BST_UNCHECKED);

            isUpdatingInterface = FALSE;
            updateDynamicInterface();
        }

        if (enableLogGeneral) {
            char defaultMsg[512];
            snprintf(defaultMsg, sizeof(defaultMsg),
                "DEFAULT SETTINGS APPLIED FOR FIRST CONNECTION:\n"
                "  • Whisper: Key=%s (%d) | Distance=%.1f m\n"
                "  • Normal: Key=%s (%d) | Distance=%.1f m\n"
                "  • Shout: Key=%s (%d) | Distance=%.1f m\n"
                "  • Voice Toggle: Key=%s (%d)\n"
                "  These settings can be changed in Advanced Options",
                getKeyName(whisperKey), whisperKey, distanceWhisper,
                getKeyName(normalKey), normalKey, distanceNormal,
                getKeyName(shoutKey), shoutKey, distanceShout,
                getKeyName(voiceToggleKey), voiceToggleKey);
            mumbleAPI.log(ownID, defaultMsg);
        }

        displayInChat("Suggested default settings applied for first connection");
    }
}

// Parse hub description to extract audio parameters | Parser la description du Root pour extraire les paramètres audio
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

    // Single pass: check for [GLOBAL] and parse | Passage unique : vérifier [GLOBAL] et parser
    BOOL globalSectionFound = FALSE;
    BOOL insideGlobalSection = FALSE;

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

        // Detect [GLOBAL] section start | Détecter le début de la section [GLOBAL]
        if (strncmp(line, "[GLOBAL]", 8) == 0) {
            globalSectionFound = TRUE;
            insideGlobalSection = TRUE;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Hub: [GLOBAL] section found - entering global parameters");
            }
            line = strtok_s(NULL, "\n\r", &context);
            continue;
        }

        // Detect section end | Détecter la fin de section
        if (insideGlobalSection && line[0] == '[') {
            insideGlobalSection = FALSE;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Hub: [GLOBAL] section ended - stopping global parameter parsing");
            }
            break; // Stop processing to avoid parsing other sections | Arrêter le traitement pour éviter de parser d'autres sections
        }

        // Only process parameters inside [GLOBAL] section | Traiter uniquement les paramètres dans [GLOBAL]
        if (!insideGlobalSection) {
            line = strtok_s(NULL, "\n\r", &context);
            continue;
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

                    // CORRECTION: Désactiver enableAutomaticChannelChange si le hub dit FALSE
                    if (enableAutomaticChannelChange) {
                        enableAutomaticChannelChange = FALSE;
                        if (enableLogGeneral) {
                            mumbleAPI.log(ownID, "Hub: ForceAutomaticChannelSwitching = FALSE - Disabling automatic channel switching");
                        }
                    }

                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "Hub: ForceAutomaticChannelSwitching = FALSE - User has full control");
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

    // ========== PARSE ZONES | PARSER LES ZONES ==========
    BOOL zoneSectionFound = FALSE;
    zoneCount = 0;
    currentZoneIndex = -1;

    char* zoneContext = NULL;
    char* descCopyZones = (char*)malloc(descLen * 2 + 1);
    if (descCopyZones) {
        dest = descCopyZones;
        src = description;
        while (*src) {
            if (strncmp(src, "<br/>", 5) == 0 || strncmp(src, "<BR/>", 5) == 0) { *dest++ = '\n'; src += 5; }
            else if (strncmp(src, "<br>", 4) == 0 || strncmp(src, "<BR>", 4) == 0) { *dest++ = '\n'; src += 4; }
            else { *dest++ = *src++; }
        }
        *dest = '\0';

        char* zoneLine = strtok_s(descCopyZones, "\n\r", &zoneContext);
        Zone* currentParsingZone = NULL;
        BOOL insideZonesSection = FALSE;

        while (zoneLine != NULL && zoneCount < MAX_ZONES) {
            while (*zoneLine == ' ' || *zoneLine == '\t') zoneLine++;
            char* zEnd = zoneLine + strlen(zoneLine) - 1;
            while (zEnd > zoneLine && (*zEnd == ' ' || *zEnd == '\t')) { *zEnd = '\0'; zEnd--; }

            if (strncmp(zoneLine, "[ZONES]", 7) == 0) {
                insideZonesSection = TRUE;
                zoneSectionFound = TRUE;
                zoneLine = strtok_s(NULL, "\n\r", &zoneContext);
                continue;
            }

            if (insideZonesSection && zoneLine[0] == '[' && strncmp(zoneLine, "[ZONES]", 7) != 0) {
                insideZonesSection = FALSE;
            }

            if (insideZonesSection) {
                if (strncmp(zoneLine, "Zone=", 5) == 0) {
                    currentParsingZone = &zones[zoneCount];
                    memset(currentParsingZone, 0, sizeof(Zone));
                    strncpy_s(currentParsingZone->name, sizeof(currentParsingZone->name), zoneLine + 5, _TRUNCATE);
                    zoneCount++;
                }
                else if (currentParsingZone) {
                    if (strncmp(zoneLine, "X1=", 3) == 0) currentParsingZone->x1 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "Z1=", 3) == 0) currentParsingZone->z1 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "X2=", 3) == 0) currentParsingZone->x2 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "Z2=", 3) == 0) currentParsingZone->z2 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "X3=", 3) == 0) currentParsingZone->x3 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "Z3=", 3) == 0) currentParsingZone->z3 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "X4=", 3) == 0) currentParsingZone->x4 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "Z4=", 3) == 0) currentParsingZone->z4 = (float)strtof(zoneLine + 3, NULL);
                    else if (strncmp(zoneLine, "AudioMinDistance=", 17) == 0) currentParsingZone->audioMinDistance = strtod(zoneLine + 17, NULL);
                    else if (strncmp(zoneLine, "AudioMaxDistance=", 17) == 0) currentParsingZone->audioMaxDistance = strtod(zoneLine + 17, NULL);
                    else if (strncmp(zoneLine, "AudioMaxVolume=", 15) == 0) currentParsingZone->audioMaxVolume = strtod(zoneLine + 15, NULL);
                    else if (strncmp(zoneLine, "Wisper=", 7) == 0) currentParsingZone->whisperDist = (float)strtof(zoneLine + 7, NULL);
                    else if (strncmp(zoneLine, "Normal=", 7) == 0) currentParsingZone->normalDist = (float)strtof(zoneLine + 7, NULL);
                    else if (strncmp(zoneLine, "Shout=", 6) == 0) currentParsingZone->shoutDist = (float)strtof(zoneLine + 6, NULL);
                    else if (strncmp(zoneLine, "SoundProof=", 11) == 0) {
                        char* value = zoneLine + 11;
                        while (*value == ' ' || *value == '\t') value++;
                        currentParsingZone->isSoundproof = (strncmp(value, "True", 4) == 0 || strncmp(value, "true", 4) == 0 || strncmp(value, "TRUE", 4) == 0);
                    }
                    else if (strncmp(zoneLine, "GroundY=", 8) == 0) {
                        currentParsingZone->groundY = (float)strtof(zoneLine + 8, NULL);
                    }
                    else if (strncmp(zoneLine, "TopY=", 5) == 0) {
                        currentParsingZone->topY = (float)strtof(zoneLine + 5, NULL);
                    }
                }
            }
            zoneLine = strtok_s(NULL, "\n\r", &zoneContext);
        }
        free(descCopyZones);
    }

        // ========== PARSE RACES AFTER ZONES | PARSER LES RACES APRÈS LES ZONES ==========
    // Reset race count | Réinitialiser le compteur de races
    raceCount = 0;
    currentPlayerRaceIndex = -1;
    currentListenAddDistance = 0.0f;

    // Create new copy for race parsing | Créer une nouvelle copie pour le parsing des races
    char* descCopyRaces = (char*)malloc(descLen * 2 + 1);
    if (!descCopyRaces) {
        return;
    }

    // Recreate copy with HTML replacement | Recréer la copie avec remplacement HTML
    dest = descCopyRaces;
    src = description;

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

    // Parse races line by line | Parser les races ligne par ligne
    char* raceContext = NULL;
    char* raceLine = strtok_s(descCopyRaces, "\n\r", &raceContext);
    Race* currentRace = NULL;
    BOOL insideRacesSection = FALSE;

    while (raceLine != NULL && raceCount < MAX_RACES) {
        // Clean whitespace | Nettoyer les espaces
        while (*raceLine == ' ' || *raceLine == '\t') raceLine++;
        char* end = raceLine + strlen(raceLine) - 1;
        while (end > raceLine && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

        // Detect [RACE] section start | Détecter le début de la section [RACE]
        if (strncmp(raceLine, "[RACE]", 6) == 0) {
            insideRacesSection = TRUE;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Race: [RACE] section found - entering race parsing");
            }
            raceLine = strtok_s(NULL, "\n\r", &raceContext);
            continue;
        }

        // Only process parameters inside [RACE] section | Traiter uniquement les paramètres dans [RACE]
        if (!insideRacesSection) {
            raceLine = strtok_s(NULL, "\n\r", &raceContext);
            continue;
        }

        // Detect race start | Détecter le début d'une race
        if (strncmp(raceLine, "Race=", 5) == 0) {
            currentRace = &races[raceCount];
            memset(currentRace, 0, sizeof(Race));

            char* raceName = raceLine + 5;
            strncpy_s(currentRace->name, sizeof(currentRace->name), raceName, _TRUNCATE);
            currentRace->isActive = TRUE;

            // Default values from global settings | Valeurs par défaut depuis les réglages globaux
            currentRace->minimumWhisper = hubMinimumWhisper;
            currentRace->maximumWhisper = hubMaximumWhisper;
            currentRace->minimumNormal = hubMinimumNormal;
            currentRace->maximumNormal = hubMaximumNormal;
            currentRace->minimumShout = hubMinimumShout;
            currentRace->maximumShout = hubMaximumShout;
            currentRace->listenAddDistance = 0.0f;
            currentRace->steamIDCount = 0;

            if (enableLogGeneral) {
                char logMsg[128];
                snprintf(logMsg, sizeof(logMsg), "Race: Parsing race '%s'", currentRace->name);
                mumbleAPI.log(ownID, logMsg);
            }

            raceCount++;
        }
        else if (currentRace) {
            // Parse SteamID list | Parser la liste de SteamID
            if (strncmp(raceLine, "SteamID=", 8) == 0) {
                char* steamIDList = raceLine + 8;

                // Parse comma-separated SteamIDs | Parser les SteamIDs séparés par des virgules
                char* tokenContext = NULL;
                char* token = strtok_s(steamIDList, ",", &tokenContext);
                while (token != NULL && currentRace->steamIDCount < MAX_STEAMIDS_PER_RACE) {
                    // Skip whitespace | Ignorer les espaces
                    while (*token == ' ' || *token == '\t') token++;

                    // Find opening parenthesis to skip name | Trouver la parenthèse ouvrante pour ignorer le nom
                    char* openParen = strchr(token, '(');
                    char* closeParen = strchr(token, ')');

                    char* steamIDStr = NULL;
                    if (openParen && closeParen && closeParen > openParen) {
                        // Format: (Name)SteamID - extract SteamID after closing parenthesis
                        steamIDStr = closeParen + 1;
                    }
                    else {
                        // No parenthesis, use whole token | Pas de parenthèses, utiliser le token entier
                        steamIDStr = token;
                    }

                    // Skip whitespace before SteamID | Ignorer les espaces avant le SteamID
                    while (*steamIDStr == ' ' || *steamIDStr == '\t') steamIDStr++;

                    // Convert to uint64_t | Convertir en uint64_t
                    uint64_t steamID = strtoull(steamIDStr, NULL, 10);

                    if (steamID > 0) {
                        currentRace->steamIDs[currentRace->steamIDCount++] = steamID;

                        if (enableLogGeneral) {
                            char logMsg[256];
                            snprintf(logMsg, sizeof(logMsg),
                                "Race: Added SteamID %llu to race '%s'",
                                steamID, currentRace->name);
                            mumbleAPI.log(ownID, logMsg);
                        }
                    }

                    token = strtok_s(NULL, ",", &tokenContext);
                }
            }
            // Parse race parameters | Parser les paramètres de race
            else if (strncmp(raceLine, "MinimumWisper=", 14) == 0 || strncmp(raceLine, "MinimumWhisper=", 15) == 0) {
                char* value = strchr(raceLine, '=') + 1;
                currentRace->minimumWhisper = strtod(value, NULL);
            }
            else if (strncmp(raceLine, "MaximumWisper=", 14) == 0 || strncmp(raceLine, "MaximumWhisper=", 15) == 0) {
                char* value = strchr(raceLine, '=') + 1;
                currentRace->maximumWhisper = strtod(value, NULL);
            }
            else if (strncmp(raceLine, "MinimumNormal=", 14) == 0) {
                char* value = strchr(raceLine, '=') + 1;
                currentRace->minimumNormal = strtod(value, NULL);
            }
            else if (strncmp(raceLine, "MaximumNormal=", 14) == 0) {
                char* value = strchr(raceLine, '=') + 1;
                currentRace->maximumNormal = strtod(value, NULL);
            }
            else if (strncmp(raceLine, "MinimumShout=", 13) == 0) {
                char* value = strchr(raceLine, '=') + 1;
                currentRace->minimumShout = strtod(value, NULL);
            }
            else if (strncmp(raceLine, "MaximumShout=", 13) == 0) {
                char* value = strchr(raceLine, '=') + 1;
                currentRace->maximumShout = strtod(value, NULL);
            }
            else if (strncmp(raceLine, "listenAddDistance=", 18) == 0) {
                char* value = strchr(raceLine, '=') + 1;
                currentRace->listenAddDistance = (float)strtod(value, NULL);
            }
        }

        raceLine = strtok_s(NULL, "\n\r", &raceContext);
    }

    // Free races copy | Libérer la copie des races
    free(descCopyRaces);

    // Check if local player belongs to a race | Vérifier si le joueur local appartient à une race
    if (steamID > 0) {
        for (size_t i = 0; i < raceCount; i++) {
            for (size_t j = 0; j < races[i].steamIDCount; j++) {
                if (races[i].steamIDs[j] == steamID) {
                    currentPlayerRaceIndex = (int)i;
                    currentListenAddDistance = races[i].listenAddDistance;

                    // ✅ ÉTAPE 1 : Appliquer les limites de race IMMÉDIATEMENT
                    hubMinimumWhisper = races[i].minimumWhisper;
                    hubMaximumWhisper = races[i].maximumWhisper;
                    hubMinimumNormal = races[i].minimumNormal;
                    hubMaximumNormal = races[i].maximumNormal;
                    hubMinimumShout = races[i].minimumShout;
                    hubMaximumShout = races[i].maximumShout;

                    if (enableLogGeneral) {
                        char logMsg[512];
                        snprintf(logMsg, sizeof(logMsg),
                            "🏃 RACE DETECTED: Player matched to race '%s' - applying race limits IMMEDIATELY (NO global override)",
                            races[i].name);
                        mumbleAPI.log(ownID, logMsg);
                    }

                    break;
                }
            }

            if (currentPlayerRaceIndex != -1) break;
        }
    }

    // ✅ ÉTAPE 2 : SI PAS DE RACE, garder les limites GLOBALES (ne PAS les réappliquer)
    if (currentPlayerRaceIndex == -1 && enableLogGeneral) {
        mumbleAPI.log(ownID, "No race found - using GLOBAL limits (already set from [GLOBAL] section)");
    }

    if (enableLogGeneral) {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Race: Parsed %zu races from hub description", raceCount);
        mumbleAPI.log(ownID, logMsg);
    }

    // ========== END RACES PARSING | FIN DU PARSING DES RACES ==========

    validatePlayerDistances();

        // ========== PARSE DEFAULT SETTINGS AFTER RACES | PARSER LES PARAMÈTRES PAR DÉFAUT APRÈS LES RACES ==========
    char* defaultSettingsContext = NULL;
    char* descCopyDefaultSettings = (char*)malloc(descLen * 2 + 1);
    if (descCopyDefaultSettings) {
        // Recréer la copie avec remplacement HTML
        dest = descCopyDefaultSettings;
        src = description;

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

        char* defaultSettingsLine = strtok_s(descCopyDefaultSettings, "\n\r", &defaultSettingsContext);
        BOOL insideDefaultSettingsSection = FALSE;

        while (defaultSettingsLine != NULL) {
            // Clean whitespace | Nettoyer les espaces
            while (*defaultSettingsLine == ' ' || *defaultSettingsLine == '\t') defaultSettingsLine++;
            char* end = defaultSettingsLine + strlen(defaultSettingsLine) - 1;
            while (end > defaultSettingsLine && (*end == ' ' || *end == '\t')) {
                *end = '\0';
                end--;
            }

            // Detect [DEFAULT_SETTINGS] section | Détecter la section [DEFAULT_SETTINGS]
            if (strncmp(defaultSettingsLine, "[DEFAULT_SETTINGS]", 18) == 0) {
                insideDefaultSettingsSection = TRUE;
                if (enableLogGeneral) {
                    mumbleAPI.log(ownID, "Hub: [DEFAULT_SETTINGS] section found - entering default settings parsing");
                }
                defaultSettingsLine = strtok_s(NULL, "\n\r", &defaultSettingsContext);
                continue;
            }

            // Only process parameters inside [DEFAULT_SETTINGS] section | Traiter uniquement les paramètres dans [DEFAULT_SETTINGS]
            if (!insideDefaultSettingsSection) {
                defaultSettingsLine = strtok_s(NULL, "\n\r", &defaultSettingsContext);
                continue;
            }

            // Parse default settings parameters | Parser les paramètres par défaut
            if (strncmp(defaultSettingsLine, "EnableDefaultSettingsOnFirstConnection=", 39) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                while (*value == ' ' || *value == '\t') value++;
                enableDefaultSettingsOnFirstConnection = (strncmp(value, "true", 4) == 0 || strncmp(value, "True", 4) == 0 || strncmp(value, "TRUE", 4) == 0);
                if (enableLogGeneral) {
                    char logMsg[128];
                    snprintf(logMsg, sizeof(logMsg), "Hub: EnableDefaultSettingsOnFirstConnection = %s", enableDefaultSettingsOnFirstConnection ? "TRUE" : "FALSE");
                    mumbleAPI.log(ownID, logMsg);
                }
            }
            else if (strncmp(defaultSettingsLine, "DefaultWhisperKey=", 18) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                defaultWhisperKey = atoi(value);
            }
            else if (strncmp(defaultSettingsLine, "DefaultNormalKey=", 17) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                defaultNormalKey = atoi(value);
            }
            else if (strncmp(defaultSettingsLine, "DefaultShoutKey=", 16) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                defaultShoutKey = atoi(value);
            }
            else if (strncmp(defaultSettingsLine, "DefaultVoiceToggleKey=", 22) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                defaultVoiceToggleKey = atoi(value);
            }
            else if (strncmp(defaultSettingsLine, "DefaultDistanceWhisper=", 23) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                defaultDistanceWhisper = (float)strtod(value, NULL);
            }
            else if (strncmp(defaultSettingsLine, "DefaultDistanceNormal=", 22) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                defaultDistanceNormal = (float)strtod(value, NULL);
            }
            else if (strncmp(defaultSettingsLine, "DefaultDistanceShout=", 21) == 0) {
                char* value = strchr(defaultSettingsLine, '=') + 1;
                defaultDistanceShout = (float)strtod(value, NULL);
            }

            defaultSettingsLine = strtok_s(NULL, "\n\r", &defaultSettingsContext);
        }

        free(descCopyDefaultSettings);

        if (enableLogGeneral) {
            char logMsg[512];
            snprintf(logMsg, sizeof(logMsg),
                "Hub: Default settings parsed - Enable=%s, WhisperKey=%d, NormalKey=%d, ShoutKey=%d, VoiceToggleKey=%d, Distances=(%.1f/%.1f/%.1f)",
                enableDefaultSettingsOnFirstConnection ? "TRUE" : "FALSE",
                defaultWhisperKey, defaultNormalKey, defaultShoutKey, defaultVoiceToggleKey,
                defaultDistanceWhisper, defaultDistanceNormal, defaultDistanceShout);
            mumbleAPI.log(ownID, logMsg);
        }
    }
    // ========== END DEFAULT SETTINGS PARSING | FIN DU PARSING DES PARAMÈTRES PAR DÉFAUT ==========


    // Display hub parameters confirmation in chat | Afficher la confirmation des paramètres du hub dans le chat
    BOOL globalSuccess = globalSectionFound;
    BOOL racesSuccess = (raceCount > 0 || insideRacesSection);
    BOOL playerInRace = (currentPlayerRaceIndex != -1);

    displayHubParametersConfirmation(globalSuccess, racesSuccess, playerInRace, zoneSectionFound);    
    // Apply default settings AFTER all parsing is complete | Appliquer les paramètres par défaut APRÈS le parsing complet
    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) == MUMBLE_STATUS_OK) {
        applyDefaultSettingsIfNeeded(description, connection);
    }

    // Detailed messages for each section | Messages détaillés pour chaque section
    if (globalSuccess && enableLogGeneral) {
        char globalMsg[256];
        snprintf(globalMsg, sizeof(globalMsg),
            "GLOBAL parameters loaded: ForceAudio=%s, ForceMuting=%s, ForceChannelSwitch=%s",
            enableAutoAudioSettings ? "TRUE" : "FALSE",
            hubForceDistanceBasedMuting ? "TRUE" : "FALSE",
            hubForceAutomaticChannelSwitching ? "TRUE" : "FALSE");
        displayInChat(globalMsg);
    }

    if (racesSuccess && enableLogGeneral) {
        if (playerInRace && currentPlayerRaceIndex != -1) {
            char playerRaceMsg[256];
            snprintf(playerRaceMsg, sizeof(playerRaceMsg),
                "RACE loaded: You are [%s] with listenAddDistance=%.1f",
                races[currentPlayerRaceIndex].name, currentListenAddDistance);
            displayInChat(playerRaceMsg);
        }
        else if (raceCount > 0) {
            char raceMsg[256];
            snprintf(raceMsg, sizeof(raceMsg),
                "RACES loaded: %zu race(s) available, but you are not in any race",
                raceCount);
            displayInChat(raceMsg);
        }
    }

    // Display error messages if sections failed to load | Afficher les messages d'erreur si les sections n'ont pas chargé
    if (!globalSuccess) {
        displayInChat("ERROR: [GLOBAL] section not found or failed to load - using defaults");
    }

    if (!racesSuccess && insideRacesSection) {
        displayInChat("ERROR: [RACE] section found but no valid races parsed");
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
    hubDescriptionMonitorRunning = TRUE;
    Sleep(2000);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Hub description monitor: PERMANENT monitoring active");
    }

    while (enableGetPlayerCoordinates && channelManagementRunning) {
        readHubDescription();
        Sleep(2000);
    }

    hubDescriptionMonitorRunning = FALSE;
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
    channelManagementRunning = TRUE;
    Sleep(3000);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Channel management thread: PERMANENT monitoring active");
    }

    while (enableGetPlayerCoordinates && hubDescriptionMonitorRunning) {
        checkConnectionStatus();

        if (enableAutomaticChannelChange) {
            manageChannelBasedOnCoordinates();
        }

        Sleep(500);
    }

    channelManagementRunning = FALSE;
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
    // Distance max propre à la voix → définit la “force” intrinsèque
    float effectiveVoiceDistance = voiceDistance;
    if (currentPlayerRaceIndex != -1 && currentListenAddDistance > 0.0f) {
        effectiveVoiceDistance += currentListenAddDistance;
    }

    // Paramètres de zone
    float minDistance = (float)hubAudioMinDistance;
    float maxVolumeFromServer = ((float)hubAudioMaxVolume / 100.0f);
    if (currentZoneIndex != -1) {
        minDistance = (float)zones[currentZoneIndex].audioMinDistance;
        maxVolumeFromServer = (float)(zones[currentZoneIndex].audioMaxVolume / 100.0f);
    }

    if (distance < 0.1f) distance = 0.1f;
    if (minDistance < 0.1f) minDistance = 0.1f;

    // Distance relative
    float normalizedDistance = distance / effectiveVoiceDistance;
    if (normalizedDistance >= 1.0f) return 0.0f;

    // Base volume proportionnel à la force de la voix
    float baseVolume = fminf(0.5f + 0.5f * (voiceDistance / 50.0f), 1.0f);

    // Atténuation physique (inverse square modifié)
    float G_phys = 1.0f / (1.0f + 4.0f * normalizedDistance * normalizedDistance);

    // Near-field boost (non scientifique) pour sons proches
    float nearFieldBoost = 1.0f;
    if (distance < voiceDistance * 0.2f) {
        nearFieldBoost = 1.0f + 0.5f * (1.0f - normalizedDistance / 0.2f);
    }

    // Psychoacoustic logistic fade
    const float k = 6.0f;
    const float midpoint = 0.75f;
    float G_fade = 1.0f / (1.0f + expf(k * (normalizedDistance - midpoint)));

    // Directivity placeholder (full omnidirectional)
    float G_dir = 1.0f;

    // Gain combiné
    float volumeMultiplier = baseVolume * G_phys * nearFieldBoost * G_fade * G_dir;

    // Appliquer le volume global
    volumeMultiplier *= maxVolumeFromServer;

    // Clamp
    if (volumeMultiplier < 0.0f) volumeMultiplier = 0.0f;
    if (volumeMultiplier > maxVolumeFromServer) volumeMultiplier = maxVolumeFromServer;

    return volumeMultiplier;
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

// Calculate scientific low-pass cutoff frequency based on distance | Calculer la fréquence de coupure scientifique basée sur la distance
static float calculateScientificCutoffFrequency(float distance) {
    // Human voice air absorption model (simplified)
    // Based on ISO 9613-1 + speech intelligibility studies

    // Hard scientific limits
    const float MAX_CUTOFF = 8000.0f;   // close voice
    const float MIN_CUTOFF = 900.0f;    // far voice (still intelligible)

    float d = distance;
    if (d < 1.0f) d = 1.0f;

    // Logarithmic decay of high frequencies
    float cutoff = MAX_CUTOFF * expf(-0.15f * logf(d + 1.0f));

    // Clamp scientifically valid range
    if (cutoff < MIN_CUTOFF) cutoff = MIN_CUTOFF;
    if (cutoff > MAX_CUTOFF) cutoff = MAX_CUTOFF;

    return cutoff;
}

// Calculate Direct-to-Reverberant Ratio based on distance | Calculer le ratio direct/réverbéré basé sur la distance
static float calculateDRR(float distance, float minDistance) {
// Direct-to-Reverberant Ratio (DRR)
// Based on room acoustics approximation

    float d = distance;
    float d_ref = minDistance;

    // Scientifically accepted approximation
    float drr = 1.0f / (1.0f + (d * d) / (d_ref * d_ref));

    // Clamp
    if (drr < 0.05f) drr = 0.05f;
    if (drr > 1.0f) drr = 1.0f;

    return drr;
}

// Apply simple diffuse simulation (fake reverb for distance perception) | Appliquer une simulation diffuse simple
static void applyDiffuseSimulation(float* samples, uint32_t sampleCount, uint16_t channelCount, float drr) {
    if (!samples || sampleCount < 2 || drr >= 0.99f) return;

    // Direct gain = DRR, diffuse gain = 1 - DRR | Gain direct = DRR, gain diffus = 1 - DRR
    float directGain = drr;
    float diffuseGain = 1.0f - drr;

    // Simple diffuse simulation: average with previous samples | Simulation diffuse simple : moyenne avec échantillons précédents
    if (channelCount == 1) {
        float prevSample = samples[0];
        for (uint32_t i = 1; i < sampleCount; i++) {
            float diffuse = (prevSample + samples[i]) * 0.5f;
            float direct = samples[i];
            prevSample = samples[i];
            samples[i] = direct * directGain + diffuse * diffuseGain;
        }
    }
    else if (channelCount == 2) {
        float prevLeft = samples[0];
        float prevRight = samples[1];
        for (uint32_t sample = 1; sample < sampleCount; sample++) {
            uint32_t leftIdx = sample * 2;
            uint32_t rightIdx = sample * 2 + 1;

            float diffuseLeft = (prevLeft + samples[leftIdx]) * 0.5f;
            float diffuseRight = (prevRight + samples[rightIdx]) * 0.5f;

            prevLeft = samples[leftIdx];
            prevRight = samples[rightIdx];

            samples[leftIdx] = samples[leftIdx] * directGain + diffuseLeft * diffuseGain;
            samples[rightIdx] = samples[rightIdx] * directGain + diffuseRight * diffuseGain;
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
    if (!audioState) return;

    audioState->targetVolume = baseVolume;
    audioState->leftVolume = leftVol;
    audioState->rightVolume = rightVol;
    audioState->lastUpdate = GetTickCount64();

    // Clamp
    audioState->targetVolume = fminf(fmaxf(audioState->targetVolume, 0.0f), 1.0f);
    audioState->leftVolume = fminf(fmaxf(audioState->leftVolume, 0.0f), 1.0f);
    audioState->rightVolume = fminf(fmaxf(audioState->rightVolume, 0.0f), 1.0f);
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
    if (!audioState) return false;

    // --- Distance ---
    float distance = 0.0f;
    float voiceDistance = 15.0f;
    float minDistance = (float)hubAudioMinDistance;
    bool hasDistanceData = false;

    float frontBack = 1.0f; // assume front by default

    for (size_t i = 0; i < adaptivePlayerCount; i++) {
        if (adaptivePlayerStates[i].userID == userID && adaptivePlayerStates[i].isValid) {

            Vector3 localPos = { localPlayerPosition.x, localPlayerPosition.y, localPlayerPosition.z };
            distance = (float)calculateDistance3D(&localPos, &adaptivePlayerStates[i].position);
            voiceDistance = adaptivePlayerStates[i].voiceDistance;

            // direction vector
            Vector3 dir = {
                adaptivePlayerStates[i].position.x - localPos.x,
                adaptivePlayerStates[i].position.y - localPos.y,
                adaptivePlayerStates[i].position.z - localPos.z
            };

            float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (len > 0.0001f) {
                dir.x /= len; dir.y /= len; dir.z /= len;
                frontBack = avatarAxisX * dir.x + avatarAxisY * dir.y + avatarAxisZ * dir.z;
            }

            hasDistanceData = true;
            break;
        }
    }

    if (!hasDistanceData) {
        distance = 5.0f;
        voiceDistance = 35.0f;
    }

    if (currentZoneIndex != -1) {
        minDistance = (float)zones[currentZoneIndex].audioMinDistance;
    }

    // --- Distance volume ---
    float distanceMultiplier = calculateVolumeMultiplierWithHubSettings(distance, voiceDistance);
    audioState->targetVolume = distanceMultiplier;

    audioState->currentVolume += (audioState->targetVolume - audioState->currentVolume) * 0.25f;

    // --- Directional psychoacoustics ---
    float rearFactor = 0.0f;
    if (frontBack < 0.0f) {
        rearFactor = fminf(-frontBack, 1.0f);
    }

    // Max 12% attenuation behind
    float directionVolume = 1.0f - (rearFactor * 0.12f);

    // --- Scientific filtering ---
    float cutoffHz = calculateScientificCutoffFrequency(distance);
    float drr = calculateDRR(distance, minDistance);

    if (rearFactor > 0.0f) {
        cutoffHz *= 0.75f;    // rear low-pass
        drr *= 0.85f;         // more diffuse
    }

    // --- APPLY FILTERS FIRST ---
    LowPassFilterState* filterState = findOrCreateLowPassState(userID);
    if (filterState) {
        applyLowPassFilter(outputPCM, sampleCount, channelCount, cutoffHz, sampleRate, filterState);
    }

    applyDiffuseSimulation(outputPCM, sampleCount, channelCount, drr);

    // --- APPLY VOLUME (CORRECT ORDER) ---
    float finalVolume = audioState->currentVolume * directionVolume;

    if (channelCount == 2) {

        float leftVolume = audioState->leftVolume * finalVolume;
        float rightVolume = audioState->rightVolume * finalVolume;

        for (uint32_t s = 0; s < sampleCount; s++) {
            outputPCM[s * 2] *= leftVolume;
            outputPCM[s * 2 + 1] *= rightVolume;
        }

    }
    else {
        for (uint32_t i = 0; i < sampleCount; i++) {
            outputPCM[i] *= finalVolume;
        }
    }

    // --- Debug ---
    if (enableLogGeneral) {
        static ULONGLONG lastLog = 0;
        ULONGLONG now = GetTickCount64();
        if (now - lastLog > 3000) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                "AUDIO FIXED: Dist=%.1fm FrontBack=%.2f Cut=%.0fHz DRR=%.2f Vol=%.0f%%",
                distance, frontBack, cutoffHz, drr, finalVolume * 100.0f);
            mumbleAPI.log(ownID, msg);
            lastLog = now;
        }
    }

    return true;
}

// ============================================================================
// MODULE 12 : SYSTÈME DE VOIX ET MODES
// ============================================================================

// Get voice distance for mode | Obtenir la distance de voix selon le mode
static float getVoiceDistanceForMode(uint8_t voiceMode) {
    float distance = distanceNormal; // Valeur par défaut

    // ✅ PRIORITÉ 1 : Zone (surcharge TOUT)
    if (currentZoneIndex != -1) {
        switch (voiceMode) {
        case 0: distance = zones[currentZoneIndex].whisperDist; break;
        case 1: distance = zones[currentZoneIndex].normalDist; break;
        case 2: distance = zones[currentZoneIndex].shoutDist; break;
        default: distance = zones[currentZoneIndex].normalDist; break;
        }

        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg),
                "getVoiceDistanceForMode: ZONE ACTIVE - Mode=%d, Distance=%.1f (from zone '%s')",
                voiceMode, distance, zones[currentZoneIndex].name);
            mumbleAPI.log(ownID, logMsg);
        }
        return distance;
    }

    // ✅ PRIORITÉ 2 : Paramètres globaux (plugin.cfg)
    switch (voiceMode) {
    case 0: distance = distanceWhisper; break;
    case 1: distance = distanceNormal; break;
    case 2: distance = distanceShout; break;
    default: distance = distanceNormal; break;
    }

    if (enableLogGeneral) {
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg),
            "getVoiceDistanceForMode: GLOBAL SETTINGS - Mode=%d, Distance=%.1f",
            voiceMode, distance);
        mumbleAPI.log(ownID, logMsg);
    }

    return distance;
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

// Send player coordinates to chat at 1-second intervals | Envoyer les coordonnées au chat toutes les secondes
static void broadcastPlayerCoordinates() {
    if (!f9CoordinateBroadcastActive) return;

    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastCoordinateBroadcast < 1000) {
        return; // Attendre au moins 1 seconde | Wait at least 1 second
    }

    lastCoordinateBroadcast = currentTime;

    // Use coordinatesValid flag instead of manual check | Utiliser le flag coordinatesValid au lieu de vérifier manuellement
    if (!coordinatesValid) {
        return;
    }

    // Convertir les coordonnées en mètres | Convert coordinates to meters
    float posX = axe_x / 100.0f;
    float posY = axe_y / 100.0f;
    float posZ = axe_z / 100.0f;

    // Formatter le message | Format the message
    char chatMessage[256];
    snprintf(chatMessage, sizeof(chatMessage),
        "📍 Position: X=%.6f Y=%.6f Z=%.6f",
        posX, posY, posZ);

    // Envoyer au chat | Send to chat
    displayInChat(chatMessage);
}

// Voice mode cycling function | Fonction pour cycliser entre les modes de voix
static void cycleVoiceMode() {
    if (!enableVoiceToggle) return;

    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastVoiceTogglePress < 300) return;
    lastVoiceTogglePress = currentTime;

    // Use the absolute truth stored globally | Utiliser la vérité absolue stockée globalement
    uint8_t newMode;
    switch (currentVoiceMode) {
    case 1: newMode = 2; break; // Normal -> Shout
    case 2: newMode = 0; break; // Shout -> Whisper
    case 0: newMode = 1; break; // Whisper -> Normal
    default: newMode = 1; break;
    }

    currentVoiceMode = newMode; // Update absolute truth | Mettre à jour la vérité absolue
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

    // Use the absolute truth stored globally | Utiliser la vérité absolue stockée globalement
    uint8_t lastVoiceMode = currentVoiceMode;

    // Detect F9 key press for coordinate broadcasting | Détecter la touche F9 pour diffuser les coordonnées
    static BOOL lastF9State = FALSE;
    BOOL currentF9State = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;

    // Rising edge detection - F9 just pressed | Détection de front montant - F9 vient d'être appuyé
    if (currentF9State && !lastF9State) {
        f9CoordinateBroadcastActive = !f9CoordinateBroadcastActive;
        if (enableLogCoordinates) {
            char msg[128];
            snprintf(msg, sizeof(msg), "F9 TOGGLE: Coordinate broadcasting %s",
                f9CoordinateBroadcastActive ? "ACTIVATED" : "DEACTIVATED");
            mumbleAPI.log(ownID, msg);
        }
    }
    lastF9State = currentF9State;

    // Send coordinates if broadcasting is active | Envoyer les coordonnées si la diffusion est active
    broadcastPlayerCoordinates();

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
                currentVoiceMode = newVoiceMode; // Update absolute truth | Mettre à jour la vérité absolue
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
            currentVoiceMode = newVoiceMode; // Update absolute truth | Mettre à jour la vérité absolue

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


// Send complete positional data at 15ms intervals | Envoi des données positionnelles complètes à 20ms
static void sendCompletePositionalData() {
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastVoiceDataSent < 20) return;
    lastVoiceDataSent = currentTime;

    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        return;
    }

    // CRITICAL FIX: Mettre à jour localVoiceData.voiceDistance AVANT envoi | Update voiceDistance BEFORE sending
    localVoiceData.voiceDistance = getVoiceDistanceForMode(currentVoiceMode);

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

    // Position locale
    Vector3 localPos = { axe_x / 100.0f, axe_y / 100.0f, axe_z / 100.0f };
    Vector3 remotePos = { remoteData->x, remoteData->y, remoteData->z };

    // Soundproof zone check (3D cube) | Vérification des zones soundproof (cube 3D)
    int localZone = getPlayerZone(localPos.x, localPos.y, localPos.z);
    int remoteZone = getPlayerZone(remotePos.x, remotePos.y, remotePos.z);

    // If local player is in a soundproof zone | Si le joueur local est dans une zone soundproof
    if (localZone != -1 && zones[localZone].isSoundproof) {
        if (enableLogGeneral) {
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg),
                "SOUNDPROOF: Local player in zone '%s' (soundproof=TRUE), remote in zone %d",
                zones[localZone].name, remoteZone);
            mumbleAPI.log(ownID, logMsg);
        }
        // If remote player is NOT in the same zone, mute completely | Si le joueur distant n'est PAS dans la même zone, mute complet
        if (remoteZone != localZone) {
            setUserAdaptiveVolumeWithSpatial(userID, 0.0f, 0.0f, 0.0f);
            return; // No audio from outside | Pas d'audio de l'extérieur
        }
    }

    // If remote player is in a soundproof zone | Si le joueur distant est dans une zone soundproof
    if (remoteZone != -1 && zones[remoteZone].isSoundproof) {
        // If local player is NOT in the same zone, mute completely | Si le joueur local n'est PAS dans la même zone, mute complet
        if (localZone != remoteZone) {
            setUserAdaptiveVolumeWithSpatial(userID, 0.0f, 0.0f, 0.0f);
            return; // No audio from inside | Pas d'audio de l'intérieur
        }
    }

    // Distance 3D
    float dx = remotePos.x - localPos.x;
    float dy = remotePos.y - localPos.y;
    float dz = remotePos.z - localPos.z;
    float distanceInMeters = sqrtf(dx * dx + dy * dy + dz * dz);

    // Direction vers le joueur distant
    float toRemoteX = dx, toRemoteY = dy, toRemoteZ = dz;
    float toRemoteLen = sqrtf(toRemoteX * toRemoteX + toRemoteY * toRemoteY + toRemoteZ * toRemoteZ);
    if (toRemoteLen > 1e-6f) {
        toRemoteX /= toRemoteLen; toRemoteY /= toRemoteLen; toRemoteZ /= toRemoteLen;
    }

    // Orientation locale (direction du regard)
    float localDirX = avatarAxisX;
    float localDirY = avatarAxisY;
    float localDirZ = avatarAxisZ;

    // Calcul avant/arrière
    float frontBack = localDirX * toRemoteX + localDirY * toRemoteY + localDirZ * toRemoteZ;

    // Calcul gauche/droite
    float leftRight = (localDirX * toRemoteZ - localDirZ * toRemoteX);
    frontBack = -frontBack; // Inverser front/back pour que face = volume fort, dos = volume faible

    // Paramètres panning
    const float MIN_PAN_THRESHOLD = 0.05f;
    const float MAX_PAN_INTENSITY = 0.95f;  // augmenté scientifiquement
    const float PAN_CURVE = 1.5f;

    float leftVolume = 1.0f;
    float rightVolume = 1.0f;

    // Gradual panning
    if (fabsf(leftRight) > MIN_PAN_THRESHOLD) {
        float panAmount = fminf(fabsf(leftRight), 1.0f);
        float smoothPan = powf(panAmount, 1.0f / PAN_CURVE);
        float attenuation = smoothPan * MAX_PAN_INTENSITY;

        if (leftRight > MIN_PAN_THRESHOLD) {
            leftVolume = 1.0f;
            rightVolume = 1.0f - attenuation;
            if (rightVolume < 0.15f) rightVolume = 0.15f;
        }
        else if (leftRight < -MIN_PAN_THRESHOLD) {
            rightVolume = 1.0f;
            leftVolume = 1.0f - attenuation;
            if (leftVolume < 0.15f) leftVolume = 0.15f;
        }
    }

    // Effet arrière — réduction des aigus et légèrement du volume
    if (frontBack < -0.2f) {
        float backFactor = fminf(fabsf(frontBack + 0.2f) / 0.8f, 1.0f);
        float backAttenuation = 0.55f + 0.45f * backFactor; // diminue vers 0.55 derrière
        leftVolume *= backAttenuation;
        rightVolume *= backAttenuation;
    }

    // Calcul volume principal
    float voiceVolume = calculateVolumeMultiplierWithHubSettings(distanceInMeters, remoteData->voiceDistance);
    float finalLeftVolume = leftVolume * voiceVolume;
    float finalRightVolume = rightVolume * voiceVolume;
    float avgVolume = (finalLeftVolume + finalRightVolume) * 0.5f;

    // Appliquer TRUE stereo
    setUserAdaptiveVolumeWithSpatial(userID, avgVolume, finalLeftVolume, finalRightVolume);
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
    // Use the absolute truth stored globally | Utiliser la vérité absolue stockée globalement
    switch (currentVoiceMode) {
    case 0: return "WHISPER";
    case 1: return "NORMAL";
    case 2: return "SHOUT";
    default: return "NORMAL";
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

        if (hAutomaticPatchFindCheck) ShowWindow(hAutomaticPatchFindCheck, SW_SHOW);
        HWND hAutomaticPatchFindLabel = GetDlgItem(hConfigDialog, 2004);
        if (hAutomaticPatchFindLabel) ShowWindow(hAutomaticPatchFindLabel, SW_SHOW);

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
        if (hSavedPathBg) ShowWindow(hSavedPathBg, SW_HIDE);

        // ✅ MASQUER la checkbox et le label Automatic Patch Find
        if (hAutomaticPatchFindCheck) ShowWindow(hAutomaticPatchFindCheck, SW_HIDE);
        HWND hAutomaticPatchFindLabel = GetDlgItem(hConfigDialog, 2004);
        if (hAutomaticPatchFindLabel) ShowWindow(hAutomaticPatchFindLabel, SW_HIDE);

        // Hide Automatic Patch Find controls | Masquer les contrôles Automatic Patch Find
        HWND hAutomaticPatchFindLabelHide3 = GetDlgItem(hConfigDialog, 2000);
        if (hAutomaticPatchFindLabelHide3) ShowWindow(hAutomaticPatchFindLabelHide3, SW_HIDE);
        if (hExplanation2) ShowWindow(hExplanation2, SW_HIDE);
        if (hExplanation3) ShowWindow(hExplanation3, SW_HIDE);
        if (hPathLabel) ShowWindow(hPathLabel, SW_HIDE);
        if (hSavedPathEdit) ShowWindow(hSavedPathEdit, SW_HIDE);
        if (hSavedPathButton) ShowWindow(hSavedPathButton, SW_HIDE);

        // Hide Automatic Patch Find controls in category 3 | Masquer les contrôles Automatic Patch Find en catégorie 3
        
        HWND hAutomaticPatchFindLabelHide = GetDlgItem(hConfigDialog, 2000);
        if (hAutomaticPatchFindLabelHide) ShowWindow(hAutomaticPatchFindLabelHide, SW_HIDE);

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
        if (hAutomaticPatchFindCheck) ShowWindow(hAutomaticPatchFindCheck, SW_HIDE);
        HWND hAutomaticPatchFindLabel = GetDlgItem(hConfigDialog, 2004);
        if (hAutomaticPatchFindLabel) ShowWindow(hAutomaticPatchFindLabel, SW_HIDE);
        if (hExplanation1) ShowWindow(hExplanation1, SW_HIDE);
        if (hExplanation2) ShowWindow(hExplanation2, SW_HIDE);
        if (hExplanation3) ShowWindow(hExplanation3, SW_HIDE);
        if (hPathLabel) ShowWindow(hPathLabel, SW_HIDE);
        if (hSavedPathEdit) ShowWindow(hSavedPathEdit, SW_HIDE);
        if (hSavedPathButton) ShowWindow(hSavedPathButton, SW_HIDE);
        // Hide Automatic Patch Find controls in category 3 | Masquer les contrôles Automatic Patch Find en catégorie 3
        HWND hAutomaticPatchFindLabelHide = GetDlgItem(hConfigDialog, 2000);
        if (hAutomaticPatchFindLabelHide) ShowWindow(hAutomaticPatchFindLabelHide, SW_HIDE);
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

    BOOL keyFound = FALSE;

    for (int vk = 1; vk < 256; vk++) {
        if (vk == 27) continue; // Skip ESC | Ignorer Echap
        if (GetAsyncKeyState(vk) & 0x8000) {
            // Touche détectée | Key detected
            switch (captureKeyTarget) {
            case 1: whisperKey = vk; if (hWhisperKeyEdit) SetWindowTextA(hWhisperKeyEdit, getKeyName(vk)); break;
            case 2: normalKey = vk; if (hNormalKeyEdit) SetWindowTextA(hNormalKeyEdit, getKeyName(vk)); break;
            case 3: shoutKey = vk; if (hShoutKeyEdit) SetWindowTextA(hShoutKeyEdit, getKeyName(vk)); break;
            case 4: configUIKey = vk; if (hConfigKeyEdit) SetWindowTextA(hConfigKeyEdit, getKeyName(vk)); break;
            case 5: voiceToggleKey = vk; if (hVoiceToggleKeyEdit) SetWindowTextA(hVoiceToggleKeyEdit, getKeyName(vk)); break;
            }

            isCapturingKey = FALSE;
            captureKeyTarget = 0;
            keyFound = TRUE;

            // Réactiver TOUS les boutons immédiatement | Re-enable ALL buttons immediately
            if (hWhisperButton) EnableWindow(hWhisperButton, TRUE);
            if (hNormalButton) EnableWindow(hNormalButton, TRUE);
            if (hShoutButton) EnableWindow(hShoutButton, TRUE);
            if (hConfigButton) EnableWindow(hConfigButton, TRUE);
            if (hVoiceToggleButton) EnableWindow(hVoiceToggleButton, TRUE);

            break;
        }
    }

    // ✅ CORRECTION : Ne pas attendre si aucune touche trouvée
    // Cette condition évite le blocage sur 200ms si l'utilisateur relâche avant la détection
    if (keyFound) {
        Sleep(100); // Attendre le relâchement | Wait for key release
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

// Find Steam installation path and parse libraries | Trouver le chemin d'installation Steam et parser les bibliothèques
static BOOL findConanExilesAutomatic(wchar_t* outPath, size_t pathSize) {
    if (!outPath || pathSize == 0) return FALSE;

    // Debug: Log the registry key access | Déboguer: Logger l'accès aux clés du registre
    if (enableLogConfig) {
        mumbleAPI.log(ownID, "DEBUG: Attempting to read Steam registry keys...");
    }

    HKEY hKey = NULL;
    // Try 64-bit registry first | Essayer le registre 64-bit d'abord
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
        0, KEY_READ, &hKey);

    if (result != ERROR_SUCCESS) {
        // Try 32-bit registry | Essayer le registre 32-bit
        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Valve\\Steam",
            0, KEY_READ, &hKey);
    }

    if (result != ERROR_SUCCESS) {
        if (enableLogConfig) {
            char errorMsg[256];
            snprintf(errorMsg, sizeof(errorMsg),
                "Registry: Steam installation key not found - Error code: %ld", result);
            mumbleAPI.log(ownID, errorMsg);
        }
        return FALSE;
    }

    wchar_t installPath[MAX_PATH] = L"";
    DWORD dataSize = sizeof(installPath);
    result = RegQueryValueExW(hKey, L"InstallPath", NULL, NULL,
        (LPBYTE)installPath, &dataSize);

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || wcslen(installPath) == 0) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "Registry: Steam InstallPath value not found");
        }
        return FALSE;
    }

    // Build path to libraryfolders.vdf | Construire le chemin vers libraryfolders.vdf
    wchar_t vdfPath[MAX_PATH];
    swprintf(vdfPath, MAX_PATH, L"%s\\steamapps\\libraryfolders.vdf", installPath);

    // Debug: Log VDF path | Déboguer: Logger le chemin du fichier VDF
    if (enableLogConfig) {
        char vdfPathUtf8[MAX_PATH];
        size_t converted = 0;
        wcstombs_s(&converted, vdfPathUtf8, MAX_PATH, vdfPath, _TRUNCATE);
        char debugMsg[512];
        snprintf(debugMsg, sizeof(debugMsg),
            "DEBUG: Looking for VDF file at: %s", vdfPathUtf8);
        mumbleAPI.log(ownID, debugMsg);
    }

    // Debug: Check if VDF file exists | Déboguer: Vérifier si le fichier VDF existe
    DWORD vdfAttribs = GetFileAttributesW(vdfPath);
    if (vdfAttribs == INVALID_FILE_ATTRIBUTES) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "DEBUG: VDF file does NOT exist at this location");
        }
        return FALSE;
    }
    else {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "DEBUG: VDF file found - proceeding to parse");
        }
    }

    // ✅ CORRECTION : Parse VDF et retourne le chemin COMPLET incluant \ConanSandbox\Saved
    return parseSteamLibraryFolders(vdfPath, outPath, pathSize);
}

// Parse Steam libraryfolders.vdf file | Parser le fichier libraryfolders.vdf de Steam
static BOOL parseSteamLibraryFolders(const wchar_t* vdfPath, wchar_t* outConanPath, size_t pathSize) {
    if (!vdfPath || !outConanPath || pathSize == 0) return FALSE;

    FILE* file = _wfopen(vdfPath, L"r, ccs=UTF-8");
    if (!file) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "DEBUG: Failed to open libraryfolders.vdf");
        }
        return FALSE;
    }

    if (enableLogConfig) {
        mumbleAPI.log(ownID, "DEBUG: Successfully opened libraryfolders.vdf - parsing...");
    }

    wchar_t line[1024];
    wchar_t currentLibraryPath[MAX_PATH] = L"";
    BOOL foundConanExiles = FALSE;
    int lineNumber = 0;
    int libraryDepth = 0; // ✅ Profondeur des accolades pour détecter les sections

    while (fgetws(line, 1024, file) && !foundConanExiles) {
        lineNumber++;
        wchar_t* p = line;

        // ✅ Nettoyer les espaces/tabs/retours à la ligne
        while (*p == L' ' || *p == L'\t' || *p == L'\r' || *p == L'\n') p++;

        // ✅ DÉTECTER LES ACCOLADES OUVRANTES/FERMANTES
        if (wcschr(p, L'{')) {
            libraryDepth++;
            if (enableLogConfig) {
                char logMsg[256];
                snprintf(logMsg, sizeof(logMsg), "DEBUG: Opening brace at line %d, depth=%d", lineNumber, libraryDepth);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        if (wcschr(p, L'}')) {
            libraryDepth--;

            // ✅ RÉINITIALISER LE CHEMIN QUAND ON SORT D'UNE BIBLIOTHÈQUE
            if (libraryDepth == 1) {
                if (enableLogConfig) {
                    mumbleAPI.log(ownID, "DEBUG: Exiting library section - resetting path");
                }
                currentLibraryPath[0] = L'\0';
            }
        }

        // ✅ CHERCHER "path" UNIQUEMENT SI DANS UNE BIBLIOTHÈQUE (depth >= 2)
        if (libraryDepth >= 2 && wcsncmp(p, L"\"path\"", 6) == 0) {
            if (enableLogConfig) {
                char logMsg[256];
                snprintf(logMsg, sizeof(logMsg), "DEBUG: Found 'path' line at line %d (depth=%d)", lineNumber, libraryDepth);
                mumbleAPI.log(ownID, logMsg);
            }

            // ✅ MÉTHODE ROBUSTE : Chercher les guillemets correctement
            wchar_t* firstQuote = wcschr(p, L'\"');       // Premier guillemet de "path"
            if (!firstQuote) continue;

            wchar_t* secondQuote = wcschr(firstQuote + 1, L'\"'); // Deuxième guillemet de path"
            if (!secondQuote) continue;

            // ✅ APRÈS "path", chercher le PROCHAIN guillemet ouvrant (après espaces/tabs)
            wchar_t* searchPos = secondQuote + 1;
            while (*searchPos && (*searchPos == L' ' || *searchPos == L'\t')) searchPos++;

            wchar_t* thirdQuote = wcschr(searchPos, L'\"'); // Premier guillemet du chemin
            if (!thirdQuote) continue;

            wchar_t* fourthQuote = wcschr(thirdQuote + 1, L'\"'); // Deuxième guillemet du chemin
            if (!fourthQuote) continue;

            // ✅ Extraire le chemin entre thirdQuote et fourthQuote
            wchar_t* pathStart = thirdQuote + 1;
            size_t pathLen = fourthQuote - pathStart;

            if (pathLen == 0 || pathLen >= MAX_PATH) continue;

            wcsncpy_s(currentLibraryPath, MAX_PATH, pathStart, pathLen);
            currentLibraryPath[pathLen] = L'\0';

            // ✅ TRIM : Supprimer espaces/tabs au DÉBUT
            wchar_t* trimStart = currentLibraryPath;
            while (*trimStart == L' ' || *trimStart == L'\t') trimStart++;

            // ✅ TRIM : Supprimer espaces/tabs à la FIN
            wchar_t* trimEnd = currentLibraryPath + wcslen(currentLibraryPath) - 1;
            while (trimEnd > trimStart && (*trimEnd == L' ' || *trimEnd == L'\t')) {
                *trimEnd = L'\0';
                trimEnd--;
            }

            // Si après trim on a une chaîne vide, ignorer
            if (wcslen(trimStart) == 0) continue;

            // Copier le chemin trimmé vers le début du buffer
            if (trimStart != currentLibraryPath) {
                wcscpy_s(currentLibraryPath, MAX_PATH, trimStart);
            }

            // ✅ NETTOYER LES DOUBLES BACKSLASHES (\\\\  →  \\)
            wchar_t cleanPath[MAX_PATH] = L"";
            size_t j = 0;
            for (size_t i = 0; i < wcslen(currentLibraryPath) && j < MAX_PATH - 1; i++) {
                cleanPath[j++] = currentLibraryPath[i];
                if (currentLibraryPath[i] == L'\\' && currentLibraryPath[i + 1] == L'\\') {
                    i++; // Sauter le second backslash
                }
            }
            cleanPath[j] = L'\0';
            wcscpy_s(currentLibraryPath, MAX_PATH, cleanPath);

            if (enableLogConfig) {
                char pathUtf8[MAX_PATH];
                size_t converted = 0;
                wcstombs_s(&converted, pathUtf8, MAX_PATH, currentLibraryPath, _TRUNCATE);
                char logMsg[512];
                snprintf(logMsg, sizeof(logMsg), "DEBUG: Extracted library path: '%s'", pathUtf8);
                mumbleAPI.log(ownID, logMsg);
            }
        }

        // ✅ CHERCHER "440900" (Conan Exiles) SI UN CHEMIN EST STOCKÉ
        if (wcslen(currentLibraryPath) > 0 && wcsncmp(p, L"\"440900\"", 8) == 0) {
            if (enableLogConfig) {
                char logMsg[256];
                snprintf(logMsg, sizeof(logMsg), "DEBUG: Found Conan Exiles (440900) at line %d", lineNumber);
                mumbleAPI.log(ownID, logMsg);
            }

            // ✅ CONSTRUIRE LE CHEMIN COMPLET
            swprintf(outConanPath, pathSize, L"%s\\steamapps\\common\\Conan Exiles\\ConanSandbox\\Saved",
                currentLibraryPath);

            if (enableLogConfig) {
                char pathUtf8[MAX_PATH];
                size_t converted = 0;
                wcstombs_s(&converted, pathUtf8, MAX_PATH, outConanPath, _TRUNCATE);
                char logMsg[512];
                snprintf(logMsg, sizeof(logMsg), "DEBUG: Testing path: %s", pathUtf8);
                mumbleAPI.log(ownID, logMsg);
            }

            // ✅ VÉRIFIER QUE LE DOSSIER EXISTE
            DWORD attribs = GetFileAttributesW(outConanPath);
            if (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY)) {
                foundConanExiles = TRUE;
                if (enableLogConfig) {
                    mumbleAPI.log(ownID, "DEBUG: Path VERIFIED - folder exists!");
                }
            }
            else {
                if (enableLogConfig) {
                    char logMsg[256];
                    snprintf(logMsg, sizeof(logMsg), "DEBUG: Path INVALID - GetFileAttributesW returned: %lu", attribs);
                    mumbleAPI.log(ownID, logMsg);
                }
            }
        }
    }

    fclose(file);

    if (foundConanExiles) {
        if (enableLogConfig) {
            char successMsg[512];
            size_t converted = 0;
            char pathUtf8[MAX_PATH];
            wcstombs_s(&converted, pathUtf8, MAX_PATH, outConanPath, _TRUNCATE);
            snprintf(successMsg, sizeof(successMsg),
                "SUCCESS: Conan Exiles found at: %s", pathUtf8);
            mumbleAPI.log(ownID, successMsg);
        }
        return TRUE;
    }
    else {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "DEBUG: Conan Exiles NOT FOUND in any library");
        }
        return FALSE;
    }
}

// Read Steam ID from Windows Registry | Lire le Steam ID depuis le registre Windows
static BOOL readSteamIDFromRegistry(uint64_t* outSteamID) {
    if (!outSteamID) return FALSE;

    HKEY hKey = NULL;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\Valve\\Steam\\ActiveProcess",
        0, KEY_READ, &hKey);

    if (result != ERROR_SUCCESS) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Registry: Steam ActiveProcess key not found - Steam may not be running");
        }
        return FALSE;
    }

    DWORD activeUser = 0;
    DWORD dataSize = sizeof(DWORD);
    result = RegQueryValueExW(hKey, L"ActiveUser", NULL, NULL,
        (LPBYTE)&activeUser, &dataSize);

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || activeUser == 0) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Registry: ActiveUser value not found or invalid");
        }
        return FALSE;
    }

    // Convert AccountID (32-bit) to SteamID64 | Convertir AccountID (32-bit) en SteamID64
    *outSteamID = 76561197960265728ULL + (uint64_t)activeUser;

    if (enableLogGeneral) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Registry: Steam ID retrieved successfully - AccountID: %lu, SteamID64: %llu",
            activeUser, *outSteamID);
        mumbleAPI.log(ownID, logMsg);
    }

    return TRUE;
}

// Load default settings from config file | Charger les paramètres par défaut depuis le fichier de config
static void loadDefaultSettingsFromConfig() {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\default_settings.cfg", configFolder);

    FILE* f = _wfopen(configFile, L"r");
    if (!f) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "Default settings file not found - will be created on first connection");
        }
        return;
    }

    wchar_t line[512];
    while (fgetws(line, 512, f)) {
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

        if (wcsncmp(key, L"ServerConfigHash", 16) == 0) {
            size_t converted = 0;
            wcstombs_s(&converted, serverConfigHash, sizeof(serverConfigHash), val, _TRUNCATE);
        }
        else if (wcsncmp(key, L"HasAppliedDefaultSettings", 25) == 0) {
            hasAppliedDefaultSettings = (wcsncmp(val, L"true", 4) == 0);
        }
        else if (wcsncmp(key, L"DefaultWhisperKey", 17) == 0) {
            defaultWhisperKey = _wtoi(val);
        }
        else if (wcsncmp(key, L"DefaultNormalKey", 16) == 0) {
            defaultNormalKey = _wtoi(val);
        }
        else if (wcsncmp(key, L"DefaultShoutKey", 15) == 0) {
            defaultShoutKey = _wtoi(val);
        }
        else if (wcsncmp(key, L"DefaultVoiceToggleKey", 21) == 0) {
            defaultVoiceToggleKey = _wtoi(val);
        }
        else if (wcsncmp(key, L"DefaultDistanceWhisper", 22) == 0) {
            defaultDistanceWhisper = (float)_wtof(val);
        }
        else if (wcsncmp(key, L"DefaultDistanceNormal", 21) == 0) {
            defaultDistanceNormal = (float)_wtof(val);
        }
        else if (wcsncmp(key, L"DefaultDistanceShout", 20) == 0) {
            defaultDistanceShout = (float)_wtof(val);
        }
    }

    fclose(f);

    if (enableLogConfig) {
        mumbleAPI.log(ownID, "Default settings loaded from config file");
    }
}

// Save default settings to config file ONLY if feature is enabled | Sauvegarder les paramètres par défaut UNIQUEMENT si la fonctionnalité est activée
static void saveDefaultSettingsToConfig() {
    // Ne sauvegarder que si la fonctionnalité est activée | Only save if feature is enabled
    if (!enableDefaultSettingsOnFirstConnection) {
        if (enableLogConfig) {
            mumbleAPI.log(ownID, "Default settings NOT saved - feature disabled (enableDefaultSettingsOnFirstConnection=false)");
        }
        return;
    }

    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\default_settings.cfg", configFolder);

    FILE* f = _wfopen(configFile, L"w");
    if (!f) return;

    fwprintf(f, L"# Default Settings Configuration | Configuration des paramètres par défaut\n");
    fwprintf(f, L"# This file tracks server configuration hash and default settings applied\n\n");

    fwprintf(f, L"ServerConfigHash=%S\n", serverConfigHash);
    fwprintf(f, L"HasAppliedDefaultSettings=%s\n", hasAppliedDefaultSettings ? L"true" : L"false");
    fwprintf(f, L"\n");

    fwprintf(f, L"# Default suggested keys | Touches par défaut suggérées\n");
    fwprintf(f, L"DefaultWhisperKey=%d\n", defaultWhisperKey);
    fwprintf(f, L"DefaultNormalKey=%d\n", defaultNormalKey);
    fwprintf(f, L"DefaultShoutKey=%d\n", defaultShoutKey);
    fwprintf(f, L"DefaultVoiceToggleKey=%d\n", defaultVoiceToggleKey);
    fwprintf(f, L"\n");

    fwprintf(f, L"# Default suggested distances (meters) | Distances par défaut suggérées (mètres)\n");
    fwprintf(f, L"DefaultDistanceWhisper=%.1f\n", defaultDistanceWhisper);
    fwprintf(f, L"DefaultDistanceNormal=%.1f\n", defaultDistanceNormal);
    fwprintf(f, L"DefaultDistanceShout=%.1f\n", defaultDistanceShout);

    fclose(f);

    if (enableLogConfig) {
        mumbleAPI.log(ownID, "Default settings saved to config file");
    }
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
        wchar_t automaticPathFromConfig[MAX_PATH] = L""; // ✅ Pour stocker le chemin AUTOMATIQUE

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

                        // ✅ Stocker le chemin MANUEL depuis la config
                        wcscpy_s(savedPathFromConfig, MAX_PATH, pathStart);
                    }
                }
                fclose(f);
            }
        }

        if (wcslen(savedPathFromConfig) > 0) {
            // Mode MANUEL : afficher le chemin manuel
            wcscpy_s(displayedPathText, MAX_PATH, savedPathFromConfig);
            wchar_t* conanSandbox = wcsstr(displayedPathText, L"\\ConanSandbox\\Saved");
            if (conanSandbox) {
                *conanSandbox = L'\0'; // Tronquer pour affichage
            }
        }
        else {
            // Aucun chemin disponible : utiliser les valeurs par défaut
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

        // Load checkmark icon from resources | Charger l'icône checkmark depuis les ressources
        HMODULE hModuleCheckmark = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)ConfigDialogProc, &hModuleCheckmark);

        HICON hCheckIcon = (HICON)LoadImageW(hModuleCheckmark, MAKEINTRESOURCEW(IDI_CHECKMARK),
            IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

        // Automatic patch find checkbox | Checkbox pour la recherche automatique de patch
        hAutomaticPatchFindCheck = CreateWindowW(L"BUTTON", L"",
            WS_CHILD | BS_AUTOCHECKBOX,  // ✅ ENLEVER WS_VISIBLE
            60, 220, 20, 20,
            hwnd, (HMENU)200, GetModuleHandle(NULL), NULL);

        // Set checkmark icon if loaded | Définir l'icône checkmark si chargée
        if (hCheckIcon) {
            SendMessage(hAutomaticPatchFindCheck, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hCheckIcon);
        }

        HWND hAutomaticPatchFindLabel = CreateWindowW(L"STATIC", L"Automatic Patch Find",
            WS_CHILD | SS_LEFT | SS_NOTIFY | WS_DISABLED,  // ✅ ENLEVER WS_VISIBLE
            80, 222, 400, 20,
            hwnd, (HMENU)2004, NULL, NULL);
        ApplyFontToControl(hAutomaticPatchFindLabel, hFont);

        SetWindowSubclass(hAutomaticPatchFindLabel, CheckboxLabelProc, 200, (DWORD_PTR)hAutomaticPatchFindCheck);

        // Set checkbox state from config | Définir l'état de la checkbox depuis la config
        // Default to TRUE if not yet configured | Par défaut TRUE si non configuré
        CheckDlgButton(hwnd, 200, enableAutomaticPatchFind ? BST_CHECKED : BST_UNCHECKED);
        ShowWindow(hAutomaticPatchFindCheck, SW_SHOW);

        hSavedPathButton = CreateWindowW(L"BUTTON", L"Browse",
            WS_CHILD | BS_OWNERDRAW,
            browseX, browseY, browseBtnWidth, browseBtnHeight, hwnd, (HMENU)105, NULL, NULL);
        ApplyFontToControl(hSavedPathButton, hFontBold);

        // ========== CATÉGORIE 2 : ADVANCED OPTIONS ==========

        // === CHECKBOX 1 : Distance-based muting ===
        hEnableDistanceMutingCheck = CreateWindowW(L"BUTTON", L"",
            WS_CHILD | BS_AUTOCHECKBOX,
            60, 145, 20, 20, hwnd, (HMENU)201, NULL, NULL);

        if (hCheckIcon) {
            SendMessage(hEnableDistanceMutingCheck, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hCheckIcon);
        }

        HWND hDistanceMutingLabel = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT | SS_NOTIFY,
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

        // Set checkbox labels | Définir les labels des checkboxes
        HWND hAutomaticPatchFindLabelText = GetDlgItem(hwnd, 2000);
        if (hAutomaticPatchFindLabelText) SetWindowTextW(hAutomaticPatchFindLabelText, L"Automatic Patch Find");

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

        case 105:
            if (enableAutomaticPatchFind) {
                // Automatic patch find is enabled | Automatic patch find est activé
                MessageBoxW(hwnd,
                    L"Automatic Patch Find is currently enabled.\n\n"
                    L"To use manual mode and browse for a custom patch location:\n"
                    L"1. Uncheck 'Automatic Patch Find' in the Patch Configuration tab\n"
                    L"2. Click 'Save Configuration'\n"
                    L"3. Then use Browse to select your custom patch location",
                    L"Manual Mode Disabled", MB_OK | MB_ICONWARNING);
            }
            else {
                browseSavedPath(hwnd);
            }
            break;

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

        case 200: // Automatic Patch Find checkbox
            if (HIWORD(wParam) == BN_CLICKED) {
                enableAutomaticPatchFind = (IsDlgButtonChecked(hwnd, 200) == BST_CHECKED);

                if (enableAutomaticPatchFind) {
                    // Find automatic path | Trouver le chemin automatique
                    wchar_t registryPath[MAX_PATH] = L"";
                    if (findConanExilesAutomatic(registryPath, MAX_PATH)) {
                        // Afficher le chemin automatique trouvé (SANS \ConanSandbox\Saved)
                        wcscpy_s(displayedPathText, MAX_PATH, registryPath);
                        wchar_t* conanSandbox = wcsstr(displayedPathText, L"\\ConanSandbox\\Saved");
                        if (conanSandbox) {
                            *conanSandbox = L'\0';
                        }

                        if (hSavedPathBg && IsWindow(hSavedPathBg)) {
                            InvalidateRect(hSavedPathBg, NULL, TRUE);
                            UpdateWindow(hSavedPathBg);
                        }
                        showStatusMessage(L"Automatic path found - Click Save to apply", FALSE);
                    }
                    else {
                        wcscpy_s(displayedPathText, MAX_PATH, L"(Not found)");
                        if (hSavedPathBg && IsWindow(hSavedPathBg)) {
                            InvalidateRect(hSavedPathBg, NULL, TRUE);
                            UpdateWindow(hSavedPathBg);
                        }
                        showStatusMessage(L"Could not find Conan Exiles - Check Steam installation", TRUE);
                    }
                }
                else {
                    // MODE MANUEL : Charger le chemin MANUEL depuis la config
                    wchar_t* configFolder = getConfigFolderPath();
                    if (configFolder) {
                        wchar_t configFile[MAX_PATH];
                        swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);
                        FILE* fRead = _wfopen(configFile, L"r");
                        if (fRead) {
                            wchar_t line[512];
                            while (fgetws(line, 512, fRead)) {
                                if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                                    wchar_t* pathStart = line + 10;
                                    wchar_t* nl = wcschr(pathStart, L'\n');
                                    if (nl) *nl = L'\0';
                                    wchar_t* cr = wcschr(pathStart, L'\r');
                                    if (cr) *cr = L'\0';

                                    wcscpy_s(displayedPathText, MAX_PATH, pathStart);
                                    wchar_t* conanSandbox = wcsstr(displayedPathText, L"\\ConanSandbox\\Saved");
                                    if (conanSandbox) {
                                        *conanSandbox = L'\0';
                                    }
                                    break;
                                }
                            }
                            fclose(fRead);
                        }
                    }

                    if (wcslen(displayedPathText) == 0) {
                        wcscpy_s(displayedPathText, MAX_PATH, L"No path configured");
                    }

                    if (hSavedPathBg && IsWindow(hSavedPathBg)) {
                        InvalidateRect(hSavedPathBg, NULL, TRUE);
                        UpdateWindow(hSavedPathBg);
                    }
                    showStatusMessage(L"Manual path displayed", FALSE);
                }
            }
            break;

        case 1: { // Save Configuration
            if (currentCategory == 1) {
                // === CATÉGORIE 1 : PATCH CONFIGURATION ===

                // ✅ CORRECTION : Construire le chemin COMPLET incluant ConanSandbox\Saved
                wchar_t pathToSave[MAX_PATH] = L"";

                if (enableAutomaticPatchFind) {
        // ✅ Mode automatique : REDEMANDER le chemin pour être sûr d'avoir le BON
        if (!findConanExilesAutomatic(pathToSave, MAX_PATH)) {
            // ❌ ÉCHEC SILENCIEUX
            showStatusMessage(L"⚠ Error: Could not find Conan Exiles automatically", TRUE);
            break;
        }

        // ✅ pathToSave contient maintenant le VRAI chemin Steam complet
        if (enableLogConfig) {
            char logMsg[512];
            size_t converted = 0;
            char pathUtf8[MAX_PATH];
            wcstombs_s(&converted, pathUtf8, MAX_PATH, pathToSave, _TRUNCATE);
            snprintf(logMsg, sizeof(logMsg),
                "✅ AUTOMATIC MODE: Using REAL Steam path: %s", pathUtf8);
            mumbleAPI.log(ownID, logMsg);
        }
    }
    else {
        // Mode manuel : utiliser displayedPathText
        if (wcslen(displayedPathText) == 0) {
            MessageBoxW(hwnd,
                L"Please select your Conan Exiles game folder using the Browse button.",
                L"Missing Path", MB_OK | MB_ICONWARNING);

            showStatusMessage(L"⚠ Error: No game path specified", TRUE);
            break;
        }

        // Construire le chemin complet (displayedPathText + \ConanSandbox\Saved)
        wcscpy_s(pathToSave, MAX_PATH, displayedPathText);
        wcscat_s(pathToSave, MAX_PATH, L"\\ConanSandbox\\Saved");

        if (enableLogConfig) {
            char logMsg[512];
            size_t converted = 0;
            char pathUtf8[MAX_PATH];
            wcstombs_s(&converted, pathUtf8, MAX_PATH, pathToSave, _TRUNCATE);
            snprintf(logMsg, sizeof(logMsg),
                "✅ MANUAL MODE: Using manual path: %s", pathUtf8);
            mumbleAPI.log(ownID, logMsg);
        }
    }

    // ✅ 2) Vérifier UNIQUEMENT que le dossier ConanSandbox\Saved existe
    DWORD savedAttribs = GetFileAttributesW(pathToSave);
    if (savedAttribs == INVALID_FILE_ATTRIBUTES || !(savedAttribs & FILE_ATTRIBUTE_DIRECTORY)) {
        wchar_t errorMsg[512];
        swprintf(errorMsg, 512,
            L"The folder 'ConanSandbox\\Saved' does not exist in:\n%s\n\n"
            L"Please verify:\n"
            L"1. This is your Conan Exiles game folder\n",
            pathToSave);

        MessageBoxW(hwnd, errorMsg, L"Folder Not Found", MB_OK | MB_ICONERROR);
        showStatusMessage(L"⚠ Error: ConanSandbox\\Saved folder not found", TRUE);
        break;
    }

    // ✅ 3) Toutes les vérifications passées → SAUVEGARDER
    wchar_t distWhisper[32], distNormal[32], distShout[32];
    swprintf(distWhisper, 32, L"%.1f", distanceWhisper);
    swprintf(distNormal, 32, L"%.1f", distanceNormal);
    swprintf(distShout, 32, L"%.1f", distanceShout);

    // Extraire le dossier du jeu (sans ConanSandbox\Saved) pour writeFullConfiguration
    wchar_t gameFolder[MAX_PATH];
    wcscpy_s(gameFolder, MAX_PATH, pathToSave);
    wchar_t* conanSandbox = wcsstr(gameFolder, L"\\ConanSandbox\\Saved");
    if (conanSandbox) {
        *conanSandbox = L'\0';
    }

    BOOL wasAlreadySaved = isPatchAlreadySaved();

    writeFullConfiguration(gameFolder, distWhisper, distNormal, distShout);

    // ✅ MISE À JOUR IMMÉDIATE DE L'AFFICHAGE APRÈS SAUVEGARDE
    if (enableAutomaticPatchFind) {
        // Afficher le chemin Steam dans l'interface
        wcscpy_s(displayedPathText, MAX_PATH, gameFolder);
        if (hSavedPathBg && IsWindow(hSavedPathBg)) {
            InvalidateRect(hSavedPathBg, NULL, TRUE);
            UpdateWindow(hSavedPathBg);
        }
    }

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
        wcstombs_s(&converted, savedPathUtf8, MAX_PATH, pathToSave, _TRUNCATE);

        snprintf(logMsg, sizeof(logMsg),
            "✅ SECURITY PASSED: Saved folder verified at: %s",
            savedPathUtf8);
        mumbleAPI.log(ownID, logMsg);
    }
}
           else if (currentCategory == 2) {
               // === CATÉGORIE 2 : ADVANCED OPTIONS (reste inchangé) ===
               enableDistanceMuting = (IsDlgButtonChecked(hwnd, 201) == BST_CHECKED);
               enableAutomaticChannelChange = (IsDlgButtonChecked(hwnd, 203) == BST_CHECKED);
               enableVoiceToggle = (IsDlgButtonChecked(hwnd, 204) == BST_CHECKED);

               wchar_t distWhisper[32], distNormal[32], distShout[32];
               GetWindowTextW(hDistanceWhisperEdit, distWhisper, 32);
               GetWindowTextW(hDistanceNormalEdit, distNormal, 32);
               GetWindowTextW(hDistanceShoutEdit, distShout, 32);

               distanceWhisper = (float)_wtof(distWhisper);
               distanceNormal = (float)_wtof(distNormal);
               distanceShout = (float)_wtof(distShout);

               wchar_t gameFolder[MAX_PATH] = L"";

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

        // Dans WM_DRAWITEM pour ctrlId == 1100
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

            // Convertir modFilePath de char* en wchar_t* pour affichage
            wchar_t displayPath[MAX_PATH] = L"";

            if (enableAutomaticPatchFind) {
                // Mode automatique : afficher le chemin automatique détecté
                wchar_t autoPath[MAX_PATH] = L"";
                if (findConanExilesAutomatic(autoPath, MAX_PATH)) {
                    // Supprimer \ConanSandbox\Saved pour n'afficher que le dossier du jeu
                    wcscpy_s(displayPath, MAX_PATH, autoPath);
                    wchar_t* conanSandbox = wcsstr(displayPath, L"\\ConanSandbox\\Saved");
                    if (conanSandbox) {
                        *conanSandbox = L'\0';
                    }
                }
                else {
                    wcscpy_s(displayPath, MAX_PATH, L"(Not found)");
                }
            }
            else {
                // Mode manuel : afficher displayedPathText (déjà sans \ConanSandbox\Saved)
                wcscpy_s(displayPath, MAX_PATH, displayedPathText);
            }

            if (wcslen(displayPath) == 0) {
                wcscpy_s(displayPath, MAX_PATH, L"(Not configured)");
            }

            if (wcslen(displayPath) > 0) {
                // Sélection de la police
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
                GetTextExtentPoint32W(hdc, displayPath, (int)wcslen(displayPath), &textSize);

                if (textSize.cx <= availWidth) {
                    // texte tient → on calcule une position exacte (pixel-perfect)
                    int textX = imageRect.left + (bmWidth - textSize.cx) / 2;
                    int textY = imageRect.top + (bmHeight - textSize.cy) / 2;

                    // ✅ Texte principal UNIQUEMENT (PAS d'ombre)
                    SetTextColor(hdc, RGB(255, 255, 255));
                    TextOutW(hdc, textX, textY, displayPath, (int)wcslen(displayPath));
                }
                else {
                    // trop long → utiliser DrawText avec DT_END_ELLIPSIS et centrer verticalement
                    RECT dtRect = imageRect;
                    dtRect.left += horizMargin;
                    dtRect.right -= horizMargin;

                    // Calcule la hauteur du texte (DT_CALCRECT) pour centrer verticalement
                    RECT calcRect = dtRect;
                    DrawTextW(hdc, displayPath, -1, &calcRect, DT_SINGLELINE | DT_CALCRECT | DT_END_ELLIPSIS);

                    int textH = calcRect.bottom - calcRect.top;
                    if (textH <= 0) textH = (bmHeight / 2);

                    // Positionner verticalement au centre
                    int top = imageRect.top + (bmHeight - textH) / 2;
                    dtRect.top = top;
                    dtRect.bottom = top + textH;

                    // ✅ Texte principal UNIQUEMENT (PAS d'ombre)
                    SetTextColor(hdc, RGB(255, 255, 255));
                    DrawTextW(hdc, displayPath, -1, &dtRect,
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

    // Try automatic patch find if enabled | Essayer le patch automatique si activé
    if (enableAutomaticPatchFind) {
        wchar_t automaticPath[MAX_PATH] = L"";
        if (findConanExilesAutomatic(automaticPath, MAX_PATH)) {
            wcscpy_s(savedPath, MAX_PATH, automaticPath);
            size_t converted = 0;
            wcstombs_s(&converted, modFilePath, MAX_PATH, automaticPath, _TRUNCATE);
            strcat_s(modFilePath, MAX_PATH, "\\Pos.txt");

            // Save automatic path to config immediately | Sauvegarder le chemin automatique immédiatement
            wchar_t gameFolder[MAX_PATH];
            wcscpy_s(gameFolder, MAX_PATH, automaticPath);
            wchar_t* conanSandbox = wcsstr(gameFolder, L"\\ConanSandbox\\Saved");
            if (conanSandbox) {
                *conanSandbox = L'\0';
            }

            wchar_t distWhisper[32], distNormal[32], distShout[32];
            swprintf(distWhisper, 32, L"%.1f", distanceWhisper);
            swprintf(distNormal, 32, L"%.1f", distanceNormal);
            swprintf(distShout, 32, L"%.1f", distanceShout);

            writeFullConfiguration(gameFolder, distWhisper, distNormal, distShout);

            if (enableLogConfig) {
                char logMsg[512];
                snprintf(logMsg, sizeof(logMsg), "Automatic patch found, applied and saved: %s", modFilePath);
                mumbleAPI.log(ownID, logMsg);
            }
        }
    }

    readConfigurationSettings();

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
                L"ACTIVE: Positional audio FORCED - MinDist=%.1f MaxDist=%.1f MaxVol=%.0f%% (Scientific model)",
                hubAudioMinDistance, hubAudioMaxDistance, hubAudioMaxVolume);
        }
        else {
            swprintf(message, 400, L"LOCKED: Positional audio: FORCED by server - enabling automatically");
        }
    }
    else {
        if (enableAutoAudioSettings) {
            swprintf(message, 400,
                L"OK: Positional audio enabled - MinDist=%.1f MaxDist=%.1f MaxVol=%.0f%% (Scientific model)",
                hubAudioMinDistance, hubAudioMaxDistance, hubAudioMaxVolume);
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

        if (currentZoneIndex != -1) {
            swprintf(whisperMsg, 300, L"Whisper: %.1f meters (ZONE: %S - Range: %.1f-%.1f)",
                distanceWhisper, zones[currentZoneIndex].name, zones[currentZoneIndex].whisperDist, zones[currentZoneIndex].whisperDist);
        }
        else if (!limitsActive) {
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

        if (currentZoneIndex != -1) {
            swprintf(normalMsg, 300, L"Normal: %.1f meters (ZONE: %S - Range: %.1f-%.1f)",
                distanceNormal, zones[currentZoneIndex].name, zones[currentZoneIndex].normalDist, zones[currentZoneIndex].normalDist);
        }
        else if (!limitsActive) {
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

        if (currentZoneIndex != -1) {
            swprintf(shoutMsg, 300, L"Shout: %.1f meters (ZONE: %S - Range: %.1f-%.1f)",
                distanceShout, zones[currentZoneIndex].name, zones[currentZoneIndex].shoutDist, zones[currentZoneIndex].shoutDist);
        }
        else if (!limitsActive) {
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

    // Apply server or zone limits | Appliquer les limites serveur ou zone
    if (currentZoneIndex != -1) {
        newWhisper = zones[currentZoneIndex].whisperDist;
        newNormal = zones[currentZoneIndex].normalDist;
        newShout = zones[currentZoneIndex].shoutDist;
    }
    else if (shouldApplyDistanceLimits()) {
        newWhisper = validateDistanceValue(distanceWhisper, (float)hubMinimumWhisper, (float)hubMaximumWhisper, "Whisper");
        newNormal = validateDistanceValue(distanceNormal, (float)hubMinimumNormal, (float)hubMaximumNormal, "Normal");
        newShout = validateDistanceValue(distanceShout, (float)hubMinimumShout, (float)hubMaximumShout, "Shout");
    }

    BOOL distanceChanged = FALSE;

    // Update UI fields according to current context (zone or user) | Mettre à jour les champs UI (zone ou utilisateur)
    if (hDistanceWhisperEdit) { wchar_t vt[32]; swprintf(vt, 32, L"%.1f", newWhisper); SetWindowTextW(hDistanceWhisperEdit, vt); }
    if (hDistanceNormalEdit) { wchar_t vt[32]; swprintf(vt, 32, L"%.1f", newNormal); SetWindowTextW(hDistanceNormalEdit, vt); }
    if (hDistanceShoutEdit) { wchar_t vt[32]; swprintf(vt, 32, L"%.1f", newShout); SetWindowTextW(hDistanceShoutEdit, vt); }

    // Only update global variables and save if NOT in a zone | Uniquement si HORS d'une zone
    if (currentZoneIndex == -1) {
        if (newWhisper != distanceWhisper) { distanceWhisper = newWhisper; distanceChanged = TRUE; }
        if (newNormal != distanceNormal) { distanceNormal = newNormal; distanceChanged = TRUE; }
        if (newShout != distanceShout) { distanceShout = newShout; distanceChanged = TRUE; }
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

// Check current player zone and update zone index | Vérifier la zone actuelle du joueur
static void checkCurrentZone() {
    if (!coordinatesValid) {
        currentZoneIndex = -1;
        return;
    }

    // Get player position in meters | Obtenir la position du joueur en mètres
    float playerX = axe_x / 100.0f;
    float playerY = axe_y / 100.0f;
    float playerZ = axe_z / 100.0f;

    // Check which zone the player is in using improved 4-coordinate polygon detection | Vérifier dans quelle zone le joueur se trouve avec la détection de polygone améliorée
    int newZoneIndex = getPlayerZone(playerX, playerY, playerZ);

    // Zone state changed | L'état de la zone a changé
    if (newZoneIndex != currentZoneIndex) {
        currentZoneIndex = newZoneIndex;

        // INSIDE ZONE | DANS LA ZONE
        if (currentZoneIndex != -1) {
            distanceWhisper = zones[currentZoneIndex].whisperDist;
            distanceNormal = zones[currentZoneIndex].normalDist;
            distanceShout = zones[currentZoneIndex].shoutDist;

            if (enableLogGeneral) {
                char logMsg[256];
                snprintf(logMsg, sizeof(logMsg),
                    "Zone entered: '%s' (Index: %d) - Whisper: %.1f, Normal: %.1f, Shout: %.1f",
                    zones[currentZoneIndex].name, currentZoneIndex,
                    zones[currentZoneIndex].whisperDist,
                    zones[currentZoneIndex].normalDist,
                    zones[currentZoneIndex].shoutDist);
                mumbleAPI.log(ownID, logMsg);
            }

            char chatMsg[256];
            snprintf(chatMsg, sizeof(chatMsg),
                "Entered Zone: '%s' - Audio override active", zones[currentZoneIndex].name);
            displayInChat(chatMsg);
        }
        // OUTSIDE ZONE | HORS DE LA ZONE
        else {
            loadVoiceDistancesFromConfig();

            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Zone left - Global settings restored");
            }

            displayInChat("Left Zone: Global settings restored");
        }

        // Update current voice distance | Mettre à jour la distance vocale actuelle
        localVoiceData.voiceDistance = getVoiceDistanceForMode(currentVoiceMode);

        // Update UI | Mettre à jour l'interface
        if (hConfigDialog && IsWindow(hConfigDialog)) {
            updateDynamicInterface();
        }

        // Apply distance changes to all players | Appliquer les changements de distance à tous les joueurs
        applyDistanceToAllPlayers();
    }
}

static void modFileWatcherThread(void* arg) {
    modFileWatcherRunning = TRUE;

    while (enableGetPlayerCoordinates && modFileWatcherRunning) {
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
            checkCurrentZone();
        }
        else if (!isModActive) {
            coordinatesValid = FALSE;
        }

        Sleep(20);
    }

    modFileWatcherRunning = FALSE;
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Mod file watcher thread: Stopped");
    }
}

// ============================================================================
// MODULE 19 : THREADS SYSTÈME
// ============================================================================


// High-frequency voice system thread | Thread du système de voix à haute fréquence
static void voiceSystemThread(void* arg) {
    voiceSystemRunning = TRUE;
    Sleep(2000);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "HIGH-FREQUENCY Voice system thread: Now active at 20ms intervals");
    }

    while (enableGetPlayerCoordinates && voiceSystemRunning) {
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

        Sleep(20);
    }

    voiceSystemRunning = FALSE;
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
    ownID = pluginID;
    setlocale(LC_NUMERIC, "C");

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "=== PLUGIN INITIALIZATION STARTED ===");
    }

    // ========== STEP 0: Check if interface should be opened | Vérifier si l'interface doit être ouverte ==========
    BOOL shouldOpenInterface = FALSE;
    wchar_t* configFolder = getConfigFolderPath();

    if (configFolder) {
        wchar_t configFile[MAX_PATH];
        swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

        DWORD fileAttribs = GetFileAttributesW(configFile);

        // Case 1: First installation (file doesn't exist) | Cas 1 : Première installation (fichier n'existe pas)
        if (fileAttribs == INVALID_FILE_ATTRIBUTES) {
            shouldOpenInterface = TRUE;
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "FIRST INSTALLATION: plugin.cfg not found - opening interface");
            }
        }
        else {
            // File exists - check if paths are VALID | Fichier existe - vérifier si les chemins sont VALIDES
            FILE* file = _wfopen(configFile, L"r");
            if (file) {
                wchar_t savedPath[MAX_PATH] = L"";
                wchar_t automaticPath[MAX_PATH] = L"";
                BOOL foundAutomaticPatchFind = FALSE;
                BOOL automaticPatchFindEnabled = FALSE;

                wchar_t line[512];
                while (fgetws(line, 512, file)) {
                    if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                        wchar_t* pathStart = line + 10;
                        wchar_t* nl = wcschr(pathStart, L'\n');
                        if (nl) *nl = L'\0';
                        wchar_t* cr = wcschr(pathStart, L'\r');
                        if (cr) *cr = L'\0';
                        wcscpy_s(savedPath, MAX_PATH, pathStart);
                    }
                    else if (wcsncmp(line, L"AutomaticSavedPath=", 19) == 0) {
                        wchar_t* pathStart = line + 19;
                        wchar_t* nl = wcschr(pathStart, L'\n');
                        if (nl) *nl = L'\0';
                        wchar_t* cr = wcschr(pathStart, L'\r');
                        if (cr) *cr = L'\0';
                        wcscpy_s(automaticPath, MAX_PATH, pathStart);
                    }
                    else if (wcsncmp(line, L"AutomaticPatchFind=", 19) == 0) {
                        wchar_t* value = line + 19;
                        while (*value == L' ' || *value == L'\t') value++;
                        foundAutomaticPatchFind = TRUE;
                        automaticPatchFindEnabled = (wcsncmp(value, L"true", 4) == 0 || wcsncmp(value, L"True", 4) == 0);
                    }
                }
                fclose(file);

                // Check if the ACTIVE path is INVALID | Vérifier si le chemin ACTIF est INVALIDE
                wchar_t pathToCheck[MAX_PATH] = L"";

                if (automaticPatchFindEnabled && wcslen(automaticPath) > 0) {
                    wcscpy_s(pathToCheck, MAX_PATH, automaticPath);
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "VALIDATION: Checking AUTOMATIC path");
                    }
                }
                else if (wcslen(savedPath) > 0) {
                    wcscpy_s(pathToCheck, MAX_PATH, savedPath);
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "VALIDATION: Checking MANUAL path");
                    }
                }

                // Case 2: Path is configured but INVALID | Cas 2 : Chemin configuré mais INVALIDE
                if (wcslen(pathToCheck) > 0) {
                    DWORD pathAttribs = GetFileAttributesW(pathToCheck);
                    if (pathAttribs == INVALID_FILE_ATTRIBUTES || !(pathAttribs & FILE_ATTRIBUTE_DIRECTORY)) {
                        shouldOpenInterface = TRUE;

                        char logMsg[512];
                        size_t converted = 0;
                        char pathUtf8[MAX_PATH];
                        wcstombs_s(&converted, pathUtf8, MAX_PATH, pathToCheck, _TRUNCATE);
                        snprintf(logMsg, sizeof(logMsg),
                            "INVALID PATH DETECTED: %s does not exist - opening interface",
                            pathUtf8);
                        mumbleAPI.log(ownID, logMsg);
                    }
                    else {
                        if (enableLogGeneral) {
                            mumbleAPI.log(ownID, "PATH VALID: Interface will NOT open");
                        }
                    }
                }
                else {
                    // No path configured at all | Aucun chemin configuré
                    shouldOpenInterface = TRUE;
                    if (enableLogGeneral) {
                        mumbleAPI.log(ownID, "NO PATH CONFIGURED: Opening interface");
                    }
                }
            }
        }
    }

    // ========== STEP 1: Reset all flags and states | Réinitialiser tous les flags et états ==========
    enableGetPlayerCoordinates = TRUE;
    overlayThreadRunning = FALSE;
    keyMonitorThreadRunning = FALSE;
    modFileWatcherRunning = FALSE;
    voiceSystemRunning = FALSE;
    channelManagementRunning = FALSE;
    hubDescriptionMonitorRunning = FALSE;

    // ========== STEP 2: Reset window handles | Réinitialiser les handles de fenêtre ==========
    hVoiceOverlay = NULL;
    hOverlayFont = NULL;
    hConfigDialog = NULL;
    hPresetSaveDialog = NULL;
    hPresetRenameDialog = NULL;

    // ========== STEP 3: Reset thread handles | Réinitialiser les handles de threads ==========
    modFileWatcherThreadHandle = NULL;
    voiceSystemThreadHandle = NULL;
    channelManagementThreadHandle = NULL;
    hubDescriptionMonitorThreadHandle = NULL;
    overlayMonitorThreadHandle = NULL;
    keyMonitorThread = NULL;

    // ========== STEP 4: Reset connection states | Réinitialiser les états de connexion ==========
    isConnectedToServer = FALSE;
    hubDescriptionAvailable = FALSE;
    hubLimitsActive = FALSE;
    coordinatesValid = FALSE;
    modDataValid = FALSE;

    // ========== STEP 5: Reset channel management | Réinitialiser la gestion des canaux ==========
    hubChannelID = -1;
    rootChannelID = -1;
    ingameChannelID = -1;
    lastTargetChannel = -1;
    lastValidChannel = -1;
    channelManagementActive = FALSE;

    // ========== STEP 6: Reset audio states | Réinitialiser les états audio ==========
    lastAudioSettingsApply = 0;
    audioVolumeCount = 0;
    memset(audioVolumeStates, 0, sizeof(audioVolumeStates));
    adaptivePlayerCount = 0;
    memset(adaptivePlayerStates, 0, sizeof(adaptivePlayerStates));
    playerMuteStateCount = 0;
    memset(playerMuteStates, 0, sizeof(playerMuteStates));
    lowPassStateCount = 0;
    memset(lowPassStates, 0, sizeof(lowPassStates));

    // ========== STEP 7: Reset voice system | Réinitialiser le système de voix ==========
    memset(&localVoiceData, 0, sizeof(CompletePositionalData));
    localVoiceData.voiceDistance = distanceNormal;
    remotePlayerCount = 0;
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    lastVoiceDataSent = 0;
    lastKeyCheck = 0;

    // ========== STEP 8: Reset races | Réinitialiser les races ==========
    raceCount = 0;
    memset(races, 0, sizeof(races));
    currentPlayerRaceIndex = -1;
    currentListenAddDistance = 0.0f;

    // ========== STEP 9: Reset mod file system | Réinitialiser le système de fichier mod ==========
    lastFileCheck = 0;
    lastSeq = -1;
    modDataValid = FALSE;

    // ========== STEP 10: Load configuration | Charger la configuration ==========
    loadVoiceDistancesFromConfig();

    // Try automatic patch find if enabled | Essayer le patch automatique si activé
    if (enableAutomaticPatchFind) {
        wchar_t automaticPath[MAX_PATH] = L"";
        if (findConanExilesAutomatic(automaticPath, MAX_PATH)) {
            wcscpy_s(savedPath, MAX_PATH, automaticPath);
            size_t converted = 0;
            wcstombs_s(&converted, modFilePath, MAX_PATH, automaticPath, _TRUNCATE);
            strcat_s(modFilePath, MAX_PATH, "\\Pos.txt");

            if (enableLogConfig) {
                char logMsg[512];
                snprintf(logMsg, sizeof(logMsg), "Automatic patch found and applied: %s", modFilePath);
                mumbleAPI.log(ownID, logMsg);
            }
        }
    }

    initializeVoicePresets();
    loadPresetsFromConfigFile();
    loadDefaultSettingsFromConfig();
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Init: Configuration loaded successfully");
    }

    // ========== STEP 11: Read Steam ID | Lire le Steam ID ==========
    if (readSteamIDFromRegistry(&steamID)) {
        char chatMsg[256];
        snprintf(chatMsg, sizeof(chatMsg),
            "Steam ID successfully retrieved: %llu", steamID);
        displayInChat(chatMsg);

        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg),
                "Steam ID initialized: %llu", steamID);
            mumbleAPI.log(ownID, logMsg);
        }
    }
    else {
        displayInChat("ERROR: Failed to retrieve Steam ID from Windows Registry. Please ensure Steam is running.");

        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "WARNING: Failed to retrieve Steam ID - some features may not work");
        }
    }

    // ========== STEP 12: Configure mod file path | Configurer le chemin du fichier mod ==========
    readConfigurationSettings();

    configFolder = getConfigFolderPath();
    if (configFolder) {
        wchar_t configFile[MAX_PATH];
        swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

        wchar_t savedPath[MAX_PATH] = L"";
        wchar_t automaticPath[MAX_PATH] = L"";

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
                    wcscpy_s(savedPath, MAX_PATH, pathStart);
                }
                else if (wcsncmp(line, L"AutomaticSavedPath=", 19) == 0) {
                    wchar_t* pathStart = line + 19;
                    wchar_t* nl = wcschr(pathStart, L'\n');
                    if (nl) *nl = L'\0';
                    wchar_t* cr = wcschr(pathStart, L'\r');
                    if (cr) *cr = L'\0';
                    wcscpy_s(automaticPath, MAX_PATH, pathStart);
                }
            }
            fclose(file);
        }

        wchar_t pathToUse[MAX_PATH] = L"";
        if (enableAutomaticPatchFind && wcslen(automaticPath) > 0) {
            wcscpy_s(pathToUse, MAX_PATH, automaticPath);
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Init: Using AUTOMATIC path for mod file");
            }
        }
        else if (wcslen(savedPath) > 0) {
            wcscpy_s(pathToUse, MAX_PATH, savedPath);
            if (enableLogGeneral) {
                mumbleAPI.log(ownID, "Init: Using MANUAL path for mod file");
            }
        }

        if (wcslen(pathToUse) > 0) {
            size_t converted = 0;
            wcstombs_s(&converted, modFilePath, MAX_PATH, pathToUse, _TRUNCATE);
            strcat_s(modFilePath, MAX_PATH, "\\Pos.txt");

            if (enableLogGeneral) {
                char msg[300];
                snprintf(msg, sizeof(msg), "Init: Configured modFilePath: %s", modFilePath);
                mumbleAPI.log(ownID, msg);
            }
        }
    }

    // ========== STEP 13: Show configuration dialog ONLY if needed | Afficher le dialogue UNIQUEMENT si nécessaire ==========
    if (shouldOpenInterface) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "OPENING INTERFACE: First installation or invalid path detected");
        }
        _beginthread(showPathSelectionDialogThread, 0, NULL);
    }
    else {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "INTERFACE NOT OPENED: Valid configuration found");
        }
    }

    // ========== STEP 14: Install key monitoring | Installer la surveillance des touches ==========
    installKeyMonitoring();

    if (enableLogGeneral) {
        char keyMsg[128];
        snprintf(keyMsg, sizeof(keyMsg), "Init: Config UI key set to: %s (VK:%d)",
            getKeyName(configUIKey), configUIKey);
        mumbleAPI.log(ownID, keyMsg);
    }

    // ========== STEP 15: Force complete initialization | Forcer l'initialisation complète ==========
    forceCompleteInitialization();

    // ========== STEP 16: Start all threads | Démarrer tous les threads ==========
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Init: Starting all monitoring threads...");
    }

    modFileWatcherThreadHandle = (HANDLE)_beginthread(modFileWatcherThread, 0, NULL);
    voiceSystemThreadHandle = (HANDLE)_beginthread(voiceSystemThread, 0, NULL);
    channelManagementThreadHandle = (HANDLE)_beginthread(channelManagementThread, 0, NULL);
    hubDescriptionMonitorThreadHandle = (HANDLE)_beginthread(hubDescriptionMonitorThread, 0, NULL);

    // ========== STEP 17: Create voice overlay if enabled | Créer l'overlay vocal si activé ==========
// ========== STEP 17: Create voice overlay if enabled | Créer l'overlay vocal si activé ==========
// Create overlay immediately if enabled, even if distance muting not yet enabled | Créer l'overlay immédiatement si activé, même si muting non activé
    if (enableVoiceOverlay) {
        createVoiceOverlay();
        overlayMonitorThreadHandle = (HANDLE)_beginthread(overlayMonitorThread, 0, NULL);

        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Init: Voice overlay created and monitor thread started");
        }
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "=== PLUGIN INITIALIZATION COMPLETE - All systems active ===");
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
    version.major = 6;
    version.minor = 4;
    version.patch = 4;

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
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "=== PLUGIN SHUTDOWN STARTED - Full cleanup ===");
    }

    // ========== STEP 1: Signal all threads to stop | Signaler à tous les threads de s'arrêter ==========
    enableGetPlayerCoordinates = FALSE;
    overlayThreadRunning = FALSE;
    keyMonitorThreadRunning = FALSE;
    modFileWatcherRunning = FALSE;
    voiceSystemRunning = FALSE;
    channelManagementRunning = FALSE;
    hubDescriptionMonitorRunning = FALSE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Shutdown: All thread stop flags set to FALSE");
    }

    // ========== STEP 2: Wait for threads to finish | Attendre que les threads se terminent ==========
    Sleep(1500);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Shutdown: Waited 1500ms for threads to stop");
    }

    // ========== STEP 3: Remove key monitoring | Supprimer la surveillance des touches ==========
    removeKeyMonitoring();

    // ========== STEP 4: Destroy windows | Détruire les fenêtres ==========
    if (hVoiceOverlay && IsWindow(hVoiceOverlay)) {
        DestroyWindow(hVoiceOverlay);
        hVoiceOverlay = NULL;
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Shutdown: Voice overlay destroyed");
        }
    }

    if (hConfigDialog && IsWindow(hConfigDialog)) {
        DestroyWindow(hConfigDialog);
        hConfigDialog = NULL;
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Shutdown: Config dialog destroyed");
        }
    }

    if (hPresetSaveDialog && IsWindow(hPresetSaveDialog)) {
        DestroyWindow(hPresetSaveDialog);
        hPresetSaveDialog = NULL;
    }

    if (hPresetRenameDialog && IsWindow(hPresetRenameDialog)) {
        DestroyWindow(hPresetRenameDialog);
        hPresetRenameDialog = NULL;
    }

    // ========== STEP 5: Delete fonts | Supprimer les polices ==========
    if (hFont) {
        DeleteObject(hFont);
        hFont = NULL;
    }
    if (hFontBold) {
        DeleteObject(hFontBold);
        hFontBold = NULL;
    }
    if (hFontLarge) {
        DeleteObject(hFontLarge);
        hFontLarge = NULL;
    }
    if (hFontEmoji) {
        DeleteObject(hFontEmoji);
        hFontEmoji = NULL;
    }
    if (hPathFont) {
        DeleteObject(hPathFont);
        hPathFont = NULL;
    }
    if (hOverlayFont) {
        DeleteObject(hOverlayFont);
        hOverlayFont = NULL;
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Shutdown: All fonts deleted");
    }

    // ========== STEP 6: Delete bitmaps | Supprimer les bitmaps ==========
    if (hBackgroundBitmap) {
        DeleteObject(hBackgroundBitmap);
        hBackgroundBitmap = NULL;
    }
    if (hBackgroundAdvancedBitmap) {
        DeleteObject(hBackgroundAdvancedBitmap);
        hBackgroundAdvancedBitmap = NULL;
    }
    if (hBackgroundPresetsBitmap) {
        DeleteObject(hBackgroundPresetsBitmap);
        hBackgroundPresetsBitmap = NULL;
    }
    if (hBackgroundSavePresetBitmap) {
        DeleteObject(hBackgroundSavePresetBitmap);
        hBackgroundSavePresetBitmap = NULL;
    }
    if (hBackgroundRenamePresetBitmap) {
        DeleteObject(hBackgroundRenamePresetBitmap);
        hBackgroundRenamePresetBitmap = NULL;
    }
    if (hPathBoxBitmap) {
        DeleteObject(hPathBoxBitmap);
        hPathBoxBitmap = NULL;
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Shutdown: All bitmaps deleted");
    }

    // ========== STEP 7: Free malloc allocations | Libérer les allocations malloc ==========
    if (lastHubDescriptionCache) {
        free(lastHubDescriptionCache);
        lastHubDescriptionCache = NULL;
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Shutdown: Hub description cache freed");
        }
    }

    // ========== STEP 8: Cleanup audio states | Nettoyer les états audio ==========
    cleanupAudioVolumeStates();
    cleanupAdaptivePlayerStates();
    cleanupPlayerMuteStates();

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Shutdown: Audio states cleaned up");
    }

    // ========== STEP 9: Reset voice system | Réinitialiser le système de voix ==========
    remotePlayerCount = 0;
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    memset(&localVoiceData, 0, sizeof(CompletePositionalData));

    // ========== STEP 10: Reset race data | Réinitialiser les données de race ==========
    raceCount = 0;
    memset(races, 0, sizeof(races));
    currentPlayerRaceIndex = -1;
    currentListenAddDistance = 0.0f;

    // ========== STEP 11: Reset channel management | Réinitialiser la gestion des canaux ==========
    hubChannelID = -1;
    rootChannelID = -1;
    ingameChannelID = -1;
    lastTargetChannel = -1;
    lastValidChannel = -1;
    channelManagementActive = FALSE;

    // ========== STEP 12: Reset connection states | Réinitialiser les états de connexion ==========
    isConnectedToServer = FALSE;
    hubDescriptionAvailable = FALSE;
    hubLimitsActive = FALSE;
    coordinatesValid = FALSE;
    modDataValid = FALSE;

    // ========== STEP 13: Reset all window handles | Réinitialiser tous les handles de fenêtre ==========
    hWhisperKeyEdit = NULL;
    hNormalKeyEdit = NULL;
    hShoutKeyEdit = NULL;
    hConfigKeyEdit = NULL;
    hWhisperButton = NULL;
    hNormalButton = NULL;
    hShoutButton = NULL;
    hConfigButton = NULL;
    hEnableDistanceMutingCheck = NULL;
    hEnableAutomaticChannelChangeCheck = NULL;
    hDistanceWhisperEdit = NULL;
    hDistanceNormalEdit = NULL;
    hDistanceShoutEdit = NULL;
    hSavedPathEdit = NULL;
    hSavedPathButton = NULL;
    hSavedPathBg = NULL;
    hCategoryPatch = NULL;
    hCategoryAdvanced = NULL;
    hCategoryPresets = NULL;
    hEnableVoiceToggleCheck = NULL;
    hVoiceToggleKeyEdit = NULL;
    hVoiceToggleButton = NULL;
    hStatusMessage = NULL;
    hDistanceLimitMessage = NULL;
    hDistanceWhisperMessage = NULL;
    hDistanceNormalMessage = NULL;
    hDistanceShoutMessage = NULL;
    hDistanceMutingMessage = NULL;
    hChannelSwitchingMessage = NULL;
    hPositionalAudioMessage = NULL;
    hDistanceServerLimitWhisper = NULL;
    hDistanceServerLimitNormal = NULL;
    hDistanceServerLimitShout = NULL;
    hVoiceText = NULL;

    // Reset preset handles | Réinitialiser les handles de presets
    for (int i = 0; i < MAX_VOICE_PRESETS; i++) {
        hPresetLabels[i] = NULL;
        hPresetLoadButtons[i] = NULL;
        hPresetRenameButtons[i] = NULL;
    }

    // ========== STEP 14: Reset interface state | Réinitialiser l'état de l'interface ==========
    isConfigDialogOpen = FALSE;
    isCapturingKey = FALSE;
    captureKeyTarget = 0;
    currentCategory = 1;
    isUpdatingInterface = FALSE;
    backgroundDrawn = FALSE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "=== PLUGIN SHUTDOWN COMPLETE - All resources freed ===");
    }
}

// Release Mumble resource | Libérer une ressource Mumble
void mumble_releaseResource(const void* pointer) {
    mumbleAPI.log(ownID, u8"Called mumble_releaseResource but expected that this never gets called -> Aborting");
    abort();
}
