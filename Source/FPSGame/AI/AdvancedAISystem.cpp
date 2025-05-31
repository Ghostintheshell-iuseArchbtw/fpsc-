#include "AdvancedAISystem.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISightPerceptionComponent.h"
#include "Perception/AIHearingPerceptionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "../Weapons/AdvancedWeaponSystem.h"
#include "../Characters/FPSCharacter.h"

// Initialize static members
TArray<UAdvancedAISystem*> UAdvancedAISystem::ActiveAISystems;
int32 UAdvancedAISystem::CurrentTimeSliceIndex = 0;
float UAdvancedAISystem::TimeSliceStartTime = 0.0f;

UAdvancedAISystem::UAdvancedAISystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // Update 10 times per second for performance
    
    // Initialize optimization settings
    OptimizationSettings.bEnableTimeSlicing = true;
    OptimizationSettings.MaxAIUpdatesPerFrame = 8.0f;
    OptimizationSettings.TimeSliceBudgetMS = 2.0f;
    OptimizationSettings.bEnableDistanceLOD = true;
    OptimizationSettings.HighDetailDistance = 1000.0f;
    OptimizationSettings.MediumDetailDistance = 2500.0f;
    OptimizationSettings.LowDetailDistance = 5000.0f;
    OptimizationSettings.CullDistance = 8000.0f;
    OptimizationSettings.HighDetailUpdateRate = 0.1f;
    OptimizationSettings.MediumDetailUpdateRate = 0.2f;
    OptimizationSettings.LowDetailUpdateRate = 0.5f;
    OptimizationSettings.CulledUpdateRate = 1.0f;
    
    // Initialize tactical data with default values
    TacticalData.AccuracyModifier = 1.0f;
    TacticalData.ReactionTime = 0.5f;
    TacticalData.AggresionLevel = 0.5f;
    TacticalData.CoverUsage = 0.7f;
    TacticalData.TeamworkFactor = 0.6f;
    TacticalData.PatrolRadius = 1000.0f;
    TacticalData.CombatRadius = 2000.0f;
    TacticalData.HearingRadius = 1500.0f;
    TacticalData.SightRange = 3000.0f;
    TacticalData.SightAngle = 90.0f;
}

void UAdvancedAISystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize AI components
    InitializeAI();
    
    // Register for time slicing if enabled
    if (OptimizationSettings.bEnableTimeSlicing)
    {
        RegisterForTimeSlicing();
    }
    
    // Initialize distance LOD
    if (OptimizationSettings.bEnableDistanceLOD)
    {
        UpdateDistanceLOD();
    }
}

void UAdvancedAISystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unregister from time slicing
    UnregisterFromTimeSlicing();
    
    Super::EndPlay(EndPlayReason);
}

void UAdvancedAISystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Update distance LOD
    if (OptimizationSettings.bEnableDistanceLOD)
    {
        UpdateDistanceLOD();
    }
    
    // Time-sliced or regular update
    if (OptimizationSettings.bEnableTimeSlicing)
    {
        // Time-sliced updates are handled by the static manager
        ProcessTimeSlicedUpdates(DeltaTime);
    }
    else
    {
        // Regular update if not time-sliced
        if (ShouldUpdateThisFrame())
        {
            UpdateAILogicOptimized(DeltaTime);
        }
    }
}

// AI Optimization Functions
void UAdvancedAISystem::UpdateDistanceLOD()
{
    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!Player || !GetOwner())
    {
        return;
    }
    
    DistanceToPlayer = FVector::Dist(GetOwner()->GetActorLocation(), Player->GetActorLocation());
    
    EAILODLevel NewLODLevel = EAILODLevel::HighDetail;
    
    if (DistanceToPlayer > OptimizationSettings.CullDistance)
    {
        NewLODLevel = EAILODLevel::Culled;
    }
    else if (DistanceToPlayer > OptimizationSettings.LowDetailDistance)
    {
        NewLODLevel = EAILODLevel::LowDetail;
    }
    else if (DistanceToPlayer > OptimizationSettings.MediumDetailDistance)
    {
        NewLODLevel = EAILODLevel::MediumDetail;
    }
    
    if (NewLODLevel != CurrentLODLevel)
    {
        SetLODLevel(NewLODLevel);
    }
}

void UAdvancedAISystem::SetLODLevel(EAILODLevel NewLODLevel)
{
    if (CurrentLODLevel == NewLODLevel)
    {
        return;
    }
    
    CurrentLODLevel = NewLODLevel;
    
    // Adjust tick interval based on LOD level
    switch (CurrentLODLevel)
    {
        case EAILODLevel::HighDetail:
            PrimaryComponentTick.TickInterval = OptimizationSettings.HighDetailUpdateRate;
            break;
        case EAILODLevel::MediumDetail:
            PrimaryComponentTick.TickInterval = OptimizationSettings.MediumDetailUpdateRate;
            break;
        case EAILODLevel::LowDetail:
            PrimaryComponentTick.TickInterval = OptimizationSettings.LowDetailUpdateRate;
            break;
        case EAILODLevel::Culled:
            PrimaryComponentTick.TickInterval = OptimizationSettings.CulledUpdateRate;
            break;
    }
    
    // Adjust perception settings based on LOD
    if (PerceptionComponent)
    {
        switch (CurrentLODLevel)
        {
            case EAILODLevel::HighDetail:
                // Full perception
                PerceptionComponent->SetSenseConfig(
                    FAISenseConfig_Sight::StaticClass(),
                    TacticalData.SightRange,
                    TacticalData.SightAngle
                );
                break;
            case EAILODLevel::MediumDetail:
                // Reduced perception range
                PerceptionComponent->SetSenseConfig(
                    FAISenseConfig_Sight::StaticClass(),
                    TacticalData.SightRange * 0.7f,
                    TacticalData.SightAngle * 0.8f
                );
                break;
            case EAILODLevel::LowDetail:
                // Minimal perception
                PerceptionComponent->SetSenseConfig(
                    FAISenseConfig_Sight::StaticClass(),
                    TacticalData.SightRange * 0.5f,
                    TacticalData.SightAngle * 0.6f
                );
                break;
            case EAILODLevel::Culled:
                // No perception updates
                break;
        }
    }
}

