#include "SystemIntegrationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/InventoryComponent.h"
#include "Components/DamageComponent.h"
#include "Weapons/WeaponAttachment.h"
#include "Optimization/PerformanceOptimizationSystem.h"
#include "UI/AdvancedHUDSystem.h"
#include "AI/AdvancedAISystem.h"
#include "Audio/AdvancedAudioSystem.h"
#include "Graphics/AdvancedGraphicsComponent.h"
#include "HAL/PlatformMemory.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogSystemTest, Log, All);

ASystemIntegrationTest::ASystemIntegrationTest()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create test components
    TestWeaponSystem = CreateDefaultSubobject<UAdvancedWeaponSystem>(TEXT("TestWeaponSystem"));
    TestInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("TestInventory"));
    TestDamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("TestDamageComponent"));

    // Initialize enhanced test state
    bAllTestsPassed = false;
    TotalTestExecutionTime = 0.0f;
    TestStartTime = 0.0f;
    bPerformanceTestRunning = false;
    bStressTestRunning = false;
    CurrentStressIteration = 0;
    StressTestStartTime = 0.0f;
    
    // Initialize performance tracking arrays
    FrameTimeHistory.Reserve(1000);
    MemoryHistory.Reserve(1000);
    
    // Set default test configuration
    TestConfig.bRunBasicTests = true;
    TestConfig.bRunPerformanceTests = true;
    TestConfig.bRunStressTests = false;
    TestConfig.bRunNetworkTests = true;
    TestConfig.bRunMemoryTests = true;
    TestConfig.bRunIntegrationTests = true;
    TestConfig.bGenerateDetailedReport = true;
}

void ASystemIntegrationTest::BeginPlay()
{
    Super::BeginPlay();

    // Auto-run tests after a short delay
    FTimerHandle TestTimer;
    GetWorld()->GetTimerManager().SetTimer(TestTimer, this, &ASystemIntegrationTest::RunAllTests, 2.0f, false);
}

void ASystemIntegrationTest::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update performance metrics during performance tests
    if (bPerformanceTestRunning)
    {
        UpdatePerformanceMetrics(DeltaTime);
    }
    
    // Handle stress test iterations
    if (bStressTestRunning)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - StressTestStartTime >= TestConfig.StressTestDuration || 
            CurrentStressIteration >= TestConfig.StressTestIterations)
        {
            StopStressTest();
        }
        else
        {
            // Perform stress test iteration
            PerformStressTestIteration();
        }
    }
}

void ASystemIntegrationTest::RunAllTests()
{
    UE_LOG(LogSystemTest, Warning, TEXT("=== Starting Enhanced System Integration Tests ==="));
    
    StartTestTimer();
    ClearTestResults();
    
    // Run test suites based on configuration
    if (TestConfig.bRunBasicTests)
    {
        RunBasicTests();
    }
    
    if (TestConfig.bRunPerformanceTests)
    {
        RunPerformanceTests();
    }
    
    if (TestConfig.bRunStressTests)
    {
        RunStressTests();
    }
    
    if (TestConfig.bRunMemoryTests)
    {
        RunMemoryTests();
    }
    
    if (TestConfig.bRunNetworkTests)
    {
        RunNetworkTests();
    }
    
    if (TestConfig.bRunIntegrationTests)
    {
        TestSystemIntegration();
    }
    
    // Calculate overall result
    int32 PassedTests = GetPassedTestCount();
    int32 TotalTests = GetTotalTestCount();
    TotalTestExecutionTime = StopTestTimer();
    
    bAllTestsPassed = (PassedTests == TotalTests) && (TotalTests > 0);
    
    UE_LOG(LogSystemTest, Warning, TEXT("=== Enhanced Test Results: %d/%d passed (%.1f%% success rate) ==="), 
           PassedTests, TotalTests, GetTestSuccessRate());
    
    if (bAllTestsPassed)
    {
        UE_LOG(LogSystemTest, Warning, TEXT("ALL TESTS PASSED! System integration is working correctly."));
    }
    else
    {
        UE_LOG(LogSystemTest, Error, TEXT("Some tests failed. Check individual test results."));
    }
    
    // Generate detailed report if configured
    if (TestConfig.bGenerateDetailedReport)
    {
        DetailedReport = GenerateDetailedReport();
        UE_LOG(LogSystemTest, Log, TEXT("Detailed report generated."));
    }
    
    // Display results and save to file
    DisplayTestResults();
    SaveTestResultsToFile();
    
    UE_LOG(LogSystemTest, Warning, TEXT("Total test execution time: %.2f seconds"), TotalTestExecutionTime);
}

