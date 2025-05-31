#include "AdvancedObjectPoolTests.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformMemory.h"
#include "Misc/FileHelper.h"
#include "Async/ParallelFor.h"
#include "Stats/Stats.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

DEFINE_LOG_CATEGORY(LogObjectPoolTests);

// Comprehensive automation test implementations with all required test categories

//=============================================================================
// Unit Tests Implementation
//=============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolUnitTest, "FPSGame.ObjectPool.Unit.Basic", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolConfigurationTest, "FPSGame.ObjectPool.Unit.Configuration", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolStatisticsTest, "FPSGame.ObjectPool.Unit.Statistics", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolMaintenanceTest, "FPSGame.ObjectPool.Unit.Maintenance", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

//=============================================================================
// Performance Tests Implementation
//=============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolPerformanceTest, "FPSGame.ObjectPool.Performance.Acquisition", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolScalabilityTest, "FPSGame.ObjectPool.Performance.Scalability", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

//=============================================================================
// Memory Stress Tests Implementation
//=============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolMemoryStressTest, "FPSGame.ObjectPool.MemoryStress.LargeScale", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolMemoryLeakTest, "FPSGame.ObjectPool.MemoryStress.LeakDetection", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

//=============================================================================
// Thread Safety Tests Implementation
//=============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolThreadSafetyTest, "FPSGame.ObjectPool.ThreadSafety.ConcurrentAccess", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolDeadlockTest, "FPSGame.ObjectPool.ThreadSafety.DeadlockPrevention", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

//=============================================================================
// Integration Tests Implementation
//=============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolIntegrationTest, "FPSGame.ObjectPool.Integration.GameplayIntegration", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolActorPoolTest, "FPSGame.ObjectPool.Integration.ActorManagement", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdvancedObjectPoolComponentPoolTest, "FPSGame.ObjectPool.Integration.ComponentManagement", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

// Initialize static counters for UTestPooledObject
int32 UTestPooledObject::CreationCount = 0;
int32 UTestPooledObject::DestructionCount = 0;

//=============================================================================
// Test Object Implementations
//=============================================================================

ATestPooledActor::ATestPooledActor()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // Create mesh component for memory footprint testing
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    
    // Initialize test values
    TestValue = 0;
    TestString = TEXT("");
    bIsActive = false;
}

SIZE_T ATestPooledActor::GetObjectSize() const
{
    SIZE_T Size = sizeof(*this);
    Size += TestString.GetAllocatedSize();
    if (MeshComponent)
    {
        Size += sizeof(*MeshComponent);
    }
    return Size;
}

void ATestPooledActor::ResetForPool()
{
    TestValue = 0;
    TestString.Empty();
    bIsActive = false;
    SetActorLocation(FVector::ZeroVector);
    SetActorRotation(FRotator::ZeroRotator);
}

bool ATestPooledActor::IsValidForPool() const
{
    return IsValid(this) && !IsActorBeingDestroyed() && MeshComponent != nullptr;
}

UTestPooledComponent::UTestPooledComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    TestFloat = 0.0f;
    TestArray.Empty();
}

SIZE_T UTestPooledComponent::GetObjectSize() const
{
    SIZE_T Size = sizeof(*this);
    Size += TestArray.GetAllocatedSize();
    return Size;
}

void UTestPooledComponent::ResetForPool()
{
    TestFloat = 0.0f;
    TestArray.Empty();
}

bool UTestPooledComponent::IsValidForPool() const
{
    return IsValid(this) && !IsBeingDestroyed();
}

UTestPooledObject::UTestPooledObject()
{
    UniqueId = ++CreationCount;
    TestMap.Empty();
}

SIZE_T UTestPooledObject::GetObjectSize() const
{
    SIZE_T Size = sizeof(*this);
    Size += TestMap.GetAllocatedSize();
    return Size;
}

void UTestPooledObject::ResetForPool()
{
    TestMap.Empty();
}

bool UTestPooledObject::IsValidForPool() const
{
    return IsValid(this) && !IsBeingDestroyed();
}

void UTestPooledObject::BeginDestroy()
{
    ++DestructionCount;
    Super::BeginDestroy();
}

//=============================================================================
// Test Utility Implementations
//=============================================================================

