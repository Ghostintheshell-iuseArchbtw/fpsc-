#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FPSNetworkManager.generated.h"

USTRUCT(BlueprintType)
struct FServerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString ServerName = TEXT("FPS Server");

	UPROPERTY(BlueprintReadWrite)
	FString MapName = TEXT("DefaultMap");

	UPROPERTY(BlueprintReadWrite)
	int32 MaxPlayers = 16;

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadWrite)
	float Ping = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bIsPasswordProtected = false;

	UPROPERTY(BlueprintReadWrite)
	FString GameMode = TEXT("Team Deathmatch");

	UPROPERTY(BlueprintReadWrite)
	FString Region = TEXT("US-East");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreated, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionDestroyed, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoined, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionSearch, bool, bWasSuccessful, const TArray<FServerInfo>&, ServerList);

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UFPSNetworkManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFPSNetworkManager();

	// Subsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Session management
	UFUNCTION(BlueprintCallable, Category = "Networking")
	void CreateSession(const FServerInfo& ServerInfo, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Networking")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category = "Networking")
	void FindSessions(int32 MaxResults = 50);

	UFUNCTION(BlueprintCallable, Category = "Networking")
	void JoinSession(int32 SessionIndex, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Networking")
	void JoinSessionBySessionResult(const FOnlineSessionSearchResult& SessionResult);

	UFUNCTION(BlueprintCallable, Category = "Networking")
	void StartSession();

	UFUNCTION(BlueprintCallable, Category = "Networking")
	void EndSession();

	// Network utilities
	UFUNCTION(BlueprintCallable, Category = "Networking")
	bool IsHost() const;

	UFUNCTION(BlueprintCallable, Category = "Networking")
	bool IsInSession() const;

	UFUNCTION(BlueprintCallable, Category = "Networking")
	FString GetPlayerName() const;

	UFUNCTION(BlueprintCallable, Category = "Networking")
	int32 GetCurrentPlayerCount() const;

	UFUNCTION(BlueprintCallable, Category = "Networking")
	int32 GetMaxPlayerCount() const;

	UFUNCTION(BlueprintCallable, Category = "Networking")
	float GetNetworkLatency() const;

	// Anti-cheat and validation
	UFUNCTION(BlueprintCallable, Category = "Networking")
	bool ValidatePlayerAction(AActor* Player, const FString& ActionName, float Value);

	UFUNCTION(BlueprintCallable, Category = "Networking")
	void ReportSuspiciousActivity(AActor* Player, const FString& Reason);

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Networking")
	FOnSessionCreated OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "Networking")
	FOnSessionDestroyed OnSessionDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Networking")
	FOnSessionJoined OnSessionJoined;

	UPROPERTY(BlueprintAssignable, Category = "Networking")
	FOnSessionSearch OnSessionSearchComplete;

protected:
	// Session interface callbacks
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful);

	// Network validation
	bool ValidateMovement(AActor* Player, const FVector& NewLocation, const FVector& Velocity);
	bool ValidateWeaponFire(AActor* Player, const FVector& FireLocation, const FVector& FireDirection);
	bool ValidateDamage(AActor* Attacker, AActor* Victim, float Damage);

private:
	// Online subsystem
	IOnlineSubsystem* OnlineSubsystem;
	IOnlineSessionPtr SessionInterface;

	// Session search
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// Current session info
	FServerInfo CurrentServerInfo;

	// Delegate handles
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;

	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;

	// Anti-cheat tracking
	TMap<AActor*, float> PlayerLastActionTimes;
	TMap<AActor*, int32> SuspiciousActivityCounts;

	// Network settings
	UPROPERTY(EditDefaultsOnly, Category = "Networking")
	float MaxAllowedSpeed = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Networking")
	float MaxAllowedAcceleration = 2048.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Networking")
	float MaxFireRate = 20.0f; // shots per second

	UPROPERTY(EditDefaultsOnly, Category = "Networking")
	int32 MaxSuspiciousActivities = 5;

	// Session constants
	static const FName SESSION_NAME;
	static const FName SERVER_NAME_SETTINGS_KEY;
	static const FName MAX_PLAYERS_SETTINGS_KEY;
	static const FName GAME_MODE_SETTINGS_KEY;
	static const FName PASSWORD_SETTINGS_KEY;
};