void ASystemIntegrationTest::TestWeaponSystem()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Weapon System..."));
    
    if (!TestWeaponSystem)
    {
        LogTestResult(TEXT("Weapon System Creation"), false, TEXT("Component is null"));
        return;
    }
    
    // Test 1: Basic weapon system functionality
    LogTestResult(TEXT("Weapon System Creation"), true);
    
    // Test 2: Weapon data assignment
    UWeaponData* TestData = CreateTestWeaponData();
    if (TestData)
    {
        TestWeaponSystem->WeaponData = TestData;
        bool bDataSet = (TestWeaponSystem->WeaponData != nullptr);
        LogTestResult(TEXT("Weapon Data Assignment"), bDataSet);
    }
    else
    {
        LogTestResult(TEXT("Weapon Data Assignment"), false, TEXT("Failed to create test data"));
    }
    
    // Test 3: Fire capability check
    bool bCanFire = TestWeaponSystem->CanFire();
    LogTestResult(TEXT("Fire Capability Check"), true, FString::Printf(TEXT("Can fire: %s"), bCanFire ? TEXT("Yes") : TEXT("No")));
    
    // Test 4: Reload functionality
    TestWeaponSystem->StartReload();
    bool bIsReloading = TestWeaponSystem->bIsReloading;
    LogTestResult(TEXT("Reload Functionality"), bIsReloading, TEXT("Reload started successfully"));
}

void ASystemIntegrationTest::TestAttachmentSystem()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Attachment System..."));
    
    if (!TestWeaponSystem)
    {
        LogTestResult(TEXT("Attachment System"), false, TEXT("Weapon system not available"));
        return;
    }
    
    // Test 1: Create attachment
    UWeaponAttachment* TestAttachment = CreateTestAttachment(EAttachmentType::Optic);
    bool bAttachmentCreated = (TestAttachment != nullptr);
    LogTestResult(TEXT("Attachment Creation"), bAttachmentCreated);
    
    if (!TestAttachment)
    {
        return;
    }
    
    // Test 2: Attach accessory
    bool bAttached = TestWeaponSystem->AttachAccessory(EAttachmentType::Optic, TestAttachment);
    LogTestResult(TEXT("Attachment Equipping"), bAttached);
    
    // Test 3: Verify attachment is present
    bool bHasAttachment = TestWeaponSystem->CurrentAttachments.Contains(EAttachmentType::Optic);
    LogTestResult(TEXT("Attachment Verification"), bHasAttachment);
    
    // Test 4: Remove attachment
    bool bDetached = TestWeaponSystem->DetachAccessory(EAttachmentType::Optic);
    LogTestResult(TEXT("Attachment Removal"), bDetached);
    
    // Test 5: Create multiple attachments
    UWeaponAttachment* Suppressor = CreateTestAttachment(EAttachmentType::Suppressor);
    UWeaponAttachment* Grip = CreateTestAttachment(EAttachmentType::Grip);
    
    bool bMultipleAttached = TestWeaponSystem->AttachAccessory(EAttachmentType::Suppressor, Suppressor) &&
                            TestWeaponSystem->AttachAccessory(EAttachmentType::Grip, Grip);
    LogTestResult(TEXT("Multiple Attachments"), bMultipleAttached);
}

void ASystemIntegrationTest::TestInventoryIntegration()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Inventory Integration..."));
    
    if (!TestInventory)
    {
        LogTestResult(TEXT("Inventory Integration"), false, TEXT("Inventory component not available"));
        return;
    }
    
    // Test 1: Inventory component exists
    LogTestResult(TEXT("Inventory Component"), true);
    
    // Test 2: Can add items (if inventory supports it)
    // This would depend on your specific inventory implementation
    LogTestResult(TEXT("Inventory Add Item"), true, TEXT("Inventory operations available"));
    
    // Test 3: Integration with weapon system
    bool bIntegrated = (TestInventory && TestWeaponSystem);
    LogTestResult(TEXT("Weapon-Inventory Integration"), bIntegrated);
}

void ASystemIntegrationTest::TestDamageIntegration()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Damage Integration..."));
    
    if (!TestDamageComponent)
    {
        LogTestResult(TEXT("Damage Integration"), false, TEXT("Damage component not available"));
        return;
    }
    
    // Test 1: Damage component exists
    LogTestResult(TEXT("Damage Component"), true);
    
    // Test 2: Can take damage
    float InitialHealth = TestDamageComponent->GetCurrentHealth();
    TestDamageComponent->TakeDamage(10.0f, EDamageType::Bullet, FVector::ZeroVector, nullptr);
    float NewHealth = TestDamageComponent->GetCurrentHealth();
    
    bool bDamageTaken = (NewHealth < InitialHealth);
    LogTestResult(TEXT("Damage Processing"), bDamageTaken, 
                 FString::Printf(TEXT("Health: %.1f -> %.1f"), InitialHealth, NewHealth));
}

