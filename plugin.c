// Copyright The Mumble Developers. All rights reserved. 
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at https://www.mumble.info/LICENSE.

// The following conditions apply exclusively to the code authored by Dino_Rex and do not affect or modify the copyright or licensing terms of the original Mumble code.

// MIT License

// The following conditions apply exclusively to the code written by Dino_Rex 
// and do not affect or modify the copyright 
// or licensing terms of the original Mumble code.

// Dino_Rex Public License v1.1

// Copyright (c) 2025 Dino_Rex

// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and the associated documentation files (the "Software"), 
// to use the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, 
// and/or sublicense the Software, 
// and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:

// 1. Attribution: The Discord handle of the original source code creator 
// (Dino_Rex) must be cited wherever the plugin is published 
// as well as in the description of the compiled Mumble plugin.
// 2. Share-Alike: Any public distribution or deployment of this software, 
// or of any modified version of it (e.g., for use on a public Conan Exiles server), 
// requires that the complete corresponding source code be made publicly available 
// under the same terms as this license.
// 3. Free of Charge: This software and all its modified versions must remain 
// entirely free of charge. It is strictly forbidden to sell or monetize 
// the Software, whether directly or indirectly.
// 4. Donations: 
//    a) Voluntary donations to the official creator of the Mumble plugin for Conan Exiles 
//       (Dino_Rex) are allowed. 
//    b) Donations made to other persons (who have modified, published, or redistributed the Software) 
//       are also allowed, but a clear disclaimer must state that 
//       the donation is intended solely for that person and NOT for the official creator 
//       of the Mumble plugin for Conan Exiles (Dino_Rex).
// 5. Non-Removal of License Terms: The clauses of this license are mandatory 
//    and must remain included in any copy, distribution, or modified version of the Software. 
//    Removal, alteration, or omission of these clauses is strictly prohibited.
// 6. Disclaimer of Warranty: 
//    The Software is provided "as is", without any warranty of any kind, express or implied, 
//    including but not limited to warranties of merchantability, fitness for a particular purpose, 
//    and non-infringement. 
//    In no event shall the author (Dino_Rex) be liable for any claim, damages, 
//    or other liability, whether in an action of contract, tort, or otherwise, 
//    arising from, out of, or in connection with the Software or the use 
//    or other dealings in the Software.
// 7. Acceptance of Terms: By downloading, using, copying, modifying, merging, publishing, 
//    or distributing this Software (in whole or in part), you acknowledge that you have read, 
//    understood, and agreed to be bound by all the terms and conditions of this license.

#include "MumblePlugin_v_1_0_x.h"
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <process.h>
#include <ws2tcpip.h>
#include <wchar.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <locale.h>
#include <ctype.h>
#include "SqlLite\sqlite3.h"

#pragma comment(lib, "ws2_32.lib")

void showPathSelectionDialogThread(void* arg);
static int showConfigInterface();
static void readConfigurationSettings();
static const char* getKeyName(int vkCode);
static void saveVoiceSettings();
static void applyDistanceToAllPlayers();
static void retrieveServerMaximumAudioDistance();
LRESULT CALLBACK ConfigDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Server Configuration | Configuration du serveur
#define SERVER_IP "192.168.0.1" // Server IP address | Adresse IP de votre serveur
#define SERVER_PORT 1234 // Opened port | Port que vous avez ouvert
#define REQUIRED_VERSION "VERSION: 2.0.0" // Required plugin version | Version requise pour le plugin

// Log Control Variables | Variables pour contrôler l'activation des logs
static BOOL enableLogCoordinates = FALSE; // Enable coordinate and position logs | Active les logs pour les coordonnées et positions
static BOOL enableLogModFile = FALSE; // Enable mod file system logs (Pos.txt) | Active les logs pour le système de fichier mod (Pos.txt)
static BOOL enableLogOffsets = FALSE; // Enable memory offset and read logs | Active les logs pour les offsets mémoire et lectures
static BOOL enableLogConfig = FALSE; // Enable configuration and .cfg file logs | Active les logs pour la configuration et fichiers .cfg
static BOOL enableLogServer = FALSE; // Enable server connection and zone logs | Active les logs pour les connexions serveur et zones
static BOOL enableLogProcess = FALSE; // Enable process search and access logs | Active les logs pour la recherche et accès aux processus
static BOOL enableLogGeneral = FALSE; // Enable general and debug logs | Active les logs généraux et de debug

// Server Communication State | État de communication serveur
static volatile BOOL versionReceived = FALSE; // Version received from server flag | Indicateur de version reçue du serveur
static volatile BOOL zonesReceived = FALSE; // Zones received from server flag | Indicateur de zones reçues du serveur

// Function Control Variables | Variables pour contrôler l'activation des fonctions
static BOOL enableSetMaximumAudioDistance = FALSE; // Enable audio distance setting function | Active la fonction de réglage de distance audio
static BOOL enableCheckPlayerZone = FALSE; // Enable player zone checking function | Active la fonction de vérification de zone joueur
static BOOL enableCheckVersionThread = FALSE; // Enable version checking thread function | Active la fonction de thread de vérification de version
static BOOL enableStartVersionCheck = FALSE; // Enable version check start function | Active la fonction de démarrage de vérification de version
static BOOL enableFindProcessId = FALSE; // Enable process ID finding function | Active la fonction de recherche d'ID de processus
static BOOL enableFindBaseAddress = FALSE; // Enable base address finding function | Active la fonction de recherche d'adresse de base
static BOOL enableReadMemoryValue = FALSE; // Enable memory reading function | Active la fonction de lecture mémoire
static BOOL enableGetPlayerCoordinates = FALSE; // Enable player coordinates retrieval | Active la récupération des coordonnées du joueur
static BOOL useServer = FALSE; // TRUE to use server, FALSE to disable | TRUE pour utiliser le serveur, FALSE pour désactiver
static BOOL getPlayerCoordinates(void); // Forward declaration | Déclaration avant

// NOUVEAU: Variables pour la limite dynamique de distance audio
static double serverMaximumAudioDistance = 30.0; // Valeur par défaut
static BOOL maxAudioDistanceRetrieved = FALSE; // Indicateur si la valeur a été récupérée
static ULONGLONG lastMaxDistanceCheck = 0; // Dernière vérification de la distance ma

// Offset Control Variables | Variables pour contrôler l'activation des offsets
static BOOL enableMemoryOffsets = FALSE; // Enable memory offset system | Active le système d'offsets mémoire
static BOOL allowOffsetsFromConfig = TRUE; // Allow enabling offsets from config file | Permet d'activer les offsets depuis le fichier de config

// Channel management variables | Variables de gestion des canaux  
static mumble_channelid_t hubChannelID = -1; // Hub channel ID | ID du canal hub
static mumble_channelid_t ingameChannelID = -1; // In-Game channel ID | ID du canal In-Game  
static mumble_channelid_t lastTargetChannel = -1; // Last target channel | Dernier canal cible
static mumble_channelid_t lastValidChannel = -1; // Last valid channel where user should be | Dernier canal valide où l'utilisateur devrait être
static BOOL channelManagementActive = FALSE; // Channel management active flag | Indicateur de gestion des canaux active
static BOOL enableAutomaticChannelChange = FALSE; // Enable automatic channel switching | Active le changement automatique de salon
static ULONGLONG lastChannelCheck = 0; // Last channel check timestamp | Horodatage de la dernière vérification des canaux

// YAW Control Variables | Variables pour le contrôle du YAW
static BOOL enableYawOffsets = TRUE; // Enable YAW offset calculations | Active les calculs d'offsets YAW
static BOOL useNativeTwoYawOffsets = FALSE; // Enable native two YAW offsets mode | Active le mode offsets natifs YAW séparés
static BOOL enableBackupYawOffsets = FALSE; // Enable backup YAW offsets | Active les offsets YAW de secours

// Mumble API Interface | Interface API Mumble
struct MumbleAPI_v_1_0_x mumbleAPI; // Mumble API structure | Structure API Mumble
mumble_plugin_id_t ownID; // Plugin unique identifier | Identifiant unique du plugin

// Player Position Variables | Variables de position du joueur
float axe_x = 0.0f; // Player X coordinate | Coordonnée X du joueur
float axe_y = 0.0f; // Player Y coordinate | Coordonnée Y du joueur
float axe_z = 0.0f; // Player Z coordinate | Coordonnée Z du joueur
float avatarAxisX = 0.0f; // Avatar X direction | Direction X de l'avatar
float avatarAxisY = 0.0f; // Avatar Y direction | Direction Y de l'avatar
float avatarAxisZ = 0.0f; // Avatar Z direction | Direction Z de l'avatar

// Mod Data Structure | Structure pour stocker les données du mod
struct ModFileData {
    int seq; // Sequence number | Numéro de séquence
    float x, y, z, yaw, yawY; // Position and rotation data | Données de position et rotation ✅ CHANGÉ yawZ → yawY
    BOOL valid; // Data validity flag | Indicateur de validité des données
};

// Adaptive Mod System Variables | Variables pour le système de Mod adaptatif
static BOOL useModFile = FALSE; // Currently using mod file flag | Indicateur d'utilisation actuelle du fichier mod
static time_t lastFileCheck = 0; // Last file check time (every 5s) | Dernière vérification du fichier (toutes les 5s)
static time_t LastFileModification = 0; // Last file modification time | Dernière modification du fichier
static int lastSeq = -1; // Last read SEQ to detect changes | Dernier SEQ lu pour détecter les changements
static BOOL modDataValid = FALSE; // Mod data validity flag | Indicateur de validité des données mod
static char modFilePath[MAX_PATH] = ""; // Mod file path | Chemin du fichier mod
static BOOL coordinatesValid = FALSE; // Coordinates validity flag | Indicateur de validité des coordonnées
static struct ModFileData currentModData = { 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, FALSE }; // Add a 0.0f for yawY | Ajout d'un 0.0f pour yawY
static ULONGLONG lastModDataTick = 0; // High resolution timestamp of last valid mod read | Horodatage HAUTE RÉSOLUTION de la dernière lecture valide du mod

// GUI Variables and Other Necessities | Variables d'interface graphique et autres nécessaires
#define CONFIG_FILE L"plugin.cfg" // Configuration file name | Nom du fichier de configuration

#ifndef _UNICODE
#define _UNICODE
#endif

// Variables pour éviter les blocages de mute/demute
static ULONGLONG lastMuteStateCheck = 0;
static BOOL forceMuteStateRefresh = FALSE;

// Variables globales pour l'interface
HWND hConfigDialog = NULL;
HWND hWhisperKeyEdit, hNormalKeyEdit, hShoutKeyEdit, hConfigKeyEdit;
HWND hWhisperButton, hNormalButton, hShoutButton, hConfigButton;
HWND hEnableDistanceMutingCheck, hEnableMemoryOffsetsCheck, hEnableAutomaticChannelChangeCheck;
HWND hDistanceWhisperEdit, hDistanceNormalEdit, hDistanceShoutEdit;
HWND hSavedPathEdit, hSavedPathButton;
HWND hCategoryPatch, hCategoryAdvanced;
HWND hEnableVoiceToggleCheck, hVoiceToggleKeyEdit, hVoiceToggleButton;
HWND hStatusMessage = NULL;
HWND hDistanceLimitMessage = NULL;
HFONT hFont = NULL, hFontBold = NULL, hFontLarge = NULL, hFontEmoji = NULL;

// NOUVEAU: Variables pour le toggle cyclique des modes de voix
static int voiceToggleKey = 84; // T par défaut
static BOOL enableVoiceToggle = FALSE; // Active le système de toggle
static ULONGLONG lastVoiceTogglePress = 0; // Anti-spam pour le toggle

static int currentCategory = 1;
static int whisperKey = 17;
static int normalKey = 86;
static int shoutKey = 16;
static int configUIKey = 121;
static BOOL enableDistanceMuting = FALSE;
static BOOL isCapturingKey = FALSE;
static int captureKeyTarget = 0;
static wchar_t savedPath[MAX_PATH] = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles\\ConanSandbox\\Saved";

wchar_t* getConfigFolderPath();
int isPatchAlreadySaved();
const wchar_t* infoText1 = L"\U0001F4A1 Please provide the path to your Conan Exiles folder.";
const wchar_t* infoText2 = L"\U0001F4C2 Example: C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles";
const wchar_t* infoText3 = L"\u26A0\uFE0F The 'Saved' folder must exist inside 'ConanSandbox' for the plugin to work.";

volatile BOOL isConnected = FALSE; // Connection indicator | Indicateur de connexion

// Display message in chat | Afficher un message dans le chat
static void displayInChat(const char* message) {
    mumbleAPI.log(ownID, message);
}

// Variables to track used offsets | Variables pour suivre les offsets utilisés
bool usedPrimaryX = true; // Primary X offset used flag | Indicateur d'utilisation de l'offset X principal
bool usedPrimaryY = true; // Primary Y offset used flag | Indicateur d'utilisation de l'offset Y principal
bool usedPrimaryZ = true; // Primary Z offset used flag | Indicateur d'utilisation de l'offset Z principal
bool usedPrimaryYawX = true; // Primary YAW X offset used flag | Indicateur d'utilisation de l'offset YAW X principal
bool usedPrimaryYawY = true; // Primary YAW Y offset used flag | Indicateur d'utilisation de l'offset YAW Y principal
bool usedPrimaryYawZ = true; // Primary YAW Z offset used flag | Indicateur d'utilisation de l'offset YAW Z principal

// Backup Offset Enable Flags | Indicateurs d'activation des offsets de secours
static BOOL enableBackupOffsetX = FALSE; // Enable backup X offset | Active l'offset X de secours
static BOOL enableBackupOffsetY = FALSE; // Enable backup Y offset | Active l'offset Y de secours
static BOOL enableBackupOffsetZ = FALSE; // Enable backup Z offset | Active l'offset Z de secours

// Ajout des nouvelles structures et variables pour le système de voice chat
#pragma pack(push, 1)
typedef struct {
    float x, y, z;          // Coordonnées du joueur
    uint8_t voiceMode;      // 0=whisper, 1=normal, 2=shout
    float voiceDistance;    // Distance de voix actuelle
    char playerName[64];    // Nom du joueur
} VoiceData;
#pragma pack(pop)

// Variables globales pour le système de voix
static VoiceData localVoiceData = { 0.0f, 0.0f, 0.0f, 1, 10.0f, "" };
static VoiceData remotePlayersData[64]; // Stockage des données des autres joueurs
static size_t remotePlayerCount = 0;
static ULONGLONG lastVoiceDataSent = 0;
static ULONGLONG lastKeyCheck = 0;

// Variables de distance configurables par l'utilisateur
static float distanceWhisper = 2.0f;   // Distance pour chuchoter
static float distanceNormal = 10.0f;   // Distance pour parler normalement  
static float distanceShout = 15.0f;    // Distance pour crier\

typedef struct {
    mumble_userid_t userID;
    char playerName[64];
    bool currentlyMuted;
    ULONGLONG lastMuteCheck;
} PlayerMuteState;