bool UAdvancedAISystem::ShouldUpdateThisFrame() const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float TimeSinceLastUpdate = CurrentTime - LastUpdateTime;
    
    return TimeSinceLastUpdate >= GetCurrentUpdateInterval();
}

float UAdvancedAISystem::GetCurrentUpdateInterval() const
{
    switch (CurrentLODLevel)
    {
        case EAILODLevel::HighDetail:
            return OptimizationSettings.HighDetailUpdateRate;
        case EAILODLevel::MediumDetail:
            return OptimizationSettings.MediumDetailUpdateRate;
        case EAILODLevel::LowDetail:
            return OptimizationSettings.LowDetailUpdateRate;
        case EAILODLevel::Culled:
            return OptimizationSettings.CulledUpdateRate;
        default:
            return OptimizationSettings.HighDetailUpdateRate;
    }
}

void UAdvancedAISystem::RegisterForTimeSlicing()
{
    if (!ActiveAISystems.Contains(this))
    {
        ActiveAISystems.Add(this);
        bIsTimeSliced = true;
    }
}

void UAdvancedAISystem::UnregisterFromTimeSlicing()
{
    ActiveAISystems.Remove(this);
    bIsTimeSliced = false;
}

// Static time-slicing manager
void UAdvancedAISystem::ProcessTimeSlicedUpdates(float DeltaTime)
{
    if (ActiveAISystems.Num() == 0)
    {
        return;
    }
    
    float CurrentTime = FPlatformTime::Seconds();
    
    // Reset time slice if starting new frame
    if (TimeSliceStartTime == 0.0f || (CurrentTime - TimeSliceStartTime) > 0.016f) // 60 FPS frame time
    {
        TimeSliceStartTime = CurrentTime;
        CurrentTimeSliceIndex = 0;
    }
    
    // Calculate how many AI systems we can update this frame
    int32 MaxUpdatesThisFrame = FMath::Min(
        (int32)OptimizationSettings.MaxAIUpdatesPerFrame,
        ActiveAISystems.Num()
    );
    
    float TimeSliceBudget = OptimizationSettings.TimeSliceBudgetMS * 0.001f; // Convert to seconds
    int32 UpdatesProcessed = 0;
    
    while (UpdatesProcessed < MaxUpdatesThisFrame && 
           (FPlatformTime::Seconds() - TimeSliceStartTime) < TimeSliceBudget)
    {
        if (CurrentTimeSliceIndex >= ActiveAISystems.Num())
        {
            CurrentTimeSliceIndex = 0;
        }
        
        UAdvancedAISystem* AISystem = ActiveAISystems[CurrentTimeSliceIndex];
        if (IsValid(AISystem) && AISystem->ShouldUpdateThisFrame())
        {
            AISystem->UpdateAILogicOptimized(DeltaTime);
            AISystem->LastUpdateTime = FPlatformTime::Seconds();
        }
        
        CurrentTimeSliceIndex++;
        UpdatesProcessed++;
    }
}

// Optimized update functions
void UAdvancedAISystem::UpdateAILogicOptimized(float DeltaTime)
{
    UpdateStartTime = FPlatformTime::Seconds();
    
    switch (CurrentLODLevel)
    {
        case EAILODLevel::HighDetail:
            UpdateHighDetailLogic(DeltaTime);
            break;
        case EAILODLevel::MediumDetail:
            UpdateMediumDetailLogic(DeltaTime);
            break;
        case EAILODLevel::LowDetail:
            UpdateLowDetailLogic(DeltaTime);
            break;
        case EAILODLevel::Culled:
            UpdateCulledLogic(DeltaTime);
            break;
    }
    
    LastUpdateCost = (FPlatformTime::Seconds() - UpdateStartTime) * 1000.0f; // Convert to milliseconds
}

void UAdvancedAISystem::UpdateHighDetailLogic(float DeltaTime)
{
    // Full AI update - all systems active
    UpdateAILogic(DeltaTime);
}

void UAdvancedAISystem::UpdateMediumDetailLogic(float DeltaTime)
{
    // Reduced frequency updates for some systems
    if (!AIController) return;
    
    // Update timers with reduced frequency
    StateChangeTimer += DeltaTime;
    TacticalDecisionTimer += DeltaTime * 0.7f; // Slower tactical decisions
    CombatTimer += DeltaTime;
    
    // Update memory less frequently
    MemoryUpdateTimer += DeltaTime;
    if (MemoryUpdateTimer >= 2.0f) // Every 2 seconds instead of 1
    {
        UpdateMemory();
        MemoryUpdateTimer = 0.0f;
    }
    
    // Make tactical decisions less frequently
    if (TacticalDecisionTimer >= 3.0f) // Every 3 seconds instead of 2
    {
        MakeTacticalDecision();
        TacticalDecisionTimer = 0.0f;
    }
    
    // Update behavior logic
    UpdateBehaviorLogic(DeltaTime);
    
    // Update combat logic if in combat (reduced complexity)
    if (AIMemory.bIsInCombat)
    {
        UpdateCombatLogic(DeltaTime);
    }
    
    // Skip movement logic optimization for medium detail
}

