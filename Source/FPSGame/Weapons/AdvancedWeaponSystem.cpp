#include "AdvancedWeaponSystem.h"
#include "WeaponAttachment.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Camera/CameraShakeBase.h"

DEFINE_LOG_CATEGORY(LogAdvancedWeapon);

UAdvancedWeaponSystem::UAdvancedWeaponSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    
    // Initialize default values
    CurrentFireMode = EFireMode::Semi;
    CurrentAmmoInMag = 30;
    TotalAmmo = 300;
    MagazineCapacity = 30;
    FireRate = 600.0f;
    ReloadTime = 2.5f;
    
    // Ballistics defaults
    MuzzleVelocity = 800.0f;
    BulletMass = 0.004f;
    BulletDrag = 0.47f;
    GravityMultiplier = 1.0f;
    
    // Accuracy defaults
    BaseAccuracy = 0.95f;
    MovementAccuracyPenalty = 0.3f;
    RecoilRecoveryRate = 2.0f;
    
    // Durability defaults
    MaxDurability = 100.0f;
    CurrentDurability = MaxDurability;
    DurabilityDecayRate = 0.1f;
    
    bIsReloading = false;
    bCanFire = true;
    LastFireTime = 0.0f;
    CurrentRecoilPattern = 0;
    
    // Create weapon mesh component
    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    
    // Create muzzle flash component
    MuzzleFlash = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("MuzzleFlash"));
    MuzzleFlash->SetupAttachment(WeaponMesh, TEXT("MuzzleSocket"));
    MuzzleFlash->bAutoActivate = false;
    
    // Create audio component
    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
    AudioComponent->SetupAttachment(WeaponMesh);
}

void UAdvancedWeaponSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize weapon data if available
    if (WeaponData)
    {
        InitializeFromWeaponData();
    }
    
    // Set up timers
    GetWorld()->GetTimerManager().SetTimer(
        DurabilityTimer,
        this,
        &UAdvancedWeaponSystem::UpdateDurability,
        1.0f,
        true
    );
}

void UAdvancedWeaponSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Update recoil recovery
    if (CurrentRecoil.Size() > 0.0f)
    {
        CurrentRecoil = FMath::VInterpTo(CurrentRecoil, FVector::ZeroVector, DeltaTime, RecoilRecoveryRate);
    }
    
    // Update weapon sway
    UpdateWeaponSway(DeltaTime);
    
    // Update attachment effects
    for (auto& Attachment : CurrentAttachments)
    {
        if (Attachment.Value && Attachment.Value->bRequiresUpdate)
        {
            Attachment.Value->UpdateAttachment(DeltaTime);
        }
    }
}

void UAdvancedWeaponSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UAdvancedWeaponSystem, CurrentAmmoInMag);
    DOREPLIFETIME(UAdvancedWeaponSystem, TotalAmmo);
    DOREPLIFETIME(UAdvancedWeaponSystem, CurrentFireMode);
    DOREPLIFETIME(UAdvancedWeaponSystem, bIsReloading);
    DOREPLIFETIME(UAdvancedWeaponSystem, CurrentDurability);
    DOREPLIFETIME(UAdvancedWeaponSystem, CurrentAttachments);
}

bool UAdvancedWeaponSystem::CanFire() const
{
    return bCanFire && 
           !bIsReloading && 
           CurrentAmmoInMag > 0 && 
           CurrentDurability > 0.0f &&
           GetWorld()->GetTimeSeconds() - LastFireTime >= GetFireDelay();
}

void UAdvancedWeaponSystem::Fire()
{
    if (!CanFire())
    {
        // Try to reload if out of ammo
        if (CurrentAmmoInMag <= 0 && TotalAmmo > 0)
        {
            StartReload();
        }
        return;
    }
    
    // Server-side firing
    if (GetOwner()->HasAuthority())
    {
        PerformFire();
    }
    else
    {
        // Client prediction
        PerformFire();
        ServerFire();
    }
}

void UAdvancedWeaponSystem::ServerFire_Implementation()
{
    if (CanFire())
    {
        PerformFire();
    }
}

