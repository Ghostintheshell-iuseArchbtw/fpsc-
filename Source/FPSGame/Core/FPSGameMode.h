#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSGameMode.generated.h"

UCLASS()
class FPSGAME_API AFPSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSGameMode();

protected:
	virtual void BeginPlay() override;

	// Game settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float RoundDuration = 600.0f; // 10 minutes

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	int32 MaxPlayers = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	bool bFriendlyFire = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float RespawnDelay = 5.0f;

	// Environmental settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	float GravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	float WindStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	FVector WindDirection = FVector(1, 0, 0);

public:
	// Game state functions
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void StartRound();

	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void EndRound();

	UFUNCTION(BlueprintPure, Category = "Game Mode")
	float GetRemainingTime() const;

	// Environmental functions
	UFUNCTION(BlueprintPure, Category = "Environment")
	FVector GetWindEffect() const { return WindDirection * WindStrength; }

	UFUNCTION(BlueprintCallable, Category = "Environment")
	void SetWindEffect(FVector Direction, float Strength);
};
