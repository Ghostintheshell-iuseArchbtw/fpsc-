#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataAsset.h"
#include "WeaponAttachment.generated.h"

// Forward declarations
class UAdvancedWeaponSystem;

UENUM(BlueprintType)
enum class EAttachmentType : uint8
{
    Optic       UMETA(DisplayName = "Optic"),
    Suppressor  UMETA(DisplayName = "Suppressor"), 
    Grip        UMETA(DisplayName = "Grip"),
    Stock       UMETA(DisplayName = "Stock"),
    Magazine    UMETA(DisplayName = "Magazine"),
    Laser       UMETA(DisplayName = "Laser"),
    Flashlight  UMETA(DisplayName = "Flashlight"),
    Bayonet     UMETA(DisplayName = "Bayonet")
};

USTRUCT(BlueprintType)
struct FAttachmentModifiers
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float AccuracyBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
    float RangeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
    float RecoilReduction = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Rate")
    float FireRateMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
    float ReloadTimeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementSpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight")
    float WeightAddition = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concealment")
    float ConcealmentPenalty = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float DurabilityBonus = 0.0f;
};

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UWeaponData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FString WeaponName = "Default Weapon";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText WeaponDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float BaseDamage = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float FireRate = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 MagazineCapacity = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float EffectiveRange = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float ReloadTime = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MuzzleVelocity = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float BaseAccuracy = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 InitialAmmo = 300;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
    TArray<FVector2D> RecoilPattern;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachments")
    TArray<EAttachmentType> SupportedAttachmentTypes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    TSoftObjectPtr<USkeletalMesh> WeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    TSoftObjectPtr<class USoundCue> FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    TSoftObjectPtr<class USoundCue> ReloadSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    TSoftObjectPtr<class UParticleSystem> MuzzleFlash;
};

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UWeaponAttachment : public UObject
{
    GENERATED_BODY()

public:
    UWeaponAttachment();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FString AttachmentName = "Default Attachment";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText AttachmentDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    EAttachmentType AttachmentType = EAttachmentType::Optic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
    FAttachmentModifiers Modifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    TSoftObjectPtr<UStaticMesh> AttachmentMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    TSoftObjectPtr<class UMaterialInterface> AttachmentMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
    TArray<FString> CompatibleWeapons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 Cost = 500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock")
    int32 UnlockLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special")
    bool bRequiresUpdate = false;

    // Legacy properties for compatibility
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    float AccuracyBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    float RangeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    float RecoilReduction = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    float FireRateMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    float ReloadTimeMultiplier = 1.0f;

    // Virtual functions
    UFUNCTION(BlueprintImplementableEvent, Category = "Attachment")
    void AttachToWeapon(UAdvancedWeaponSystem* WeaponSystem);

    UFUNCTION(BlueprintImplementableEvent, Category = "Attachment")
    void DetachFromWeapon();

    UFUNCTION(BlueprintImplementableEvent, Category = "Attachment")
    void UpdateAttachment(float DeltaTime);

    // Helper functions
    UFUNCTION(BlueprintPure, Category = "Attachment")
    FAttachmentModifiers GetModifiers() const { return Modifiers; }

    UFUNCTION(BlueprintPure, Category = "Attachment")
    bool IsCompatibleWith(const FString& WeaponName) const;

    // Advanced attachment system functions
    UFUNCTION(BlueprintCallable, Category = "Advanced Attachment")
    FWeaponCompatibilityCheck CheckCompatibilityAdvanced(UAdvancedWeaponSystem* WeaponSystem, const TArray<UWeaponAttachment*>& ExistingAttachments) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachment")
    FWeaponStatsPreview CalculateStatsPreview(UAdvancedWeaponSystem* WeaponSystem, const TArray<UWeaponAttachment*>& ExistingAttachments) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachment")
    TArray<FSmartAttachmentSuggestion> GetSmartSuggestions(UAdvancedWeaponSystem* WeaponSystem, const TArray<UWeaponAttachment*>& AvailableAttachments) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachment")
    FAttachmentConflictInfo ResolveConflict(UAdvancedWeaponSystem* WeaponSystem, UWeaponAttachment* ConflictingAttachment) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachment")
    void ApplyModifiersToStats(UPARAM(ref) FWeaponStats& Stats) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachment")
    float CalculateSynergyBonus(const TArray<UWeaponAttachment*>& ExistingAttachments) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachment")
    float CalculateOverallStatScore(const FWeaponStats& Stats) const;
};

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API AWeaponAttachment : public AActor
{
    GENERATED_BODY()

public:
    AWeaponAttachment();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* AttachmentMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment Data")
    UWeaponAttachment* AttachmentData;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    UAdvancedWeaponSystem* AttachedWeapon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    bool bVisibleWhenAttached = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FVector AttachmentOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FRotator AttachmentRotation = FRotator::ZeroRotator;

public:
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Attachment")
    virtual void AttachToWeapon(UAdvancedWeaponSystem* WeaponSystem, const FString& SocketName = "");

    UFUNCTION(BlueprintCallable, Category = "Attachment")
    virtual void DetachFromWeapon();

    UFUNCTION(BlueprintCallable, Category = "Attachment")
    virtual void UpdateAttachment(float DeltaTime);

    UFUNCTION(BlueprintPure, Category = "Attachment")
    bool IsAttached() const { return AttachedWeapon != nullptr; }

    UFUNCTION(BlueprintPure, Category = "Attachment")
    UWeaponAttachment* GetAttachmentData() const { return AttachmentData; }

    UFUNCTION(BlueprintCallable, Category = "Attachment")
    void SetAttachmentData(UWeaponAttachment* NewData);

    // Special attachment behaviors
    UFUNCTION(BlueprintImplementableEvent, Category = "Special")
    void OnAttached();

    UFUNCTION(BlueprintImplementableEvent, Category = "Special")
    void OnDetached();

    UFUNCTION(BlueprintImplementableEvent, Category = "Special")
    void OnWeaponFired();

    UFUNCTION(BlueprintImplementableEvent, Category = "Special")
    void OnWeaponReloaded();
};