bool UAdvancedWeaponSystem::ServerFire_Validate()
{
    return true;
}

void UAdvancedWeaponSystem::PerformFire()
{
    LastFireTime = GetWorld()->GetTimeSeconds();
    CurrentAmmoInMag--;
    
    // Calculate projectile trajectory with ballistics
    FVector StartLocation = GetMuzzleLocation();
    FVector FireDirection = CalculateFireDirection();
    
    // Apply recoil
    ApplyRecoil();
    
    // Spawn projectile or perform hitscan
    if (bUseProjectiles)
    {
        SpawnProjectile(StartLocation, FireDirection);
    }
    else
    {
        PerformHitscan(StartLocation, FireDirection);
    }
    
    // Play effects
    PlayFireEffects();
    
    // Update durability
    CurrentDurability = FMath::Max(0.0f, CurrentDurability - DurabilityDecayRate);
    
    // Multicast effects to all clients
    if (GetOwner()->HasAuthority())
    {
        MulticastFireEffects();
    }
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Weapon fired. Ammo: %d/%d, Durability: %.1f"), 
           CurrentAmmoInMag, TotalAmmo, CurrentDurability);
}

void UAdvancedWeaponSystem::MulticastFireEffects_Implementation()
{
    PlayFireEffects();
}

FVector UAdvancedWeaponSystem::CalculateFireDirection()
{
    FVector BaseDirection = GetOwner()->GetActorForwardVector();
    
    // Get current accuracy
    float CurrentAccuracy = CalculateCurrentAccuracy();
    
    // Apply spread based on accuracy
    float SpreadAngle = (1.0f - CurrentAccuracy) * MaxSpreadAngle;
    
    // Random spread within cone
    FVector SpreadDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(BaseDirection, SpreadAngle);
    
    return SpreadDirection;
}

float UAdvancedWeaponSystem::CalculateCurrentAccuracy()
{
    float Accuracy = BaseAccuracy;
    
    // Check if owner is moving
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        if (Character->GetVelocity().Size() > 100.0f)
        {
            Accuracy *= (1.0f - MovementAccuracyPenalty);
        }
    }
    
    // Apply recoil penalty
    float RecoilPenalty = CurrentRecoil.Size() / 100.0f;
    Accuracy *= (1.0f - FMath::Clamp(RecoilPenalty, 0.0f, 0.8f));
    
    // Apply durability penalty
    float DurabilityFactor = CurrentDurability / MaxDurability;
    Accuracy *= DurabilityFactor;
    
    // Apply attachment bonuses
    for (const auto& Attachment : CurrentAttachments)
    {
        if (Attachment.Value)
        {
            Accuracy += Attachment.Value->AccuracyBonus;
        }
    }
    
    return FMath::Clamp(Accuracy, 0.1f, 1.0f);
}

void UAdvancedWeaponSystem::ApplyRecoil()
{
    if (RecoilPatterns.Num() == 0) return;
    
    // Get current recoil pattern
    int32 PatternIndex = CurrentRecoilPattern % RecoilPatterns.Num();
    FVector2D RecoilOffset = RecoilPatterns[PatternIndex];
    
    // Apply recoil to current recoil accumulation
    CurrentRecoil.X += RecoilOffset.X;
    CurrentRecoil.Y += RecoilOffset.Y;
    
    // Increment pattern index
    CurrentRecoilPattern++;
    
    // Apply recoil to camera if this is the local player
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
    {
        if (PC->IsLocalController())
        {
            PC->AddPitchInput(-RecoilOffset.Y * RecoilMultiplier);
            PC->AddYawInput(RecoilOffset.X * RecoilMultiplier);
        }
    }
}

void UAdvancedWeaponSystem::SpawnProjectile(const FVector& StartLocation, const FVector& Direction)
{
    if (!ProjectileClass) return;
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    SpawnParams.Instigator = Cast<APawn>(GetOwner());
    
    FRotator SpawnRotation = Direction.Rotation();
    
    if (AActor* Projectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, StartLocation, SpawnRotation, SpawnParams))
    {
        // Set initial velocity and ballistic properties
        if (UProjectileMovementComponent* ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>())
        {
            ProjectileMovement->InitialSpeed = MuzzleVelocity;
            ProjectileMovement->MaxSpeed = MuzzleVelocity;
            ProjectileMovement->ProjectileGravityScale = GravityMultiplier;
        }
    }
}

