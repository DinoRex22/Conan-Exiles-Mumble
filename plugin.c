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

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "74.210.183.203" // Adresse IP de votre serveur
#define SERVER_PORT 5924 // Port que vous avez ouvert
#define REQUIRED_VERSION "VERSION: 2.0.0" // Version requise pour le plugin

static volatile BOOL versionReceived = FALSE;
static volatile BOOL zonesReceived = FALSE;

// Variables pour contr�ler l'activation des fonctions
static BOOL enableSetMaximumAudioDistance = FALSE;
static BOOL enableCheckPlayerZone = FALSE;
static BOOL enableCheckVersionThread = FALSE;
static BOOL enableStartVersionCheck = FALSE;
static BOOL enableFindProcessId = FALSE;
static BOOL enableFindBaseAddress = FALSE;
static BOOL enableReadMemoryValue = FALSE;
static BOOL enableGetPlayerCoordinates = FALSE;

struct MumbleAPI_v_1_0_x mumbleAPI;
mumble_plugin_id_t ownID;

float axe_x = 0.0f;
float axe_y = 0.0f;
float axe_z = 0.0f;

volatile BOOL isConnected = FALSE; // Indicateur de connexion

static void displayInChat(const char* message) {
    mumbleAPI.log(ownID, message);
}

// Ajout de variables pour suivre les offsets utilis�s
bool usedPrimaryX = true;
bool usedPrimaryY = true;
bool usedPrimaryZ = true;

static BOOL enableBackupOffsetX = FALSE;
static BOOL enableBackupOffsetY = FALSE;
static BOOL enableBackupOffsetZ = FALSE;

struct Zone {
    float x1;
    float y1;
    float x2;
    float y2;
    double maxDistance;
};

struct Zone* zones = NULL;
size_t zoneCount = 0;

static void parseZones(const char* response) {
    // Cherche le d�but de la section "ZONES:" dans la r�ponse
    const char* zonesData = strstr(response, "ZONES: ");
    if (zonesData == NULL) {
        displayInChat("Erreur: donn�es de zones non trouv�es dans la r�ponse");
        return;
    }

    // Avancer le pointeur au-del� de "ZONES: "
    zonesData += 7;

    // Compte le nombre de zones (assumant qu'elles sont s�par�es par des virgules)
    zoneCount = 1;
    const char* p = zonesData;
    while (*p) {
        if (*p == ';') { // Changez en point-virgule si les zones sont s�par�es par des points-virgules
            zoneCount++;
        }
        p++;
    }

    // Alloue la m�moire pour les zones
    zones = (struct Zone*)malloc(zoneCount * sizeof(struct Zone));
    if (zones == NULL) {
        displayInChat("Erreur d'allocation de m�moire pour les zones");
        return;
    }

    // Remplit les informations de chaque zone
    p = zonesData;
    for (size_t i = 0; i < zoneCount; i++) {
        int result = sscanf_s(p, "%f,%f,%f,%f,%lf", &zones[i].x1, &zones[i].y1, &zones[i].x2, &zones[i].y2, &zones[i].maxDistance);
        if (result != 5) {
            char errorMessage[256];
            snprintf(errorMessage, sizeof(errorMessage), "Erreur d'analyse pour la zone %zu", i + 1);
            displayInChat(errorMessage);
            free(zones);
            zones = NULL;
            zoneCount = 0;
            return;
        }

        // Passe � la prochaine zone (en utilisant le d�limiteur `;` ou `,`)
        while (*p && *p != ';') {
            p++;
        }
        if (*p == ';') {
            p++;  // Ignore le point-virgule
        }
    }
}

static void setMaximumAudioDistance(double newMaxDistance) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_MAXIMUM_DISTANCE, newMaxDistance);
    if (result != MUMBLE_STATUS_OK) {}
}

static void setMinimumAudioDistance(double newMinDistance) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_MINIMUM_DISTANCE, newMinDistance);
    if (result != MUMBLE_STATUS_OK) {}
}

static void setAudioBloom(double bloomValue) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_BLOOM, bloomValue);
    if (result != MUMBLE_STATUS_OK) {}
}

