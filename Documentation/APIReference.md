# API Reference - Advanced FPS Game

This document provides a comprehensive reference for the major classes and interfaces in the Advanced FPS Game project.

## Core Systems

### AdvancedFPSGameMode

The main game mode class that manages match flow, objectives, and team management.

#### Key Methods

```cpp
// Match Management
void StartMatch();
void EndMatch(int32 WinningTeam);
void HandlePlayerKill(APlayerController* Killer, APlayerController* Victim, AActor* DamageCauser);
void HandleObjectiveCompleted(int32 ObjectiveID, int32 TeamID);

// Player Management
int32 AssignPlayerToTeam(APlayerController* Player);
void SwitchPlayerTeam(APlayerController* Player, int32 NewTeam);
void SpawnPlayer(APlayerController* Player);
void ScheduleRespawn(APlayerController* Player);

// Team Management
void UpdateTeamScore(int32 TeamID, int32 Points);
int32 GetLeadingTeam() const;
TArray<FTeamInfo> GetTeamInfo() const;

// Statistics
FGameModeStats GetMatchStatistics() const;
TArray<FPlayerInfo> GetPlayerList() const;
```

#### Events

```cpp
UPROPERTY(BlueprintAssignable)
FMatchEvent OnMatchStarted;

UPROPERTY(BlueprintAssignable)
FMatchEndEvent OnMatchEnded;

UPROPERTY(BlueprintAssignable)
FPlayerJoinEvent OnPlayerJoined;

UPROPERTY(BlueprintAssignable)
FScoreUpdateEvent OnScoreUpdated;
```

### AdvancedPlayerController

Handles player input, settings, spectating, and communication.

#### Key Methods

```cpp
// Input Handling
void Move(const FInputActionValue& Value);
void Look(const FInputActionValue& Value);
void StartFire();
void StopFire();
void StartAim();
void StopAim();

// Spectator System
void EnterSpectatorMode(ESpectatorMode Mode);
void ExitSpectatorMode();
void SpectatorNext();
void SpectatorPrevious();

// Communication
void SendChatMessage(const FString& Message, bool bTeamOnly);
void StartVoiceChat();
void StopVoiceChat();

// Settings Management
void ApplySettings(const FPlayerSettings& Settings);
FPlayerSettings GetCurrentSettings() const;

// Statistics
void AddKill(bool bHeadshot = false);
void AddDeath();
void AddAssist();
FPlayerStatistics GetSessionStatistics() const;

// Admin Commands
void ExecuteAdminCommand(const FString& Command);
```

#### Events

```cpp
UPROPERTY(BlueprintAssignable)
FInputEvent OnFireStarted;

UPROPERTY(BlueprintAssignable)
FChatMessageEvent OnChatMessageReceived;

UPROPERTY(BlueprintAssignable)
FSpectatorModeEvent OnSpectatorModeChanged;

UPROPERTY(BlueprintAssignable)
FSettingsEvent OnSettingsChanged;
```

### AdvancedWeaponSystem

Comprehensive weapon system with realistic ballistics and modular attachments.

#### Key Methods

```cpp
// Firing System
bool CanFire() const;
void Fire();
void StartReload();
void CompleteReload();

// Attachment System
bool AttachAccessory(EAttachmentType Type, UWeaponAttachment* Attachment);
bool DetachAccessory(EAttachmentType Type);
void ApplyAttachmentModifications();

// Fire Modes
void CycleFireMode();
TArray<EFireMode> GetAvailableFireModes() const;

// Ballistics
FVector CalculateFireDirection();
float CalculateCurrentAccuracy();
void ApplyRecoil();

// Properties
float GetFireDelay() const;
float GetModifiedReloadTime() const;
float GetEffectiveRange() const;
FVector GetMuzzleLocation() const;
```

#### Configuration

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite)
UWeaponData* WeaponData;

UPROPERTY(EditAnywhere, BlueprintReadWrite)
float BaseDamage = 35.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite)
float FireRate = 600.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite)
int32 MagazineCapacity = 30;

UPROPERTY(EditAnywhere, BlueprintReadWrite)
float MuzzleVelocity = 800.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite)
float BaseAccuracy = 0.95f;
```

### AdvancedHUDSystem

Comprehensive HUD system with multiple display modes and widgets.

#### Key Methods

```cpp
// HUD Management
void SetHUDStyle(EHUDStyle Style);
void ToggleHUDVisibility();
void RefreshHUD();

// Widget Management
void AddWidget(UUserWidget* Widget, int32 ZOrder = 0);
void RemoveWidget(UUserWidget* Widget);
void ClearAllWidgets();

// HUD Elements
void DrawCrosshair(FVector2D Center);
void DrawHealthBar(float HealthPercent, FVector2D Position);
void DrawAmmoCounter(int32 CurrentAmmo, int32 TotalAmmo, FVector2D Position);
void DrawDamageIndicator(FVector2D Direction, float Intensity);

