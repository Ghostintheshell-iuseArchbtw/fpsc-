#include "PerformanceOptimizationSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "HAL/PlatformMemory.h"
#include "Stats/Stats.h"
#include "Async/ParallelFor.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

APerformanceOptimizationSystem::APerformanceOptimizationSystem()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.016f; // 60 FPS tick rate
    
    // Initialize default settings
    OptimizationSettings.bEnableLODSystem = true;
    OptimizationSettings.bEnableFrustumCulling = true;
    OptimizationSettings.bEnableOcclusionCulling = true;
    OptimizationSettings.bEnableObjectPooling = true;
    OptimizationSettings.bEnableAsyncProcessing = true;
    OptimizationSettings.bEnableGarbageCollection = true;
    OptimizationSettings.bEnablePerformanceMonitoring = true;
    OptimizationSettings.bEnableThermalMonitoring = true;
    OptimizationSettings.bEnablePredictiveOptimization = true;
    OptimizationSettings.bEnableGPUProfiling = true;
    
    // Initialize frame time history
    FrameTimeHistory.SetNum(FrameTimeHistorySize);
    for (int32 i = 0; i < FrameTimeHistorySize; ++i)
    {
        FrameTimeHistory[i] = 0.016f; // Default to 60 FPS
    }

    // Initialize thermal history
    CPUTemperatureHistory.SetNum(ThermalHistorySize);
    GPUTemperatureHistory.SetNum(ThermalHistorySize);
    for (int32 i = 0; i < ThermalHistorySize; ++i)
    {
        CPUTemperatureHistory[i] = 45.0f; // Default safe temperature
        GPUTemperatureHistory[i] = 50.0f;
    }

    // Initialize GPU tracking
    GPUUsageHistory.SetNum(FrameTimeHistorySize);
    GPUMemoryHistory.SetNum(FrameTimeHistorySize);
}

void APerformanceOptimizationSystem::BeginPlay()
{
    Super::BeginPlay();
    InitializeSystem();
}

void APerformanceOptimizationSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    ProcessFrameTimeHistory(DeltaTime);
    
    // Update LOD system
    if (OptimizationSettings.bEnableLODSystem)
    {
        LODUpdateTimer += DeltaTime;
        if (LODUpdateTimer >= 0.1f) // Update LOD every 100ms
        {
            UpdateLODSystem();
            LODUpdateTimer = 0.0f;
        }
    }
    
    // Update culling system
    if (OptimizationSettings.bEnableFrustumCulling || OptimizationSettings.bEnableOcclusionCulling)
    {
        CullingUpdateTimer += DeltaTime;
        if (CullingUpdateTimer >= 0.05f) // Update culling every 50ms
        {
            UpdateCullingSystem();
            CullingUpdateTimer = 0.0f;
        }
    }
    
    // Update performance monitoring
    if (OptimizationSettings.bEnablePerformanceMonitoring)
    {
        PerformanceUpdateTimer += DeltaTime;
        if (PerformanceUpdateTimer >= OptimizationSettings.MonitoringUpdateInterval)
        {
            UpdatePerformanceMetrics();
            
            // Update thermal monitoring
            if (OptimizationSettings.bEnableThermalMonitoring)
            {
                UpdateThermalMetrics();
            }
            
            // Update GPU profiling
            if (OptimizationSettings.bEnableGPUProfiling)
            {
                UpdateGPUMetrics();
            }
            
            // Update predictive optimization
            if (OptimizationSettings.bEnablePredictiveOptimization)
            {
                UpdatePredictiveMetrics();
            }
            
            PerformanceUpdateTimer = 0.0f;
        }
    }
    
    // Garbage collection
    if (OptimizationSettings.bEnableGarbageCollection)
    {
        GCTimer += DeltaTime;
        if (GCTimer >= OptimizationSettings.GCInterval)
        {
            CollectGarbageIfNeeded();
            GCTimer = 0.0f;
        }
    }
    
    // Pool cleanup
    if (OptimizationSettings.bEnableObjectPooling)
    {
        PoolCleanupTimer += DeltaTime;
        if (PoolCleanupTimer >= OptimizationSettings.PoolCleanupInterval)
        {
            CleanupPools();
            PoolCleanupTimer = 0.0f;
        }
    }
    
    // Process async tasks
    if (OptimizationSettings.bEnableAsyncProcessing)
    {
        ProcessAsyncTasks();
    }
    
    // Auto-optimize based on performance
    if (!IsPerformanceTargetMet())
    {
        OptimizeBasedOnPerformance();
        
        // Apply thermal throttling if needed
        if (OptimizationSettings.bEnableThermalMonitoring && IsThermalThrottlingNeeded())
        {
            ApplyThermalThrottling();
        }
        
        // Apply predictive optimizations
        if (OptimizationSettings.bEnablePredictiveOptimization)
        {
            ApplyPredictiveOptimizations();
        }
    }
}

void APerformanceOptimizationSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up async tasks
    for (auto& Task : AsyncTasks)
    {
        if (Task.IsValid())
        {
            Task.Wait();
        }
    }
    AsyncTasks.Empty();
    
    Super::EndPlay(EndPlayReason);
}

void APerformanceOptimizationSystem::InitializeSystem()
{
    UE_LOG(LogTemp, Warning, TEXT("Initializing Performance Optimization System"));
    
    // Initialize object pools for common objects
    InitializePool(AStaticMeshActor::StaticClass(), OptimizationSettings.DefaultPoolSize);
    
    // Load optimization settings from config
    LoadOptimizationSettings();
}

void APerformanceOptimizationSystem::UpdateLODSystem()
{
    if (!OptimizationSettings.bEnableLODSystem) return;
    
    // Process LOD updates asynchronously
    if (OptimizationSettings.bEnableAsyncProcessing)
    {
        AddAsyncTask([this]()
        {
            for (auto& LODPair : RegisteredLODActors)
            {
                if (IsValid(LODPair.Key))
                {
                    UpdateLODForActor(LODPair.Key, LODPair.Value);
                }
            }
        });
    }
    else
    {
        // Process synchronously
        for (auto& LODPair : RegisteredLODActors)
        {
            if (IsValid(LODPair.Key))
            {
                UpdateLODForActor(LODPair.Key, LODPair.Value);
            }
        }
    }
}

void APerformanceOptimizationSystem::RegisterActorForLOD(AActor* Actor, const FLODSettings& LODSettings)
{
    if (!IsValid(Actor)) return;
    
    RegisteredLODActors.Add(Actor, LODSettings);
}

void APerformanceOptimizationSystem::UnregisterActorFromLOD(AActor* Actor)
{
    if (!IsValid(Actor)) return;
    
    RegisteredLODActors.Remove(Actor);
}