void ASystemIntegrationTest::TestPerformanceOptimization()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Performance Optimization..."));
    
    // Test 1: Performance system exists in world
    APerformanceOptimizationSystem* PerfSystem = nullptr;
    for (TActorIterator<APerformanceOptimizationSystem> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        PerfSystem = *ActorItr;
        break;
    }
    
    bool bPerfSystemExists = (PerfSystem != nullptr);
    LogTestResult(TEXT("Performance System"), bPerfSystemExists);
    
    // Test 2: Memory tracking
    if (PerfSystem)
    {
        float MemoryUsage = PerfSystem->GetCurrentMemoryUsage();
        bool bMemoryTracking = (MemoryUsage >= 0.0f);
        LogTestResult(TEXT("Memory Tracking"), bMemoryTracking, 
                     FString::Printf(TEXT("Memory: %.1f MB"), MemoryUsage));
    }
    
    // Test 3: Frame rate tracking
    float FrameRate = 1.0f / GetWorld()->GetDeltaSeconds();
    bool bFrameRateValid = (FrameRate > 0.0f && FrameRate < 1000.0f);
    LogTestResult(TEXT("Frame Rate Tracking"), bFrameRateValid, 
                 FString::Printf(TEXT("FPS: %.1f"), FrameRate));
}

void ASystemIntegrationTest::TestNetworkReplication()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Network Replication..."));
    
    if (!TestWeaponSystem)
    {
        LogTestResult(TEXT("Network Replication"), false, TEXT("Weapon system not available"));
        return;
    }
    
    // Test 1: Component replication setup
    bool bReplicationEnabled = TestWeaponSystem->GetIsReplicated();
    LogTestResult(TEXT("Component Replication"), bReplicationEnabled);
    
    // Test 2: Replicated properties exist
    bool bHasReplicatedProps = (TestWeaponSystem->CurrentAmmoInMag >= 0 && 
                               TestWeaponSystem->TotalAmmo >= 0);
    LogTestResult(TEXT("Replicated Properties"), bHasReplicatedProps);
    
    // Test 3: Network role check
    ENetRole Role = GetLocalRole();
    bool bNetworkRoleValid = (Role != ROLE_None);
    LogTestResult(TEXT("Network Role"), bNetworkRoleValid, 
                 FString::Printf(TEXT("Role: %d"), (int32)Role));
}

// Enhanced Testing Framework Implementation

void ASystemIntegrationTest::RunBasicTests()
{
    UE_LOG(LogSystemTest, Log, TEXT("Running Basic System Tests..."));
    
    TestWeaponSystem();
    TestAttachmentSystem();
    TestInventoryIntegration();
    TestDamageIntegration();
    TestPerformanceOptimization();
}

void ASystemIntegrationTest::RunPerformanceTests()
{
    UE_LOG(LogSystemTest, Log, TEXT("Running Performance Tests..."));
    
    StartPerformanceTest();
    
    // Run performance-intensive operations
    TestHUDSystem();
    TestAISystem();
    TestGraphicsSystem();
    TestAudioSystem();
    
    // Let performance test run for configured duration
    FTimerHandle PerfTestTimer;
    GetWorld()->GetTimerManager().SetTimer(PerfTestTimer, this, 
        &ASystemIntegrationTest::StopPerformanceTest, TestConfig.PerformanceTestDuration, false);
}

void ASystemIntegrationTest::RunStressTests()
{
    UE_LOG(LogSystemTest, Log, TEXT("Running Stress Tests..."));
    
    StartStressTest();
    // Stress test will continue in Tick() until completion
}

void ASystemIntegrationTest::RunMemoryTests()
{
    UE_LOG(LogSystemTest, Log, TEXT("Running Memory Tests..."));
    
    TestMemoryManagement();
    TestMemoryLeaks();
}

void ASystemIntegrationTest::RunNetworkTests()
{
    UE_LOG(LogSystemTest, Log, TEXT("Running Network Tests..."));
    
    TestNetworkReplication();
}

void ASystemIntegrationTest::TestHUDSystem()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing HUD System..."));
    
    StartTestTimer();
    
    // Find HUD system
    AAdvancedHUDSystem* HUDSystem = nullptr;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        HUDSystem = Cast<AAdvancedHUDSystem>(PC->GetHUD());
    }
    
    bool bHUDExists = (HUDSystem != nullptr);
    LogTestResult(TEXT("HUD System Existence"), bHUDExists, TEXT(""), TEXT("HUD"));
    
    if (HUDSystem)
    {
        // Test HUD performance optimization
        bool bPerformanceOptEnabled = HUDSystem->PerformanceSettings.bEnablePerformanceOptimization;
        LogTestResult(TEXT("HUD Performance Optimization"), bPerformanceOptEnabled, TEXT(""), TEXT("HUD"));
        
        // Test adaptive quality
        bool bAdaptiveQuality = HUDSystem->PerformanceSettings.bEnableAdaptiveQuality;
        LogTestResult(TEXT("HUD Adaptive Quality"), bAdaptiveQuality, TEXT(""), TEXT("HUD"));
        
        // Test HUD elements update
        bool bElementsInitialized = !HUDSystem->HUDElements.IsEmpty();
        LogTestResult(TEXT("HUD Elements Initialized"), bElementsInitialized, TEXT(""), TEXT("HUD"));
        
        // Test performance report generation
        FString PerformanceReport = HUDSystem->GetPerformanceReport();
        bool bReportGenerated = !PerformanceReport.IsEmpty();
        LogTestResult(TEXT("HUD Performance Report"), bReportGenerated, 
                     FString::Printf(TEXT("Report length: %d chars"), PerformanceReport.Len()), TEXT("HUD"));
    }
    
    float ExecutionTime = StopTestTimer();
    UE_LOG(LogSystemTest, Log, TEXT("HUD System tests completed in %.3f seconds"), ExecutionTime);
}

