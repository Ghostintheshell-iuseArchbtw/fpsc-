#include "WeaponAttachment.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "AdvancedWeaponSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogWeaponAttachment, Log, All);

UWeaponAttachment::UWeaponAttachment()
{
    // Initialize default values
    AttachmentName = "Default Attachment";
    AttachmentType = EAttachmentType::Optic;
    Cost = 500;
    UnlockLevel = 1;
    bRequiresUpdate = false;

    // Initialize modifiers to neutral values
    Modifiers.DamageMultiplier = 1.0f;
    Modifiers.AccuracyBonus = 0.0f;
    Modifiers.RangeMultiplier = 1.0f;
    Modifiers.RecoilReduction = 0.0f;
    Modifiers.FireRateMultiplier = 1.0f;
    Modifiers.ReloadTimeMultiplier = 1.0f;
    Modifiers.MovementSpeedMultiplier = 1.0f;
    Modifiers.WeightAddition = 0.0f;
    Modifiers.ConcealmentPenalty = 0.0f;
    Modifiers.DurabilityBonus = 0.0f;

    // Set legacy properties for backward compatibility
    DamageMultiplier = 1.0f;
    AccuracyBonus = 0.0f;
    RangeMultiplier = 1.0f;
    RecoilReduction = 0.0f;
    FireRateMultiplier = 1.0f;
    ReloadTimeMultiplier = 1.0f;
}

bool UWeaponAttachment::IsCompatibleWith(const FString& WeaponName) const
{
    if (CompatibleWeapons.Num() == 0)
    {
        // If no specific weapons listed, compatible with all
        return true;
    }

    return CompatibleWeapons.Contains(WeaponName);
}

AWeaponAttachment::AWeaponAttachment()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create attachment mesh component
    AttachmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachmentMesh"));
    RootComponent = AttachmentMesh;

    // Initialize default values
    bVisibleWhenAttached = true;
    AttachmentOffset = FVector::ZeroVector;
    AttachmentRotation = FRotator::ZeroRotator;
    AttachedWeapon = nullptr;
}

void AWeaponAttachment::BeginPlay()
{
    Super::BeginPlay();

    // Ensure we have attachment data
    if (!AttachmentData)
    {
        UE_LOG(LogWeaponAttachment, Warning, TEXT("AWeaponAttachment %s has no AttachmentData set!"), *GetName());
    }
}

void AWeaponAttachment::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update attachment if required
    if (AttachmentData && AttachmentData->bRequiresUpdate && IsAttached())
    {
        UpdateAttachment(DeltaTime);
    }
}

void AWeaponAttachment::AttachToWeapon(UAdvancedWeaponSystem* WeaponSystem, const FString& SocketName)
{
    if (!WeaponSystem)
    {
        UE_LOG(LogWeaponAttachment, Warning, TEXT("Cannot attach %s: WeaponSystem is null"), *GetName());
        return;
    }

    if (!AttachmentData)
    {
        UE_LOG(LogWeaponAttachment, Warning, TEXT("Cannot attach %s: AttachmentData is null"), *GetName());
        return;
    }

    // Detach from previous weapon if already attached
    if (AttachedWeapon)
    {
        DetachFromWeapon();
    }

    AttachedWeapon = WeaponSystem;

    // Attach to weapon mesh
    USkeletalMeshComponent* WeaponMesh = WeaponSystem->GetWeaponMesh();
    if (WeaponMesh)
    {
        FString SocketToUse = SocketName;
        if (SocketToUse.IsEmpty())
        {
            // Use default socket name based on attachment type
            switch (AttachmentData->AttachmentType)
            {
                case EAttachmentType::Optic:
                    SocketToUse = "OpticSocket";
                    break;
                case EAttachmentType::Suppressor:
                    SocketToUse = "MuzzleSocket";
                    break;
                case EAttachmentType::Grip:
                    SocketToUse = "GripSocket";
                    break;
                case EAttachmentType::Stock:
                    SocketToUse = "StockSocket";
                    break;
                case EAttachmentType::Laser:
                case EAttachmentType::Flashlight:
                    SocketToUse = "RailSocket";
                    break;
                default:
                    SocketToUse = "AttachmentSocket";
                    break;
            }
        }

        AttachToComponent(
            WeaponMesh,
            FAttachmentTransformRules::SnapToTargetIncludingScale,
            FName(*SocketToUse)
        );

        // Apply attachment offset and rotation
        SetActorRelativeLocation(AttachmentOffset);
        SetActorRelativeRotation(AttachmentRotation);

        // Set visibility
        SetActorHiddenInGame(!bVisibleWhenAttached);

        UE_LOG(LogWeaponAttachment, Log, TEXT("Attached %s to weapon %s at socket %s"), 
               *GetName(), *WeaponSystem->GetName(), *SocketToUse);

        // Call blueprint event
        OnAttached();
    }
    else
    {
        UE_LOG(LogWeaponAttachment, Warning, TEXT("Cannot attach %s: WeaponMesh is null"), *GetName());
    }
}