int32 APerformanceOptimizationSystem::CalculateLODLevel(AActor* Actor, const FLODSettings& LODSettings)
{
    if (!IsValid(Actor)) return 0;
    
    float Distance = CalculateDistanceToPlayer(Actor);
    
    if (Distance > LODSettings.CullDistance)
    {
        return -1; // Cull the actor
    }
    else if (Distance > LODSettings.LOD3Distance)
    {
        return 3;
    }
    else if (Distance > LODSettings.LOD2Distance)
    {
        return 2;
    }
    else if (Distance > LODSettings.LOD1Distance)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void APerformanceOptimizationSystem::UpdateLODForActor(AActor* Actor, const FLODSettings& LODSettings)
{
    if (!IsValid(Actor)) return;
    
    int32 LODLevel = CalculateLODLevel(Actor, LODSettings);
    
    if (LODLevel == -1)
    {
        CullActor(Actor);
    }
    else
    {
        UncullActor(Actor);
        SetActorLODLevel(Actor, LODLevel);
    }
}

void APerformanceOptimizationSystem::SetActorLODLevel(AActor* Actor, int32 LODLevel)
{
    if (!IsValid(Actor)) return;
    
    // Set LOD for static mesh components
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
    
    for (UStaticMeshComponent* MeshComp : StaticMeshComponents)
    {
        if (IsValid(MeshComp))
        {
            MeshComp->SetForcedLodModel(LODLevel + 1); // UE uses 1-based LOD indexing
        }
    }
    
    // Set LOD for skeletal mesh components
    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
    
    for (USkeletalMeshComponent* MeshComp : SkeletalMeshComponents)
    {
        if (IsValid(MeshComp))
        {
            MeshComp->SetForcedLOD(LODLevel + 1);
        }
    }
}

AActor* APerformanceOptimizationSystem::GetPooledObject(UClass* ObjectClass)
{
    if (!ObjectClass) return nullptr;
    
    TArray<FPooledObject>* Pool = ObjectPools.Find(ObjectClass);
    if (!Pool) return nullptr;
    
    // Find an unused object
    for (FPooledObject& PooledObj : *Pool)
    {
        if (!PooledObj.bInUse && IsValid(PooledObj.Actor))
        {
            PooledObj.bInUse = true;
            PooledObj.LastUsedTime = GetWorld()->GetTimeSeconds();
            PooledObj.Actor->SetActorHiddenInGame(false);
            PooledObj.Actor->SetActorEnableCollision(true);
            return PooledObj.Actor;
        }
    }
    
    // No available object, create new one if pool isn't full
    if (Pool->Num() < OptimizationSettings.DefaultPoolSize * 2) // Allow pool growth
    {
        if (AActor* NewActor = GetWorld()->SpawnActor<AActor>(ObjectClass))
        {
            FPooledObject NewPooledObj;
            NewPooledObj.Actor = NewActor;
            NewPooledObj.bInUse = true;
            NewPooledObj.LastUsedTime = GetWorld()->GetTimeSeconds();
            Pool->Add(NewPooledObj);
            return NewActor;
        }
    }
    
    return nullptr;
}

void APerformanceOptimizationSystem::ReturnPooledObject(AActor* Object)
{
    if (!IsValid(Object)) return;
    
    UClass* ObjectClass = Object->GetClass();
    TArray<FPooledObject>* Pool = ObjectPools.Find(ObjectClass);
    if (!Pool) return;
    
    // Find the object in the pool
    for (FPooledObject& PooledObj : *Pool)
    {
        if (PooledObj.Actor == Object)
        {
            PooledObj.bInUse = false;
            PooledObj.LastUsedTime = GetWorld()->GetTimeSeconds();
            
            // Reset object state
            Object->SetActorHiddenInGame(true);
            Object->SetActorEnableCollision(false);
            Object->SetActorLocation(FVector::ZeroVector);
            Object->SetActorRotation(FRotator::ZeroRotator);
            break;
        }
    }
}

void APerformanceOptimizationSystem::InitializePool(UClass* ObjectClass, int32 PoolSize)
{
    if (!ObjectClass || !GetWorld()) return;
    
    TArray<FPooledObject>& Pool = ObjectPools.FindOrAdd(ObjectClass);
    Pool.Empty();
    
    for (int32 i = 0; i < PoolSize; ++i)
    {
        if (AActor* NewActor = GetWorld()->SpawnActor<AActor>(ObjectClass))
        {
            FPooledObject PooledObj;
            PooledObj.Actor = NewActor;
            PooledObj.bInUse = false;
            PooledObj.LastUsedTime = 0.0f;
            
            // Hide the object initially
            NewActor->SetActorHiddenInGame(true);
            NewActor->SetActorEnableCollision(false);
            
            Pool.Add(PooledObj);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Initialized object pool for %s with %d objects"), 
           *ObjectClass->GetName(), Pool.Num());
}

void APerformanceOptimizationSystem::CleanupPools()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float CleanupThreshold = 300.0f; // 5 minutes
    
    for (auto& PoolPair : ObjectPools)
    {
        TArray<FPooledObject>& Pool = PoolPair.Value;
        
        // Remove unused objects that haven't been used for a while
        for (int32 i = Pool.Num() - 1; i >= 0; --i)
        {
            FPooledObject& PooledObj = Pool[i];
            
            if (!PooledObj.bInUse && 
                (CurrentTime - PooledObj.LastUsedTime) > CleanupThreshold)
            {
                if (IsValid(PooledObj.Actor))
                {
                    PooledObj.Actor->Destroy();
                }
                Pool.RemoveAt(i);
            }
        }
    }
}

void APerformanceOptimizationSystem::UpdateCullingSystem()
{
    if (!OptimizationSettings.bEnableFrustumCulling && !OptimizationSettings.bEnableOcclusionCulling) 
        return;
    
    // Clear previous frame data
    VisibleActors.Empty();
    
    // Get all relevant actors
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
    
    // Process culling asynchronously
    if (OptimizationSettings.bEnableAsyncProcessing && AllActors.Num() > 100)
    {
        AddAsyncTask([this, AllActors]()
        {
            for (AActor* Actor : AllActors)
            {
                if (IsValid(Actor) && Actor != this)
                {
                    bool bShouldCull = false;
                    
                    // Distance culling
                    float Distance = CalculateDistanceToPlayer(Actor);
                    if (Distance > OptimizationSettings.MaxCullingDistance)
                    {
                        bShouldCull = true;
                    }
                    
                    // Frustum culling
                    if (!bShouldCull && OptimizationSettings.bEnableFrustumCulling)
                    {
                        if (!IsActorInViewFrustum(Actor))
                        {
                            bShouldCull = true;
                        }
                    }
                    
                    // Occlusion culling
                    if (!bShouldCull && OptimizationSettings.bEnableOcclusionCulling)
                    {
                        if (IsActorOccluded(Actor))
                        {
                            bShouldCull = true;
                        }
                    }
                    
                    if (bShouldCull)
                    {
                        CullActor(Actor);
                    }
                    else
                    {
                        UncullActor(Actor);
                        VisibleActors.Add(Actor);
                    }
                }
            }
        });
    }
    else
    {
        // Process synchronously
        for (AActor* Actor : AllActors)
        {
            if (IsValid(Actor) && Actor != this)
            {
                bool bShouldCull = false;
                
                float Distance = CalculateDistanceToPlayer(Actor);
                if (Distance > OptimizationSettings.MaxCullingDistance)
                {
                    bShouldCull = true;
                }
                
                if (!bShouldCull && OptimizationSettings.bEnableFrustumCulling)
                {
                    if (!IsActorInViewFrustum(Actor))
                    {
                        bShouldCull = true;
                    }
                }
                
                if (!bShouldCull && OptimizationSettings.bEnableOcclusionCulling)
                {
                    if (IsActorOccluded(Actor))
                    {
                        bShouldCull = true;
                    }
                }
                
                if (bShouldCull)
                {
                    CullActor(Actor);
                }
                else
                {
                    UncullActor(Actor);
                    VisibleActors.Add(Actor);
                }
            }
        }
    }
}

bool APerformanceOptimizationSystem::IsActorInViewFrustum(AActor* Actor)
{
    if (!IsValid(Actor)) return false;
    
    APlayerController* PC = GetPlayerController();
    if (!PC || !PC->GetPawn()) return true; // Default to visible if no player
    
    FVector ActorLocation = Actor->GetActorLocation();
    FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
    FVector ViewDirection = PC->GetControlRotation().Vector();
    
    // Simple frustum check - check if actor is within view angle
    FVector ToActor = (ActorLocation - PlayerLocation).GetSafeNormal();
    float DotProduct = FVector::DotProduct(ViewDirection, ToActor);
    
    // FOV check (assuming 90 degree FOV)
    return DotProduct > FMath::Cos(FMath::DegreesToRadians(45.0f));
}

bool APerformanceOptimizationSystem::IsActorOccluded(AActor* Actor)
{
    if (!IsValid(Actor)) return false;
    
    APlayerController* PC = GetPlayerController();
    if (!PC || !PC->GetPawn()) return false;
    
    FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
    FVector ActorLocation = Actor->GetActorLocation();
    
    // Perform line trace to check for occlusion
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(PC->GetPawn());
    QueryParams.AddIgnoredActor(Actor);
    
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        PlayerLocation,
        ActorLocation,
        ECC_Visibility,
        QueryParams
    );
    
    return bHit; // If we hit something, the actor is occluded
}

