#include "AdvancedFPSGameMode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameState.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"

DEFINE_LOG_CATEGORY(LogAdvancedGameMode);

AAdvancedFPSGameMode::AAdvancedFPSGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    
    // Initialize default values
    CurrentGameType = EGameType::TeamDeathmatch;
    MatchState = EMatchState::WaitingToStart;
    TeamAScore = 0;
    TeamBScore = 0;
    ScoreLimit = 100;
    TimeLimit = 600.0f; // 10 minutes
    WarmupTime = 30.0f;
    PostMatchTime = 15.0f;
    
    CurrentMatchTime = 0.0f;
    MaxPlayers = 16;
    MaxTeamSize = 8;
    
    // Economy system defaults
    StartingMoney = 1000;
    KillReward = 300;
    AssistReward = 150;
    ObjectiveReward = 500;
    
    bUseLoadouts = true;
    bAllowSpectating = true;
    bEnableVoiceChat = true;
    
    RespawnDelay = 5.0f;
    
    // Initialize team info
    FTeamInfo TeamA;
    TeamA.TeamID = 0;
    TeamA.TeamName = TEXT("Team Alpha");
    TeamA.TeamColor = FLinearColor::Blue;
    TeamA.Score = 0;
    Teams.Add(TeamA);
    
    FTeamInfo TeamB;
    TeamB.TeamID = 1;
    TeamB.TeamName = TEXT("Team Bravo");
    TeamB.TeamColor = FLinearColor::Red;
    TeamB.Score = 0;
    Teams.Add(TeamB);
}

void AAdvancedFPSGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize match
    InitializeMatch();
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Advanced FPS Game Mode started. Game Type: %s"), 
           *UEnum::GetValueAsString(CurrentGameType));
}

void AAdvancedFPSGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    UpdateMatchTime(DeltaTime);
    CheckMatchConditions();
    UpdateObjectives(DeltaTime);
}

void AAdvancedFPSGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AAdvancedFPSGameMode, CurrentGameType);
    DOREPLIFETIME(AAdvancedFPSGameMode, MatchState);
    DOREPLIFETIME(AAdvancedFPSGameMode, TeamAScore);
    DOREPLIFETIME(AAdvancedFPSGameMode, TeamBScore);
    DOREPLIFETIME(AAdvancedFPSGameMode, CurrentMatchTime);
    DOREPLIFETIME(AAdvancedFPSGameMode, Teams);
    DOREPLIFETIME(AAdvancedFPSGameMode, ActiveObjectives);
    DOREPLIFETIME(AAdvancedFPSGameMode, MatchStatistics);
}

void AAdvancedFPSGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    if (!NewPlayer) return;
    
    // Create player info
    FPlayerInfo PlayerInfo;
    PlayerInfo.PlayerID = NewPlayer->GetUniqueID();
    PlayerInfo.PlayerName = NewPlayer->GetPlayerState<APlayerState>()->GetPlayerName();
    PlayerInfo.TeamID = AssignPlayerToTeam(NewPlayer);
    PlayerInfo.Money = StartingMoney;
    PlayerInfo.bIsAlive = false;
    PlayerInfo.bIsReady = false;
    
    ConnectedPlayers.Add(PlayerInfo.PlayerID, PlayerInfo);
    
    // Notify other players
    BroadcastPlayerJoined(PlayerInfo);
    
    // Send game state to new player
    SendGameStateToPlayer(NewPlayer);
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Player %s joined the game. Assigned to Team %d"), 
           *PlayerInfo.PlayerName, PlayerInfo.TeamID);
}

