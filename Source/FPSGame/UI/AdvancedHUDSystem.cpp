#include "AdvancedHUDSystem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/Engine.h"

AAdvancedHUDSystem::AAdvancedHUDSystem()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Initialize default values
    CurrentHUDStyle = EHUDStyle::Tactical;
    CrosshairScale = 1.0f;
    CrosshairColor = FLinearColor::White;
    bShowMinimap = true;
    MinimapZoom = 1.0f;
    bHUDEnabled = true;
    
    // Initialize collections
    ActiveDamageIndicators.Empty();
    KillFeedEntries.Empty();
    ActiveWidgets.Empty();
    
    // Set default colors
    HealthBarColor = FLinearColor::Green;
    ArmorBarColor = FLinearColor::Blue;
    AmmoTextColor = FLinearColor::White;
    
    // Performance settings
    MaxDamageIndicators = 10;
    MaxKillFeedEntries = 5;
    DamageIndicatorLifetime = 3.0f;
    KillFeedEntryLifetime = 5.0f;
}

void AAdvancedHUDSystem::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeHUD();
    SetupEventBindings();
}

void AAdvancedHUDSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!bHUDEnabled) return;
    
    // Enhanced performance-aware updates
    UpdatePerformanceMetrics(DeltaTime);
    
    // Apply performance optimizations if needed
    if (PerformanceSettings.bEnablePerformanceOptimization)
    {
        OptimizeHUDPerformance();
    }
    
    // Adaptive update intervals based on performance
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Update elements based on their priority and current performance
    if (ShouldUpdateElement(EHUDElement::DamageIndicator, CurrentTime))
    {
        UpdateDamageIndicators(DeltaTime);
        ElementUpdateSettings.LastUpdateTimes[EHUDElement::DamageIndicator] = CurrentTime;
    }
    
    if (ShouldUpdateElement(EHUDElement::KillFeed, CurrentTime))
    {
        UpdateKillFeed(DeltaTime);
        ElementUpdateSettings.LastUpdateTimes[EHUDElement::KillFeed] = CurrentTime;
    }
    
    if (ShouldUpdateElement(EHUDElement::Minimap, CurrentTime))
    {
        UpdateMinimap(DeltaTime);
        ElementUpdateSettings.LastUpdateTimes[EHUDElement::Minimap] = CurrentTime;
    }
    
    // Update adaptive quality and memory optimization less frequently
    CurrentPerformanceMetrics.OptimizationTimer += DeltaTime;
    if (CurrentPerformanceMetrics.OptimizationTimer >= 1.0f) // Every second
    {
        UpdateAdaptiveQuality();
        if (PerformanceSettings.bEnableMemoryOptimization)
        {
            OptimizeMemoryUsage();
        }
        CurrentPerformanceMetrics.OptimizationTimer = 0.0f;
    }
}

void AAdvancedHUDSystem::DrawHUD()
{
    Super::DrawHUD();
    
    if (!bHUDEnabled) return;
    
    // Performance-aware rendering with smart culling
    float CurrentTime = GetWorld()->GetTimeSeconds();
    CurrentPerformanceMetrics.VisibleElements = 0;
    CurrentPerformanceMetrics.CulledElements = 0;
    
    // Apply quality scaling if adaptive quality is enabled
    float QualityScale = PerformanceSettings.bEnableAdaptiveQuality ? 
        CurrentPerformanceMetrics.AdaptiveQualityLevel : 1.0f;
    
    // Draw high-priority elements always
    if (ShouldUpdateElement(EHUDElement::Crosshair, CurrentTime))
    {
        DrawCrosshair();
        CurrentPerformanceMetrics.VisibleElements++;
    }
    else
    {
        CurrentPerformanceMetrics.CulledElements++;
    }
    
    if (ShouldUpdateElement(EHUDElement::HealthBar, CurrentTime))
    {
        DrawHealthAndArmor();
        CurrentPerformanceMetrics.VisibleElements++;
    }
    else
    {
        CurrentPerformanceMetrics.CulledElements++;
    }
    
    if (ShouldUpdateElement(EHUDElement::AmmoCounter, CurrentTime))
    {
        DrawAmmoCounter();
        CurrentPerformanceMetrics.VisibleElements++;
    }
    else
    {
        CurrentPerformanceMetrics.CulledElements++;
    }
    
    // Draw medium-priority elements based on performance
    if (!CurrentPerformanceMetrics.bIsPerformanceLow)
    {
        if (ShouldUpdateElement(EHUDElement::DamageIndicator, CurrentTime))
        {
            DrawDamageIndicators();
            CurrentPerformanceMetrics.VisibleElements++;
        }
        
        if (ShouldUpdateElement(EHUDElement::KillFeed, CurrentTime))
        {
            DrawKillFeed();
            CurrentPerformanceMetrics.VisibleElements++;
        }
        
        if (ShouldUpdateElement(EHUDElement::Compass, CurrentTime))
        {
            DrawCompass();
            CurrentPerformanceMetrics.VisibleElements++;
        }
    }
    else
    {
        CurrentPerformanceMetrics.CulledElements += 3; // Count culled medium-priority elements
    }
    
    // Draw low-priority elements only in high performance mode
    if (CurrentPerformanceMetrics.CurrentFPS > PerformanceSettings.HighPerformanceThreshold)
    {
        if (ShouldUpdateElement(EHUDElement::Minimap, CurrentTime))
        {
            DrawMinimap();
            CurrentPerformanceMetrics.VisibleElements++;
        }
    }
    else
    {
        CurrentPerformanceMetrics.CulledElements++;
    }
    
    // Debug info (only if enabled and performance allows)
    if (bShowDebugInfo && !CurrentPerformanceMetrics.bIsPerformanceLow)
    {
        DrawDebugInfo();
        CurrentPerformanceMetrics.VisibleElements++;
    }
}