void APerformanceOptimizationSystem::CullActor(AActor* Actor)
{
    if (!IsValid(Actor)) return;
    
    Actor->SetActorHiddenInGame(true);
    Actor->SetActorTickEnabled(false);
    
    if (!CulledActors.Contains(Actor))
    {
        CulledActors.Add(Actor);
    }
}

void APerformanceOptimizationSystem::UncullActor(AActor* Actor)
{
    if (!IsValid(Actor)) return;
    
    Actor->SetActorHiddenInGame(false);
    Actor->SetActorTickEnabled(true);
    
    CulledActors.Remove(Actor);
}

void APerformanceOptimizationSystem::UpdatePerformanceMetrics()
{
    // Update frame rate and timing
    CurrentMetrics.FrameTime = GetAverageFrameTime();
    CurrentMetrics.FrameRate = 1.0f / CurrentMetrics.FrameTime;
    
    // Update memory usage
    CurrentMetrics.MemoryUsageMB = GetMemoryUsageMB();
    
    // Update actor counts
    CurrentMetrics.VisibleActors = VisibleActors.Num();
    
    // Get rendering stats
    CurrentMetrics.DrawCalls = 0; // Would need access to rendering stats
    CurrentMetrics.TriangleCount = 0; // Would need access to rendering stats
    
    // Broadcast performance change event
    OnPerformanceChanged.Broadcast(CurrentMetrics);
    
    // Log performance metrics periodically
    static float LogTimer = 0.0f;
    LogTimer += OptimizationSettings.MonitoringUpdateInterval;
    if (LogTimer >= 10.0f) // Log every 10 seconds
    {
        LogPerformanceMetrics();
        LogTimer = 0.0f;
    }
}