// Variables globales pour le système de mute
static PlayerMuteState playerMuteStates[64];
static size_t playerMuteStateCount = 0;
static ULONGLONG lastDistanceCheck = 0;

// Global key monitoring variables | Variables de surveillance des touches globales
static BOOL isConfigDialogOpen = FALSE; // Flag to prevent multiple dialogs | Flag pour empêcher plusieurs dialogues
static DWORD lastKeyPressTime = 0; // Prevent key repeat | Empêcher la répétition de touches

// Variables de surveillance des touches globales - NOUVELLE APPROCHE
static BOOL keyMonitorThreadRunning = FALSE;
static HANDLE keyMonitorThread = NULL;
static BOOL lastKeyState = FALSE;

// Variable pour forcer un refresh complet des états de mute
static BOOL forceGlobalMuteRefresh = FALSE;
static ULONGLONG lastGlobalRefresh = 0;

// Thread de surveillance simple qui fonctionne toujours | Simple monitoring thread that always works
static void keyMonitorThreadFunction(void* arg) {
    keyMonitorThreadRunning = TRUE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Key monitor thread: Started with ultra-reactive detection");
    }

    while (keyMonitorThreadRunning) {
        // Vérifier l'état actuel de la touche
        BOOL currentKeyState = (GetAsyncKeyState(configUIKey) & 0x8000) != 0;

        // DÉTECTION DE FRONT MONTANT (touche pressée)
        if (currentKeyState && !lastKeyState) {
            // Touche vient d'être pressée !
            if (!isConfigDialogOpen) {
                isConfigDialogOpen = TRUE;

                if (enableLogGeneral) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "KEY INSTANT-DETECTED! %s (VK:%d) - opening interface immediately...",
                        getKeyName(configUIKey), configUIKey);
                    mumbleAPI.log(ownID, msg);
                }

                // Créer l'interface dans un thread séparé
                _beginthread(showPathSelectionDialogThread, 0, NULL);
            }
        }

        // Mettre à jour l'état précédent
        lastKeyState = currentKeyState;

        // Vérification très fréquente pour réactivité maximale
        Sleep(20); // 20ms = détection quasi-instantanée
    }

    keyMonitorThreadRunning = FALSE;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Key monitor thread: Stopped");
    }
}

// Nouvelle fonction pour obtenir la position de la souris et l'écran approprié
static void getMousePositionAndScreen(POINT* mousePos, HMONITOR* targetMonitor, RECT* monitorRect) {
    // Obtenir la position actuelle de la souris
    GetCursorPos(mousePos);

    // Trouver l'écran où se trouve la souris
    *targetMonitor = MonitorFromPoint(*mousePos, MONITOR_DEFAULTTONEAREST);

    // Obtenir les informations de l'écran
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(*targetMonitor, &monitorInfo);
    *monitorRect = monitorInfo.rcWork; // Zone de travail (sans la barre des tâches)
}

// Fonction pour forcer la fenêtre au premier plan (même en jeu plein écran)
static void forceWindowToForeground(HWND hwnd) {
    // Étape 1: Assurer que la fenêtre est visible
    ShowWindow(hwnd, SW_SHOW);

    // Étape 2: La placer au premier plan avec les flags les plus agressifs
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);

    // Étape 3: Forcer l'activation
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);

    if (currentThreadId != foregroundThreadId) {
        // Attacher temporairement notre thread au thread de premier plan
        AttachThreadInput(currentThreadId, foregroundThreadId, TRUE);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        AttachThreadInput(currentThreadId, foregroundThreadId, FALSE);
    }
    else {
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
    }

    // Étape 4: S'assurer que la fenêtre reste visible
    BringWindowToTop(hwnd);

    // Étape 5: Flash pour attirer l'attention si nécessaire
    FLASHWINFO flashInfo = { 0 };
    flashInfo.cbSize = sizeof(FLASHWINFO);
    flashInfo.hwnd = hwnd;
    flashInfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
    flashInfo.uCount = 3;
    flashInfo.dwTimeout = 100;
    FlashWindowEx(&flashInfo);
}

// Modifier showConfigInterface pour positionner à la souris
static int showConfigInterface() {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Function started");
    }

    // Load current values from config
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
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ConfigDialogProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = CONFIG_CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(248, 249, 250));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    // Unregister class if it exists
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

    // NOUVEAU: Obtenir la position de la souris et l'écran approprié
    POINT mousePos;
    HMONITOR targetMonitor;
    RECT monitorRect;
    getMousePositionAndScreen(&mousePos, &targetMonitor, &monitorRect);

    // Dimensions de la fenêtre
    int windowWidth = 600;
    int windowHeight = 740;

    // Calculer la position pour centrer autour de la souris
    int windowX = mousePos.x - (windowWidth / 2);
    int windowY = mousePos.y - (windowHeight / 2);

    // S'assurer que la fenêtre reste dans les limites de l'écran
    if (windowX < monitorRect.left) {
        windowX = monitorRect.left + 10;
    }
    else if (windowX + windowWidth > monitorRect.right) {
        windowX = monitorRect.right - windowWidth - 10;
    }

    if (windowY < monitorRect.top) {
        windowY = monitorRect.top + 10;
    }
    else if (windowY + windowHeight > monitorRect.bottom) {
        windowY = monitorRect.bottom - windowHeight - 10;
    }

    if (enableLogGeneral) {
        char posMsg[256];
        snprintf(posMsg, sizeof(posMsg), "showConfigInterface: Positioning window at mouse location - Mouse: (%ld,%ld), Window: (%d,%d)",
            mousePos.x, mousePos.y, windowX, windowY);
        mumbleAPI.log(ownID, posMsg);
    }

    // Créer la fenêtre à la position calculée
    hConfigDialog = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // NOUVEAU: WS_EX_TOOLWINDOW aide à rester au premier plan
        CONFIG_CLASS_NAME,
        L"\U0001F3AE Plugin Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX, windowY, windowWidth, windowHeight, // NOUVEAU: Position calculée
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

    // Définir la transparence
    SetLayeredWindowAttributes(hConfigDialog, 0, 250, LWA_ALPHA);

    // NOUVEAU: Forcer la fenêtre au premier plan de manière agressive
    forceWindowToForeground(hConfigDialog);

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "showConfigInterface: Window positioned at mouse and forced to foreground");
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

// Démarrer le thread de surveillance | Start monitoring thread
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

// Nouvelle fonction simplifiée pour installer la surveillance | New simplified function to install monitoring
static void installKeyMonitoring() {
    if (enableLogGeneral) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Installing key monitoring for: %s (VK:%d)",
            getKeyName(configUIKey), configUIKey);
        mumbleAPI.log(ownID, msg);
    }

    startKeyMonitorThread();
}

// Fonction pour afficher un message de statut dans l'interface | Function to display status message in interface
static void showStatusMessage(const wchar_t* message, BOOL isError) {
    if (hStatusMessage) {
        SetWindowTextW(hStatusMessage, message);

        // Changer la couleur selon le type de message | Change color based on message type
        if (isError) {
            // Rouge pour les erreurs | Red for errors
            SendMessage(hStatusMessage, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hStatusMessage), (LPARAM)hStatusMessage);
        }
        else {
            // Vert pour les succès | Green for success
            SendMessage(hStatusMessage, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hStatusMessage), (LPARAM)hStatusMessage);
        }

        // Masquer le message après 5 secondes | Hide message after 5 seconds
        SetTimer(hConfigDialog, 2, 5000, NULL);
    }
}

// Fonction pour effacer le message de statut | Function to clear status message
static void clearStatusMessage() {
    if (hStatusMessage) {
        SetWindowTextW(hStatusMessage, L"");
    }
}

// Arrêter le thread de surveillance | Stop monitoring thread
static void stopKeyMonitorThread() {
    keyMonitorThreadRunning = FALSE;
    if (keyMonitorThread != NULL) {
        // Attendre que le thread se termine | Wait for thread to finish
        Sleep(500); // Simple delay instead of WaitForSingleObject
        keyMonitorThread = NULL;

        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Key monitor thread stopped");
        }
    }
}

// Nouvelle fonction pour désinstaller | New function to uninstall
static void removeKeyMonitoring() {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Removing key monitoring");
    }

    stopKeyMonitorThread();
}

// Zone Structure | Structure de zone
struct Zone {
    float x1; // Zone X1 coordinate | Coordonnée X1 de la zone
    float y1; // Zone Y1 coordinate | Coordonnée Y1 de la zone
    float x2; // Zone X2 coordinate | Coordonnée X2 de la zone
    float y2; // Zone Y2 coordinate | Coordonnée Y2 de la zone
    double maxDistance; // Maximum audio distance in zone | Distance audio maximale dans la zone
};

struct Zone* zones = NULL; // Zones array pointer | Pointeur vers le tableau de zones
size_t zoneCount = 0; // Number of zones | Nombre de zones

// Parse zones from server response | Analyser les zones depuis la réponse du serveur
static void parseZones(const char* response) {
    // Search for beginning of "ZONES:" section in response | Cherche le début de la section "ZONES:" dans la réponse
    const char* zonesData = strstr(response, "ZONES: ");
    if (zonesData == NULL) {
        displayInChat("Error: zone data not found in response | Erreur: données de zones non trouvées dans la réponse");
        return;
    }

    // Advance pointer beyond "ZONES: " | Avancer le pointeur au-delà de "ZONES: "
    zonesData += 7;

    // Count number of zones (assuming they are separated by commas) | Compte le nombre de zones (assumant qu'elles sont séparées par des virgules)
    zoneCount = 1;
    const char* p = zonesData;
    while (*p) {
        if (*p == ';') { // Change to semicolon if zones are separated by semicolons | Changez en point-virgule si les zones sont séparées par des points-virgules
            zoneCount++;
        }
        p++;
    }

    // Allocate memory for zones | Alloue la mémoire pour les zones
    zones = (struct Zone*)malloc(zoneCount * sizeof(struct Zone));
    if (zones == NULL) {
        displayInChat("Memory allocation error for zones | Erreur d'allocation de mémoire pour les zones");
        return;
    }

    // Fill information for each zone | Remplit les informations de chaque zone
    p = zonesData;
    for (size_t i = 0; i < zoneCount; i++) {
        int result = sscanf_s(p, "%f,%f,%f,%f,%lf", &zones[i].x1, &zones[i].y1, &zones[i].x2, &zones[i].y2, &zones[i].maxDistance);
        if (result != 5) {
            char errorMessage[256];
            snprintf(errorMessage, sizeof(errorMessage), "Analysis error for zone %zu | Erreur d'analyse pour la zone %zu", i + 1, i + 1);
            displayInChat(errorMessage);
            free(zones);
            zones = NULL;
            zoneCount = 0;
            return;
        }

        // Move to next zone (using delimiter `;` or `,`) | Passe à la prochaine zone (en utilisant le délimiteur `;` ou `,`)
        while (*p && *p != ';') {
            p++;
        }
        if (*p == ';') {
            p++;  // Skip semicolon | Ignore le point-virgule
        }
    }
}

// Modifier updateDistanceLimitMessage pour FORCER la récupération
static void updateDistanceLimitMessage() {
    if (!hDistanceLimitMessage) return;

    retrieveServerMaximumAudioDistance(TRUE);

    wchar_t limitMsg[256];
    swprintf(limitMsg, 256, L"Maximum distance limit: %.1f meters (set by Mumble's Maximum Distance setting)",
        serverMaximumAudioDistance);

    SetWindowTextW(hDistanceLimitMessage, limitMsg);
}

// Modifier la fonction pour accepter un paramètre de force
static void retrieveServerMaximumAudioDistance(BOOL forceUpdate) {
    ULONGLONG currentTime = GetTickCount64();

    // Vérifier seulement toutes les 5 secondes pour éviter le spam, SAUF si forceUpdate = TRUE
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
        // Utiliser une valeur par défaut sécurisée
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

// NOUVEAU: Fonction pour appliquer la limite maximale aux distances de voix
static void applyMaximumDistanceLimits() {
    if (!maxAudioDistanceRetrieved) {
        retrieveServerMaximumAudioDistance(FALSE);
    }

    BOOL distanceChanged = FALSE;

    // Vérifier et limiter les distances
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

    // NOUVEAU: Mettre à jour le message dans l'interface
    updateDistanceLimitMessage();

    if (distanceChanged) {
        localVoiceData.voiceDistance = getVoiceDistanceForMode(localVoiceData.voiceMode);
        saveVoiceSettings();
        applyDistanceToAllPlayers();
    }
}

// NOUVEAU: Fonction pour générer un message dynamique sur les limites de distance
static void showDynamicDistanceLimitMessage() {
    if (!maxAudioDistanceRetrieved) {
        retrieveServerMaximumAudioDistance(FALSE);
    }

    char dynamicMsg[1024];
    snprintf(dynamicMsg, sizeof(dynamicMsg),
        "🔊 VOICE DISTANCE INFORMATION:\n"
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

// Set maximum audio distance | Définir la distance audio maximale
static void setMaximumAudioDistance(double newMaxDistance) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_MAXIMUM_DISTANCE, newMaxDistance);
    if (result != MUMBLE_STATUS_OK) {}
}

// Set minimum audio distance | Définir la distance audio minimale
static void setMinimumAudioDistance(double newMinDistance) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_MINIMUM_DISTANCE, newMinDistance);
    if (result != MUMBLE_STATUS_OK) {}
}

// Set audio bloom effect | Définir l'effet de bloom audio
static void setAudioBloom(double bloomValue) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_BLOOM, bloomValue);
    if (result != MUMBLE_STATUS_OK) {}
}

// Set minimum audio volume | Définir le volume audio minimal
static void setMinimumAudioVolume(double minVolume) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_MINIMUM_VOLUME, minVolume);
    if (result != MUMBLE_STATUS_OK) {}
}



// Fonction pour calculer la distance entre deux points
static float calculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// Fonction pour obtenir la distance de voix selon le mode
static float getVoiceDistanceForMode(uint8_t voiceMode) {
    switch (voiceMode) {
    case 0: return distanceWhisper; // Whisper
    case 1: return distanceNormal;  // Normal
    case 2: return distanceShout;   // Shout
    default: return distanceNormal;
    }
}

// Fonction pour obtenir le nom du joueur local
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

