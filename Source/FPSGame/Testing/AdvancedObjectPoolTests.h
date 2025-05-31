#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "../Optimization/AdvancedObjectPoolManager.h"

/**
 * Comprehensive test suite for the Advanced Object Pooling System
 * Tests all aspects of object pooling including performance, memory management,
 * thread safety, and integration with Unreal Engine systems
 */

// Unit Tests
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolUnitTest, "FPSGame.ObjectPooling.Unit.Basic",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolConfigurationTest, "FPSGame.ObjectPooling.Unit.Configuration",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolStatisticsTest, "FPSGame.ObjectPooling.Unit.Statistics",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolMaintenanceTest, "FPSGame.ObjectPooling.Unit.Maintenance",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

// Performance Tests
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolPerformanceTest, "FPSGame.ObjectPooling.Performance.Acquisition",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolScalabilityTest, "FPSGame.ObjectPooling.Performance.Scalability",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

// Memory Stress Tests
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolMemoryStressTest, "FPSGame.ObjectPooling.MemoryStress.LargeScale",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolMemoryLeakTest, "FPSGame.ObjectPooling.MemoryStress.LeakDetection",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

// Thread Safety Tests
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolThreadSafetyTest, "FPSGame.ObjectPooling.Unit.ThreadSafety",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolDeadlockTest, "FPSGame.ObjectPooling.ThreadSafety.DeadlockPrevention",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

// Integration Tests
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolIntegrationTest, "FPSGame.ObjectPooling.Integration.GameSystems",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolActorPoolTest, "FPSGame.ObjectPooling.Integration.ActorManagement",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolComponentPoolTest, "FPSGame.ObjectPooling.Integration.ComponentManagement",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 * Test Actor class for object pooling tests
 */
UCLASS()
class FPSGAME_API ATestPooledActor : public AActor
{
    GENERATED_BODY()

public:
    ATestPooledActor();

    // Test properties to verify object state
    UPROPERTY()
    int32 TestValue;

    UPROPERTY()
    FString TestString;

    UPROPERTY()
    bool bIsActive;

    // Memory footprint calculation
    virtual SIZE_T GetObjectSize() const;

    // Reset for pooling
    virtual void ResetForPool();

    // Validation
    virtual bool IsValidForPool() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
    class UStaticMeshComponent* MeshComponent;
};

/**
 * Test Component class for object pooling tests
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UTestPooledComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTestPooledComponent();

    // Test properties
    UPROPERTY()
    float TestFloat;

    UPROPERTY()
    TArray<int32> TestArray;

    // Memory and validation methods
    virtual SIZE_T GetObjectSize() const;
    virtual void ResetForPool();
    virtual bool IsValidForPool() const;
};

/**
 * Test UObject class for object pooling tests
 */
UCLASS()
class FPSGAME_API UTestPooledObject : public UObject
{
    GENERATED_BODY()

public:
    UTestPooledObject();

    // Test properties
    UPROPERTY()
    int32 UniqueId;

    UPROPERTY()
    TMap<FString, float> TestMap;

    // Memory and validation methods
    virtual SIZE_T GetObjectSize() const;
    virtual void ResetForPool();
    virtual bool IsValidForPool() const;

    // Track creation for testing
    static int32 CreationCount;
    static int32 DestructionCount;

protected:
    virtual void BeginDestroy() override;
};

/**
 * Performance measurement utilities for object pool testing
 */
USTRUCT()
struct FPerformanceTimer
{
    GENERATED_BODY()

    double StartTime = 0.0;
    double EndTime = 0.0;
    double Duration = 0.0;
    
    void Start() { StartTime = FPlatformTime::Seconds(); }
    void Stop() 
    { 
        EndTime = FPlatformTime::Seconds(); 
        Duration = EndTime - StartTime;
    }
    double GetDurationMs() const { return Duration * 1000.0; }
};

/**
 * Memory snapshot utilities for memory stress testing
 */
USTRUCT()
struct FMemorySnapshot
{
    GENERATED_BODY()

    SIZE_T UsedPhysical = 0;
    SIZE_T UsedVirtual = 0;
    SIZE_T PeakUsedPhysical = 0;
    SIZE_T PeakUsedVirtual = 0;
    SIZE_T AvailablePhysical = 0;
    SIZE_T AvailableVirtual = 0;
    