void UAdvancedAISystem::UpdateLowDetailLogic(float DeltaTime)
{
    // Minimal AI update - only essential systems
    if (!AIController) return;
    
    StateChangeTimer += DeltaTime;
    
    // Very infrequent memory updates
    MemoryUpdateTimer += DeltaTime;
    if (MemoryUpdateTimer >= 5.0f) // Every 5 seconds
    {
        UpdateMemory();
        MemoryUpdateTimer = 0.0f;
    }
    
    // Basic behavior logic only
    switch (CurrentBehaviorState)
    {
        case EAIBehaviorState::Patrol:
            HandlePatrolState(DeltaTime);
            break;
        case EAIBehaviorState::Combat:
            // Simplified combat
            if (CurrentTarget && AIMemory.bIsInCombat)
            {
                HandleCombatState(DeltaTime);
            }
            break;
        default:
            // Skip other states for low detail
            break;
    }
}

void UAdvancedAISystem::UpdateCulledLogic(float DeltaTime)
{
    // Minimal update - only state preservation
    StateChangeTimer += DeltaTime;
    
    // Only update if the AI is in combat and close to becoming visible
    if (AIMemory.bIsInCombat && DistanceToPlayer < OptimizationSettings.CullDistance * 1.1f)
    {
        // Very basic combat state maintenance
        if (StateChangeTimer > 30.0f) // Reset combat after 30 seconds of being culled
        {
            AIMemory.bIsInCombat = false;
            SetBehaviorState(EAIBehaviorState::Patrol);
        }
    }
}

void UAdvancedAISystem::InitializeAI()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;
    
    // Get AI Controller
    APawn* PawnOwner = Cast<APawn>(Owner);
    if (PawnOwner)
    {
        AIController = Cast<AAIController>(PawnOwner->GetController());
        if (AIController)
        {
            BehaviorTreeComponent = AIController->FindComponentByClass<UBehaviorTreeComponent>();
            BlackboardComponent = AIController->FindComponentByClass<UBlackboardComponent>();
            PerceptionComponent = AIController->FindComponentByClass<UAIPerceptionComponent>();
            
            // Setup perception component if available
            if (PerceptionComponent)
            {
                PerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &UAdvancedAISystem::OnPerceptionUpdated);
                
                // Configure sight perception
                UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
                if (SightConfig)
                {
                    SightConfig->SightRadius = TacticalData.SightRange;
                    SightConfig->LoseSightRadius = TacticalData.SightRange + 500.0f;
                    SightConfig->PeripheralVisionAngleDegrees = TacticalData.SightAngle;
                    SightConfig->SetMaxAge(10.0f);
                    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
                    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
                    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
                    PerceptionComponent->ConfigureSense(*SightConfig);
                }
                
                // Configure hearing perception
                UAISenseConfig_Hearing* HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
                if (HearingConfig)
                {
                    HearingConfig->HearingRange = TacticalData.HearingRadius;
                    HearingConfig->SetMaxAge(5.0f);
                    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
                    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
                    HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
                    PerceptionComponent->ConfigureSense(*HearingConfig);
                }
                
                PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
            }
            
            // Start behavior tree if available
            if (BehaviorTreeComponent && BehaviorTreeAsset)
            {
                AIController->RunBehaviorTree(BehaviorTreeAsset);
            }
        }
    }
    
    // Find weapon component
    CurrentWeapon = Owner->FindComponentByClass<AAdvancedWeaponSystem>();
}

void UAdvancedAISystem::UpdateAILogic(float DeltaTime)
{
    if (!AIController) return;
    
    // Update timers
    StateChangeTimer += DeltaTime;
    MemoryUpdateTimer += DeltaTime;
    TacticalDecisionTimer += DeltaTime;
    CombatTimer += DeltaTime;
    
    // Update memory periodically
    if (MemoryUpdateTimer >= 1.0f)
    {
        UpdateMemory();
        MemoryUpdateTimer = 0.0f;
    }
    
    // Make tactical decisions periodically
    if (TacticalDecisionTimer >= 2.0f)
    {
        MakeTacticalDecision();
        TacticalDecisionTimer = 0.0f;
    }
    
    // Update behavior logic
    UpdateBehaviorLogic(DeltaTime);
    
    // Process perception data
    ProcessPerceptionData();
    
    // Update combat logic if in combat
    if (AIMemory.bIsInCombat)
    {
        UpdateCombatLogic(DeltaTime);
    }
    
    // Update movement logic
    UpdateMovementLogic(DeltaTime);
}