void AAdvancedHUDSystem::InitializeHUD()
{
    // Load HUD assets based on style
    LoadHUDAssets();
    
    // Create main HUD widget
    if (MainHUDWidgetClass)
    {
        MainHUDWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(), MainHUDWidgetClass);
        if (MainHUDWidget)
        {
            MainHUDWidget->AddToViewport();
            RegisterWidget("MainHUD", MainHUDWidget);
        }
    }
    
    // Initialize minimap
    InitializeMinimap();
    
    // Setup crosshair
    SetupCrosshair();
}

void AAdvancedHUDSystem::LoadHUDAssets()
{
    // Load style-specific assets
    switch (CurrentHUDStyle)
    {
        case EHUDStyle::Minimal:
            LoadMinimalAssets();
            break;
        case EHUDStyle::Tactical:
            LoadTacticalAssets();
            break;
        case EHUDStyle::Classic:
            LoadClassicAssets();
            break;
        case EHUDStyle::Futuristic:
            LoadFuturisticAssets();
            break;
    }
}

void AAdvancedHUDSystem::LoadMinimalAssets()
{
    // Load minimal HUD textures and fonts
    CrosshairTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Crosshairs/MinimalCrosshair"));
    HUDFont = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/MinimalFont"));
}

void AAdvancedHUDSystem::LoadTacticalAssets()
{
    // Load tactical HUD textures and fonts
    CrosshairTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Crosshairs/TacticalCrosshair"));
    HUDFont = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/TacticalFont"));
    CompassTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Compass/TacticalCompass"));
}

void AAdvancedHUDSystem::LoadClassicAssets()
{
    // Load classic FPS HUD textures and fonts
    CrosshairTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Crosshairs/ClassicCrosshair"));
    HUDFont = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/ClassicFont"));
}

void AAdvancedHUDSystem::LoadFuturisticAssets()
{
    // Load futuristic HUD textures and fonts
    CrosshairTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Crosshairs/FuturisticCrosshair"));
    HUDFont = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/FuturisticFont"));
    HUDOverlayTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Overlays/FuturisticOverlay"));
}

void AAdvancedHUDSystem::SetupEventBindings()
{
    // Bind to player events
    if (APlayerController* PC = GetOwningPlayerController())
    {
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            // Bind to damage events, weapon changes, etc.
            // These would typically be bound through delegates in a real implementation
        }
    }
}

void AAdvancedHUDSystem::DrawCrosshair()
{
    if (!CrosshairTexture) return;
    
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    FVector2D CrosshairSize = FVector2D(32.0f, 32.0f) * CrosshairScale;
    FVector2D CrosshairPos = (ViewportSize - CrosshairSize) * 0.5f;
    
    // Apply weapon spread to crosshair
    if (GetOwningPlayerController() && GetOwningPlayerController()->GetPawn())
    {
        // Get weapon spread and apply to crosshair
        float Spread = GetCurrentWeaponSpread();
        CrosshairSize *= (1.0f + Spread * 0.5f);
    }
    
    DrawTexture(CrosshairTexture, CrosshairPos.X, CrosshairPos.Y, 
                CrosshairSize.X, CrosshairSize.Y, 0.0f, 0.0f, 1.0f, 1.0f, CrosshairColor);
}

void AAdvancedHUDSystem::DrawHealthAndArmor()
{
    if (!GetOwningPlayerController() || !GetOwningPlayerController()->GetPawn()) return;
    
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    // Get player health and armor values
    float Health = GetPlayerHealth();
    float MaxHealth = GetPlayerMaxHealth();
    float Armor = GetPlayerArmor();
    float MaxArmor = GetPlayerMaxArmor();
    
    // Draw health bar
    FVector2D HealthBarPos = FVector2D(50.0f, ViewportSize.Y - 100.0f);
    FVector2D HealthBarSize = FVector2D(200.0f, 20.0f);
    DrawHealthBar(HealthBarPos, HealthBarSize, Health, MaxHealth);
    
    // Draw armor bar
    FVector2D ArmorBarPos = FVector2D(50.0f, ViewportSize.Y - 70.0f);
    FVector2D ArmorBarSize = FVector2D(200.0f, 15.0f);
    DrawArmorBar(ArmorBarPos, ArmorBarSize, Armor, MaxArmor);
}

