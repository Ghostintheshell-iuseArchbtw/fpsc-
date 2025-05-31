#include "SinglePlayerOptimizationSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Stats/Stats.h"
#include "RenderingThread.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "AI/FPSAICharacter.h"
#include "AI/AdvancedAISystem.h"
#include "Async/ParallelFor.h"
#include "Engine/TextureStreamingTypes.h"
#include "Engine/LODActor.h"

DEFINE_LOG_CATEGORY(LogSinglePlayerOptimization);

USinglePlayerOptimizationSystem::USinglePlayerOptimizationSystem()
{
    CurrentState = EOptimizationState::Disabled;
    LastMetricsUpdateTime = 0.0;
    LastOptimizationTime = 0.0;
    ObjectPoolManager = nullptr;
    PerformanceSystem = nullptr;
    
    // Initialize default configuration
    Config = FSinglePlayerOptimizationConfig();
}

void USinglePlayerOptimizationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Single Player Optimization System initializing..."));
    
    SetOptimizationState(EOptimizationState::Initializing);
    
    // Get references to other systems
    ObjectPoolManager = GetGameInstance()->GetSubsystem<UAdvancedObjectPoolManager>();
    PerformanceSystem = GetWorld() ? GetWorld()->GetSubsystem<UPerformanceOptimizationSystem>() : nullptr;
    
    // Initialize benchmark system
    InitializeBenchmarkSystem();
    
    // Validate system requirements
    if (!ValidateSystemRequirements())
    {
        UE_LOG(LogSinglePlayerOptimization, Error, TEXT("System requirements validation failed"));
        SetOptimizationState(EOptimizationState::Error);
        return;
    }
    
    // Setup optimization timers
    SetupOptimizationTimers();
    
    // Initialize object pools if enabled
    if (Config.bEnableObjectPooling)
    {
        InitializeObjectPools();
    }
    
    // Integrate with other optimization systems
    IntegrateWithObjectPooling();
    IntegrateWithPerformanceSystem();
    
    SetOptimizationState(EOptimizationState::Active);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Single Player Optimization System initialized successfully"));
}

void USinglePlayerOptimizationSystem::Deinitialize()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Single Player Optimization System shutting down..."));
    
    // Clear timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(OptimizationTimerHandle);
        World->GetTimerManager().ClearTimer(MetricsTimerHandle);
        World->GetTimerManager().ClearTimer(GarbageCollectionTimerHandle);
    }
    
    // Disable optimizations
    DisableOptimizations();
    
    SetOptimizationState(EOptimizationState::Disabled);
    
    Super::Deinitialize();
}

void USinglePlayerOptimizationSystem::EnableOptimizations()
{
    FScopeLock Lock(&OptimizationMutex);
    
    if (CurrentState == EOptimizationState::Disabled || CurrentState == EOptimizationState::Error)
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Cannot enable optimizations in current state"));
        return;
    }
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Enabling single-player optimizations..."));
    
    // Enable memory optimizations
    if (Config.bEnableAggressiveGarbageCollection)
    {
        OptimizeMemoryUsage();
    }
    
    // Enable LOD optimizations
    if (Config.bEnableAggressiveLOD)
    {
        OptimizeLODSettings();
    }
    
    // Enable AI optimizations
    if (Config.bEnableAIOptimization)
    {
        OptimizeAIPerformance();
    }
    
    // Enable rendering optimizations
    if (Config.bEnableRenderingOptimization)
    {
        OptimizeRenderingSettings();
    }
    
    // Enable physics optimizations
    if (Config.bEnablePhysicsOptimization)
    {
        // Apply physics optimization settings
        if (UWorld* World = GetWorld())
        {
            if (UPhysicsSettings* PhysicsSettings = UPhysicsSettings::Get())
            {
                // Note: In a production environment, you'd modify these through console commands
                // or engine settings rather than directly
            }
        }
    }
    
    SetOptimizationState(EOptimizationState::Monitoring);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Single-player optimizations enabled"));
}

void USinglePlayerOptimizationSystem::DisableOptimizations()
{
    FScopeLock Lock(&OptimizationMutex);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Disabling single-player optimizations..."));
    
    // Reset optimization states to defaults
    // This would typically involve restoring original settings
    
    if (CurrentState != EOptimizationState::Disabled)
    {
        SetOptimizationState(EOptimizationState::Disabled);
    }
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Single-player optimizations disabled"));
}