void UAdvancedAISystem::SetBehaviorState(EAIBehaviorState NewState)
{
    if (CurrentBehaviorState != NewState)
    {
        EAIBehaviorState OldState = CurrentBehaviorState;
        CurrentBehaviorState = NewState;
        StateChangeTimer = 0.0f;
        
        // Update blackboard if available
        if (BlackboardComponent)
        {
            BlackboardComponent->SetValueAsEnum(TEXT("BehaviorState"), static_cast<uint8>(NewState));
        }
        
        // Broadcast state change
        OnAIStateChanged.Broadcast(OldState, NewState);
        
        UE_LOG(LogTemp, Log, TEXT("AI State Changed: %s -> %s"), 
               *UEnum::GetValueAsString(OldState), 
               *UEnum::GetValueAsString(NewState));
    }
}

void UAdvancedAISystem::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;
    
    // Check if this is a player character
    AFPSCharacter* PlayerCharacter = Cast<AFPSCharacter>(Actor);
    if (PlayerCharacter)
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            // Enemy detected
            CurrentTarget = Actor;
            LastKnownPlayerLocation = Actor->GetActorLocation();
            AIMemory.bHasSeenPlayer = true;
            AIMemory.LastCombatTime = GetWorld()->GetTimeSeconds();
            
            // Add to memory
            AIMemory.LastKnownEnemyPositions.AddUnique(LastKnownPlayerLocation);
            AIMemory.ThreatLevels.Add(Actor, CalculateThreatLevel(Actor));
            
            // Change to combat state
            if (CurrentBehaviorState != EAIBehaviorState::Combat)
            {
                SetBehaviorState(EAIBehaviorState::Combat);
                AIMemory.bIsInCombat = true;
            }
            
            OnEnemyDetected.Broadcast(Actor);
        }
        else
        {
            // Lost sight of enemy
            if (CurrentTarget == Actor)
            {
                CurrentTarget = nullptr;
                SetBehaviorState(EAIBehaviorState::Search);
            }
        }
    }
}

void UAdvancedAISystem::OnDamageReceived(float DamageAmount, AActor* DamageInstigator)
{
    OnTakingDamage.Broadcast(DamageAmount);
    
    if (DamageInstigator)
    {
        // Update threat level
        float CurrentThreat = AIMemory.ThreatLevels.FindRef(DamageInstigator);
        AIMemory.ThreatLevels.Add(DamageInstigator, CurrentThreat + DamageAmount * 0.1f);
        
        // If not already in combat, enter combat state
        if (!AIMemory.bIsInCombat)
        {
            CurrentTarget = DamageInstigator;
            LastKnownPlayerLocation = DamageInstigator->GetActorLocation();
            AIMemory.bIsInCombat = true;
            SetBehaviorState(EAIBehaviorState::Combat);
        }
        
        // Consider retreating if heavily damaged
        if (ShouldRetreat())
        {
            SetBehaviorState(EAIBehaviorState::Retreat);
        }
    }
}

void UAdvancedAISystem::UpdateBehaviorLogic(float DeltaTime)
{
    switch (CurrentBehaviorState)
    {
        case EAIBehaviorState::Patrol:
            HandlePatrolState(DeltaTime);
            break;
        case EAIBehaviorState::Investigate:
            HandleInvestigateState(DeltaTime);
            break;
        case EAIBehaviorState::Combat:
            HandleCombatState(DeltaTime);
            break;
        case EAIBehaviorState::Search:
            HandleSearchState(DeltaTime);
            break;
        case EAIBehaviorState::Retreat:
            HandleRetreatState(DeltaTime);
            break;
        case EAIBehaviorState::TakeCover:
            HandleTakeCoverState(DeltaTime);
            break;
        case EAIBehaviorState::Flank:
            HandleFlankState(DeltaTime);
            break;
        case EAIBehaviorState::Suppress:
            HandleSuppressState(DeltaTime);
            break;
        case EAIBehaviorState::CallForBackup:
            CallForBackup(LastKnownPlayerLocation);
            SetBehaviorState(EAIBehaviorState::Combat);
            break;
    }
}

FVector UAdvancedAISystem::FindBestCoverPoint(FVector ThreatLocation, float SearchRadius)
{
    AActor* Owner = GetOwner();
    if (!Owner) return FVector::ZeroVector;
    
    FVector OwnerLocation = Owner->GetActorLocation();
    FVector BestCoverPoint = OwnerLocation;
    float BestScore = -1.0f;
    
    // Search for cover points in a grid pattern
    int32 GridSize = 10;
    float GridSpacing = SearchRadius / GridSize;
    
    for (int32 x = -GridSize; x <= GridSize; x++)
    {
        for (int32 y = -GridSize; y <= GridSize; y++)
        {
            FVector TestPoint = OwnerLocation + FVector(x * GridSpacing, y * GridSpacing, 0.0f);
            
            // Check if point is reachable
            FHitResult HitResult;
            bool bHit = GetWorld()->LineTraceSingleByChannel(
                HitResult,
                OwnerLocation,
                TestPoint,
                ECC_WorldStatic
            );
            
            if (!bHit)
            {
                // Check if point provides cover from threat
                bool bInCover = GetWorld()->LineTraceSingleByChannel(
                    HitResult,
                    TestPoint + FVector(0, 0, 100), // Slightly elevated
                    ThreatLocation,
                    ECC_WorldStatic
                );
                
                if (bInCover)
                {
                    float Distance = FVector::Dist(OwnerLocation, TestPoint);
                    float ThreatDistance = FVector::Dist(TestPoint, ThreatLocation);
                    
                    // Score based on cover quality and distance
                    float Score = (ThreatDistance / 100.0f) - (Distance / 200.0f);
                    
                    if (Score > BestScore)
                    {
                        BestScore = Score;
                        BestCoverPoint = TestPoint;
                    }
                }
            }
        }
    }
    
    return BestCoverPoint;
}

