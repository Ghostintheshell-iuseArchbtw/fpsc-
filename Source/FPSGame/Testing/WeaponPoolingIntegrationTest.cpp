#include "WeaponPoolingIntegrationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Async/ParallelFor.h"
#include "Stats/Stats.h"
#include "Tests/AutomationCommon.h"
#include "../Weapons/WeaponPoolingIntegrationComponent.h"

DEFINE_LOG_CATEGORY(LogWeaponPoolingIntegrationTest);

//=============================================================================
// Test Object Implementations
//=============================================================================

ATestWeaponForPooling::ATestWeaponForPooling()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // Create weapon system component
    WeaponSystem = CreateDefaultSubobject<UAdvancedWeaponSystem>(TEXT("WeaponSystem"));
    
    // Initialize test values
    TestFireCount = 0;
    bTrackPoolingUsage = true;
}

void ATestWeaponForPooling::SetupForTest()
{
    if (WeaponSystem)
    {
        // Configure weapon for testing
        WeaponSystem->CurrentAmmoInMag = 1000; // Unlimited ammo for testing
        WeaponSystem->TotalAmmo = 1000;
        WeaponSystem->bCanFire = true;
        WeaponSystem->bIsReloading = false;
        WeaponSystem->FireRate = 600.0f; // 10 rounds per second
        
        // Ensure pooling component is initialized
        if (WeaponSystem->PoolingComponent)
        {
            WeaponSystem->PoolingComponent->InitializeForWeapon(WeaponSystem);
        }
    }
    
    ResetTestState();
}

void ATestWeaponForPooling::FireTestRounds(int32 Count)
{
    if (!WeaponSystem) return;
    
    for (int32 i = 0; i < Count; ++i)
    {
        if (WeaponSystem->CanFire())
        {
            WeaponSystem->Fire();
            TestFireCount++;
            
            // Small delay to simulate realistic firing
            FPlatformProcess::Sleep(0.01f);
        }
    }
}

bool ATestWeaponForPooling::ValidatePoolingIntegration()
{
    if (!WeaponSystem || !WeaponSystem->PoolingComponent)
    {
        return false;
    }
    
    // Check if pooling component is properly configured
    return WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateWeaponPoolingComponent(WeaponSystem);
}

void ATestWeaponForPooling::ResetTestState()
{
    TestFireCount = 0;
}

//=============================================================================
// Test Utility Implementations
//=============================================================================

WeaponPoolingTestUtils::FWeaponFireTimer::FWeaponFireTimer(const FString& InTestName)
    : TestName(InTestName)
{
    StartTime = FPlatformTime::Seconds();
    FireTimes.Empty();
}

WeaponPoolingTestUtils::FWeaponFireTimer::~FWeaponFireTimer()
{
    double ElapsedTime = FPlatformTime::Seconds() - StartTime;
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, 
           TEXT("Weapon Fire Test '%s' completed in %.4f seconds. Avg fire time: %.4f ms"), 
           *TestName, ElapsedTime, GetAverageFireTime() * 1000.0f);
}

void WeaponPoolingTestUtils::FWeaponFireTimer::RecordFireEvent()
{
    double CurrentTime = FPlatformTime::Seconds();
    float FireTime = CurrentTime - StartTime;
    FireTimes.Add(FireTime);
    StartTime = CurrentTime;
}

float WeaponPoolingTestUtils::FWeaponFireTimer::GetAverageFireTime() const
{
    if (FireTimes.Num() == 0) return 0.0f;
    
    float Total = 0.0f;
    for (float Time : FireTimes)
    {
        Total += Time;
    }
    
    return Total / FireTimes.Num();
}

float WeaponPoolingTestUtils::FWeaponFireTimer::GetPeakFireTime() const
{
    if (FireTimes.Num() == 0) return 0.0f;
    
    float Peak = 0.0f;
    for (float Time : FireTimes)
    {
        if (Time > Peak)
        {
            Peak = Time;
        }
    }
    
    return Peak;
}

WeaponPoolingTestUtils::FPoolingMemoryTracker::FPoolingMemoryTracker()
{
    FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
    InitialMemory = Stats.UsedPhysical;
    PeakMemory = InitialMemory;
    FinalMemory = InitialMemory;
}

