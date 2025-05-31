#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "AIController.h"
#include "Perception/PawnSensingComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "../Components/DamageComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Weapons/FPSWeapon.h"
#include "FPSAICharacter.generated.h"

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle,
	Patrol,
	Alert,
	Combat,
	Searching,
	Investigating,
	Retreating,
	Dead
};

UENUM(BlueprintType)
enum class EAICombatStyle : uint8
{
	Aggressive,
	Defensive,
	Tactical,
	Sniper,
	Rusher
};

USTRUCT(BlueprintType)
struct FAIStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Accuracy = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReactionTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SightRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HearingRange = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CombatRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PatrolSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AlertSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CombatSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AggressionLevel = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float IntelligenceLevel = 0.7f;
};

UCLASS()
class FPSGAME_API AFPSAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSAICharacter();

protected:
	virtual void BeginPlay() override;

	// AI Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPawnSensingComponent* PawnSensingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDamageComponent* DamageComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInventoryComponent* InventoryComponent;

	// Current weapon
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AFPSWeapon* CurrentWeapon;

	// AI Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FAIStats AIStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EAICombatStyle CombatStyle;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EAIState CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	AActor* CurrentTarget;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	FVector LastKnownTargetLocation;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float LastTargetSeenTime;

	// Patrol system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FVector> PatrolPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Patrol")
	int32 CurrentPatrolIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float PatrolWaitTime = 3.0f;

	// Combat variables
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float LastFireTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MinFireDelay = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxFireDelay = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FVector CoverLocation;

	// AI behavior functions
	UFUNCTION()
	void OnSeePlayer(APawn* Pawn);

	UFUNCTION()
	void OnHearNoise(APawn* Instigator, const FVector& Location, float Volume);

	void SetAIState(EAIState NewState);
	void UpdateAI(float DeltaTime);

	// State functions
	void HandleIdleState(float DeltaTime);
	void HandlePatrolState(float DeltaTime);
	void HandleAlertState(float DeltaTime);
	void HandleCombatState(float DeltaTime);
	void HandleSearchingState(float DeltaTime);
	void HandleInvestigatingState(float DeltaTime);
	void HandleRetreatingState(float DeltaTime);

	// Combat functions
	void StartCombat(AActor* Target);
	void UpdateCombat(float DeltaTime);
	bool CanSeeTarget(AActor* Target);
	bool IsInWeaponRange(AActor* Target);
	void AimAtTarget(AActor* Target);
	void FireAtTarget();
	void FindCover();
	void MoveToCover();

	// Movement functions
	void MoveToLocation(FVector Location);
	void StopMovement();
	void UpdateMovementSpeed();

	// Patrol functions
	void StartPatrol();
	void MoveToNextPatrolPoint();
	void HandlePatrolWait();

	// Investigation functions
	void InvestigateLocation(FVector Location);

	// Utility functions
	float GetDistanceToTarget(AActor* Target);
	FVector PredictTargetLocation(AActor* Target);
	bool HasValidTarget();
	void LoseTarget();

	// Weapon functions
	void EquipWeapon(TSubclassOf<AFPSWeapon> WeaponClass);
	void UnequipWeapon();

	// Timer handles
	FTimerHandle PatrolWaitTimer;
	FTimerHandle FireDelayTimer;
	FTimerHandle AlertTimer;
	FTimerHandle SearchTimer;

public:
	virtual void Tick(float DeltaTime) override;

	// Getters
	UFUNCTION(BlueprintPure, Category = "AI")
	EAIState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintPure, Category = "AI")
	bool HasTarget() const { return CurrentTarget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "AI")
	float GetAccuracy() const { return AIStats.Accuracy; }

	// Damage handling
	UFUNCTION()
	void OnTakeDamage(float Damage, EDamageType DamageType, FVector HitLocation, AActor* DamageDealer);

	UFUNCTION()
	void OnAIDeath();

	// Public interface for external systems
	UFUNCTION(BlueprintCallable, Category = "AI")
	void AlertToLocation(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetCombatStyle(EAICombatStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void AddPatrolPoint(FVector Point);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAIStats(FAIStats NewStats);
};