FVector UAdvancedAISystem::FindFlankingPosition(FVector EnemyLocation, float FlankRadius)
{
    AActor* Owner = GetOwner();
    if (!Owner) return FVector::ZeroVector;
    
    FVector OwnerLocation = Owner->GetActorLocation();
    FVector DirectionToEnemy = (EnemyLocation - OwnerLocation).GetSafeNormal();
    
    // Calculate perpendicular vectors for flanking
    FVector RightFlank = FVector::CrossProduct(DirectionToEnemy, FVector::UpVector).GetSafeNormal() * FlankRadius;
    FVector LeftFlank = -RightFlank;
    
    FVector RightFlankPos = EnemyLocation + RightFlank;
    FVector LeftFlankPos = EnemyLocation + LeftFlank;
    
    // Choose the flanking position that's more accessible
    FHitResult HitResult;
    bool bRightAccessible = !GetWorld()->LineTraceSingleByChannel(
        HitResult, OwnerLocation, RightFlankPos, ECC_WorldStatic
    );
    
    bool bLeftAccessible = !GetWorld()->LineTraceSingleByChannel(
        HitResult, OwnerLocation, LeftFlankPos, ECC_WorldStatic
    );
    
    if (bRightAccessible && bLeftAccessible)
    {
        // Choose randomly
        return FMath::RandBool() ? RightFlankPos : LeftFlankPos;
    }
    else if (bRightAccessible)
    {
        return RightFlankPos;
    }
    else if (bLeftAccessible)
    {
        return LeftFlankPos;
    }
    
    return EnemyLocation; // Fallback
}

bool UAdvancedAISystem::ShouldRetreat()
{
    AActor* Owner = GetOwner();
    if (!Owner) return false;
    
    // Check health percentage (assuming character has health component)
    APawn* PawnOwner = Cast<APawn>(Owner);
    if (PawnOwner)
    {
        // Simple health check - in real implementation, get from health component
        float HealthPercentage = 0.5f; // Placeholder
        
        // Retreat if health is low or outnumbered
        if (HealthPercentage < 0.3f)
        {
            return true;
        }
        
        // Retreat if facing multiple threats
        int32 ThreatCount = 0;
        for (auto& ThreatPair : AIMemory.ThreatLevels)
        {
            if (ThreatPair.Value > 0.5f)
            {
                ThreatCount++;
            }
        }
        
        return ThreatCount >= 3;
    }
    
    return false;
}