void AAdvancedHUDSystem::DrawHealthBar(const FVector2D& Position, const FVector2D& Size, float Health, float MaxHealth)
{
    // Background
    DrawRect(FLinearColor::Black, Position.X, Position.Y, Size.X, Size.Y);
    
    // Health fill
    float HealthPercent = FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
    FLinearColor HealthColor = FMath::Lerp(FLinearColor::Red, FLinearColor::Green, HealthPercent);
    DrawRect(HealthColor, Position.X + 2.0f, Position.Y + 2.0f, 
             (Size.X - 4.0f) * HealthPercent, Size.Y - 4.0f);
    
    // Health text
    FString HealthText = FString::Printf(TEXT("%.0f/%.0f"), Health, MaxHealth);
    DrawText(HealthText, FLinearColor::White, Position.X + Size.X + 10.0f, Position.Y, HUDFont, 1.0f);
}

void AAdvancedHUDSystem::DrawArmorBar(const FVector2D& Position, const FVector2D& Size, float Armor, float MaxArmor)
{
    if (MaxArmor <= 0.0f) return;
    
    // Background
    DrawRect(FLinearColor::Black, Position.X, Position.Y, Size.X, Size.Y);
    
    // Armor fill
    float ArmorPercent = FMath::Clamp(Armor / MaxArmor, 0.0f, 1.0f);
    DrawRect(ArmorBarColor, Position.X + 2.0f, Position.Y + 2.0f, 
             (Size.X - 4.0f) * ArmorPercent, Size.Y - 4.0f);
    
    // Armor text
    FString ArmorText = FString::Printf(TEXT("%.0f/%.0f"), Armor, MaxArmor);
    DrawText(ArmorText, FLinearColor::White, Position.X + Size.X + 10.0f, Position.Y, HUDFont, 1.0f);
}

void AAdvancedHUDSystem::DrawAmmoCounter()
{
    if (!GetOwningPlayerController() || !GetOwningPlayerController()->GetPawn()) return;
    
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    // Get ammo counts
    int32 CurrentAmmo = GetCurrentAmmo();
    int32 MaxAmmo = GetMaxAmmo();
    int32 ReserveAmmo = GetReserveAmmo();
    
    // Draw ammo counter
    FString AmmoText = FString::Printf(TEXT("%d / %d"), CurrentAmmo, ReserveAmmo);
    FVector2D AmmoPos = FVector2D(ViewportSize.X - 200.0f, ViewportSize.Y - 100.0f);
    
    DrawText(AmmoText, AmmoTextColor, AmmoPos.X, AmmoPos.Y, HUDFont, 1.5f);
    
    // Draw reload indicator if needed
    if (IsReloading())
    {
        FString ReloadText = TEXT("RELOADING...");
        DrawText(ReloadText, FLinearColor::Yellow, AmmoPos.X, AmmoPos.Y - 30.0f, HUDFont, 1.0f);
    }
}

void AAdvancedHUDSystem::DrawDamageIndicators()
{
    for (const FDamageIndicator& Indicator : ActiveDamageIndicators)
    {
        float Alpha = FMath::Clamp(1.0f - (Indicator.ElapsedTime / DamageIndicatorLifetime), 0.0f, 1.0f);
        FLinearColor IndicatorColor = FLinearColor::Red;
        IndicatorColor.A = Alpha;
        
        // Calculate screen position from world direction
        FVector2D ScreenPos = CalculateDamageIndicatorScreenPos(Indicator.Direction);
        
        // Draw damage indicator
        DrawDamageIndicatorAt(ScreenPos, IndicatorColor, Indicator.Damage);
    }
}

void AAdvancedHUDSystem::DrawDamageIndicatorAt(const FVector2D& ScreenPos, const FLinearColor& Color, float Damage)
{
    // Draw damage arrow/indicator
    FVector2D IndicatorSize = FVector2D(40.0f, 40.0f);
    DrawRect(Color, ScreenPos.X - IndicatorSize.X * 0.5f, ScreenPos.Y - IndicatorSize.Y * 0.5f, 
             IndicatorSize.X, IndicatorSize.Y);
    
    // Draw damage amount
    FString DamageText = FString::Printf(TEXT("%.0f"), Damage);
    DrawText(DamageText, Color, ScreenPos.X + 25.0f, ScreenPos.Y - 10.0f, HUDFont, 0.8f);
}

FVector2D AAdvancedHUDSystem::CalculateDamageIndicatorScreenPos(const FVector& WorldDirection)
{
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    // Convert world direction to screen space
    FVector2D ScreenCenter = ViewportSize * 0.5f;
    FVector2D DirectionScreen = FVector2D(WorldDirection.Y, -WorldDirection.X) * 100.0f;
    
    return ScreenCenter + DirectionScreen;
}

void AAdvancedHUDSystem::DrawKillFeed()
{
    if (KillFeedEntries.Num() == 0) return;
    
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    FVector2D KillFeedPos = FVector2D(ViewportSize.X - 400.0f, 50.0f);
    
    for (int32 i = 0; i < KillFeedEntries.Num(); ++i)
    {
        const FKillFeedEntry& Entry = KillFeedEntries[i];
        float Alpha = FMath::Clamp(1.0f - (Entry.ElapsedTime / KillFeedEntryLifetime), 0.0f, 1.0f);
        
        FLinearColor TextColor = FLinearColor::White;
        TextColor.A = Alpha;
        
        FString KillText = FString::Printf(TEXT("%s killed %s"), *Entry.KillerName, *Entry.VictimName);
        DrawText(KillText, TextColor, KillFeedPos.X, KillFeedPos.Y + (i * 25.0f), HUDFont, 1.0f);
    }
}

