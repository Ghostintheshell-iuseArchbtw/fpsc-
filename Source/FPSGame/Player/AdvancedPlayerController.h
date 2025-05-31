#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"
#include "AdvancedPlayerController.generated.h"

// Forward declarations
class UInputMappingContext;
class UInputAction;
class AAdvancedWeaponSystem;
class AAdvancedHUDSystem;
class AAdvancedAudioSystem;
class AFPSNetworkManager;
class AAdvancedFPSCharacter;

UENUM(BlueprintType)
enum class EPlayerState : uint8
{
    Playing         UMETA(DisplayName = "Playing"),
    Spectating      UMETA(DisplayName = "Spectating"),
    Dead            UMETA(DisplayName = "Dead"),
    Connecting      UMETA(DisplayName = "Connecting"),
    Loading         UMETA(DisplayName = "Loading"),
    Menu            UMETA(DisplayName = "Menu")
};

UENUM(BlueprintType)
enum class ESpectatorMode : uint8
{
    Free            UMETA(DisplayName = "Free Camera"),
    Following       UMETA(DisplayName = "Following Player"),
    FirstPerson     UMETA(DisplayName = "First Person"),
    ThirdPerson     UMETA(DisplayName = "Third Person")
};

USTRUCT(BlueprintType)
struct FPlayerSettings
{
    GENERATED_BODY()

    // Input Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bInvertMouseY = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float ADSSensitivityMultiplier = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bToggleADS = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bToggleCrouch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bToggleSprint = false;

    // Audio Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float MasterVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float EffectsVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float MusicVolume = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    float VoiceChatVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    bool bEnableVoiceChat = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    bool bPushToTalk = true;

    // Video Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
    int32 FieldOfView = 90;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
    bool bShowFPS = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
    float Brightness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
    float Contrast = 1.0f;

    // HUD Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    bool bShowMinimap = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    bool bShowKillFeed = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    bool bShowCrosshair = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    float HUDScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    FLinearColor CrosshairColor = FLinearColor::White;

    // Gameplay Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bAutoReload = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bAutoSwitchWeapons = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bShowDamageNumbers = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bAutoPickupWeapons = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bAutoPickupAmmo = true;
};

USTRUCT(BlueprintType)
struct FKeybindSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FKey> ActionBindings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FKey> AxisBindings;
};