void UAdvancedAISystem::CallForBackup(FVector Location)
{
    // Broadcast to nearby AI units
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), nullptr, Location); // Placeholder for radio sound
    
    // Find nearby AI units and alert them
    TArray<AActor*> NearbyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), NearbyActors);
    
    for (AActor* Actor : NearbyActors)
    {
        if (Actor != GetOwner())
        {
            UAdvancedAISystem* OtherAI = Actor->FindComponentByClass<UAdvancedAISystem>();
            if (OtherAI && FVector::Dist(Actor->GetActorLocation(), Location) < 2000.0f)
            {
                OtherAI->LastKnownPlayerLocation = Location;
                OtherAI->SetBehaviorState(EAIBehaviorState::Investigate);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("AI calling for backup at location: %s"), *Location.ToString());
}

float UAdvancedAISystem::CalculateThreatLevel(AActor* PotentialThreat)
{
    if (!PotentialThreat) return 0.0f;
    
    float ThreatLevel = 1.0f;
    
    // Check if target has weapons
    AAdvancedWeaponSystem* Weapon = PotentialThreat->FindComponentByClass<AAdvancedWeaponSystem>();
    if (Weapon)
    {
        ThreatLevel += 2.0f;
    }
    
    // Distance factor
    float Distance = FVector::Dist(GetOwner()->GetActorLocation(), PotentialThreat->GetActorLocation());
    ThreatLevel += FMath::Max(0.0f, 3.0f - (Distance / 1000.0f));
    
    // Line of sight factor
    if (CanSeeTarget(PotentialThreat))
    {
        ThreatLevel += 1.0f;
    }
    
    return FMath::Clamp(ThreatLevel, 0.0f, 10.0f);
}

bool UAdvancedAISystem::CanSeeTarget(AActor* Target)
{
    if (!Target) return false;
    
    AActor* Owner = GetOwner();
    if (!Owner) return false;
    
    return ValidateLineOfSight(
        Owner->GetActorLocation() + FVector(0, 0, 100), // Eye level
        Target->GetActorLocation() + FVector(0, 0, 100),
        Owner
    );
}

FVector UAdvancedAISystem::PredictTargetLocation(AActor* Target, float PredictionTime)
{
    if (!Target) return FVector::ZeroVector;
    
    APawn* TargetPawn = Cast<APawn>(Target);
    if (TargetPawn && TargetPawn->GetMovementComponent())
    {
        FVector CurrentVelocity = TargetPawn->GetMovementComponent()->Velocity;
        return Target->GetActorLocation() + (CurrentVelocity * PredictionTime);
    }
    
    return Target->GetActorLocation();
}

void UAdvancedAISystem::UpdateTacticalDataForDifficulty()
{
    switch (Difficulty)
    {
        case EAIDifficulty::Easy:
            TacticalData.AccuracyModifier = 0.6f;
            TacticalData.ReactionTime = 1.0f;
            TacticalData.AggresionLevel = 0.3f;
            TacticalData.CoverUsage = 0.4f;
            break;
        case EAIDifficulty::Medium:
            TacticalData.AccuracyModifier = 0.8f;
            TacticalData.ReactionTime = 0.7f;
            TacticalData.AggresionLevel = 0.5f;
            TacticalData.CoverUsage = 0.6f;
            break;
        case EAIDifficulty::Hard:
            TacticalData.AccuracyModifier = 1.0f;
            TacticalData.ReactionTime = 0.5f;
            TacticalData.AggresionLevel = 0.7f;
            TacticalData.CoverUsage = 0.8f;
            break;
        case EAIDifficulty::Expert:
            TacticalData.AccuracyModifier = 1.2f;
            TacticalData.ReactionTime = 0.3f;
            TacticalData.AggresionLevel = 0.8f;
            TacticalData.CoverUsage = 0.9f;
            break;
        case EAIDifficulty::Tactical:
            TacticalData.AccuracyModifier = 1.5f;
            TacticalData.ReactionTime = 0.2f;
            TacticalData.AggresionLevel = 0.6f;
            TacticalData.CoverUsage = 1.0f;
            TacticalData.TeamworkFactor = 1.0f;
            break;
    }
}

void UAdvancedAISystem::ApplyPersonalityModifiers()
{
    switch (Personality)
    {
        case EAIPersonality::Aggressive:
            TacticalData.AggresionLevel *= 1.5f;
            TacticalData.CoverUsage *= 0.7f;
            break;
        case EAIPersonality::Defensive:
            TacticalData.CoverUsage *= 1.3f;
            TacticalData.AggresionLevel *= 0.7f;
            break;
        case EAIPersonality::Tactical:
            TacticalData.TeamworkFactor *= 1.2f;
            TacticalData.CoverUsage *= 1.1f;
            break;
        case EAIPersonality::Stealthy:
            TacticalData.SightRange *= 1.2f;
            TacticalData.AggresionLevel *= 0.8f;
            break;
        case EAIPersonality::Support:
            TacticalData.TeamworkFactor *= 1.5f;
            TacticalData.CoverUsage *= 1.2f;
            break;
    }
}

// Behavior state handlers
void UAdvancedAISystem::HandlePatrolState(float DeltaTime)
{
    if (StateChangeTimer > 10.0f) // Change patrol point every 10 seconds
    {
        FVector PatrolPoint = GetRandomPatrolPoint();
        if (BlackboardComponent)
        {
            BlackboardComponent->SetValueAsVector(TEXT("PatrolPoint"), PatrolPoint);
        }
        StateChangeTimer = 0.0f;
    }
}

void UAdvancedAISystem::HandleCombatState(float DeltaTime)
{
    if (!CurrentTarget)
    {
        SetBehaviorState(EAIBehaviorState::Search);
        return;
    }
    
    if (CanSeeTarget(CurrentTarget))
    {
        AimAtTarget(CurrentTarget);
        
        // Fire weapon if ready
        if (CombatTimer > TacticalData.ReactionTime)
        {
            FireWeapon();
            CombatTimer = 0.0f;
        }
        
        // Consider taking cover
        if (FMath::RandFloat() < TacticalData.CoverUsage && !IsInCover())
        {
            SetBehaviorState(EAIBehaviorState::TakeCover);
        }
    }
    else
    {
        // Lost sight, search for target
        SetBehaviorState(EAIBehaviorState::Search);
    }
}

void UAdvancedAISystem::HandleSearchState(float DeltaTime)
{
    if (StateChangeTimer > 15.0f) // Search for 15 seconds
    {
        AIMemory.bIsInCombat = false;
        SetBehaviorState(EAIBehaviorState::Patrol);
    }
    
    // Move to last known position
    if (BlackboardComponent && AIMemory.LastKnownEnemyPositions.Num() > 0)
    {
        FVector SearchPoint = AIMemory.LastKnownEnemyPositions.Last();
        BlackboardComponent->SetValueAsVector(TEXT("SearchPoint"), SearchPoint);
    }
}

void UAdvancedAISystem::HandleTakeCoverState(float DeltaTime)
{
    if (CurrentTarget)
    {
        FVector CoverPoint = FindBestCoverPoint(CurrentTarget->GetActorLocation());
        if (BlackboardComponent)
        {
            BlackboardComponent->SetValueAsVector(TEXT("CoverPoint"), CoverPoint);
        }
        
        // Return to combat after reaching cover
        if (StateChangeTimer > 3.0f)
        {
            SetBehaviorState(EAIBehaviorState::Combat);
        }
    }
}

void UAdvancedAISystem::HandleFlankState(float DeltaTime)
{
    if (CurrentTarget)
    {
        FVector FlankPoint = FindFlankingPosition(CurrentTarget->GetActorLocation());
        if (BlackboardComponent)
        {
            BlackboardComponent->SetValueAsVector(TEXT("FlankPoint"), FlankPoint);
        }
        
        // Switch to combat when flanking position is reached
        if (StateChangeTimer > 5.0f)
        {
            SetBehaviorState(EAIBehaviorState::Combat);
        }
    }
}

void UAdvancedAISystem::HandleSuppressState(float DeltaTime)
{
    if (CurrentTarget)
    {
        // Continuous fire to suppress enemy
        if (CombatTimer > 0.2f) // High rate of fire
        {
            FireWeapon();
            CombatTimer = 0.0f;
        }
        
        // Suppress for limited time
        if (StateChangeTimer > 8.0f)
        {
            SetBehaviorState(EAIBehaviorState::Combat);
        }
    }
}

void UAdvancedAISystem::HandleRetreatState(float DeltaTime)
{
    // Find retreat point away from threats
    FVector RetreatPoint = GetOwner()->GetActorLocation();
    
    for (auto& ThreatPair : AIMemory.ThreatLevels)
    {
        if (ThreatPair.Key && ThreatPair.Value > 0.5f)
        {
            FVector ThreatLocation = ThreatPair.Key->GetActorLocation();
            FVector AwayDirection = (GetOwner()->GetActorLocation() - ThreatLocation).GetSafeNormal();
            RetreatPoint += AwayDirection * 1000.0f;
        }
    }
    
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsVector(TEXT("RetreatPoint"), RetreatPoint);
    }
    
    // Return to patrol after retreating
    if (StateChangeTimer > 10.0f)
    {
        AIMemory.bIsInCombat = false;
        SetBehaviorState(EAIBehaviorState::Patrol);
    }
}