void AAdvancedHUDSystem::DrawMinimap()
{
    if (!bShowMinimap) return;
    
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    FVector2D MinimapPos = FVector2D(ViewportSize.X - 250.0f, 50.0f);
    FVector2D MinimapSize = FVector2D(200.0f, 200.0f);
    
    // Draw minimap background
    DrawRect(FLinearColor::Black, MinimapPos.X, MinimapPos.Y, MinimapSize.X, MinimapSize.Y);
    DrawRect(FLinearColor::Gray, MinimapPos.X + 2.0f, MinimapPos.Y + 2.0f, 
             MinimapSize.X - 4.0f, MinimapSize.Y - 4.0f);
    
    // Draw player position and rotation
    DrawMinimapPlayer(MinimapPos, MinimapSize);
    
    // Draw other players/enemies
    DrawMinimapEntities(MinimapPos, MinimapSize);
}

void AAdvancedHUDSystem::DrawMinimapPlayer(const FVector2D& MinimapPos, const FVector2D& MinimapSize)
{
    FVector2D PlayerPos = MinimapPos + (MinimapSize * 0.5f);
    DrawRect(FLinearColor::Blue, PlayerPos.X - 3.0f, PlayerPos.Y - 3.0f, 6.0f, 6.0f);
    
    // Draw player direction indicator
    if (APlayerController* PC = GetOwningPlayerController())
    {
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            FRotator PlayerRotation = PlayerPawn->GetActorRotation();
            FVector2D DirectionIndicator = FVector2D(
                FMath::Sin(FMath::DegreesToRadians(PlayerRotation.Yaw)),
                -FMath::Cos(FMath::DegreesToRadians(PlayerRotation.Yaw))
            ) * 15.0f;
            
            FVector2D EndPos = PlayerPos + DirectionIndicator;
            // Draw line would require custom implementation
        }
    }
}

void AAdvancedHUDSystem::DrawMinimapEntities(const FVector2D& MinimapPos, const FVector2D& MinimapSize)
{
    // Draw other players, enemies, objectives, etc.
    // This would iterate through relevant actors in the world
    for (const FMinimapEntity& Entity : MinimapEntities)
    {
        FVector2D EntityScreenPos = WorldToMinimapPos(Entity.WorldPosition, MinimapPos, MinimapSize);
        DrawMinimapEntity(EntityScreenPos, Entity);
    }
}

FVector2D AAdvancedHUDSystem::WorldToMinimapPos(const FVector& WorldPos, const FVector2D& MinimapPos, const FVector2D& MinimapSize)
{
    if (!GetOwningPlayerController() || !GetOwningPlayerController()->GetPawn()) 
        return MinimapPos + (MinimapSize * 0.5f);
    
    FVector PlayerPos = GetOwningPlayerController()->GetPawn()->GetActorLocation();
    FVector RelativePos = WorldPos - PlayerPos;
    
    // Scale and convert to minimap coordinates
    FVector2D MinimapRelativePos = FVector2D(RelativePos.Y, -RelativePos.X) / (5000.0f / MinimapZoom);
    return MinimapPos + (MinimapSize * 0.5f) + MinimapRelativePos;
}

void AAdvancedHUDSystem::DrawMinimapEntity(const FVector2D& ScreenPos, const FMinimapEntity& Entity)
{
    FLinearColor EntityColor = FLinearColor::Red;
    
    switch (Entity.Type)
    {
        case EMinimapEntityType::Player:
            EntityColor = FLinearColor::Blue;
            break;
        case EMinimapEntityType::Enemy:
            EntityColor = FLinearColor::Red;
            break;
        case EMinimapEntityType::Objective:
            EntityColor = FLinearColor::Yellow;
            break;
        case EMinimapEntityType::PointOfInterest:
            EntityColor = FLinearColor::Green;
            break;
    }
    
    DrawRect(EntityColor, ScreenPos.X - 2.0f, ScreenPos.Y - 2.0f, 4.0f, 4.0f);
}

void AAdvancedHUDSystem::DrawCompass()
{
    if (!GetOwningPlayerController() || !GetOwningPlayerController()->GetPawn()) return;
    
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    FVector2D CompassPos = FVector2D(ViewportSize.X * 0.5f - 100.0f, 50.0f);
    FVector2D CompassSize = FVector2D(200.0f, 30.0f);
    
    // Draw compass background
    DrawRect(FLinearColor::Black, CompassPos.X, CompassPos.Y, CompassSize.X, CompassSize.Y);
    
    // Get player rotation
    FRotator PlayerRotation = GetOwningPlayerController()->GetPawn()->GetActorRotation();
    float Yaw = PlayerRotation.Yaw;
    
    // Draw compass markings
    DrawCompassMarkings(CompassPos, CompassSize, Yaw);
}