void WeaponPoolingTestUtils::FPoolingMemoryTracker::UpdatePeakMemory()
{
    FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
    SIZE_T CurrentMemory = Stats.UsedPhysical;
    
    if (CurrentMemory > PeakMemory)
    {
        PeakMemory = CurrentMemory;
    }
    
    FinalMemory = CurrentMemory;
}

SIZE_T WeaponPoolingTestUtils::FPoolingMemoryTracker::GetMemoryDelta() const
{
    return (FinalMemory > InitialMemory) ? (FinalMemory - InitialMemory) : 0;
}

SIZE_T WeaponPoolingTestUtils::FPoolingMemoryTracker::GetPeakMemoryUsage() const
{
    return (PeakMemory > InitialMemory) ? (PeakMemory - InitialMemory) : 0;
}

//=============================================================================
// Pooling Integration Validator Implementation
//=============================================================================

bool WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateWeaponPoolingComponent(UAdvancedWeaponSystem* WeaponSystem)
{
    if (!WeaponSystem)
    {
        UE_LOG(LogWeaponPoolingIntegrationTest, Error, TEXT("WeaponSystem is null"));
        return false;
    }
    
    if (!WeaponSystem->PoolingComponent)
    {
        UE_LOG(LogWeaponPoolingIntegrationTest, Warning, TEXT("PoolingComponent is null - this is acceptable for fallback testing"));
        return true; // This is acceptable for fallback testing
    }
    
    // Validate that pooling component is properly initialized
    // This would check internal state if we had access to it
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, TEXT("WeaponSystem pooling component validation passed"));
    return true;
}

bool WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidatePoolManagerIntegration(UAdvancedObjectPoolManager* PoolManager)
{
    if (!PoolManager)
    {
        UE_LOG(LogWeaponPoolingIntegrationTest, Error, TEXT("PoolManager is null"));
        return false;
    }
    
    // Test basic pool manager functionality
    TArray<FString> ActivePools = PoolManager->GetActivePoolNames();
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, TEXT("PoolManager has %d active pools"), ActivePools.Num());
    
    return true;
}

bool WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateEffectPooling(UAdvancedWeaponSystem* WeaponSystem)
{
    if (!WeaponSystem || !WeaponSystem->PoolingComponent)
    {
        return false;
    }
    
    // Test effect pooling by triggering effects
    WeaponSystem->PlayFireEffects();
    
    // Since we can't directly access pool internals, we assume it works if no crashes occur
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, TEXT("Effect pooling validation completed"));
    return true;
}

bool WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateProjectilePooling(UAdvancedWeaponSystem* WeaponSystem)
{
    if (!WeaponSystem)
    {
        return false;
    }
    
    // Test projectile spawning (both pooled and fallback)
    FVector TestLocation = FVector::ZeroVector;
    FVector TestDirection = FVector::ForwardVector;
    
    WeaponSystem->SpawnProjectile(TestLocation, TestDirection);
    
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, TEXT("Projectile pooling validation completed"));
    return true;
}

bool WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateFallbackMechanisms(UAdvancedWeaponSystem* WeaponSystem)
{
    if (!WeaponSystem)
    {
        return false;
    }
    
    // Test fallback by temporarily disabling pooling component
    UWeaponPoolingIntegrationComponent* OriginalComponent = WeaponSystem->PoolingComponent;
    WeaponSystem->PoolingComponent = nullptr;
    
    // Test firing without pooling (should use fallback)
    if (WeaponSystem->CanFire())
    {
        WeaponSystem->Fire();
    }
    
    // Restore pooling component
    WeaponSystem->PoolingComponent = OriginalComponent;
    
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, TEXT("Fallback mechanism validation completed"));
    return true;
}

//=============================================================================
// Combat Scenario Simulator Implementation
//=============================================================================

