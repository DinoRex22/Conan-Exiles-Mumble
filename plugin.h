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

#ifndef PLUGIN_H
#define PLUGIN_H

// ============================================================================
// INCLUDES
// ============================================================================
#include "MumblePlugin_v_1_0_x.h"
#include <windows.h>
#include <stdbool.h>

// ============================================================================
// MACROS
// ============================================================================
#define CONFIG_FILE L"plugin.cfg"
#define MIN_AUDIBLE_VOLUME 0.1f

#ifndef _UNICODE
#define _UNICODE
#endif

// ============================================================================
// STRUCTURES
// ============================================================================

// Mod file data structure | Structure des données du fichier mod
struct ModFileData {
    int seq;
    float x, y, z, yaw, yawY;
    BOOL valid;
};

// Complete positional data | Données positionnelles complètes
#pragma pack(push, 1)
typedef struct {
    float x, y, z;
    float dirX, dirY, dirZ;
    float axisX, axisY, axisZ;
    float voiceDistance;
    char playerName[16];
} CompletePositionalData;
#pragma pack(pop)

// Vector3 structure | Structure Vector3
typedef struct {
    float x, y, z;
} Vector3;

// Adaptive player data | Données par joueur pour le volume adaptatif
typedef struct {
    mumble_userid_t userID;
    char playerName[64];
    Vector3 position;
    float voiceDistance;
    float currentVolume;
    bool isValid;
    ULONGLONG lastVolumeUpdate;
} AdaptivePlayerData;

// Audio volume state structure | Structure pour stocker les volumes par utilisateur
typedef struct {
    mumble_userid_t userID;
    float targetVolume;
    float currentVolume;
    float leftVolume;
    float rightVolume;
    ULONGLONG lastUpdate;
    bool isValid;
} AudioVolumeState;

// Low pass filter state | État du filtre passe-bas
typedef struct {
    mumble_userid_t userID;
    float lastSampleLeft;
    float lastSampleRight;
    float lastCutoff;
    float lastAlpha;
    ULONGLONG lastUpdate;
    bool isInitialized;
} LowPassFilterState;

// Player mute state | État de mute d'un joueur
typedef struct {
    mumble_userid_t userID;
    char playerName[64];
    bool currentlyMuted;
    ULONGLONG lastMuteCheck;
} PlayerMuteState;

// ============================================================================
// VOICE RANGE PRESETS | PRESETS DE PORTÉE VOCALE
// ============================================================================

#define MAX_VOICE_PRESETS 10
#define PRESET_NAME_MAX_LENGTH 64

// Voice range preset structure | Structure de preset de portée vocale
typedef struct {
    char name[PRESET_NAME_MAX_LENGTH];
    float whisperDistance;
    float normalDistance;
    float shoutDistance;
    BOOL isUsed;
} VoiceRangePreset;

// Voice preset global variables | Variables globales pour les presets vocaux
extern VoiceRangePreset voicePresets[MAX_VOICE_PRESETS];
extern int currentPresetIndex;
extern HWND hCategoryPresets;

// Voice preset functions | Fonctions pour les presets vocaux
static void initializeVoicePresets(void);
static void saveVoicePreset(int presetIndex, const char* presetName);
static void loadVoicePreset(int presetIndex);
static BOOL renameVoicePreset(int presetIndex, const char* newName);
static void savePresetsToConfigFile(void);
static void loadPresetsFromConfigFile(void);

// Voice preset interface functions | Fonctions d'interface pour les presets vocaux
static void showPresetSaveDialog(void);
static LRESULT CALLBACK PresetSaveDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PresetRenameDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void createPresetsCategory(void);
static void updatePresetLabels(void);

// ============================================================================
// EXTERN VARIABLES
// ============================================================================

// Plugin control variables
extern BOOL enableGetPlayerCoordinates;
extern BOOL TEMP;

// Log control variables
extern BOOL enableLogCoordinates;
extern BOOL enableLogModFile;
extern BOOL enableLogConfig;
extern BOOL enableLogGeneral;

// Mumble API interface
extern struct MumbleAPI_v_1_0_x mumbleAPI;
extern mumble_plugin_id_t ownID;

// Audio distance variables
extern double serverMaximumAudioDistance;
extern BOOL maxAudioDistanceRetrieved;
extern ULONGLONG lastMaxDistanceCheck;

// Air diffusion option
extern BOOL enableAirDiffusion;

// Overlay border highlight variables
extern BOOL overlayBorderHighlight;
extern mumble_userid_t overlayHighlightUserID;
extern char overlaySpeakerText[128];
extern CRITICAL_SECTION overlayTextLock;

// Overlay variables
extern HWND hVoiceOverlay;
extern HWND hVoiceText;
extern BOOL enableVoiceOverlay;
extern HFONT hOverlayFont;
extern BOOL overlayThreadRunning;

// Channel management variables
extern mumble_channelid_t hubChannelID;
extern mumble_channelid_t rootChannelID;
extern mumble_channelid_t ingameChannelID;
extern mumble_channelid_t lastTargetChannel;
extern mumble_channelid_t lastValidChannel;
extern BOOL channelManagementActive;
extern BOOL enableAutomaticChannelChange;
extern ULONGLONG lastChannelCheck;

// Player position variables
extern float axe_x;
extern float axe_y;
extern float axe_z;
extern float avatarAxisX;
extern float avatarAxisY;
extern float avatarAxisZ;

// Adaptive mod system variables
extern time_t lastFileCheck;
extern time_t LastFileModification;
extern int lastSeq;
extern BOOL modDataValid;
extern char modFilePath[MAX_PATH];
extern BOOL coordinatesValid;
extern struct ModFileData currentModData;
extern ULONGLONG lastModDataTick;