void AWeaponAttachment::DetachFromWeapon()
{
    if (!AttachedWeapon)
    {
        return;
    }

    UE_LOG(LogWeaponAttachment, Log, TEXT("Detaching %s from weapon %s"), 
           *GetName(), *AttachedWeapon->GetName());

    // Detach from weapon
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Clear reference
    AttachedWeapon = nullptr;

    // Show the attachment again
    SetActorHiddenInGame(false);

    // Call blueprint event
    OnDetached();
}

void AWeaponAttachment::UpdateAttachment(float DeltaTime)
{
    // Base implementation - can be overridden in subclasses or blueprints
    // For example, laser attachments might update their targeting dot here
    
    if (!IsAttached() || !AttachmentData)
    {
        return;
    }

    // Handle specific attachment types
    switch (AttachmentData->AttachmentType)
    {
        case EAttachmentType::Laser:
            // Update laser dot position
            // This would typically involve raycasting from the attachment
            break;
            
        case EAttachmentType::Flashlight:
            // Update flashlight direction and intensity
            break;
            
        default:
            // Most attachments don't need per-frame updates
            break;
    }
}

void AWeaponAttachment::SetAttachmentData(UWeaponAttachment* NewData)
{
    AttachmentData = NewData;
    
    if (AttachmentData && AttachmentData->AttachmentMesh.IsValid())
    {
        // Load and set the attachment mesh
        UStaticMesh* Mesh = AttachmentData->AttachmentMesh.LoadSynchronous();
        if (Mesh && AttachmentMesh)
        {
            AttachmentMesh->SetStaticMesh(Mesh);
        }
    }
}

// Add advanced weapon attachment system functions after existing code

// Advanced compatibility checking
FWeaponCompatibilityCheck UWeaponAttachment::CheckCompatibilityAdvanced(UAdvancedWeaponSystem* WeaponSystem, const TArray<UWeaponAttachment*>& ExistingAttachments) const
{
    FWeaponCompatibilityCheck Result;
    Result.bIsCompatible = true;
    Result.CompatibilityScore = 1.0f;
    
    if (!WeaponSystem)
    {
        Result.bIsCompatible = false;
        Result.ReasonIfIncompatible = "Invalid weapon system";
        Result.CompatibilityScore = 0.0f;
        return Result;
    }

    // Check basic compatibility
    if (!IsCompatibleWith(WeaponSystem->GetWeaponName()))
    {
        Result.bIsCompatible = false;
        Result.ReasonIfIncompatible = "Weapon not supported";
        Result.CompatibilityScore = 0.0f;
        return Result;
    }

    // Check for conflicts with existing attachments
    for (const UWeaponAttachment* ExistingAttachment : ExistingAttachments)
    {
        if (!ExistingAttachment) continue;

        // Check for attachment conflicts
        for (const FString& ConflictType : ConflictingAttachmentTypes)
        {
            if (ExistingAttachment->AttachmentName.Contains(ConflictType))
            {
                Result.ConflictingAttachments.Add(ExistingAttachment->AttachmentName);
                Result.CompatibilityScore *= 0.5f; // Reduce compatibility score
            }
        }

        // Check for slot conflicts
        if (ExistingAttachment->AttachmentType == AttachmentType)
        {
            Result.bIsCompatible = false;
            Result.ReasonIfIncompatible = FString::Printf(TEXT("Slot already occupied by %s"), *ExistingAttachment->AttachmentName);
            return Result;
        }
    }

    // Check advanced compatibility criteria
    if (AdvancedModifiers.EnvironmentalRequirements.Num() > 0)
    {
        // Check environmental compatibility
        bool bEnvironmentCompatible = false;
        for (const FString& Environment : AdvancedModifiers.EnvironmentalRequirements)
        {
            if (WeaponSystem->GetCurrentEnvironment().Contains(Environment))
            {
                bEnvironmentCompatible = true;
                break;
            }
        }
        
        if (!bEnvironmentCompatible)
        {
            Result.CompatibilityScore *= 0.7f;
            Result.ReasonIfIncompatible = "Environmental requirements not met";
        }
    }

    // Final compatibility check
    if (Result.CompatibilityScore < 0.3f)
    {
        Result.bIsCompatible = false;
        if (Result.ReasonIfIncompatible.IsEmpty())
        {
            Result.ReasonIfIncompatible = "Low compatibility score";
        }
    }

    return Result;
}