// NOUVELLE FONCTION: Applique immédiatement la nouvelle distance à tous les joueurs connectés
static void applyDistanceToAllPlayers() {
    if (!enableDistanceMuting) return;

    ULONGLONG currentTime = GetTickCount64();
    mumble_connection_t connection;

    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        return;
    }

    // FORCER la réévaluation de TOUS les joueurs dans notre liste
    for (size_t i = 0; i < playerMuteStateCount; i++) {
        PlayerMuteState* muteState = &playerMuteStates[i];

        // Trouver les données du joueur correspondant
        VoiceData* playerData = NULL;
        for (size_t j = 0; j < remotePlayerCount; j++) {
            if (strcmp(remotePlayersData[j].playerName, muteState->playerName) == 0) {
                playerData = &remotePlayersData[j];
                break;
            }
        }

        if (playerData) {
            // Calculer la distance avec LES NOUVELLES coordonnées locales
            float distance = calculateDistance(localVoiceData.x, localVoiceData.y, localVoiceData.z,
                playerData->x, playerData->y, playerData->z);

            // Utiliser LA NOUVELLE distance de voix du joueur local (qui vient de changer)
            float maxHearingDistance = playerData->voiceDistance;
            bool shouldMute = (distance > maxHearingDistance);

            // APPLIQUER IMMÉDIATEMENT le mute/unmute
            bool mumbleCurrentlyMuted = false;
            if (mumbleAPI.isUserLocallyMuted(ownID, connection, muteState->userID, &mumbleCurrentlyMuted) == MUMBLE_STATUS_OK) {
                if (mumbleCurrentlyMuted != shouldMute) {
                    mumble_error_t result = mumbleAPI.requestLocalMute(ownID, connection, muteState->userID, shouldMute);

                    if (result == MUMBLE_STATUS_OK) {
                        muteState->currentlyMuted = shouldMute;
                        muteState->lastMuteCheck = currentTime;

                        if (enableLogGeneral) {
                            char logMsg[256];
                            snprintf(logMsg, sizeof(logMsg),
                                "INSTANT-APPLY: Player %s: %s (Distance: %.1fm, Mode changed!)",
                                playerData->playerName,
                                shouldMute ? "MUTED" : "UNMUTED",
                                distance);
                            mumbleAPI.log(ownID, logMsg);
                        }
                    }
                }
            }
        }
    }
}

// NOUVEAU: Fonction pour cycliser entre les modes de voix
static void cycleVoiceMode() {
    if (!enableVoiceToggle) return;

    ULONGLONG currentTime = GetTickCount64();
    // Anti-spam: minimum 300ms entre les pressions
    if (currentTime - lastVoiceTogglePress < 300) return;
    lastVoiceTogglePress = currentTime;

    uint8_t newMode;
    switch (localVoiceData.voiceMode) {
    case 1: newMode = 2; break; // Normal → Shout
    case 2: newMode = 0; break; // Shout → Whisper  
    case 0: newMode = 1; break; // Whisper → Normal
    default: newMode = 1; break; // Par défaut Normal
    }

    // Appliquer le nouveau mode
    localVoiceData.voiceMode = newMode;
    localVoiceData.voiceDistance = getVoiceDistanceForMode(newMode);

    // Forcer l'envoi immédiat
    lastVoiceDataSent = 0;
    applyDistanceToAllPlayers();

    // Message de confirmation
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

// REMPLACER la fonction updateVoiceMode existante par celle-ci :
static void updateVoiceMode() {
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastKeyCheck < 50) return;
    lastKeyCheck = currentTime;

    static uint8_t lastVoiceMode = 1;
    uint8_t newVoiceMode = localVoiceData.voiceMode;

    // NOUVEAU: Vérifier la touche de toggle AVANT les touches normales
    static BOOL lastToggleKeyState = FALSE;
    BOOL currentToggleKeyState = (GetAsyncKeyState(voiceToggleKey) & 0x8000) != 0;

    if (enableVoiceToggle && currentToggleKeyState && !lastToggleKeyState) {
        // Touche de toggle pressée - cycliser le mode
        cycleVoiceMode();
        lastToggleKeyState = currentToggleKeyState;
        return; // Sortir pour éviter les conflits avec les touches normales
    }
    lastToggleKeyState = currentToggleKeyState;

    // NOUVEAU: Vérifier les touches de mode de voix normales SEULEMENT si EnableVoiceToggle est FALSE
    if (!enableVoiceToggle) {
        if (GetAsyncKeyState(whisperKey) & 0x8000) {
            newVoiceMode = 0; // Whisper
        }
        else if (GetAsyncKeyState(shoutKey) & 0x8000) {
            newVoiceMode = 2; // Shout
        }
        else if (GetAsyncKeyState(normalKey) & 0x8000) {
            newVoiceMode = 1; // Normal
        }
        else {
            newVoiceMode = localVoiceData.voiceMode; // Garder le mode actuel
        }
    }
    // Si enableVoiceToggle est TRUE, on ignore complètement les touches individuelles

    // Le reste reste identique
    if (newVoiceMode != lastVoiceMode) {
        localVoiceData.voiceMode = newVoiceMode;
        localVoiceData.voiceDistance = getVoiceDistanceForMode(newVoiceMode);
        lastVoiceMode = newVoiceMode;

        lastVoiceDataSent = 0;
        applyDistanceToAllPlayers();

        char modeNames[][10] = { "Whisper", "Normal", "Shout" };
        char chatMessage[128];
        snprintf(chatMessage, sizeof(chatMessage),
            "Voice mode: %s - Distance: %.1f meters",
            modeNames[newVoiceMode],
            localVoiceData.voiceDistance);

        displayInChat(chatMessage);

        if (enableLogGeneral) {
            char logMsg[128];
            snprintf(logMsg, sizeof(logMsg), "INSTANT Voice mode changed to: %s (Distance: %.1fm) - Applied immediately!",
                modeNames[newVoiceMode], localVoiceData.voiceDistance);
            mumbleAPI.log(ownID, logMsg);
        }
    }
}

// Fonction pour envoyer les données de voix aux autres joueurs - OPTIMISÉE 200MS
static void sendVoiceDataToAll() {
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastVoiceDataSent < 40) return; // CHANGÉ: 1000ms → 40ms
    lastVoiceDataSent = currentTime;

    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        if (enableLogGeneral) {
            mumbleAPI.log(ownID, "Failed to get active server connection");
        }
        return;
    }

    // Mettre à jour les coordonnées locales
    localVoiceData.x = axe_x / 100.0f;
    localVoiceData.y = axe_y / 100.0f;
    localVoiceData.z = axe_z / 100.0f;

    // Obtenir la liste de tous les utilisateurs
    mumble_userid_t* allUsers = NULL;
    size_t userCount = 0;

    if (mumbleAPI.getAllUsers(ownID, connection, &allUsers, &userCount) == MUMBLE_STATUS_OK) {
        if (allUsers && userCount > 0) {
            // OPTIMISÉ: Réduire les logs pour éviter le spam
            static ULONGLONG lastDebugLog = 0;
            if (enableLogGeneral && (currentTime - lastDebugLog > 5000)) { // Log toutes les 5 secondes seulement
                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg),
                    "HIGH-FREQ: Sending to %zu users every 200ms - Mode: %d, Distance: %.1f",
                    userCount, localVoiceData.voiceMode, localVoiceData.voiceDistance);
                mumbleAPI.log(ownID, debugMsg);
                lastDebugLog = currentTime;
            }

            // Envoyer les données à tous les utilisateurs
            mumble_error_t result = mumbleAPI.sendData(ownID, connection, allUsers, userCount,
                (const uint8_t*)&localVoiceData, sizeof(VoiceData),
                "ConanExiles_VoiceData");

            // OPTIMISÉ: Logger les erreurs moins fréquemment
            static ULONGLONG lastErrorLog = 0;
            if (result != MUMBLE_STATUS_OK && (currentTime - lastErrorLog > 2000)) {
                char errorMsg[128];
                snprintf(errorMsg, sizeof(errorMsg), "HIGH-FREQ send failed: Error %d (Users: %zu)",
                    result, userCount);
                mumbleAPI.log(ownID, errorMsg);
                lastErrorLog = currentTime;
            }

            mumbleAPI.freeMemory(ownID, allUsers);
        }
    }
}

// Fonction pour trouver ou créer l'état de mute d'un joueur
static PlayerMuteState* findOrCreatePlayerMuteState(mumble_userid_t userID, const char* playerName) {
    // Chercher un état existant
    for (size_t i = 0; i < playerMuteStateCount; i++) {
        if (playerMuteStates[i].userID == userID) {
            return &playerMuteStates[i];
        }
    }

    // Créer un nouvel état si on a de la place
    if (playerMuteStateCount < 64) {
        PlayerMuteState* newState = &playerMuteStates[playerMuteStateCount];
        newState->userID = userID;
        strncpy_s(newState->playerName, sizeof(newState->playerName), playerName, _TRUNCATE);
        newState->currentlyMuted = false;
        newState->lastMuteCheck = 0;
        playerMuteStateCount++;
        return newState;
    }

    return NULL;
}

// Force un refresh de tous les états toutes les 5 secondes
static void checkForceGlobalRefresh() {
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastGlobalRefresh > 5000) { // 5 secondes
        forceGlobalMuteRefresh = TRUE;
        lastGlobalRefresh = currentTime;
    }
}

// Fonction optimisée pour gérer les données de voix reçues - RÉACTIVITÉ MAXIMALE
static void processReceivedVoiceData(const VoiceData* receivedData, mumble_userid_t senderID) {
    if (!receivedData || !enableDistanceMuting) return;

    ULONGLONG currentTime = GetTickCount64();

    // Calculer la distance
    float distance = calculateDistance(localVoiceData.x, localVoiceData.y, localVoiceData.z,
        receivedData->x, receivedData->y, receivedData->z);

    // CORRECTION: Utiliser la distance de voix du JOUEUR DISTANT (celui qui parle)
    float maxHearingDistance = receivedData->voiceDistance;
    bool shouldMute = (distance > maxHearingDistance);

    // Trouver l'état de mute
    PlayerMuteState* muteState = findOrCreatePlayerMuteState(senderID, receivedData->playerName);
    if (!muteState) return;

    // Vérifier si on doit forcer un refresh global
    checkForceGlobalRefresh();

    bool forceCheck = (muteState->lastMuteCheck == 0) || forceGlobalMuteRefresh;
    bool timeCheck = (currentTime - muteState->lastMuteCheck > 40);

    if (forceCheck || timeCheck) {
        if (forceGlobalMuteRefresh) {
            forceGlobalMuteRefresh = FALSE; // Reset après usage
        }

        mumble_connection_t connection;
        if (mumbleAPI.getActiveServerConnection(ownID, &connection) == MUMBLE_STATUS_OK) {

            // CORRECTION: TOUJOURS appliquer l'état calculé
            mumble_error_t result = mumbleAPI.requestLocalMute(ownID, connection, senderID, shouldMute);

            if (result == MUMBLE_STATUS_OK) {
                muteState->currentlyMuted = shouldMute;
                muteState->lastMuteCheck = currentTime;

                if (forceCheck || enableLogGeneral) {
                    static ULONGLONG lastMuteLog = 0;
                    if (forceCheck || (currentTime - lastMuteLog > 2000)) {
                        char logMsg[256];
                        const char* modeNames[] = { "Whisper", "Normal", "Shout" };
                        snprintf(logMsg, sizeof(logMsg),
                            "FIXED: %s (%s, %.1fm): %s (Dist: %.1fm vs Max: %.1fm)",
                            receivedData->playerName,
                            modeNames[receivedData->voiceMode],
                            receivedData->voiceDistance,
                            shouldMute ? "MUTED" : "UNMUTED",
                            distance,
                            maxHearingDistance);
                        mumbleAPI.log(ownID, logMsg);
                        if (!forceCheck) lastMuteLog = currentTime;
                    }
                }
            }
        }
    }

    // Mettre à jour les données
    bool found = false;
    for (size_t i = 0; i < remotePlayerCount; i++) {
        if (strcmp(remotePlayersData[i].playerName, receivedData->playerName) == 0) {
            remotePlayersData[i] = *receivedData;
            found = true;
            break;
        }
    }

    if (!found && remotePlayerCount < 64) {
        remotePlayersData[remotePlayerCount] = *receivedData;
        remotePlayerCount++;
    }
}

// Fonction pour nettoyer les états de mute
static void cleanupPlayerMuteStates() {
    playerMuteStateCount = 0;
    memset(playerMuteStates, 0, sizeof(playerMuteStates));
    lastDistanceCheck = 0;
}

// Nettoyage complet à la déconnexion
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerDisconnected(mumble_connection_t connection) {
    channelManagementActive = FALSE;
    lastValidChannel = -1;

    // NOUVEAU: Nettoyer complètement le système de voix
    cleanupPlayerMuteStates();

    // Réinitialiser toutes les données distantes
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    remotePlayerCount = 0;

    // Réinitialiser les données locales
    memset(localVoiceData.playerName, 0, sizeof(localVoiceData.playerName));
    lastVoiceDataSent = 0;
    lastDistanceCheck = 0;

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Voice chat system reset on disconnection");
    }
}

// Thread pour mettre à jour le système de voix - HAUTE FRÉQUENCE
static void voiceSystemThread(void* arg) {
    Sleep(2000); // Attendre 2 secondes puis démarrer

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "HIGH-FREQUENCY Voice system thread: Now active at 200ms intervals");
    }

    while (enableGetPlayerCoordinates) {
        // Mettre à jour le nom du joueur si nécessaire
        if (strlen(localVoiceData.playerName) == 0) {
            getLocalPlayerName();
        }

        // Vérifier les changements de mode de voix TOUJOURS
        updateVoiceMode();

        // OPTIMISÉ: Debug moins fréquent
        if (enableLogGeneral) {
            static ULONGLONG lastDebugTime = 0;
            ULONGLONG currentTime = GetTickCount64();
            if (currentTime - lastDebugTime > 20000) { // Toutes les 20 secondes au lieu de 10
                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg),
                    "HIGH-FREQ Voice system status - coordinatesValid: %s, playerCount: %zu, interval: 200ms",
                    coordinatesValid ? "TRUE" : "FALSE",
                    remotePlayerCount);
                mumbleAPI.log(ownID, debugMsg);
                lastDebugTime = currentTime;
            }
        }

        // Envoyer les données de voix si les coordonnées sont valides ET que le muting est activé
        if (coordinatesValid && enableDistanceMuting) {
            sendVoiceDataToAll();
        }

        Sleep(25); // CHANGÉ: 100ms → 25ms pour plus de réactivité dans la détection des touches
    }

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "HIGH-FREQUENCY Voice system thread: Stopped");
    }
}

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

    // NOUVEAU: Appliquer les limites après le chargement
    applyMaximumDistanceLimits();

    if (enableLogConfig) {
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg),
            "Voice distances loaded and limited - Whisper: %.1fm, Normal: %.1fm, Shout: %.1fm (Max: %.1fm)",
            distanceWhisper, distanceNormal, distanceShout, serverMaximumAudioDistance);
        mumbleAPI.log(ownID, logMsg);
    }
}

const int numZones = sizeof(zones) / sizeof(zones[0]); // Number of zones calculation | Calcul du nombre de zones