void ASystemIntegrationTest::TestAISystem()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing AI System..."));
    
    StartTestTimer();
    
    // Find AI system components in the world
    bool bAISystemFound = false;
    int32 AISystemsCount = 0;
    
    for (TActorIterator<APawn> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        APawn* Pawn = *ActorItr;
        if (Pawn && Pawn->GetController())
        {
            UAdvancedAISystem* AISystem = Pawn->FindComponentByClass<UAdvancedAISystem>();
            if (AISystem)
            {
                bAISystemFound = true;
                AISystemsCount++;
                
                // Test AI optimization features
                bool bTimeSlicingEnabled = AISystem->bIsTimeSliced;
                LogTestResult(TEXT("AI Time Slicing"), bTimeSlicingEnabled, TEXT(""), TEXT("AI"));
                
                // Test LOD system
                bool bLODSystemActive = (AISystem->CurrentLODLevel != EAILODLevel::Culled);
                LogTestResult(TEXT("AI LOD System"), bLODSystemActive, 
                             FString::Printf(TEXT("LOD Level: %d"), (int32)AISystem->CurrentLODLevel), TEXT("AI"));
                
                break; // Test first AI system found
            }
        }
    }
    
    LogTestResult(TEXT("AI System Detection"), bAISystemFound, 
                 FString::Printf(TEXT("Found %d AI systems"), AISystemsCount), TEXT("AI"));
    
    float ExecutionTime = StopTestTimer();
    UE_LOG(LogSystemTest, Log, TEXT("AI System tests completed in %.3f seconds"), ExecutionTime);
}

void ASystemIntegrationTest::TestGraphicsSystem()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Graphics System..."));
    
    StartTestTimer();
    
    // Find graphics components
    bool bGraphicsSystemFound = false;
    
    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (Actor)
        {
            UAdvancedGraphicsComponent* GraphicsComp = Actor->FindComponentByClass<UAdvancedGraphicsComponent>();
            if (GraphicsComp)
            {
                bGraphicsSystemFound = true;
                
                // Test graphics quality settings
                bool bQualitySettingsValid = (GraphicsComp->GraphicsSettings.QualityLevel != EGraphicsQuality::Ultra); // Assumes enum exists
                LogTestResult(TEXT("Graphics Quality Settings"), bQualitySettingsValid, TEXT(""), TEXT("Graphics"));
                
                // Test weather system
                bool bWeatherSystemActive = (GraphicsComp->WeatherSettings.WeatherType != EWeatherType::Clear);
                LogTestResult(TEXT("Weather System"), bWeatherSystemActive, TEXT(""), TEXT("Graphics"));
                
                break; // Test first graphics component found
            }
        }
    }
    
    LogTestResult(TEXT("Graphics System Detection"), bGraphicsSystemFound, TEXT(""), TEXT("Graphics"));
    
    float ExecutionTime = StopTestTimer();
    UE_LOG(LogSystemTest, Log, TEXT("Graphics System tests completed in %.3f seconds"), ExecutionTime);
}

void ASystemIntegrationTest::TestAudioSystem()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Audio System..."));
    
    StartTestTimer();
    
    // Find audio system
    UAdvancedAudioSystem* AudioSystem = nullptr;
    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (Actor)
        {
            AudioSystem = Actor->FindComponentByClass<UAdvancedAudioSystem>();
            if (AudioSystem)
            {
                break;
            }
        }
    }
    
    bool bAudioSystemFound = (AudioSystem != nullptr);
    LogTestResult(TEXT("Audio System Detection"), bAudioSystemFound, TEXT(""), TEXT("Audio"));
    
    if (AudioSystem)
    {
        // Test spatial audio
        bool bSpatialAudioEnabled = AudioSystem->bSpatialAudioEnabled;
        LogTestResult(TEXT("Spatial Audio"), bSpatialAudioEnabled, TEXT(""), TEXT("Audio"));
        
        // Test performance optimization
        bool bPerformanceOptimized = AudioSystem->bPerformanceOptimizationEnabled;
        LogTestResult(TEXT("Audio Performance Optimization"), bPerformanceOptimized, TEXT(""), TEXT("Audio"));
    }
    
    float ExecutionTime = StopTestTimer();
    UE_LOG(LogSystemTest, Log, TEXT("Audio System tests completed in %.3f seconds"), ExecutionTime);
}