void AAdvancedFPSGameMode::Logout(AController* Exiting)
{
    if (APlayerController* PC = Cast<APlayerController>(Exiting))
    {
        int32 PlayerID = PC->GetUniqueID();
        if (ConnectedPlayers.Contains(PlayerID))
        {
            FPlayerInfo PlayerInfo = ConnectedPlayers[PlayerID];
            ConnectedPlayers.Remove(PlayerID);
            
            // Notify other players
            BroadcastPlayerLeft(PlayerInfo);
            
            UE_LOG(LogAdvancedGameMode, Log, TEXT("Player %s left the game"), *PlayerInfo.PlayerName);
        }
    }
    
    Super::Logout(Exiting);
}

void AAdvancedFPSGameMode::StartMatch()
{
    if (MatchState != EMatchState::WaitingToStart)
    {
        return;
    }
    
    MatchState = EMatchState::InProgress;
    CurrentMatchTime = 0.0f;
    
    // Initialize objectives based on game type
    InitializeObjectives();
    
    // Spawn all players
    for (auto& PlayerPair : ConnectedPlayers)
    {
        FPlayerInfo& PlayerInfo = PlayerPair.Value;
        if (APlayerController* PC = GetPlayerControllerByID(PlayerInfo.PlayerID))
        {
            SpawnPlayer(PC);
        }
    }
    
    // Broadcast match start
    MulticastMatchStarted();
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Match started. Game Type: %s"), 
           *UEnum::GetValueAsString(CurrentGameType));
}

void AAdvancedFPSGameMode::EndMatch(int32 WinningTeam)
{
    if (MatchState == EMatchState::Ended)
    {
        return;
    }
    
    MatchState = EMatchState::Ended;
    
    // Calculate final statistics
    CalculateFinalStatistics();
    
    // Broadcast match end
    MulticastMatchEnded(WinningTeam);
    
    // Start post-match timer
    GetWorld()->GetTimerManager().SetTimer(
        PostMatchTimer,
        this,
        &AAdvancedFPSGameMode::HandlePostMatch,
        PostMatchTime,
        false
    );
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Match ended. Winning team: %d"), WinningTeam);
}

void AAdvancedFPSGameMode::MulticastMatchStarted_Implementation()
{
    OnMatchStarted.Broadcast();
}

void AAdvancedFPSGameMode::MulticastMatchEnded_Implementation(int32 WinningTeam)
{
    OnMatchEnded.Broadcast(WinningTeam);
}

void AAdvancedFPSGameMode::HandlePlayerKill(APlayerController* Killer, APlayerController* Victim, AActor* DamageCauser)
{
    if (!Killer || !Victim) return;
    
    int32 KillerID = Killer->GetUniqueID();
    int32 VictimID = Victim->GetUniqueID();
    
    // Update statistics
    if (ConnectedPlayers.Contains(KillerID))
    {
        FPlayerInfo& KillerInfo = ConnectedPlayers[KillerID];
        KillerInfo.Kills++;
        KillerInfo.Money += KillReward;
        
        // Update match statistics
        MatchStatistics.TotalKills++;
        
        // Update team score based on game type
        UpdateTeamScore(KillerInfo.TeamID, GetKillScore());
    }
    
    if (ConnectedPlayers.Contains(VictimID))
    {
        FPlayerInfo& VictimInfo = ConnectedPlayers[VictimID];
        VictimInfo.Deaths++;
        VictimInfo.bIsAlive = false;
        
        // Schedule respawn
        ScheduleRespawn(Victim);
    }
    
    // Broadcast kill event
    BroadcastKillEvent(KillerID, VictimID);
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Player kill: %d killed %d"), KillerID, VictimID);
}

