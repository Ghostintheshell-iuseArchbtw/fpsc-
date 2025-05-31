#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "../Weapons/AdvancedWeaponSystem.h"
#include "../Optimization/AdvancedObjectPoolManager.h"
#include "WeaponPoolingIntegrationTest.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWeaponPoolingIntegrationTest, Log, All);

/**
 * Comprehensive test suite for weapon system integration with object pooling
 * Tests all aspects of the pooling integration including projectiles, effects, and performance
 */

// Unit Tests for Weapon Pooling Integration
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPoolingBasicIntegrationTest, "FPSGame.WeaponPooling.Integration.Basic",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPoolingProjectileTest, "FPSGame.WeaponPooling.Integration.Projectiles",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPoolingEffectsTest, "FPSGame.WeaponPooling.Integration.Effects",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPoolingPerformanceTest, "FPSGame.WeaponPooling.Performance.RapidFire",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPoolingStressTest, "FPSGame.WeaponPooling.Stress.MultiWeapon",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPoolingFallbackTest, "FPSGame.WeaponPooling.Integration.Fallback",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 * Test weapon class for pooling integration testing
 */
UCLASS()
class FPSGAME_API ATestWeaponForPooling : public AActor
{
    GENERATED_BODY()

public:
    ATestWeaponForPooling();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAdvancedWeaponSystem* WeaponSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Config")
    int32 TestFireCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Config")
    bool bTrackPoolingUsage = true;

    // Test helper functions
    void SetupForTest();
    void FireTestRounds(int32 Count);
    bool ValidatePoolingIntegration();
    void ResetTestState();
};

/**
 * Performance metrics for weapon pooling tests
 */
USTRUCT(BlueprintType)
struct FWeaponPoolingMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float AverageFireTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PeakFireTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalShotsFired = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PooledObjectsUsed = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 FallbackSpawnsUsed = 0;

    UPROPERTY(BlueprintReadOnly)
    float MemoryUsageDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PoolHitRate = 0.0f;

    void Reset()
    {
        AverageFireTime = 0.0f;
        PeakFireTime = 0.0f;
        TotalShotsFired = 0;
        PooledObjectsUsed = 0;
        FallbackSpawnsUsed = 0;
        MemoryUsageDelta = 0.0f;
        PoolHitRate = 0.0f;
    }
};

/**
 * Weapon pooling test utilities
 */
namespace WeaponPoolingTestUtils
{
    /**
     * Performance measurement utilities for weapon firing
     */
    struct FWeaponFireTimer
    {
        double StartTime;
        FString TestName;
        TArray<float> FireTimes;

        FWeaponFireTimer(const FString& InTestName);
        ~FWeaponFireTimer();
        
        void RecordFireEvent();
        float GetAverageFireTime() const;
        float GetPeakFireTime() const;
    };

    /**
     * Memory tracking for pooling efficiency
     */
    struct FPoolingMemoryTracker
    {
        SIZE_T InitialMemory;
        SIZE_T PeakMemory;
        SIZE_T FinalMemory;

        FPoolingMemoryTracker();
        void UpdatePeakMemory();
        SIZE_T GetMemoryDelta() const;
        SIZE_T GetPeakMemoryUsage() const;
    };

    /**
     * Pooling integration validator
     */
    class FPoolingIntegrationValidator
    {
    public:
        static bool ValidateWeaponPoolingComponent(UAdvancedWeaponSystem* WeaponSystem);
        static bool ValidatePoolManagerIntegration(UAdvancedObjectPoolManager* PoolManager);
        static bool ValidateEffectPooling(UAdvancedWeaponSystem* WeaponSystem);
        static bool ValidateProjectilePooling(UAdvancedWeaponSystem* WeaponSystem);
        static bool ValidateFallbackMechanisms(UAdvancedWeaponSystem* WeaponSystem);
    };

    /**
     * Combat scenario simulation for integration testing
     */
    class FCombatScenarioSimulator
    {
    public:
        static bool SimulateRapidFireScenario(UAdvancedWeaponSystem* WeaponSystem, int32 RoundsCount);
        static bool SimulateMultiWeaponScenario(TArray<UAdvancedWeaponSystem*> WeaponSystems);
        static bool SimulateBurstFireScenario(UAdvancedWeaponSystem* WeaponSystem, int32 BurstCount);
        static bool SimulateReloadAndFireScenario(UAdvancedWeaponSystem* WeaponSystem);
        static FWeaponPoolingMetrics MeasurePoolingPerformance(UAdvancedWeaponSystem* WeaponSystem, int32 TestDuration);
    };
}