void UAdvancedWeaponSystem::PerformHitscan(const FVector& StartLocation, const FVector& Direction)
{
    float Range = GetEffectiveRange();
    FVector EndLocation = StartLocation + (Direction * Range);
    
    FHitResult Hit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.bTraceComplex = true;
    
    if (GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, QueryParams))
    {
        // Process hit
        if (Hit.GetActor())
        {
            float Damage = CalculateDamage(Hit);
            ApplyDamage(Hit.GetActor(), Damage, Hit);
            
            // Spawn impact effects
            SpawnImpactEffects(Hit);
        }
    }
}

float UAdvancedWeaponSystem::CalculateDamage(const FHitResult& Hit)
{
    float Damage = BaseDamage;
    
    // Distance falloff
    float Distance = FVector::Dist(GetMuzzleLocation(), Hit.Location);
    float DistanceFactor = FMath::Clamp(1.0f - (Distance / GetEffectiveRange()), 0.1f, 1.0f);
    Damage *= DistanceFactor;
    
    // Durability affects damage
    float DurabilityFactor = CurrentDurability / MaxDurability;
    Damage *= DurabilityFactor;
    
    // Check for headshot
    if (Hit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase))
    {
        Damage *= HeadshotMultiplier;
    }
    
    return Damage;
}

void UAdvancedWeaponSystem::StartReload()
{
    if (bIsReloading || TotalAmmo <= 0 || CurrentAmmoInMag >= MagazineCapacity)
    {
        return;
    }
    
    bIsReloading = true;
    
    // Play reload animation and sound
    PlayReloadEffects();
    
    // Set reload timer
    float ActualReloadTime = GetModifiedReloadTime();
    GetWorld()->GetTimerManager().SetTimer(
        ReloadTimer,
        this,
        &UAdvancedWeaponSystem::CompleteReload,
        ActualReloadTime,
        false
    );
    
    if (GetOwner()->HasAuthority())
    {
        MulticastReloadEffects();
    }
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Reload started. Time: %.1f seconds"), ActualReloadTime);
}

void UAdvancedWeaponSystem::CompleteReload()
{
    if (!bIsReloading) return;
    
    int32 AmmoNeeded = MagazineCapacity - CurrentAmmoInMag;
    int32 AmmoToReload = FMath::Min(AmmoNeeded, TotalAmmo);
    
    CurrentAmmoInMag += AmmoToReload;
    TotalAmmo -= AmmoToReload;
    
    bIsReloading = false;
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Reload completed. Ammo: %d/%d"), CurrentAmmoInMag, TotalAmmo);
}

void UAdvancedWeaponSystem::MulticastReloadEffects_Implementation()
{
    PlayReloadEffects();
}

bool UAdvancedWeaponSystem::AttachAccessory(EAttachmentType Type, UWeaponAttachment* Attachment)
{
    if (!Attachment || !CanAttachAccessory(Type, Attachment))
    {
        return false;
    }
    
    // Remove existing attachment of this type
    if (CurrentAttachments.Contains(Type))
    {
        DetachAccessory(Type);
    }
    
    // Attach new accessory
    CurrentAttachments.Add(Type, Attachment);
    Attachment->AttachToWeapon(this);
    
    // Apply attachment modifications
    ApplyAttachmentModifications();
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Attached accessory: %s"), *Attachment->GetName());
    return true;
}

bool UAdvancedWeaponSystem::DetachAccessory(EAttachmentType Type)
{
    if (!CurrentAttachments.Contains(Type))
    {
        return false;
    }
    
    UWeaponAttachment* Attachment = CurrentAttachments[Type];
    if (Attachment)
    {
        Attachment->DetachFromWeapon();
        CurrentAttachments.Remove(Type);
        
        // Reapply all attachment modifications
        ApplyAttachmentModifications();
        
        UE_LOG(LogAdvancedWeapon, Log, TEXT("Detached accessory: %s"), *Attachment->GetName());
        return true;
    }
    
    return false;
}