// Real-time stat preview calculation
FWeaponStatsPreview UWeaponAttachment::CalculateStatsPreview(UAdvancedWeaponSystem* WeaponSystem, const TArray<UWeaponAttachment*>& ExistingAttachments) const
{
    FWeaponStatsPreview Preview;
    
    if (!WeaponSystem)
    {
        return Preview;
    }

    // Get base weapon stats
    Preview.BaseStats = WeaponSystem->GetBaseWeaponStats();
    Preview.CurrentStats = WeaponSystem->GetCurrentWeaponStats();

    // Calculate preview stats with this attachment added
    FWeaponStats TempStats = Preview.CurrentStats;
    
    // Apply this attachment's modifiers
    ApplyModifiersToStats(TempStats);
    
    // Apply synergy bonuses with existing attachments
    float SynergyMultiplier = CalculateSynergyBonus(ExistingAttachments);
    if (SynergyMultiplier != 1.0f)
    {
        TempStats.BaseDamage *= SynergyMultiplier;
        TempStats.BaseAccuracy += (SynergyMultiplier - 1.0f) * 0.1f; // Small accuracy bonus for synergy
        TempStats.FireRate *= SynergyMultiplier;
    }

    Preview.PreviewStats = TempStats;

    // Calculate differences for UI display
    Preview.StatDifferences.DamageDifference = Preview.PreviewStats.BaseDamage - Preview.CurrentStats.BaseDamage;
    Preview.StatDifferences.AccuracyDifference = Preview.PreviewStats.BaseAccuracy - Preview.CurrentStats.BaseAccuracy;
    Preview.StatDifferences.FireRateDifference = Preview.PreviewStats.FireRate - Preview.CurrentStats.FireRate;
    Preview.StatDifferences.RangeDifference = Preview.PreviewStats.EffectiveRange - Preview.CurrentStats.EffectiveRange;
    Preview.StatDifferences.ReloadTimeDifference = Preview.PreviewStats.ReloadTime - Preview.CurrentStats.ReloadTime;
    Preview.StatDifferences.RecoilDifference = Preview.PreviewStats.VerticalRecoil - Preview.CurrentStats.VerticalRecoil;
    
    // Set improvement flags
    Preview.StatDifferences.bDamageImproved = Preview.StatDifferences.DamageDifference > 0;
    Preview.StatDifferences.bAccuracyImproved = Preview.StatDifferences.AccuracyDifference > 0;
    Preview.StatDifferences.bFireRateImproved = Preview.StatDifferences.FireRateDifference > 0;
    Preview.StatDifferences.bRangeImproved = Preview.StatDifferences.RangeDifference > 0;
    Preview.StatDifferences.bReloadTimeImproved = Preview.StatDifferences.ReloadTimeDifference < 0; // Lower is better
    Preview.StatDifferences.bRecoilImproved = Preview.StatDifferences.RecoilDifference < 0; // Lower is better

    return Preview;
}

void UWeaponAttachment::ApplyModifiersToStats(FWeaponStats& Stats) const
{
    // Apply base modifiers
    Stats.BaseDamage *= Modifiers.DamageMultiplier;
    Stats.BaseAccuracy += Modifiers.AccuracyBonus;
    Stats.FireRate *= Modifiers.FireRateMultiplier;
    Stats.ReloadTime *= Modifiers.ReloadTimeMultiplier;
    Stats.EffectiveRange *= Modifiers.RangeMultiplier;
    Stats.VerticalRecoil *= (1.0f - Modifiers.RecoilReduction);
    Stats.HorizontalRecoil *= (1.0f - Modifiers.RecoilReduction);

    // Apply advanced modifiers
    if (AdvancedModifiers.ConditionalModifiers.Num() > 0)
    {
        // Apply conditional modifiers based on current game state
        for (const FConditionalModifier& ConditionalMod : AdvancedModifiers.ConditionalModifiers)
        {
            // Check if condition is met (simplified for example)
            if (ConditionalMod.Condition == "CrouchedFiring")
            {
                Stats.BaseAccuracy += ConditionalMod.AccuracyBonus;
                Stats.VerticalRecoil *= (1.0f - ConditionalMod.RecoilReduction);
            }
            else if (ConditionalMod.Condition == "MovingFiring")
            {
                Stats.BaseAccuracy -= ConditionalMod.AccuracyBonus; // Penalty when moving
            }
        }
    }

    // Apply environmental effects
    if (AdvancedModifiers.EnvironmentalModifiers.Num() > 0)
    {
        for (const FEnvironmentalModifier& EnvMod : AdvancedModifiers.EnvironmentalModifiers)
        {
            // Apply environmental modifiers (example: night vision in darkness)
            if (EnvMod.EnvironmentType == "Darkness" && AttachmentType == EAttachmentType::Optic)
            {
                Stats.BaseAccuracy += EnvMod.AccuracyBonus;
            }
        }
    }

    // Clamp values to valid ranges
    Stats.BaseAccuracy = FMath::Clamp(Stats.BaseAccuracy, 0.1f, 1.0f);
    Stats.FireRate = FMath::Max(Stats.FireRate, 60.0f); // Minimum 1 shot per second
    Stats.ReloadTime = FMath::Max(Stats.ReloadTime, 0.5f); // Minimum 0.5 seconds
    Stats.EffectiveRange = FMath::Max(Stats.EffectiveRange, 50.0f); // Minimum 50 units
}

