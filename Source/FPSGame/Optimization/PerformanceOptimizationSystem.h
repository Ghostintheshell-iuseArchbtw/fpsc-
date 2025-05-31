#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshComponent.h"
#include "Components/ActorComponent.h"
#include "HAL/ThreadSafeBool.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "PerformanceOptimizationSystem.generated.h"

USTRUCT(BlueprintType)
struct FLODSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LOD0Distance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LOD1Distance = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LOD2Distance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LOD3Distance = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CullDistance = 15000.0f;
};

USTRUCT(BlueprintType)
struct FPerformanceMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float FrameRate = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float FrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float GPUTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CPUTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 DrawCalls = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TriangleCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 VisibleActors = 0;

    // Thermal monitoring
    UPROPERTY(BlueprintReadOnly)
    float CPUTemperature = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float GPUTemperature = 0.0f;

    // GPU profiling
    UPROPERTY(BlueprintReadOnly)
    float GPUMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float GPUUtilization = 0.0f;

    // Predictive metrics
    UPROPERTY(BlueprintReadOnly)
    float PredictedFrameDropRisk = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ThermalThrottlingRisk = 0.0f;
};

USTRUCT(BlueprintType)
struct FPooledObject
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* Actor = nullptr;

    UPROPERTY()
    bool bInUse = false;

    UPROPERTY()
    float LastUsedTime = 0.0f;

    FPooledObject()
    {
        Actor = nullptr;
        bInUse = false;
        LastUsedTime = 0.0f;
    }
};

USTRUCT(BlueprintType)
struct FOptimizationSettings
{
    GENERATED_BODY()

    // LOD System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    bool bEnableLODSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    FLODSettings DefaultLODSettings;

    // Culling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling")
    bool bEnableFrustumCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling")
    bool bEnableOcclusionCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling")
    float MaxCullingDistance = 20000.0f;

    // Object Pooling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
    bool bEnableObjectPooling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
    int32 DefaultPoolSize = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
    float PoolCleanupInterval = 60.0f;

    // Threading
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threading")
    bool bEnableAsyncProcessing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threading")
    int32 MaxAsyncTasks = 4;

    // Memory Management
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    bool bEnableGarbageCollection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    float GCInterval = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    float MaxMemoryUsageMB = 2048.0f;

    // Performance Monitoring
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monitoring")
    bool bEnablePerformanceMonitoring = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monitoring")
    float MonitoringUpdateInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monitoring")
    float TargetFrameRate = 60.0f;

    // Thermal monitoring
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    bool bEnableThermalMonitoring = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float CPUThermalThreshold = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float GPUThermalThreshold = 85.0f;

    // Predictive optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Predictive")
    bool bEnablePredictiveOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Predictive")
    float PredictionHorizonSeconds = 5.0f;