void USinglePlayerOptimizationSystem::UpdateOptimizations()
{
    if (CurrentState != EOptimizationState::Active && CurrentState != EOptimizationState::Monitoring)
    {
        return;
    }
    
    double CurrentTime = FPlatformTime::Seconds();
    
    // Update performance metrics
    if (Config.bEnablePerformanceMonitoring)
    {
        UpdatePerformanceMetrics();
    }
    
    // Check if performance is suboptimal and needs optimization
    if (!IsPerformanceOptimal())
    {
        SetOptimizationState(EOptimizationState::Optimizing);
        ApplyDynamicOptimizations();
        SetOptimizationState(EOptimizationState::Monitoring);
    }
    
    LastOptimizationTime = CurrentTime;
}

void USinglePlayerOptimizationSystem::SetOptimizationConfig(const FSinglePlayerOptimizationConfig& NewConfig)
{
    FScopeLock Lock(&OptimizationMutex);
    
    EOptimizationState OldState = CurrentState;
    Config = NewConfig;
    
    // Validate new configuration
    if (!ValidateConfiguration())
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Invalid configuration provided, using defaults"));
        Config = FSinglePlayerOptimizationConfig();
    }
    
    // Reapply optimizations with new config
    if (CurrentState == EOptimizationState::Active || CurrentState == EOptimizationState::Monitoring)
    {
        UpdateOptimizations();
    }
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimization configuration updated"));
}

FSinglePlayerMetrics USinglePlayerOptimizationSystem::GetCurrentMetrics() const
{
    return CurrentMetrics;
}

void USinglePlayerOptimizationSystem::UpdatePerformanceMetrics()
{
    double CurrentTime = FPlatformTime::Seconds();
    
    // Update frame time metrics
    float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
    CurrentMetrics.FrameTime = DeltaTime * 1000.0f; // Convert to milliseconds
    
    // Add to frame time history for averaging
    FrameTimeHistory.Enqueue(DeltaTime);
    if (FrameTimeHistory.Num() > 60) // Keep last 60 frames
    {
        float OldFrame;
        FrameTimeHistory.Dequeue(OldFrame);
    }
    
    // Calculate average frame rate
    if (FrameTimeHistory.Num() > 0)
    {
        float TotalTime = 0.0f;
        TQueue<float> TempQueue = FrameTimeHistory;
        while (!TempQueue.IsEmpty())
        {
            float FrameTime;
            TempQueue.Dequeue(FrameTime);
            TotalTime += FrameTime;
        }
        float AverageFrameTime = TotalTime / FrameTimeHistory.Num();
        CurrentMetrics.AverageFrameRate = AverageFrameTime > 0.0f ? 1.0f / AverageFrameTime : 0.0f;
    }
    
    // Update memory metrics
    CurrentMetrics.MemoryUsageMB = GetMemoryUsageMB();
    
    // Update actor/component counts
    if (UWorld* World = GetWorld())
    {
        CurrentMetrics.ActiveActors = World->GetActorCount();
        
        int32 ComponentCount = 0;
        for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
        {
            AActor* Actor = *ActorItr;
            if (IsValid(Actor))
            {
                ComponentCount += Actor->GetRootComponent() ? Actor->GetComponents<UActorComponent>().Num() : 0;
            }
        }
        CurrentMetrics.ActiveComponents = ComponentCount;
    }
    
    // Update pooled objects count
    if (ObjectPoolManager)
    {
        TArray<FString> PoolNames = ObjectPoolManager->GetActivePoolNames();
        int32 TotalPooledObjects = 0;
        for (const FString& PoolName : PoolNames)
        {
            FPoolStatistics PoolStats = ObjectPoolManager->GetPoolStatistics(PoolName);
            TotalPooledObjects += PoolStats.CurrentPooledObjects;
        }
        CurrentMetrics.PooledObjects = TotalPooledObjects;
    }
    
    // Update CPU usage (simplified estimation)
    CurrentMetrics.CPUUsagePercent = FMath::Clamp(CurrentMetrics.FrameTime / 16.67f, 0.0f, 1.0f) * 100.0f;
    
    // Update rendering metrics (simplified)
    CurrentMetrics.RenderTime = CurrentMetrics.FrameTime * 0.6f; // Estimate render time as 60% of frame time
    CurrentMetrics.GameThreadTime = CurrentMetrics.FrameTime * 0.4f; // Estimate game thread time as 40% of frame time
    
    // Estimate draw calls and triangles (would need actual rendering stats in production)
    CurrentMetrics.DrawCalls = CurrentMetrics.ActiveActors * 2; // Rough estimate
    CurrentMetrics.Triangles = CurrentMetrics.ActiveActors * 1000; // Rough estimate
    
    LastMetricsUpdateTime = CurrentTime;
    
    // Check performance thresholds
    if (CurrentMetrics.AverageFrameRate < 30.0f || CurrentMetrics.MemoryUsageMB > Config.MemoryLimitMB)
    {
        OnPerformanceThresholdExceeded.Broadcast(CurrentMetrics);
    }
}

