#include "FPSNetworkManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystemUtils.h"

// Session constants
const FName UFPSNetworkManager::SESSION_NAME = TEXT("FPSGameSession");
const FName UFPSNetworkManager::SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");
const FName UFPSNetworkManager::MAX_PLAYERS_SETTINGS_KEY = TEXT("MaxPlayers");
const FName UFPSNetworkManager::GAME_MODE_SETTINGS_KEY = TEXT("GameMode");
const FName UFPSNetworkManager::PASSWORD_SETTINGS_KEY = TEXT("Password");

UFPSNetworkManager::UFPSNetworkManager()
{
	OnlineSubsystem = nullptr;
	SessionInterface = nullptr;

	// Bind delegates
	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UFPSNetworkManager::OnCreateSessionComplete);
	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UFPSNetworkManager::OnDestroySessionComplete);
	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UFPSNetworkManager::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UFPSNetworkManager::OnJoinSessionComplete);
	OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &UFPSNetworkManager::OnStartOnlineGameComplete);
}

void UFPSNetworkManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Get online subsystem
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
		
		if (SessionInterface.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Network Manager: Online subsystem initialized successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Network Manager: Failed to get session interface"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Network Manager: Online subsystem not found"));
	}
}

void UFPSNetworkManager::Deinitialize()
{
	// Clean up delegate handles
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
	}

	Super::Deinitialize();
}

void UFPSNetworkManager::CreateSession(const FServerInfo& ServerInfo, const FString& Password)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Network Manager: Session interface is invalid"));
		OnSessionCreated.Broadcast(false);
		return;
	}

	// Store server info
	CurrentServerInfo = ServerInfo;

	// Check if session exists and destroy it first
	auto ExistingSession = SessionInterface->GetNamedSession(SESSION_NAME);
	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(SESSION_NAME);
	}

	// Setup session settings
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = (OnlineSubsystem->GetSubsystemName() == "NULL");
	SessionSettings->bUsesPresence = !SessionSettings->bIsLANMatch;
	SessionSettings->NumPublicConnections = ServerInfo.MaxPlayers;
	SessionSettings->NumPrivateConnections = 0;
	SessionSettings->bAllowInvites = true;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bAllowJoinViaPresence = !SessionSettings->bIsLANMatch;
	SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;
	SessionSettings->bUsesStats = true;
	SessionSettings->bAntiCheatProtected = true;

	// Set custom settings
	SessionSettings->Set(SERVER_NAME_SETTINGS_KEY, ServerInfo.ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings->Set(GAME_MODE_SETTINGS_KEY, ServerInfo.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings->Set(MAX_PLAYERS_SETTINGS_KEY, ServerInfo.MaxPlayers, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	if (!Password.IsEmpty())
	{
		SessionSettings->Set(PASSWORD_SETTINGS_KEY, Password, EOnlineDataAdvertisementType::DontAdvertise);
	}

	// Bind delegate
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	// Create the session
	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetLocalPlayerByIndex(0);
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), SESSION_NAME, *SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		OnSessionCreated.Broadcast(false);
	}
}

void UFPSNetworkManager::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		OnSessionDestroyed.Broadcast(false);
		return;
	}

	OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(SESSION_NAME))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		OnSessionDestroyed.Broadcast(false);
	}
}

void UFPSNetworkManager::FindSessions(int32 MaxResults)
{
	if (!SessionInterface.IsValid())
	{
		OnSessionSearchComplete.Broadcast(false, TArray<FServerInfo>());
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = MaxResults;
	SessionSearch->bIsLanQuery = (OnlineSubsystem->GetSubsystemName() == "NULL");
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, !SessionSearch->bIsLanQuery, EOnlineComparisonOp::Equals);

	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetLocalPlayerByIndex(0);
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		OnSessionSearchComplete.Broadcast(false, TArray<FServerInfo>());
	}
}