// Check player zone and adjust audio settings | Vérifier la zone du joueur et ajuster les paramètres audio
static void checkPlayerZone() {
    if (!enableCheckPlayerZone) return;

    bool inZone = false;
    for (size_t i = 0; i < zoneCount; ++i) {
        if (axe_x / 100.0f >= zones[i].x1 && axe_x / 100.0f <= zones[i].x2 &&
            axe_y / 100.0f >= zones[i].y1 && axe_y / 100.0f <= zones[i].y2) {
            //setMinimumAudioDistance(2.0); // Set zone audio settings | Définir les paramètres audio de zone
            //setMaximumAudioDistance(zones[i].maxDistance);
            //setMinimumAudioVolume(0.0);
            //setAudioBloom(0.75);
            inZone = true;
            break;
        }
    }

    if (!inZone) {
        //setMinimumAudioDistance(5.0); // Set default audio settings | Définir les paramètres audio par défaut
        //setMaximumAudioDistance(25.0);
        //setMinimumAudioVolume(0.0);
        //setAudioBloom(0.2);
    }
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

// Automatic channel management based on coordinates validity | Gestion automatique des canaux selon la validité des coordonnées
static void manageChannelBasedOnCoordinates() {
    if (!enableAutomaticChannelChange) return; // Check if automatic change is enabled | Vérifier si le changement automatique est activé
    if (!channelManagementActive) return;

    ULONGLONG currentTick = GetTickCount64();
    if (currentTick - lastChannelCheck < 500) return; // 0.5 seconds | 0.5 seconde
    lastChannelCheck = currentTick;

    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        return;
    }

    bool synchronized = false;
    if (mumbleAPI.isConnectionSynchronized(ownID, connection, &synchronized) != MUMBLE_STATUS_OK || !synchronized) {
        return;
    }

    mumble_userid_t localUserID;
    if (mumbleAPI.getLocalUserID(ownID, connection, &localUserID) != MUMBLE_STATUS_OK) {
        return;
    }

    mumble_channelid_t currentChannel;
    if (mumbleAPI.getChannelOfUser(ownID, connection, localUserID, &currentChannel) != MUMBLE_STATUS_OK) {
        return;
    }

    if (coordinatesValid) {
        if (currentChannel != ingameChannelID && ingameChannelID != -1) {
            mumbleAPI.requestUserMove(ownID, connection, localUserID, ingameChannelID, NULL);
            lastValidChannel = ingameChannelID;
        }
    }
    else {
        if (currentChannel != hubChannelID && hubChannelID != -1) {
            mumbleAPI.requestUserMove(ownID, connection, localUserID, hubChannelID, NULL);
            lastValidChannel = hubChannelID;
        }
    }
}

// Initialize channel IDs | Initialiser les IDs des canaux
static void initializeChannelIDs() {
    mumble_connection_t connection;
    if (mumbleAPI.getActiveServerConnection(ownID, &connection) != MUMBLE_STATUS_OK) {
        return;
    }

    bool synchronized = false;
    if (mumbleAPI.isConnectionSynchronized(ownID, connection, &synchronized) != MUMBLE_STATUS_OK || !synchronized) {
        return;
    }

    // Find hub channel | Trouver le canal hub
    if (mumbleAPI.findChannelByName(ownID, connection, "hub", &hubChannelID) != MUMBLE_STATUS_OK) {
        hubChannelID = 0; // Default channel if hub doesn't exist | Canal par défaut si hub n'existe pas
    }

    // Find In-Game channel | Trouver le canal In-Game
    if (mumbleAPI.findChannelByName(ownID, connection, "ingame", &ingameChannelID) != MUMBLE_STATUS_OK) {
        ingameChannelID = -1; // Not found | Pas trouvé
    }

    if (hubChannelID != -1 && ingameChannelID != -1) {
        channelManagementActive = TRUE;
    }
}

// Conversion des codes de touches en noms
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

// Traitement de la capture de touches
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

// Fonction pour parcourir les dossiers (moderne)
void browseSavedPath(HWND hwnd) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(hwnd, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    IFileOpenDialog* pFileOpen = NULL;
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        &IID_IFileOpenDialog, (void**)&pFileOpen);

    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        hr = pFileOpen->lpVtbl->GetOptions(pFileOpen, &dwOptions);
        if (SUCCEEDED(hr)) {
            hr = pFileOpen->lpVtbl->SetOptions(pFileOpen, dwOptions | FOS_PICKFOLDERS);
        }

        if (SUCCEEDED(hr)) {
            hr = pFileOpen->lpVtbl->SetTitle(pFileOpen, L"Select your Conan Exiles game folder");
        }

        if (SUCCEEDED(hr)) {
            hr = pFileOpen->lpVtbl->Show(pFileOpen, hwnd);

            if (SUCCEEDED(hr)) {
                IShellItem* pItem = NULL;
                hr = pFileOpen->lpVtbl->GetResult(pFileOpen, &pItem);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath = NULL;
                    hr = pItem->lpVtbl->GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                    if (SUCCEEDED(hr)) {
                        SetWindowTextW(hSavedPathEdit, pszFilePath);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->lpVtbl->Release(pItem);
                }
            }
        }
        pFileOpen->lpVtbl->Release(pFileOpen);
    }
    CoUninitialize();
}

// Fonction pour afficher/masquer les contrôles selon la catégorie | Show/hide controls based on category
void ShowCategoryControls(int category) {
    currentCategory = category;

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
    if (category == 1) { // Patch Configuration
        if (hExplanation1) ShowWindow(hExplanation1, SW_SHOW);
        if (hExplanation2) ShowWindow(hExplanation2, SW_SHOW);
        if (hExplanation3) ShowWindow(hExplanation3, SW_SHOW);
        if (hPathLabel) ShowWindow(hPathLabel, SW_SHOW);
        if (hSavedPathEdit) ShowWindow(hSavedPathEdit, SW_SHOW);
        if (hSavedPathButton) ShowWindow(hSavedPathButton, SW_SHOW);

        // Masquer les options avancées
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
        if (hDistanceLimitMessage) ShowWindow(hDistanceLimitMessage, SW_HIDE);
        if (hEnableVoiceToggleCheck) ShowWindow(hEnableVoiceToggleCheck, SW_HIDE);
        if (hVoiceToggleKeyEdit) ShowWindow(hVoiceToggleKeyEdit, SW_HIDE);
        if (hVoiceToggleButton) ShowWindow(hVoiceToggleButton, SW_HIDE);
        if (hToggleLabel) ShowWindow(hToggleLabel, SW_HIDE);
    }
    else if (category == 2) { // Advanced Options
        if (hExplanation1) ShowWindow(hExplanation1, SW_HIDE);
        if (hExplanation2) ShowWindow(hExplanation2, SW_HIDE);
        if (hExplanation3) ShowWindow(hExplanation3, SW_HIDE);
        if (hPathLabel) ShowWindow(hPathLabel, SW_HIDE);
        if (hSavedPathEdit) ShowWindow(hSavedPathEdit, SW_HIDE);
        if (hSavedPathButton) ShowWindow(hSavedPathButton, SW_HIDE);
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
        if (hDistanceLimitMessage) ShowWindow(hDistanceLimitMessage, SW_SHOW);
        if (hEnableVoiceToggleCheck) ShowWindow(hEnableVoiceToggleCheck, SW_SHOW);
        if (hVoiceToggleKeyEdit) ShowWindow(hVoiceToggleKeyEdit, SW_SHOW);
        if (hVoiceToggleButton) ShowWindow(hVoiceToggleButton, SW_SHOW);
        if (hToggleLabel) ShowWindow(hToggleLabel, SW_SHOW);
        updateDistanceLimitMessage();
    }

    if (hCategoryPatch && hCategoryAdvanced) {
        SendMessage(hCategoryPatch, BM_SETSTATE, (category == 1) ? TRUE : FALSE, 0);
        SendMessage(hCategoryAdvanced, BM_SETSTATE, (category == 2) ? TRUE : FALSE, 0);
    }
}

// Check if Saved folder exists in game folder | Vérifie que le dossier Saved existe dans le dossier du jeu
int savedExistsInFolder(const wchar_t* folderPath) {
    wchar_t savedPath[MAX_PATH];
    swprintf(savedPath, MAX_PATH, L"%s\\ConanSandbox\\Saved", folderPath);
    DWORD attribs = GetFileAttributesW(savedPath);
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

// NOUVEAU: Fonction pour sauvegarder les changements de distance/mode
static void saveVoiceSettings() {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    // Lire le fichier existant
    wchar_t lines[100][1024];
    int lineCount = 0;
    BOOL foundWhisper = FALSE, foundNormal = FALSE, foundShout = FALSE;

    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, configFile, L"r");
    if (err == 0 && f) {
        while (fgetws(lines[lineCount], 1024, f) && lineCount < 99) {
            // Vérifier si cette ligne contient une distance
            if (wcsncmp(lines[lineCount], L"DistanceWhisper=", 16) == 0) {
                swprintf(lines[lineCount], 1024, L"DistanceWhisper=%.1f\n", distanceWhisper);
                foundWhisper = TRUE;
            }
            else if (wcsncmp(lines[lineCount], L"DistanceNormal=", 15) == 0) {
                swprintf(lines[lineCount], 1024, L"DistanceNormal=%.1f\n", distanceNormal);
                foundNormal = TRUE;
            }
            else if (wcsncmp(lines[lineCount], L"DistanceShout=", 14) == 0) {
                swprintf(lines[lineCount], 1024, L"DistanceShout=%.1f\n", distanceShout);
                foundShout = TRUE;
            }
            lineCount++;
        }
        fclose(f);
    }

    // Ajouter les lignes manquantes
    if (!foundWhisper && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"DistanceWhisper=%.1f\n", distanceWhisper);
    }
    if (!foundNormal && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"DistanceNormal=%.1f\n", distanceNormal);
    }
    if (!foundShout && lineCount < 99) {
        swprintf(lines[lineCount++], 1024, L"DistanceShout=%.1f\n", distanceShout);
    }

    // Réécrire le fichier - CORRIGÉ: Utilisation de _wfopen_s
    f = NULL;
    err = _wfopen_s(&f, configFile, L"w");
    if (err == 0 && f) {
        for (int i = 0; i < lineCount; i++) {
            fwprintf(f, L"%s", lines[i]);
        }
        fclose(f);
    }
}

// Write full Saved path to config file | Écriture du chemin complet Saved dans le fichier de configuration
void writeFullConfiguration(const wchar_t* gameFolder, const wchar_t* distWhisper, const wchar_t* distNormal, const wchar_t* distShout) {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) {
        return;
    }

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    FILE* file = _wfopen(configFile, L"w");
    if (!file) {
        return;
    }

    wchar_t savedPathFull[MAX_PATH];
    swprintf(savedPathFull, MAX_PATH, L"%s\\ConanSandbox\\Saved", gameFolder);

    // NOUVEAU: Mettre à jour les variables globales avec les nouvelles valeurs AVANT validation
    distanceWhisper = (float)_wtof(distWhisper);
    distanceNormal = (float)_wtof(distNormal);
    distanceShout = (float)_wtof(distShout);

    // NOUVEAU: Appliquer les limites maximales
    applyMaximumDistanceLimits();

    // NOUVEAU: Utiliser les valeurs limitées pour l'écriture du fichier
    wchar_t limitedWhisper[32], limitedNormal[32], limitedShout[32];
    swprintf(limitedWhisper, 32, L"%.1f", distanceWhisper);
    swprintf(limitedNormal, 32, L"%.1f", distanceNormal);
    swprintf(limitedShout, 32, L"%.1f", distanceShout);

    fwprintf(file, L"SavedPath=%s\n", savedPathFull);
    fwprintf(file, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
    fwprintf(file, L"EnableMemoryOffsets=%s\n", enableMemoryOffsets ? L"true" : L"false");
    fwprintf(file, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
    fwprintf(file, L"WhisperKey=%d\n", whisperKey);
    fwprintf(file, L"NormalKey=%d\n", normalKey);
    fwprintf(file, L"ShoutKey=%d\n", shoutKey);
    fwprintf(file, L"ConfigUIKey=%d\n", configUIKey);
    fwprintf(file, L"DistanceWhisper=%s\n", limitedWhisper);
    fwprintf(file, L"DistanceNormal=%s\n", limitedNormal);
    fwprintf(file, L"DistanceShout=%s\n", limitedShout);
    fwprintf(file, L"VoiceToggleKey=%d\n", voiceToggleKey);
    fwprintf(file, L"EnableVoiceToggle=%s\n", enableVoiceToggle ? L"true" : L"false");
    fclose(file);

    // Mettre à jour localVoiceData.voiceDistance
    localVoiceData.voiceDistance = getVoiceDistanceForMode(localVoiceData.voiceMode);

    // Update modFilePath
    size_t converted = 0;
    char modFilePathTemp[MAX_PATH] = "";
    wcstombs_s(&converted, modFilePathTemp, MAX_PATH, savedPathFull, _TRUNCATE);
    snprintf(modFilePath, MAX_PATH, "%s\\Pos.txt", modFilePathTemp);

    // Reinstall keyboard hook with new config UI key
    removeKeyMonitoring();
    installKeyMonitoring();

    // NOUVEAU: Afficher le message dynamique sur les limites
    showDynamicDistanceLimitMessage();
}

// Modern folder browser (IFileDialog) | Explorateur de dossier moderne (IFileDialog)
void browseFolderModern(HWND hwnd) {
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
                    SetWindowTextW(hSavedPathEdit, path); // CHANGÉ: hEdit → hSavedPathEdit
                    CoTaskMemFree(path);
                }
                psi->lpVtbl->Release(psi);
            }
        }
        pfd->lpVtbl->Release(pfd);
    }
}

// Apply font to control | Application de la police à un contrôle
void ApplyFontToControl(HWND control, HFONT font) {
    if (font && control) {
        SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
    }
}