void UAdvancedWeaponSystem::ApplyAttachmentModifications()
{
    // Reset to base values
    float ModifiedFireRate = FireRate;
    float ModifiedReloadTime = ReloadTime;
    float ModifiedAccuracy = BaseAccuracy;
    float ModifiedRange = EffectiveRange;
    
    // Apply all attachment modifications
    for (const auto& Attachment : CurrentAttachments)
    {
        if (Attachment.Value)
        {
            ModifiedFireRate *= Attachment.Value->FireRateMultiplier;
            ModifiedReloadTime *= Attachment.Value->ReloadTimeMultiplier;
            ModifiedAccuracy += Attachment.Value->AccuracyBonus;
            ModifiedRange *= Attachment.Value->RangeMultiplier;
        }
    }
    
    // Apply modifications
    // Note: In a real implementation, you'd update the actual weapon properties
}

void UAdvancedWeaponSystem::CycleFireMode()
{
    TArray<EFireMode> AvailableModes = GetAvailableFireModes();
    if (AvailableModes.Num() <= 1) return;
    
    int32 CurrentIndex = AvailableModes.Find(CurrentFireMode);
    CurrentIndex = (CurrentIndex + 1) % AvailableModes.Num();
    CurrentFireMode = AvailableModes[CurrentIndex];
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Fire mode changed to: %s"), 
           *UEnum::GetValueAsString(CurrentFireMode));
}

TArray<EFireMode> UAdvancedWeaponSystem::GetAvailableFireModes() const
{
    TArray<EFireMode> Modes;
    
    if (WeaponData)
    {
        return WeaponData->SupportedFireModes;
    }
    
    // Default modes
    Modes.Add(EFireMode::Semi);
    Modes.Add(EFireMode::Auto);
    
    return Modes;
}

float UAdvancedWeaponSystem::GetFireDelay() const
{
    float BaseDelay = 60.0f / FireRate; // Convert RPM to seconds
    
    // Apply attachment modifications
    for (const auto& Attachment : CurrentAttachments)
    {
        if (Attachment.Value)
        {
            BaseDelay *= Attachment.Value->FireRateMultiplier;
        }
    }
    
    return BaseDelay;
}

float UAdvancedWeaponSystem::GetModifiedReloadTime() const
{
    float ModifiedTime = ReloadTime;
    
    // Apply attachment modifications
    for (const auto& Attachment : CurrentAttachments)
    {
        if (Attachment.Value)
        {
            ModifiedTime *= Attachment.Value->ReloadTimeMultiplier;
        }
    }
    
    return ModifiedTime;
}

float UAdvancedWeaponSystem::GetEffectiveRange() const
{
    float Range = EffectiveRange;
    
    // Apply attachment modifications
    for (const auto& Attachment : CurrentAttachments)
    {
        if (Attachment.Value)
        {
            Range *= Attachment.Value->RangeMultiplier;
        }
    }
    
    return Range;
}

FVector UAdvancedWeaponSystem::GetMuzzleLocation() const
{
    if (WeaponMesh)
    {
        return WeaponMesh->GetSocketLocation(TEXT("MuzzleSocket"));
    }
    
    return GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 100.0f;
}

void UAdvancedWeaponSystem::PlayFireEffects()
{
    // Play muzzle flash
    if (MuzzleFlash)
    {
        MuzzleFlash->ActivateSystem();
    }
    
    // Play fire sound
    if (FireSound && AudioComponent)
    {
        AudioComponent->SetSound(FireSound);
        AudioComponent->Play();
    }
    
    // Play fire animation
    if (FireAnimation && WeaponMesh && WeaponMesh->GetAnimInstance())
    {
        WeaponMesh->GetAnimInstance()->Montage_Play(FireAnimation);
    }
    
    // Apply camera shake
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
    {
        if (PC->IsLocalController() && FireCameraShake)
        {
            PC->ClientStartCameraShake(FireCameraShake);
        }
    }
}

