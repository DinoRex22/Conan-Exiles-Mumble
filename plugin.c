// Copyright The Mumble Developers. All rights reserved. 
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at https://www.mumble.info/LICENSE.

// The following conditions apply exclusively to the code authored by Dino_Rex and do not affect or modify the copyright or licensing terms of the original Mumble code.

// Copyright (c) Dino_Rex
// This code is made available under the following conditions:
// 1. Plugin creators must credit the author (Dino_Rex) and include a link to the Discord https://discord.gg/tFBbQzmDaZ in both the source code and the description of the compiled Mumble plugin.
// 2. All usage of this code must remain open source. A link to the open-source project must be shared on the Discord https://discord.gg/tFBbQzmDaZ in the "open-source" channel.

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

#define SERVER_IP "Private" // IP address of your server
#define SERVER_PORT Private // Port you have opened
#define REQUIRED_VERSION "VERSION: 2.0.0" // Required version for the plugin

static volatile BOOL versionReceived = FALSE;
static volatile BOOL zonesReceived = FALSE;

// Variables to control the activation of features
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

volatile BOOL isConnected = FALSE; // Connection indicator

static void displayInChat(const char* message) {
    mumbleAPI.log(ownID, message);
}

// Adding variables to track the offsets used
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
    // Searches for the beginning of the "ZONES:" section in the response
    const char* zonesData = strstr(response, "ZONES: ");
    if (zonesData == NULL) {
        displayInChat("Error: zone data not found in the response.");
        return;
    }

    // Move the pointer beyond "ZONES: "
    zonesData += 7;

    // Counts the number of zones (assuming they are separated by commas)
    zoneCount = 1;
    const char* p = zonesData;
    while (*p) {
        if (*p == ';') { // Change to semicolon if the zones are separated by semicolons
            zoneCount++;
        }
        p++;
    }

    // Allocates memory for the zones
    zones = (struct Zone*)malloc(zoneCount * sizeof(struct Zone));
    if (zones == NULL) {
        displayInChat("Memory allocation error for the zones.");
        return;
    }

    // Fills in the information for each zone
    p = zonesData;
    for (size_t i = 0; i < zoneCount; i++) {
        int result = sscanf_s(p, "%f,%f,%f,%f,%lf", &zones[i].x1, &zones[i].y1, &zones[i].x2, &zones[i].y2, &zones[i].maxDistance);
        if (result != 5) {
            char errorMessage[256];
            snprintf(errorMessage, sizeof(errorMessage), "Parsing error for the zone. %zu", i + 1);
            displayInChat(errorMessage);
            free(zones);
            zones = NULL;
            zoneCount = 0;
            return;
        }

       // Move to the next zone (using the delimiter `;` or `,`)
        while (*p && *p != ';') {
            p++;
        }
        if (*p == ';') {
            p++;  // Ignore the semicolon
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

static volatile BOOL fusionRequestSent = FALSE; // New flag to check if the FUSION request has been sent

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
        displayInChat("Memory allocation error.");
        closesocket(sock);
        WSACleanup();
        return;
    }

    int bytesRead = recv(sock, buffer, 16383, 0);
    if(bytesRead > 0) {
        buffer[bytesRead] = '\0';

     // Variable for the strtok_s context.
        char* context = NULL;

    // Splits the response by line using \n as the separator.
        char* line = strtok_s(buffer, "\n", &context);
        while (line != NULL) {
            // Checks if the line contains "VERSION:"
            if (strncmp(line, "VERSION:", 8) == 0) {
                isConnected = (strcmp(line, REQUIRED_VERSION) == 0);
                mumbleAPI.log(ownID, isConnected ? "The version is compatible." : "The version is not compatible.");
                versionReceived = TRUE; // Mark the version as received
            }
            // Checks if the line contains "ZONES:"
            else if (strncmp(line, "ZONES:", 6) == 0) {
                parseZones(line); // Calls parseZones to analyze the zones
                zonesReceived = TRUE; // Mark the zones as received
            }
            // Move to the next line
            line = strtok_s(NULL, "\n", &context);
        }
    }
    else {
        displayInChat("Error while reading the server's response.");
    }

   // If version and zones are received, close the connection
    if (versionReceived && zonesReceived) {
        closesocket(sock); // Close the connection
        WSACleanup();     // Free the connection resources
    }

    free(buffer);
}

