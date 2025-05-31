#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/AdvancedWeaponSystem.h"
#include "Weapons/CommonWeaponAttachments.h"
#include "Engine/DataTable.h"
#include "SystemIntegrationTest.generated.h"

// Enhanced test result structure
USTRUCT(BlueprintType)
struct FTestResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString TestName;

    UPROPERTY(BlueprintReadOnly)
    bool bPassed;

    UPROPERTY(BlueprintReadOnly)
    FString Details;

    UPROPERTY(BlueprintReadOnly)
    float ExecutionTime;

    UPROPERTY(BlueprintReadOnly)
    FDateTime Timestamp;

    UPROPERTY(BlueprintReadOnly)
    FString Category;

    FTestResult()
    {
        TestName = TEXT("");
        bPassed = false;
        Details = TEXT("");
        ExecutionTime = 0.0f;
        Timestamp = FDateTime::Now();
        Category = TEXT("General");
    }
};

// Performance test metrics
USTRUCT(BlueprintType)
struct FPerformanceTestMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float AverageFPS;

    UPROPERTY(BlueprintReadOnly)
    float MinFPS;

    UPROPERTY(BlueprintReadOnly)
    float MaxFPS;

    UPROPERTY(BlueprintReadOnly)
    float AverageFrameTime;

    UPROPERTY(BlueprintReadOnly)
    float MaxFrameTime;

    UPROPERTY(BlueprintReadOnly)
    float MemoryUsageMB;

    UPROPERTY(BlueprintReadOnly)
    float PeakMemoryUsageMB;

    UPROPERTY(BlueprintReadOnly)
    int32 DrawCalls;

    UPROPERTY(BlueprintReadOnly)
    int32 TriangleCount;

    FPerformanceTestMetrics()
    {
        AverageFPS = 0.0f;
        MinFPS = 0.0f;
        MaxFPS = 0.0f;
        AverageFrameTime = 0.0f;
        MaxFrameTime = 0.0f;
        MemoryUsageMB = 0.0f;
        PeakMemoryUsageMB = 0.0f;
        DrawCalls = 0;
        TriangleCount = 0;
    }
};

// Test suite configuration
USTRUCT(BlueprintType)
struct FTestSuiteConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRunBasicTests = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRunPerformanceTests = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRunStressTests = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRunNetworkTests = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRunMemoryTests = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRunIntegrationTests = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bGenerateDetailedReport = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PerformanceTestDuration = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StressTestDuration = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StressTestIterations = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AcceptableMinFPS = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AcceptableMaxMemoryMB = 512.0f;
};

/**
 * Enhanced test actor to verify all FPS game systems work together correctly
 * Includes performance testing, stress testing, and automated reporting
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API ASystemIntegrationTest : public AActor
{
    GENERATED_BODY()

public:
    ASystemIntegrationTest();

protected:
    virtual void BeginPlay() override;

    // Test Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test Components")
    UAdvancedWeaponSystem* TestWeaponSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test Components")
    class UInventoryComponent* TestInventory;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test Components")
    class UDamageComponent* TestDamageComponent;

    // Enhanced Test Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Configuration")
    FTestSuiteConfig TestConfig;

    // Test Data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Data")
    UWeaponData* TestWeaponData;

    // Enhanced Test Results
    UPROPERTY(BlueprintReadOnly, Category = "Test Results")
    TArray<FTestResult> TestResults;

    UPROPERTY(BlueprintReadOnly, Category = "Test Results")
    FPerformanceTestMetrics PerformanceMetrics;

    UPROPERTY(BlueprintReadOnly, Category = "Test Results")
    bool bAllTestsPassed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Test Results")
    FString DetailedReport;

    UPROPERTY(BlueprintReadOnly, Category = "Test Results")
    float TotalTestExecutionTime = 0.0f;

    // Performance tracking for tests
    TArray<float> FrameTimeHistory;
    TArray<float> MemoryHistory;
    float TestStartTime = 0.0f;
    bool bPerformanceTestRunning = false;

    // Stress test state
    bool bStressTestRunning = false;
    int32 CurrentStressIteration = 0;
    float StressTestStartTime = 0.0f;

public:
    virtual void Tick(float DeltaTime) override;

    // Enhanced Test Functions
    UFUNCTION(BlueprintCallable, Category = "Testing")
    void RunAllTests();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void RunBasicTests();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void RunPerformanceTests();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void RunStressTests();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void RunMemoryTests();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void RunNetworkTests();

    // Individual test functions (enhanced)
    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestWeaponSystem();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestAttachmentSystem();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestInventoryIntegration();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestDamageIntegration();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestPerformanceOptimization();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestNetworkReplication();

    // New enhanced test functions
    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestHUDSystem();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestAISystem();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestGraphicsSystem();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestAudioSystem();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestMemoryManagement();

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void TestSystemIntegration();

    // Performance testing functions
    UFUNCTION(BlueprintCallable, Category = "Performance Testing")
    void StartPerformanceTest();

    UFUNCTION(BlueprintCallable, Category = "Performance Testing")
    void StopPerformanceTest();

    UFUNCTION(BlueprintCallable, Category = "Performance Testing")
    void StartStressTest();

    UFUNCTION(BlueprintCallable, Category = "Performance Testing")
    void StopStressTest();

    // Reporting functions
    UFUNCTION(BlueprintCallable, Category = "Reporting")
    FString GenerateDetailedReport();

    UFUNCTION(BlueprintCallable, Category = "Reporting")
    void ExportTestResults(const FString& FilePath = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Reporting")
    void SaveTestResultsToFile();

    // Utility functions
    UFUNCTION(BlueprintPure, Category = "Testing")
    TArray<FTestResult> GetTestResults() const { return TestResults; }

    UFUNCTION(BlueprintPure, Category = "Testing")
    FPerformanceTestMetrics GetPerformanceMetrics() const { return PerformanceMetrics; }

    UFUNCTION(BlueprintPure, Category = "Testing")
    bool AllTestsPassed() const { return bAllTestsPassed; }

    UFUNCTION(BlueprintPure, Category = "Testing")
    int32 GetPassedTestCount() const;

    UFUNCTION(BlueprintPure, Category = "Testing")
    int32 GetTotalTestCount() const { return TestResults.Num(); }

    UFUNCTION(BlueprintPure, Category = "Testing")
    float GetTestSuccessRate() const;

    UFUNCTION(BlueprintCallable, Category = "Testing")
    void ResetTestResults();

private:
    // Enhanced logging and reporting
    void LogTestResult(const FString& TestName, bool bPassed, const FString& Details = TEXT(""), const FString& Category = TEXT("General"));
    void StartTestTimer();
    float StopTestTimer();
    void UpdatePerformanceMetrics(float DeltaTime);
    void CalculateFinalPerformanceMetrics();
    
    // Test helper functions
    void ClearTestResults();
    UWeaponData* CreateTestWeaponData();
    UWeaponAttachment* CreateTestAttachment(EAttachmentType Type);
    
    // Stress test helpers
    void PerformStressTestIteration();
    void ValidateSystemStability();
    
    // Memory testing helpers
    void ForceGarbageCollection();
    float GetCurrentMemoryUsage();
    void TestMemoryLeaks();
    
    // Reporting helpers
    FString FormatTestResultsAsTable();
    FString GeneratePerformanceReport();
    FString GenerateSystemStatusReport();
    void DisplayTestResults();
};
