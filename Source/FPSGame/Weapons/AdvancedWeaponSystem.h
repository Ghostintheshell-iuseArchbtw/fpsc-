#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "WeaponAttachment.h"
#include "AdvancedWeaponSystem.generated.h"

// Forward declarations
class APerformanceOptimizationSystem;
class AAdvancedHUDSystem;
class AAdvancedAudioSystem;
class UAdvancedWeaponComponent;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    AssaultRifle    UMETA(DisplayName = "Assault Rifle"),
    SniperRifle     UMETA(DisplayName = "Sniper Rifle"),
    Shotgun         UMETA(DisplayName = "Shotgun"),
    Pistol          UMETA(DisplayName = "Pistol"),
    SMG             UMETA(DisplayName = "SMG"),
    LMG             UMETA(DisplayName = "LMG"),
    GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
    RocketLauncher  UMETA(DisplayName = "Rocket Launcher")
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
    Single      UMETA(DisplayName = "Single"),
    Burst       UMETA(DisplayName = "Burst"),
    FullAuto    UMETA(DisplayName = "Full Auto")
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Firing      UMETA(DisplayName = "Firing"),
    Reloading   UMETA(DisplayName = "Reloading"),
    Switching   UMETA(DisplayName = "Switching"),
    Jammed      UMETA(DisplayName = "Jammed")
};

USTRUCT(BlueprintType)
struct FWeaponStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
    float BaseDamage = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
    float HeadshotMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
    float Range = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
    float FireRate = 600.0f; // Rounds per minute

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
    int32 MagazineSize = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Stats")
    float ReloadTime = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float BaseAccuracy = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float MovementAccuracyPenalty = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float CrouchAccuracyBonus = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accuracy")
    float ProneAccuracyBonus = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
    float VerticalRecoil = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
    float HorizontalRecoil = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
    float RecoilRecoveryRate = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float BulletVelocity = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float BulletDrop = 9.81f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float WindResistance = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration")
    float ArmorPenetration = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration")
    float WallPenetration = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float MaxDurability = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float DurabilityLossPerShot = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight")
    float Weight = 3.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic")
    int32 BaseCost = 2500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic")
    int32 AmmoCost = 5;
};

USTRUCT(BlueprintType)
struct FAttachmentSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class AWeaponAttachment> AllowedAttachmentClass;

    UPROPERTY(BlueprintReadOnly)
    class AWeaponAttachment* CurrentAttachment = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform SlotTransform;
};

USTRUCT(BlueprintType)
struct FBallisticsData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector StartLocation;

    UPROPERTY(BlueprintReadOnly)
    FVector EndLocation;

    UPROPERTY(BlueprintReadOnly)
    FVector Velocity;

    UPROPERTY(BlueprintReadOnly)
    float FlightTime;

    UPROPERTY(BlueprintReadOnly)
    float DropAmount;

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> TrajectoryPoints;
};