bool APerformanceOptimizationSystem::IsPerformanceTargetMet() const
{
    return CurrentMetrics.FrameRate >= OptimizationSettings.TargetFrameRate * 0.9f && // 90% of target
           CurrentMetrics.MemoryUsageMB <= OptimizationSettings.MaxMemoryUsageMB;
}

void APerformanceOptimizationSystem::OptimizeBasedOnPerformance()
{
    if (CurrentMetrics.FrameRate < OptimizationSettings.TargetFrameRate * 0.8f)
    {
        // Performance is significantly below target
        ApplyLowPerformanceOptimizations();
    }
    else if (CurrentMetrics.FrameRate > OptimizationSettings.TargetFrameRate * 1.1f)
    {
        // Performance is above target, can afford higher quality
        ApplyHighPerformanceOptimizations();
    }
}

void APerformanceOptimizationSystem::ApplyLowPerformanceOptimizations()
{
    // Reduce LOD distances
    OptimizeLODSettings();
    
    // Increase culling aggressiveness
    OptimizeCullingSettings();
    
    // Reduce pool sizes
    OptimizePoolSizes();
    
    // Force garbage collection
    ForceGarbageCollection();
    
    OnOptimizationApplied.Broadcast(1); // Low performance optimization level
}

void APerformanceOptimizationSystem::ApplyHighPerformanceOptimizations()
{
    // Increase LOD distances for better quality
    for (auto& LODPair : RegisteredLODActors)
    {
        LODPair.Value.LOD1Distance *= 1.1f;
        LODPair.Value.LOD2Distance *= 1.1f;
        LODPair.Value.LOD3Distance *= 1.1f;
    }
    
    OnOptimizationApplied.Broadcast(3); // High performance optimization level
}

void APerformanceOptimizationSystem::OptimizeLODSettings()
{
    for (auto& LODPair : RegisteredLODActors)
    {
        LODPair.Value.LOD1Distance *= 0.9f;
        LODPair.Value.LOD2Distance *= 0.9f;
        LODPair.Value.LOD3Distance *= 0.9f;
        LODPair.Value.CullDistance *= 0.9f;
    }
}

void APerformanceOptimizationSystem::OptimizeCullingSettings()
{
    OptimizationSettings.MaxCullingDistance *= 0.9f;
}

void APerformanceOptimizationSystem::OptimizePoolSizes()
{
    for (auto& PoolPair : ObjectPools)
    {
        TArray<FPooledObject>& Pool = PoolPair.Value;
        
        // Remove excess unused objects
        for (int32 i = Pool.Num() - 1; i >= OptimizationSettings.DefaultPoolSize; --i)
        {
            if (!Pool[i].bInUse && IsValid(Pool[i].Actor))
            {
                Pool[i].Actor->Destroy();
                Pool.RemoveAt(i);
            }
        }
    }
}

void APerformanceOptimizationSystem::ForceGarbageCollection()
{
    GEngine->ForceGarbageCollection(true);
}

float APerformanceOptimizationSystem::GetMemoryUsageMB() const
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    return MemStats.UsedPhysical / (1024.0f * 1024.0f);
}