void AAdvancedFPSGameMode::HandleObjectiveCompleted(int32 ObjectiveID, int32 TeamID)
{
    if (!ActiveObjectives.Contains(ObjectiveID))
    {
        return;
    }
    
    FObjectiveInfo& Objective = ActiveObjectives[ObjectiveID];
    Objective.bIsCompleted = true;
    Objective.CompletingTeam = TeamID;
    
    // Award points and money
    UpdateTeamScore(TeamID, Objective.Points);
    
    // Reward players on the team
    for (auto& PlayerPair : ConnectedPlayers)
    {
        FPlayerInfo& PlayerInfo = PlayerPair.Value;
        if (PlayerInfo.TeamID == TeamID)
        {
            PlayerInfo.Money += ObjectiveReward;
            PlayerInfo.ObjectiveScore += Objective.Points;
        }
    }
    
    // Broadcast objective completion
    MulticastObjectiveCompleted(ObjectiveID, TeamID);
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Objective %d completed by team %d"), ObjectiveID, TeamID);
}

void AAdvancedFPSGameMode::MulticastObjectiveCompleted_Implementation(int32 ObjectiveID, int32 TeamID)
{
    OnObjectiveCompleted.Broadcast(ObjectiveID, TeamID);
}

int32 AAdvancedFPSGameMode::AssignPlayerToTeam(APlayerController* Player)
{
    if (!Player) return 0;
    
    // Count players on each team
    int32 TeamACounts[2] = {0, 0};
    
    for (const auto& PlayerPair : ConnectedPlayers)
    {
        if (PlayerPair.Value.TeamID < 2)
        {
            TeamACounts[PlayerPair.Value.TeamID]++;
        }
    }
    
    // Assign to team with fewer players
    return (TeamACounts[0] <= TeamACounts[1]) ? 0 : 1;
}

void AAdvancedFPSGameMode::SwitchPlayerTeam(APlayerController* Player, int32 NewTeam)
{
    if (!Player || NewTeam < 0 || NewTeam >= Teams.Num())
    {
        return;
    }
    
    int32 PlayerID = Player->GetUniqueID();
    if (ConnectedPlayers.Contains(PlayerID))
    {
        FPlayerInfo& PlayerInfo = ConnectedPlayers[PlayerID];
        int32 OldTeam = PlayerInfo.TeamID;
        PlayerInfo.TeamID = NewTeam;
        
        // Respawn player with new team
        if (PlayerInfo.bIsAlive)
        {
            KillPlayer(Player);
            ScheduleRespawn(Player);
        }
        
        BroadcastTeamSwitch(PlayerID, OldTeam, NewTeam);
        
        UE_LOG(LogAdvancedGameMode, Log, TEXT("Player %d switched from team %d to team %d"), 
               PlayerID, OldTeam, NewTeam);
    }
}

void AAdvancedFPSGameMode::SpawnPlayer(APlayerController* Player)
{
    if (!Player) return;
    
    int32 PlayerID = Player->GetUniqueID();
    if (!ConnectedPlayers.Contains(PlayerID))
    {
        return;
    }
    
    FPlayerInfo& PlayerInfo = ConnectedPlayers[PlayerID];
    
    // Find spawn point
    FVector SpawnLocation;
    FRotator SpawnRotation;
    
    if (FindSpawnPoint(PlayerInfo.TeamID, SpawnLocation, SpawnRotation))
    {
        // Spawn player character
        if (APawn* NewPawn = SpawnPlayerCharacter(Player, SpawnLocation, SpawnRotation))
        {
            Player->Possess(NewPawn);
            PlayerInfo.bIsAlive = true;
            
            // Apply loadout
            if (bUseLoadouts)
            {
                ApplyPlayerLoadout(Player, PlayerInfo.SelectedLoadout);
            }
            
            UE_LOG(LogAdvancedGameMode, Log, TEXT("Player %d spawned at location %s"), 
                   PlayerID, *SpawnLocation.ToString());
        }
    }
}

void AAdvancedFPSGameMode::ScheduleRespawn(APlayerController* Player)
{
    if (!Player) return;
    
    int32 PlayerID = Player->GetUniqueID();
    
    // Set respawn timer
    FTimerHandle RespawnTimer;
    FTimerDelegate RespawnDelegate;
    RespawnDelegate.BindUFunction(this, FName("RespawnPlayer"), PlayerID);
    
    GetWorld()->GetTimerManager().SetTimer(
        RespawnTimer,
        RespawnDelegate,
        RespawnDelay,
        false
    );
    
    // Store timer handle for potential cancellation
    PendingRespawns.Add(PlayerID, RespawnTimer);
}

