#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "AdvancedFPSGameMode.generated.h"

// Forward declarations
class APerformanceOptimizationSystem;
class AAdvancedHUDSystem;
class AAdvancedAudioSystem;
class AEnvironmentalDestructionSystem;
class AFPSNetworkManager;
class AAdvancedWeaponSystem;

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Warmup          UMETA(DisplayName = "Warmup"),
    Active          UMETA(DisplayName = "Active"),
    Ending          UMETA(DisplayName = "Ending"),
    PostGame        UMETA(DisplayName = "Post Game")
};

UENUM(BlueprintType)
enum class EGameType : uint8
{
    TeamDeathMatch  UMETA(DisplayName = "Team Deathmatch"),
    FreeForAll      UMETA(DisplayName = "Free For All"),
    Domination      UMETA(DisplayName = "Domination"),
    SearchDestroy   UMETA(DisplayName = "Search & Destroy"),
    Capture         UMETA(DisplayName = "Capture the Flag"),
    Hardcore        UMETA(DisplayName = "Hardcore"),
    Battle          UMETA(DisplayName = "Battle Royale")
};

USTRUCT(BlueprintType)
struct FGameSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EGameType GameType = EGameType::TeamDeathMatch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 MaxPlayers = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 MaxTeams = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float MatchDuration = 600.0f; // 10 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 ScoreLimit = 75;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float WarmupDuration = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bFriendlyFire = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bAutoBalance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    float RespawnDelay = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bAllowSpectating = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardcore")
    bool bHardcoreMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardcore")
    float HardcoreDamageMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardcore")
    bool bHardcoreNoHUD = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    bool bEconomySystem = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 StartingMoney = 800;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 MoneyPerKill = 300;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 MoneyPerRound = 1000;
};

USTRUCT(BlueprintType)
struct FPlayerScore
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 Kills = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Deaths = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Assists = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Score = 0;

    UPROPERTY(BlueprintReadOnly)
    float Accuracy = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 Headshots = 0;

    UPROPERTY(BlueprintReadOnly)
    float DamageDealt = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float DamageTaken = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 Money = 0;

    float GetKDRatio() const { return Deaths > 0 ? (float)Kills / Deaths : Kills; }
};

USTRUCT(BlueprintType)
struct FTeamScore
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TeamID = 0;

    UPROPERTY(BlueprintReadOnly)
    FString TeamName;

    UPROPERTY(BlueprintReadOnly)
    int32 Score = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Kills = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Deaths = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<APlayerController*> Players;
};