// Main window procedure | Procédure de la fenêtre principale
LRESULT CALLBACK ConfigDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND control;

    switch (msg) {
    case WM_CREATE:
        hConfigDialog = hwnd;
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

        // Titre principal
        control = CreateWindowW(L"STATIC", L"\U0001F3AE Plugin Settings",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            10, 15, 580, 35, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(control, hFontLarge);

        // Boutons de catégories
        hCategoryPatch = CreateWindowW(L"BUTTON", L"\U0001F4C1 Patch Configuration",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            80, 65, 200, 40, hwnd, (HMENU)301, NULL, NULL);
        ApplyFontToControl(hCategoryPatch, hFontBold);

        hCategoryAdvanced = CreateWindowW(L"BUTTON", L"\u2699\uFE0F Advanced Options",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            300, 65, 200, 40, hwnd, (HMENU)302, NULL, NULL);
        ApplyFontToControl(hCategoryAdvanced, hFontBold);

        // Ligne de séparation
        control = CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
            40, 120, 520, 2, hwnd, NULL, NULL, NULL);

        // CONTENU CATÉGORIE 1: PATCH CONFIGURATION
        control = CreateWindowW(L"STATIC", infoText1,
            WS_VISIBLE | WS_CHILD,
            40, 140, 520, 25, hwnd, (HMENU)401, NULL, NULL);
        ApplyFontToControl(control, hFont);

        control = CreateWindowW(L"STATIC", infoText2,
            WS_VISIBLE | WS_CHILD,
            40, 170, 520, 25, hwnd, (HMENU)402, NULL, NULL);
        ApplyFontToControl(control, hFont);

        control = CreateWindowW(L"STATIC", infoText3,
            WS_VISIBLE | WS_CHILD,
            40, 200, 520, 25, hwnd, (HMENU)403, NULL, NULL);
        ApplyFontToControl(control, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4CD Game Installation Path",
            WS_VISIBLE | WS_CHILD,
            40, 240, 300, 25, hwnd, (HMENU)404, NULL, NULL);
        ApplyFontToControl(control, hFontBold);

        hSavedPathEdit = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            40, 270, 380, 35, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hSavedPathEdit, hFont);

        hSavedPathButton = CreateWindowW(L"BUTTON", L"\U0001F50D Browse",
            WS_VISIBLE | WS_CHILD,
            430, 270, 100, 35, hwnd, (HMENU)105, NULL, NULL);
        ApplyFontToControl(hSavedPathButton, hFont);

        // CONTENU CATÉGORIE 2: ADVANCED OPTIONS (INITIALEMENT MASQUÉ)
        control = CreateWindowW(L"STATIC", L"\U0001F527 Plugin Features",
            WS_CHILD,
            40, 140, 250, 25, hwnd, (HMENU)501, NULL, NULL);
        ApplyFontToControl(control, hFontBold);

        hEnableDistanceMutingCheck = CreateWindowW(L"BUTTON", L"\U0001F4CF Enable distance-based muting",
            WS_CHILD | BS_AUTOCHECKBOX,
            60, 170, 320, 25, hwnd, (HMENU)201, NULL, NULL);
        ApplyFontToControl(hEnableDistanceMutingCheck, hFont);

        hEnableAutomaticChannelChangeCheck = CreateWindowW(L"BUTTON", L"\U0001F504 Enable automatic channel switching",
            WS_CHILD | BS_AUTOCHECKBOX,
            60, 200, 380, 25, hwnd, (HMENU)203, NULL, NULL);
        ApplyFontToControl(hEnableAutomaticChannelChangeCheck, hFont);

        hEnableVoiceToggleCheck = CreateWindowW(L"BUTTON", L"\U0001F504 Enable voice mode toggle button",
            WS_CHILD | BS_AUTOCHECKBOX,
            60, 230, 380, 25, hwnd, (HMENU)204, NULL, NULL);
        ApplyFontToControl(hEnableVoiceToggleCheck, hFont);

        control = CreateWindowW(L"STATIC", L"\u2328\uFE0F Keyboard Shortcuts",
            WS_CHILD,
            40, 270, 250, 25, hwnd, (HMENU)502, NULL, NULL);
        ApplyFontToControl(control, hFontBold);

        control = CreateWindowW(L"STATIC", L"\U0001F92B Whisper Mode:",
            WS_CHILD,
            60, 305, 130, 25, hwnd, (HMENU)503, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hWhisperKeyEdit = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
            200, 303, 110, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hWhisperKeyEdit, hFont);

        hWhisperButton = CreateWindowW(L"BUTTON", L"\u270F\uFE0F Set Key",
            WS_CHILD,
            320, 303, 90, 30, hwnd, (HMENU)101, NULL, NULL);
        ApplyFontToControl(hWhisperButton, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4AC Normal Chat:",
            WS_CHILD,
            60, 340, 130, 25, hwnd, (HMENU)504, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hNormalKeyEdit = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
            200, 338, 110, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hNormalKeyEdit, hFont);

        hNormalButton = CreateWindowW(L"BUTTON", L"\u270F\uFE0F Set Key",
            WS_CHILD,
            320, 338, 90, 30, hwnd, (HMENU)102, NULL, NULL);
        ApplyFontToControl(hNormalButton, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4E2 Shout Mode:",
            WS_CHILD,
            60, 375, 130, 25, hwnd, (HMENU)505, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hShoutKeyEdit = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
            200, 373, 110, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hShoutKeyEdit, hFont);

        hShoutButton = CreateWindowW(L"BUTTON", L"\u270F\uFE0F Set Key",
            WS_CHILD,
            320, 373, 90, 30, hwnd, (HMENU)103, NULL, NULL);
        ApplyFontToControl(hShoutButton, hFont);

        control = CreateWindowW(L"STATIC", L"\u2699\uFE0F Config Panel:",
            WS_CHILD,
            60, 410, 130, 25, hwnd, (HMENU)506, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hConfigKeyEdit = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
            200, 408, 110, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hConfigKeyEdit, hFont);

        hConfigButton = CreateWindowW(L"BUTTON", L"\u270F\uFE0F Set Key",
            WS_CHILD,
            320, 408, 90, 30, hwnd, (HMENU)104, NULL, NULL);
        ApplyFontToControl(hConfigButton, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4A1 Press the assigned key in-game to open this panel",
            WS_CHILD,
            60, 445, 400, 25, hwnd, (HMENU)507, NULL, NULL);
        ApplyFontToControl(control, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F504 Toggle Mode:",
            WS_CHILD,
            60, 480, 130, 25, hwnd, (HMENU)513, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hVoiceToggleKeyEdit = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
            200, 478, 110, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hVoiceToggleKeyEdit, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4A1 Press the assigned key in-game to open this panel",
            WS_CHILD,
            60, 445, 400, 25, hwnd, (HMENU)507, NULL, NULL);
        ApplyFontToControl(control, hFont);

        // NOUVEAU: Titre pour la section Toggle Mode
        control = CreateWindowW(L"STATIC", L"\U0001F504 Voice Toggle Settings",
            WS_CHILD,
            40, 470, 250, 25, hwnd, (HMENU)514, NULL, NULL);
        ApplyFontToControl(control, hFontBold);

        control = CreateWindowW(L"STATIC", L"\U0001F504 Toggle Mode:",
            WS_CHILD,
            60, 500, 130, 25, hwnd, (HMENU)513, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hVoiceToggleKeyEdit = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
            200, 498, 110, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hVoiceToggleKeyEdit, hFont);

        hVoiceToggleButton = CreateWindowW(L"BUTTON", L"\u270F\uFE0F Set Key",
            WS_CHILD,
            320, 478, 90, 30, hwnd, (HMENU)106, NULL, NULL);
        ApplyFontToControl(hVoiceToggleButton, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4CF Voice Range Settings (meters)",
            WS_CHILD,
            40, 540, 350, 25, hwnd, (HMENU)508, NULL, NULL);
        ApplyFontToControl(control, hFontBold);

        control = CreateWindowW(L"STATIC", L"\U0001F92B Whisper:",
            WS_CHILD,
            60, 575, 90, 25, hwnd, (HMENU)509, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hDistanceWhisperEdit = CreateWindowW(L"EDIT", L"2.0",
            WS_CHILD | WS_BORDER | ES_CENTER,
            155, 573, 60, 28, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hDistanceWhisperEdit, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4AC Normal:",
            WS_CHILD,
            230, 575, 80, 25, hwnd, (HMENU)510, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hDistanceNormalEdit = CreateWindowW(L"EDIT", L"10.0",
            WS_CHILD | WS_BORDER | ES_CENTER,
            315, 573, 60, 28, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hDistanceNormalEdit, hFont);

        control = CreateWindowW(L"STATIC", L"\U0001F4E2 Shout:",
            WS_CHILD,
            390, 575, 70, 25, hwnd, (HMENU)511, NULL, NULL);
        ApplyFontToControl(control, hFont);

        hDistanceShoutEdit = CreateWindowW(L"EDIT", L"15.0",
            WS_CHILD | WS_BORDER | ES_CENTER,
            465, 573, 60, 28, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hDistanceShoutEdit, hFont);

        // NOUVEAU: Message des limites de distance
        hDistanceLimitMessage = CreateWindowW(L"STATIC", L"Maximum distance limit: 30 meters (set by Mumble's Maximum Distance setting)",
            WS_CHILD | SS_CENTER,
            40, 610, 520, 20, hwnd, (HMENU)512, NULL, NULL);
        ApplyFontToControl(hDistanceLimitMessage, hFont);

        // Boutons principaux - AJUSTÉS
        control = CreateWindowW(L"BUTTON", L"\u2705 Save Configuration",
            WS_VISIBLE | WS_CHILD,
            140, 640, 160, 40, hwnd, (HMENU)1, NULL, NULL);
        ApplyFontToControl(control, hFont);

        control = CreateWindowW(L"BUTTON", L"\u274C Cancel",
            WS_VISIBLE | WS_CHILD,
            320, 640, 120, 40, hwnd, (HMENU)2, NULL, NULL);
        ApplyFontToControl(control, hFont);

        // NOUVEAU: Zone d'affichage des messages de statut | NEW: Status message display area
        hStatusMessage = CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            40, 690, 520, 25, hwnd, (HMENU)600, NULL, NULL);
        ApplyFontToControl(hStatusMessage, hFont);

        loadVoiceDistancesFromConfig();

        // Charger les valeurs ACTUELLES depuis la configuration
        wchar_t gamePathFromConfig[MAX_PATH] = L"";

        // Lire le chemin depuis le fichier de config
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

                        // Extract parent directory (remove \ConanSandbox\Saved) | Extraire le répertoire parent
                        wcscpy_s(gamePathFromConfig, MAX_PATH, pathStart);
                        wchar_t* conanSandbox = wcsstr(gamePathFromConfig, L"\\ConanSandbox\\Saved");
                        if (conanSandbox) {
                            *conanSandbox = L'\0'; // Truncate at \ConanSandbox\Saved
                        }
                        break;
                    }
                }
                fclose(f);
                wchar_t* nl = wcschr(savedPath, L'\n');
                if (nl) *nl = L'\0';
                wchar_t* cr = wcschr(savedPath, L'\r');
                if (cr) *cr = L'\0';
                size_t converted = 0;
                wcstombs_s(&converted, modFilePath, MAX_PATH, savedPath, _TRUNCATE);
                snprintf(modFilePath, MAX_PATH, "%s\\Pos.txt", modFilePath);
            }
        }

        // Set the actual values | Définir les valeurs réelles
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

        // NOUVEAU: Définir les valeurs de distance actuelles dans les champs
        wchar_t whisperText[32], normalText[32], shoutText[32];
        swprintf(whisperText, 32, L"%.1f", distanceWhisper);
        swprintf(normalText, 32, L"%.1f", distanceNormal);
        swprintf(shoutText, 32, L"%.1f", distanceShout);

        SetWindowTextW(hDistanceWhisperEdit, whisperText);
        SetWindowTextW(hDistanceNormalEdit, normalText);
        SetWindowTextW(hDistanceShoutEdit, shoutText);

        updateDistanceLimitMessage();

        ShowCategoryControls(1);
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        int controlId = GetDlgCtrlID(hwndStatic);

        if (controlId == 512) { // Message des limites de distance
            SetTextColor(hdcStatic, RGB(100, 100, 100)); // Gris discret
        }
        else if (controlId == 600) { // Message de statut
            wchar_t messageText[256];
            GetWindowTextW(hwndStatic, messageText, 256);

            // Détecter le type de message par son contenu | Detect message type by content
            if (wcsstr(messageText, L"\u26A0") || wcsstr(messageText, L"Error") || wcsstr(messageText, L"does not exist")) {
                // Message d'erreur - rouge | Error message - red
                SetTextColor(hdcStatic, RGB(220, 53, 69));
            }
            else if (wcsstr(messageText, L"\u2705") || wcsstr(messageText, L"\u2699") || wcsstr(messageText, L"success")) {
                // Message de succès - vert | Success message - green
                SetTextColor(hdcStatic, RGB(40, 167, 69));
            }
            else {
                // Message normal - gris | Normal message - gray
                SetTextColor(hdcStatic, RGB(108, 117, 125));
            }
        }
        else {
            SetTextColor(hdcStatic, RGB(33, 37, 41));
        }

        SetBkColor(hdcStatic, RGB(248, 249, 250));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, RGB(33, 37, 41));
        SetBkColor(hdcEdit, RGB(255, 255, 255));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 301: ShowCategoryControls(1); break;
        case 302: ShowCategoryControls(2); break;
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

        case 1: { // Sauvegarder
            wchar_t gameFolder[MAX_PATH];
            GetWindowTextW(hSavedPathEdit, gameFolder, MAX_PATH);

            if (!savedExistsInFolder(gameFolder)) {
                showStatusMessage(L"\u26A0\uFE0F The 'Saved' folder does not exist in ConanSandbox. It must be present for the plugin to work.", TRUE);
                break;
            }

            // Vérifier si la configuration était déjà sauvée AVANT la sauvegarde
            BOOL wasAlreadySaved = isPatchAlreadySaved();

            enableDistanceMuting = (IsDlgButtonChecked(hwnd, 201) == BST_CHECKED);
            enableAutomaticChannelChange = (IsDlgButtonChecked(hwnd, 203) == BST_CHECKED);
            enableVoiceToggle = (IsDlgButtonChecked(hwnd, 204) == BST_CHECKED);

            wchar_t distWhisper[32], distNormal[32], distShout[32];
            GetWindowTextW(hDistanceWhisperEdit, distWhisper, 32);
            GetWindowTextW(hDistanceNormalEdit, distNormal, 32);
            GetWindowTextW(hDistanceShoutEdit, distShout, 32);

            writeFullConfiguration(gameFolder, distWhisper, distNormal, distShout);

            // NOUVEAU: Appeler saveVoiceSettings pour s'assurer que les distances sont correctement sauvées
            saveVoiceSettings();

            // Afficher le message seulement si ce n'était pas déjà sauvé
            if (!wasAlreadySaved) {
                showStatusMessage(L"\u2705 Configuration saved successfully!", FALSE);
            }
            else {
                showStatusMessage(L"\u2699\uFE0F Configuration updated successfully!", FALSE);
            }
            break;
        }

        case 2: DestroyWindow(hwnd); break;
        }
        break;

    case WM_TIMER:
        if (wParam == 1) {
            // Timer pour capture des touches | Timer for key capture
            processKeyCapture();
        }
        else if (wParam == 2) {
            // Timer pour effacer le message de statut | Timer to clear status message
            clearStatusMessage();
            KillTimer(hwnd, 2);
        }
        break;

    case WM_DESTROY:
        if (hFont) DeleteObject(hFont);
        if (hFontBold) DeleteObject(hFontBold);
        if (hFontLarge) DeleteObject(hFontLarge);
        if (hFontEmoji) DeleteObject(hFontEmoji);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}

// Check if patch is already saved in config file | Vérifie si le patch est déjà enregistré dans le fichier de configuration
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
            // Check if there's something after 'SavedPath=' | Vérifie qu'il y a quelque chose après 'SavedPath='
            wchar_t* value = line + 10;
            // Ignore spaces and line breaks | Ignore les espaces et les retours à la ligne
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

// Path selection dialog thread function | Fonction de thread pour la boîte de dialogue de sélection de chemin
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