// Global variables for the interface
extern HWND hConfigDialog;
extern HWND hWhisperKeyEdit, hNormalKeyEdit, hShoutKeyEdit, hConfigKeyEdit;
extern HWND hWhisperButton, hNormalButton, hShoutButton, hConfigButton;
extern HWND hEnableDistanceMutingCheck, hEnableAutomaticChannelChangeCheck;
extern HWND hDistanceWhisperEdit, hDistanceNormalEdit, hDistanceShoutEdit;
extern HWND hSavedPathEdit, hSavedPathButton;
extern HWND hCategoryPatch, hCategoryAdvanced;
extern HWND hEnableVoiceToggleCheck, hVoiceToggleKeyEdit, hVoiceToggleButton;
extern HWND hStatusMessage;
extern HWND hDistanceLimitMessage;
extern HFONT hFont, hFontBold, hFontLarge, hFontEmoji;

// Interface message controls
extern HWND hDistanceWhisperMessage;
extern HWND hDistanceNormalMessage;
extern HWND hDistanceShoutMessage;
extern HWND hDistanceMutingMessage;
extern HWND hChannelSwitchingMessage;
extern HWND hPositionalAudioMessage;
extern HWND hDistanceServerLimitWhisper;
extern HWND hDistanceServerLimitNormal;
extern HWND hDistanceServerLimitShout;

// Interface state variables
extern int currentCategory;
extern BOOL isCapturingKey;
extern int captureKeyTarget;
extern wchar_t savedPath[MAX_PATH];
extern BOOL isUpdatingInterface;
extern ULONGLONG lastInterfaceUpdate;

// Interface text constants
extern const wchar_t* infoText1;
extern const wchar_t* infoText2;
extern const wchar_t* infoText3;

// Voice toggle variables
extern int voiceToggleKey;
extern BOOL enableVoiceToggle;
extern ULONGLONG lastVoiceTogglePress;

// Key bindings
extern int whisperKey;
extern int normalKey;
extern int shoutKey;
extern int configUIKey;

// Key monitoring variables
extern BOOL isConfigDialogOpen;
extern DWORD lastKeyPressTime;
extern BOOL keyMonitorThreadRunning;
extern HANDLE keyMonitorThread;
extern BOOL lastKeyState;

// Connection and hub state variables
extern BOOL isConnectedToServer;
extern BOOL hubDescriptionAvailable;
extern BOOL hubLimitsActive;
extern ULONGLONG lastConnectionCheck;

// Hub audio parameters
extern double hubAudioMinDistance;
extern double hubAudioMaxDistance;
extern double hubAudioMaxVolume;
extern double hubAudioBloom;
extern double hubAudioFilterIntensity;
extern BOOL hubForcePositionalAudio;
extern ULONGLONG lastHubDescriptionCheck;
extern char* lastHubDescriptionCache;

// Hub distance limits
extern double hubMinimumWhisper;
extern double hubMaximumWhisper;
extern double hubMinimumNormal;
extern double hubMaximumNormal;
extern double hubMinimumShout;
extern double hubMaximumShout;
extern BOOL hubForceDistanceBasedMuting;
extern BOOL hubForceAutomaticChannelSwitching;

// Voice system variables
extern CompletePositionalData localVoiceData;
extern CompletePositionalData remotePlayersData[64];
extern size_t remotePlayerCount;
extern ULONGLONG lastVoiceDataSent;
extern ULONGLONG lastKeyCheck;

// Voice distance settings
extern float distanceWhisper;
extern float distanceNormal;
extern float distanceShout;

// Voice features
extern BOOL enableDistanceMuting;

// Mute system variables
extern PlayerMuteState playerMuteStates[64];
extern size_t playerMuteStateCount;
extern ULONGLONG lastDistanceCheck;

// Refresh variables
extern BOOL forceGlobalMuteRefresh;
extern ULONGLONG lastGlobalRefresh;

// Automatic audio settings variables
extern BOOL enableAutoAudioSettings;
extern ULONGLONG lastAudioSettingsApply;

// Adaptive system variables
extern AdaptivePlayerData adaptivePlayerStates[64];
extern size_t adaptivePlayerCount;
extern Vector3 localPlayerPosition;

// Audio volume states
extern AudioVolumeState audioVolumeStates[64];
extern size_t audioVolumeCount;

// Low pass filter variables
extern LowPassFilterState lowPassStates[64];
extern size_t lowPassStateCount;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Update dynamic interface
static void updateDynamicInterface(void);

// Force interface refresh
static void forceInterfaceRefresh(void);

// Update voice overlay display
static void updateVoiceOverlay(void);

// Initialize channel IDs
static void initializeChannelIDs(void);

// Update consolidated distance messages
static void updateConsolidatedDistanceMessages(void);

// Calculate local positional audio
static void calculateLocalPositionalAudio(const CompletePositionalData* remoteData, mumble_userid_t userID);

// Apply distance changes to all connected players
static void applyDistanceToAllPlayers(void);

// Save voice settings
static void saveVoiceSettings(void);

// Remove key monitoring
static void removeKeyMonitoring(void);

// Install key monitoring
static void installKeyMonitoring(void);

// Show status message
static void showStatusMessage(const wchar_t* message, BOOL isError);

// Clear status message
static void clearStatusMessage(void);

// Handle distance edit change
static void handleDistanceEditChange(int editId);

// Update distance muting message
static void updateDistanceMutingMessage(void);

// Update channel switching message
static void updateChannelSwitchingMessage(void);

// Update positional audio message
static void updatePositionalAudioMessage(void);

static void DrawButtonWithBitmap(LPDRAWITEMSTRUCT lpDIS);

#endif // PLUGIN_H