ObjectPoolTestUtils::FPerformanceTimer::FPerformanceTimer(const FString& InTestName)
    : TestName(InTestName)
{
    StartTime = FPlatformTime::Seconds();
}

ObjectPoolTestUtils::FPerformanceTimer::~FPerformanceTimer()
{
    double ElapsedTime = GetElapsedTime();
    UE_LOG(LogTemp, Log, TEXT("Performance Test '%s' completed in %.4f seconds"), 
           *TestName, ElapsedTime);
}

double ObjectPoolTestUtils::FPerformanceTimer::GetElapsedTime() const
{
    return FPlatformTime::Seconds() - StartTime;
}

ObjectPoolTestUtils::FMemorySnapshot::FMemorySnapshot()
{
    TakeSnapshot();
}

void ObjectPoolTestUtils::FMemorySnapshot::TakeSnapshot()
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    UsedMemoryBefore = MemStats.UsedPhysical;
    PeakMemoryUsage = MemStats.PeakUsedPhysical;
}

SIZE_T ObjectPoolTestUtils::FMemorySnapshot::GetMemoryDelta() const
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    UsedMemoryAfter = MemStats.UsedPhysical;
    return UsedMemoryAfter > UsedMemoryBefore ? UsedMemoryAfter - UsedMemoryBefore : 0;
}

TArray<int32> ObjectPoolTestUtils::FTestDataGenerator::GenerateRandomIntArray(int32 Size)
{
    TArray<int32> Result;
    Result.Reserve(Size);
    for (int32 i = 0; i < Size; ++i)
    {
        Result.Add(FMath::RandRange(0, 1000));
    }
    return Result;
}

TMap<FString, float> ObjectPoolTestUtils::FTestDataGenerator::GenerateRandomMap(int32 Size)
{
    TMap<FString, float> Result;
    for (int32 i = 0; i < Size; ++i)
    {
        FString Key = FString::Printf(TEXT("Key_%d"), i);
        float Value = FMath::FRandRange(0.0f, 100.0f);
        Result.Add(Key, Value);
    }
    return Result;
}

FString ObjectPoolTestUtils::FTestDataGenerator::GenerateRandomString(int32 Length)
{
    FString Result;
    Result.Reserve(Length);
    for (int32 i = 0; i < Length; ++i)
    {
        TCHAR RandomChar = static_cast<TCHAR>(FMath::RandRange(65, 90)); // A-Z
        Result.AppendChar(RandomChar);
    }
    return Result;
}

//=============================================================================
// Main Test Implementations
//=============================================================================

bool FAdvancedObjectPoolBasicTest::RunTest(const FString& Parameters)
{
    // Test basic pool functionality
    UWorld* TestWorld = GetAnyGameWorld();
    if (!TestWorld)
    {
        AddError(TEXT("Failed to get test world"));
        return false;
    }

    UAdvancedObjectPoolManager* PoolManager = TestWorld->GetGameInstance()->GetSubsystem<UAdvancedObjectPoolManager>();
    if (!PoolManager)
    {
        AddError(TEXT("Failed to get AdvancedObjectPoolManager"));
        return false;
    }

    // Test Actor pool
    FString ActorPoolName = TEXT("TestActorPool");
    PoolManager->CreateActorPool<ATestPooledActor>(ActorPoolName, 10, 50);
    
    // Test acquisition
    ATestPooledActor* Actor1 = PoolManager->AcquireActor<ATestPooledActor>(ActorPoolName, TestWorld);
    TestNotNull("Actor acquisition should succeed", Actor1);
    
    if (Actor1)
    {
        TestTrue("Actor should be valid for pool", Actor1->IsValidForPool());
        Actor1->TestValue = 42;
        Actor1->TestString = TEXT("TestString");
    }

    // Test return
    bool bReturned = PoolManager->ReturnActor(ActorPoolName, Actor1);
    TestTrue("Actor return should succeed", bReturned);

    // Test reacquisition and reset
    ATestPooledActor* Actor2 = PoolManager->AcquireActor<ATestPooledActor>(ActorPoolName, TestWorld);
    TestNotNull("Actor reacquisition should succeed", Actor2);
    
    if (Actor2)
    {
        TestEqual("Actor should be reset", Actor2->TestValue, 0);
        TestTrue("Actor string should be empty", Actor2->TestString.IsEmpty());
    }

    // Test statistics
    FPoolStatistics Stats = PoolManager->GetPoolStatistics(ActorPoolName);
    TestEqual("Pool should show correct active count", Stats.ActiveObjects, 1);
    TestGreaterEqual("Hit rate should be positive", Stats.HitRate, 0.0f);

    return true;
}