void AAdvancedHUDSystem::DrawCompassMarkings(const FVector2D& CompassPos, const FVector2D& CompassSize, float PlayerYaw)
{
    const TArray<FString> Directions = {TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"), 
                                       TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW")};
    
    for (int32 i = 0; i < 8; ++i)
    {
        float DirectionYaw = i * 45.0f;
        float RelativeYaw = DirectionYaw - PlayerYaw;
        
        // Normalize to -180 to 180
        while (RelativeYaw > 180.0f) RelativeYaw -= 360.0f;
        while (RelativeYaw < -180.0f) RelativeYaw += 360.0f;
        
        // Check if this direction is visible on compass
        if (FMath::Abs(RelativeYaw) <= 90.0f)
        {
            float CompassX = CompassPos.X + CompassSize.X * 0.5f + (RelativeYaw / 90.0f) * (CompassSize.X * 0.5f);
            DrawText(Directions[i], FLinearColor::White, CompassX - 5.0f, CompassPos.Y + 5.0f, HUDFont, 0.8f);
        }
    }
}

void AAdvancedHUDSystem::DrawDebugInfo()
{
    FVector2D ViewportSize;
    GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
    
    FVector2D DebugPos = FVector2D(10.0f, 10.0f);
    
    // FPS Counter
    float FPS = 1.0f / GetWorld()->GetDeltaSeconds();
    FString FPSText = FString::Printf(TEXT("FPS: %.1f"), FPS);
    DrawText(FPSText, FLinearColor::Yellow, DebugPos.X, DebugPos.Y, HUDFont, 1.0f);
    
    // Memory usage
    FString MemoryText = FString::Printf(TEXT("Memory: %.1f MB"), GetUsedMemoryMB());
    DrawText(MemoryText, FLinearColor::Yellow, DebugPos.X, DebugPos.Y + 20.0f, HUDFont, 1.0f);
    
    // Active widgets count
    FString WidgetText = FString::Printf(TEXT("Active Widgets: %d"), ActiveWidgets.Num());
    DrawText(WidgetText, FLinearColor::Yellow, DebugPos.X, DebugPos.Y + 40.0f, HUDFont, 1.0f);
}

// Update functions
void AAdvancedHUDSystem::UpdateDamageIndicators(float DeltaTime)
{
    for (int32 i = ActiveDamageIndicators.Num() - 1; i >= 0; --i)
    {
        ActiveDamageIndicators[i].ElapsedTime += DeltaTime;
        
        if (ActiveDamageIndicators[i].ElapsedTime >= DamageIndicatorLifetime)
        {
            ActiveDamageIndicators.RemoveAt(i);
        }
    }
}

void AAdvancedHUDSystem::UpdateKillFeed(float DeltaTime)
{
    for (int32 i = KillFeedEntries.Num() - 1; i >= 0; --i)
    {
        KillFeedEntries[i].ElapsedTime += DeltaTime;
        
        if (KillFeedEntries[i].ElapsedTime >= KillFeedEntryLifetime)
        {
            KillFeedEntries.RemoveAt(i);
        }
    }
}

void AAdvancedHUDSystem::UpdateMinimap(float DeltaTime)
{
    // Update minimap entities
    MinimapEntities.Empty();
    
    // Add relevant actors to minimap
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<APawn> ActorItr(World); ActorItr; ++ActorItr)
        {
            APawn* Pawn = *ActorItr;
            if (Pawn && Pawn != GetOwningPlayerController()->GetPawn())
            {
                FMinimapEntity Entity;
                Entity.WorldPosition = Pawn->GetActorLocation();
                Entity.Type = EMinimapEntityType::Player; // Would determine based on team, etc.
                MinimapEntities.Add(Entity);
            }
        }
    }
}

void AAdvancedHUDSystem::UpdatePerformanceMetrics(float DeltaTime)
{
    // Update frame time history
    CurrentPerformanceMetrics.FrameTimeHistory[CurrentPerformanceMetrics.FrameHistoryIndex] = DeltaTime;
    CurrentPerformanceMetrics.FrameHistoryIndex = 
        (CurrentPerformanceMetrics.FrameHistoryIndex + 1) % CurrentPerformanceMetrics.FrameHistorySize;
    
    // Calculate average frame time
    float TotalFrameTime = 0.0f;
    for (float FrameTime : CurrentPerformanceMetrics.FrameTimeHistory)
    {
        TotalFrameTime += FrameTime;
    }
    CurrentPerformanceMetrics.AverageFrameTime = TotalFrameTime / CurrentPerformanceMetrics.FrameHistorySize;
    CurrentPerformanceMetrics.CurrentFPS = 1.0f / CurrentPerformanceMetrics.AverageFrameTime;
    
    // Update memory usage
    CurrentPerformanceMetrics.MemoryUsageMB = GetUsedMemoryMB();
    
    // Determine performance state
    CurrentPerformanceMetrics.bIsPerformanceLow = 
        CurrentPerformanceMetrics.CurrentFPS < PerformanceSettings.LowPerformanceThreshold;
    CurrentPerformanceMetrics.bIsMemoryHigh = 
        CurrentPerformanceMetrics.MemoryUsageMB > PerformanceSettings.MaxMemoryUsageMB;
}

void AAdvancedHUDSystem::OptimizeHUDPerformance()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Apply optimizations based on current performance
    if (CurrentPerformanceMetrics.bIsPerformanceLow)
    {
        ApplyPerformanceOptimizations();
    }
    
    // Apply memory optimizations if needed
    if (CurrentPerformanceMetrics.bIsMemoryHigh)
    {
        OptimizeMemoryUsage();
    }
    
    // Update last optimization time
    CurrentPerformanceMetrics.LastOptimizationTime = CurrentTime;
}