static void startVersionCheck() {
    if (!enableStartVersionCheck) return;
    _beginthread(connectToServer, 0, NULL);
}

// Function to get the process ID of ConanSandbox.exe
static BOOL findProcessId(const TCHAR* processName, DWORD* processID) {
    if (!enableFindProcessId) return FALSE;  // Check if enabled

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

// Function to get the base address of ConanSandbox.exe
static BOOL findBaseAddress(DWORD processID, LPVOID* baseAddress) {
    if (!enableFindBaseAddress) return FALSE;  // Check if enabled

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

// Function to read a float at a given memory address
static BOOL readMemoryValue(HANDLE hProcess, LPVOID address, float* value) {
    if (!enableReadMemoryValue) return FALSE;  // Check if activated

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
            return FALSE; // Returns FALSE if an error is encountered
        }
        currentAddress += offsets[i];
    }
    return readMemoryValue(hProcess, (LPVOID)currentAddress, value);
}

// Function to get the player's coordinates
static BOOL getPlayerCoordinates() {
    if (!enableGetPlayerCoordinates) return FALSE;  // Check if enabled

    // Before reading the coordinates, check the version.
    static bool versionChecked = false;  // Variable to track if the version has been checked.
    static bool versionIncompatibleLogged = false; // Variable to track if the message has been displayed.

    if (!versionChecked) {
        // Check the connection before retrieving the coordinates.
        if (!isConnected) {
            startVersionCheck(); // Start the version check thread.
            return FALSE; // Do not proceed if not connected.
        }
        versionIncompatibleLogged = false; // Reset if the version is correct.
        versionChecked = true; // Mark that the version has been checked.
    }

    static bool processIdNotFound = false;
    static bool successMessageLogged = false; // Declaration here for tracking successes.
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
            //mumbleAPI.log(ownID, u8"Error: Unable to find ConanSandbox.exe (2).");
            processIdNotFound = true;
        }
        return FALSE;
    }

    processIdNotFound = false;
    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (hProcess == NULL) {
        //mumbleAPI.log(ownID, u8"Error: Unable to open ConanSandbox.exe.");
        return FALSE;
    }

    if (!findBaseAddress(processID, &baseAddress)) {
        //mumbleAPI.log(ownID, u8"Error: Unable to find ConanSandbox.exe 1.");
        CloseHandle(hProcess);
        return FALSE;
    }

    BOOL errorX = FALSE, errorY = FALSE, errorZ = FALSE;
    static bool errorLoggedX = false, errorLoggedY = false, errorLoggedZ = false;

    // Read the X axis with separated offsets.
    DWORD_PTR currentAddressX = (DWORD_PTR)baseAddress + baseAddressOffsetX_Principal;
    if (!readCoordinates(hProcess, currentAddressX, offsetX, sizeof(offsetX) / sizeof(offsetX[0]), &axe_x)) {
        usedPrimaryX = false; // Indicate that the main offset failed.
        if (enableBackupOffsetX) { // Check if the backup offsets are enabled.
            currentAddressX = (DWORD_PTR)baseAddress + baseAddressOffsetX_Backup;
            if (!readCoordinates(hProcess, currentAddressX, backupOffsetX, sizeof(backupOffsetX) / sizeof(backupOffsetX[0]), &axe_x)) {
                errorX = TRUE; // Log the error.
            }
        }
    }

  // Read the Y axis with separated offsets.
    DWORD_PTR currentAddressY = (DWORD_PTR)baseAddress + baseAddressOffsetY_Principal;
    if (!readCoordinates(hProcess, currentAddressY, offsetY, sizeof(offsetY) / sizeof(offsetY[0]), &axe_y)) {
        usedPrimaryY = false; // Indicate that the main offset failed.
        if (enableBackupOffsetY) { // Check if the backup offsets are enabled.
            currentAddressY = (DWORD_PTR)baseAddress + baseAddressOffsetY_Backup;
            if (!readCoordinates(hProcess, currentAddressY, backupOffsetY, sizeof(backupOffsetY) / sizeof(backupOffsetY[0]), &axe_y)) {
                errorY = TRUE; // Log the error.
            }
        }
    }

    // Read the Z axis with separated offsets.
    DWORD_PTR currentAddressZ = (DWORD_PTR)baseAddress + baseAddressOffsetZ_Principal;
    if (!readCoordinates(hProcess, currentAddressZ, offsetZ, sizeof(offsetZ) / sizeof(offsetZ[0]), &axe_z)) {
        usedPrimaryZ = false; // Indicate that the main offset failed.
        if (enableBackupOffsetZ) { // Check if the backup offsets are enabled.
            currentAddressZ = (DWORD_PTR)baseAddress + baseAddressOffsetZ_Backup;
            if (!readCoordinates(hProcess, currentAddressZ, backupOffsetZ, sizeof(backupOffsetZ) / sizeof(backupOffsetZ[0]), &axe_z)) {
                errorZ = TRUE; // Log the error.
            }
        }
    }

   // Log success message only once.
    if (!successMessageLogged && !(errorX || errorY || errorZ)) {
        // Create the success message based on the offsets used.
        char successMsg[256];
        sprintf_s(successMsg, sizeof(successMsg), u8"Success: Coordinates found: X %s, Y %s, Z %s.",
            usedPrimaryX ? "primary" : "secondary",
            usedPrimaryY ? "primary" : "secondary",
            usedPrimaryZ ? "primary" : "secondary";
        //mumbleAPI.log(ownID, successMsg);
        successMessageLogged = true; // Mark that the message has been logged.
    }
    else if (errorX || errorY || errorZ) {
        // Reset successMessageLogged if an error occurs.
        successMessageLogged = false;
    }

    // Basic mechanism for primary axes.
    static bool errorLogged = false;
    static time_t lastErrorTime = 0; // To keep track of the time of the last error message.
    if ((errorX || errorY || errorZ) && (!errorLogged || difftime(time(NULL), lastErrorTime) >= 60)) {
        char errorMsg[256];
        strcpy_s(errorMsg, sizeof(errorMsg), u8"Error: Unable to retrieve axis position. : ");
        if (errorX) strcat_s(errorMsg, sizeof(errorMsg), "X ");
        if (errorY) strcat_s(errorMsg, sizeof(errorMsg), "Y ");
        if (errorZ) strcat_s(errorMsg, sizeof(errorMsg), "Z ");

        // Add the retry message here.
        strcat_s(errorMsg, sizeof(errorMsg), u8"Retry in progress.");

        mumbleAPI.log(ownID, errorMsg);
        errorLogged = true;
        lastErrorTime = time(NULL); // Updates the timestamp of the last error message.
    }
    else if (!errorX && !errorY && !errorZ) {
        errorLogged = false; // Reset if all readings are successful.
    }

    // Grouping of secondary axis error messages.
    static bool errorLoggedBackup = false;
    static time_t lastErrorTimeBackup = 0; // To keep track of the time of the last error message.
    if ((errorX || errorY || errorZ) && (!errorLoggedBackup || difftime(time(NULL), lastErrorTimeBackup) >= 60)) {
        char errorMsg[256];
        strcpy_s(errorMsg, sizeof(errorMsg), u8"Error: Unable to read backup axes. : ");
        if (errorX) strcat_s(errorMsg, sizeof(errorMsg), "X ");
        if (errorY) strcat_s(errorMsg, sizeof(errorMsg), "Y ");
        if (errorZ) strcat_s(errorMsg, sizeof(errorMsg), "Z ");

       // Add the retry message here.
        strcat_s(errorMsg, sizeof(errorMsg), u8"Retrying...");

        //mumbleAPI.log(ownID, errorMsg);
        errorLoggedBackup = true;
        lastErrorTimeBackup = time(NULL); // Updates the time of the last error message
    }
    else if (!errorX && !errorY && !errorZ) {
        errorLoggedBackup = false; // Reset if all readings succeed
    }

    CloseHandle(hProcess);
    return !(errorX || errorY || errorZ);
}