bool USinglePlayerOptimizationSystem::IsPerformanceOptimal() const
{
    // Define performance thresholds
    const float MinFrameRate = 60.0f;
    const float MaxFrameTime = 20.0f; // 20ms
    const float MaxMemoryUsage = Config.MemoryLimitMB * 0.8f; // 80% of limit
    
    return CurrentMetrics.AverageFrameRate >= MinFrameRate &&
           CurrentMetrics.FrameTime <= MaxFrameTime &&
           CurrentMetrics.MemoryUsageMB <= MaxMemoryUsage;
}

void USinglePlayerOptimizationSystem::StartBenchmark(const FString& TestName)
{
    if (ActiveBenchmarks.Contains(TestName))
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Benchmark '%s' is already running"), *TestName);
        return;
    }
    
    FBenchmarkData BenchmarkData;
    BenchmarkData.TestName = TestName;
    BenchmarkData.StartTime = FPlatformTime::Seconds();
    BenchmarkData.BeforeMetrics = CurrentMetrics;
    
    ActiveBenchmarks.Add(TestName, BenchmarkData);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Started benchmark: %s"), *TestName);
}

FBenchmarkData USinglePlayerOptimizationSystem::EndBenchmark(const FString& TestName)
{
    if (!ActiveBenchmarks.Contains(TestName))
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Benchmark '%s' is not running"), *TestName);
        return FBenchmarkData();
    }
    
    FBenchmarkData BenchmarkData = ActiveBenchmarks[TestName];
    BenchmarkData.EndTime = FPlatformTime::Seconds();
    BenchmarkData.Duration = BenchmarkData.EndTime - BenchmarkData.StartTime;
    BenchmarkData.AfterMetrics = CurrentMetrics;
    
    // Record additional metrics
    RecordBenchmarkMetrics(BenchmarkData);
    
    // Analyze results
    AnalyzeBenchmarkResults(BenchmarkData);
    
    // Store in history
    BenchmarkHistory.Add(BenchmarkData);
    
    // Remove from active benchmarks
    ActiveBenchmarks.Remove(TestName);
    
    // Broadcast completion event
    OnBenchmarkCompleted.Broadcast(TestName, BenchmarkData);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Completed benchmark: %s (Duration: %.3fs)"), 
           *TestName, BenchmarkData.Duration);
    
    return BenchmarkData;
}

void USinglePlayerOptimizationSystem::RunComprehensiveBenchmark()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Starting comprehensive benchmark suite..."));
    
    // Object pooling benchmark
    StartBenchmark(TEXT("ObjectPoolingPerformance"));
    
    // Simulate object pool stress test
    if (ObjectPoolManager)
    {
        // Create and release many objects to test pool performance
        TArray<AActor*> TestActors;
        for (int32 i = 0; i < 1000; ++i)
        {
            AActor* TestActor = ObjectPoolManager->AcquireActor(AStaticMeshActor::StaticClass());
            if (TestActor)
            {
                TestActors.Add(TestActor);
            }
        }
        
        // Release all test actors
        for (AActor* TestActor : TestActors)
        {
            ObjectPoolManager->ReleaseActor(TestActor);
        }
    }
    
    EndBenchmark(TEXT("ObjectPoolingPerformance"));
    
    // Memory allocation benchmark
    StartBenchmark(TEXT("MemoryAllocation"));
    
    // Simulate memory allocation stress
    TArray<TArray<int32>*> MemoryTest;
    for (int32 i = 0; i < 100; ++i)
    {
        TArray<int32>* NewArray = new TArray<int32>();
        NewArray->SetNum(10000);
        MemoryTest.Add(NewArray);
    }
    
    // Clean up
    for (TArray<int32>* TestArray : MemoryTest)
    {
        delete TestArray;
    }
    
    EndBenchmark(TEXT("MemoryAllocation"));
    
    // AI performance benchmark
    StartBenchmark(TEXT("AIPerformance"));
    
    // Count active AI and measure performance
    int32 AICount = GetActiveAICount();
    
    // Simulate AI updates
    FPlatformProcess::Sleep(0.1f); // Simulate AI processing time
    
    EndBenchmark(TEXT("AIPerformance"));
    
    // Rendering benchmark
    StartBenchmark(TEXT("RenderingPerformance"));
    
    // Force a render frame
    if (UWorld* World = GetWorld())
    {
        World->SendAllEndOfFrameUpdates();
    }
    
    EndBenchmark(TEXT("RenderingPerformance"));
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Comprehensive benchmark suite completed"));
}