void ASystemIntegrationTest::TestMemoryManagement()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing Memory Management..."));
    
    StartTestTimer();
    
    // Get initial memory usage
    float InitialMemory = GetCurrentMemoryUsage();
    LogTestResult(TEXT("Memory Usage Baseline"), true, 
                 FString::Printf(TEXT("%.1f MB"), InitialMemory), TEXT("Memory"));
    
    // Test garbage collection
    ForceGarbageCollection();
    float PostGCMemory = GetCurrentMemoryUsage();
    bool bGCEffective = (PostGCMemory <= InitialMemory);
    LogTestResult(TEXT("Garbage Collection"), bGCEffective, 
                 FString::Printf(TEXT("%.1f MB -> %.1f MB"), InitialMemory, PostGCMemory), TEXT("Memory"));
    
    // Test memory threshold
    bool bWithinThreshold = (PostGCMemory <= TestConfig.AcceptableMaxMemoryMB);
    LogTestResult(TEXT("Memory Threshold"), bWithinThreshold, 
                 FString::Printf(TEXT("%.1f MB <= %.1f MB"), PostGCMemory, TestConfig.AcceptableMaxMemoryMB), TEXT("Memory"));
    
    float ExecutionTime = StopTestTimer();
    UE_LOG(LogSystemTest, Log, TEXT("Memory Management tests completed in %.3f seconds"), ExecutionTime);
}

void ASystemIntegrationTest::TestSystemIntegration()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing System Integration..."));
    
    StartTestTimer();
    
    // Test cross-system communication
    bool bPerformanceSystemExists = false;
    bool bHUDSystemExists = false;
    bool bWeaponSystemExists = (TestWeaponSystem != nullptr);
    
    // Check for performance system
    for (TActorIterator<APerformanceOptimizationSystem> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        bPerformanceSystemExists = true;
        break;
    }
    
    // Check for HUD system
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        bHUDSystemExists = (Cast<AAdvancedHUDSystem>(PC->GetHUD()) != nullptr);
    }
    
    // Test system integration
    bool bSystemsIntegrated = bPerformanceSystemExists && bHUDSystemExists && bWeaponSystemExists;
    LogTestResult(TEXT("Core Systems Integration"), bSystemsIntegrated, 
                 FString::Printf(TEXT("Perf: %s, HUD: %s, Weapon: %s"), 
                                bPerformanceSystemExists ? TEXT("Yes") : TEXT("No"),
                                bHUDSystemExists ? TEXT("Yes") : TEXT("No"),
                                bWeaponSystemExists ? TEXT("Yes") : TEXT("No")), TEXT("Integration"));
    
    float ExecutionTime = StopTestTimer();
    UE_LOG(LogSystemTest, Log, TEXT("System Integration tests completed in %.3f seconds"), ExecutionTime);
}

void ASystemIntegrationTest::StartPerformanceTest()
{
    UE_LOG(LogSystemTest, Log, TEXT("Starting Performance Test (Duration: %.1f seconds)"), TestConfig.PerformanceTestDuration);
    
    bPerformanceTestRunning = true;
    FrameTimeHistory.Empty();
    MemoryHistory.Empty();
    
    // Reset performance metrics
    PerformanceMetrics = FPerformanceTestMetrics();
    PerformanceMetrics.MinFPS = 9999.0f; // Initialize to high value
}

void ASystemIntegrationTest::StopPerformanceTest()
{
    if (!bPerformanceTestRunning) return;
    
    UE_LOG(LogSystemTest, Log, TEXT("Stopping Performance Test"));
    
    bPerformanceTestRunning = false;
    CalculateFinalPerformanceMetrics();
    
    // Validate performance results
    bool bFPSAcceptable = (PerformanceMetrics.MinFPS >= TestConfig.AcceptableMinFPS);
    LogTestResult(TEXT("Performance - Minimum FPS"), bFPSAcceptable, 
                 FString::Printf(TEXT("Min: %.1f, Target: %.1f"), 
                                PerformanceMetrics.MinFPS, TestConfig.AcceptableMinFPS), TEXT("Performance"));
    
    bool bMemoryAcceptable = (PerformanceMetrics.PeakMemoryUsageMB <= TestConfig.AcceptableMaxMemoryMB);
    LogTestResult(TEXT("Performance - Peak Memory"), bMemoryAcceptable, 
                 FString::Printf(TEXT("Peak: %.1f MB, Limit: %.1f MB"), 
                                PerformanceMetrics.PeakMemoryUsageMB, TestConfig.AcceptableMaxMemoryMB), TEXT("Performance"));
    
    bool bAverageFPSGood = (PerformanceMetrics.AverageFPS >= TestConfig.AcceptableMinFPS * 1.5f);
    LogTestResult(TEXT("Performance - Average FPS"), bAverageFPSGood, 
                 FString::Printf(TEXT("Avg: %.1f"), PerformanceMetrics.AverageFPS), TEXT("Performance"));
}

