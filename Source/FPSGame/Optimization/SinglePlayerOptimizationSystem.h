#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "HAL/CriticalSection.h"
#include "Engine/TimerManager.h"
#include "Containers/Queue.h"
#include "AdvancedObjectPoolManager.h"
#include "PerformanceOptimizationSystem.h"
#include "SinglePlayerOptimizationSystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSinglePlayerOptimization, Log, All);

// Performance metrics for single-player optimization
USTRUCT(BlueprintType)
struct FSinglePlayerMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float FrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float AverageFrameRate = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float CPUUsagePercent = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float MemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 ActiveActors = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 ActiveComponents = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 PooledObjects = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float RenderTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float GameThreadTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 DrawCalls = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 Triangles = 0;

    FSinglePlayerMetrics()
    {
        FrameTime = 0.0f;
        AverageFrameRate = 0.0f;
        CPUUsagePercent = 0.0f;
        MemoryUsageMB = 0.0f;
        ActiveActors = 0;
        ActiveComponents = 0;
        PooledObjects = 0;
        RenderTime = 0.0f;
        GameThreadTime = 0.0f;
        DrawCalls = 0;
        Triangles = 0;
    }
};

// Single-player optimization settings
USTRUCT(BlueprintType)
struct FSinglePlayerOptimizationConfig
{
    GENERATED_BODY()

    // Memory optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    bool bEnableAggressiveGarbageCollection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    float GarbageCollectionInterval = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    float MemoryLimitMB = 4096.0f;

    // Object pooling integration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pooling")
    bool bEnableObjectPooling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pooling")
    bool bPoolProjectiles = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pooling")
    bool bPoolParticleEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pooling")
    bool bPoolAudioComponents = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pooling")
    bool bPoolDecals = true;

    // Level of Detail (LOD) optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    bool bEnableAggressiveLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float LODDistanceMultiplier = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    bool bEnableDistanceCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    float CullingDistance = 10000.0f;

    // AI optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    bool bEnableAIOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    int32 MaxActiveAI = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float AIUpdateFrequency = 0.1f;

    // Physics optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    bool bEnablePhysicsOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float PhysicsSubstepDeltaTime = 0.016f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    int32 MaxPhysicsSubsteps = 4;

    // Rendering optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableRenderingOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableOcclusionCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableTextureLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    float TextureLODBias = 0.0f;

    // Performance monitoring
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monitoring")
    bool bEnablePerformanceMonitoring = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monitoring")
    float MetricsUpdateInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monitoring")
    bool bLogPerformanceMetrics = false;

    FSinglePlayerOptimizationConfig()
    {
        bEnableAggressiveGarbageCollection = true;
        GarbageCollectionInterval = 30.0f;
        MemoryLimitMB = 4096.0f;
        bEnableObjectPooling = true;
        bPoolProjectiles = true;
        bPoolParticleEffects = true;
        bPoolAudioComponents = true;
        bPoolDecals = true;
        bEnableAggressiveLOD = true;
        LODDistanceMultiplier = 0.8f;
        bEnableDistanceCulling = true;
        CullingDistance = 10000.0f;
        bEnableAIOptimization = true;
        MaxActiveAI = 20;
        AIUpdateFrequency = 0.1f;
        bEnablePhysicsOptimization = true;
        PhysicsSubstepDeltaTime = 0.016f;
        MaxPhysicsSubsteps = 4;
        bEnableRenderingOptimization = true;
        bEnableOcclusionCulling = true;
        bEnableTextureLOD = true;
        TextureLODBias = 0.0f;
        bEnablePerformanceMonitoring = true;
        MetricsUpdateInterval = 1.0f;
        bLogPerformanceMetrics = false;
    }
};

// Benchmark data for performance testing
USTRUCT(BlueprintType)
struct FBenchmarkData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    FString TestName;

    UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    float StartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    float EndTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    float Duration = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    FSinglePlayerMetrics BeforeMetrics;

    UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    FSinglePlayerMetrics AfterMetrics;

    UPROPERTY(BlueprintReadOnly, Category = "Benchmark")
    TMap<FString, float> CustomMetrics;

    FBenchmarkData()
    {
        TestName = TEXT("");
        StartTime = 0.0f;
        EndTime = 0.0f;
        Duration = 0.0f;
    }
};

// Optimization state for tracking system performance
UENUM(BlueprintType)
enum class EOptimizationState : uint8
{
    Disabled        UMETA(DisplayName = "Disabled"),
    Initializing    UMETA(DisplayName = "Initializing"),
    Active          UMETA(DisplayName = "Active"),
    Monitoring      UMETA(DisplayName = "Monitoring"),
    Optimizing      UMETA(DisplayName = "Optimizing"),
    Error           UMETA(DisplayName = "Error")
};

// Events for monitoring optimization system
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOptimizationStateChanged, EOptimizationState, OldState, EOptimizationState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerformanceThresholdExceeded, const FSinglePlayerMetrics&, Metrics);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBenchmarkCompleted, const FString&, TestName, const FBenchmarkData&, Results);