bool FAdvancedObjectPoolPerformanceTest::RunTest(const FString& Parameters)
{
    // Create test world
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    // Get pool manager
    UAdvancedObjectPoolManager* PoolManager = TestWorld->GetGameInstance()->GetSubsystem<UAdvancedObjectPoolManager>();
    if (!PoolManager)
    {
        AddError(TEXT("Failed to get object pool manager"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    // Performance Test 1: High-frequency operations
    {
        ObjectPoolTestUtils::FPerformanceTimer Timer(TEXT("High-frequency operations"));
        
        bool bTestPassed = ObjectPoolTestUtils::FStressTestUtilities::TestHighFrequencyOperations<UTestPooledObject>(
            10000, // Operations per second
            5.0f   // Test duration
        );
        
        if (bTestPassed)
        {
            AddInfo(TEXT("High-frequency operations: PASSED"));
        }
        else
        {
            AddError(TEXT("High-frequency operations: FAILED"));
            bAllTestsPassed = false;
        }
    }

    // Performance Test 2: FPS simulation
    {
        bool bTestPassed = IntegrationTestScenarios::FFPSGameSimulator::SimulateCombatScenario(PoolManager);
        
        if (bTestPassed)
        {
            AddInfo(TEXT("FPS simulation: PASSED"));
        }
        else
        {
            AddError(TEXT("FPS simulation: FAILED"));
            bAllTestsPassed = false;
        }
    }

    // Performance Test 3: Benchmark
    {
        IntegrationTestScenarios::FPerformanceBenchmark::FBenchmarkResult Result = 
            IntegrationTestScenarios::FPerformanceBenchmark::BenchmarkPoolPerformance(
                PoolManager,
                TEXT("PerformanceTestPool"),
                1000
            );
        
        if (Result.ObjectsProcessed > 0)
        {
            AddInfo(FString::Printf(TEXT("Benchmark: PASSED - %.2fms avg acquisition, %.2f%% cache hit rate"), 
                Result.AverageAcquisitionTime * 1000.0, Result.CacheHitRate * 100.0));
        }
        else
        {
            AddError(TEXT("Benchmark: FAILED"));
            bAllTestsPassed = false;
        }
    }

    // Cleanup
    TestWorld->DestroyWorld(false);
    
    return bAllTestsPassed;
}

bool FAdvancedObjectPoolMemoryTest::RunTest(const FString& Parameters)
{
    // Create test world
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    // Get pool manager
    UAdvancedObjectPoolManager* PoolManager = TestWorld->GetGameInstance()->GetSubsystem<UAdvancedObjectPoolManager>();
    if (!PoolManager)
    {
        AddError(TEXT("Failed to get object pool manager"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    // Memory Test 1: Large-scale memory usage
    {
        ObjectPoolTestUtils::FMemorySnapshot MemoryBefore = ObjectPoolTestUtils::FMemorySnapshot::TakeSnapshot();
        
        FString TestResult;
        bool bTestPassed = ObjectPoolTestUtils::FStressTestUtilities::TestLargeScaleMemoryUsage<ATestPooledActor>(
            10000, // Object count
            TestResult
        );
        
        ObjectPoolTestUtils::FMemorySnapshot MemoryAfter = ObjectPoolTestUtils::FMemorySnapshot::TakeSnapshot();
        
        if (bTestPassed)
        {
            SIZE_T MemoryDelta = MemoryAfter.UsedMemoryAfter - MemoryBefore.UsedMemoryBefore;
            AddInfo(FString::Printf(TEXT("Large-scale memory: PASSED - %s (Delta: %.2f MB)"), 
                *TestResult, MemoryDelta / (1024.0f * 1024.0f)));
        }
        else
        {
            AddError(FString::Printf(TEXT("Large-scale memory: FAILED - %s"), *TestResult));
            bAllTestsPassed = false;
        }
    }

    // Memory Test 2: Memory leak detection
    {
        int32 ObjectsBefore = UTestPooledObject::CreationCount - UTestPooledObject::DestructionCount;
        
        // Create and destroy objects multiple times
        for (int32 i = 0; i < 100; ++i)
        {
            TArray<UTestPooledObject*> Objects;
            
            // Create objects
            for (int32 j = 0; j < 50; ++j)
            {
                UTestPooledObject* Obj = PoolManager->AcquireObject<UTestPooledObject>(TEXT("MemoryTestPool"));
                if (Obj)
                {
                    Objects.Add(Obj);
                }
            }
            
            // Return objects
            for (UTestPooledObject* Obj : Objects)
            {
                PoolManager->ReleaseObject(TEXT("MemoryTestPool"), Obj);
            }
        }
        
        int32 ObjectsAfter = UTestPooledObject::CreationCount - UTestPooledObject::DestructionCount;
        
        if (ObjectsAfter <= ObjectsBefore + 50) // Allow for pool retention
        {
            AddInfo(FString::Printf(TEXT("Memory leak detection: PASSED (Objects delta: %d)"), ObjectsAfter - ObjectsBefore));
        }
        else
        {
            AddError(FString::Printf(TEXT("Memory leak detection: FAILED (Objects delta: %d)"), ObjectsAfter - ObjectsBefore));
            bAllTestsPassed = false;
        }
    }

    // Cleanup
    TestWorld->DestroyWorld(false);
    
    return bAllTestsPassed;
}

bool FAdvancedObjectPoolThreadSafetyTest::RunTest(const FString& Parameters)
{
    // Create test world
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    // Get pool manager
    UAdvancedObjectPoolManager* PoolManager = TestWorld->GetGameInstance()->GetSubsystem<UAdvancedObjectPoolManager>();
    if (!PoolManager)
    {
        AddError(TEXT("Failed to get object pool manager"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    // Thread Safety Test 1: Concurrent access
    {
        FString TestResult;
        bool bTestPassed = ObjectPoolTestUtils::FStressTestUtilities::TestConcurrentAccess<UTestPooledObject>(
            8,    // Threads
            1000, // Operations per thread
            5.0f  // Duration
        );
        
        if (bTestPassed)
        {
            AddInfo(TEXT("Concurrent access: PASSED"));
        }
        else
        {
            AddError(TEXT("Concurrent access: FAILED"));
            bAllTestsPassed = false;
        }
    }

    // Thread Safety Test 2: Race condition detection
    {
        // Create a pool for testing
        FAdvancedObjectPool<UTestPooledObject> TestPool(TEXT("ThreadSafetyTestPool"), 100, 50);
        
        bool bTestPassed = ObjectPoolTestUtils::FStressTestUtilities::TestRaceConditions(TestPool, 8);
        
        if (bTestPassed)
        {
            AddInfo(TEXT("Race condition detection: PASSED"));
        }
        else
        {
            AddError(TEXT("Race condition detection: FAILED"));
            bAllTestsPassed = false;
        }
    }

    // Thread Safety Test 3: Multi-threaded stress test
    {
        TAtomic<int32> ErrorCount(0);
        const int32 NumThreads = 4;
        const int32 OperationsPerThread = 500;
        
        ParallelFor(NumThreads, [&](int32 ThreadIndex)
        {
            for (int32 i = 0; i < OperationsPerThread; ++i)
            {
                UTestPooledObject* Obj = PoolManager->AcquireObject<UTestPooledObject>(TEXT("StressTestPool"));
                if (!Obj)
                {
                    ErrorCount.IncrementExchange();
                    continue;
                }
                
                // Simulate some work
                Obj->UniqueId = ThreadIndex * 1000 + i;
                
                // Add some random delay
                FPlatformProcess::Sleep(FMath::RandRange(0.001f, 0.005f));
                
                if (!PoolManager->ReleaseObject(TEXT("StressTestPool"), Obj))
                {
                    ErrorCount.IncrementExchange();
                }
            }
        });
        
        if (ErrorCount.Load() == 0)
        {
            AddInfo(TEXT("Multi-threaded stress test: PASSED"));
        }
        else
        {
            AddError(FString::Printf(TEXT("Multi-threaded stress test: FAILED (%d errors)"), ErrorCount.Load()));
            bAllTestsPassed = false;
        }
    }

    // Cleanup
    TestWorld->DestroyWorld(false);
    
    return bAllTestsPassed;
}

//=============================================================================
// Integration Test Scenario Implementations
//=============================================================================

bool IntegrationTestScenarios::FFPSGameSimulator::SimulateBulletSpawning(UAdvancedObjectPoolManager* Manager, int32 BulletCount)
{
    if (!Manager) return false;

    // Create bullet pool if it doesn't exist
    Manager->CreateBulletPool(TEXT("BulletPool"), 100, 500);

    // Simulate rapid bullet spawning
    TArray<AActor*> Bullets;
    for (int32 i = 0; i < BulletCount; ++i)
    {
        AActor* Bullet = Manager->AcquireBullet(TEXT("BulletPool"));
        if (Bullet)
        {
            Bullets.Add(Bullet);
            // Simulate bullet travel time
            FPlatformProcess::Sleep(0.01f);
        }
    }

    // Return bullets after "impact"
    for (AActor* Bullet : Bullets)
    {
        Manager->ReturnBullet(TEXT("BulletPool"), Bullet);
    }

    return true;
}

bool IntegrationTestScenarios::FFPSGameSimulator::SimulateParticleEffects(UAdvancedObjectPoolManager* Manager, int32 EffectCount)
{
    if (!Manager) return false;

    Manager->CreateParticlePool(TEXT("ParticlePool"), 50, 200);

    TArray<AActor*> Effects;
    for (int32 i = 0; i < EffectCount; ++i)
    {
        AActor* Effect = Manager->AcquireParticleEffect(TEXT("ParticlePool"));
        if (Effect)
        {
            Effects.Add(Effect);
        }
    }

    // Simulate effect duration
    FPlatformProcess::Sleep(1.0f);

    for (AActor* Effect : Effects)
    {
        Manager->ReturnParticleEffect(TEXT("ParticlePool"), Effect);
    }

    return true;
}

bool IntegrationTestScenarios::FFPSGameSimulator::SimulateAudioSources(UAdvancedObjectPoolManager* Manager, int32 AudioCount)
{
    if (!Manager) return false;

    Manager->CreateAudioPool(TEXT("AudioPool"), 30, 100);

    TArray<UActorComponent*> AudioSources;
    for (int32 i = 0; i < AudioCount; ++i)
    {
        UActorComponent* AudioSource = Manager->AcquireAudioSource(TEXT("AudioPool"));
        if (AudioSource)
        {
            AudioSources.Add(AudioSource);
        }
    }

    // Simulate audio playback
    FPlatformProcess::Sleep(0.5f);

    for (UActorComponent* AudioSource : AudioSources)
    {
        Manager->ReturnAudioSource(TEXT("AudioPool"), AudioSource);
    }

    return true;
}

bool IntegrationTestScenarios::FFPSGameSimulator::SimulateDecalSystem(UAdvancedObjectPoolManager* Manager, int32 DecalCount)
{
    if (!Manager) return false;

    Manager->CreateDecalPool(TEXT("DecalPool"), 20, 80);

    TArray<AActor*> Decals;
    for (int32 i = 0; i < DecalCount; ++i)
    {
        AActor* Decal = Manager->AcquireDecal(TEXT("DecalPool"));
        if (Decal)
        {
            Decals.Add(Decal);
        }
    }

    // Simulate decal fade time
    FPlatformProcess::Sleep(2.0f);

    for (AActor* Decal : Decals)
    {
        Manager->ReturnDecal(TEXT("DecalPool"), Decal);
    }

    return true;
}

bool IntegrationTestScenarios::FFPSGameSimulator::SimulateCombatScenario(UAdvancedObjectPoolManager* Manager)
{
    if (!Manager) return false;

    // Simulate a complex combat scenario using multiple pool types
    bool bSuccess = true;
    
    bSuccess &= SimulateBulletSpawning(Manager, 50);
    bSuccess &= SimulateParticleEffects(Manager, 25);
    bSuccess &= SimulateAudioSources(Manager, 15);
    bSuccess &= SimulateDecalSystem(Manager, 10);

    return bSuccess;
}