void AAdvancedHUDSystem::ApplyPerformanceOptimizations()
{
    // Reduce update intervals for low-priority elements
    if (PerformanceSettings.bEnableSmartCulling)
    {
        CullLowPriorityElements();
    }
    
    // Apply adaptive quality scaling
    if (PerformanceSettings.bEnableAdaptiveQuality)
    {
        float TargetQuality = FMath::Clamp(
            CurrentPerformanceMetrics.CurrentFPS / PerformanceSettings.TargetFPS,
            0.3f, 1.0f
        );
        ApplyQualityScaling(TargetQuality);
    }
    
    // Adjust global HUD update interval
    float PerformanceRatio = CurrentPerformanceMetrics.CurrentFPS / PerformanceSettings.TargetFPS;
    HUDUpdateInterval = FMath::Clamp(
        PerformanceSettings.MinUpdateInterval / PerformanceRatio,
        PerformanceSettings.MinUpdateInterval,
        PerformanceSettings.MaxUpdateInterval
    );
}

bool AAdvancedHUDSystem::ShouldUpdateElement(EHUDElement Element, float CurrentTime)
{
    // Check if element exists in update settings
    if (!ElementUpdateSettings.LastUpdateTimes.Contains(Element))
    {
        return true; // Update if not tracked
    }
    
    float LastUpdateTime = ElementUpdateSettings.LastUpdateTimes[Element];
    float UpdateInterval;
    
    // Use appropriate update interval based on performance state
    if (CurrentPerformanceMetrics.bIsPerformanceLow && 
        ElementUpdateSettings.LowPerformanceIntervals.Contains(Element))
    {
        UpdateInterval = ElementUpdateSettings.LowPerformanceIntervals[Element];
    }
    else if (ElementUpdateSettings.BaseUpdateIntervals.Contains(Element))
    {
        UpdateInterval = ElementUpdateSettings.BaseUpdateIntervals[Element];
    }
    else
    {
        UpdateInterval = 0.033f; // Default 30 FPS
    }
    
    // Apply adaptive quality scaling to update interval
    if (PerformanceSettings.bEnableAdaptiveQuality)
    {
        UpdateInterval *= (2.0f - CurrentPerformanceMetrics.AdaptiveQualityLevel);
    }
    
    return (CurrentTime - LastUpdateTime) >= UpdateInterval;
}

void AAdvancedHUDSystem::UpdateAdaptiveQuality()
{
    if (!PerformanceSettings.bEnableAdaptiveQuality) return;
    
    float PerformanceScore = CalculatePerformanceScore();
    float TargetQuality = FMath::Clamp(PerformanceScore, 0.3f, 1.0f);
    
    // Smooth quality transitions
    float QualityChangeRate = 2.0f; // Quality units per second
    float DeltaTime = GetWorld()->GetDeltaSeconds();
    
    if (TargetQuality > CurrentPerformanceMetrics.AdaptiveQualityLevel)
    {
        // Increase quality slowly
        CurrentPerformanceMetrics.AdaptiveQualityLevel = FMath::FInterpTo(
            CurrentPerformanceMetrics.AdaptiveQualityLevel,
            TargetQuality,
            DeltaTime,
            QualityChangeRate * 0.5f // Slower quality increases
        );
    }
    else
    {
        // Decrease quality quickly
        CurrentPerformanceMetrics.AdaptiveQualityLevel = FMath::FInterpTo(
            CurrentPerformanceMetrics.AdaptiveQualityLevel,
            TargetQuality,
            DeltaTime,
            QualityChangeRate
        );
    }
    
    CurrentPerformanceMetrics.AdaptiveQualityLevel = 
        FMath::Clamp(CurrentPerformanceMetrics.AdaptiveQualityLevel, 0.3f, 1.0f);
}

void AAdvancedHUDSystem::CullLowPriorityElements()
{
    // Temporarily disable low-priority elements during performance issues
    TArray<EHUDElement> LowPriorityElements = {
        EHUDElement::ObjectiveMarker,
        EHUDElement::Minimap,
        EHUDElement::WeaponInfo,
        EHUDElement::ScoreBoard
    };
    
    for (EHUDElement Element : LowPriorityElements)
    {
        if (HUDElements.Contains(Element))
        {
            FHUDElementData& ElementData = HUDElements[Element];
            if (CurrentPerformanceMetrics.bIsPerformanceLow)
            {
                ElementData.bVisible = false;
            }
            else if (CurrentPerformanceMetrics.CurrentFPS > PerformanceSettings.HighPerformanceThreshold)
            {
                ElementData.bVisible = true; // Re-enable when performance improves
            }
        }
    }
}