void AAdvancedFPSGameMode::RespawnPlayer(int32 PlayerID)
{
    APlayerController* Player = GetPlayerControllerByID(PlayerID);
    if (Player && MatchState == EMatchState::InProgress)
    {
        SpawnPlayer(Player);
    }
    
    PendingRespawns.Remove(PlayerID);
}

void AAdvancedFPSGameMode::UpdateTeamScore(int32 TeamID, int32 Points)
{
    if (TeamID < 0 || TeamID >= Teams.Num())
    {
        return;
    }
    
    Teams[TeamID].Score += Points;
    
    // Update legacy score variables for backwards compatibility
    if (TeamID == 0)
    {
        TeamAScore = Teams[0].Score;
    }
    else if (TeamID == 1)
    {
        TeamBScore = Teams[1].Score;
    }
    
    // Broadcast score update
    MulticastScoreUpdated(TeamID, Teams[TeamID].Score);
}

void AAdvancedFPSGameMode::MulticastScoreUpdated_Implementation(int32 TeamID, int32 NewScore)
{
    OnScoreUpdated.Broadcast(TeamID, NewScore);
}

void AAdvancedFPSGameMode::UpdateMatchTime(float DeltaTime)
{
    if (MatchState == EMatchState::InProgress)
    {
        CurrentMatchTime += DeltaTime;
    }
}

void AAdvancedFPSGameMode::CheckMatchConditions()
{
    if (MatchState != EMatchState::InProgress)
    {
        return;
    }
    
    // Check time limit
    if (TimeLimit > 0 && CurrentMatchTime >= TimeLimit)
    {
        int32 WinningTeam = GetLeadingTeam();
        EndMatch(WinningTeam);
        return;
    }
    
    // Check score limit
    if (ScoreLimit > 0)
    {
        for (int32 i = 0; i < Teams.Num(); i++)
        {
            if (Teams[i].Score >= ScoreLimit)
            {
                EndMatch(i);
                return;
            }
        }
    }
    
    // Game type specific conditions
    CheckGameTypeSpecificConditions();
}

void AAdvancedFPSGameMode::CheckGameTypeSpecificConditions()
{
    switch (CurrentGameType)
    {
        case EGameType::TeamDeathmatch:
            // Already handled by score limit
            break;
            
        case EGameType::Domination:
            CheckDominationConditions();
            break;
            
        case EGameType::SearchAndDestroy:
            CheckSearchAndDestroyConditions();
            break;
            
        case EGameType::CaptureTheFlag:
            CheckCaptureTheFlagConditions();
            break;
            
        case EGameType::BattleRoyale:
            CheckBattleRoyaleConditions();
            break;
            
        default:
            break;
    }
}

void AAdvancedFPSGameMode::CheckDominationConditions()
{
    // Check if any team controls all objectives
    int32 TeamControlCounts[2] = {0, 0};
    
    for (const auto& ObjectivePair : ActiveObjectives)
    {
        const FObjectiveInfo& Objective = ObjectivePair.Value;
        if (Objective.ControllingTeam >= 0 && Objective.ControllingTeam < 2)
        {
            TeamControlCounts[Objective.ControllingTeam]++;
        }
    }
    
    // Check for total control
    for (int32 i = 0; i < 2; i++)
    {
        if (TeamControlCounts[i] == ActiveObjectives.Num() && ActiveObjectives.Num() > 0)
        {
            EndMatch(i);
            return;
        }
    }
}