// Check if mod file is active (updated in last 5 seconds) | Vérifie si le fichier mod est actif (mis à jour dans les 5 dernières secondes)
static BOOL checkModFileActive() {
    // Use simple function to verify file existence | Utilise une fonction simple pour vérifier l'existence du fichier
    DWORD attributes = GetFileAttributesA(modFilePath);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        // File doesn't exist or path is invalid | Le fichier n'existe pas ou le chemin est invalide
        return FALSE;
    }

    // If we get here, file exists | Si on arrive ici, le fichier existe
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(modFilePath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE; // Unable to get file information | Impossible d'obtenir les informations du fichier
    }
    FindClose(hFind);

    ULARGE_INTEGER ull = { 0 };
    ull.LowPart = findFileData.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = findFileData.ftLastWriteTime.dwHighDateTime;
    time_t fileTime = (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);

    // File is considered active if modified less than 5 seconds ago | Le fichier est considéré comme actif s'il a été modifié il y a moins de 5 secondes
    if (time(NULL) - fileTime <= 5) {
        return TRUE;
    }

    return FALSE;
}

// Read mod file data safely and robustly | Lit les données du fichier mod de manière sécurisée et robuste
static BOOL readModFileData(struct ModFileData* data) {
    if (!data) return FALSE;

    FILE* file = fopen(modFilePath, "rb"); // Open in binary mode to avoid encoding issues | Ouvre en mode binaire pour éviter les problèmes d'encodage
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

    // Universal format parsing - no cleaning needed | Parsing du format universel - pas de nettoyage nécessaire
    // Format: SEQ=309 X=1955.529907 Y=354.150146 Z=300.107513 YAW=106.453 YAWY=348.125

    // Search and read SEQ | Rechercher et lire le SEQ
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

    // Search and read X | Rechercher et lire le X
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

    // Search and read Y | Rechercher et lire le Y
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

    // Search and read Z | Rechercher et lire le Z
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

    // Search and read YAW | Rechercher et lire le YAW
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

    // Search and read YAWY | Rechercher et lire le YAWY
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
        else if (!isModActive) {
            if (GetTickCount64() - lastModDataTick > 10000) { // 10 seconds
                // If mod file not updated for 10 seconds, consider it inactive
                useModFile = FALSE;
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
            useModFile = TRUE;
            lastModDataTick = currentTick;
        }
        else if (!isModActive) {
            if (currentTick - lastModDataTick > 1500) {
                useModFile = FALSE;
            }
        }

        if (!useModFile) {
            if (enableMemoryOffsets && getPlayerCoordinates()) {
                coordinatesValid = TRUE;
            }
            else {
                coordinatesValid = FALSE;
            }
        }

        Sleep(20);
    }
}

static volatile int connectionAttempts = 0; // Connection attempt counter | Compteur de tentatives de connexion

static volatile BOOL fusionRequestSent = FALSE; // New flag to check if FUSION request was sent | Nouveau flag pour vérifier si la requête FUSION a été envoyée

// Connect to server and retrieve version/zones | Se connecter au serveur et récupérer version/zones
static void connectToServer(void* param) {
    if (!useServer) {
        if (enableLogServer) {
            displayInChat("Server is disabled, connection ignored | Le serveur est désactivé, connexion ignorée");
        }
        return;
    }

    if (connectionAttempts >= 2) return;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        if (enableLogServer) {
            displayInChat("WSAStartup failed");
        }
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        connectionAttempts++;
        closesocket(sock);
        return;
    }

    Sleep(1000);

    if (!fusionRequestSent) {
        send(sock, "FUSION\n", (int)strlen("FUSION\n"), 0);
        fusionRequestSent = TRUE;
    }

    char* buffer = (char*)malloc(16384);
    if (buffer == NULL) {
        if (enableLogServer) {
            displayInChat("Memory allocation error | Erreur d'allocation de mémoire");
        }
        closesocket(sock);
        WSACleanup();
        return;
    }

    int bytesRead = recv(sock, buffer, 16383, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';

        // Context variable for strtok_s | Variable pour le contexte de strtok_s
        char* context = NULL;

        // Split response by line using \n as separator | Découpe la réponse par ligne en utilisant \n comme séparateur
        char* line = strtok_s(buffer, "\n", &context);
        while (line != NULL) {
            // Check if line contains "VERSION:" | Vérifie si la ligne contient "VERSION:"
            if (strncmp(line, "VERSION:", 8) == 0) {
                isConnected = (strcmp(line, REQUIRED_VERSION) == 0);
                if (enableLogServer) {
                    mumbleAPI.log(ownID, isConnected ? "The version is compatible." : "The version is not compatible.");
                }
                versionReceived = TRUE; // Mark version as received | Marquer la version comme reçue
            }
            // Check if line contains "ZONES:" | Vérifie si la ligne contient "ZONES:"
            else if (strncmp(line, "ZONES:", 6) == 0) {
                parseZones(line); // Call parseZones to analyze zones | Appelle parseZones pour analyser les zones
                zonesReceived = TRUE; // Mark zones as received | Marquer les zones comme reçues
            }
            // Move to next line | Passe à la ligne suivante
            line = strtok_s(NULL, "\n", &context);
        }
    }
    else {
        if (enableLogServer) {
            displayInChat("Error reading server response | Erreur lors de la lecture de la réponse du serveur");
        }
    }

    // If version and zones received, close connection | Si version et zones reçus, fermer la connexion
    if (versionReceived && zonesReceived) {
        closesocket(sock); // Close connection | Ferme la connexion
        WSACleanup();      // Release connection resources | Libère les ressources de la connexion
    }

    free(buffer);
}

// Start version check thread | Démarrer le thread de vérification de version
static void startVersionCheck() {
    if (!useServer) {
        if (enableLogServer) {
            displayInChat("Version checking is disabled | La vérification de version est désactivée");
        }
        return;
    }

    if (!enableStartVersionCheck) return;
    _beginthread(connectToServer, 0, NULL);
}

// Function to get process ID of ConanSandbox.exe | Fonction pour obtenir l'ID du processus ConanSandbox.exe
static BOOL findProcessId(const TCHAR* processName, DWORD* processID) {
    if (!enableFindProcessId) return FALSE;  // Check if enabled | Vérification si activée

    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32 = { 0 };
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return FALSE;
    }

    do {
        if (_tcsicmp(pe32.szExeFile, processName) == 0) {
            *processID = pe32.th32ProcessID;
            CloseHandle(hProcessSnap);
            return TRUE;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return FALSE;
}

// Function to get base address of ConanSandbox.exe | Fonction pour obtenir l'adresse de base de ConanSandbox.exe
static BOOL findBaseAddress(DWORD processID, LPVOID* baseAddress) {
    if (!enableFindBaseAddress) return FALSE;  // Check if enabled | Vérification si activée

    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 me32 = { 0 };

    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
    if (hModuleSnap == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    me32.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(hModuleSnap, &me32)) {
        CloseHandle(hModuleSnap);
        return FALSE;
    }

    *baseAddress = me32.modBaseAddr;
    CloseHandle(hModuleSnap);
    return TRUE;
}

// Function to read a float at given memory address | Fonction pour lire un float à une adresse mémoire donnée
static BOOL readMemoryValue(HANDLE hProcess, LPVOID address, float* value) {
    if (!enableReadMemoryValue) return FALSE;  // Check if enabled | Vérification si activée

    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, address, value, sizeof(float), &bytesRead) && bytesRead == sizeof(float)) {
        return TRUE;
    }
    return FALSE;
}

// Read coordinates using offset chain | Lire les coordonnées en utilisant une chaîne d'offsets
static BOOL readCoordinates(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR* offsets, SIZE_T offsetCount, float* value) {
    DWORD_PTR currentAddress = baseAddress;
    for (SIZE_T i = 0; i < offsetCount; ++i) {
        if (!ReadProcessMemory(hProcess, (LPCVOID)currentAddress, &currentAddress, sizeof(currentAddress), NULL)) {
            return FALSE; // Return FALSE if error encountered | Retourne FALSE si une erreur est rencontrée
        }
        currentAddress += offsets[i];
    }
    return readMemoryValue(hProcess, (LPVOID)currentAddress, value);
}

// Function to read avatar X axis without backup | Fonction pour lire l'axe X de l'avatar sans sauvegarde
static BOOL readavatarAxisX(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR* offsets, SIZE_T offsetCount, float* value) {
    // Use same logic as Y axis but without backup offsets | On utilise la même logique que pour l'axe Y, mais sans les offsets de secours
    return readCoordinates(hProcess, baseAddress, offsets, offsetCount, value);
}

// Function to read avatar Y axis without backup | Fonction pour lire l'axe Y de l'avatar sans sauvegarde
static BOOL readavatarAxisY(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR* offsets, SIZE_T offsetCount, float* value) {
    // Use same logic as Y axis but without backup offsets | On utilise la même logique que pour l'axe Y, mais sans les offsets de secours
    return readCoordinates(hProcess, baseAddress, offsets, offsetCount, value);
}

// Function to read avatar Z axis without backup | Fonction pour lire l'axe Z de l'avatar sans sauvegarde
static BOOL readavatarAxisZ(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR* offsets, SIZE_T offsetCount, float* value) {
    // Use same logic as other axes but without backup offsets | On utilise la même logique que pour les autres axes, mais sans les offsets de secours
    return readCoordinates(hProcess, baseAddress, offsets, offsetCount, value);
}

// Function to read yaw and convert to X and Y axes | Fonction pour lire le yaw et le convertir en axes X et Y
static BOOL readYawToAxes(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR* offsets, SIZE_T offsetCount, float* yawX, float* yawY) {
    float yaw;
    if (readCoordinates(hProcess, baseAddress, offsets, offsetCount, &yaw)) {
        // Convert yaw to radians then to X and Y components | Conversion du yaw en radians puis en composantes X et Y
        float yawRad = yaw * 3.14159265f / 180.0f;
        *yawX = (float)cos(yawRad);
        *yawY = (float)sin(yawRad);
        return TRUE;
    }
    return FALSE;
}

// Function to read yaw Z angle and convert to a normalized vector component | Fonction pour lire l'angle Z du lacet (pitch) et le convertir en une composante de vecteur normalisée
static BOOL readYawZ(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR* offsets, SIZE_T offsetCount, float* yawZ) {
    float angle;
    // Read the pitch angle in degrees | Lit l'angle de tangage (pitch) en degrés
    if (readCoordinates(hProcess, baseAddress, offsets, offsetCount, &angle)) {
        // Convert pitch angle (in degrees) to the Z component of a directional vector. | Convertit l'angle de tangage (en degrés) en composante Z d'un vecteur directionnel.
        // Mumble expects this as sin(pitch_in_radians). | Mumble attend cette valeur sous la forme sin(pitch_en_radians).
        // We multiply by -1 because in many games, looking up is a negative angle. | Nous multiplions par -1 car dans de nombreux jeux, regarder vers le haut correspond à un angle négatif.
        float pitchRad = -angle * 3.14159265f / 180.0f;
        *yawZ = sinf(pitchRad);
        return TRUE;
    }
    return FALSE;
}