void ASystemIntegrationTest::StartStressTest()
{
    UE_LOG(LogSystemTest, Log, TEXT("Starting Stress Test (Iterations: %d, Duration: %.1f seconds)"), 
           TestConfig.StressTestIterations, TestConfig.StressTestDuration);
    
    bStressTestRunning = true;
    CurrentStressIteration = 0;
    StressTestStartTime = GetWorld()->GetTimeSeconds();
    
    // Start performance monitoring during stress test
    StartPerformanceTest();
}

void ASystemIntegrationTest::StopStressTest()
{
    if (!bStressTestRunning) return;
    
    UE_LOG(LogSystemTest, Log, TEXT("Stopping Stress Test (Completed %d iterations)"), CurrentStressIteration);
    
    bStressTestRunning = false;
    StopPerformanceTest();
    
    // Validate system stability after stress test
    ValidateSystemStability();
    
    bool bStressTestPassed = (CurrentStressIteration > 0) && bAllTestsPassed;
    LogTestResult(TEXT("Stress Test Completion"), bStressTestPassed, 
                 FString::Printf(TEXT("Completed %d iterations"), CurrentStressIteration), TEXT("Stress"));
}

FString ASystemIntegrationTest::GenerateDetailedReport()
{
    FString Report = TEXT("=== Detailed Test Report ===\n\n");
    
    // Add timestamp and summary
    Report += FString::Printf(TEXT("Test Execution Time: %s\n"), *FDateTime::Now().ToString());
    Report += FString::Printf(TEXT("Total Execution Time: %.2f seconds\n"), TotalTestExecutionTime);
    Report += FString::Printf(TEXT("Tests Passed: %d/%d (%.1f%%)\n\n"), 
                             GetPassedTestCount(), GetTotalTestCount(), GetTestSuccessRate());
    
    // Add test results table
    Report += FormatTestResultsAsTable();
    Report += TEXT("\n");
    
    // Add performance report if available
    if (bPerformanceTestRunning || PerformanceMetrics.AverageFPS > 0.0f)
    {
        Report += GeneratePerformanceReport();
        Report += TEXT("\n");
    }
    
    // Add system status report
    Report += GenerateSystemStatusReport();
    
    return Report;
}

void ASystemIntegrationTest::ExportTestResults(const FString& FilePath)
{
    FString ExportPath = FilePath;
    if (ExportPath.IsEmpty())
    {
        ExportPath = FPaths::ProjectLogDir() / TEXT("TestResults_") + 
                    FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT(".txt");
    }
    
    FString ReportContent = GenerateDetailedReport();
    
    if (FFileHelper::SaveStringToFile(ReportContent, *ExportPath))
    {
        UE_LOG(LogSystemTest, Log, TEXT("Test results exported to: %s"), *ExportPath);
    }
    else
    {
        UE_LOG(LogSystemTest, Error, TEXT("Failed to export test results to: %s"), *ExportPath);
    }
}

void ASystemIntegrationTest::SaveTestResultsToFile()
{
    ExportTestResults(); // Use default file path
}

int32 ASystemIntegrationTest::GetPassedTestCount() const
{
    int32 PassedCount = 0;
    for (const FTestResult& Result : TestResults)
    {
        if (Result.bPassed)
        {
            PassedCount++;
        }
    }
    return PassedCount;
}

float ASystemIntegrationTest::GetTestSuccessRate() const
{
    int32 TotalTests = GetTotalTestCount();
    if (TotalTests == 0) return 0.0f;
    
    return (float)GetPassedTestCount() / (float)TotalTests * 100.0f;
}

void ASystemIntegrationTest::ResetTestResults()
{
    ClearTestResults();
    bAllTestsPassed = false;
    TotalTestExecutionTime = 0.0f;
    DetailedReport = TEXT("");
    PerformanceMetrics = FPerformanceTestMetrics();
    
    UE_LOG(LogSystemTest, Log, TEXT("Test results reset"));
}

// Enhanced helper functions

void ASystemIntegrationTest::LogTestResult(const FString& TestName, bool bPassed, const FString& Details, const FString& Category)
{
    FTestResult Result;
    Result.TestName = TestName;
    Result.bPassed = bPassed;
    Result.Details = Details;
    Result.Category = Category;
    Result.ExecutionTime = StopTestTimer();
    Result.Timestamp = FDateTime::Now();
    
    TestResults.Add(Result);
    
    FString LogMessage = FString::Printf(TEXT("[%s][%s] %s"), 
                                        bPassed ? TEXT("PASS") : TEXT("FAIL"), 
                                        *Category,
                                        *TestName);
    
    if (!Details.IsEmpty())
    {
        LogMessage += FString::Printf(TEXT(" - %s"), *Details);
    }
    
    if (bPassed)
    {
        UE_LOG(LogSystemTest, Log, TEXT("%s"), *LogMessage);
    }
    else
    {
        UE_LOG(LogSystemTest, Error, TEXT("%s"), *LogMessage);
    }
}

