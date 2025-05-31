#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "AI/AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISightPerceptionComponent.h"
#include "Perception/AIHearingPerceptionComponent.h"
#include "AdvancedAISystem.generated.h"

class AFPSCharacter;
class AAdvancedWeaponSystem;

UENUM(BlueprintType)
enum class EAIBehaviorState : uint8
{
    Patrol          UMETA(DisplayName = "Patrol"),
    Investigate     UMETA(DisplayName = "Investigate"),
    Combat          UMETA(DisplayName = "Combat"),
    Search          UMETA(DisplayName = "Search"),
    Retreat         UMETA(DisplayName = "Retreat"),
    TakeCover       UMETA(DisplayName = "Take Cover"),
    Flank           UMETA(DisplayName = "Flank"),
    Suppress        UMETA(DisplayName = "Suppress"),
    CallForBackup   UMETA(DisplayName = "Call For Backup")
};

UENUM(BlueprintType)
enum class EAIDifficulty : uint8
{
    Easy        UMETA(DisplayName = "Easy"),
    Medium      UMETA(DisplayName = "Medium"),
    Hard        UMETA(DisplayName = "Hard"),
    Expert      UMETA(DisplayName = "Expert"),
    Tactical    UMETA(DisplayName = "Tactical")
};

UENUM(BlueprintType)
enum class EAIPersonality : uint8
{
    Aggressive      UMETA(DisplayName = "Aggressive"),
    Defensive       UMETA(DisplayName = "Defensive"),
    Tactical        UMETA(DisplayName = "Tactical"),
    Stealthy        UMETA(DisplayName = "Stealthy"),
    Support         UMETA(DisplayName = "Support")
};

USTRUCT(BlueprintType)
struct FAITacticalData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AccuracyModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReactionTime = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AggresionLevel = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CoverUsage = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TeamworkFactor = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PatrolRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CombatRadius = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HearingRadius = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SightRange = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SightAngle = 90.0f;
};

USTRUCT(BlueprintType)
struct FAIMemory
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FVector> LastKnownEnemyPositions;

    UPROPERTY(BlueprintReadWrite)
    TArray<FVector> InterestPoints;

    UPROPERTY(BlueprintReadWrite)
    TArray<FVector> CoverPoints;

    UPROPERTY(BlueprintReadWrite)
    TMap<AActor*, float> ThreatLevels;

    UPROPERTY(BlueprintReadWrite)
    float LastCombatTime = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bHasSeenPlayer = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsInCombat = false;
};

USTRUCT(BlueprintType)
struct FAIOptimizationSettings
{
    GENERATED_BODY()

    // Time-slicing settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableTimeSlicing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MaxAIUpdatesPerFrame = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float TimeSliceBudgetMS = 2.0f;

    // Distance-based LOD settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    bool bEnableDistanceLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float HighDetailDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float MediumDetailDistance = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float LowDetailDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float CullDistance = 8000.0f;

    // Update frequency scaling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float HighDetailUpdateRate = 0.1f; // 10 times per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MediumDetailUpdateRate = 0.2f; // 5 times per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float LowDetailUpdateRate = 0.5f; // 2 times per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float CulledUpdateRate = 1.0f; // 1 time per second
};