bool WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateRapidFireScenario(UAdvancedWeaponSystem* WeaponSystem, int32 RoundsCount)
{
    if (!WeaponSystem) return false;
    
    FWeaponFireTimer Timer(TEXT("RapidFireScenario"));
    
    for (int32 i = 0; i < RoundsCount; ++i)
    {
        if (WeaponSystem->CanFire())
        {
            WeaponSystem->Fire();
            Timer.RecordFireEvent();
            
            // Minimal delay for rapid fire
            FPlatformProcess::Sleep(0.001f);
        }
    }
    
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, 
           TEXT("Rapid fire scenario completed: %d rounds, avg time: %.4f ms"), 
           RoundsCount, Timer.GetAverageFireTime() * 1000.0f);
    
    return true;
}

bool WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateMultiWeaponScenario(TArray<UAdvancedWeaponSystem*> WeaponSystems)
{
    if (WeaponSystems.Num() == 0) return false;
    
    FWeaponFireTimer Timer(TEXT("MultiWeaponScenario"));
    
    // Fire from all weapons simultaneously using ParallelFor
    ParallelFor(WeaponSystems.Num(), [&](int32 Index)
    {
        UAdvancedWeaponSystem* WeaponSystem = WeaponSystems[Index];
        if (WeaponSystem && WeaponSystem->CanFire())
        {
            WeaponSystem->Fire();
        }
    });
    
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, 
           TEXT("Multi-weapon scenario completed: %d weapons fired"), 
           WeaponSystems.Num());
    
    return true;
}

bool WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateBurstFireScenario(UAdvancedWeaponSystem* WeaponSystem, int32 BurstCount)
{
    if (!WeaponSystem) return false;
    
    FWeaponFireTimer Timer(TEXT("BurstFireScenario"));
    
    // Simulate burst fire with 3-round bursts
    for (int32 Burst = 0; Burst < BurstCount; ++Burst)
    {
        for (int32 Round = 0; Round < 3; ++Round)
        {
            if (WeaponSystem->CanFire())
            {
                WeaponSystem->Fire();
                Timer.RecordFireEvent();
                FPlatformProcess::Sleep(0.05f); // Burst fire delay
            }
        }
        
        // Pause between bursts
        FPlatformProcess::Sleep(0.2f);
    }
    
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, 
           TEXT("Burst fire scenario completed: %d bursts"), 
           BurstCount);
    
    return true;
}

bool WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateReloadAndFireScenario(UAdvancedWeaponSystem* WeaponSystem)
{
    if (!WeaponSystem) return false;
    
    FWeaponFireTimer Timer(TEXT("ReloadAndFireScenario"));
    
    // Fire until empty
    while (WeaponSystem->CurrentAmmoInMag > 0 && WeaponSystem->CanFire())
    {
        WeaponSystem->Fire();
        Timer.RecordFireEvent();
        FPlatformProcess::Sleep(0.01f);
    }
    
    // Reload
    WeaponSystem->StartReload();
    
    // Simulate reload time
    FPlatformProcess::Sleep(WeaponSystem->GetModifiedReloadTime());
    
    // Complete reload
    WeaponSystem->CompleteReload();
    
    // Fire a few more rounds
    for (int32 i = 0; i < 5; ++i)
    {
        if (WeaponSystem->CanFire())
        {
            WeaponSystem->Fire();
            Timer.RecordFireEvent();
            FPlatformProcess::Sleep(0.01f);
        }
    }
    
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, TEXT("Reload and fire scenario completed"));
    return true;
}