static void continuousGetPlayerCoordinates(void* arg) {
    while (enableGetPlayerCoordinates) {
        getPlayerCoordinates();  // Call the function to get the player's coordinates
        Sleep(100); // Wait 100 milliseconds before the next iteration to avoid overload
    }
}

// Plugin initialization function
mumble_error_t mumble_init(mumble_plugin_id_t pluginID) {
    ownID = pluginID;
    //mumbleAPI.log(ownID, u8"Plugin loaded.");

    // Enable all functions
    enableSetMaximumAudioDistance = TRUE;
    enableCheckPlayerZone = TRUE;
    enableCheckVersionThread = TRUE;
    enableStartVersionCheck = TRUE;
    enableFindProcessId = TRUE;
    enableFindBaseAddress = TRUE;
    enableReadMemoryValue = TRUE;
    enableGetPlayerCoordinates = TRUE;

   // Start the thread to get the player's coordinates
    _beginthread(continuousGetPlayerCoordinates, 0, NULL);

    connectionAttempts = 0; // Reset the connection retry counter

    return MUMBLE_STATUS_OK;
}

uint32_t mumble_getFeatures() {
    return MUMBLE_FEATURE_POSITIONAL;
}

uint8_t mumble_initPositionalData(const char* const* programNames, const uint64_t* programPIDs, size_t programCount) {
    return MUMBLE_PDEC_OK;
}