void AAdvancedFPSGameMode::CheckSearchAndDestroyConditions()
{
    // Check if all objectives are destroyed or all players eliminated
    bool AllObjectivesDestroyed = true;
    for (const auto& ObjectivePair : ActiveObjectives)
    {
        if (!ObjectivePair.Value.bIsCompleted)
        {
            AllObjectivesDestroyed = false;
            break;
        }
    }
    
    if (AllObjectivesDestroyed)
    {
        EndMatch(0); // Attacking team wins
    }
}

void AAdvancedFPSGameMode::CheckCaptureTheFlagConditions()
{
    // Check flag capture counts
    // This would be implemented based on specific CTF mechanics
}

void AAdvancedFPSGameMode::CheckBattleRoyaleConditions()
{
    // Count alive players
    int32 AlivePlayers = 0;
    int32 LastAliveTeam = -1;
    
    for (const auto& PlayerPair : ConnectedPlayers)
    {
        if (PlayerPair.Value.bIsAlive)
        {
            AlivePlayers++;
            LastAliveTeam = PlayerPair.Value.TeamID;
        }
    }
    
    if (AlivePlayers <= 1)
    {
        EndMatch(LastAliveTeam);
    }
}

int32 AAdvancedFPSGameMode::GetLeadingTeam() const
{
    int32 LeadingTeam = 0;
    int32 HighestScore = Teams[0].Score;
    
    for (int32 i = 1; i < Teams.Num(); i++)
    {
        if (Teams[i].Score > HighestScore)
        {
            HighestScore = Teams[i].Score;
            LeadingTeam = i;
        }
    }
    
    return LeadingTeam;
}

void AAdvancedFPSGameMode::InitializeMatch()
{
    // Reset scores
    for (FTeamInfo& Team : Teams)
    {
        Team.Score = 0;
    }
    
    TeamAScore = 0;
    TeamBScore = 0;
    
    // Initialize match statistics
    MatchStatistics.MatchStartTime = GetWorld()->GetTimeSeconds();
    MatchStatistics.TotalKills = 0;
    MatchStatistics.TotalDeaths = 0;
    MatchStatistics.TotalDamageDealt = 0;
    
    MatchState = EMatchState::WaitingToStart;
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Match initialized"));
}

void AAdvancedFPSGameMode::InitializeObjectives()
{
    ActiveObjectives.Empty();
    
    switch (CurrentGameType)
    {
        case EGameType::Domination:
            CreateDominationObjectives();
            break;
            
        case EGameType::SearchAndDestroy:
            CreateSearchAndDestroyObjectives();
            break;
            
        case EGameType::CaptureTheFlag:
            CreateCaptureTheFlagObjectives();
            break;
            
        default:
            break;
    }
}

void AAdvancedFPSGameMode::CreateDominationObjectives()
{
    // Create control points for domination
    for (int32 i = 0; i < 3; i++)
    {
        FObjectiveInfo Objective;
        Objective.ObjectiveID = i;
        Objective.ObjectiveType = EObjectiveType::ControlPoint;
        Objective.ObjectiveName = FString::Printf(TEXT("Control Point %c"), 'A' + i);
        Objective.Points = 10;
        Objective.ControllingTeam = -1;
        Objective.bIsCompleted = false;
        
        ActiveObjectives.Add(i, Objective);
    }
}

void AAdvancedFPSGameMode::CreateSearchAndDestroyObjectives()
{
    // Create bomb sites for search and destroy
    for (int32 i = 0; i < 2; i++)
    {
        FObjectiveInfo Objective;
        Objective.ObjectiveID = i;
        Objective.ObjectiveType = EObjectiveType::DestroyTarget;
        Objective.ObjectiveName = FString::Printf(TEXT("Bomb Site %c"), 'A' + i);
        Objective.Points = 100;
        Objective.ControllingTeam = -1;
        Objective.bIsCompleted = false;
        
        ActiveObjectives.Add(i, Objective);
    }
}