// Kill Feed
void AddKillFeedEntry(const FKillFeedEntry& Entry);
void UpdateKillFeed(float DeltaTime);

// Minimap
void UpdateMinimap(FVector PlayerLocation, FRotator PlayerRotation);
void AddMinimapMarker(FVector Location, EMarkerType Type);
```

#### HUD Styles

```cpp
UENUM(BlueprintType)
enum class EHUDStyle : uint8
{
    Default         UMETA(DisplayName = "Default"),
    Minimal         UMETA(DisplayName = "Minimal"),
    Competitive     UMETA(DisplayName = "Competitive"),
    Tactical        UMETA(DisplayName = "Tactical"),
    Custom          UMETA(DisplayName = "Custom")
};
```

## Networking System

### FPSNetworkManager

Handles all networking operations including server browser, matchmaking, and anti-cheat.

#### Key Methods

```cpp
// Server Management
void StartServer(const FServerConfig& Config);
void StopServer();
bool IsServerRunning() const;

// Client Connection
void ConnectToServer(const FString& ServerAddress, int32 Port);
void DisconnectFromServer();
EConnectionState GetConnectionState() const;

// Server Browser
void RefreshServerList();
TArray<FServerInfo> GetServerList() const;

// Matchmaking
void StartMatchmaking(const FMatchmakingCriteria& Criteria);
void CancelMatchmaking();

// Anti-Cheat
void ReportSuspiciousActivity(int32 PlayerID, ECheatType CheatType);
bool ValidatePlayerAction(const FPlayerAction& Action);
```

### Network Events

```cpp
UPROPERTY(BlueprintAssignable)
FServerListEvent OnServerListUpdated;

UPROPERTY(BlueprintAssignable)
FConnectionEvent OnConnectionStateChanged;

UPROPERTY(BlueprintAssignable)
FMatchFoundEvent OnMatchFound;

UPROPERTY(BlueprintAssignable)
FCheatDetectedEvent OnCheatDetected;
```

## Audio System

### AdvancedAudioSystem

3D spatial audio system with dynamic mixing and environmental effects.

#### Key Methods

```cpp
// Audio Management
void PlaySound(USoundBase* Sound, FVector Location = FVector::ZeroVector);
void PlaySoundAttached(USoundBase* Sound, USceneComponent* AttachComponent);
void StopSound(UAudioComponent* AudioComponent);
void StopAllSounds();

// 3D Audio
void Set3DAudioSettings(const F3DAudioSettings& Settings);
void UpdateListenerPosition(FVector Position, FRotator Rotation);

// Environmental Audio
void SetEnvironmentalPreset(EEnvironmentalPreset Preset);
void UpdateEnvironmentalEffects(float DeltaTime);

// Voice Chat
void StartVoiceCapture();
void StopVoiceCapture();
void SetVoiceChatChannel(int32 Channel);

// Audio Buses
void SetBusVolume(EAudioBus Bus, float Volume);
float GetBusVolume(EAudioBus Bus) const;
```

## Destruction System

### EnvironmentalDestructionSystem

Real-time destruction system with multiple damage types and debris management.

#### Key Methods

```cpp
// Destruction Management
void ApplyDamage(AActor* Target, float Damage, EDestructionType Type);
void CreateDestruction(FVector Location, float Radius, float Force);
void RegisterDestructibleActor(AActor* Actor);
void UnregisterDestructibleActor(AActor* Actor);

// Debris System
void SpawnDebris(FVector Location, TArray<UStaticMesh*> DebrisMeshes);
void CleanupDebris(float MaxAge = 30.0f);

// Performance Management
void UpdateDestructionLOD();
void OptimizeDestructionPerformance();
```

## Performance System

### PerformanceOptimizationSystem

Comprehensive performance monitoring and optimization system.

#### Key Methods

```cpp
// Performance Monitoring
FPerformanceMetrics GetCurrentMetrics() const;
void StartPerformanceCapture();
void StopPerformanceCapture();

// LOD Management
void UpdateLODLevels();
void SetLODSettings(const FLODSettings& Settings);

// Culling System
void UpdateCulling();
void SetCullingSettings(const FCullingSettings& Settings);

// Object Pooling
template<typename T>
T* GetPooledObject();
template<typename T>
void ReturnPooledObject(T* Object);

// Memory Management
void ForceGarbageCollection();
FMemoryStats GetMemoryStatistics() const;
```

## Data Structures

### Weapon System

```cpp
USTRUCT(BlueprintType)
struct FWeaponData
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseDamage;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FireRate;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MagazineCapacity;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EFireMode> SupportedFireModes;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector2D> RecoilPattern;
};

USTRUCT(BlueprintType)
struct FAttachmentModifiers
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DamageMultiplier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AccuracyBonus = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RangeMultiplier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RecoilReduction = 0.0f;
};
```

### Player System

```cpp
USTRUCT(BlueprintType)
struct FPlayerStatistics
{
    UPROPERTY(BlueprintReadOnly)
    int32 Kills;
    