// Function to fill position data for Mumble
bool mumble_fetchPositionalData(float* avatarPos, float* avatarDir, float* avatarAxis, float* cameraPos,
    float* cameraDir, float* cameraAxis, const char** context, const char** identity) {

    // Converting coordinates from centimeters to meters
    avatarPos[0] = axe_x / 100.0f; // Convert X to meters
    avatarPos[1] = axe_y / 100.0f; // Convert Y to meters
    avatarPos[2] = axe_z / 10.0f; // Convert Z to meters

    // Copier la position de l'avatar dans cameraPos (sans offset)
    cameraPos[0] = avatarPos[0]; // X
    cameraPos[1] = avatarPos[1]; // Y
    cameraPos[2] = avatarPos[2]; // Z

    /* // Conversion des coordonnées de centimètres en mètres
    avatarPos[0] = floor(axe_x / 100.0f); // Conversion from X to meters, without decimals
    avatarPos[1] = floor(axe_y / 100.0f); // Conversion from Y to meters, without decimals
    avatarPos[2] = floor(axe_z / 10.0f);  // Conversion from Z to meters, without decimals*/

  // Checking the area to adjust the audio distance
    checkPlayerZone();

   // Fill the other fields with zeros
    memset(avatarDir, 0, 3 * sizeof(float));
    memset(avatarAxis, 0, 3 * sizeof(float));
    memset(cameraDir, 0, 3 * sizeof(float));
    memset(cameraAxis, 0, 3 * sizeof(float));
    *context = "";
    *identity = "";

    // mumbleAPI.log(ownID, u8"Position updated.");
    return true;
}

// Function to clean up positional data
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

// Return the plugin ID
static mumble_plugin_id_t mumble_getPluginID() {
    return ownID;
}

// Start displaying coordinates
void mumble_shutdown() {
    // Désactiver toutes les fonctions
    enableSetMaximumAudioDistance = FALSE;
    enableCheckPlayerZone = FALSE;
    enableCheckVersionThread = FALSE;
    enableStartVersionCheck = FALSE;
    enableFindProcessId = FALSE;
    enableFindBaseAddress = FALSE;
    enableReadMemoryValue = FALSE;
    enableGetPlayerCoordinates = FALSE;
}