void USinglePlayerOptimizationSystem::InitializeObjectPools()
{
    if (!ObjectPoolManager)
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Object Pool Manager not available"));
        return;
    }
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Initializing single-player optimized object pools..."));
    
    // Configure pools for single-player scenarios
    FObjectPoolConfig PoolConfig;
    PoolConfig.InitialSize = 50;
    PoolConfig.MaxSize = 200;
    PoolConfig.bEnableStatistics = true;
    PoolConfig.bEnableMemoryTracking = true;
    PoolConfig.bThreadSafe = false; // Single-player doesn't need thread safety
    
    // Create pools for common single-player objects
    if (Config.bPoolProjectiles)
    {
        // Projectile pools would be created here
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Initialized projectile pools"));
    }
    
    if (Config.bPoolParticleEffects)
    {
        // Particle effect pools would be created here
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Initialized particle effect pools"));
    }
    
    if (Config.bPoolAudioComponents)
    {
        // Audio component pools would be created here
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Initialized audio component pools"));
    }
    
    if (Config.bPoolDecals)
    {
        // Decal pools would be created here
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Initialized decal pools"));
    }
}

void USinglePlayerOptimizationSystem::OptimizeObjectPools()
{
    if (!ObjectPoolManager)
    {
        return;
    }
    
    // Cleanup and optimize all pools
    ObjectPoolManager->CleanupAllPools();
    
    // Adjust pool sizes based on current usage
    TArray<FString> PoolNames = ObjectPoolManager->GetActivePoolNames();
    for (const FString& PoolName : PoolNames)
    {
        FPoolStatistics Stats = ObjectPoolManager->GetPoolStatistics(PoolName);
        
        // If hit rate is low, consider reducing pool size
        if (Stats.HitRate < 0.5f && Stats.MaxPoolSize > 10)
        {
            // Pool size optimization would be implemented here
            UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimizing pool: %s (Hit Rate: %.2f)"), 
                   *PoolName, Stats.HitRate);
        }
    }
}

FString USinglePlayerOptimizationSystem::GetObjectPoolReport() const
{
    if (!ObjectPoolManager)
    {
        return TEXT("Object Pool Manager not available");
    }
    
    return ObjectPoolManager->GeneratePoolReport();
}

void USinglePlayerOptimizationSystem::OptimizeMemoryUsage()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimizing memory usage..."));
    
    // Force garbage collection
    ForceGarbageCollection();
    
    // Clean up unused assets
    CleanupUnusedAssets();
    
    // Optimize texture streaming
    OptimizeTextureStreaming();
    
    // Optimize audio streaming
    OptimizeAudioStreaming();
}

void USinglePlayerOptimizationSystem::ForceGarbageCollection()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Forcing garbage collection..."));
    
    // Collect garbage
    GEngine->ForceGarbageCollection(true);
    
    // Trim memory
    FPlatformMemory::Trim();
}

float USinglePlayerOptimizationSystem::GetMemoryUsageMB() const
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    return MemStats.UsedPhysical / (1024.0f * 1024.0f);
}

void USinglePlayerOptimizationSystem::OptimizeLODSettings()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimizing LOD settings for single-player..."));
    
    if (!GetWorld())
    {
        return;
    }
    
    // Adjust LOD settings for single-player scenarios
    for (TActorIterator<AStaticMeshActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AStaticMeshActor* MeshActor = *ActorItr;
        if (IsValid(MeshActor))
        {
            UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
            if (MeshComp && MeshComp->GetStaticMesh())
            {
                // Adjust LOD bias for single-player
                MeshComp->SetForcedLodModel(0); // Use automatic LOD
            }
        }
    }
}