// Function to get player coordinates | Fonction pour obtenir les coordonnées du joueur
static BOOL getPlayerCoordinates() {
    // Check if memory offsets are enabled
    if (!enableMemoryOffsets) {
        return FALSE;
    }

    if (!enableGetPlayerCoordinates) return FALSE;  // Check if enabled | Vérification si activée

    // Before reading coordinates, check version | Avant de lire les coordonnées, vérifiez la version
    static bool versionChecked = false;  // Variable to track if version was checked | Variable pour le suivi de la vérification de version
    static bool versionIncompatibleLogged = false; // Variable to track if message was displayed | Variable pour le suivi de l'affichage du message

    if (!versionChecked && useServer) {
        // Check connection before getting coordinates | Vérifie la connexion avant d'obtenir les coordonnées
        if (!isConnected) {
            startVersionCheck(); // Start version check thread | Démarre le thread de vérification de version
            return FALSE; // Don't proceed if not connected | Ne pas procéder si pas connecté
        }
        versionIncompatibleLogged = false; // Reset if version is correct | Réinitialiser si la version est correcte
        versionChecked = true; // Mark that version was checked | Marquer que la version a été vérifiée
    }

    static bool processIdNotFound = false;
    static bool successMessageLogged = false; // Declaration here for success tracking | Déclaration ici pour le suivi des succès
    DWORD processID = 0;
    LPVOID baseAddress = NULL;
    HANDLE hProcess = NULL;

    // Primary base addresses for coordinates | Adresses de base principales pour les coordonnées
    DWORD_PTR baseAddressOffsetX_Principal = 0x05AA56F0;
    DWORD_PTR baseAddressOffsetY_Principal = 0x05AA56F0;
    DWORD_PTR baseAddressOffsetZ_Principal = 0x05AA56F0;

    // Base addresses for yaw offsets | Adresses de base pour les offsets yaw
    DWORD_PTR baseAddressOffsetYaw_Single = 0x05D9CF70; // For mode 1 single yaw offset | Pour le mode 1 offset yaw unique
    DWORD_PTR baseAddressOffsetYawX_Native = 0x05B3AF98; // For mode 2 native yaw X offsets | Pour le mode 2 offsets natifs yaw X
    DWORD_PTR baseAddressOffsetYawY_Native = 0x05CFDF60; // For mode 2 native yaw Y offsets | Pour le mode 2 offsets natifs yaw Y
    DWORD_PTR baseAddressOffsetYawZ_Principal = 0x05AA18F8; // TO REPLACE: Base address for YAW Z | À REMPLACER: Adresse de base pour YAW Z

    // Backup base addresses | Adresses de base de secours
    DWORD_PTR baseAddressOffsetX_Backup = 0x05884C30;
    DWORD_PTR baseAddressOffsetY_Backup = 0x05884C30;
    DWORD_PTR baseAddressOffsetZ_Backup = 0x05884C30;

    // Backup base addresses for yaw offsets | Adresses de base backup pour les offsets yaw
    DWORD_PTR baseAddressOffsetYaw_Single_Backup = 0x05D9CF70; // Backup for mode 1 single yaw offset | Backup pour le mode 1 offset yaw unique
    DWORD_PTR baseAddressOffsetYawX_Native_Backup = 0x05884C30; // Backup for mode 2 native yaw X offsets | Backup pour le mode 2 offsets natifs yaw X
    DWORD_PTR baseAddressOffsetYawY_Native_Backup = 0x05884C30; // Backup for mode 2 native yaw Y offsets | Backup pour le mode 2 offsets natifs yaw Y
    DWORD_PTR baseAddressOffsetYawZ_Backup = 0x05D9CF70; // TO REPLACE: Backup base address for YAW Z | À REMPLACER: Adresse de base de secours pour YAW Z

    // Primary offsets | Offsets principaux
    DWORD_PTR offsetX[] = { 0x218, 0x7B0, 0x1E0, 0xA00, 0x3F0, 0x170, 0x1B0 };
    DWORD_PTR offsetY[] = { 0x218, 0x7B0, 0x1E0, 0xA00, 0x3F0, 0x170, 0x1B4 };
    DWORD_PTR offsetZ[] = { 0x218, 0x7B0, 0x1E0, 0xA00, 0x3F0, 0x170, 0x1B8 };


    // Yaw offsets | Offsets pour le yaw
    DWORD_PTR offsetYawSingle[] = { 0x30, 0x400, 0x170, 0x100, 0x40, 0xF8, 0x1E4 }; // Single yaw offset | Offset pour le yaw unique
    DWORD_PTR offsetYawXNative[] = { 0x10, 0x420, 0xC58 }; // Native yaw X offset | Offset pour yaw X natif
    DWORD_PTR offsetYawYNative[] = { 0x8, 0xC8, 0x8, 0x438, 0x14C }; // Native yaw Y offset | Offset pour yaw Y natif
    DWORD_PTR offsetYawZ[] = { 0x38, 0x0, 0x30, 0x78, 0x170, 0x20, 0x418 }; // TO REPLACE: Offsets for YAW Z | À REMPLACER: Offsets pour YAW Z

    // Backup offsets | Offsets de secours
    DWORD_PTR backupOffsetX[] = { 0x0, 0x48, 0x8, 0x510, 0x8, 0x948, 0x1B0 };
    DWORD_PTR backupOffsetY[] = { 0x0, 0x448, 0x100, 0x30, 0xF8, 0xF8, 0x1B4 };
    DWORD_PTR backupOffsetZ[] = { 0x0, 0x370, 0x80, 0x80, 0x20, 0x170, 0x1B8 };

    // Backup yaw offsets | Offsets de secours pour le yaw
    DWORD_PTR backupOffsetYawSingle[] = { 0x0, 0x48, 0x8, 0x510, 0x8, 0x948, 0x1BC }; // Single yaw backup | Backup yaw unique
    DWORD_PTR backupOffsetYawXNative[] = { 0x0, 0x48, 0x8, 0x510, 0x8, 0x948, 0x1B8 }; // Native yaw X backup | Backup yaw X natif
    DWORD_PTR backupOffsetYawYNative[] = { 0x0, 0x448, 0x100, 0x30, 0xF8, 0xF8, 0x1BC }; // Native yaw Y backup | Backup yaw Y natif
    DWORD_PTR backupOffsetYawZ[] = { 0x0, 0x48, 0x8, 0x510, 0x8, 0x948, 0x1C0 }; // TO REPLACE: Backup offsets for YAW Z | À REMPLACER: Offsets de secours pour YAW Z

    TCHAR targetProcess[] = TEXT("ConanSandbox.exe"); // Target process name | Nom du processus cible

    if (!findProcessId(targetProcess, &processID)) {
        if (!processIdNotFound) {
            if (enableLogProcess) {
                mumbleAPI.log(ownID, u8"Erreur : Impossible de trouver ConanSandbox.exe (2).");
            }
            processIdNotFound = true;
        }
        return FALSE;
    }

    processIdNotFound = false;
    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (hProcess == NULL) {
        if (enableLogProcess) {
            mumbleAPI.log(ownID, u8"Erreur : Impossible d'ouvrir ConanSandbox.exe.");
        }
        return FALSE;
    }

    if (!findBaseAddress(processID, &baseAddress)) {
        if (enableLogProcess) {
            mumbleAPI.log(ownID, u8"Erreur : Impossible de trouver ConanSandbox.exe 1.");
        }
        CloseHandle(hProcess);
        return FALSE;
    }

    BOOL errorX = FALSE, errorY = FALSE, errorZ = FALSE;
    static bool errorLoggedX = false, errorLoggedY = false, errorLoggedZ = false;

    // Read X axis with separate offsets | Lire l'axe X avec les offsets séparés
    DWORD_PTR currentAddressX = (DWORD_PTR)baseAddress + baseAddressOffsetX_Principal;
    if (!readCoordinates(hProcess, currentAddressX, offsetX, sizeof(offsetX) / sizeof(offsetX[0]), &axe_x)) {
        usedPrimaryX = false; // Indicate primary offset failed | Indiquer que l'offset principal a échoué
        if (enableBackupOffsetX) { // Check if backup offsets are enabled | Vérifier si les offsets de secours sont activés
            currentAddressX = (DWORD_PTR)baseAddress + baseAddressOffsetX_Backup;
            if (!readCoordinates(hProcess, currentAddressX, backupOffsetX, sizeof(backupOffsetX) / sizeof(backupOffsetX[0]), &axe_x)) {
                errorX = TRUE; // Log error | Enregistrer l'erreur
            }
        }
    }

    // Read Y axis with separate offsets | Lire l'axe Y avec les offsets séparés
    DWORD_PTR currentAddressY = (DWORD_PTR)baseAddress + baseAddressOffsetY_Principal;
    if (!readCoordinates(hProcess, currentAddressY, offsetY, sizeof(offsetY) / sizeof(offsetY[0]), &axe_y)) {
        usedPrimaryY = false; // Indicate primary offset failed | Indiquer que l'offset principal a échoué
        if (enableBackupOffsetY) { // Check if backup offsets are enabled | Vérifier si les offsets de secours sont activés
            currentAddressY = (DWORD_PTR)baseAddress + baseAddressOffsetY_Backup;
            if (!readCoordinates(hProcess, currentAddressY, backupOffsetY, sizeof(backupOffsetY) / sizeof(backupOffsetY[0]), &axe_y)) {
                errorY = TRUE; // Log error | Enregistrer l'erreur
            }
        }
    }

    // Read Z axis with separate offsets | Lire l'axe Z avec les offsets séparés
    DWORD_PTR currentAddressZ = (DWORD_PTR)baseAddress + baseAddressOffsetZ_Principal;
    if (!readCoordinates(hProcess, currentAddressZ, offsetZ, sizeof(offsetZ) / sizeof(offsetZ[0]), &axe_z)) {
        usedPrimaryZ = false; // Indicate primary offset failed | Indiquer que l'offset principal a échoué
        if (enableBackupOffsetZ) { // Check if backup offsets are enabled | Vérifier si les offsets de secours sont activés
            currentAddressZ = (DWORD_PTR)baseAddress + baseAddressOffsetZ_Backup;
            if (!readCoordinates(hProcess, currentAddressZ, backupOffsetZ, sizeof(backupOffsetZ) / sizeof(backupOffsetZ[0]), &axe_z)) {
                errorZ = TRUE; // Log error | Enregistrer l'erreur
            }
        }
    }

    // Yaw offset handling if enabled | Gestion des offsets yaw si activés
    if (enableYawOffsets) {
        BOOL yawErrorX = FALSE, yawErrorY = FALSE, yawErrorZ = FALSE;

        if (useNativeTwoYawOffsets) {
            // Mode 2: Native separate offsets for X and Y
            DWORD_PTR currentAddressYawXNative = (DWORD_PTR)baseAddress + baseAddressOffsetYawX_Native;
            if (!readavatarAxisX(hProcess, currentAddressYawXNative, offsetYawXNative, sizeof(offsetYawXNative) / sizeof(offsetYawXNative[0]), &avatarAxisX)) {
                usedPrimaryYawX = false;
                if (enableBackupYawOffsets) {
                    currentAddressYawXNative = (DWORD_PTR)baseAddress + baseAddressOffsetYawX_Native_Backup;
                    if (!readavatarAxisX(hProcess, currentAddressYawXNative, backupOffsetYawXNative, sizeof(backupOffsetYawXNative) / sizeof(backupOffsetYawXNative[0]), &avatarAxisX)) {
                        yawErrorX = TRUE;
                    }
                }
            }

            DWORD_PTR currentAddressYawYNative = (DWORD_PTR)baseAddress + baseAddressOffsetYawY_Native;
            if (!readavatarAxisY(hProcess, currentAddressYawYNative, offsetYawYNative, sizeof(offsetYawYNative) / sizeof(offsetYawYNative[0]), &avatarAxisY)) {
                usedPrimaryYawY = false;
                if (enableBackupYawOffsets) {
                    currentAddressYawYNative = (DWORD_PTR)baseAddress + baseAddressOffsetYawY_Native_Backup;
                    if (!readavatarAxisY(hProcess, currentAddressYawYNative, backupOffsetYawYNative, sizeof(backupOffsetYawYNative) / sizeof(backupOffsetYawYNative[0]), &avatarAxisY)) {
                        yawErrorY = TRUE;
                    }
                }
            }
        }
        else {
            // Mode 1: Single yaw offset converted to X/Y
            DWORD_PTR currentAddressYawSingle = (DWORD_PTR)baseAddress + baseAddressOffsetYaw_Single;
            if (!readYawToAxes(hProcess, currentAddressYawSingle, offsetYawSingle, sizeof(offsetYawSingle) / sizeof(offsetYawSingle[0]), &avatarAxisX, &avatarAxisY)) {
                usedPrimaryYawX = false;
                usedPrimaryYawY = false;
                if (enableBackupYawOffsets) {
                    currentAddressYawSingle = (DWORD_PTR)baseAddress + baseAddressOffsetYaw_Single_Backup;
                    if (!readYawToAxes(hProcess, currentAddressYawSingle, backupOffsetYawSingle, sizeof(backupOffsetYawSingle) / sizeof(backupOffsetYawSingle[0]), &avatarAxisX, &avatarAxisY)) {
                        yawErrorX = TRUE;
                        yawErrorY = TRUE;
                    }
                }
            }
        }

        // Read YAW Z in both modes
        DWORD_PTR currentAddressYawZ = (DWORD_PTR)baseAddress + baseAddressOffsetYawZ_Principal;
        if (!readYawZ(hProcess, currentAddressYawZ, offsetYawZ, sizeof(offsetYawZ) / sizeof(offsetYawZ[0]), &avatarAxisZ)) {
            usedPrimaryYawZ = false;
            if (enableBackupYawOffsets) {
                currentAddressYawZ = (DWORD_PTR)baseAddress + baseAddressOffsetYawZ_Backup;
                if (!readYawZ(hProcess, currentAddressYawZ, backupOffsetYawZ, sizeof(backupOffsetYawZ) / sizeof(backupOffsetYawZ[0]), &avatarAxisZ)) {
                    yawErrorZ = TRUE;
                    avatarAxisZ = 0.0f; // Default to 0 on failure
                }
            }
            else {
                avatarAxisZ = 0.0f; // Default to 0 if backup is disabled and primary fails
            }
        }
    }

    // Success message logging | Journalisation du message de réussite
    if (!successMessageLogged && !(errorX || errorY || errorZ)) {
        // Create success message based on used offsets | Créer le message de succès en fonction des offsets utilisés
        char successMsg[256];
        sprintf_s(successMsg, sizeof(successMsg), u8"Succès : Coordonnées trouvées : X %s, Y %s, Z %s.",
            usedPrimaryX ? "principal" : "secondaire",
            usedPrimaryY ? "principal" : "secondaire",
            usedPrimaryZ ? "principal" : "secondaire");
        if (enableLogOffsets) {
            //mumbleAPI.log(ownID, successMsg);
        }
        successMessageLogged = true; // Mark that message was logged | Marquer que le message a été logué
    }
    else if (errorX || errorY || errorZ) {
        // Reset successMessageLogged if an error occurs | Réinitialiser successMessageLogged si une erreur se produit
        successMessageLogged = false;
    }

    // Basic mechanism for main axes | Mécanisme de base pour les axes principaux
    static bool errorLogged = false;
    static time_t lastErrorTime = 0; // To track time of last error message | Pour garder la trace du temps du dernier message d'erreur
    if ((errorX || errorY || errorZ) && (!errorLogged || difftime(time(NULL), lastErrorTime) >= 60)) {
        char errorMsg[256];
        strcpy_s(errorMsg, sizeof(errorMsg), u8"Erreur : Impossible de récupérer la position des axes : ");
        if (errorX) strcat_s(errorMsg, sizeof(errorMsg), "X ");
        if (errorY) strcat_s(errorMsg, sizeof(errorMsg), "Y ");
        if (errorZ) strcat_s(errorMsg, sizeof(errorMsg), "Z ");

        // Add retry message here | Ajoutez le message de nouvelle tentative ici
        strcat_s(errorMsg, sizeof(errorMsg), u8"nouvelle tentative en cours.");

        if (enableLogOffsets) {
            mumbleAPI.log(ownID, errorMsg);
        }
        errorLogged = true;
        lastErrorTime = time(NULL); // Update last error message time | Met à jour le temps du dernier message d'erreur
    }
    else if (!errorX && !errorY && !errorZ) {
        errorLogged = false; // Reset if all readings succeed | Réinitialiser si toutes les lectures réussissent
    }

    // Group backup axis error messages | Regroupement des messages d'erreur des axes de secours
    static bool errorLoggedBackup = false;
    static time_t lastErrorTimeBackup = 0; // To track time of last error message | Pour garder la trace du temps du dernier message d'erreur
    if ((errorX || errorY || errorZ) && (!errorLoggedBackup || difftime(time(NULL), lastErrorTimeBackup) >= 60)) {
        char errorMsg[256];
        strcpy_s(errorMsg, sizeof(errorMsg), u8"Erreur : Impossible de lire les axes de secours : ");
        if (errorX) strcat_s(errorMsg, sizeof(errorMsg), "X ");
        if (errorY) strcat_s(errorMsg, sizeof(errorMsg), "Y ");
        if (errorZ) strcat_s(errorMsg, sizeof(errorMsg), "Z ");

        // Add retry message here | Ajoutez le message de nouvelle tentative ici
        strcat_s(errorMsg, sizeof(errorMsg), u8"nouvelle tentative en cours.");

        if (enableLogOffsets) {
            mumbleAPI.log(ownID, errorMsg);
        }
        errorLoggedBackup = true;
        lastErrorTimeBackup = time(NULL); // Update last error message time | Met à jour le temps du dernier message d'erreur
    }
    else if (!errorX && !errorY && !errorZ) {
        errorLoggedBackup = false; // Reset if all readings succeed | Réinitialiser si toutes les lectures réussissent
    }

    CloseHandle(hProcess);
    return !(errorX || errorY || errorZ);
}