void UFPSNetworkManager::JoinSession(int32 SessionIndex, const FString& Password)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid() || SessionIndex >= SessionSearch->SearchResults.Num())
	{
		OnSessionJoined.Broadcast(false);
		return;
	}

	JoinSessionBySessionResult(SessionSearch->SearchResults[SessionIndex]);
}

void UFPSNetworkManager::JoinSessionBySessionResult(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		OnSessionJoined.Broadcast(false);
		return;
	}

	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetLocalPlayerByIndex(0);
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), SESSION_NAME, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		OnSessionJoined.Broadcast(false);
	}
}

void UFPSNetworkManager::StartSession()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	OnStartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);
	SessionInterface->StartSession(SESSION_NAME);
}

void UFPSNetworkManager::EndSession()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionInterface->EndSession(SESSION_NAME);
}

bool UFPSNetworkManager::IsHost() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer;
	}
	return false;
}

bool UFPSNetworkManager::IsInSession() const
{
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	return SessionInterface->GetNamedSession(SESSION_NAME) != nullptr;
}

FString UFPSNetworkManager::GetPlayerName() const
{
	if (const ULocalPlayer* LocalPlayer = GetGameInstance()->GetLocalPlayerByIndex(0))
	{
		return LocalPlayer->GetNickname();
	}
	return TEXT("Player");
}

int32 UFPSNetworkManager::GetCurrentPlayerCount() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetNumPlayerControllers();
	}
	return 0;
}

int32 UFPSNetworkManager::GetMaxPlayerCount() const
{
	return CurrentServerInfo.MaxPlayers;
}

float UFPSNetworkManager::GetNetworkLatency() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APlayerState* PlayerState = PC->GetPlayerState<APlayerState>())
			{
				return PlayerState->GetPingInMilliseconds();
			}
		}
	}
	return 0.0f;
}

bool UFPSNetworkManager::ValidatePlayerAction(AActor* Player, const FString& ActionName, float Value)
{
	if (!Player || !IsHost())
	{
		return true; // Only validate on server
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float* LastActionTime = PlayerLastActionTimes.Find(Player);

	if (ActionName == TEXT("Fire"))
	{
		if (LastActionTime && (CurrentTime - *LastActionTime) < (1.0f / MaxFireRate))
		{
			ReportSuspiciousActivity(Player, TEXT("Fire rate too high"));
			return false;
		}
		PlayerLastActionTimes.Add(Player, CurrentTime);
	}
	else if (ActionName == TEXT("Movement"))
	{
		if (Value > MaxAllowedSpeed)
		{
			ReportSuspiciousActivity(Player, TEXT("Movement speed too high"));
			return false;
		}
	}

	return true;
}

void UFPSNetworkManager::ReportSuspiciousActivity(AActor* Player, const FString& Reason)
{
	if (!Player)
	{
		return;
	}

	int32* SuspiciousCount = SuspiciousActivityCounts.Find(Player);
	int32 NewCount = SuspiciousCount ? (*SuspiciousCount + 1) : 1;
	SuspiciousActivityCounts.Add(Player, NewCount);

	UE_LOG(LogTemp, Warning, TEXT("Suspicious activity from %s: %s (Count: %d)"), 
		*Player->GetName(), *Reason, NewCount);

	if (NewCount >= MaxSuspiciousActivities)
	{
		// Kick player or take other action
		if (APlayerController* PC = Cast<APlayerController>(Player))
		{
			PC->ClientTravel(TEXT(""), TRAVEL_Absolute);
		}
	}
}

// Session callbacks
void UFPSNetworkManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}

	if (bWasSuccessful)
	{
		StartSession();
	}

	OnSessionCreated.Broadcast(bWasSuccessful);
}

void UFPSNetworkManager::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
	}

	OnSessionDestroyed.Broadcast(bWasSuccessful);
}