float UWeaponAttachment::CalculateSynergyBonus(const TArray<UWeaponAttachment*>& ExistingAttachments) const
{
    float SynergyMultiplier = 1.0f;
    
    for (const UWeaponAttachment* ExistingAttachment : ExistingAttachments)
    {
        if (!ExistingAttachment) continue;

        // Check for synergy bonuses
        for (const FSynergyBonus& Synergy : AdvancedModifiers.SynergyBonuses)
        {
            if (ExistingAttachment->AttachmentName.Contains(Synergy.RequiredAttachmentType))
            {
                SynergyMultiplier += Synergy.BonusMultiplier;
                UE_LOG(LogWeaponAttachment, Log, TEXT("Synergy bonus applied: %s + %s = %.2f multiplier"), 
                       *AttachmentName, *ExistingAttachment->AttachmentName, Synergy.BonusMultiplier);
            }
        }

        // Check for anti-synergy penalties
        for (const FAntiSynergyPenalty& AntiSynergy : AdvancedModifiers.AntiSynergyPenalties)
        {
            if (ExistingAttachment->AttachmentName.Contains(AntiSynergy.ConflictingAttachmentType))
            {
                SynergyMultiplier -= AntiSynergy.PenaltyMultiplier;
                UE_LOG(LogWeaponAttachment, Warning, TEXT("Anti-synergy penalty applied: %s + %s = %.2f penalty"), 
                       *AttachmentName, *ExistingAttachment->AttachmentName, AntiSynergy.PenaltyMultiplier);
            }
        }
    }

    return FMath::Clamp(SynergyMultiplier, 0.1f, 3.0f); // Clamp between 10% and 300%
}

TArray<FSmartAttachmentSuggestion> UWeaponAttachment::GetSmartSuggestions(UAdvancedWeaponSystem* WeaponSystem, const TArray<UWeaponAttachment*>& AvailableAttachments) const
{
    TArray<FSmartAttachmentSuggestion> Suggestions;
    
    if (!WeaponSystem) return Suggestions;

    // Analyze current weapon weaknesses and suggest complementary attachments
    FWeaponStats CurrentStats = WeaponSystem->GetCurrentWeaponStats();
    
    for (UWeaponAttachment* Attachment : AvailableAttachments)
    {
        if (!Attachment || Attachment == this) continue;

        FSmartAttachmentSuggestion Suggestion;
        Suggestion.SuggestedAttachment = Attachment;
        Suggestion.Reason = "";
        Suggestion.Priority = ESuggestionPriority::Low;
        Suggestion.CompatibilityScore = 0.5f;

        // Check if this attachment addresses weapon weaknesses
        if (CurrentStats.BaseAccuracy < 0.7f && Attachment->Modifiers.AccuracyBonus > 0.0f)
        {
            Suggestion.Reason = "Improves weapon accuracy";
            Suggestion.Priority = ESuggestionPriority::High;
            Suggestion.CompatibilityScore += 0.3f;
        }

        if (CurrentStats.VerticalRecoil > 1.0f && Attachment->Modifiers.RecoilReduction > 0.0f)
        {
            Suggestion.Reason += (Suggestion.Reason.IsEmpty() ? "" : " and ") + FString("Reduces recoil");
            Suggestion.Priority = ESuggestionPriority::Medium;
            Suggestion.CompatibilityScore += 0.2f;
        }

        if (CurrentStats.EffectiveRange < 300.0f && Attachment->Modifiers.RangeMultiplier > 1.0f)
        {
            Suggestion.Reason += (Suggestion.Reason.IsEmpty() ? "" : " and ") + FString("Extends effective range");
            Suggestion.Priority = ESuggestionPriority::Medium;
            Suggestion.CompatibilityScore += 0.2f;
        }

        // Check for synergy with current attachment
        float SynergyBonus = CalculateSynergyBonus({Attachment});
        if (SynergyBonus > 1.1f)
        {
            Suggestion.Reason += (Suggestion.Reason.IsEmpty() ? "" : " and ") + FString("Creates synergy bonus");
            Suggestion.Priority = ESuggestionPriority::High;
            Suggestion.CompatibilityScore += 0.3f;
        }

        // Only add meaningful suggestions
        if (Suggestion.CompatibilityScore > 0.6f && !Suggestion.Reason.IsEmpty())
        {
            Suggestions.Add(Suggestion);
        }
    }

    // Sort suggestions by priority and compatibility
    Suggestions.Sort([](const FSmartAttachmentSuggestion& A, const FSmartAttachmentSuggestion& B) {
        if (A.Priority != B.Priority)
            return (int32)A.Priority > (int32)B.Priority;
        return A.CompatibilityScore > B.CompatibilityScore;
    });

    return Suggestions;
}

