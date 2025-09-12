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



#pragma comment(lib, "ws2_32.lib")

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

HWND hEdit; // Edit control handle | Handle du contrôle d'édition
HWND hStatus; // Status control handle | Handle du contrôle de statut
HFONT hFont; // Font handle | Handle de police

// Concise explanatory text | Texte explicatif concis
const wchar_t* infoText1 = L"Please provide the path to your Conan Exiles folder."; // Indicate Conan Exiles folder | Indiquez le dossier de Conan Exiles
const wchar_t* infoText2 = L"Example: C:\\Program Files (x86)\\Steam\\steamapps\\common\\Conan Exiles"; // Example path | Exemple de chemin
const wchar_t* infoText3 = L"The 'Saved' folder must exist inside 'ConanSandbox' for the plugin to work."; // Saved folder requirement | Exigence du dossier Saved

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
            snprintf(errorMessage, sizeof(errorMessage), "Analysis error for zone %zu | Erreur d'analyse pour la zone %zu", i + 1);
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

// Check if Saved folder exists in game folder | Vérifie que le dossier Saved existe dans le dossier du jeu
int savedExistsInFolder(const wchar_t* folderPath) {
    wchar_t savedPath[MAX_PATH];
    swprintf(savedPath, MAX_PATH, L"%s\\ConanSandbox\\Saved", folderPath);
    DWORD attribs = GetFileAttributesW(savedPath);
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

// Write full Saved path to config file | Écriture du chemin complet Saved dans le fichier de configuration
void writePatch(const wchar_t* folderPath) {
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) {
        SetWindowTextW(hStatus, L"Error: Could not access the Documents folder.");
        return;
    }

    // Create full configuration file path | Créer le chemin complet du fichier de configuration
    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);

    // Debug log | Log pour débugger
    if (enableLogConfig) {
        mumbleAPI.log(ownID, "Attempting to create plugin.cfg in Documents.");
    }

    FILE* file = _wfopen(configFile, L"w");
    if (!file) {
        char errorMsg[256];
        strerror_s(errorMsg, sizeof(errorMsg), errno);
        if (enableLogConfig) {
            mumbleAPI.log(ownID, errorMsg);
        }
        SetWindowTextW(hStatus, L"Error: Could not create the configuration file.");
        return;
    }

    wchar_t savedPath[MAX_PATH];
    swprintf(savedPath, MAX_PATH, L"%s\\ConanSandbox\\Saved", folderPath);
    fwprintf(file, L"SavedPath=%s\n", savedPath);
    fwprintf(file, L"EnableMemoryOffsets=false\n");
    fwprintf(file, L"EnableAutomaticChannelChange=false\n");
    fclose(file);

    SetWindowTextW(hStatus, L"Configuration saved in Documents!");
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
                    SetWindowTextW(hEdit, path);
                    CoTaskMemFree(path);
                }
                psi->lpVtbl->Release(psi);
            }
        }
        pfd->lpVtbl->Release(pfd);
    }
}