void UAdvancedWeaponSystem::PlayReloadEffects()
{
    // Play reload sound
    if (ReloadSound && AudioComponent)
    {
        AudioComponent->SetSound(ReloadSound);
        AudioComponent->Play();
    }
    
    // Play reload animation
    if (ReloadAnimation && WeaponMesh && WeaponMesh->GetAnimInstance())
    {
        WeaponMesh->GetAnimInstance()->Montage_Play(ReloadAnimation);
    }
}

void UAdvancedWeaponSystem::SpawnImpactEffects(const FHitResult& Hit)
{
    // Spawn impact particle effect
    if (ImpactEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            ImpactEffect,
            Hit.Location,
            Hit.Normal.Rotation()
        );
    }
    
    // Play impact sound
    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            ImpactSound,
            Hit.Location
        );
    }
}

void UAdvancedWeaponSystem::UpdateWeaponSway(float DeltaTime)
{
    // Simple weapon sway based on movement
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        FVector Velocity = Character->GetVelocity();
        float Speed = Velocity.Size();
        
        if (Speed > 0.0f)
        {
            float SwayAmount = Speed / 600.0f; // Normalize to max run speed
            FVector SwayOffset = FVector(
                FMath::Sin(GetWorld()->GetTimeSeconds() * 2.0f) * SwayAmount,
                FMath::Cos(GetWorld()->GetTimeSeconds() * 1.5f) * SwayAmount,
                0.0f
            );
            
            // Apply sway to weapon mesh
            if (WeaponMesh)
            {
                FVector CurrentLocation = WeaponMesh->GetRelativeLocation();
                FVector TargetLocation = CurrentLocation + SwayOffset;
                WeaponMesh->SetRelativeLocation(
                    FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 2.0f)
                );
            }
        }
    }
}

void UAdvancedWeaponSystem::UpdateDurability()
{
    // Passive durability decay
    if (CurrentDurability > 0.0f)
    {
        CurrentDurability = FMath::Max(0.0f, CurrentDurability - 0.01f);
        
        if (CurrentDurability <= 0.0f)
        {
            UE_LOG(LogAdvancedWeapon, Warning, TEXT("Weapon durability depleted!"));
        }
    }
}

void UAdvancedWeaponSystem::InitializeFromWeaponData()
{
    if (!WeaponData) return;
    
    // Copy values from weapon data
    MagazineCapacity = WeaponData->MagazineCapacity;
    FireRate = WeaponData->FireRate;
    BaseDamage = WeaponData->BaseDamage;
    EffectiveRange = WeaponData->EffectiveRange;
    ReloadTime = WeaponData->ReloadTime;
    MuzzleVelocity = WeaponData->MuzzleVelocity;
    BaseAccuracy = WeaponData->BaseAccuracy;
    RecoilPatterns = WeaponData->RecoilPattern;
    
    // Set initial ammo
    CurrentAmmoInMag = MagazineCapacity;
    TotalAmmo = WeaponData->InitialAmmo;
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Weapon initialized from data: %s"), *WeaponData->GetName());
}

bool UAdvancedWeaponSystem::CanAttachAccessory(EAttachmentType Type, UWeaponAttachment* Attachment) const
{
    if (!Attachment) return false;
    
    // Check if weapon supports this attachment type
    if (WeaponData && !WeaponData->SupportedAttachmentTypes.Contains(Type))
    {
        return false;
    }
    
    return true;
}

void UAdvancedWeaponSystem::ApplyDamage(AActor* Target, float Damage, const FHitResult& Hit)
{
    if (!Target) return;
    
    // Apply damage using Unreal's damage system
    UGameplayStatics::ApplyDamage(
        Target,
        Damage,
        GetOwner()->GetInstigatorController(),
        GetOwner(),
        UDamageType::StaticClass()
    );
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Applied %.1f damage to %s"), Damage, *Target->GetName());
}

// Add advanced attachment system integration functions