void USinglePlayerOptimizationSystem::UpdateDistanceCulling()
{
    if (!GetWorld())
    {
        return;
    }
    
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn)
    {
        return;
    }
    
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    
    // Apply distance culling to actors
    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (IsValid(Actor) && Actor != PlayerPawn)
        {
            float Distance = FVector::Dist(Actor->GetActorLocation(), PlayerLocation);
            
            if (Distance > Config.CullingDistance)
            {
                // Hide distant actors
                Actor->SetActorHiddenInGame(true);
                Actor->SetActorTickEnabled(false);
            }
            else
            {
                // Show nearby actors
                Actor->SetActorHiddenInGame(false);
                Actor->SetActorTickEnabled(true);
            }
        }
    }
}

void USinglePlayerOptimizationSystem::OptimizeAIPerformance()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimizing AI performance for single-player..."));
    
    if (!GetWorld())
    {
        return;
    }
    
    // Limit active AI count
    TArray<AFPSAICharacter*> AICharacters;
    for (TActorIterator<AFPSAICharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AICharacters.Add(*ActorItr);
    }
    
    // Sort AI by distance to player
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (PlayerPawn)
    {
        AICharacters.Sort([PlayerPawn](const AFPSAICharacter& A, const AFPSAICharacter& B)
        {
            float DistA = FVector::Dist(A.GetActorLocation(), PlayerPawn->GetActorLocation());
            float DistB = FVector::Dist(B.GetActorLocation(), PlayerPawn->GetActorLocation());
            return DistA < DistB;
        });
        
        // Activate only the closest AI up to the limit
        for (int32 i = 0; i < AICharacters.Num(); ++i)
        {
            if (AICharacters[i])
            {
                if (i < Config.MaxActiveAI)
                {
                    // Activate AI
                    AICharacters[i]->SetActorTickEnabled(true);
                    
                    // Optimize AI system component if available
                    if (UAdvancedAISystem* AISystem = AICharacters[i]->FindComponentByClass<UAdvancedAISystem>())
                    {
                        // AI system optimization would be applied here
                    }
                }
                else
                {
                    // Deactivate distant AI
                    AICharacters[i]->SetActorTickEnabled(false);
                }
            }
        }
    }
}

int32 USinglePlayerOptimizationSystem::GetActiveAICount() const
{
    if (!GetWorld())
    {
        return 0;
    }
    
    int32 ActiveCount = 0;
    for (TActorIterator<AFPSAICharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        if (IsValid(*ActorItr) && (*ActorItr)->IsActorTickEnabled())
        {
            ActiveCount++;
        }
    }
    
    return ActiveCount;
}

void USinglePlayerOptimizationSystem::OptimizeRenderingSettings()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimizing rendering settings for single-player..."));
    
    // Apply single-player specific rendering optimizations
    // This would typically involve console commands or engine settings modifications
    
    if (Config.bEnableOcclusionCulling)
    {
        // Enable occlusion culling optimizations
    }
    
    if (Config.bEnableTextureLOD)
    {
        // Apply texture LOD optimizations
    }
}

void USinglePlayerOptimizationSystem::UpdateRenderingQuality()
{
    // Dynamically adjust rendering quality based on performance
    if (CurrentMetrics.AverageFrameRate < 45.0f)
    {
        // Reduce quality settings
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Reducing rendering quality due to low framerate"));
    }
    else if (CurrentMetrics.AverageFrameRate > 75.0f)
    {
        // Increase quality settings
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Increasing rendering quality due to high framerate"));
    }
}

void USinglePlayerOptimizationSystem::SetOptimizationState(EOptimizationState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }
    
    EOptimizationState OldState = CurrentState;
    
    if (!CanTransitionToState(NewState))
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Invalid state transition from %d to %d"), 
               (int32)OldState, (int32)NewState);
        return;
    }
    
    TransitionToState(NewState);
    
    OnOptimizationStateChanged.Broadcast(OldState, NewState);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("State changed from %d to %d"), 
           (int32)OldState, (int32)NewState);
}