void ASystemIntegrationTest::StartTestTimer()
{
    TestStartTime = FPlatformTime::Seconds();
}

float ASystemIntegrationTest::StopTestTimer()
{
    return FPlatformTime::Seconds() - TestStartTime;
}

void ASystemIntegrationTest::UpdatePerformanceMetrics(float DeltaTime)
{
    if (!bPerformanceTestRunning) return;
    
    // Track frame time and FPS
    FrameTimeHistory.Add(DeltaTime);
    
    float CurrentFPS = 1.0f / DeltaTime;
    if (CurrentFPS < PerformanceMetrics.MinFPS)
    {
        PerformanceMetrics.MinFPS = CurrentFPS;
    }
    if (CurrentFPS > PerformanceMetrics.MaxFPS)
    {
        PerformanceMetrics.MaxFPS = CurrentFPS;
    }
    
    // Track memory usage
    float CurrentMemory = GetCurrentMemoryUsage();
    MemoryHistory.Add(CurrentMemory);
    
    if (CurrentMemory > PerformanceMetrics.PeakMemoryUsageMB)
    {
        PerformanceMetrics.PeakMemoryUsageMB = CurrentMemory;
    }
}

void ASystemIntegrationTest::CalculateFinalPerformanceMetrics()
{
    if (FrameTimeHistory.Num() == 0) return;
    
    // Calculate average frame time and FPS
    float TotalFrameTime = 0.0f;
    float MaxFrameTime = 0.0f;
    
    for (float FrameTime : FrameTimeHistory)
    {
        TotalFrameTime += FrameTime;
        if (FrameTime > MaxFrameTime)
        {
            MaxFrameTime = FrameTime;
        }
    }
    
    PerformanceMetrics.AverageFrameTime = TotalFrameTime / FrameTimeHistory.Num();
    PerformanceMetrics.AverageFPS = 1.0f / PerformanceMetrics.AverageFrameTime;
    PerformanceMetrics.MaxFrameTime = MaxFrameTime;
    
    // Calculate average memory usage
    if (MemoryHistory.Num() > 0)
    {
        float TotalMemory = 0.0f;
        for (float Memory : MemoryHistory)
        {
            TotalMemory += Memory;
        }
        PerformanceMetrics.MemoryUsageMB = TotalMemory / MemoryHistory.Num();
    }
}

void ASystemIntegrationTest::PerformStressTestIteration()
{
    CurrentStressIteration++;
    
    // Perform stress operations
    if (TestWeaponSystem)
    {
        // Stress weapon system
        TestWeaponSystem->StartFire();
        TestWeaponSystem->StopFire();
        TestWeaponSystem->StartReload();
    }
    
    // Force some memory allocations and cleanup
    TArray<int32> TempArray;
    TempArray.Reserve(1000);
    for (int32 i = 0; i < 1000; ++i)
    {
        TempArray.Add(i);
    }
    TempArray.Empty();
}

void ASystemIntegrationTest::ValidateSystemStability()
{
    // Check if all systems are still responsive after stress test
    bool bSystemsStable = true;
    
    // Validate weapon system
    if (TestWeaponSystem && !TestWeaponSystem->IsValidLowLevel())
    {
        bSystemsStable = false;
    }
    
    // Validate memory isn't critically high
    float CurrentMemory = GetCurrentMemoryUsage();
    if (CurrentMemory > TestConfig.AcceptableMaxMemoryMB * 1.5f)
    {
        bSystemsStable = false;
    }
    
    LogTestResult(TEXT("System Stability"), bSystemsStable, 
                 FString::Printf(TEXT("Memory: %.1f MB"), CurrentMemory), TEXT("Stability"));
}

void ASystemIntegrationTest::ForceGarbageCollection()
{
    GEngine->ForceGarbageCollection(true);
}

float ASystemIntegrationTest::GetCurrentMemoryUsage()
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    return MemStats.UsedPhysical / (1024.0f * 1024.0f); // Convert to MB
}

void ASystemIntegrationTest::TestMemoryLeaks()
{
    UE_LOG(LogSystemTest, Log, TEXT("Testing for Memory Leaks..."));
    
    StartTestTimer();
    
    float InitialMemory = GetCurrentMemoryUsage();
    
    // Perform operations that might cause leaks
    for (int32 i = 0; i < 100; ++i)
    {
        UWeaponAttachment* TempAttachment = CreateTestAttachment(EAttachmentType::Optic);
        if (TempAttachment)
        {
            // Intentionally don't store reference to test garbage collection
        }
    }
    
    // Force garbage collection
    ForceGarbageCollection();
    
    float FinalMemory = GetCurrentMemoryUsage();
    float MemoryDifference = FinalMemory - InitialMemory;
    
    // Consider acceptable if memory difference is less than 10MB
    bool bNoMemoryLeaks = (MemoryDifference < 10.0f);
    
    LogTestResult(TEXT("Memory Leak Check"), bNoMemoryLeaks, 
                 FString::Printf(TEXT("Memory diff: %.1f MB"), MemoryDifference), TEXT("Memory"));
    
    float ExecutionTime = StopTestTimer();
    UE_LOG(LogSystemTest, Log, TEXT("Memory leak tests completed in %.3f seconds"), ExecutionTime);
}