void APerformanceOptimizationSystem::OptimizeMemoryUsage()
{
    // Force garbage collection
    ForceGarbageCollection();
    
    // Clean up object pools
    CleanupPools();
    
    // Remove invalid actors from tracking
    for (auto It = RegisteredLODActors.CreateIterator(); It; ++It)
    {
        if (!IsValid(It.Key()))
        {
            It.RemoveCurrent();
        }
    }
    
    CulledActors.RemoveAll([](AActor* Actor) { return !IsValid(Actor); });
    VisibleActors.RemoveAll([](AActor* Actor) { return !IsValid(Actor); });
}

void APerformanceOptimizationSystem::ProcessAsyncTasks()
{
    FScopeLock Lock(&TaskCriticalSection);
    
    // Remove completed tasks
    for (int32 i = AsyncTasks.Num() - 1; i >= 0; --i)
    {
        if (AsyncTasks[i].IsReady())
        {
            AsyncTasks.RemoveAt(i);
        }
    }
    
    // Limit number of concurrent tasks
    if (AsyncTasks.Num() > OptimizationSettings.MaxAsyncTasks)
    {
        // Wait for oldest task to complete
        if (AsyncTasks[0].IsValid())
        {
            AsyncTasks[0].Wait();
        }
        AsyncTasks.RemoveAt(0);
    }
}

void APerformanceOptimizationSystem::AddAsyncTask(TFunction<void()> Task)
{
    if (!OptimizationSettings.bEnableAsyncProcessing) return;
    
    FScopeLock Lock(&TaskCriticalSection);
    
    TFuture<void> Future = Async(EAsyncExecution::ThreadPool, Task);
    AsyncTasks.Add(MoveTemp(Future));
}

void APerformanceOptimizationSystem::SetOptimizationLevel(int32 Level)
{
    switch (Level)
    {
        case 1: // Low performance
            ApplyLowPerformanceOptimizations();
            break;
        case 2: // Medium performance
            ResetToDefaultSettings();
            break;
        case 3: // High performance
            ApplyHighPerformanceOptimizations();
            break;
        default:
            ResetToDefaultSettings();
            break;
    }
}

void APerformanceOptimizationSystem::ResetToDefaultSettings()
{
    // Reset to default optimization settings
    OptimizationSettings = FOptimizationSettings();
    LoadOptimizationSettings();
}

void APerformanceOptimizationSystem::SaveOptimizationSettings()
{
    // Save settings to config file
    // Implementation would use config system or save game
    UE_LOG(LogTemp, Warning, TEXT("Saving optimization settings"));
}

void APerformanceOptimizationSystem::LoadOptimizationSettings()
{
    // Load settings from config file
    // Implementation would use config system or save game
    UE_LOG(LogTemp, Warning, TEXT("Loading optimization settings"));
}

float APerformanceOptimizationSystem::CalculateDistanceToPlayer(AActor* Actor)
{
    if (!IsValid(Actor)) return 0.0f;
    
    APlayerController* PC = GetPlayerController();
    if (!PC || !PC->GetPawn()) return 0.0f;
    
    return FVector::Dist(Actor->GetActorLocation(), PC->GetPawn()->GetActorLocation());
}

APlayerController* APerformanceOptimizationSystem::GetPlayerController()
{
    return GetWorld()->GetFirstPlayerController();
}

void APerformanceOptimizationSystem::CollectGarbageIfNeeded()
{
    if (CurrentMetrics.MemoryUsageMB > OptimizationSettings.MaxMemoryUsageMB * 0.8f)
    {
        ForceGarbageCollection();
    }
}

void APerformanceOptimizationSystem::ProcessFrameTimeHistory(float DeltaTime)
{
    FrameTimeHistory[FrameTimeHistoryIndex] = DeltaTime;
    FrameTimeHistoryIndex = (FrameTimeHistoryIndex + 1) % FrameTimeHistorySize;
}

float APerformanceOptimizationSystem::GetAverageFrameTime() const
{
    float Total = 0.0f;
    for (float FrameTime : FrameTimeHistory)
    {
        Total += FrameTime;
    }
    return Total / FrameTimeHistorySize;
}