void AAdvancedHUDSystem::OptimizeMemoryUsage()
{
    // Clean up expired damage indicators
    for (int32 i = ActiveDamageIndicators.Num() - 1; i >= 0; --i)
    {
        if (ActiveDamageIndicators[i].TimeRemaining <= 0.0f)
        {
            ActiveDamageIndicators.RemoveAt(i);
        }
    }
    
    // Clean up expired kill feed entries
    for (int32 i = KillFeedEntries.Num() - 1; i >= 0; --i)
    {
        if (KillFeedEntries[i].ElapsedTime >= KillFeedEntries[i].DisplayTime)
        {
            KillFeedEntries.RemoveAt(i);
        }
    }
    
    // Limit collection sizes when memory is high
    if (CurrentPerformanceMetrics.bIsMemoryHigh)
    {
        int32 MaxDamageIndicators = FMath::Max(5, MaxDamageIndicators / 2);
        if (ActiveDamageIndicators.Num() > MaxDamageIndicators)
        {
            ActiveDamageIndicators.RemoveAt(0, ActiveDamageIndicators.Num() - MaxDamageIndicators);
        }
        
        int32 MaxKillFeedEntries = FMath::Max(3, this->MaxKillFeedEntries / 2);
        if (KillFeedEntries.Num() > MaxKillFeedEntries)
        {
            KillFeedEntries.RemoveAt(0, KillFeedEntries.Num() - MaxKillFeedEntries);
        }
    }
    
    // Clean up inactive widgets
    TArray<FString> WidgetsToRemove;
    for (auto& WidgetPair : ActiveWidgets)
    {
        if (!IsValid(WidgetPair.Value) || !WidgetPair.Value->IsInViewport())
        {
            WidgetsToRemove.Add(WidgetPair.Key);
        }
    }
    
    for (const FString& WidgetName : WidgetsToRemove)
    {
        ActiveWidgets.Remove(WidgetName);
    }
}

float AAdvancedHUDSystem::CalculatePerformanceScore() const
{
    // Calculate normalized performance score (0.0 to 1.0)
    float FPSScore = FMath::Clamp(
        CurrentPerformanceMetrics.CurrentFPS / PerformanceSettings.TargetFPS,
        0.0f, 1.0f
    );
    
    float MemoryScore = FMath::Clamp(
        1.0f - (CurrentPerformanceMetrics.MemoryUsageMB / PerformanceSettings.MaxMemoryUsageMB),
        0.0f, 1.0f
    );
    
    // Weighted average (FPS is more important for HUD responsiveness)
    return (FPSScore * 0.7f) + (MemoryScore * 0.3f);
}

void AAdvancedHUDSystem::ApplyQualityScaling(float QualityLevel)
{
    CurrentPerformanceMetrics.AdaptiveQualityLevel = FMath::Clamp(QualityLevel, 0.3f, 1.0f);
    
    // Scale HUD element properties based on quality level
    HUDScale = 0.7f + (0.3f * CurrentPerformanceMetrics.AdaptiveQualityLevel);
    HUDOpacity = 0.8f + (0.2f * CurrentPerformanceMetrics.AdaptiveQualityLevel);
    
    // Adjust crosshair quality
    CrosshairSize = CrosshairSize * CurrentPerformanceMetrics.AdaptiveQualityLevel;
    
    // Adjust quality-dependent update intervals
    for (auto& ElementPair : ElementUpdateSettings.BaseUpdateIntervals)
    {
        EHUDElement Element = ElementPair.Key;
        float BaseInterval = ElementPair.Value;
        
        // Higher quality = more frequent updates
        float QualityMultiplier = 2.0f - CurrentPerformanceMetrics.AdaptiveQualityLevel;
        ElementUpdateSettings.BaseUpdateIntervals[Element] = BaseInterval * QualityMultiplier;
    }
}

void AAdvancedHUDSystem::ResetPerformanceOptimization()
{
    // Reset to default settings
    CurrentPerformanceMetrics.AdaptiveQualityLevel = 1.0f;
    HUDScale = 1.0f;
    HUDOpacity = 1.0f;
    HUDUpdateInterval = 0.016f;
    
    // Re-enable all HUD elements
    for (auto& ElementPair : HUDElements)
    {
        ElementPair.Value.bVisible = true;
    }
    
    // Reset update intervals to defaults
    ElementUpdateSettings = FHUDElementUpdateSettings();
    
    UE_LOG(LogTemp, Log, TEXT("HUD Performance optimization reset to defaults"));
}

FString AAdvancedHUDSystem::GetPerformanceReport() const
{
    return FString::Printf(
        TEXT("HUD Performance Report:\n")
        TEXT("Current FPS: %.1f\n")
        TEXT("Average Frame Time: %.3f ms\n")
        TEXT("Memory Usage: %.1f MB\n")
        TEXT("Adaptive Quality: %.2f\n")
        TEXT("Visible Elements: %d\n")
        TEXT("Culled Elements: %d\n")
        TEXT("Performance State: %s\n")
        TEXT("Memory State: %s"),
        CurrentPerformanceMetrics.CurrentFPS,
        CurrentPerformanceMetrics.AverageFrameTime * 1000.0f,
        CurrentPerformanceMetrics.MemoryUsageMB,
        CurrentPerformanceMetrics.AdaptiveQualityLevel,
        CurrentPerformanceMetrics.VisibleElements,
        CurrentPerformanceMetrics.CulledElements,
        CurrentPerformanceMetrics.bIsPerformanceLow ? TEXT("Low") : TEXT("Good"),
        CurrentPerformanceMetrics.bIsMemoryHigh ? TEXT("High") : TEXT("Normal")
    );
}

// Public interface functions
void AAdvancedHUDSystem::SetHUDStyle(EHUDStyle NewStyle)
{
    CurrentHUDStyle = NewStyle;
    LoadHUDAssets();
}