void AAdvancedFPSGameMode::CreateCaptureTheFlagObjectives()
{
    // Create flag objectives
    for (int32 i = 0; i < 2; i++)
    {
        FObjectiveInfo Objective;
        Objective.ObjectiveID = i;
        Objective.ObjectiveType = EObjectiveType::CaptureFlag;
        Objective.ObjectiveName = FString::Printf(TEXT("Team %d Flag"), i + 1);
        Objective.Points = 50;
        Objective.ControllingTeam = i;
        Objective.bIsCompleted = false;
        
        ActiveObjectives.Add(i, Objective);
    }
}

void AAdvancedFPSGameMode::UpdateObjectives(float DeltaTime)
{
    for (auto& ObjectivePair : ActiveObjectives)
    {
        FObjectiveInfo& Objective = ObjectivePair.Value;
        
        // Update objective-specific logic
        if (Objective.ObjectiveType == EObjectiveType::ControlPoint)
        {
            UpdateControlPointObjective(Objective, DeltaTime);
        }
    }
}

void AAdvancedFPSGameMode::UpdateControlPointObjective(FObjectiveInfo& Objective, float DeltaTime)
{
    // This would check for players in the control point area
    // and update control progress
}

bool AAdvancedFPSGameMode::FindSpawnPoint(int32 TeamID, FVector& OutLocation, FRotator& OutRotation)
{
    // Find appropriate spawn points for the team
    TArray<AActor*> SpawnPoints;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), SpawnPoints);
    
    // Filter spawn points by team
    TArray<AActor*> TeamSpawnPoints;
    for (AActor* SpawnPoint : SpawnPoints)
    {
        // Check if spawn point is for this team (would need team-specific spawn point class)
        TeamSpawnPoints.Add(SpawnPoint);
    }
    
    if (TeamSpawnPoints.Num() > 0)
    {
        // Choose random spawn point
        AActor* ChosenSpawn = TeamSpawnPoints[FMath::RandRange(0, TeamSpawnPoints.Num() - 1)];
        OutLocation = ChosenSpawn->GetActorLocation();
        OutRotation = ChosenSpawn->GetActorRotation();
        return true;
    }
    
    // Fallback to world origin
    OutLocation = FVector::ZeroVector;
    OutRotation = FRotator::ZeroRotator;
    return false;
}

APawn* AAdvancedFPSGameMode::SpawnPlayerCharacter(APlayerController* Player, const FVector& Location, const FRotator& Rotation)
{
    if (!Player || !DefaultPawnClass)
    {
        return nullptr;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Player;
    SpawnParams.Instigator = nullptr;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    return GetWorld()->SpawnActor<APawn>(DefaultPawnClass, Location, Rotation, SpawnParams);
}

void AAdvancedFPSGameMode::ApplyPlayerLoadout(APlayerController* Player, const FPlayerLoadout& Loadout)
{
    // This would equip the player with their selected weapons and equipment
    if (APawn* PlayerPawn = Player->GetPawn())
    {
        // Implementation would depend on your weapon/equipment system
    }
}

void AAdvancedFPSGameMode::KillPlayer(APlayerController* Player)
{
    if (APawn* PlayerPawn = Player->GetPawn())
    {
        PlayerPawn->Destroy();
    }
}

APlayerController* AAdvancedFPSGameMode::GetPlayerControllerByID(int32 PlayerID)
{
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (APlayerController* PC = Iterator->Get())
        {
            if (PC->GetUniqueID() == PlayerID)
            {
                return PC;
            }
        }
    }
    return nullptr;
}

int32 AAdvancedFPSGameMode::GetKillScore() const
{
    switch (CurrentGameType)
    {
        case EGameType::TeamDeathmatch:
        case EGameType::FreeForAll:
            return 1;
        case EGameType::SearchAndDestroy:
            return 2;
        default:
            return 1;
    }
}