    // GPU profiling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU")
    bool bEnableGPUProfiling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU")
    float GPUMemoryThresholdMB = 6144.0f;
};

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API APerformanceOptimizationSystem : public AActor
{
    GENERATED_BODY()

public:
    APerformanceOptimizationSystem();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FOptimizationSettings OptimizationSettings;

    // Performance Metrics
    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    FPerformanceMetrics CurrentMetrics;

    // LOD System
    UFUNCTION(BlueprintCallable, Category = "LOD")
    void UpdateLODSystem();

    UFUNCTION(BlueprintCallable, Category = "LOD")
    void RegisterActorForLOD(AActor* Actor, const FLODSettings& LODSettings);

    UFUNCTION(BlueprintCallable, Category = "LOD")
    void UnregisterActorFromLOD(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "LOD")
    int32 CalculateLODLevel(AActor* Actor, const FLODSettings& LODSettings);

    // Object Pooling
    UFUNCTION(BlueprintCallable, Category = "Pooling")
    AActor* GetPooledObject(UClass* ObjectClass);

    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void ReturnPooledObject(AActor* Object);

    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void InitializePool(UClass* ObjectClass, int32 PoolSize);

    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void CleanupPools();

    // Culling System
    UFUNCTION(BlueprintCallable, Category = "Culling")
    void UpdateCullingSystem();

    UFUNCTION(BlueprintCallable, Category = "Culling")
    bool IsActorInViewFrustum(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Culling")
    bool IsActorOccluded(AActor* Actor);

    // Performance Monitoring
    UFUNCTION(BlueprintCallable, Category = "Monitoring")
    void UpdatePerformanceMetrics();

    UFUNCTION(BlueprintCallable, Category = "Monitoring")
    FPerformanceMetrics GetPerformanceMetrics() const { return CurrentMetrics; }

    UFUNCTION(BlueprintCallable, Category = "Monitoring")
    bool IsPerformanceTargetMet() const;

    UFUNCTION(BlueprintCallable, Category = "Monitoring")
    void OptimizeBasedOnPerformance();

    // Memory Management
    UFUNCTION(BlueprintCallable, Category = "Memory")
    void ForceGarbageCollection();

    UFUNCTION(BlueprintCallable, Category = "Memory")
    float GetMemoryUsageMB() const;

    UFUNCTION(BlueprintCallable, Category = "Memory")
    void OptimizeMemoryUsage();

    // Threading
    UFUNCTION(BlueprintCallable, Category = "Threading")
    void ProcessAsyncTasks();

    UFUNCTION(BlueprintCallable, Category = "Threading")
    void AddAsyncTask(TFunction<void()> Task);

    // Thermal monitoring
    UFUNCTION(BlueprintCallable, Category = "Thermal")
    void UpdateThermalMetrics();

    UFUNCTION(BlueprintCallable, Category = "Thermal")
    float GetCPUTemperature() const;

    UFUNCTION(BlueprintCallable, Category = "Thermal")
    float GetGPUTemperature() const;

    UFUNCTION(BlueprintCallable, Category = "Thermal")
    bool IsThermalThrottlingNeeded() const;

    // GPU profiling
    UFUNCTION(BlueprintCallable, Category = "GPU")
    void UpdateGPUMetrics();

    UFUNCTION(BlueprintCallable, Category = "GPU")
    float GetGPUMemoryUsage() const;

    UFUNCTION(BlueprintCallable, Category = "GPU")
    float GetGPUUtilization() const;

    // Predictive optimization
    UFUNCTION(BlueprintCallable, Category = "Predictive")
    void UpdatePredictiveMetrics();

    UFUNCTION(BlueprintCallable, Category = "Predictive")
    float PredictFrameDropRisk() const;

    UFUNCTION(BlueprintCallable, Category = "Predictive")
    void ApplyPredictiveOptimizations();

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void SetOptimizationLevel(int32 Level);

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void ResetToDefaultSettings();

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void SaveOptimizationSettings();

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void LoadOptimizationSettings();

    // Events
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerformanceChanged, const FPerformanceMetrics&, Metrics);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPerformanceChanged OnPerformanceChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOptimizationApplied, int32, OptimizationLevel);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnOptimizationApplied OnOptimizationApplied;

private:
    // Internal data structures
    UPROPERTY()
    TMap<AActor*, FLODSettings> RegisteredLODActors;

    UPROPERTY()
    TMap<UClass*, TArray<FPooledObject>> ObjectPools;

    UPROPERTY()
    TArray<AActor*> CulledActors;

    UPROPERTY()
    TArray<AActor*> VisibleActors;

    // Timers
    float LODUpdateTimer = 0.0f;
    float CullingUpdateTimer = 0.0f;
    float PerformanceUpdateTimer = 0.0f;
    float GCTimer = 0.0f;
    float PoolCleanupTimer = 0.0f;

    // Threading
    TArray<TFuture<void>> AsyncTasks;
    FCriticalSection TaskCriticalSection;

    // Performance tracking
    TArray<float> FrameTimeHistory;
    int32 FrameTimeHistoryIndex = 0;
    static const int32 FrameTimeHistorySize = 60;

    // Thermal tracking
    TArray<float> CPUTemperatureHistory;
    TArray<float> GPUTemperatureHistory;
    int32 ThermalHistoryIndex = 0;
    static const int32 ThermalHistorySize = 30;

    // GPU profiling
    TArray<float> GPUUsageHistory;
    TArray<float> GPUMemoryHistory;

    // Predictive data
    TArray<float> FrameDropPredictions;
    float LastPredictionTime = 0.0f;

    // Internal functions
    void InitializeSystem();
    void UpdateLODForActor(AActor* Actor, const FLODSettings& LODSettings);
    void SetActorLODLevel(AActor* Actor, int32 LODLevel);
    void CullActor(AActor* Actor);
    void UncullActor(AActor* Actor);
    float CalculateDistanceToPlayer(AActor* Actor);
    APlayerController* GetPlayerController();
    void CollectGarbageIfNeeded();
    void OptimizeLODSettings();
    void OptimizeCullingSettings();
    void OptimizePoolSizes();
    void ProcessFrameTimeHistory(float DeltaTime);
    float GetAverageFrameTime() const;
    void ApplyLowPerformanceOptimizations();
    void ApplyHighPerformanceOptimizations();
    void LogPerformanceMetrics() const;
};