USTRUCT(BlueprintType, meta = (UseCustomStructureParam = "SlotName"))
struct FWeaponDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString WeaponName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWeaponType WeaponType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWeaponStats Stats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EFireMode> AvailableFireModes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FAttachmentSlot> AttachmentSlots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USkeletalMesh> WeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundCue> FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundCue> ReloadSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UParticleSystem> MuzzleFlash;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UAnimMontage> FireAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UAnimMontage> ReloadAnimation;
};

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UAdvancedWeaponSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    AAdvancedWeaponSystem();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Core Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USkeletalMeshComponent* WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* AudioComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UParticleSystemComponent* MuzzleFlashComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* SightComponent;

    // Weapon Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config", Replicated)
    FWeaponStats WeaponStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
    EWeaponType WeaponType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
    TArray<EFireMode> AvailableFireModes;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon State", ReplicatedUsing = OnRep_CurrentFireMode)
    EFireMode CurrentFireMode;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon State", ReplicatedUsing = OnRep_WeaponState)
    EWeaponState CurrentWeaponState;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon State", Replicated)
    int32 CurrentAmmo;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon State", Replicated)
    int32 ReserveAmmo;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon State", Replicated)
    float CurrentDurability;

    // Data Table
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    UDataTable* WeaponDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    FName WeaponRowName;

    // Attachment System
    UPROPERTY(BlueprintReadOnly, Category = "Attachments", Replicated)
    TArray<FAttachmentSlot> AttachmentSlots;

    // Ballistics System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    bool bUseRealisticBallistics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float WindSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    FVector WindDirection = FVector::ZeroVector;

    // Audio Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* ReloadSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* DryFireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* SwitchFireModeSound;

    // Visual Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    UParticleSystem* MuzzleFlashEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    UParticleSystem* ShellEjectEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    UParticleSystem* TracerEffect;

    // Core Weapon Functions
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void StartFiring();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void StopFiring();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void Reload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void SwitchFireMode();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual bool CanFire() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual bool CanReload() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void SetWeaponData(FName RowName);

    // Ballistics Functions
    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    FBallisticsData CalculateBallistics(const FVector& StartLocation, const FVector& TargetLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    TArray<FVector> GetTrajectoryPoints(const FVector& StartLocation, const FVector& InitialVelocity, float TimeStep = 0.1f, int32 MaxSteps = 100) const;

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    float CalculateBulletDrop(float Distance, float Velocity) const;

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    FVector ApplyWindEffect(const FVector& Velocity, float FlightTime) const;

    // Attachment Functions
    UFUNCTION(BlueprintCallable, Category = "Attachments")
    bool AttachWeaponAttachment(const FString& SlotName, class AWeaponAttachment* Attachment);

    UFUNCTION(BlueprintCallable, Category = "Attachments")
    bool RemoveWeaponAttachment(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Attachments")
    class AWeaponAttachment* GetAttachment(const FString& SlotName) const;

    UFUNCTION(BlueprintCallable, Category = "Attachments")
    void UpdateWeaponStatsWithAttachments();

    // Durability System
    UFUNCTION(BlueprintCallable, Category = "Durability")
    void ApplyDurabilityDamage(float Damage);

    UFUNCTION(BlueprintCallable, Category = "Durability")
    void RepairWeapon(float RepairAmount);

    UFUNCTION(BlueprintCallable, Category = "Durability")
    float GetDurabilityPercentage() const;

    UFUNCTION(BlueprintCallable, Category = "Durability")
    bool IsWeaponJammed() const;

    // Network Functions
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartFiring();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStopFiring();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerReload();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSwitchFireMode();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayFireEffect();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayReloadEffect();

    // Events
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponFired, AAdvancedWeaponSystem*, Weapon, const FVector&, HitLocation);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWeaponFired OnWeaponFired;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded, AAdvancedWeaponSystem*, Weapon);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWeaponReloaded OnWeaponReloaded;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, CurrentAmmo, int32, ReserveAmmo);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAmmoChanged OnAmmoChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireModeChanged, EFireMode, NewFireMode);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFireModeChanged OnFireModeChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponJammed, AAdvancedWeaponSystem*, Weapon);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWeaponJammed OnWeaponJammed;

    // Advanced attachment system delegates
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttachmentChanged, EAttachmentType, AttachmentType, UWeaponAttachment*, Attachment, bool, bAttached);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAttachmentChanged OnAttachmentChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttachmentCompatibilityChecked, UWeaponAttachment*, Attachment, const FWeaponCompatibilityCheck&, CompatibilityResult);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAttachmentCompatibilityChecked OnAttachmentCompatibilityChecked;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttachmentConflictDetected, UWeaponAttachment*, NewAttachment, UWeaponAttachment*, ExistingAttachment, const FAttachmentConflictInfo&, ConflictInfo);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAttachmentConflictDetected OnAttachmentConflictDetected;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponStatsPreviewUpdated, const FWeaponStatsPreview&, Preview);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWeaponStatsPreviewUpdated OnWeaponStatsPreviewUpdated;

    // Advanced attachment system functions
    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    FWeaponStats GetBaseWeaponStats() const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    FWeaponStats GetCurrentWeaponStats() const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    FString GetWeaponName() const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    FString GetCurrentEnvironment() const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    bool AttachAccessoryAdvanced(EAttachmentType Type, UWeaponAttachment* Attachment);

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    FWeaponStatsPreview PreviewAttachmentStats(EAttachmentType Type, UWeaponAttachment* Attachment) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    TArray<FSmartAttachmentSuggestion> GetAttachmentSuggestions(const TArray<UWeaponAttachment*>& AvailableAttachments) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    void UpdateAttachmentPreview(EAttachmentType Type, UWeaponAttachment* Attachment);

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    bool ValidateAttachmentCompatibility(EAttachmentType Type, UWeaponAttachment* Attachment) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    TArray<UWeaponAttachment*> GetConflictingAttachments(UWeaponAttachment* Attachment) const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    void OptimizeAttachmentConfiguration();

    UFUNCTION(BlueprintCallable, Category = "Advanced Attachments")
    float CalculateWeaponPerformanceScore(const FWeaponStats& Stats) const;

    // Getters
    UFUNCTION(BlueprintPure, Category = "Weapon")
    float GetCurrentAccuracy() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    float GetCurrentRecoil() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    float GetEffectiveRange() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    FVector GetMuzzleLocation() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    FRotator GetMuzzleRotation() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool IsAutomatic() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    float GetTimeBetweenShots() const;

    UFUNCTION(BlueprintPure, Category = "Components")
    USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

private:
    // Internal state
    bool bIsFiring = false;
    bool bIsReloading = false;
    float LastFireTime = 0.0f;
    float ReloadStartTime = 0.0f;
    int32 CurrentBurstCount = 0;
    int32 BurstShotsFired = 0;
    
    // Recoil state
    FVector2D CurrentRecoil = FVector2D::ZeroVector;
    FVector2D RecoilVelocity = FVector2D::ZeroVector;
    
    // Timers
    FTimerHandle FireTimerHandle;
    FTimerHandle ReloadTimerHandle;
    FTimerHandle RecoilRecoveryTimerHandle;

    // System references
    UPROPERTY()
    APerformanceOptimizationSystem* PerformanceSystem;

    UPROPERTY()
    AAdvancedHUDSystem* HUDSystem;

    UPROPERTY()
    AAdvancedAudioSystem* AudioSystem;

    // Internal functions
    void FireSingle();
    void FireBurst();
    void FireAuto();
    void ProcessFireInput();
    void HandleRecoil();
    void UpdateRecoilRecovery(float DeltaTime);
    void PlayFireEffects();
    void PlayReloadEffects();
    void SpawnTracer(const FVector& StartLocation, const FVector& EndLocation);
    void PerformLineTrace(FHitResult& HitResult);
    void ApplyDamage(const FHitResult& HitResult);
    float CalculateCurrentAccuracy() const;
    FVector CalculateSpread() const;
    void CheckForJam();
    void InitializeFromDataTable();
    void LoadWeaponAssets();

    // Network replication functions
    UFUNCTION()
    void OnRep_CurrentFireMode();

    UFUNCTION()
    void OnRep_WeaponState();

    UFUNCTION()
    void OnRep_CurrentAmmo();
};