static void setMinimumAudioVolume(double minVolume) {
    if (!enableSetMaximumAudioDistance) return;

    mumble_error_t result = mumbleAPI.setMumbleSetting_double(ownID, MUMBLE_SK_AUDIO_OUTPUT_PA_MINIMUM_VOLUME, minVolume);
    if (result != MUMBLE_STATUS_OK) {}
}

const int numZones = sizeof(zones) / sizeof(zones[0]);

static void checkPlayerZone() {
    if (!enableCheckPlayerZone) return;

    bool inZone = false;
    for (size_t i = 0; i < zoneCount; ++i) {
        if (axe_x / 100.0f >= zones[i].x1 && axe_x / 100.0f <= zones[i].x2 &&
            axe_y / 100.0f >= zones[i].y1 && axe_y / 100.0f <= zones[i].y2) {
            //setMinimumAudioDistance(2.0);
            //setMaximumAudioDistance(10.0);
            //setMinimumAudioVolume(0.0);
            //setAudioBloom(0.75);
            inZone = true;
            break;
        }
    }

    if (!inZone) {
        //setMinimumAudioDistance(2.0);
        //setMaximumAudioDistance(10.0);
        //setMinimumAudioVolume(0.0);
        //setAudioBloom(0.75);
    }
}

static volatile int connectionAttempts = 0;

static volatile BOOL fusionRequestSent = FALSE; // Nouveau flag pour v�rifier si la requ�te FUSION a �t� envoy�e

static void connectToServer(void* param) {
    if (connectionAttempts >= 2) return;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        displayInChat("WSAStartup failed");
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in serverAddr;
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
        displayInChat("Erreur d'allocation de m�moire");
        closesocket(sock);
        WSACleanup();
        return;
    }

    int bytesRead = recv(sock, buffer, 16383, 0);
    if(bytesRead > 0) {
        buffer[bytesRead] = '\0';

        // Variable pour le contexte de strtok_s
        char* context = NULL;

        // D�coupe la r�ponse par ligne en utilisant \n comme s�parateur
        char* line = strtok_s(buffer, "\n", &context);
        while (line != NULL) {
            // V�rifie si la ligne contient "VERSION:"
            if (strncmp(line, "VERSION:", 8) == 0) {
                isConnected = (strcmp(line, REQUIRED_VERSION) == 0);
                mumbleAPI.log(ownID, isConnected ? "The version is compatible." : "The version is not compatible.");
                versionReceived = TRUE; // Marquer la version comme re�ue
            }
            // V�rifie si la ligne contient "ZONES:"
            else if (strncmp(line, "ZONES:", 6) == 0) {
                parseZones(line); // Appelle parseZones pour analyser les zones
                zonesReceived = TRUE; // Marquer les zones comme re�ues
            }
            // Passe � la ligne suivante
            line = strtok_s(NULL, "\n", &context);
        }
    }
    else {
        displayInChat("Erreur lors de la lecture de la r�ponse du serveur");
    }

    // Si version et zones re�us, fermer la connexion
    if (versionReceived && zonesReceived) {
        closesocket(sock); // Ferme la connexion
        WSACleanup();      // Lib�re les ressources de la connexion
    }

    free(buffer);
}

static void startVersionCheck() {
    if (!enableStartVersionCheck) return;
    _beginthread(connectToServer, 0, NULL);
}