FWeaponStats UAdvancedWeaponSystem::GetBaseWeaponStats() const
{
    return WeaponStats; // Return the base weapon stats without any modifications
}

FWeaponStats UAdvancedWeaponSystem::GetCurrentWeaponStats() const
{
    // Calculate current stats with all attachments applied
    FWeaponStats CurrentStats = WeaponStats;
    
    for (const auto& AttachmentPair : CurrentAttachments)
    {
        if (UWeaponAttachment* Attachment = AttachmentPair.Value)
        {
            Attachment->ApplyModifiersToStats(CurrentStats);
        }
    }
    
    return CurrentStats;
}

FString UAdvancedWeaponSystem::GetWeaponName() const
{
    if (WeaponData)
    {
        return WeaponData->WeaponName;
    }
    return "Unknown Weapon";
}

FString UAdvancedWeaponSystem::GetCurrentEnvironment() const
{
    // In a real implementation, this would check the current map/environment
    // For now, return a default environment
    return "Urban";
}

bool UAdvancedWeaponSystem::AttachAccessoryAdvanced(EAttachmentType Type, UWeaponAttachment* Attachment)
{
    if (!Attachment) return false;
    
    // Get existing attachments for compatibility checking
    TArray<UWeaponAttachment*> ExistingAttachments;
    for (const auto& AttachmentPair : CurrentAttachments)
    {
        if (AttachmentPair.Value)
        {
            ExistingAttachments.Add(AttachmentPair.Value);
        }
    }
    
    // Perform advanced compatibility check
    FWeaponCompatibilityCheck CompatibilityResult = Attachment->CheckCompatibilityAdvanced(this, ExistingAttachments);
    
    if (!CompatibilityResult.bIsCompatible)
    {
        UE_LOG(LogAdvancedWeapon, Warning, TEXT("Cannot attach %s: %s"), 
               *Attachment->AttachmentName, *CompatibilityResult.ReasonIfIncompatible);
        
        // Broadcast compatibility event for UI
        OnAttachmentCompatibilityChecked.Broadcast(Attachment, CompatibilityResult);
        return false;
    }
    
    // Handle conflicts with existing attachments
    if (CurrentAttachments.Contains(Type))
    {
        UWeaponAttachment* ExistingAttachment = CurrentAttachments[Type];
        FAttachmentConflictInfo ConflictInfo = Attachment->ResolveConflict(this, ExistingAttachment);
        
        if (ConflictInfo.bHasConflict)
        {
            // Broadcast conflict event for UI to handle
            OnAttachmentConflictDetected.Broadcast(Attachment, ExistingAttachment, ConflictInfo);
            
            // Auto-resolve based on recommendation
            if (ConflictInfo.RecommendedResolution == EConflictResolution::KeepExisting)
            {
                return false;
            }
            else if (ConflictInfo.RecommendedResolution == EConflictResolution::ReplaceExisting)
            {
                DetachAccessory(Type);
            }
        }
    }
    
    // Attach the accessory
    CurrentAttachments.Add(Type, Attachment);
    
    // Apply attachment modifications
    ApplyAttachmentModifications();
    
    // Broadcast attachment event
    OnAttachmentChanged.Broadcast(Type, Attachment, true);
    
    UE_LOG(LogAdvancedWeapon, Log, TEXT("Successfully attached %s (Compatibility Score: %.2f)"), 
           *Attachment->AttachmentName, CompatibilityResult.CompatibilityScore);
    
    return true;
}

FWeaponStatsPreview UAdvancedWeaponSystem::PreviewAttachmentStats(EAttachmentType Type, UWeaponAttachment* Attachment) const
{
    if (!Attachment)
    {
        return FWeaponStatsPreview();
    }
    
    // Get existing attachments excluding the one being previewed
    TArray<UWeaponAttachment*> ExistingAttachments;
    for (const auto& AttachmentPair : CurrentAttachments)
    {
        if (AttachmentPair.Value && AttachmentPair.Key != Type)
        {
            ExistingAttachments.Add(AttachmentPair.Value);
        }
    }
    
    return Attachment->CalculateStatsPreview(const_cast<UAdvancedWeaponSystem*>(this), ExistingAttachments);
}