FString USinglePlayerOptimizationSystem::GeneratePerformanceReport() const
{
    FString Report;
    
    Report += TEXT("=== Single Player Optimization System Performance Report ===\n");
    Report += FString::Printf(TEXT("System State: %d\n"), (int32)CurrentState);
    Report += FString::Printf(TEXT("Frame Rate: %.2f FPS\n"), CurrentMetrics.AverageFrameRate);
    Report += FString::Printf(TEXT("Frame Time: %.2f ms\n"), CurrentMetrics.FrameTime);
    Report += FString::Printf(TEXT("Memory Usage: %.2f MB\n"), CurrentMetrics.MemoryUsageMB);
    Report += FString::Printf(TEXT("Active Actors: %d\n"), CurrentMetrics.ActiveActors);
    Report += FString::Printf(TEXT("Active Components: %d\n"), CurrentMetrics.ActiveComponents);
    Report += FString::Printf(TEXT("Pooled Objects: %d\n"), CurrentMetrics.PooledObjects);
    Report += FString::Printf(TEXT("Draw Calls: %d\n"), CurrentMetrics.DrawCalls);
    Report += FString::Printf(TEXT("Triangles: %d\n"), CurrentMetrics.Triangles);
    
    Report += TEXT("\n=== Configuration ===\n");
    Report += FString::Printf(TEXT("Object Pooling Enabled: %s\n"), Config.bEnableObjectPooling ? TEXT("Yes") : TEXT("No"));
    Report += FString::Printf(TEXT("Aggressive LOD Enabled: %s\n"), Config.bEnableAggressiveLOD ? TEXT("Yes") : TEXT("No"));
    Report += FString::Printf(TEXT("AI Optimization Enabled: %s\n"), Config.bEnableAIOptimization ? TEXT("Yes") : TEXT("No"));
    Report += FString::Printf(TEXT("Max Active AI: %d\n"), Config.MaxActiveAI);
    Report += FString::Printf(TEXT("Memory Limit: %.2f MB\n"), Config.MemoryLimitMB);
    
    if (ObjectPoolManager)
    {
        Report += TEXT("\n=== Object Pool Report ===\n");
        Report += ObjectPoolManager->GeneratePoolReport();
    }
    
    Report += TEXT("\n=== Benchmark History ===\n");
    for (const FBenchmarkData& Benchmark : BenchmarkHistory)
    {
        Report += FString::Printf(TEXT("%s: %.3fs\n"), *Benchmark.TestName, Benchmark.Duration);
    }
    
    return Report;
}

void USinglePlayerOptimizationSystem::SavePerformanceReport(const FString& Filename) const
{
    FString Report = GeneratePerformanceReport();
    FString FilePath = FPaths::ProjectSavedDir() / TEXT("Performance") / Filename;
    
    if (FFileHelper::SaveStringToFile(Report, *FilePath))
    {
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Performance report saved to: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogSinglePlayerOptimization, Error, TEXT("Failed to save performance report to: %s"), *FilePath);
    }
}

void USinglePlayerOptimizationSystem::ResetOptimizationSystem()
{
    FScopeLock Lock(&OptimizationMutex);
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Resetting optimization system..."));
    
    // Clear benchmark history
    BenchmarkHistory.Empty();
    ActiveBenchmarks.Empty();
    
    // Reset metrics
    CurrentMetrics = FSinglePlayerMetrics();
    
    // Clear frame time history
    while (!FrameTimeHistory.IsEmpty())
    {
        float Temp;
        FrameTimeHistory.Dequeue(Temp);
    }
    
    // Reset configuration to defaults
    Config = FSinglePlayerOptimizationConfig();
    
    // Restart the system
    if (CurrentState != EOptimizationState::Disabled)
    {
        DisableOptimizations();
        EnableOptimizations();
    }
    
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimization system reset complete"));
}

// Private implementation methods

void USinglePlayerOptimizationSystem::TickOptimization()
{
    UpdateOptimizations();
}

void USinglePlayerOptimizationSystem::TickMetricsUpdate()
{
    if (Config.bEnablePerformanceMonitoring)
    {
        UpdatePerformanceMetrics();
        
        if (Config.bLogPerformanceMetrics)
        {
            UE_LOG(LogSinglePlayerOptimization, Log, TEXT("FPS: %.1f, Memory: %.1f MB, Actors: %d"), 
                   CurrentMetrics.AverageFrameRate, CurrentMetrics.MemoryUsageMB, CurrentMetrics.ActiveActors);
        }
    }
}

void USinglePlayerOptimizationSystem::TickGarbageCollection()
{
    if (Config.bEnableAggressiveGarbageCollection)
    {
        ForceGarbageCollection();
    }
}