USTRUCT(BlueprintType)
struct FMatchStatistics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float MatchDuration = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalKills = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalDeaths = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageAccuracy = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalHeadshots = 0;

    UPROPERTY(BlueprintReadOnly)
    float TotalDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    APlayerController* MVPPlayer = nullptr;

    UPROPERTY(BlueprintReadOnly)
    int32 WinningTeam = -1;
};

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API AAdvancedFPSGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAdvancedFPSGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Game Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
    FGameSettings GameSettings;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    EGamePhase CurrentGamePhase = EGamePhase::Warmup;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float TimeRemaining = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentRound = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 MaxRounds = 12;

    // Team Management
    UPROPERTY(BlueprintReadOnly, Category = "Teams")
    TArray<FTeamScore> Teams;

    // Player Management
    UPROPERTY(BlueprintReadOnly, Category = "Players")
    TMap<APlayerController*, FPlayerScore> PlayerScores;

    // Statistics
    UPROPERTY(BlueprintReadOnly, Category = "Statistics")
    FMatchStatistics MatchStats;

    // System References
    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    APerformanceOptimizationSystem* PerformanceSystem;

    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    AAdvancedAudioSystem* AudioSystem;

    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    AEnvironmentalDestructionSystem* DestructionSystem;

    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    AFPSNetworkManager* NetworkManager;

    // Game Flow Functions
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    virtual void StartMatch();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    virtual void EndMatch();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    virtual void StartNewRound();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    virtual void EndCurrentRound();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    virtual void RestartMatch();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    virtual bool ShouldEndMatch() const;

    // Player Management
    UFUNCTION(BlueprintCallable, Category = "Players")
    virtual void OnPlayerJoined(APlayerController* NewPlayer);

    UFUNCTION(BlueprintCallable, Category = "Players")
    virtual void OnPlayerLeft(APlayerController* LeavingPlayer);

    UFUNCTION(BlueprintCallable, Category = "Players")
    virtual void AssignPlayerToTeam(APlayerController* Player, int32 TeamID = -1);

    UFUNCTION(BlueprintCallable, Category = "Players")
    virtual void RespawnPlayer(APlayerController* Player);

    UFUNCTION(BlueprintCallable, Category = "Players")
    virtual void SpectatePlayer(APlayerController* Spectator, APlayerController* Target);

    // Scoring System
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    virtual void OnPlayerKilled(APlayerController* Killer, APlayerController* Victim, AAdvancedWeaponSystem* Weapon);

    UFUNCTION(BlueprintCallable, Category = "Scoring")
    virtual void OnPlayerAssist(APlayerController* Assistant, APlayerController* Victim);

    UFUNCTION(BlueprintCallable, Category = "Scoring")
    virtual void OnObjectiveCompleted(APlayerController* Player, const FString& ObjectiveType);

    UFUNCTION(BlueprintCallable, Category = "Scoring")
    virtual void AddScore(APlayerController* Player, int32 Points, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category = "Scoring")
    virtual void AddTeamScore(int32 TeamID, int32 Points);

    // Team Management
    UFUNCTION(BlueprintCallable, Category = "Teams")
    virtual int32 GetPlayerTeam(APlayerController* Player) const;

    UFUNCTION(BlueprintCallable, Category = "Teams")
    virtual int32 GetSmallestTeam() const;

    UFUNCTION(BlueprintCallable, Category = "Teams")
    virtual void BalanceTeams();

    UFUNCTION(BlueprintCallable, Category = "Teams")
    virtual bool ArePlayersOnSameTeam(APlayerController* Player1, APlayerController* Player2) const;

    UFUNCTION(BlueprintCallable, Category = "Teams")
    virtual int32 GetWinningTeam() const;

    // Economy System
    UFUNCTION(BlueprintCallable, Category = "Economy")
    virtual void AddMoney(APlayerController* Player, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Economy")
    virtual bool SpendMoney(APlayerController* Player, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Economy")
    virtual int32 GetPlayerMoney(APlayerController* Player) const;

    // Statistics
    UFUNCTION(BlueprintCallable, Category = "Statistics")
    virtual void UpdatePlayerStatistics(APlayerController* Player, const FString& StatName, float Value);

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    virtual FPlayerScore GetPlayerScore(APlayerController* Player) const;

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    virtual APlayerController* GetMVPPlayer() const;

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    virtual void CalculateMatchStatistics();

    // Game Mode Specific Functions
    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void SetGameType(EGameType NewGameType);

    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void ConfigureTeamDeathmatch();

    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void ConfigureFreeForAll();

    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void ConfigureDomination();

    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void ConfigureSearchAndDestroy();

    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void ConfigureCaptureTheFlag();

    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void ConfigureHardcore();

    UFUNCTION(BlueprintCallable, Category = "Game Modes")
    virtual void ConfigureBattleRoyale();

    // Event Handlers
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnMatchStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnMatchEnded(const FMatchStatistics& Stats);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnRoundStarted(int32 RoundNumber);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnRoundEnded(int32 WinningTeam);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnPlayerScoreChanged(APlayerController* Player, const FPlayerScore& NewScore);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnTeamScoreChanged(int32 TeamID, int32 NewScore);

    // Overridden Functions
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    virtual void HandleMatchIsWaitingToStart() override;
    virtual void HandleMatchHasStarted() override;
    virtual void HandleMatchHasEnded() override;

    // Utility Functions
    UFUNCTION(BlueprintPure, Category = "Utility")
    virtual FString GetFormattedTime(float TimeSeconds) const;

    UFUNCTION(BlueprintPure, Category = "Utility")
    virtual bool IsMatchActive() const;

    UFUNCTION(BlueprintPure, Category = "Utility")
    virtual bool IsWarmupPhase() const;

    UFUNCTION(BlueprintPure, Category = "Utility")
    virtual float GetMatchProgress() const;

    UFUNCTION(BlueprintCallable, Category = "Admin")
    virtual void AdminKickPlayer(APlayerController* TargetPlayer, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category = "Admin")
    virtual void AdminBanPlayer(APlayerController* TargetPlayer, const FString& Reason, float BanDuration);

    UFUNCTION(BlueprintCallable, Category = "Admin")
    virtual void AdminChangeMap(const FString& MapName);

    // Events
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerKilledDelegate, APlayerController*, Killer, APlayerController*, Victim, AAdvancedWeaponSystem*, Weapon);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlayerKilledDelegate OnPlayerKilledDelegate;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EGamePhase, NewPhase);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGamePhaseChanged OnGamePhaseChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreChanged, APlayerController*, Player, int32, NewScore);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnScoreChanged OnScoreChanged;

private:
    // Internal state
    FTimerHandle MatchTimerHandle;
    FTimerHandle RoundTimerHandle;
    FTimerHandle WarmupTimerHandle;
    
    float MatchStartTime = 0.0f;
    
    // Internal functions
    void InitializeSystems();
    void InitializeTeams();
    void UpdateGameTimer();
    void CheckWinConditions();
    void AssignRandomTeam(APlayerController* Player);
    void BroadcastGameState();
    void CleanupMatch();
    void SaveMatchResults();
    void UpdateMatchStatistics();
    int32 CalculatePlayerScore(const FPlayerScore& Score) const;
    void HandleTeamBalancing();
    void ProcessEndOfRound();
    void ResetPlayerStats();
    void SpawnSystemActors();
    void ConfigureSystemSettings();
};