void AAdvancedHUDSystem::ShowDamageIndicator(const FVector& DamageDirection, float DamageAmount)
{
    if (ActiveDamageIndicators.Num() >= MaxDamageIndicators)
    {
        ActiveDamageIndicators.RemoveAt(0);
    }
    
    FDamageIndicator NewIndicator;
    NewIndicator.Direction = DamageDirection.GetSafeNormal();
    NewIndicator.Damage = DamageAmount;
    NewIndicator.ElapsedTime = 0.0f;
    
    ActiveDamageIndicators.Add(NewIndicator);
}

void AAdvancedHUDSystem::AddKillFeedEntry(const FString& KillerName, const FString& VictimName, const FString& WeaponName)
{
    if (KillFeedEntries.Num() >= MaxKillFeedEntries)
    {
        KillFeedEntries.RemoveAt(0);
    }
    
    FKillFeedEntry NewEntry;
    NewEntry.KillerName = KillerName;
    NewEntry.VictimName = VictimName;
    NewEntry.WeaponName = WeaponName;
    NewEntry.ElapsedTime = 0.0f;
    
    KillFeedEntries.Add(NewEntry);
}

void AAdvancedHUDSystem::SetCrosshairStyle(const FString& StyleName)
{
    // Load crosshair texture based on style name
    FString TexturePath = FString::Printf(TEXT("/Game/UI/Crosshairs/%s"), *StyleName);
    CrosshairTexture = LoadObject<UTexture2D>(nullptr, *TexturePath);
}

void AAdvancedHUDSystem::SetCrosshairColor(const FLinearColor& NewColor)
{
    CrosshairColor = NewColor;
}

void AAdvancedHUDSystem::SetCrosshairScale(float NewScale)
{
    CrosshairScale = FMath::Clamp(NewScale, 0.1f, 3.0f);
}

void AAdvancedHUDSystem::ToggleMinimap()
{
    bShowMinimap = !bShowMinimap;
}

void AAdvancedHUDSystem::SetMinimapZoom(float NewZoom)
{
    MinimapZoom = FMath::Clamp(NewZoom, 0.5f, 3.0f);
}

void AAdvancedHUDSystem::RegisterWidget(const FString& WidgetName, UUserWidget* Widget)
{
    if (Widget)
    {
        ActiveWidgets.Add(WidgetName, Widget);
    }
}

void AAdvancedHUDSystem::UnregisterWidget(const FString& WidgetName)
{
    if (UUserWidget* Widget = ActiveWidgets.FindRef(WidgetName))
    {
        Widget->RemoveFromParent();
        ActiveWidgets.Remove(WidgetName);
    }
}

UUserWidget* AAdvancedHUDSystem::GetWidget(const FString& WidgetName)
{
    return ActiveWidgets.FindRef(WidgetName);
}

void AAdvancedHUDSystem::ShowWidget(const FString& WidgetName)
{
    if (UUserWidget* Widget = GetWidget(WidgetName))
    {
        Widget->SetVisibility(ESlateVisibility::Visible);
    }
}

void AAdvancedHUDSystem::HideWidget(const FString& WidgetName)
{
    if (UUserWidget* Widget = GetWidget(WidgetName))
    {
        Widget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void AAdvancedHUDSystem::ToggleDebugInfo()
{
    bShowDebugInfo = !bShowDebugInfo;
}

void AAdvancedHUDSystem::SetHUDEnabled(bool bEnabled)
{
    bHUDEnabled = bEnabled;
}

// Helper functions (these would typically get data from player/weapon components)
float AAdvancedHUDSystem::GetPlayerHealth()
{
    // Implementation would get actual health from player character
    return 100.0f; // Placeholder
}

float AAdvancedHUDSystem::GetPlayerMaxHealth()
{
    return 100.0f; // Placeholder
}

float AAdvancedHUDSystem::GetPlayerArmor()
{
    return 75.0f; // Placeholder
}

float AAdvancedHUDSystem::GetPlayerMaxArmor()
{
    return 100.0f; // Placeholder
}

int32 AAdvancedHUDSystem::GetCurrentAmmo()
{
    return 30; // Placeholder
}

int32 AAdvancedHUDSystem::GetMaxAmmo()
{
    return 30; // Placeholder
}

int32 AAdvancedHUDSystem::GetReserveAmmo()
{
    return 120; // Placeholder
}

bool AAdvancedHUDSystem::IsReloading()
{
    return false; // Placeholder
}

float AAdvancedHUDSystem::GetCurrentWeaponSpread()
{
    return 0.1f; // Placeholder
}

float AAdvancedHUDSystem::GetUsedMemoryMB()
{
    // Get actual memory usage
    return FPlatformMemory::GetStats().UsedPhysical / (1024.0f * 1024.0f);
}

void AAdvancedHUDSystem::SetupCrosshair()
{
    // Initialize crosshair settings
    if (!CrosshairTexture)
    {
        LoadHUDAssets();
    }
}

void AAdvancedHUDSystem::InitializeMinimap()
{
    // Initialize minimap settings and data
    MinimapEntities.Empty();
    bShowMinimap = true;
    MinimapZoom = 1.0f;
}