FAttachmentConflictInfo UWeaponAttachment::ResolveConflict(UAdvancedWeaponSystem* WeaponSystem, UWeaponAttachment* ConflictingAttachment) const
{
    FAttachmentConflictInfo ConflictInfo;
    ConflictInfo.bHasConflict = false;
    
    if (!ConflictingAttachment) return ConflictInfo;

    // Check for direct conflicts
    if (AttachmentType == ConflictingAttachment->AttachmentType)
    {
        ConflictInfo.bHasConflict = true;
        ConflictInfo.ConflictType = EConflictType::SlotConflict;
        ConflictInfo.ConflictDescription = FString::Printf(
            TEXT("Both attachments use the same slot: %s"), 
            *UEnum::GetValueAsString(AttachmentType));
        
        // Suggest resolution based on stats
        FWeaponStatsPreview ThisPreview = CalculateStatsPreview(WeaponSystem, {});
        FWeaponStatsPreview OtherPreview = ConflictingAttachment->CalculateStatsPreview(WeaponSystem, {});
        
        float ThisScore = CalculateOverallStatScore(ThisPreview.PreviewStats);
        float OtherScore = CalculateOverallStatScore(OtherPreview.PreviewStats);
        
        if (ThisScore > OtherScore)
        {
            ConflictInfo.RecommendedResolution = EConflictResolution::ReplaceExisting;
            ConflictInfo.ResolutionReason = FString::Printf(
                TEXT("%s provides better overall performance (%.2f vs %.2f)"), 
                *AttachmentName, ThisScore, OtherScore);
        }
        else
        {
            ConflictInfo.RecommendedResolution = EConflictResolution::KeepExisting;
            ConflictInfo.ResolutionReason = FString::Printf(
                TEXT("%s provides better overall performance (%.2f vs %.2f)"), 
                *ConflictingAttachment->AttachmentName, OtherScore, ThisScore);
        }
    }

    // Check for compatibility conflicts
    for (const FString& ConflictType : ConflictingAttachmentTypes)
    {
        if (ConflictingAttachment->AttachmentName.Contains(ConflictType))
        {
            ConflictInfo.bHasConflict = true;
            ConflictInfo.ConflictType = EConflictType::CompatibilityConflict;
            ConflictInfo.ConflictDescription = FString::Printf(
                TEXT("Attachment types are incompatible: %s conflicts with %s"), 
                *AttachmentName, *ConflictType);
            ConflictInfo.RecommendedResolution = EConflictResolution::ShowWarning;
            ConflictInfo.ResolutionReason = "Attachments may not work together optimally";
        }
    }

    return ConflictInfo;
}

float UWeaponAttachment::CalculateOverallStatScore(const FWeaponStats& Stats) const
{
    // Weight different stats based on general importance
    float Score = 0.0f;
    Score += Stats.BaseDamage * 0.25f;           // 25% weight on damage
    Score += Stats.BaseAccuracy * 100.0f * 0.20f; // 20% weight on accuracy
    Score += (Stats.FireRate / 10.0f) * 0.15f;   // 15% weight on fire rate
    Score += (Stats.EffectiveRange / 10.0f) * 0.15f; // 15% weight on range
    Score += (1.0f / Stats.ReloadTime) * 25.0f * 0.10f; // 10% weight on reload speed
    Score += (2.0f - Stats.VerticalRecoil) * 25.0f * 0.15f; // 15% weight on recoil control
    
    return Score;
}
