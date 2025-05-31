#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Components/Widget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/Overlay.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateColor.h"
#include "AdvancedHUDSystem.generated.h"

class AFPSCharacter;
class UFPSWeapon;
class UInventoryComponent;
class UDamageComponent;

UENUM(BlueprintType)
enum class EHUDElement : uint8
{
    Crosshair,
    HealthBar,
    AmmoCounter,
    WeaponInfo,
    Minimap,
    Compass,
    ObjectiveMarker,
    DamageIndicator,
    InteractionPrompt,
    InventorySlots,
    WeaponWheel,
    RadarBlips,
    StatusEffects,
    ScoreBoard,
    ChatWindow,
    KillFeed
};

UENUM(BlueprintType)
enum class EHUDStyle : uint8
{
    Minimal,
    Standard,
    Tactical,
    Immersive,
    Competitive
};

USTRUCT(BlueprintType)
struct FHUDElementData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVisible = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Size = FVector2D(100.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Opacity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 1.0f;
};

USTRUCT(BlueprintType)
struct FDamageIndicator
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FVector Direction = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    float Intensity = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float TimeRemaining = 2.0f;

    UPROPERTY(BlueprintReadWrite)
    FLinearColor Color = FLinearColor::Red;
};

USTRUCT(BlueprintType)
struct FKillFeedEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString KillerName;

    UPROPERTY(BlueprintReadWrite)
    FString VictimName;

    UPROPERTY(BlueprintReadWrite)
    FString WeaponName;

    UPROPERTY(BlueprintReadWrite)
    bool bHeadshot = false;

    UPROPERTY(BlueprintReadWrite)
    float DisplayTime = 5.0f;
};

UCLASS(BlueprintType)
class FPSGAME_API AAdvancedHUDSystem : public AHUD
{
    GENERATED_BODY()

public:
    AAdvancedHUDSystem();

protected:
    virtual void BeginPlay() override;
    virtual void DrawHUD() override;
    virtual void Tick(float DeltaTime) override;

    // Core HUD Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    EHUDStyle CurrentHUDStyle = EHUDStyle::Standard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    TMap<EHUDElement, FHUDElementData> HUDElements;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    bool bShowDebugInfo = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    float HUDScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    float HUDOpacity = 1.0f;

    // Crosshair Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    UTexture2D* CrosshairTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    FLinearColor CrosshairColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    float CrosshairSize = 32.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    float CrosshairSpread = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    bool bDynamicCrosshair = true;

    // Health and Status
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    UTexture2D* HealthBarTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    UTexture2D* HealthBarBackground;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    FLinearColor HealthColor = FLinearColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    FLinearColor LowHealthColor = FLinearColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float LowHealthThreshold = 0.25f;

    // Ammo and Weapon Info
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    UFont* WeaponFont;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FLinearColor AmmoColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FLinearColor LowAmmoColor = FLinearColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FLinearColor NoAmmoColor = FLinearColor::Red;

    // Minimap and Radar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    UTexture2D* MinimapBackground;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    UTexture2D* PlayerBlip;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    UTexture2D* EnemyBlip;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    UTexture2D* ObjectiveBlip;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float MinimapSize = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float MinimapRange = 1000.0f;

    // Damage Indicators
    UPROPERTY(BlueprintReadWrite, Category = "Damage")
    TArray<FDamageIndicator> DamageIndicators;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    UTexture2D* DamageIndicatorTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float DamageIndicatorSize = 64.0f;

    // Kill Feed
    UPROPERTY(BlueprintReadWrite, Category = "KillFeed")
    TArray<FKillFeedEntry> KillFeedEntries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
    int32 MaxKillFeedEntries = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
    UFont* KillFeedFont;

    // Animation and Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float HitMarkerDuration = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float DamageFlashDuration = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UCurveFloat* DamageFlashCurve;

    // Widget References
    UPROPERTY(BlueprintReadWrite, Category = "Widgets")
    TSubclassOf<UUserWidget> MainHUDWidgetClass;

    UPROPERTY(BlueprintReadWrite, Category = "Widgets")
    UUserWidget* MainHUDWidget;

    UPROPERTY(BlueprintReadWrite, Category = "Widgets")
    TSubclassOf<UUserWidget> InventoryWidgetClass;

    UPROPERTY(BlueprintReadWrite, Category = "Widgets")
    UUserWidget* InventoryWidget;

    UPROPERTY(BlueprintReadWrite, Category = "Widgets")
    TSubclassOf<UUserWidget> WeaponWheelWidgetClass;

