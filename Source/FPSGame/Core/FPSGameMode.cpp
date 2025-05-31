#include "FPSGameMode.h"
#include "../Characters/FPSCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

AFPSGameMode::AFPSGameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = AFPSCharacter::StaticClass();

	// Set default game settings
	bUseSeamlessTravel = true;
}

void AFPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Initialize game settings
	if (GetWorld())
	{
		// Set gravity
		GetWorld()->GetGravityZ() *= GravityScale;
	}

	// Start the round
	StartRound();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("FPS Game Mode started!"));
	}
}

void AFPSGameMode::StartRound()
{
	// Implementation for starting a new round
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, TEXT("Round started!"));
	}

	// Set round timer
	FTimerHandle RoundTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AFPSGameMode::EndRound, RoundDuration, false);
}

void AFPSGameMode::EndRound()
{
	// Implementation for ending the round
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Round ended!"));
	}

	// You can add logic here for:
	// - Calculating scores
	// - Showing end-game UI
	// - Starting a new round
	// - Returning to lobby
}

float AFPSGameMode::GetRemainingTime() const
{
	// Return remaining time in the round
	// This is a simplified implementation
	return RoundDuration; // You'd calculate this based on when the round started
}

void AFPSGameMode::SetWindEffect(FVector Direction, float Strength)
{
	WindDirection = Direction.GetSafeNormal();
	WindStrength = Strength;

	if (GEngine)
	{
		FString WindMessage = FString::Printf(TEXT("Wind set: Direction=(%s), Strength=%.1f"), 
			*WindDirection.ToString(), WindStrength);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, WindMessage);
	}
}