void UAdvancedAISystem::HandleInvestigateState(float DeltaTime)
{
    // Move towards points of interest
    if (AIMemory.InterestPoints.Num() > 0)
    {
        FVector InvestigatePoint = AIMemory.InterestPoints[0];
        if (BlackboardComponent)
        {
            BlackboardComponent->SetValueAsVector(TEXT("InvestigatePoint"), InvestigatePoint);
        }
        
        // Remove investigated point
        if (FVector::Dist(GetOwner()->GetActorLocation(), InvestigatePoint) < 100.0f)
        {
            AIMemory.InterestPoints.RemoveAt(0);
        }
    }
    
    // Return to patrol if nothing to investigate
    if (AIMemory.InterestPoints.Num() == 0 || StateChangeTimer > 20.0f)
    {
        SetBehaviorState(EAIBehaviorState::Patrol);
    }
}

// Utility functions
bool UAdvancedAISystem::ValidateLineOfSight(FVector Start, FVector End, AActor* IgnoreActor)
{
    FHitResult HitResult;
    FCollisionQueryParams Params;
    if (IgnoreActor)
    {
        Params.AddIgnoredActor(IgnoreActor);
    }
    
    return !GetWorld()->LineTraceSingleByChannel(
        HitResult, Start, End, ECC_WorldStatic, Params
    );
}

FVector UAdvancedAISystem::GetRandomPatrolPoint()
{
    AActor* Owner = GetOwner();
    if (!Owner) return FVector::ZeroVector;
    
    FVector BaseLocation = Owner->GetActorLocation();
    float RandomAngle = FMath::RandRange(0.0f, 360.0f);
    float RandomDistance = FMath::RandRange(200.0f, TacticalData.PatrolRadius);
    
    return BaseLocation + FVector(
        FMath::Cos(FMath::DegreesToRadians(RandomAngle)) * RandomDistance,
        FMath::Sin(FMath::DegreesToRadians(RandomAngle)) * RandomDistance,
        0.0f
    );
}

void UAdvancedAISystem::UpdateMemory()
{
    // Clean old memory entries
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Remove old threat levels
    TArray<AActor*> ActorsToRemove;
    for (auto& ThreatPair : AIMemory.ThreatLevels)
    {
        if (!IsValid(ThreatPair.Key))
        {
            ActorsToRemove.Add(ThreatPair.Key);
        }
        else
        {
            // Decay threat over time
            AIMemory.ThreatLevels[ThreatPair.Key] *= 0.98f;
            if (AIMemory.ThreatLevels[ThreatPair.Key] < 0.1f)
            {
                ActorsToRemove.Add(ThreatPair.Key);
            }
        }
    }
    
    for (AActor* Actor : ActorsToRemove)
    {
        AIMemory.ThreatLevels.Remove(Actor);
    }
    
    // Limit memory arrays
    if (AIMemory.LastKnownEnemyPositions.Num() > 10)
    {
        AIMemory.LastKnownEnemyPositions.RemoveAt(0);
    }
    
    if (AIMemory.InterestPoints.Num() > 5)
    {
        AIMemory.InterestPoints.RemoveAt(0);
    }
}