void APerformanceOptimizationSystem::LogPerformanceMetrics() const
{
    UE_LOG(LogTemp, Warning, TEXT("Performance Metrics - FPS: %.1f, Memory: %.1fMB, Visible Actors: %d"), 
           CurrentMetrics.FrameRate, CurrentMetrics.MemoryUsageMB, CurrentMetrics.VisibleActors);
    
    if (OptimizationSettings.bEnableThermalMonitoring)
    {
        UE_LOG(LogTemp, Warning, TEXT("Thermal Metrics - CPU: %.1f°C, GPU: %.1f°C"), 
               CurrentMetrics.CPUTemperature, CurrentMetrics.GPUTemperature);
    }
    
    if (OptimizationSettings.bEnableGPUProfiling)
    {
        UE_LOG(LogTemp, Warning, TEXT("GPU Metrics - Utilization: %.1f%%, Memory: %.1fMB"), 
               CurrentMetrics.GPUUtilization, CurrentMetrics.GPUMemoryUsageMB);
    }
}

// Thermal monitoring implementation
void APerformanceOptimizationSystem::UpdateThermalMetrics()
{
    // Simulate thermal monitoring (in production, use platform-specific APIs)
    float CPUTemp = GetCPUTemperature();
    float GPUTemp = GetGPUTemperature();
    
    // Update thermal history
    CPUTemperatureHistory[ThermalHistoryIndex] = CPUTemp;
    GPUTemperatureHistory[ThermalHistoryIndex] = GPUTemp;
    ThermalHistoryIndex = (ThermalHistoryIndex + 1) % ThermalHistorySize;
    
    // Update current metrics
    CurrentMetrics.CPUTemperature = CPUTemp;
    CurrentMetrics.GPUTemperature = GPUTemp;
    
    // Calculate thermal throttling risk
    CurrentMetrics.ThermalThrottlingRisk = FMath::Max(
        (CPUTemp - 60.0f) / (OptimizationSettings.CPUThermalThreshold - 60.0f),
        (GPUTemp - 65.0f) / (OptimizationSettings.GPUThermalThreshold - 65.0f)
    );
    CurrentMetrics.ThermalThrottlingRisk = FMath::Clamp(CurrentMetrics.ThermalThrottlingRisk, 0.0f, 1.0f);
}

float APerformanceOptimizationSystem::GetCPUTemperature() const
{
    // Simulate CPU temperature based on frame time and usage
    float BaseTemp = 45.0f;
    float LoadFactor = FMath::Clamp(CurrentMetrics.FrameTime / 0.033f, 0.0f, 2.0f); // 30fps = high load
    return BaseTemp + (LoadFactor * 25.0f) + FMath::RandRange(-2.0f, 2.0f);
}

float APerformanceOptimizationSystem::GetGPUTemperature() const
{
    // Simulate GPU temperature based on draw calls and frame time
    float BaseTemp = 50.0f;
    float LoadFactor = FMath::Clamp(CurrentMetrics.DrawCalls / 3000.0f, 0.0f, 2.0f);
    return BaseTemp + (LoadFactor * 30.0f) + FMath::RandRange(-3.0f, 3.0f);
}

bool APerformanceOptimizationSystem::IsThermalThrottlingNeeded() const
{
    return CurrentMetrics.CPUTemperature > OptimizationSettings.CPUThermalThreshold ||
           CurrentMetrics.GPUTemperature > OptimizationSettings.GPUThermalThreshold;
}

// GPU profiling implementation
void APerformanceOptimizationSystem::UpdateGPUMetrics()
{
    float GPUUsage = GetGPUUtilization();
    float GPUMemory = GetGPUMemoryUsage();
    
    // Update GPU history
    GPUUsageHistory[FrameTimeHistoryIndex] = GPUUsage;
    GPUMemoryHistory[FrameTimeHistoryIndex] = GPUMemory;
    
    // Update current metrics
    CurrentMetrics.GPUUtilization = GPUUsage;
    CurrentMetrics.GPUMemoryUsageMB = GPUMemory;
}

float APerformanceOptimizationSystem::GetGPUMemoryUsage() const
{
    // Simulate GPU memory usage based on visible actors and effects
    float BaseMemory = 1024.0f; // 1GB base
    float ActorMemory = CurrentMetrics.VisibleActors * 2.0f; // 2MB per visible actor
    float EffectMemory = CurrentMetrics.DrawCalls * 0.5f; // 0.5MB per draw call
    return BaseMemory + ActorMemory + EffectMemory + FMath::RandRange(-50.0f, 50.0f);
}