USTRUCT(BlueprintType)
struct FWeaponCompatibilityCheck
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    bool bIsCompatible = false;

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    FString ReasonIfIncompatible;

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    TArray<FString> ConflictingAttachments;

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    float CompatibilityScore = 0.0f; // 0.0 to 1.0
};

USTRUCT(BlueprintType)
struct FWeaponStatsPreview
{
    GENERATED_BODY()

    // Base stats (current weapon without attachments)
    UPROPERTY(BlueprintReadOnly, Category = "Base Stats")
    FWeaponStats BaseStats;

    // Modified stats (with current attachments)
    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    FWeaponStats CurrentStats;

    // Preview stats (with hypothetical attachment)
    UPROPERTY(BlueprintReadOnly, Category = "Preview Stats")
    FWeaponStats PreviewStats;

    // Stat differences for UI display
    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    FWeaponStatsDifference StatDifferences;
};

USTRUCT(BlueprintType)
struct FWeaponStatsDifference
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float DamageDifference = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float AccuracyDifference = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float RangeDifference = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float RecoilDifference = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float FireRateDifference = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float ReloadTimeDifference = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float MobilityDifference = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float WeightDifference = 0.0f;

    // Overall effectiveness score change
    UPROPERTY(BlueprintReadOnly, Category = "Differences")
    float OverallEffectivenessChange = 0.0f;
};

USTRUCT(BlueprintType)
struct FAttachmentConflictInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Conflict")
    EAttachmentType ConflictingType = EAttachmentType::Optic;

    UPROPERTY(BlueprintReadOnly, Category = "Conflict")
    FString ConflictingAttachmentName;

    UPROPERTY(BlueprintReadOnly, Category = "Conflict")
    FString ConflictReason;

    UPROPERTY(BlueprintReadOnly, Category = "Conflict")
    bool bCanResolveAutomatically = false;
};

USTRUCT(BlueprintType)
struct FSmartAttachmentSuggestion
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Suggestion")
    TSubclassOf<UWeaponAttachment> SuggestedAttachmentClass;

    UPROPERTY(BlueprintReadOnly, Category = "Suggestion")
    FString SuggestionReason;

    UPROPERTY(BlueprintReadOnly, Category = "Suggestion")
    float EffectivenessBonus = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Suggestion")
    float CompatibilityScore = 0.0f;
};

// Enhanced attachment modifiers with more granular control
USTRUCT(BlueprintType)
struct FAdvancedAttachmentModifiers : public FAttachmentModifiers
{
    GENERATED_BODY()

    // Conditional modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
    float StandingAccuracyBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
    float CrouchingAccuracyBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
    float ProneAccuracyBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
    float MovingAccuracyPenalty = 0.0f;

    // Range-dependent modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
    float ShortRangeDamageMultiplier = 1.0f; // 0-50m

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
    float MediumRangeDamageMultiplier = 1.0f; // 50-200m

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
    float LongRangeDamageMultiplier = 1.0f; // 200m+

    // Environmental modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float IndoorEffectivenessMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float OutdoorEffectivenessMultiplier = 1.0f;

    // Time-based modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float DayTimeEffectivenessMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float NightTimeEffectivenessMultiplier = 1.0f;

    // Synergy bonuses (when multiple attachments work together)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synergy")
    TMap<EAttachmentType, float> SynergyBonuses;

    // Anti-synergy penalties (when attachments conflict)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synergy")
    TMap<EAttachmentType, float> AntiSynergyPenalties;
};