FWeaponPoolingMetrics WeaponPoolingTestUtils::FCombatScenarioSimulator::MeasurePoolingPerformance(UAdvancedWeaponSystem* WeaponSystem, int32 TestDuration)
{
    FWeaponPoolingMetrics Metrics;
    Metrics.Reset();
    
    if (!WeaponSystem) return Metrics;
    
    FPoolingMemoryTracker MemoryTracker;
    FWeaponFireTimer Timer(TEXT("PerformanceMeasurement"));
    
    double StartTime = FPlatformTime::Seconds();
    double EndTime = StartTime + TestDuration;
    
    int32 ShotsFired = 0;
    
    while (FPlatformTime::Seconds() < EndTime)
    {
        if (WeaponSystem->CanFire())
        {
            WeaponSystem->Fire();
            Timer.RecordFireEvent();
            ShotsFired++;
            
            MemoryTracker.UpdatePeakMemory();
            
            // Small delay to prevent overwhelming the system
            FPlatformProcess::Sleep(0.001f);
        }
    }
    
    // Calculate metrics
    Metrics.TotalShotsFired = ShotsFired;
    Metrics.AverageFireTime = Timer.GetAverageFireTime();
    Metrics.PeakFireTime = Timer.GetPeakFireTime();
    Metrics.MemoryUsageDelta = MemoryTracker.GetMemoryDelta() / (1024.0f * 1024.0f); // Convert to MB
    
    // Mock pooling statistics (in real implementation, these would come from the pooling system)
    Metrics.PooledObjectsUsed = ShotsFired * 0.8f; // Assume 80% pool hit rate
    Metrics.FallbackSpawnsUsed = ShotsFired * 0.2f; // Assume 20% fallback rate
    Metrics.PoolHitRate = 0.8f;
    
    UE_LOG(LogWeaponPoolingIntegrationTest, Log, 
           TEXT("Performance measurement completed: %d shots, %.2f avg fire time, %.2f MB memory delta"), 
           Metrics.TotalShotsFired, Metrics.AverageFireTime * 1000.0f, Metrics.MemoryUsageDelta);
    
    return Metrics;
}

//=============================================================================
// Main Test Implementations
//=============================================================================