float APerformanceOptimizationSystem::GetGPUUtilization() const
{
    // Simulate GPU utilization based on frame time and complexity
    float BaseUtilization = 30.0f;
    float FrameTimeUtilization = FMath::Clamp(CurrentMetrics.FrameTime / 0.016f, 0.0f, 1.0f) * 50.0f;
    float ComplexityUtilization = FMath::Clamp(CurrentMetrics.DrawCalls / 2000.0f, 0.0f, 1.0f) * 20.0f;
    return FMath::Clamp(BaseUtilization + FrameTimeUtilization + ComplexityUtilization, 0.0f, 100.0f);
}

// Predictive optimization implementation
void APerformanceOptimizationSystem::UpdatePredictiveMetrics()
{
    CurrentMetrics.PredictedFrameDropRisk = PredictFrameDropRisk();
    
    // Store prediction for trend analysis
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastPredictionTime >= 1.0f)
    {
        FrameDropPredictions.Add(CurrentMetrics.PredictedFrameDropRisk);
        if (FrameDropPredictions.Num() > 30) // Keep last 30 seconds
        {
            FrameDropPredictions.RemoveAt(0);
        }
        LastPredictionTime = CurrentTime;
    }
}

float APerformanceOptimizationSystem::PredictFrameDropRisk() const
{
    if (FrameTimeHistory.Num() < 10) return 0.0f;
    
    // Analyze frame time trend
    float RecentAverage = 0.0f;
    float OlderAverage = 0.0f;
    int32 RecentSamples = FMath::Min(10, FrameTimeHistory.Num());
    int32 OlderSamples = FMath::Min(20, FrameTimeHistory.Num() - RecentSamples);
    
    // Calculate recent average (last 10 frames)
    for (int32 i = 0; i < RecentSamples; ++i)
    {
        int32 Index = (FrameTimeHistoryIndex - 1 - i + FrameTimeHistorySize) % FrameTimeHistorySize;
        RecentAverage += FrameTimeHistory[Index];
    }
    RecentAverage /= RecentSamples;
    
    // Calculate older average (frames 10-30)
    for (int32 i = RecentSamples; i < RecentSamples + OlderSamples; ++i)
    {
        int32 Index = (FrameTimeHistoryIndex - 1 - i + FrameTimeHistorySize) % FrameTimeHistorySize;
        OlderAverage += FrameTimeHistory[Index];
    }
    OlderAverage /= OlderSamples;
    
    // Calculate trend and risk
    float Trend = (RecentAverage - OlderAverage) / OlderAverage;
    float FrameTimeRisk = FMath::Clamp((RecentAverage - 0.016f) / 0.017f, 0.0f, 1.0f);
    float TrendRisk = FMath::Clamp(Trend * 10.0f, 0.0f, 1.0f);
    
    return FMath::Clamp(FrameTimeRisk + TrendRisk + CurrentMetrics.ThermalThrottlingRisk, 0.0f, 1.0f);
}

void APerformanceOptimizationSystem::ApplyPredictiveOptimizations()
{
    if (CurrentMetrics.PredictedFrameDropRisk > 0.7f)
    {
        // High risk - apply aggressive optimizations
        ApplyLowPerformanceOptimizations();
        
        UE_LOG(LogTemp, Warning, TEXT("Applying predictive optimizations due to high frame drop risk: %.2f"), 
               CurrentMetrics.PredictedFrameDropRisk);
    }
    else if (CurrentMetrics.PredictedFrameDropRisk > 0.4f)
    {
        // Medium risk - apply moderate optimizations
        OptimizeLODSettings();
        OptimizeCullingSettings();
    }
}

void APerformanceOptimizationSystem::ApplyThermalThrottling()
{
    UE_LOG(LogTemp, Warning, TEXT("Applying thermal throttling - CPU: %.1f°C, GPU: %.1f°C"), 
           CurrentMetrics.CPUTemperature, CurrentMetrics.GPUTemperature);
    
    // Reduce performance to manage thermals
    ApplyLowPerformanceOptimizations();
    
    // Additional thermal-specific optimizations
    OptimizationSettings.TargetFrameRate = FMath::Max(30.0f, OptimizationSettings.TargetFrameRate * 0.8f);
    
    // Force more aggressive culling
    OptimizationSettings.MaxCullingDistance *= 0.7f;
    
    // Reduce pool sizes to lower memory pressure
    OptimizePoolSizes();
}