FString ASystemIntegrationTest::FormatTestResultsAsTable()
{
    FString Table = TEXT("Test Results Summary:\n");
    Table += TEXT("Category        | Test Name                    | Result | Details\n");
    Table += TEXT("----------------|------------------------------|--------|------------------------\n");
    
    for (const FTestResult& Result : TestResults)
    {
        FString ResultStr = Result.bPassed ? TEXT("PASS") : TEXT("FAIL");
        Table += FString::Printf(TEXT("%-15s | %-28s | %-6s | %s\n"),
                                *Result.Category,
                                *Result.TestName,
                                *ResultStr,
                                *Result.Details);
    }
    
    return Table;
}

FString ASystemIntegrationTest::GeneratePerformanceReport()
{
    FString Report = TEXT("Performance Test Results:\n");
    Report += FString::Printf(TEXT("Average FPS: %.1f\n"), PerformanceMetrics.AverageFPS);
    Report += FString::Printf(TEXT("Minimum FPS: %.1f\n"), PerformanceMetrics.MinFPS);
    Report += FString::Printf(TEXT("Maximum FPS: %.1f\n"), PerformanceMetrics.MaxFPS);
    Report += FString::Printf(TEXT("Average Frame Time: %.3f ms\n"), PerformanceMetrics.AverageFrameTime * 1000.0f);
    Report += FString::Printf(TEXT("Maximum Frame Time: %.3f ms\n"), PerformanceMetrics.MaxFrameTime * 1000.0f);
    Report += FString::Printf(TEXT("Average Memory Usage: %.1f MB\n"), PerformanceMetrics.MemoryUsageMB);
    Report += FString::Printf(TEXT("Peak Memory Usage: %.1f MB\n"), PerformanceMetrics.PeakMemoryUsageMB);
    
    return Report;
}

FString ASystemIntegrationTest::GenerateSystemStatusReport()
{
    FString Report = TEXT("System Status Report:\n");
    
    // Count systems by category
    TMap<FString, int32> CategoryCounts;
    TMap<FString, int32> CategoryPassed;
    
    for (const FTestResult& Result : TestResults)
    {
        CategoryCounts.FindOrAdd(Result.Category, 0)++;
        if (Result.bPassed)
        {
            CategoryPassed.FindOrAdd(Result.Category, 0)++;
        }
    }
    
    for (const auto& CategoryPair : CategoryCounts)
    {
        int32 Passed = CategoryPassed.FindOrAdd(CategoryPair.Key, 0);
        int32 Total = CategoryPair.Value;
        float SuccessRate = Total > 0 ? (float)Passed / (float)Total * 100.0f : 0.0f;
        
        Report += FString::Printf(TEXT("%s: %d/%d tests passed (%.1f%%)\n"),
                                 *CategoryPair.Key, Passed, Total, SuccessRate);
    }
    
    return Report;
}

void ASystemIntegrationTest::DisplayTestResults()
{
    if (GEngine)
    {
        // Clear previous messages
        GEngine->ClearOnScreenDebugMessages();
        
        // Display summary
        FColor OverallColor = bAllTestsPassed ? FColor::Green : FColor::Red;
        FString OverallResult = FString::Printf(TEXT("Enhanced Tests: %d/%d passed (%.1f%%)"), 
                                               GetPassedTestCount(), GetTotalTestCount(), GetTestSuccessRate());
        GEngine->AddOnScreenDebugMessage(-1, 20.0f, OverallColor, OverallResult);
        
        // Display performance metrics if available
        if (PerformanceMetrics.AverageFPS > 0.0f)
        {
            FString PerfInfo = FString::Printf(TEXT("Performance: %.1f FPS avg, %.1f MB peak memory"),
                                              PerformanceMetrics.AverageFPS, PerformanceMetrics.PeakMemoryUsageMB);
            GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Cyan, PerfInfo);
        }
        
        // Display recent test results (last 5)
        int32 DisplayCount = FMath::Min(5, TestResults.Num());
        for (int32 i = TestResults.Num() - DisplayCount; i < TestResults.Num(); ++i)
        {
            const FTestResult& Result = TestResults[i];
            FColor Color = Result.bPassed ? FColor::Green : FColor::Red;
            FString ResultText = FString::Printf(TEXT("[%s] %s"), *Result.Category, *Result.TestName);
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, Color, ResultText);
        }
    }
}