    UPROPERTY(BlueprintReadOnly)
    int32 Deaths;
    
    UPROPERTY(BlueprintReadOnly)
    int32 Assists;
    
    UPROPERTY(BlueprintReadOnly)
    float DamageDealt;
    
    UPROPERTY(BlueprintReadOnly)
    float Accuracy;
    
    UPROPERTY(BlueprintReadOnly)
    float KillDeathRatio;
};

USTRUCT(BlueprintType)
struct FPlayerSettings
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MouseSensitivity = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FieldOfView = 90.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bInvertMouseY = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGraphicsSettings GraphicsSettings;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MasterVolume = 1.0f;
};
```

### Networking

```cpp
USTRUCT(BlueprintType)
struct FServerInfo
{
    UPROPERTY(BlueprintReadOnly)
    FString ServerName;
    
    UPROPERTY(BlueprintReadOnly)
    FString Map;
    
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers;
    
    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers;
    
    UPROPERTY(BlueprintReadOnly)
    int32 Ping;
    
    UPROPERTY(BlueprintReadOnly)
    EGameType GameMode;
};
```

## Enumerations

### Game States

```cpp
UENUM(BlueprintType)
enum class EMatchState : uint8
{
    WaitingToStart  UMETA(DisplayName = "Waiting to Start"),
    InProgress      UMETA(DisplayName = "In Progress"),
    Ended          UMETA(DisplayName = "Ended"),
    Paused         UMETA(DisplayName = "Paused")
};

UENUM(BlueprintType)
enum class EGameType : uint8
{
    TeamDeathmatch  UMETA(DisplayName = "Team Deathmatch"),
    FreeForAll      UMETA(DisplayName = "Free For All"),
    Domination      UMETA(DisplayName = "Domination"),
    SearchAndDestroy UMETA(DisplayName = "Search & Destroy"),
    CaptureTheFlag  UMETA(DisplayName = "Capture The Flag"),
    BattleRoyale    UMETA(DisplayName = "Battle Royale")
};
```

### Weapon Types

```cpp
UENUM(BlueprintType)
enum class EFireMode : uint8
{
    Semi    UMETA(DisplayName = "Semi-Automatic"),
    Auto    UMETA(DisplayName = "Full-Automatic"),
    Burst   UMETA(DisplayName = "Burst Fire"),
    Safety  UMETA(DisplayName = "Safety")
};

UENUM(BlueprintType)
enum class EAttachmentType : uint8
{
    Optic       UMETA(DisplayName = "Optic"),
    Suppressor  UMETA(DisplayName = "Suppressor"),
    Grip        UMETA(DisplayName = "Grip"),
    Stock       UMETA(DisplayName = "Stock"),
    Magazine    UMETA(DisplayName = "Magazine"),
    Laser       UMETA(DisplayName = "Laser"),
    Flashlight  UMETA(DisplayName = "Flashlight")
};
```

## Usage Examples

### Basic Weapon Setup

```cpp
// Create weapon system component
UAdvancedWeaponSystem* WeaponSystem = CreateDefaultSubobject<UAdvancedWeaponSystem>(TEXT("WeaponSystem"));

// Configure weapon data
UWeaponData* RifleData = LoadObject<UWeaponData>(nullptr, TEXT("/Game/Weapons/Data/AssaultRifle"));
WeaponSystem->SetWeaponData(RifleData);

// Fire weapon
if (WeaponSystem->CanFire())
{
    WeaponSystem->Fire();
}

// Attach scope
UWeaponAttachment* Scope = LoadObject<UWeaponAttachment>(nullptr, TEXT("/Game/Weapons/Attachments/RedDotSight"));
WeaponSystem->AttachAccessory(EAttachmentType::Optic, Scope);
```

### Player Controller Integration

```cpp
// Set up Enhanced Input
if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
{
    EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AMyPlayerController::StartFire);
    EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AMyPlayerController::Reload);
}

// Handle player statistics
void AMyPlayerController::OnPlayerKill()
{
    AddKill();
    AddScore(100);
}

// Apply player settings
FPlayerSettings NewSettings;
NewSettings.MouseSensitivity = 1.5f;
NewSettings.FieldOfView = 110.0f;
ApplySettings(NewSettings);
```

### Networking Integration

```cpp
// Start dedicated server
FServerConfig Config;
Config.ServerName = TEXT("My FPS Server");
Config.MaxPlayers = 16;
Config.Map = TEXT("TestMap");
NetworkManager->StartServer(Config);

// Connect to server
NetworkManager->ConnectToServer(TEXT("192.168.1.100"), 7777);

// Handle connection events
NetworkManager->OnConnectionStateChanged.AddDynamic(this, &AMyGameInstance::OnConnectionChanged);
```

This API reference provides the foundation for using and extending the Advanced FPS Game systems. For more detailed implementation examples, see the source code and documentation files.