UENUM(BlueprintType)
enum class EAILODLevel : uint8
{
    HighDetail = 0,
    MediumDetail = 1,
    LowDetail = 2,
    Culled = 3
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStateChanged, EAIBehaviorState, OldState, EAIBehaviorState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDetected, AActor*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTakingDamage, float, Damage);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UAdvancedAISystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UAdvancedAISystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    EAIDifficulty Difficulty = EAIDifficulty::Medium;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    EAIPersonality Personality = EAIPersonality::Tactical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    FAITacticalData TacticalData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    TSubclassOf<class UBehaviorTree> BehaviorTreeAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    TSubclassOf<class UBlackboardAsset> BlackboardAsset;

    // Current State
    UPROPERTY(BlueprintReadOnly, Category = "AI State")
    EAIBehaviorState CurrentBehaviorState = EAIBehaviorState::Patrol;

    UPROPERTY(BlueprintReadOnly, Category = "AI State")
    FAIMemory AIMemory;

    UPROPERTY(BlueprintReadOnly, Category = "AI State")
    AActor* CurrentTarget = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "AI State")
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    // Events
    UPROPERTY(BlueprintAssignable)
    FOnAIStateChanged OnAIStateChanged;

    UPROPERTY(BlueprintAssignable)
    FOnEnemyDetected OnEnemyDetected;

    UPROPERTY(BlueprintAssignable)
    FOnTakingDamage OnTakingDamage;

    // Core AI Functions
    UFUNCTION(BlueprintCallable, Category = "AI System")
    void SetBehaviorState(EAIBehaviorState NewState);

    UFUNCTION(BlueprintCallable, Category = "AI System")
    void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    UFUNCTION(BlueprintCallable, Category = "AI System")
    void OnDamageReceived(float DamageAmount, AActor* DamageInstigator);

    UFUNCTION(BlueprintCallable, Category = "AI System")
    void UpdateAILogic(float DeltaTime);

    // Tactical Functions
    UFUNCTION(BlueprintCallable, Category = "AI Tactics")
    FVector FindBestCoverPoint(FVector ThreatLocation, float SearchRadius = 1000.0f);

    UFUNCTION(BlueprintCallable, Category = "AI Tactics")
    FVector FindFlankingPosition(FVector EnemyLocation, float FlankRadius = 800.0f);

    UFUNCTION(BlueprintCallable, Category = "AI Tactics")
    bool ShouldRetreat();

    UFUNCTION(BlueprintCallable, Category = "AI Tactics")
    void CallForBackup(FVector Location);

    UFUNCTION(BlueprintCallable, Category = "AI Tactics")
    float CalculateThreatLevel(AActor* PotentialThreat);

    // Combat Functions
    UFUNCTION(BlueprintCallable, Category = "AI Combat")
    bool CanSeeTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "AI Combat")
    FVector PredictTargetLocation(AActor* Target, float PredictionTime = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "AI Combat")
    void AimAtTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "AI Combat")
    void FireWeapon();

    UFUNCTION(BlueprintCallable, Category = "AI Combat")
    void ReloadWeapon();

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "AI Utility")
    void UpdateTacticalDataForDifficulty();

    UFUNCTION(BlueprintCallable, Category = "AI Utility")
    void ApplyPersonalityModifiers();

    UFUNCTION(BlueprintCallable, Category = "AI Utility")
    bool IsInCover();

    UFUNCTION(BlueprintCallable, Category = "AI Utility")
    void UpdateMemory();

    // AI Optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization")
    FAIOptimizationSettings OptimizationSettings;

    UPROPERTY(BlueprintReadOnly, Category = "Optimization")
    EAILODLevel CurrentLODLevel = EAILODLevel::HighDetail;

    UPROPERTY(BlueprintReadOnly, Category = "Optimization")
    float DistanceToPlayer = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Optimization")
    float LastUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Optimization")
    bool bIsTimeSliced = false;

    // Static optimization management
    static TArray<UAdvancedAISystem*> ActiveAISystems;
    static int32 CurrentTimeSliceIndex;
    static float TimeSliceStartTime;

    // AI Update Functions
    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    void UpdateDistanceLOD();

    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    void SetLODLevel(EAILODLevel NewLODLevel);

    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    bool ShouldUpdateThisFrame() const;

    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    float GetCurrentUpdateInterval() const;

    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    void RegisterForTimeSlicing();

    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    void UnregisterFromTimeSlicing();

    // Static time-slicing manager
    UFUNCTION(BlueprintCallable, Category = "AI|Optimization", CallInEditor = true)
    static void ProcessTimeSlicedUpdates(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    static int32 GetActiveAICount() { return ActiveAISystems.Num(); }

    // Optimized update functions
    void UpdateAILogicOptimized(float DeltaTime);
    void UpdateHighDetailLogic(float DeltaTime);
    void UpdateMediumDetailLogic(float DeltaTime);
    void UpdateLowDetailLogic(float DeltaTime);
    void UpdateCulledLogic(float DeltaTime);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "AI|Optimization")
    float GetAIUpdateCost() const { return LastUpdateCost; }

private:
    float LastUpdateCost = 0.0f;
    double UpdateStartTime = 0.0;

    // Internal systems
    UPROPERTY()
    class AAIController* AIController;

    UPROPERTY()
    class UBehaviorTreeComponent* BehaviorTreeComponent;

    UPROPERTY()
    class UBlackboardComponent* BlackboardComponent;

    UPROPERTY()
    class UAIPerceptionComponent* PerceptionComponent;

    UPROPERTY()
    AAdvancedWeaponSystem* CurrentWeapon;

    // Timers and counters
    float StateChangeTimer = 0.0f;
    float MemoryUpdateTimer = 0.0f;
    float TacticalDecisionTimer = 0.0f;
    float CombatTimer = 0.0f;
    float LastFireTime = 0.0f;
    
    // Internal functions
    void InitializeAI();
    void UpdateBehaviorLogic(float DeltaTime);
    void UpdateCombatLogic(float DeltaTime);
    void UpdateMovementLogic(float DeltaTime);
    void ProcessPerceptionData();
    void MakeTacticalDecision();
    bool ValidateLineOfSight(FVector Start, FVector End, AActor* IgnoreActor = nullptr);
    FVector GetRandomPatrolPoint();
    void HandleCombatState(float DeltaTime);
    void HandlePatrolState(float DeltaTime);
    void HandleInvestigateState(float DeltaTime);
    void HandleSearchState(float DeltaTime);
    void HandleTakeCoverState(float DeltaTime);
    void HandleFlankState(float DeltaTime);
    void HandleSuppressState(float DeltaTime);
    void HandleRetreatState(float DeltaTime);
};