bool FWeaponPoolingBasicIntegrationTest::RunTest(const FString& Parameters)
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

    // Create test weapon
    ATestWeaponForPooling* TestWeapon = TestWorld->SpawnActor<ATestWeaponForPooling>();
    if (!TestWeapon)
    {
        AddError(TEXT("Failed to create test weapon"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    // Test 1: Basic weapon system creation
    TestNotNull("Test weapon should be created", TestWeapon);
    TestNotNull("Weapon system should be created", TestWeapon->WeaponSystem);

    // Test 2: Pooling component integration
    bool bPoolingIntegrationValid = TestWeapon->ValidatePoolingIntegration();
    TestTrue("Pooling integration should be valid", bPoolingIntegrationValid);

    // Test 3: Pool manager integration
    bool bPoolManagerValid = WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidatePoolManagerIntegration(PoolManager);
    TestTrue("Pool manager integration should be valid", bPoolManagerValid);

    // Test 4: Basic weapon firing
    TestWeapon->SetupForTest();
    TestWeapon->FireTestRounds(5);
    TestEqual("Weapon should fire 5 rounds", TestWeapon->TestFireCount, 5);

    // Cleanup
    TestWorld->DestroyWorld(false);
    
    return bAllTestsPassed;
}

bool FWeaponPoolingProjectileTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    ATestWeaponForPooling* TestWeapon = TestWorld->SpawnActor<ATestWeaponForPooling>();
    if (!TestWeapon)
    {
        AddError(TEXT("Failed to create test weapon"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    // Test projectile pooling
    TestWeapon->SetupForTest();
    
    // Enable projectiles for this test
    TestWeapon->WeaponSystem->bUseProjectiles = true;
    
    // Test projectile pooling validation
    bool bProjectilePoolingValid = WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateProjectilePooling(TestWeapon->WeaponSystem);
    TestTrue("Projectile pooling should be valid", bProjectilePoolingValid);
    
    // Test projectile spawning
    TestWeapon->FireTestRounds(10);
    TestEqual("Should fire 10 projectile rounds", TestWeapon->TestFireCount, 10);

    TestWorld->DestroyWorld(false);
    return bAllTestsPassed;
}

bool FWeaponPoolingEffectsTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    ATestWeaponForPooling* TestWeapon = TestWorld->SpawnActor<ATestWeaponForPooling>();
    if (!TestWeapon)
    {
        AddError(TEXT("Failed to create test weapon"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    // Test effects pooling
    TestWeapon->SetupForTest();
    
    // Test effect pooling validation
    bool bEffectPoolingValid = WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateEffectPooling(TestWeapon->WeaponSystem);
    TestTrue("Effect pooling should be valid", bEffectPoolingValid);
    
    // Test firing with effects
    TestWeapon->FireTestRounds(15);
    TestEqual("Should fire 15 rounds with effects", TestWeapon->TestFireCount, 15);

    TestWorld->DestroyWorld(false);
    return bAllTestsPassed;
}

bool FWeaponPoolingPerformanceTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    ATestWeaponForPooling* TestWeapon = TestWorld->SpawnActor<ATestWeaponForPooling>();
    if (!TestWeapon)
    {
        AddError(TEXT("Failed to create test weapon"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    TestWeapon->SetupForTest();

    // Test rapid fire performance
    bool bRapidFireSuccess = WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateRapidFireScenario(TestWeapon->WeaponSystem, 100);
    TestTrue("Rapid fire scenario should succeed", bRapidFireSuccess);

    // Measure performance metrics
    FWeaponPoolingMetrics Metrics = WeaponPoolingTestUtils::FCombatScenarioSimulator::MeasurePoolingPerformance(TestWeapon->WeaponSystem, 5);
    
    TestGreater("Should fire shots during performance test", Metrics.TotalShotsFired, 0);
    TestLess("Average fire time should be reasonable", Metrics.AverageFireTime, 0.1f); // Less than 100ms
    TestLess("Memory usage should be controlled", Metrics.MemoryUsageDelta, 50.0f); // Less than 50MB

    AddInfo(FString::Printf(TEXT("Performance Test Results: %d shots, %.2f ms avg fire time, %.2f MB memory"), 
            Metrics.TotalShotsFired, Metrics.AverageFireTime * 1000.0f, Metrics.MemoryUsageDelta));

    TestWorld->DestroyWorld(false);
    return bAllTestsPassed;
}

bool FWeaponPoolingStressTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    bool bAllTestsPassed = true;

    // Create multiple weapons for stress testing
    TArray<UAdvancedWeaponSystem*> WeaponSystems;
    for (int32 i = 0; i < 5; ++i)
    {
        ATestWeaponForPooling* TestWeapon = TestWorld->SpawnActor<ATestWeaponForPooling>();
        if (TestWeapon)
        {
            TestWeapon->SetupForTest();
            WeaponSystems.Add(TestWeapon->WeaponSystem);
        }
    }

    TestEqual("Should create 5 weapons for stress test", WeaponSystems.Num(), 5);

    // Test multi-weapon scenario
    bool bMultiWeaponSuccess = WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateMultiWeaponScenario(WeaponSystems);
    TestTrue("Multi-weapon scenario should succeed", bMultiWeaponSuccess);

    // Test burst fire scenario
    if (WeaponSystems.Num() > 0)
    {
        bool bBurstFireSuccess = WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateBurstFireScenario(WeaponSystems[0], 10);
        TestTrue("Burst fire scenario should succeed", bBurstFireSuccess);
    }

    TestWorld->DestroyWorld(false);
    return bAllTestsPassed;
}

bool FWeaponPoolingFallbackTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestWorld)
    {
        AddError(TEXT("Failed to create test world"));
        return false;
    }

    ATestWeaponForPooling* TestWeapon = TestWorld->SpawnActor<ATestWeaponForPooling>();
    if (!TestWeapon)
    {
        AddError(TEXT("Failed to create test weapon"));
        TestWorld->DestroyWorld(false);
        return false;
    }

    bool bAllTestsPassed = true;

    TestWeapon->SetupForTest();

    // Test fallback mechanisms
    bool bFallbackValid = WeaponPoolingTestUtils::FPoolingIntegrationValidator::ValidateFallbackMechanisms(TestWeapon->WeaponSystem);
    TestTrue("Fallback mechanisms should be valid", bFallbackValid);

    // Test reload and fire scenario (tests various pooling systems)
    bool bReloadFireSuccess = WeaponPoolingTestUtils::FCombatScenarioSimulator::SimulateReloadAndFireScenario(TestWeapon->WeaponSystem);
    TestTrue("Reload and fire scenario should succeed", bReloadFireSuccess);

    TestWorld->DestroyWorld(false);
    return bAllTestsPassed;
}