// Fonction pour obtenir l'ID du processus ConanSandbox.exe / Function to get the process ID of ConanSandbox.exe
static BOOL findProcessId(const TCHAR* processName, DWORD* processID) {
    if (!enableFindProcessId) return FALSE;  // V�rification si activ�e

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

// Fonction pour obtenir l'adresse de base de ConanSandbox.exe / Function to get the base address of ConanSandbox.exe
static BOOL findBaseAddress(DWORD processID, LPVOID* baseAddress) {
    if (!enableFindBaseAddress) return FALSE;  // V�rification si activ�e

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

// Fonction pour lire un float � une adresse m�moire donn�e / Function to read a float at a given memory address
static BOOL readMemoryValue(HANDLE hProcess, LPVOID address, float* value) {
    if (!enableReadMemoryValue) return FALSE;  // V�rification si activ�e

    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, address, value, sizeof(float), &bytesRead) && bytesRead == sizeof(float)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL readCoordinates(HANDLE hProcess, DWORD_PTR baseAddress, DWORD_PTR* offsets, SIZE_T offsetCount, float* value) {
    DWORD_PTR currentAddress = baseAddress;
    for (SIZE_T i = 0; i < offsetCount; ++i) {
        if (!ReadProcessMemory(hProcess, (LPCVOID)currentAddress, &currentAddress, sizeof(currentAddress), NULL)) {
            return FALSE; // Retourne FALSE si une erreur est rencontr�e
        }
        currentAddress += offsets[i];
    }
    return readMemoryValue(hProcess, (LPVOID)currentAddress, value);
}

// Fonction pour obtenir les coordonn�es du joueur / Function to get the player's coordinates
static BOOL getPlayerCoordinates() {
    if (!enableGetPlayerCoordinates) return FALSE;  // V�rification si activ�e

    // Avant de lire les coordonn�es, v�rifiez la version
    static bool versionChecked = false;  // Variable pour suivre si la version a �t� v�rifi�e
    static bool versionIncompatibleLogged = false; // Variable pour suivre si le message a �t� affich�

    if (!versionChecked) {
        // V�rifie la connexion avant d'obtenir les coordonn�es
        if (!isConnected) {
            startVersionCheck(); // D�marre le thread de v�rification de version
            return FALSE; // Ne pas proc�der si pas connect�
        }
        versionIncompatibleLogged = false; // R�initialiser si la version est correcte
        versionChecked = true; // Marquer que la version a �t� v�rifi�e
    }

    static bool processIdNotFound = false;
    static bool successMessageLogged = false; // D�claration ici pour le suivi des succ�s
    DWORD processID = 0;
    LPVOID baseAddress = NULL;
    HANDLE hProcess = NULL;

    DWORD_PTR baseAddressOffsetX_Principal = 0x05D4E600;
    DWORD_PTR baseAddressOffsetY_Principal = 0x05D4E600;
    DWORD_PTR baseAddressOffsetZ_Principal = 0x05D4E600;

    DWORD_PTR baseAddressOffsetX_Backup = 0x05884C30;
    DWORD_PTR baseAddressOffsetY_Backup = 0x05884C30;
    DWORD_PTR baseAddressOffsetZ_Backup = 0x05884C30;

    // Offsets principaux
    DWORD_PTR offsetX[] = { 0x30, 0x320, 0x70, 0x20, 0x460, 0x170, 0x1B0 };
    DWORD_PTR offsetY[] = { 0x30, 0x320, 0x70, 0x20, 0x460, 0x170, 0x1B4 };
    DWORD_PTR offsetZ[] = { 0x30, 0x320, 0x70, 0x20, 0x460, 0x170, 0x1B8 };

    // Offsets de secours
    DWORD_PTR backupOffsetX[] = { 0x0, 0x48, 0x8, 0x510, 0x8, 0x948, 0x1B0 };
    DWORD_PTR backupOffsetY[] = { 0x0, 0x448, 0x100, 0x30, 0xF8, 0xF8, 0x1B4 };
    DWORD_PTR backupOffsetZ[] = { 0x0, 0x370, 0x80, 0x80, 0x20, 0x170, 0x1B8 };

    TCHAR targetProcess[] = TEXT("ConanSandbox.exe");

    if (!findProcessId(targetProcess, &processID)) {
        if (!processIdNotFound) {
            //mumbleAPI.log(ownID, u8"Erreur : Impossible de trouver ConanSandbox.exe (2).");
            processIdNotFound = true;
        }
        return FALSE;
    }

    processIdNotFound = false;
    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (hProcess == NULL) {
        //mumbleAPI.log(ownID, u8"Erreur : Impossible d'ouvrir ConanSandbox.exe.");
        return FALSE;
    }

    if (!findBaseAddress(processID, &baseAddress)) {
        //mumbleAPI.log(ownID, u8"Erreur : Impossible de trouver ConanSandbox.exe 1.");
        CloseHandle(hProcess);
        return FALSE;
    }

    BOOL errorX = FALSE, errorY = FALSE, errorZ = FALSE;
    static bool errorLoggedX = false, errorLoggedY = false, errorLoggedZ = false;

    // Lire l'axe X avec les offsets s�par�s
    DWORD_PTR currentAddressX = (DWORD_PTR)baseAddress + baseAddressOffsetX_Principal;
    if (!readCoordinates(hProcess, currentAddressX, offsetX, sizeof(offsetX) / sizeof(offsetX[0]), &axe_x)) {
        usedPrimaryX = false; // Indiquer que l'offset principal a �chou�
        if (enableBackupOffsetX) { // V�rifier si les offsets de secours sont activ�s
            currentAddressX = (DWORD_PTR)baseAddress + baseAddressOffsetX_Backup;
            if (!readCoordinates(hProcess, currentAddressX, backupOffsetX, sizeof(backupOffsetX) / sizeof(backupOffsetX[0]), &axe_x)) {
                errorX = TRUE; // Enregistrer l'erreur
            }
        }
    }

    // Lire l'axe Y avec les offsets s�par�s
    DWORD_PTR currentAddressY = (DWORD_PTR)baseAddress + baseAddressOffsetY_Principal;
    if (!readCoordinates(hProcess, currentAddressY, offsetY, sizeof(offsetY) / sizeof(offsetY[0]), &axe_y)) {
        usedPrimaryY = false; // Indiquer que l'offset principal a �chou�
        if (enableBackupOffsetY) { // V�rifier si les offsets de secours sont activ�s
            currentAddressY = (DWORD_PTR)baseAddress + baseAddressOffsetY_Backup;
            if (!readCoordinates(hProcess, currentAddressY, backupOffsetY, sizeof(backupOffsetY) / sizeof(backupOffsetY[0]), &axe_y)) {
                errorY = TRUE; // Enregistrer l'erreur
            }
        }
    }

    // Lire l'axe Z avec les offsets s�par�s
    DWORD_PTR currentAddressZ = (DWORD_PTR)baseAddress + baseAddressOffsetZ_Principal;
    if (!readCoordinates(hProcess, currentAddressZ, offsetZ, sizeof(offsetZ) / sizeof(offsetZ[0]), &axe_z)) {
        usedPrimaryZ = false; // Indiquer que l'offset principal a �chou�
        if (enableBackupOffsetZ) { // V�rifier si les offsets de secours sont activ�s
            currentAddressZ = (DWORD_PTR)baseAddress + baseAddressOffsetZ_Backup;
            if (!readCoordinates(hProcess, currentAddressZ, backupOffsetZ, sizeof(backupOffsetZ) / sizeof(backupOffsetZ[0]), &axe_z)) {
                errorZ = TRUE; // Enregistrer l'erreur
            }
        }
    }

    // Log message de r�ussite une seule fois
    if (!successMessageLogged && !(errorX || errorY || errorZ)) {
        // Cr�er le message de succ�s en fonction des offsets utilis�s
        char successMsg[256];
        sprintf_s(successMsg, sizeof(successMsg), u8"Succ�s : Coordonn�es trouv�es : X %s, Y %s, Z %s.",
            usedPrimaryX ? "principal" : "secondaire",
            usedPrimaryY ? "principal" : "secondaire",
            usedPrimaryZ ? "principal" : "secondaire");
        //mumbleAPI.log(ownID, successMsg);
        successMessageLogged = true; // Marque que le message a �t� logu�
    }
    else if (errorX || errorY || errorZ) {
        // R�initialiser successMessageLogged si une erreur se produit
        successMessageLogged = false;
    }

    // M�canisme de base pour les axes principaux
    static bool errorLogged = false;
    static time_t lastErrorTime = 0; // Pour garder la trace du temps du dernier message d'erreur
    if ((errorX || errorY || errorZ) && (!errorLogged || difftime(time(NULL), lastErrorTime) >= 60)) {
        char errorMsg[256];
        strcpy_s(errorMsg, sizeof(errorMsg), u8"Erreur : Impossible de r�cup�rer la position des axes : ");
        if (errorX) strcat_s(errorMsg, sizeof(errorMsg), "X ");
        if (errorY) strcat_s(errorMsg, sizeof(errorMsg), "Y ");
        if (errorZ) strcat_s(errorMsg, sizeof(errorMsg), "Z ");

        // Ajoutez le message de nouvelle tentative ici
        strcat_s(errorMsg, sizeof(errorMsg), u8"nouvelle tentative en cours.");

        mumbleAPI.log(ownID, errorMsg);
        errorLogged = true;
        lastErrorTime = time(NULL); // Met � jour le temps du dernier message d'erreur
    }
    else if (!errorX && !errorY && !errorZ) {
        errorLogged = false; // R�initialiser si toutes les lectures r�ussissent
    }

    // Regroupement des messages d'erreur des axes de secours
    static bool errorLoggedBackup = false;
    static time_t lastErrorTimeBackup = 0; // Pour garder la trace du temps du dernier message d'erreur
    if ((errorX || errorY || errorZ) && (!errorLoggedBackup || difftime(time(NULL), lastErrorTimeBackup) >= 60)) {
        char errorMsg[256];
        strcpy_s(errorMsg, sizeof(errorMsg), u8"Erreur : Impossible de lire les axes de secours : ");
        if (errorX) strcat_s(errorMsg, sizeof(errorMsg), "X ");
        if (errorY) strcat_s(errorMsg, sizeof(errorMsg), "Y ");
        if (errorZ) strcat_s(errorMsg, sizeof(errorMsg), "Z ");

        // Ajoutez le message de nouvelle tentative ici
        strcat_s(errorMsg, sizeof(errorMsg), u8"nouvelle tentative en cours.");

        //mumbleAPI.log(ownID, errorMsg);
        errorLoggedBackup = true;
        lastErrorTimeBackup = time(NULL); // Met � jour le temps du dernier message d'erreur
    }
    else if (!errorX && !errorY && !errorZ) {
        errorLoggedBackup = false; // R�initialiser si toutes les lectures r�ussissent
    }

    CloseHandle(hProcess);
    return !(errorX || errorY || errorZ);
}

static void continuousGetPlayerCoordinates(void* arg) {
    while (enableGetPlayerCoordinates) {
        getPlayerCoordinates();  // Appelle la fonction pour obtenir les coordonn�es du joueur
        Sleep(100); // Attendre 100 millisecondes avant la prochaine it�ration pour �viter une surcharge
    }
}

// Fonction d'initialisation du plugin / Plugin initialization function
mumble_error_t mumble_init(mumble_plugin_id_t pluginID) {
    ownID = pluginID;
    //mumbleAPI.log(ownID, u8"Plugin charg�.");

    // Activer toutes les fonctions
    enableSetMaximumAudioDistance = TRUE;
    enableCheckPlayerZone = TRUE;
    enableCheckVersionThread = TRUE;
    enableStartVersionCheck = TRUE;
    enableFindProcessId = TRUE;
    enableFindBaseAddress = TRUE;
    enableReadMemoryValue = TRUE;
    enableGetPlayerCoordinates = TRUE;

    // D�marrer le thread pour obtenir les coordonn�es du joueur
    _beginthread(continuousGetPlayerCoordinates, 0, NULL);

    connectionAttempts = 0; // R�initialiser le compteur de tentatives de connexion

    return MUMBLE_STATUS_OK;
}

uint32_t mumble_getFeatures() {
    return MUMBLE_FEATURE_POSITIONAL;
}

uint8_t mumble_initPositionalData(const char* const* programNames, const uint64_t* programPIDs, size_t programCount) {
    return MUMBLE_PDEC_OK;
}

// Fonction pour remplir les donn�es de position pour Mumble
bool mumble_fetchPositionalData(float* avatarPos, float* avatarDir, float* avatarAxis, float* cameraPos,
    float* cameraDir, float* cameraAxis, const char** context, const char** identity) {

    // Conversion des coordonn�es de centim�tres en m�tres
    avatarPos[0] = axe_x / 100.0f; // Conversion de X en m�tres
    avatarPos[1] = axe_y / 100.0f; // Conversion de Y en m�tres
    avatarPos[2] = axe_z / 10.0f; // Conversion de Z en m�tres

    // Copier la position de l'avatar dans cameraPos (sans offset)
    cameraPos[0] = avatarPos[0]; //X
    cameraPos[1] = avatarPos[1]; // Y
    cameraPos[2] = avatarPos[2]; // Z

    /* // Conversion des coordonn�es de centim�tres en m�tres
    avatarPos[0] = floor(axe_x / 100.0f); // Conversion de X en m�tres, sans d�cimales
    avatarPos[1] = floor(axe_y / 100.0f); // Conversion de Y en m�tres, sans d�cimales
    avatarPos[2] = floor(axe_z / 10.0f);  // Conversion de Z en m�tres, sans d�cimales*/

    // V�rification de la zone pour ajuster la distance audio
    checkPlayerZone();

    // Remplir les autres champs avec des z�ros
    memset(avatarDir, 0, 3 * sizeof(float));
    memset(avatarAxis, 0, 3 * sizeof(float));
    memset(cameraDir, 0, 3 * sizeof(float));
    memset(cameraAxis, 0, 3 * sizeof(float));
    *context = "";
    *identity = "";

    // mumbleAPI.log(ownID, u8"Position mise � jour.");
    return true;
}

// Fonction pour nettoyer les donn�es de position / Function to clean up positional data
void mumble_shutdownPositionalData() {}

mumble_version_t mumble_getAPIVersion() {
    // This constant will always hold the API version  that fits the included header files
    return MUMBLE_PLUGIN_API_VERSION;
}

void mumble_registerAPIFunctions(void* apiStruct) {
    // Provided mumble_getAPIVersion returns MUMBLE_PLUGIN_API_VERSION, this cast will make sure
    // that the passed pointer will be cast to the proper type
    mumbleAPI = MUMBLE_API_CAST(apiStruct);
}

void mumble_releaseResource(const void* pointer) {
    // As we never pass a resource to Mumble that needs releasing, this function should never
    // get called
    mumbleAPI.log(ownID, u8"Called mumble_releaseResource but expected that this never gets called -> Aborting");
    abort();
}

struct MumbleStringWrapper mumble_getName() {
    static const char* name = u8"Conan_exiles";

    struct MumbleStringWrapper wrapper = { 0 };
    wrapper.data = name;
    wrapper.size = strlen(name);
    wrapper.needsReleasing = false;

    return wrapper;
}

mumble_version_t mumble_getVersion() {
    mumble_version_t version = { 0 };
    version.major = 1;
    version.minor = 0;
    version.patch = 0;

    return version;
}

struct MumbleStringWrapper mumble_getAuthor() {
    static const char* author = u8"Creator's Discord : Dino_Rex";

    struct MumbleStringWrapper wrapper = { 0 };
    wrapper.data = author;
    wrapper.size = strlen(author);
    wrapper.needsReleasing = false;

    return wrapper;
}

struct MumbleStringWrapper mumble_getDescription() {
    static const char* description = u8"Creator's Discord : Dino_Rex Discord: https://discord.gg/tFBbQzmDaZ";

    struct MumbleStringWrapper wrapper = { 0 };
    wrapper.data = description;
    wrapper.size = strlen(description);
    wrapper.needsReleasing = false;

    return wrapper;
}

// Retourner l'ID du plugin / Return the plugin ID
static mumble_plugin_id_t mumble_getPluginID() {
    return ownID;
}

// Fonction de nettoyage du plugin / Start displaying coordinates
void mumble_shutdown() {
    // D�sactiver toutes les fonctions
    enableSetMaximumAudioDistance = FALSE;
    enableCheckPlayerZone = FALSE;
    enableCheckVersionThread = FALSE;
    enableStartVersionCheck = FALSE;
    enableFindProcessId = FALSE;
    enableFindBaseAddress = FALSE;
    enableReadMemoryValue = FALSE;
    enableGetPlayerCoordinates = FALSE;
}