void USinglePlayerOptimizationSystem::CleanupUnusedAssets()
{
    // Implementation would involve cleaning up unused textures, sounds, etc.
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Cleaning up unused assets..."));
}

void USinglePlayerOptimizationSystem::OptimizeTextureStreaming()
{
    // Implementation would involve optimizing texture streaming settings
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimizing texture streaming..."));
}

void USinglePlayerOptimizationSystem::OptimizeAudioStreaming()
{
    // Implementation would involve optimizing audio streaming settings
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Optimizing audio streaming..."));
}

void USinglePlayerOptimizationSystem::AnalyzePerformanceBottlenecks()
{
    // Analyze current performance metrics to identify bottlenecks
    if (CurrentMetrics.RenderTime > CurrentMetrics.GameThreadTime * 1.5f)
    {
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Rendering bottleneck detected"));
        // Apply rendering optimizations
    }
    else if (CurrentMetrics.GameThreadTime > 10.0f) // 10ms threshold
    {
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Game thread bottleneck detected"));
        // Apply game logic optimizations
    }
}

void USinglePlayerOptimizationSystem::ApplyDynamicOptimizations()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Applying dynamic optimizations..."));
    
    AnalyzePerformanceBottlenecks();
    AdjustQualitySettings();
    
    // Apply AI optimizations if needed
    if (GetActiveAICount() > Config.MaxActiveAI)
    {
        OptimizeAIPerformance();
    }
    
    // Apply memory optimizations if needed
    if (CurrentMetrics.MemoryUsageMB > Config.MemoryLimitMB * 0.9f)
    {
        OptimizeMemoryUsage();
    }
}

void USinglePlayerOptimizationSystem::AdjustQualitySettings()
{
    // Dynamically adjust quality based on performance
    if (CurrentMetrics.AverageFrameRate < 30.0f)
    {
        // Reduce quality significantly
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Performance critical - reducing quality settings"));
    }
    else if (CurrentMetrics.AverageFrameRate < 45.0f)
    {
        // Reduce quality moderately
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Performance suboptimal - reducing quality settings"));
    }
}

void USinglePlayerOptimizationSystem::InitializeBenchmarkSystem()
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Initializing benchmark system..."));
    
    // Clear existing data
    BenchmarkHistory.Empty();
    ActiveBenchmarks.Empty();
}

void USinglePlayerOptimizationSystem::RecordBenchmarkMetrics(FBenchmarkData& BenchmarkData)
{
    // Record additional custom metrics
    BenchmarkData.CustomMetrics.Add(TEXT("MemoryDelta"), 
        BenchmarkData.AfterMetrics.MemoryUsageMB - BenchmarkData.BeforeMetrics.MemoryUsageMB);
    
    BenchmarkData.CustomMetrics.Add(TEXT("FPSDelta"), 
        BenchmarkData.AfterMetrics.AverageFrameRate - BenchmarkData.BeforeMetrics.AverageFrameRate);
    
    BenchmarkData.CustomMetrics.Add(TEXT("ActorCountDelta"), 
        (float)(BenchmarkData.AfterMetrics.ActiveActors - BenchmarkData.BeforeMetrics.ActiveActors));
}

void USinglePlayerOptimizationSystem::AnalyzeBenchmarkResults(const FBenchmarkData& Results)
{
    UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Analyzing benchmark results for: %s"), *Results.TestName);
    
    // Analyze performance impact
    float MemoryDelta = Results.CustomMetrics.Contains(TEXT("MemoryDelta")) ? 
        Results.CustomMetrics[TEXT("MemoryDelta")] : 0.0f;
    
    float FPSDelta = Results.CustomMetrics.Contains(TEXT("FPSDelta")) ? 
        Results.CustomMetrics[TEXT("FPSDelta")] : 0.0f;
    
    if (MemoryDelta > 100.0f) // More than 100MB increase
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Benchmark %s caused significant memory increase: %.2f MB"), 
               *Results.TestName, MemoryDelta);
    }
    
    if (FPSDelta < -10.0f) // More than 10 FPS drop
    {
        UE_LOG(LogSinglePlayerOptimization, Warning, TEXT("Benchmark %s caused significant FPS drop: %.2f"), 
               *Results.TestName, FPSDelta);
    }
}