    UPROPERTY(BlueprintReadWrite, Category = "Widgets")
    UUserWidget* WeaponWheelWidget;

public:
    // Public Interface
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetHUDStyle(EHUDStyle NewStyle);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetHUDElementVisibility(EHUDElement Element, bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetHUDElementData(EHUDElement Element, const FHUDElementData& Data);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateCrosshairSpread(float SpreadAmount);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowHitMarker(bool bHeadshot = false);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void AddDamageIndicator(const FVector& DamageDirection, float Intensity = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void AddKillFeedEntry(const FString& Killer, const FString& Victim, const FString& Weapon, bool bHeadshot = false);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowInteractionPrompt(const FString& InteractionText, const FString& InputKey);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void HideInteractionPrompt();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ToggleInventory();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowWeaponWheel();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void HideWeaponWheel();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateHealthDisplay(float CurrentHealth, float MaxHealth);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateAmmoDisplay(int32 CurrentAmmo, int32 MaxAmmo, int32 ReserveAmmo);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateWeaponInfo(const FString& WeaponName, const FString& FireMode);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void FlashDamageEffect();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateObjectiveMarker(const FVector& WorldLocation, const FString& ObjectiveText);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowStatusEffect(const FString& EffectName, float Duration, UTexture2D* EffectIcon);

protected:
    // Drawing Functions
    void DrawCrosshair();
    void DrawHealthBar();
    void DrawAmmoCounter();
    void DrawWeaponInfo();
    void DrawMinimap();
    void DrawCompass();
    void DrawDamageIndicators(float DeltaTime);
    void DrawKillFeed();
    void DrawHitMarker();
    void DrawObjectiveMarkers();
    void DrawDebugInfo();

    // Helper Functions
    FVector2D GetScreenCenter() const;
    FVector2D WorldToMinimap(const FVector& WorldLocation) const;
    float GetHealthPercentage() const;
    FLinearColor GetHealthColor() const;
    FLinearColor GetAmmoColor(int32 CurrentAmmo, int32 MaxAmmo) const;
    void UpdateDamageIndicators(float DeltaTime);
    void UpdateKillFeed(float DeltaTime);
    AFPSCharacter* GetPlayerCharacter() const;

    // Animation State
    float HitMarkerTimer = 0.0f;
    float DamageFlashTimer = 0.0f;
    bool bShowHitMarker = false;
    bool bShowInteractionPrompt = false;
    FString InteractionText;
    FString InteractionKey;
    bool bInventoryVisible = false;
    bool bWeaponWheelVisible = false;

    // Cached References
    UPROPERTY()
    AFPSCharacter* CachedPlayerCharacter;

    UPROPERTY()
    UInventoryComponent* CachedInventory;

    UPROPERTY()
    UDamageComponent* CachedDamageComponent;

    // Performance Optimization
    float LastHUDUpdateTime = 0.0f;
    float HUDUpdateInterval = 0.016f; // ~60 FPS
    
    // Enhanced Performance Optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    struct FHUDPerformanceSettings
    {
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool bEnablePerformanceOptimization = true;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float TargetFPS = 60.0f;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float MinUpdateInterval = 0.008f; // 120 FPS max
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float MaxUpdateInterval = 0.05f; // 20 FPS min
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool bEnableAdaptiveQuality = true;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool bEnableSmartCulling = true;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float LowPerformanceThreshold = 30.0f;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float HighPerformanceThreshold = 90.0f;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool bEnableMemoryOptimization = true;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float MaxMemoryUsageMB = 256.0f;
        
        FHUDPerformanceSettings()
        {
            bEnablePerformanceOptimization = true;
            TargetFPS = 60.0f;
            MinUpdateInterval = 0.008f;
            MaxUpdateInterval = 0.05f;
            bEnableAdaptiveQuality = true;
            bEnableSmartCulling = true;
            LowPerformanceThreshold = 30.0f;
            HighPerformanceThreshold = 90.0f;
            bEnableMemoryOptimization = true;
            MaxMemoryUsageMB = 256.0f;
        }
    } PerformanceSettings;
    
    // Performance tracking
    struct FHUDPerformanceMetrics
    {
        float CurrentFPS = 60.0f;
        float AverageFrameTime = 0.016f;
        float MemoryUsageMB = 0.0f;
        float LastOptimizationTime = 0.0f;
        bool bIsPerformanceLow = false;
        bool bIsMemoryHigh = false;
        float AdaptiveQualityLevel = 1.0f;
        int32 VisibleElements = 0;
        int32 CulledElements = 0;
        float OptimizationTimer = 0.0f;
        TArray<float> FrameTimeHistory;
        int32 FrameHistoryIndex = 0;
        static const int32 FrameHistorySize = 30;
        
        FHUDPerformanceMetrics()
        {
            FrameTimeHistory.SetNum(FrameHistorySize);
            for (int32 i = 0; i < FrameHistorySize; ++i)
            {
                FrameTimeHistory[i] = 0.016f; // Default to 60 FPS
            }
        }
    } CurrentPerformanceMetrics;
    
    // Smart update intervals for different HUD elements
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    struct FHUDElementUpdateSettings
    {
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TMap<EHUDElement, float> BaseUpdateIntervals;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TMap<EHUDElement, float> LowPerformanceIntervals;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TMap<EHUDElement, float> LastUpdateTimes;
        
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TMap<EHUDElement, int32> UpdatePriorities;
        
        FHUDElementUpdateSettings()
        {
            // Base intervals (normal performance)
            BaseUpdateIntervals.Add(EHUDElement::Crosshair, 0.016f);         // High priority - 60 FPS
            BaseUpdateIntervals.Add(EHUDElement::HealthBar, 0.033f);         // Medium priority - 30 FPS
            BaseUpdateIntervals.Add(EHUDElement::AmmoCounter, 0.033f);       // Medium priority - 30 FPS
            BaseUpdateIntervals.Add(EHUDElement::WeaponInfo, 0.1f);          // Low priority - 10 FPS
            BaseUpdateIntervals.Add(EHUDElement::Minimap, 0.1f);             // Low priority - 10 FPS
            BaseUpdateIntervals.Add(EHUDElement::Compass, 0.066f);           // Medium priority - 15 FPS
            BaseUpdateIntervals.Add(EHUDElement::DamageIndicator, 0.016f);   // High priority - 60 FPS
            BaseUpdateIntervals.Add(EHUDElement::KillFeed, 0.05f);           // Medium priority - 20 FPS
            BaseUpdateIntervals.Add(EHUDElement::InteractionPrompt, 0.033f); // Medium priority - 30 FPS
            BaseUpdateIntervals.Add(EHUDElement::ObjectiveMarker, 0.2f);     // Low priority - 5 FPS
            
            // Low performance intervals (reduced update rates)
            LowPerformanceIntervals.Add(EHUDElement::Crosshair, 0.033f);     // Reduced to 30 FPS
            LowPerformanceIntervals.Add(EHUDElement::HealthBar, 0.1f);       // Reduced to 10 FPS
            LowPerformanceIntervals.Add(EHUDElement::AmmoCounter, 0.1f);     // Reduced to 10 FPS
            LowPerformanceIntervals.Add(EHUDElement::WeaponInfo, 0.5f);      // Reduced to 2 FPS
            LowPerformanceIntervals.Add(EHUDElement::Minimap, 0.5f);         // Reduced to 2 FPS
            LowPerformanceIntervals.Add(EHUDElement::Compass, 0.2f);         // Reduced to 5 FPS
            LowPerformanceIntervals.Add(EHUDElement::DamageIndicator, 0.033f); // Reduced to 30 FPS
            LowPerformanceIntervals.Add(EHUDElement::KillFeed, 0.2f);        // Reduced to 5 FPS
            LowPerformanceIntervals.Add(EHUDElement::InteractionPrompt, 0.1f); // Reduced to 10 FPS
            LowPerformanceIntervals.Add(EHUDElement::ObjectiveMarker, 1.0f); // Reduced to 1 FPS
            
            // Update priorities (0 = highest, higher numbers = lower priority)
            UpdatePriorities.Add(EHUDElement::Crosshair, 0);
            UpdatePriorities.Add(EHUDElement::DamageIndicator, 1);
            UpdatePriorities.Add(EHUDElement::HealthBar, 2);
            UpdatePriorities.Add(EHUDElement::AmmoCounter, 3);
            UpdatePriorities.Add(EHUDElement::InteractionPrompt, 4);
            UpdatePriorities.Add(EHUDElement::Compass, 5);
            UpdatePriorities.Add(EHUDElement::KillFeed, 6);
            UpdatePriorities.Add(EHUDElement::WeaponInfo, 7);
            UpdatePriorities.Add(EHUDElement::Minimap, 8);
            UpdatePriorities.Add(EHUDElement::ObjectiveMarker, 9);
            
            // Initialize last update times
            for (const auto& Pair : BaseUpdateIntervals)
            {
                LastUpdateTimes.Add(Pair.Key, 0.0f);
            }
        }
    } ElementUpdateSettings;
    
    // Advanced performance optimization functions
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void UpdatePerformanceMetrics(float DeltaTime);
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeHUDPerformance();
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ApplyPerformanceOptimizations();
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    bool ShouldUpdateElement(EHUDElement Element, float CurrentTime);
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void UpdateAdaptiveQuality();
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void CullLowPriorityElements();
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeMemoryUsage();
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    float CalculatePerformanceScore() const;
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ApplyQualityScaling(float QualityLevel);
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ResetPerformanceOptimization();
    
    UFUNCTION(BlueprintCallable, Category = "Performance")
    FString GetPerformanceReport() const;
};