/**
 * Single Player Optimization System
 * Provides comprehensive performance optimization specifically tailored for single-player FPS gameplay
 * Integrates with the Advanced Object Pooling System and other optimization components
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API USinglePlayerOptimizationSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    USinglePlayerOptimizationSystem();

    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Core optimization functions
    UFUNCTION(BlueprintCallable, Category = "Single Player Optimization")
    void EnableOptimizations();

    UFUNCTION(BlueprintCallable, Category = "Single Player Optimization")
    void DisableOptimizations();

    UFUNCTION(BlueprintCallable, Category = "Single Player Optimization")
    void UpdateOptimizations();

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "Single Player Optimization")
    void SetOptimizationConfig(const FSinglePlayerOptimizationConfig& NewConfig);

    UFUNCTION(BlueprintCallable, Category = "Single Player Optimization")
    FSinglePlayerOptimizationConfig GetOptimizationConfig() const { return Config; }

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    FSinglePlayerMetrics GetCurrentMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void UpdatePerformanceMetrics();

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    float GetAverageFrameRate() const { return CurrentMetrics.AverageFrameRate; }

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    bool IsPerformanceOptimal() const;

    // Benchmark system
    UFUNCTION(BlueprintCallable, Category = "Benchmark")
    void StartBenchmark(const FString& TestName);

    UFUNCTION(BlueprintCallable, Category = "Benchmark")
    FBenchmarkData EndBenchmark(const FString& TestName);

    UFUNCTION(BlueprintCallable, Category = "Benchmark")
    void RunComprehensiveBenchmark();

    UFUNCTION(BlueprintCallable, Category = "Benchmark")
    TArray<FBenchmarkData> GetBenchmarkHistory() const { return BenchmarkHistory; }

    // Object pooling integration
    UFUNCTION(BlueprintCallable, Category = "Object Pooling Integration")
    void InitializeObjectPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pooling Integration")
    void OptimizeObjectPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pooling Integration")
    FString GetObjectPoolReport() const;

    // Memory optimization
    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void OptimizeMemoryUsage();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void ForceGarbageCollection();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    float GetMemoryUsageMB() const;

    // LOD and culling optimization
    UFUNCTION(BlueprintCallable, Category = "LOD Optimization")
    void OptimizeLODSettings();

    UFUNCTION(BlueprintCallable, Category = "LOD Optimization")
    void UpdateDistanceCulling();

    // AI optimization
    UFUNCTION(BlueprintCallable, Category = "AI Optimization")
    void OptimizeAIPerformance();

    UFUNCTION(BlueprintCallable, Category = "AI Optimization")
    int32 GetActiveAICount() const;

    // Rendering optimization
    UFUNCTION(BlueprintCallable, Category = "Rendering Optimization")
    void OptimizeRenderingSettings();

    UFUNCTION(BlueprintCallable, Category = "Rendering Optimization")
    void UpdateRenderingQuality();

    // State management
    UFUNCTION(BlueprintCallable, Category = "State Management")
    EOptimizationState GetOptimizationState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category = "State Management")
    void SetOptimizationState(EOptimizationState NewState);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnOptimizationStateChanged OnOptimizationStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPerformanceThresholdExceeded OnPerformanceThresholdExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBenchmarkCompleted OnBenchmarkCompleted;

    // Utility functions
    UFUNCTION(BlueprintCallable, Category = "Utilities")
    FString GeneratePerformanceReport() const;

    UFUNCTION(BlueprintCallable, Category = "Utilities")
    void SavePerformanceReport(const FString& Filename) const;

    UFUNCTION(BlueprintCallable, Category = "Utilities")
    void ResetOptimizationSystem();

protected:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FSinglePlayerOptimizationConfig Config;

    // Current state
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EOptimizationState CurrentState;

    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    FSinglePlayerMetrics CurrentMetrics;

    // Benchmark data
    UPROPERTY()
    TArray<FBenchmarkData> BenchmarkHistory;

    UPROPERTY()
    TMap<FString, FBenchmarkData> ActiveBenchmarks;

    // Integration references
    UPROPERTY()
    class UAdvancedObjectPoolManager* ObjectPoolManager;

    UPROPERTY()
    class UPerformanceOptimizationSystem* PerformanceSystem;

    // Timers and intervals
    FTimerHandle OptimizationTimerHandle;
    FTimerHandle MetricsTimerHandle;
    FTimerHandle GarbageCollectionTimerHandle;

    // Performance tracking
    TQueue<float> FrameTimeHistory;
    double LastMetricsUpdateTime;
    double LastOptimizationTime;

    // Thread safety
    mutable FCriticalSection OptimizationMutex;

private:
    // Internal optimization functions
    void TickOptimization();
    void TickMetricsUpdate();
    void TickGarbageCollection();

    // Memory management
    void CleanupUnusedAssets();
    void OptimizeTextureStreaming();
    void OptimizeAudioStreaming();

    // Performance analysis
    void AnalyzePerformanceBottlenecks();
    void ApplyDynamicOptimizations();
    void AdjustQualitySettings();

    // Benchmark utilities
    void InitializeBenchmarkSystem();
    void RecordBenchmarkMetrics(FBenchmarkData& BenchmarkData);
    void AnalyzeBenchmarkResults(const FBenchmarkData& Results);

    // Integration helpers
    void IntegrateWithObjectPooling();
    void IntegrateWithPerformanceSystem();
    void SetupOptimizationTimers();

    // State management
    void TransitionToState(EOptimizationState NewState);
    bool CanTransitionToState(EOptimizationState NewState) const;

    // Validation
    bool ValidateConfiguration() const;
    bool ValidateSystemRequirements() const;
};