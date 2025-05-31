#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "FPSAIController.generated.h"

UCLASS()
class FPSGAME_API AFPSAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFPSAIController();

protected:
	virtual void BeginPlay() override;
	virtual void Possess(APawn* InPawn) override;

	// AI Perception Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerceptionComponent;

	// Sight configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	// Hearing configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UAISenseConfig_Hearing* HearingConfig;

	// Damage configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UAISenseConfig_Damage* DamageConfig;

	// Behavior Tree
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTree;

	// Blackboard keys
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName TargetActorKey = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName LastKnownLocationKey = TEXT("LastKnownLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName AIStateKey = TEXT("AIState");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName PatrolPointKey = TEXT("PatrolPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName CoverLocationKey = TEXT("CoverLocation");

	// AI perception callbacks
	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Utility functions
	void SetupPerception();
	void SetupBehaviorTree();

	// Blackboard helpers
	void SetTargetActor(AActor* Target);
	void SetLastKnownLocation(FVector Location);
	void SetAIState(int32 State);
	void ClearTarget();

public:
	// Public interface
	UFUNCTION(BlueprintCallable, Category = "AI")
	AActor* GetTargetActor() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	FVector GetLastKnownLocation() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	int32 GetAIState() const;

	// Combat functions
	UFUNCTION(BlueprintCallable, Category = "AI")
	bool CanSeeActor(AActor* Actor) const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	float GetDistanceToActor(AActor* Actor) const;

	// Override for debug display
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
};