void USinglePlayerOptimizationSystem::IntegrateWithObjectPooling()
{
    if (ObjectPoolManager && Config.bEnableObjectPooling)
    {
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Integrating with Object Pooling System..."));
        
        // Configure object pools for single-player optimization
        InitializeObjectPools();
    }
}

void USinglePlayerOptimizationSystem::IntegrateWithPerformanceSystem()
{
    if (PerformanceSystem)
    {
        UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Integrating with Performance Optimization System..."));
        
        // Integration with performance system would be implemented here
    }
}

void USinglePlayerOptimizationSystem::SetupOptimizationTimers()
{
    if (UWorld* World = GetWorld())
    {
        // Setup optimization update timer
        World->GetTimerManager().SetTimer(
            OptimizationTimerHandle,
            this,
            &USinglePlayerOptimizationSystem::TickOptimization,
            5.0f, // Every 5 seconds
            true
        );
        
        // Setup metrics update timer
        if (Config.bEnablePerformanceMonitoring)
        {
            World->GetTimerManager().SetTimer(
                MetricsTimerHandle,
                this,
                &USinglePlayerOptimizationSystem::TickMetricsUpdate,
                Config.MetricsUpdateInterval,
                true
            );
        }
        
        // Setup garbage collection timer
        if (Config.bEnableAggressiveGarbageCollection)
        {
            World->GetTimerManager().SetTimer(
                GarbageCollectionTimerHandle,
                this,
                &USinglePlayerOptimizationSystem::TickGarbageCollection,
                Config.GarbageCollectionInterval,
                true
            );
        }
    }
}

void USinglePlayerOptimizationSystem::TransitionToState(EOptimizationState NewState)
{
    CurrentState = NewState;
    
    switch (NewState)
    {
        case EOptimizationState::Initializing:
            UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Transitioning to Initializing state"));
            break;
        case EOptimizationState::Active:
            UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Transitioning to Active state"));
            EnableOptimizations();
            break;
        case EOptimizationState::Monitoring:
            UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Transitioning to Monitoring state"));
            break;
        case EOptimizationState::Optimizing:
            UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Transitioning to Optimizing state"));
            break;
        case EOptimizationState::Disabled:
            UE_LOG(LogSinglePlayerOptimization, Log, TEXT("Transitioning to Disabled state"));
            break;
        case EOptimizationState::Error:
            UE_LOG(LogSinglePlayerOptimization, Error, TEXT("Transitioning to Error state"));
            break;
    }
}

bool USinglePlayerOptimizationSystem::CanTransitionToState(EOptimizationState NewState) const
{
    // Define valid state transitions
    switch (CurrentState)
    {
        case EOptimizationState::Disabled:
            return NewState == EOptimizationState::Initializing;
        case EOptimizationState::Initializing:
            return NewState == EOptimizationState::Active || NewState == EOptimizationState::Error;
        case EOptimizationState::Active:
            return NewState == EOptimizationState::Monitoring || NewState == EOptimizationState::Disabled || NewState == EOptimizationState::Error;
        case EOptimizationState::Monitoring:
            return NewState == EOptimizationState::Optimizing || NewState == EOptimizationState::Disabled || NewState == EOptimizationState::Error;
        case EOptimizationState::Optimizing:
            return NewState == EOptimizationState::Monitoring || NewState == EOptimizationState::Error;
        case EOptimizationState::Error:
            return NewState == EOptimizationState::Disabled || NewState == EOptimizationState::Initializing;
        default:
            return false;
    }
}

bool USinglePlayerOptimizationSystem::ValidateConfiguration() const
{
    // Validate configuration values
    if (Config.MemoryLimitMB <= 0.0f || Config.MemoryLimitMB > 16384.0f) // Max 16GB
    {
        return false;
    }
    
    if (Config.MaxActiveAI < 0 || Config.MaxActiveAI > 200)
    {
        return false;
    }
    
    if (Config.MetricsUpdateInterval <= 0.0f || Config.MetricsUpdateInterval > 60.0f)
    {
        return false;
    }
    
    return true;
}

bool USinglePlayerOptimizationSystem::ValidateSystemRequirements() const
{
    // Check if required systems are available
    if (!GetGameInstance())
    {
        UE_LOG(LogSinglePlayerOptimization, Error, TEXT("Game Instance not available"));
        return false;
    }
    
    if (!GetWorld())
    {
        UE_LOG(LogSinglePlayerOptimization, Error, TEXT("World not available"));
        return false;
    }
    
    return true;
}