    void Capture()
    {
        FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
        UsedPhysical = Stats.UsedPhysical;
        UsedVirtual = Stats.UsedVirtual;
        PeakUsedPhysical = Stats.PeakUsedPhysical;
        PeakUsedVirtual = Stats.PeakUsedVirtual;
        AvailablePhysical = Stats.AvailablePhysical;
        AvailableVirtual = Stats.AvailableVirtual;
    }
    
    SIZE_T GetMemoryDelta(const FMemorySnapshot& Other) const
    {
        return (UsedPhysical > Other.UsedPhysical) ? 
               (UsedPhysical - Other.UsedPhysical) : 0;
    }
};

/**
 * Performance testing utilities
 */
namespace ObjectPoolTestUtils
{
    // Timing utilities
    struct FPerformanceTimer
    {
        double StartTime;
        FString TestName;

        FPerformanceTimer(const FString& InTestName);
        ~FPerformanceTimer();
        double GetElapsedTime() const;
    };

    // Memory tracking utilities
    struct FMemorySnapshot
    {
        SIZE_T UsedMemoryBefore;
        SIZE_T UsedMemoryAfter;
        SIZE_T PeakMemoryUsage;

        FMemorySnapshot();
        void TakeSnapshot();
        SIZE_T GetMemoryDelta() const;
    };

    // Test data generation
    class FTestDataGenerator
    {
    public:
        static TArray<int32> GenerateRandomIntArray(int32 Size);
        static TMap<FString, float> GenerateRandomMap(int32 Size);
        static FString GenerateRandomString(int32 Length);
    };

    // Pool validation utilities
    class FPoolValidator
    {
    public:
        template<typename T>
        static bool ValidatePoolState(const FAdvancedObjectPool<T>& Pool);

        template<typename T>
        static bool ValidatePoolStatistics(const FAdvancedObjectPool<T>& Pool);

        static bool ValidateManagerState(UAdvancedObjectPoolManager* Manager);
    };
}

/**
 * Thread safety testing utilities
 */
namespace ThreadSafetyTestUtils
{
    // Multi-threaded test executor
    class FMultiThreadTestExecutor
    {
    public:
        struct FThreadTestResult
        {
            bool bSuccess;
            FString ErrorMessage;
            double ExecutionTime;
            int32 OperationsCompleted;
        };

        template<typename TObject>
        static TArray<FThreadTestResult> RunConcurrentAcquisitionTest(
            FAdvancedObjectPool<TObject>& Pool,
            int32 NumThreads,
            int32 OperationsPerThread,
            float TestDurationSeconds
        );

        template<typename TObject>
        static bool TestRaceConditions(
            FAdvancedObjectPool<TObject>& Pool,
            int32 NumThreads = 8
        );
    };
}

/**
 * Integration test scenarios
 */
namespace IntegrationTestScenarios
{
    // FPS game simulation scenarios
    class FFPSGameSimulator
    {
    public:
        static bool SimulateBulletSpawning(UAdvancedObjectPoolManager* Manager, int32 BulletCount);
        static bool SimulateParticleEffects(UAdvancedObjectPoolManager* Manager, int32 EffectCount);
        static bool SimulateAudioSources(UAdvancedObjectPoolManager* Manager, int32 AudioCount);
        static bool SimulateDecalSystem(UAdvancedObjectPoolManager* Manager, int32 DecalCount);
        static bool SimulateCombatScenario(UAdvancedObjectPoolManager* Manager);
    };

    // Performance benchmarking
    class FPerformanceBenchmark
    {
    public:
        struct FBenchmarkResult
        {
            double AverageAcquisitionTime;
            double AverageReturnTime;
            double PeakMemoryUsage;
            double CacheHitRate;
            int32 ObjectsProcessed;
        };

        static FBenchmarkResult BenchmarkPoolPerformance(
            UAdvancedObjectPoolManager* Manager,
            const FString& PoolName,
            int32 IterationCount
        );

        static void GeneratePerformanceReport(
            const TArray<FBenchmarkResult>& Results,
            const FString& ReportPath
        );
    };
}