void UAdvancedAISystem::UpdateCombatLogic(float DeltaTime)
{
    // Implement advanced combat decision making
    if (!CurrentTarget) return;
    
    float ThreatLevel = CalculateThreatLevel(CurrentTarget);
    float DistanceToTarget = FVector::Dist(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
    
    // Decision making based on threat level and distance
    if (ThreatLevel > 7.0f && DistanceToTarget < 500.0f)
    {
        // High threat, close range - retreat or take cover
        if (FMath::RandFloat() < 0.6f)
        {
            SetBehaviorState(EAIBehaviorState::Retreat);
        }
        else
        {
            SetBehaviorState(EAIBehaviorState::TakeCover);
        }
    }
    else if (DistanceToTarget > 1500.0f && TacticalData.TeamworkFactor > 0.7f)
    {
        // Long range, good teamwork - try flanking
        if (FMath::RandFloat() < 0.4f)
        {
            SetBehaviorState(EAIBehaviorState::Flank);
        }
    }
    else if (ThreatLevel > 5.0f && FMath::RandFloat() < 0.3f)
    {
        // Moderate threat - call for backup
        SetBehaviorState(EAIBehaviorState::CallForBackup);
    }
}

void UAdvancedAISystem::UpdateMovementLogic(float DeltaTime)
{
    // Update movement based on current state
    APawn* PawnOwner = Cast<APawn>(GetOwner());
    if (!PawnOwner) return;
    
    UCharacterMovementComponent* MovementComp = PawnOwner->FindComponentByClass<UCharacterMovementComponent>();
    if (!MovementComp) return;
    
    // Adjust movement speed based on state
    switch (CurrentBehaviorState)
    {
        case EAIBehaviorState::Patrol:
            MovementComp->MaxWalkSpeed = 200.0f;
            break;
        case EAIBehaviorState::Combat:
            MovementComp->MaxWalkSpeed = 300.0f;
            break;
        case EAIBehaviorState::Retreat:
            MovementComp->MaxWalkSpeed = 600.0f;
            break;
        case EAIBehaviorState::Search:
            MovementComp->MaxWalkSpeed = 250.0f;
            break;
        default:
            MovementComp->MaxWalkSpeed = 400.0f;
            break;
    }
}

void UAdvancedAISystem::ProcessPerceptionData()
{
    if (!PerceptionComponent) return;
    
    TArray<AActor*> PerceivedActors;
    PerceptionComponent->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);
    
    for (AActor* Actor : PerceivedActors)
    {
        if (Actor && Actor != GetOwner())
        {
            // Add to interest points if not already tracking
            FVector ActorLocation = Actor->GetActorLocation();
            if (!AIMemory.InterestPoints.Contains(ActorLocation))
            {
                AIMemory.InterestPoints.Add(ActorLocation);
            }
            
            // Update threat assessment
            float ThreatLevel = CalculateThreatLevel(Actor);
            AIMemory.ThreatLevels.Add(Actor, ThreatLevel);
        }
    }
}

void UAdvancedAISystem::MakeTacticalDecision()
{
    // High-level tactical decision making
    if (AIMemory.bIsInCombat && CurrentTarget)
    {
        float RandomValue = FMath::RandFloat();
        
        // Personality-based decision making
        switch (Personality)
        {
            case EAIPersonality::Aggressive:
                if (RandomValue < 0.6f)
                {
                    SetBehaviorState(EAIBehaviorState::Combat);
                }
                else if (RandomValue < 0.8f)
                {
                    SetBehaviorState(EAIBehaviorState::Flank);
                }
                else
                {
                    SetBehaviorState(EAIBehaviorState::Suppress);
                }
                break;
                
            case EAIPersonality::Defensive:
                if (RandomValue < 0.7f)
                {
                    SetBehaviorState(EAIBehaviorState::TakeCover);
                }
                else
                {
                    SetBehaviorState(EAIBehaviorState::Combat);
                }
                break;
                
            case EAIPersonality::Tactical:
                if (RandomValue < 0.3f)
                {
                    SetBehaviorState(EAIBehaviorState::Flank);
                }
                else if (RandomValue < 0.6f)
                {
                    SetBehaviorState(EAIBehaviorState::TakeCover);
                }
                else if (RandomValue < 0.8f)
                {
                    SetBehaviorState(EAIBehaviorState::Combat);
                }
                else
                {
                    SetBehaviorState(EAIBehaviorState::CallForBackup);
                }
                break;
        }
    }
}

bool UAdvancedAISystem::IsInCover()
{
    if (!CurrentTarget) return false;
    
    return !ValidateLineOfSight(
        GetOwner()->GetActorLocation() + FVector(0, 0, 100),
        CurrentTarget->GetActorLocation() + FVector(0, 0, 100),
        GetOwner()
    );
}

void UAdvancedAISystem::AimAtTarget(AActor* Target)
{
    if (!Target || !AIController) return;
    
    FVector PredictedLocation = PredictTargetLocation(Target, 0.5f);
    FVector AimDirection = (PredictedLocation - GetOwner()->GetActorLocation()).GetSafeNormal();
    
    // Apply accuracy modifier based on difficulty
    if (TacticalData.AccuracyModifier < 1.0f)
    {
        // Add some inaccuracy for easier difficulties
        float InaccuracyAngle = (1.0f - TacticalData.AccuracyModifier) * 10.0f; // Max 10 degrees
        FVector RandomOffset = FMath::VRand() * FMath::Sin(FMath::DegreesToRadians(InaccuracyAngle));
        AimDirection = (AimDirection + RandomOffset).GetSafeNormal();
    }
    
    FRotator AimRotation = AimDirection.Rotation();
    AIController->SetControlRotation(AimRotation);
}

void UAdvancedAISystem::FireWeapon()
{
    if (CurrentWeapon && GetWorld()->GetTimeSeconds() - LastFireTime > 0.1f) // Rate limit
    {
        // Use weapon system to fire
        // CurrentWeapon->Fire(); // Uncomment when weapon system is integrated
        
        LastFireTime = GetWorld()->GetTimeSeconds();
        
        UE_LOG(LogTemp, Log, TEXT("AI firing weapon"));
    }
}

void UAdvancedAISystem::ReloadWeapon()
{
    if (CurrentWeapon)
    {
        // Use weapon system to reload
        // CurrentWeapon->Reload(); // Uncomment when weapon system is integrated
        
        UE_LOG(LogTemp, Log, TEXT("AI reloading weapon"));
    }
}