TArray<FSmartAttachmentSuggestion> UAdvancedWeaponSystem::GetAttachmentSuggestions(const TArray<UWeaponAttachment*>& AvailableAttachments) const
{
    TArray<FSmartAttachmentSuggestion> AllSuggestions;
    
    // Get suggestions for each attachment type
    for (const UWeaponAttachment* AvailableAttachment : AvailableAttachments)
    {
        if (!AvailableAttachment) continue;
        
        // Skip if already attached
        bool bAlreadyAttached = false;
        for (const auto& AttachmentPair : CurrentAttachments)
        {
            if (AttachmentPair.Value == AvailableAttachment)
            {
                bAlreadyAttached = true;
                break;
            }
        }
        
        if (bAlreadyAttached) continue;
        
        // Get suggestions for this attachment
        TArray<FSmartAttachmentSuggestion> AttachmentSuggestions = 
            AvailableAttachment->GetSmartSuggestions(const_cast<UAdvancedWeaponSystem*>(this), AvailableAttachments);
        
        AllSuggestions.Append(AttachmentSuggestions);
    }
    
    // Remove duplicates and sort by priority
    TMap<UWeaponAttachment*, FSmartAttachmentSuggestion> UniqueSuggestions;
    for (const FSmartAttachmentSuggestion& Suggestion : AllSuggestions)
    {
        if (!UniqueSuggestions.Contains(Suggestion.SuggestedAttachment) ||
            UniqueSuggestions[Suggestion.SuggestedAttachment].CompatibilityScore < Suggestion.CompatibilityScore)
        {
            UniqueSuggestions.Add(Suggestion.SuggestedAttachment, Suggestion);
        }
    }
    
    TArray<FSmartAttachmentSuggestion> FinalSuggestions;
    UniqueSuggestions.GenerateValueArray(FinalSuggestions);
    
    // Sort by priority and compatibility score
    FinalSuggestions.Sort([](const FSmartAttachmentSuggestion& A, const FSmartAttachmentSuggestion& B) {
        if (A.Priority != B.Priority)
            return (int32)A.Priority > (int32)B.Priority;
        return A.CompatibilityScore > B.CompatibilityScore;
    });
    
    // Limit to top 5 suggestions
    if (FinalSuggestions.Num() > 5)
    {
        FinalSuggestions.SetNum(5);
    }
    
    return FinalSuggestions;
}

void UAdvancedWeaponSystem::UpdateAttachmentPreview(EAttachmentType Type, UWeaponAttachment* Attachment)
{
    if (!Attachment) return;
    
    // Calculate real-time preview
    FWeaponStatsPreview Preview = PreviewAttachmentStats(Type, Attachment);
    
    // Broadcast preview update for UI
    OnWeaponStatsPreviewUpdated.Broadcast(Preview);
    
    // Log preview for debugging
    UE_LOG(LogAdvancedWeapon, VeryVerbose, TEXT("Preview Stats for %s: Damage %.1f->%.1f, Accuracy %.2f->%.2f"), 
           *Attachment->AttachmentName, 
           Preview.CurrentStats.BaseDamage, Preview.PreviewStats.BaseDamage,
           Preview.CurrentStats.BaseAccuracy, Preview.PreviewStats.BaseAccuracy);
}

bool UAdvancedWeaponSystem::ValidateAttachmentCompatibility(EAttachmentType Type, UWeaponAttachment* Attachment) const
{
    if (!Attachment) return false;
    
    // Get existing attachments
    TArray<UWeaponAttachment*> ExistingAttachments;
    for (const auto& AttachmentPair : CurrentAttachments)
    {
        if (AttachmentPair.Value && AttachmentPair.Key != Type) // Exclude same type to allow replacement
        {
            ExistingAttachments.Add(AttachmentPair.Value);
        }
    }
    
    // Check advanced compatibility
    FWeaponCompatibilityCheck CompatibilityResult = Attachment->CheckCompatibilityAdvanced(const_cast<UAdvancedWeaponSystem*>(this), ExistingAttachments);
    
    return CompatibilityResult.bIsCompatible && CompatibilityResult.CompatibilityScore >= 0.5f;
}