USTRUCT(BlueprintType)
struct FPlayerStatistics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TotalKills = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalDeaths = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalAssists = 0;

    UPROPERTY(BlueprintReadOnly)
    float TotalPlayTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalMatchesPlayed = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalMatchesWon = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageAccuracy = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalHeadshots = 0;

    UPROPERTY(BlueprintReadOnly)
    float TotalDamageDealt = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, int32> WeaponKills;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, float> WeaponAccuracy;
};

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API AAdvancedPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AAdvancedPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Input System
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* FireAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* AimAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* ReloadAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* CrouchAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* InteractAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* WeaponSlot1Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* WeaponSlot2Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* WeaponSlot3Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* ScoreboardAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MenuAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* VoiceChatAction;

    // Player State
    UPROPERTY(BlueprintReadOnly, Category = "Player State", ReplicatedUsing = OnRep_PlayerState)
    EPlayerState CurrentPlayerState = EPlayerState::Connecting;

    UPROPERTY(BlueprintReadOnly, Category = "Player State", Replicated)
    int32 TeamID = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Player State", Replicated)
    FString PlayerDisplayName;

    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FPlayerSettings PlayerSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FKeybindSettings KeybindSettings;

    // Statistics
    UPROPERTY(BlueprintReadOnly, Category = "Statistics")
    FPlayerStatistics PlayerStats;

    // Spectator System
    UPROPERTY(BlueprintReadOnly, Category = "Spectator", Replicated)
    ESpectatorMode SpectatorMode = ESpectatorMode::Free;

    UPROPERTY(BlueprintReadOnly, Category = "Spectator", Replicated)
    APlayerController* SpectatingTarget = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Spectator")
    TArray<APlayerController*> SpectatorTargets;

    // System References
    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    AAdvancedHUDSystem* HUDSystem;

    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    AAdvancedAudioSystem* AudioSystem;

    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    AFPSNetworkManager* NetworkManager;

    // Input Handling
    UFUNCTION(BlueprintCallable, Category = "Input")
    void Move(const FInputActionValue& Value);

    UFUNCTION(BlueprintCallable, Category = "Input")
    void Look(const FInputActionValue& Value);

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StartJump();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StopJump();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StartAim();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StopAim();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void Reload();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StartCrouch();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StopCrouch();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StartSprint();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StopSprint();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void Interact();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void SelectWeaponSlot1();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void SelectWeaponSlot2();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void SelectWeaponSlot3();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void ToggleScoreboard();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void ToggleMenu();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StartVoiceChat();

    UFUNCTION(BlueprintCallable, Category = "Input")
    void StopVoiceChat();

    // Player Management
    UFUNCTION(BlueprintCallable, Category = "Player")
    void SetPlayerTeam(int32 NewTeamID);

    UFUNCTION(BlueprintCallable, Category = "Player")
    void SetPlayerDisplayName(const FString& NewName);

    UFUNCTION(BlueprintCallable, Category = "Player")
    void SetPlayerState(EPlayerState NewState);

    UFUNCTION(BlueprintCallable, Category = "Player")
    void RespawnPlayer();

    UFUNCTION(BlueprintCallable, Category = "Player")
    void KillPlayer();

    // Spectator Functions
    UFUNCTION(BlueprintCallable, Category = "Spectator")
    void StartSpectating();

    UFUNCTION(BlueprintCallable, Category = "Spectator")
    void StopSpectating();

    UFUNCTION(BlueprintCallable, Category = "Spectator")
    void SetSpectatorMode(ESpectatorMode NewMode);

    UFUNCTION(BlueprintCallable, Category = "Spectator")
    void SpectateNextPlayer();

    UFUNCTION(BlueprintCallable, Category = "Spectator")
    void SpectatePreviousPlayer();

    UFUNCTION(BlueprintCallable, Category = "Spectator")
    void SpectatePlayer(APlayerController* TargetPlayer);

    // Settings Management
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplyPlayerSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SavePlayerSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void LoadPlayerSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ResetSettingsToDefault();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMouseSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetFieldOfView(int32 NewFOV);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMasterVolume(float NewVolume);

    // Statistics
    UFUNCTION(BlueprintCallable, Category = "Statistics")
    void UpdateStatistics(const FString& StatName, float Value);

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    void AddKill(const FString& WeaponName);

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    void AddDeath();

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    void AddAssist();

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    void UpdateAccuracy(const FString& WeaponName, float ShotsHit, float ShotsFired);

    UFUNCTION(BlueprintPure, Category = "Statistics")
    float GetKDRatio() const;

    UFUNCTION(BlueprintPure, Category = "Statistics")
    float GetWinRate() const;

    // Communication
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendChatMessage(const FString& Message, bool bTeamOnly = false);

    UFUNCTION(BlueprintCallable, Category = "Communication")
    void StartVoiceChat();

    UFUNCTION(BlueprintCallable, Category = "Communication")
    void StopVoiceChat();

    UFUNCTION(BlueprintCallable, Category = "Communication")
    void MutePlayer(APlayerController* TargetPlayer);

    UFUNCTION(BlueprintCallable, Category = "Communication")
    void UnmutePlayer(APlayerController* TargetPlayer);

    // Network Functions
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetPlayerTeam(int32 NewTeamID);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetPlayerDisplayName(const FString& NewName);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSendChatMessage(const FString& Message, bool bTeamOnly);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateStatistic(const FString& StatName, float Value);

    UFUNCTION(Client, Reliable)
    void ClientReceiveChatMessage(const FString& SenderName, const FString& Message, bool bTeamMessage);

    UFUNCTION(Client, Reliable)
    void ClientShowKillFeed(const FString& KillerName, const FString& VictimName, const FString& WeaponName);

    UFUNCTION(Client, Reliable)
    void ClientShowDamageIndicator(const FVector& DamageDirection, float DamageAmount);

    UFUNCTION(Client, Reliable)
    void ClientPlaySound(USoundBase* Sound, float Volume = 1.0f);

    // Admin Functions
    UFUNCTION(BlueprintCallable, Category = "Admin")
    void AdminKickPlayer(const FString& PlayerName, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category = "Admin")
    void AdminBanPlayer(const FString& PlayerName, const FString& Reason, float Duration);

    UFUNCTION(BlueprintCallable, Category = "Admin")
    void AdminChangeMap(const FString& MapName);

    UFUNCTION(BlueprintCallable, Category = "Admin")
    void AdminSetGameMode(const FString& GameModeName);

    // Events
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChanged, EPlayerState, NewState);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlayerStateChanged OnPlayerStateChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamChanged, int32, NewTeamID);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTeamChanged OnTeamChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChatMessage, const FString&, SenderName, const FString&, Message, bool, bTeamMessage);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnChatMessage OnChatMessage;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSettingsChanged, const FPlayerSettings&, NewSettings);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSettingsChanged OnSettingsChanged;

    // Overridden Functions
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void OnRep_Pawn() override;
    virtual void ClientRestart_Implementation(APawn* NewPawn) override;

    // Utility Functions
    UFUNCTION(BlueprintPure, Category = "Utility")
    bool IsPlayerAlive() const;

    UFUNCTION(BlueprintPure, Category = "Utility")
    bool IsSpectating() const;

    UFUNCTION(BlueprintPure, Category = "Utility")
    bool IsOnTeam(int32 TestTeamID) const;

    UFUNCTION(BlueprintPure, Category = "Utility")
    AAdvancedFPSCharacter* GetAdvancedCharacter() const;

    UFUNCTION(BlueprintPure, Category = "Utility")
    bool HasPermission(const FString& PermissionName) const;

private:
    // Internal state
    bool bScoreboardVisible = false;
    bool bMenuVisible = false;
    bool bVoiceChatActive = false;
    float LastInputTime = 0.0f;
    
    // Muted players
    UPROPERTY()
    TArray<APlayerController*> MutedPlayers;
    
    // Input state
    FVector2D MovementInput = FVector2D::ZeroVector;
    FVector2D LookInput = FVector2D::ZeroVector;
    
    // Timers
    FTimerHandle RespawnTimerHandle;
    FTimerHandle IdleTimerHandle;
    
    // Internal functions
    void InitializeSystems();
    void UpdateSpectatorTargets();
    void HandleIdleTimeout();
    void ProcessMovementInput();
    void ProcessLookInput();
    void UpdateHUDElements();
    void ValidateSettings();
    void ApplyVideoSettings();
    void ApplyAudioSettings();
    void ApplyInputSettings();
    void LoadDefaultSettings();
    void CheckPermissions();

    // Network replication functions
    UFUNCTION()
    void OnRep_PlayerState();

    UFUNCTION()
    void OnRep_TeamID();

    UFUNCTION()
    void OnRep_SpectatorMode();
};