void UFPSNetworkManager::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
	}

	TArray<FServerInfo> ServerList;

	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			FServerInfo ServerInfo;
			
			// Extract server information
			SearchResult.Session.SessionSettings.Get(SERVER_NAME_SETTINGS_KEY, ServerInfo.ServerName);
			SearchResult.Session.SessionSettings.Get(GAME_MODE_SETTINGS_KEY, ServerInfo.GameMode);
			SearchResult.Session.SessionSettings.Get(MAX_PLAYERS_SETTINGS_KEY, ServerInfo.MaxPlayers);
			
			ServerInfo.CurrentPlayers = ServerInfo.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
			ServerInfo.Ping = SearchResult.PingInMs;
			ServerInfo.bIsPasswordProtected = SearchResult.Session.SessionSettings.Settings.Contains(PASSWORD_SETTINGS_KEY);

			ServerList.Add(ServerInfo);
		}
	}

	OnSessionSearchComplete.Broadcast(bWasSuccessful, ServerList);
}

void UFPSNetworkManager::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	}

	bool bSuccessful = (Result == EOnJoinSessionCompleteResult::Success);

	if (bSuccessful)
	{
		// Get the connect string and travel to the session
		FString TravelURL;
		if (SessionInterface->GetResolvedConnectString(SESSION_NAME, TravelURL))
		{
			if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
			{
				PlayerController->ClientTravel(TravelURL, TRAVEL_Absolute);
			}
		}
		else
		{
			bSuccessful = false;
		}
	}

	OnSessionJoined.Broadcast(bSuccessful);
}

void UFPSNetworkManager::OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("Session started: %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failed"));
}

// Network validation methods
bool UFPSNetworkManager::ValidateMovement(AActor* Player, const FVector& NewLocation, const FVector& Velocity)
{
	if (!Player || !IsHost())
	{
		return true;
	}

	// Check for teleportation (sudden position change)
	FVector CurrentLocation = Player->GetActorLocation();
	float Distance = FVector::Dist(CurrentLocation, NewLocation);
	float MaxDistance = MaxAllowedSpeed * GetWorld()->GetDeltaSeconds() * 2.0f; // Allow some buffer

	if (Distance > MaxDistance)
	{
		ReportSuspiciousActivity(Player, FString::Printf(TEXT("Teleportation detected: %f units"), Distance));
		return false;
	}

	// Check velocity magnitude
	if (Velocity.Size() > MaxAllowedSpeed)
	{
		ReportSuspiciousActivity(Player, FString::Printf(TEXT("Invalid velocity: %f"), Velocity.Size()));
		return false;
	}

	return true;
}

bool UFPSNetworkManager::ValidateWeaponFire(AActor* Player, const FVector& FireLocation, const FVector& FireDirection)
{
	if (!Player || !IsHost())
	{
		return true;
	}

	// Validate fire location is near player
	FVector PlayerLocation = Player->GetActorLocation();
	float Distance = FVector::Dist(PlayerLocation, FireLocation);
	
	if (Distance > 200.0f) // Allow some tolerance for weapon length
	{
		ReportSuspiciousActivity(Player, TEXT("Weapon fire from invalid location"));
		return false;
	}

	// Validate fire direction is normalized
	if (!FireDirection.IsNormalized())
	{
		ReportSuspiciousActivity(Player, TEXT("Invalid fire direction"));
		return false;
	}

	return ValidatePlayerAction(Player, TEXT("Fire"), 1.0f);
}

bool UFPSNetworkManager::ValidateDamage(AActor* Attacker, AActor* Victim, float Damage)
{
	if (!Attacker || !Victim || !IsHost())
	{
		return true;
	}

	// Check if damage is reasonable
	if (Damage <= 0.0f || Damage > 1000.0f)
	{
		ReportSuspiciousActivity(Attacker, FString::Printf(TEXT("Invalid damage amount: %f"), Damage));
		return false;
	}

	// Check distance between attacker and victim
	float Distance = FVector::Dist(Attacker->GetActorLocation(), Victim->GetActorLocation());
	if (Distance > 5000.0f) // Max weapon range
	{
		ReportSuspiciousActivity(Attacker, TEXT("Damage from excessive distance"));
		return false;
	}

	return true;
}