void AAdvancedFPSGameMode::CalculateFinalStatistics()
{
    MatchStatistics.MatchEndTime = GetWorld()->GetTimeSeconds();
    MatchStatistics.MatchDuration = MatchStatistics.MatchEndTime - MatchStatistics.MatchStartTime;
    
    // Calculate player statistics
    for (auto& PlayerPair : ConnectedPlayers)
    {
        FPlayerInfo& PlayerInfo = PlayerPair.Value;
        
        // Calculate K/D ratio
        PlayerInfo.KillDeathRatio = (PlayerInfo.Deaths > 0) ? 
            (float)PlayerInfo.Kills / (float)PlayerInfo.Deaths : (float)PlayerInfo.Kills;
            
        // Calculate accuracy
        if (PlayerInfo.ShotsFired > 0)
        {
            PlayerInfo.Accuracy = (float)PlayerInfo.ShotsHit / (float)PlayerInfo.ShotsFired;
        }
    }
    
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Final statistics calculated. Match duration: %.1f seconds"), 
           MatchStatistics.MatchDuration);
}

void AAdvancedFPSGameMode::HandlePostMatch()
{
    // Handle post-match logic (return to lobby, load next map, etc.)
    UE_LOG(LogAdvancedGameMode, Log, TEXT("Post-match period ended"));
}

void AAdvancedFPSGameMode::BroadcastPlayerJoined(const FPlayerInfo& PlayerInfo)
{
    MulticastPlayerJoined(PlayerInfo);
}

void AAdvancedFPSGameMode::BroadcastPlayerLeft(const FPlayerInfo& PlayerInfo)
{
    MulticastPlayerLeft(PlayerInfo);
}

void AAdvancedFPSGameMode::BroadcastKillEvent(int32 KillerID, int32 VictimID)
{
    MulticastKillEvent(KillerID, VictimID);
}

void AAdvancedFPSGameMode::BroadcastTeamSwitch(int32 PlayerID, int32 OldTeam, int32 NewTeam)
{
    MulticastTeamSwitch(PlayerID, OldTeam, NewTeam);
}

void AAdvancedFPSGameMode::MulticastPlayerJoined_Implementation(const FPlayerInfo& PlayerInfo)
{
    OnPlayerJoined.Broadcast(PlayerInfo);
}

void AAdvancedFPSGameMode::MulticastPlayerLeft_Implementation(const FPlayerInfo& PlayerInfo)
{
    OnPlayerLeft.Broadcast(PlayerInfo);
}

void AAdvancedFPSGameMode::MulticastKillEvent_Implementation(int32 KillerID, int32 VictimID)
{
    OnPlayerKilled.Broadcast(KillerID, VictimID);
}

void AAdvancedFPSGameMode::MulticastTeamSwitch_Implementation(int32 PlayerID, int32 OldTeam, int32 NewTeam)
{
    OnTeamSwitch.Broadcast(PlayerID, OldTeam, NewTeam);
}

void AAdvancedFPSGameMode::SendGameStateToPlayer(APlayerController* Player)
{
    if (!Player) return;
    
    // Send current match state, scores, time, etc.
    ClientReceiveGameState(Player, MatchState, TeamAScore, TeamBScore, CurrentMatchTime);
}

void AAdvancedFPSGameMode::ClientReceiveGameState_Implementation(APlayerController* Player, EMatchState State, int32 ScoreA, int32 ScoreB, float MatchTime)
{
    // Client-side game state update
}

FGameModeStats AAdvancedFPSGameMode::GetMatchStatistics() const
{
    return MatchStatistics;
}

TArray<FPlayerInfo> AAdvancedFPSGameMode::GetPlayerList() const
{
    TArray<FPlayerInfo> Players;
    for (const auto& PlayerPair : ConnectedPlayers)
    {
        Players.Add(PlayerPair.Value);
    }
    return Players;
}

TArray<FTeamInfo> AAdvancedFPSGameMode::GetTeamInfo() const
{
    return Teams;
}