// Apply font to control | Application de la police à un contrôle
void ApplyFontToControl(HWND control) {
    SendMessageW(control, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// Main window procedure | Procédure de la fenêtre principale
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static UINT_PTR closeTimer = 0; // Close timer handle | Handle du minuteur de fermeture
    HWND control;

    switch (msg) {
    case WM_CREATE:
        // Create font for UI controls | Créer la police pour les contrôles UI
        hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        // Create information text controls | Créer les contrôles de texte d'information
        control = CreateWindowW(L"STATIC", infoText1, WS_VISIBLE | WS_CHILD,
            20, 15, 660, 25, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(control);

        control = CreateWindowW(L"STATIC", infoText2, WS_VISIBLE | WS_CHILD,
            20, 45, 660, 25, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(control);

        control = CreateWindowW(L"STATIC", infoText3, WS_VISIBLE | WS_CHILD,
            20, 75, 660, 25, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(control);

        control = CreateWindowW(L"STATIC", L"Select the Conan Exiles folder:", WS_VISIBLE | WS_CHILD,
            20, 110, 350, 25, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(control);

        // Create path input edit control | Créer le contrôle d'édition pour saisir le chemin
        hEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            20, 140, 500, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hEdit);

        // Create browse button | Créer le bouton parcourir
        control = CreateWindowW(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD,
            540, 140, 120, 30, hwnd, (HMENU)1, NULL, NULL);
        ApplyFontToControl(control);

        // Create validate button | Créer le bouton valider
        control = CreateWindowW(L"BUTTON", L"Validate", WS_VISIBLE | WS_CHILD,
            300, 190, 120, 35, hwnd, (HMENU)2, NULL, NULL);
        ApplyFontToControl(control);

        // Create status display control | Créer le contrôle d'affichage de statut
        hStatus = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            20, 240, 660, 30, hwnd, NULL, NULL, NULL);
        ApplyFontToControl(hStatus);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: // Browse button clicked | Bouton parcourir cliqué
            browseFolderModern(hwnd);
            break;
        case 2: { // Validate button clicked | Bouton valider cliqué
            wchar_t folderPath[MAX_PATH];
            GetWindowTextW(hEdit, folderPath, MAX_PATH);
            if (!savedExistsInFolder(folderPath)) {
                SetWindowTextW(hStatus, L"The 'Saved' folder does not exist in ConanSandbox. "
                    L"It must be present for the plugin to work.");
                break;
            }
            writePatch(folderPath);
            // Update mod file path based on selected folder | Mise à jour du chemin du fichier mod en fonction du dossier sélectionné
            snprintf(modFilePath, MAX_PATH, "%s\\ConanSandbox\\Saved\\Pos.txt", folderPath);
            SetWindowTextW(hStatus, L"Saved folder path has been configured!");
            closeTimer = SetTimer(hwnd, 1, 3000, NULL);
            break;
        }
        }
        break;

    case WM_TIMER:
        if (wParam == 1 && closeTimer) {
            KillTimer(hwnd, 1);
            closeTimer = 0;
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        if (hFont) DeleteObject(hFont); // Clean up font resource | Nettoyer la ressource de police
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

// Show path selection dialog | Afficher la boîte de dialogue de sélection de chemin
void showPathSelectionDialog() {
    if (isPatchAlreadySaved()) {
        return;
    }
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    const wchar_t CLASS_NAME[] = L"PatchWindowClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Path Configuration",
        WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 320,
        NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) return;

    ShowWindow(hwnd, SW_SHOW);
    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return;
}

// Path selection dialog thread function | Fonction de thread pour la boîte de dialogue de sélection de chemin
void showPathSelectionDialogThread(void* arg) {
    showPathSelectionDialog();
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
    static bool versionChecked = false;  // Variable to track if version was checked | Variable pour suivre si la version a été vérifiée
    static bool versionIncompatibleLogged = false; // Variable to track if message was displayed | Variable pour suivre si le message a été affiché

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

    // Valeurs par défaut (si manquantes)
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
                // Trim val
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
            }
            fclose(f);
        }
    }

    // If file doesn't exist or missing key => rewrite | Si le fichier n'existe pas ou qu'il manque une clé => réécriture
    if (!fileExists || !foundSavedPath || !foundEnableMemoryOffsets || !foundEnableAutomaticChannelChange) {
        FILE* f = _wfopen(configFile, L"w");
        if (f) {
            fwprintf(f, L"SavedPath=%s\n", foundSavedPath ? savedPathValue : L"");
            fwprintf(f, L"EnableMemoryOffsets=%s\n", enableMemoryOffsets ? L"true" : L"false");
            fwprintf(f, L"EnableAutomaticChannelChange=%s\n",
                enableAutomaticChannelChange ? L"true" : L"false");
            fclose(f);
        }
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

    if (!isPatchAlreadySaved()) {
        _beginthread(showPathSelectionDialogThread, 0, NULL);
    }

    // Read saved path and convert to modFilePath
    wchar_t savedPath[MAX_PATH] = L"";
    wchar_t* configFolder = getConfigFolderPath();
    if (!configFolder) {
        return MUMBLE_STATUS_OK;
    }

    wchar_t configFile[MAX_PATH];
    swprintf(configFile, MAX_PATH, L"%s\\plugin.cfg", configFolder);
    FILE* file = _wfopen(configFile, L"r");
    if (file) {
        wchar_t line[512];
        while (fgetws(line, 512, file)) {
            if (wcsncmp(line, L"SavedPath=", 10) == 0) {
                wcscpy_s(savedPath, MAX_PATH, line + 10); // Get path after "SavedPath="
                break;
            }
        }
        fclose(file);
        // New line and return character cleanup
        wchar_t* nl = wcschr(savedPath, L'\n');
        if (nl) *nl = L'\0';
        wchar_t* cr = wcschr(savedPath, L'\r');
        if (cr) *cr = L'\0';
        // wchar_t to char conversion for modFilePath
        size_t converted = 0;
        wcstombs_s(&converted, modFilePath, MAX_PATH, savedPath, _TRUNCATE);
        // Add Pos.txt to path | Ajout de Pos.txt au chemin
        snprintf(modFilePath, MAX_PATH, "%s\\Pos.txt", modFilePath);
    }

    // Read configuration settings for offsets
    readConfigurationSettings();

    // Debug log | Log pour débugger
    if (enableLogConfig) {
        char debugMsg[512];
        snprintf(debugMsg, sizeof(debugMsg), "Chemin configuré pour Pos.txt: %s", modFilePath);
        mumbleAPI.log(ownID, debugMsg);
    }

    // Start new single mod file watcher and reader thread
    _beginthread(modFileWatcherThread, 0, NULL);

    connectionAttempts = 0;

    return MUMBLE_STATUS_OK;
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
    version.major = 3; // Major version number | Numéro de version majeure
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

// Called when connecting to a server
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerConnected(mumble_connection_t connection) {
    channelManagementActive = FALSE;
    hubChannelID = -1;
    ingameChannelID = -1;
    lastTargetChannel = -1;
    lastValidChannel = -1;
}

// Called when disconnecting from a server
PLUGIN_EXPORT void PLUGIN_CALLING_CONVENTION mumble_onServerDisconnected(mumble_connection_t connection) {
    channelManagementActive = FALSE;
    lastValidChannel = -1;
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

// Plugin cleanup function | Fonction de nettoyage du plugin
void mumble_shutdown() {
    // Disable all functions | Désactiver toutes les fonctions
    enableSetMaximumAudioDistance = FALSE; // Disable audio distance setting | Désactiver le réglage de distance audio
    enableCheckPlayerZone = FALSE; // Disable player zone checking | Désactiver la vérification de zone joueur
    enableCheckVersionThread = FALSE; // Disable version checking thread | Désactiver le thread de vérification de version
    enableStartVersionCheck = FALSE; // Disable version check start | Désactiver le démarrage de vérification de version
    enableFindProcessId = FALSE; // Disable process ID finding | Désactiver la recherche d'ID de processus
    enableFindBaseAddress = FALSE; // Disable base address finding | Désactiver la recherche d'adresse de base
    enableReadMemoryValue = FALSE; // Disable memory reading | Désactiver la lecture mémoire
    enableGetPlayerCoordinates = FALSE; // Disable player coordinates retrieval | Désactiver la récupération des coordonnées du joueur
}