// Read configuration from file | Lire la configuration depuis le fichier
static void readConfigurationSettings() {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) return;

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    BOOL fileExists = (GetFileAttributesW(configFile) != INVALID_FILE_ATTRIBUTES);
    BOOL foundSavedPath = FALSE;
    BOOL foundEnableMemoryOffsets = FALSE;
    BOOL foundEnableAutomaticChannelChange = FALSE;

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

                wchar_t valLower[16];
                int i = 0;
                for (; i < 15 && val[i]; ++i) valLower[i] = (wchar_t)towlower(val[i]);
                valLower[i] = 0;

                if (wcsncmp(key, L"SavedPath", 9) == 0) {
                    wcsncpy_s(savedPathValue, MAX_PATH, val, _TRUNCATE);
                    foundSavedPath = TRUE;
                }
                else if (wcsncmp(key, L"EnableMemoryOffsets", 19) == 0) {
                    enableMemoryOffsets = (wcscmp(valLower, L"true") == 0 || wcscmp(valLower, L"1") == 0);
                    foundEnableMemoryOffsets = TRUE;
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
                // NOUVEAU: Lire aussi les distances ici
                else if (wcsncmp(key, L"DistanceWhisper", 15) == 0) {
                    distanceWhisper = (float)_wtof(val);
                }
                else if (wcsncmp(key, L"DistanceNormal", 14) == 0) {
                    distanceNormal = (float)_wtof(val);
                }
                else if (wcsncmp(key, L"DistanceShout", 13) == 0) {
                    distanceShout = (float)_wtof(val);
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

    // Si le fichier n'existe pas ou qu'il manque une clé => réécriture
    if (!fileExists || !foundSavedPath || !foundEnableMemoryOffsets || !foundEnableAutomaticChannelChange) {
        FILE* f = _wfopen(configFile, L"w");
        if (f) {
            fwprintf(f, L"SavedPath=%s\n", foundSavedPath ? savedPathValue : L"");
            fwprintf(f, L"EnableMemoryOffsets=%s\n", enableMemoryOffsets ? L"true" : L"false");
            fwprintf(f, L"EnableAutomaticChannelChange=%s\n", enableAutomaticChannelChange ? L"true" : L"false");
            fwprintf(f, L"WhisperKey=%d\n", whisperKey);
            fwprintf(f, L"NormalKey=%d\n", normalKey);
            fwprintf(f, L"ShoutKey=%d\n", shoutKey);
            fwprintf(f, L"ConfigUIKey=%d\n", configUIKey);
            fwprintf(f, L"EnableDistanceMuting=%s\n", enableDistanceMuting ? L"true" : L"false");
            // NOUVEAU: Écrire aussi les distances par défaut
            fwprintf(f, L"DistanceWhisper=%.1f\n", distanceWhisper);
            fwprintf(f, L"DistanceNormal=%.1f\n", distanceNormal);
            fwprintf(f, L"DistanceShout=%.1f\n", distanceShout);
            fclose(f);
        }
    }
}

// NOUVEAU: Fonction pour forcer l'initialisation complète
static void forceCompleteInitialization() {
    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Force complete initialization starting...");
    }

    // S'assurer que tous les systèmes sont prêts
    readConfigurationSettings();

    // Vérifier si modFilePath est configuré
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

                        // Convertir en chemin modFilePath
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
        snprintf(msg, sizeof(msg), "Configuration status - modFilePath: %s, enableDistanceMuting: %s, enableMemoryOffsets: %s",
            modFilePath, enableDistanceMuting ? "TRUE" : "FALSE", enableMemoryOffsets ? "TRUE" : "FALSE");
        mumbleAPI.log(ownID, msg);
    }
}

// Plugin initialization function
mumble_error_t mumble_init(mumble_plugin_id_t pluginID) {
    ownID = pluginID;
    setlocale(LC_NUMERIC, "C");

    // Enable all functions
    enableSetMaximumAudioDistance = TRUE;
    enableCheckPlayerZone = TRUE;
    enableCheckVersionThread = TRUE;
    enableStartVersionCheck = TRUE;
    enableFindProcessId = TRUE;
    enableFindBaseAddress = TRUE;
    enableReadMemoryValue = TRUE;
    enableGetPlayerCoordinates = TRUE;

    // Initialization
    useModFile = FALSE;
    lastFileCheck = 0;
    lastSeq = -1;
    modDataValid = FALSE;

    // NOUVEAU: Initialisation du système de voix
    memset(&localVoiceData, 0, sizeof(VoiceData));
    localVoiceData.voiceMode = 1; // Normal par défaut
    localVoiceData.voiceDistance = distanceNormal;
    remotePlayerCount = 0;
    lastVoiceDataSent = 0;
    lastKeyCheck = 0;

    // Charger les distances de voix depuis la configuration
    loadVoiceDistancesFromConfig();

    // NOUVEAU: Lecture et activation automatique de la configuration
    readConfigurationSettings(); // Déplacer avant les autres initialisations

    // NOUVEAU: Construire automatiquement le chemin modFilePath
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
                    // Nettoyer les retours à la ligne
                    wchar_t* nl = wcschr(pathStart, L'\n');
                    if (nl) *nl = L'\0';
                    wchar_t* cr = wcschr(pathStart, L'\r');
                    if (cr) *cr = L'\0';

                    // Convertir en chemin modFilePath
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

    // Read saved path and convert to modFilePath
    wchar_t savedPath[MAX_PATH] = L"";
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

    // NOUVEAU: Forcer l'initialisation complète
    forceCompleteInitialization();

    // Maintenant démarrer les threads
    _beginthread(modFileWatcherThread, 0, NULL);

    _beginthread(voiceSystemThread, 0, NULL);

    connectionAttempts = 0;

    return MUMBLE_STATUS_OK;
}

// NOUVEAU: Ajoutez cette fonction pour recevoir les données
PLUGIN_EXPORT bool PLUGIN_CALLING_CONVENTION mumble_onReceiveData(mumble_connection_t connection,
    mumble_userid_t sender,
    const uint8_t* data,
    size_t dataLength,
    const char* dataID) {
    // Vérifier si c'est notre type de données
    if (strcmp(dataID, "ConanExiles_VoiceData") == 0 && dataLength == sizeof(VoiceData)) {
        const VoiceData* receivedData = (const VoiceData*)data;
        processReceivedVoiceData(receivedData, sender);
        return true; // Données traitées
    }

    return false; // Pas nos données
}

uint32_t mumble_getFeatures() {
    return MUMBLE_FEATURE_POSITIONAL;
}

uint8_t mumble_initPositionalData(const char* const* programNames, const uint64_t* programPIDs, size_t programCount) {
    return MUMBLE_PDEC_OK;
}

bool mumble_fetchPositionalData(float* avatarPos, float* avatarDir, float* avatarAxis, float* cameraPos,
    float* cameraDir, float* cameraAxis, const char** context, const char** identity) {

    // Avatar positions (Y = height) | Positions de l'avatar (Y = hauteur)
    avatarPos[0] = axe_x / 100.0f;
    avatarPos[1] = axe_y / 100.0f;
    avatarPos[2] = axe_z / 100.0f;

    cameraPos[0] = avatarPos[0];
    cameraPos[1] = avatarPos[1];
    cameraPos[2] = avatarPos[2];


    avatarDir[0] = avatarAxisX;
    avatarDir[1] = avatarAxisY;
    avatarDir[2] = avatarAxisZ;

    cameraDir[0] = avatarDir[0];
    cameraDir[1] = avatarDir[1];
    cameraDir[2] = avatarDir[2];

    // Dynamic "up" axis calculation from yaw/pitch | Calcul dynamique de l'axe "up" depuis yaw/pitch
    const float deg2rad = 3.14159265f / 180.0f;
    float yawRad = currentModData.yaw * deg2rad;
    float pitchRad = -currentModData.yawY * deg2rad; // Same convention as avatarDir.Y | même convention que avatarDir.Y

    float sy = sinf(yawRad);
    float cy = cosf(yawRad);
    float sp = sinf(pitchRad);
    float cp = cosf(pitchRad);

    // Calculate up vector: up = up0*cp - forward_yaw*sp | Calcul du vecteur up : up = up0*cp - forward_yaw*sp
    avatarAxis[0] = -sy * sp; // X component | Composante X
    avatarAxis[1] = cp;       // Y component | Composante Y
    avatarAxis[2] = -cy * sp; // Z component | Composante Z

    // Safe vector normalization | Normalisation sécurisée du vecteur
    float uLen = sqrtf(avatarAxis[0] * avatarAxis[0] + avatarAxis[1] * avatarAxis[1] + avatarAxis[2] * avatarAxis[2]);
    if (uLen > 1e-6f) {
        avatarAxis[0] /= uLen; avatarAxis[1] /= uLen; avatarAxis[2] /= uLen;
    }

    cameraAxis[0] = avatarAxis[0];
    cameraAxis[1] = avatarAxis[1];
    cameraAxis[2] = avatarAxis[2];

    *context = useModFile ? "MOD_ACTIVE" : (enableMemoryOffsets ? "MEMORY_OFFSETS" : "NO_DATA");
    checkPlayerZone();
    manageChannelBasedOnCoordinates();
    *identity = "";
    return true;
}

// Fonction pour nettoyer les données de position / Function to clean up positional data
void mumble_shutdownPositionalData() {}

mumble_version_t mumble_getAPIVersion() {
    // This constant will always hold the API version that fits the included header files | Cette constante contiendra toujours la version de l'API qui correspond aux fichiers d'en-tête inclus
    return MUMBLE_PLUGIN_API_VERSION;
}

// Register Mumble API functions | Enregistrer les fonctions de l'API Mumble
void mumble_registerAPIFunctions(void* apiStruct) {
    // Provided mumble_getAPIVersion returns MUMBLE_PLUGIN_API_VERSION, this cast will make sure | Pourvu que mumble_getAPIVersion retourne MUMBLE_PLUGIN_API_VERSION, ce cast s'assurera
    // that the passed pointer will be cast to the proper type | que le pointeur passé sera casté vers le type approprié
    mumbleAPI = MUMBLE_API_CAST(apiStruct);
}

// Release Mumble resource | Libérer une ressource Mumble
void mumble_releaseResource(const void* pointer) {
    // As we never pass a resource to Mumble that needs releasing, this function should never | Comme nous ne passons jamais une ressource à Mumble qui nécessite une libération, cette fonction ne devrait jamais
    // get called | être appelée
    mumbleAPI.log(ownID, u8"Called mumble_releaseResource but expected that this never gets called -> Aborting");
    abort();
}

// Get plugin name | Obtenir le nom du plugin
struct MumbleStringWrapper mumble_getName() {
    static const char* name = u8"Conan_exiles"; // Plugin name | Nom du plugin

    struct MumbleStringWrapper wrapper = { 0 }; // Initialize wrapper structure | Initialiser la structure wrapper
    wrapper.data = name; // Set name data | Définir les données du nom
    wrapper.size = strlen(name); // Calculate name length | Calculer la longueur du nom
    wrapper.needsReleasing = false; // No memory release needed | Aucune libération mémoire nécessaire

    return wrapper;
}

// Get plugin version | Obtenir la version du plugin
mumble_version_t mumble_getVersion() {
    mumble_version_t version = { 0 }; // Initialize version structure | Initialiser la structure de version
    version.major = 4; // Major version number | Numéro de version majeure
    version.minor = 0; // Minor version number | Numéro de version mineure
    version.patch = 3; // Patch version number | Numéro de version de correctif

    return version;
}

// Get plugin author information | Obtenir les informations de l'auteur du plugin
struct MumbleStringWrapper mumble_getAuthor() {
    static const char* author = u8"Creator's Discord : Dino_Rex"; // Author information | Informations de l'auteur

    struct MumbleStringWrapper wrapper = { 0 }; // Initialize wrapper structure | Initialiser la structure wrapper
    wrapper.data = author; // Set author data | Définir les données de l'auteur
    wrapper.size = strlen(author); // Calculate author string length | Calculer la longueur de la chaîne auteur
    wrapper.needsReleasing = false; // No memory release needed | Aucune libération mémoire nécessaire

    return wrapper;
}

// Get plugin description | Obtenir la description du plugin
struct MumbleStringWrapper mumble_getDescription() {
    static const char* description = u8"Creator's Discord : Dino_Rex Discord: https://discord.gg/tFBbQzmDaZ"; // Plugin description | Description du plugin

    struct MumbleStringWrapper wrapper = { 0 }; // Initialize wrapper structure | Initialiser la structure wrapper
    wrapper.data = description; // Set description data | Définir les données de description
    wrapper.size = strlen(description); // Calculate description string length | Calculer la longueur de la chaîne de description
    wrapper.needsReleasing = false; // No memory release needed | Aucune libération mémoire nécessaire

    return wrapper;
}

// Return the plugin ID | Retourner l'ID du plugin
static mumble_plugin_id_t mumble_getPluginID() {
    return ownID; // Return stored plugin identifier | Retourner l'identifiant de plugin stocké
}

// Called when the client has finished synchronizing with the server
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerSynchronized(mumble_connection_t connection) {
    initializeChannelIDs();
}

PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerConnected(mumble_connection_t connection) {
    channelManagementActive = FALSE;
    hubChannelID = -1;
    ingameChannelID = -1;
    lastTargetChannel = -1;
    lastValidChannel = -1;

    // NOUVEAU: Réinitialiser le système de voix
    cleanupPlayerMuteStates();
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    remotePlayerCount = 0;
    lastDistanceCheck = 0;

    // NOUVEAU: Récupérer la distance maximale du nouveau serveur
    maxAudioDistanceRetrieved = FALSE; // Forcer une nouvelle récupération
    retrieveServerMaximumAudioDistance(FALSE);
    applyMaximumDistanceLimits();

    if (enableLogGeneral) {
        mumbleAPI.log(ownID, "Voice chat system initialized on connection with server distance limits");
    }
}

// Called whenever any user enters a channel
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onChannelEntered(mumble_connection_t connection,
    mumble_userid_t userID,
    mumble_channelid_t previousChannelID,
    mumble_channelid_t newChannelID) {
    if (!enableAutomaticChannelChange) return; // Check if automatic change is enabled | Vérifier si le changement automatique est activé
    if (!channelManagementActive) return;

    // Check if it's the local user | Vérifier si c'est l'utilisateur local
    mumble_userid_t localUserID;
    if (mumbleAPI.getLocalUserID(ownID, connection, &localUserID) != MUMBLE_STATUS_OK) {
        return;
    }

    if (userID == localUserID) {

        if (!coordinatesValid) {
            // If invalid coordinates and not in hub, send back to hub | Si coordonnées invalides et pas dans hub, renvoyer vers hub
            if (newChannelID != hubChannelID && hubChannelID != -1) {
                Sleep(50);
                mumbleAPI.requestUserMove(ownID, connection, localUserID, hubChannelID, NULL);
            }
        }

    }
}

// Nettoyage lors de la sortie d'un canal
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onChannelExited(mumble_connection_t connection,
    mumble_userid_t userID,
    mumble_channelid_t channelID) {

    // Si c'est un autre utilisateur qui quitte, nettoyer ses données
    if (userID != 0) { // 0 = utilisateur local
        for (size_t i = 0; i < playerMuteStateCount; i++) {
            if (playerMuteStates[i].userID == userID) {
                // Décaler les éléments pour combler le trou
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

        // Nettoyer aussi les données remote
        for (size_t i = 0; i < remotePlayerCount; i++) {
            // Note: On ne peut pas facilement associer userID avec playerName, 
            // donc on fait un nettoyage périodique séparé
        }
    }
}

void mumble_shutdown() {
    removeKeyMonitoring();

    // NOUVEAU: Nettoyer le système de voix
    remotePlayerCount = 0;
    memset(&localVoiceData, 0, sizeof(VoiceData));
    memset(remotePlayersData, 0, sizeof(remotePlayersData));
    cleanupPlayerMuteStates(); // Nettoyer les états de mute

    // Disable all functions
    enableSetMaximumAudioDistance = FALSE;
    enableCheckPlayerZone = FALSE;
    enableCheckVersionThread = FALSE;
    enableStartVersionCheck = FALSE;
    enableFindProcessId = FALSE;
    enableFindBaseAddress = FALSE;
    enableReadMemoryValue = FALSE;
    enableGetPlayerCoordinates = FALSE;
}