TArray<UWeaponAttachment*> UAdvancedWeaponSystem::GetConflictingAttachments(UWeaponAttachment* Attachment) const
{
    TArray<UWeaponAttachment*> ConflictingAttachments;
    
    if (!Attachment) return ConflictingAttachments;
    
    for (const auto& AttachmentPair : CurrentAttachments)
    {
        if (UWeaponAttachment* ExistingAttachment = AttachmentPair.Value)
        {
            FAttachmentConflictInfo ConflictInfo = Attachment->ResolveConflict(const_cast<UAdvancedWeaponSystem*>(this), ExistingAttachment);
            if (ConflictInfo.bHasConflict)
            {
                ConflictingAttachments.Add(ExistingAttachment);
            }
        }
    }
    
    return ConflictingAttachments;
}

void UAdvancedWeaponSystem::OptimizeAttachmentConfiguration()
{
    // Get all currently attached attachments
    TArray<UWeaponAttachment*> CurrentAttachmentList;
    for (const auto& AttachmentPair : CurrentAttachments)
    {
        if (AttachmentPair.Value)
        {
            CurrentAttachmentList.Add(AttachmentPair.Value);
        }
    }
    
    // Calculate current overall performance score
    FWeaponStats CurrentStats = GetCurrentWeaponStats();
    float CurrentScore = CalculateWeaponPerformanceScore(CurrentStats);
    
    // Try different attachment combinations to find optimal configuration
    // This is a simplified optimization - in practice, you might want a more sophisticated algorithm
    TArray<EAttachmentType> AttachmentTypes;
    CurrentAttachments.GetKeys(AttachmentTypes);
    
    for (EAttachmentType Type : AttachmentTypes)
    {
        UWeaponAttachment* CurrentAttachment = CurrentAttachments[Type];
        if (!CurrentAttachment) continue;
        
        // Try removing this attachment and see if it improves overall score
        CurrentAttachments.Remove(Type);
        
        FWeaponStats TestStats = GetCurrentWeaponStats();
        float TestScore = CalculateWeaponPerformanceScore(TestStats);
        
        if (TestScore > CurrentScore * 1.05f) // 5% improvement threshold
        {
            UE_LOG(LogAdvancedWeapon, Log, TEXT("Optimization: Removing %s improves performance from %.2f to %.2f"), 
                   *CurrentAttachment->AttachmentName, CurrentScore, TestScore);
            
            // Keep the attachment removed
            OnAttachmentChanged.Broadcast(Type, nullptr, false);
        }
        else
        {
            // Re-add the attachment
            CurrentAttachments.Add(Type, CurrentAttachment);
        }
    }
    
    ApplyAttachmentModifications();
}

float UAdvancedWeaponSystem::CalculateWeaponPerformanceScore(const FWeaponStats& Stats) const
{
    // Calculate a weighted performance score
    float Score = 0.0f;
    
    // Damage contribution (25%)
    Score += (Stats.BaseDamage / 100.0f) * 25.0f;
    
    // Accuracy contribution (20%)
    Score += Stats.BaseAccuracy * 20.0f;
    
    // Fire rate contribution (15%)
    Score += (Stats.FireRate / 1000.0f) * 15.0f;
    
    // Range contribution (15%)
    Score += (Stats.EffectiveRange / 1000.0f) * 15.0f;
    
    // Reload speed contribution (10%)
    Score += (5.0f - FMath::Min(Stats.ReloadTime, 5.0f)) * 2.0f; // Inverted - faster reload is better
    
    // Recoil control contribution (15%)
    Score += (2.0f - FMath::Min(Stats.VerticalRecoil, 2.0f)) * 7.5f; // Inverted - less recoil is better
    Score += (2.0f - FMath::Min(Stats.HorizontalRecoil, 2.0f)) * 7.5f;
    
    return FMath::Max(Score, 0.0f);
}
