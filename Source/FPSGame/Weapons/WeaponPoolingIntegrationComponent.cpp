#include "WeaponPoolingIntegrationComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DecalActor.h"

DEFINE_LOG_CATEGORY(LogWeaponPoolingIntegration);

UWeaponPoolingIntegrationComponent::UWeaponPoolingIntegrationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f; // Check every second for cleanup
}

void UWeaponPoolingIntegrationComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Find object pool manager
    ObjectPoolManager = UAdvancedObjectPoolManager::GetInstance(GetWorld());
    if (!ObjectPoolManager)
    {
        UE_LOG(LogWeaponPoolingIntegration, Error, TEXT("Failed to find AdvancedObjectPoolManager"));
        return;
    }
    
    // Find ballistics system
    if (AActor* Owner = GetOwner())
    {
        BallisticsSystem = Owner->FindComponentByClass<UBallisticsSystem>();
        if (!BallisticsSystem)
        {
            UE_LOG(LogWeaponPoolingIntegration, Warning, TEXT("No BallisticsSystem found on owner"));
        }
    }
    
    InitializeWeaponPools();
    PrewarmWeaponPools();
    
    UE_LOG(LogWeaponPoolingIntegration, Log, TEXT("WeaponPoolingIntegrationComponent initialized"));
}

void UWeaponPoolingIntegrationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastCleanupTime >= CleanupInterval)
    {
        CleanupFinishedObjects();
        UpdatePoolingStatistics();
        LastCleanupTime = CurrentTime;
    }
}

AActor* UWeaponPoolingIntegrationComponent::SpawnPooledProjectile(TSubclassOf<AActor> ProjectileClass, const FVector& Location, const FRotator& Rotation, EAmmoType AmmoType)
{
    if (!ObjectPoolManager || !ProjectileClass || !bUsePooledProjectiles)
    {
        return nullptr;
    }
    
    FString PoolName = GetPoolName(TEXT("Projectile"), AmmoType);
    AActor* Projectile = ObjectPoolManager->AcquireActor(ProjectileClass, PoolName);
    
    if (Projectile)
    {
        Projectile->SetActorLocation(Location);
        Projectile->SetActorRotation(Rotation);
        Projectile->SetActorHiddenInGame(false);
        Projectile->SetActorEnableCollision(true);
        
        ActivePooledProjectiles.AddUnique(Projectile);
        ProjectilesSpawned++;
        
        UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Spawned pooled projectile: %s"), 
               *Projectile->GetClass()->GetName());
    }
    else
    {
        UE_LOG(LogWeaponPoolingIntegration, Warning, TEXT("Failed to acquire projectile from pool: %s"), *PoolName);
    }
    
    return Projectile;
}

void UWeaponPoolingIntegrationComponent::ReturnPooledProjectile(AActor* Projectile)
{
    if (!ObjectPoolManager || !Projectile)
    {
        return;
    }
    
    // Hide and disable collision
    Projectile->SetActorHiddenInGame(true);
    Projectile->SetActorEnableCollision(false);
    
    // Remove from tracking
    ActivePooledProjectiles.Remove(Projectile);
    
    // Return to pool
    ObjectPoolManager->ReleaseActor(Projectile);
    
    UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Returned projectile to pool"));
}

UParticleSystemComponent* UWeaponPoolingIntegrationComponent::SpawnPooledMuzzleFlash(UParticleSystem* MuzzleFlashEffect, const FVector& Location, const FRotator& Rotation)
{
    if (!ObjectPoolManager || !MuzzleFlashEffect || !bUsePooledParticleEffects)
    {
        return nullptr;
    }
    
    UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(
        ObjectPoolManager->AcquireObject(UParticleSystemComponent::StaticClass(), TEXT("MuzzleFlashPool"))
    );
    
    if (ParticleComp)
    {
        ParticleComp->SetTemplate(MuzzleFlashEffect);
        ParticleComp->SetWorldLocation(Location);
        ParticleComp->SetWorldRotation(Rotation);
        ParticleComp->Activate(true);
        
        ActivePooledParticleEffects.AddUnique(ParticleComp);
        EffectsSpawned++;
        
        UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Spawned pooled muzzle flash"));
    }
    
    return ParticleComp;
}

UParticleSystemComponent* UWeaponPoolingIntegrationComponent::SpawnPooledShellEject(UParticleSystem* ShellEjectEffect, const FVector& Location, const FRotator& Rotation)
{
    if (!ObjectPoolManager || !ShellEjectEffect || !bUsePooledParticleEffects)
    {
        return nullptr;
    }
    
    UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(
        ObjectPoolManager->AcquireObject(UParticleSystemComponent::StaticClass(), TEXT("ShellEjectPool"))
    );
    
    if (ParticleComp)
    {
        ParticleComp->SetTemplate(ShellEjectEffect);
        ParticleComp->SetWorldLocation(Location);
        ParticleComp->SetWorldRotation(Rotation);
        ParticleComp->Activate(true);
        
        ActivePooledParticleEffects.AddUnique(ParticleComp);
        EffectsSpawned++;
        
        UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Spawned pooled shell eject effect"));
    }
    
    return ParticleComp;
}

UParticleSystemComponent* UWeaponPoolingIntegrationComponent::SpawnPooledImpactEffect(UParticleSystem* ImpactEffect, const FVector& Location, const FRotator& Rotation, ESurfaceType SurfaceType)
{
    if (!ObjectPoolManager || !ImpactEffect || !bUsePooledParticleEffects)
    {
        return nullptr;
    }
    
    FString PoolName = GetPoolName(TEXT("ImpactEffect"), SurfaceType);
    UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(
        ObjectPoolManager->AcquireObject(UParticleSystemComponent::StaticClass(), PoolName)
    );
    
    if (ParticleComp)
    {
        ParticleComp->SetTemplate(ImpactEffect);
        ParticleComp->SetWorldLocation(Location);
        ParticleComp->SetWorldRotation(Rotation);
        ParticleComp->Activate(true);
        
        ActivePooledParticleEffects.AddUnique(ParticleComp);
        EffectsSpawned++;
        
        UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Spawned pooled impact effect for surface: %s"), 
               *UEnum::GetValueAsString(SurfaceType));
    }
    
    return ParticleComp;
}

UParticleSystemComponent* UWeaponPoolingIntegrationComponent::SpawnPooledTracer(UParticleSystem* TracerEffect, const FVector& StartLocation, const FVector& EndLocation)
{
    if (!ObjectPoolManager || !TracerEffect || !bUsePooledParticleEffects)
    {
        return nullptr;
    }
    
    UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(
        ObjectPoolManager->AcquireObject(UParticleSystemComponent::StaticClass(), TEXT("TracerPool"))
    );
    
    if (ParticleComp)
    {
        ParticleComp->SetTemplate(TracerEffect);
        ParticleComp->SetWorldLocation(StartLocation);
        
        // Calculate rotation to point towards end location
        FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
        FRotator TracerRotation = Direction.Rotation();
        ParticleComp->SetWorldRotation(TracerRotation);
        
        ParticleComp->Activate(true);
        
        ActivePooledParticleEffects.AddUnique(ParticleComp);
        EffectsSpawned++;
        
        UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Spawned pooled tracer effect"));
    }
    
    return ParticleComp;
}

void UWeaponPoolingIntegrationComponent::ReturnPooledParticleEffect(UParticleSystemComponent* ParticleEffect)
{
    if (!ObjectPoolManager || !ParticleEffect)
    {
        return;
    }
    
    // Deactivate the particle system
    ParticleEffect->Deactivate();
    ParticleEffect->SetTemplate(nullptr);
    
    // Remove from tracking
    ActivePooledParticleEffects.Remove(ParticleEffect);
    
    // Return to pool
    ObjectPoolManager->ReleaseObject(ParticleEffect);
    
    UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Returned particle effect to pool"));
}

UAudioComponent* UWeaponPoolingIntegrationComponent::SpawnPooledWeaponAudio(USoundCue* WeaponSound, const FVector& Location, bool bSpatialize)
{
    if (!ObjectPoolManager || !WeaponSound || !bUsePooledAudio)
    {
        return nullptr;
    }
    
    UAudioComponent* AudioComp = Cast<UAudioComponent>(
        ObjectPoolManager->AcquireObject(UAudioComponent::StaticClass(), TEXT("WeaponAudioPool"))
    );
    
    if (AudioComp)
    {
        AudioComp->SetSound(WeaponSound);
        AudioComp->SetWorldLocation(Location);
        AudioComp->bSpatialize = bSpatialize;
        AudioComp->Play();
        
        ActivePooledAudioComponents.AddUnique(AudioComp);
        AudioSpawned++;
        
        UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Spawned pooled weapon audio: %s"), 
               *WeaponSound->GetName());
    }
    
    return AudioComp;
}

void UWeaponPoolingIntegrationComponent::ReturnPooledAudioComponent(UAudioComponent* AudioComponent)
{
    if (!ObjectPoolManager || !AudioComponent)
    {
        return;
    }
    
    // Stop audio and clear sound
    AudioComponent->Stop();
    AudioComponent->SetSound(nullptr);
    
    // Remove from tracking
    ActivePooledAudioComponents.Remove(AudioComponent);
    
    // Return to pool
    ObjectPoolManager->ReleaseObject(AudioComponent);
    
    UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Returned audio component to pool"));
}

UDecalComponent* UWeaponPoolingIntegrationComponent::SpawnPooledDecal(UMaterialInterface* DecalMaterial, const FVector& Location, const FRotator& Rotation, const FVector& Size, float Lifespan)
{
    if (!ObjectPoolManager || !DecalMaterial || !bUsePooledDecals)
    {
        return nullptr;
    }
    
    UDecalComponent* DecalComp = Cast<UDecalComponent>(
        ObjectPoolManager->AcquireObject(UDecalComponent::StaticClass(), TEXT("DecalPool"))
    );
    
    if (DecalComp)
    {
        DecalComp->SetDecalMaterial(DecalMaterial);
        DecalComp->SetWorldLocation(Location);
        DecalComp->SetWorldRotation(Rotation);
        DecalComp->DecalSize = Size;
        DecalComp->SetLifeSpan(Lifespan);
        
        ActivePooledDecals.AddUnique(DecalComp);
        DecalsSpawned++;
        
        UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Spawned pooled decal"));
    }
    
    return DecalComp;
}

void UWeaponPoolingIntegrationComponent::ReturnPooledDecal(UDecalComponent* DecalComponent)
{
    if (!ObjectPoolManager || !DecalComponent)
    {
        return;
    }
    
    // Clear decal material and hide
    DecalComponent->SetDecalMaterial(nullptr);
    DecalComponent->SetVisibility(false);
    
    // Remove from tracking
    ActivePooledDecals.Remove(DecalComponent);
    
    // Return to pool
    ObjectPoolManager->ReleaseObject(DecalComponent);
    
    UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Returned decal to pool"));
}

void UWeaponPoolingIntegrationComponent::FirePooledBullet(FVector Origin, FVector Direction, EAmmoType AmmoType, EBulletType BulletType, AActor* Instigator)
{
    if (!BallisticsSystem)
    {
        UE_LOG(LogWeaponPoolingIntegration, Warning, TEXT("No BallisticsSystem available for pooled bullet firing"));
        return;
    }
    
    // Use the ballistics system to fire the bullet
    // The ballistics system should integrate with pooling for tracer effects and impact effects
    BallisticsSystem->FireBullet(Origin, Direction, AmmoType, BulletType, Instigator);
    
    UE_LOG(LogWeaponPoolingIntegration, VeryVerbose, TEXT("Fired pooled bullet: %s %s"), 
           *UEnum::GetValueAsString(AmmoType), *UEnum::GetValueAsString(BulletType));
}

void UWeaponPoolingIntegrationComponent::OptimizeWeaponPooling()
{
    if (!ObjectPoolManager)
    {
        return;
    }
    
    // Clean up finished objects immediately
    CleanupFinishedObjects();
    
    // Optimize pool sizes based on usage patterns
    int32 ProjectileUsage = ActivePooledProjectiles.Num();
    int32 EffectUsage = ActivePooledParticleEffects.Num();
    int32 AudioUsage = ActivePooledAudioComponents.Num();
    int32 DecalUsage = ActivePooledDecals.Num();
    
    // Adjust pool sizes if needed (basic adaptive pooling)
    if (ProjectileUsage > ProjectilePoolSize * 0.8f)
    {
        ObjectPoolManager->ExpandPool(TEXT("ProjectilePool"), 20);
        UE_LOG(LogWeaponPoolingIntegration, Log, TEXT("Expanded projectile pool due to high usage"));
    }
    
    if (EffectUsage > ParticleEffectPoolSize * 0.8f)
    {
        ObjectPoolManager->ExpandPool(TEXT("MuzzleFlashPool"), 10);
        ObjectPoolManager->ExpandPool(TEXT("ImpactEffectPool"), 15);
        UE_LOG(LogWeaponPoolingIntegration, Log, TEXT("Expanded particle effect pools due to high usage"));
    }
    
    UE_LOG(LogWeaponPoolingIntegration, Log, TEXT("Weapon pooling optimization completed"));
}

FString UWeaponPoolingIntegrationComponent::GetWeaponPoolingReport() const
{
    FString Report = FString::Printf(TEXT("=== Weapon Pooling Performance Report ===\n"));
    Report += FString::Printf(TEXT("Active Projectiles: %d/%d\n"), ActivePooledProjectiles.Num(), ProjectilePoolSize);
    Report += FString::Printf(TEXT("Active Particle Effects: %d/%d\n"), ActivePooledParticleEffects.Num(), ParticleEffectPoolSize);
    Report += FString::Printf(TEXT("Active Audio Components: %d/%d\n"), ActivePooledAudioComponents.Num(), AudioComponentPoolSize);
    Report += FString::Printf(TEXT("Active Decals: %d/%d\n"), ActivePooledDecals.Num(), DecalPoolSize);
    Report += FString::Printf(TEXT("\nTotal Spawned This Session:\n"));
    Report += FString::Printf(TEXT("  Projectiles: %d\n"), ProjectilesSpawned);
    Report += FString::Printf(TEXT("  Effects: %d\n"), EffectsSpawned);
    Report += FString::Printf(TEXT("  Audio: %d\n"), AudioSpawned);
    Report += FString::Printf(TEXT("  Decals: %d\n"), DecalsSpawned);
    Report += FString::Printf(TEXT("\nPool Usage Efficiency:\n"));
    Report += FString::Printf(TEXT("  Projectiles: %.1f%%\n"), ActivePooledProjectiles.Num() > 0 ? (float)ActivePooledProjectiles.Num() / ProjectilePoolSize * 100.0f : 0.0f);
    Report += FString::Printf(TEXT("  Effects: %.1f%%\n"), ActivePooledParticleEffects.Num() > 0 ? (float)ActivePooledParticleEffects.Num() / ParticleEffectPoolSize * 100.0f : 0.0f);
    Report += FString::Printf(TEXT("  Audio: %.1f%%\n"), ActivePooledAudioComponents.Num() > 0 ? (float)ActivePooledAudioComponents.Num() / AudioComponentPoolSize * 100.0f : 0.0f);
    Report += FString::Printf(TEXT("  Decals: %.1f%%\n"), ActivePooledDecals.Num() > 0 ? (float)ActivePooledDecals.Num() / DecalPoolSize * 100.0f : 0.0f);
    
    return Report;
}

void UWeaponPoolingIntegrationComponent::PrewarmWeaponPools()
{
    if (!ObjectPoolManager)
    {
        return;
    }
    
    // Prewarm essential weapon effect pools
    ObjectPoolManager->PrewarmPool(TEXT("MuzzleFlashPool"), ParticleEffectPoolSize / 4);
    ObjectPoolManager->PrewarmPool(TEXT("ImpactEffectPool"), ParticleEffectPoolSize / 2);
    ObjectPoolManager->PrewarmPool(TEXT("TracerPool"), ParticleEffectPoolSize / 4);
    ObjectPoolManager->PrewarmPool(TEXT("WeaponAudioPool"), AudioComponentPoolSize / 2);
    ObjectPoolManager->PrewarmPool(TEXT("DecalPool"), DecalPoolSize / 2);
    
    UE_LOG(LogWeaponPoolingIntegration, Log, TEXT("Prewarmed weapon pools for optimal performance"));
}

void UWeaponPoolingIntegrationComponent::InitializeWeaponPools()
{
    if (!ObjectPoolManager)
    {
        return;
    }
    
    // Create projectile pools for different ammo types
    ObjectPoolManager->CreateActorPool(TEXT("ProjectilePool_556"), ProjectilePoolSize, ProjectilePoolSize * 2);
    ObjectPoolManager->CreateActorPool(TEXT("ProjectilePool_762"), ProjectilePoolSize, ProjectilePoolSize * 2);
    ObjectPoolManager->CreateActorPool(TEXT("ProjectilePool_9mm"), ProjectilePoolSize, ProjectilePoolSize * 2);
    
    // Create particle effect pools
    ObjectPoolManager->CreateObjectPool(TEXT("MuzzleFlashPool"), UParticleSystemComponent::StaticClass(), ParticleEffectPoolSize, ParticleEffectPoolSize * 2);
    ObjectPoolManager->CreateObjectPool(TEXT("ShellEjectPool"), UParticleSystemComponent::StaticClass(), ParticleEffectPoolSize / 2, ParticleEffectPoolSize);
    ObjectPoolManager->CreateObjectPool(TEXT("TracerPool"), UParticleSystemComponent::StaticClass(), ParticleEffectPoolSize / 4, ParticleEffectPoolSize / 2);
    
    // Create impact effect pools for different surfaces
    ObjectPoolManager->CreateObjectPool(TEXT("ImpactEffect_Metal"), UParticleSystemComponent::StaticClass(), ParticleEffectPoolSize / 4, ParticleEffectPoolSize / 2);
    ObjectPoolManager->CreateObjectPool(TEXT("ImpactEffect_Concrete"), UParticleSystemComponent::StaticClass(), ParticleEffectPoolSize / 4, ParticleEffectPoolSize / 2);
    ObjectPoolManager->CreateObjectPool(TEXT("ImpactEffect_Wood"), UParticleSystemComponent::StaticClass(), ParticleEffectPoolSize / 4, ParticleEffectPoolSize / 2);
    ObjectPoolManager->CreateObjectPool(TEXT("ImpactEffect_Flesh"), UParticleSystemComponent::StaticClass(), ParticleEffectPoolSize / 6, ParticleEffectPoolSize / 3);
    
    // Create audio pools
    ObjectPoolManager->CreateObjectPool(TEXT("WeaponAudioPool"), UAudioComponent::StaticClass(), AudioComponentPoolSize, AudioComponentPoolSize * 2);
    
    // Create decal pools
    ObjectPoolManager->CreateObjectPool(TEXT("DecalPool"), UDecalComponent::StaticClass(), DecalPoolSize, DecalPoolSize * 2);
    
    UE_LOG(LogWeaponPoolingIntegration, Log, TEXT("Initialized weapon-specific object pools"));
}

void UWeaponPoolingIntegrationComponent::CleanupFinishedObjects()
{
    // Clean up finished projectiles
    for (int32 i = ActivePooledProjectiles.Num() - 1; i >= 0; --i)
    {
        AActor* Projectile = ActivePooledProjectiles[i];
        if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed())
        {
            ActivePooledProjectiles.RemoveAt(i);
        }
        // Additional logic could check projectile state/velocity to determine if it should be returned
    }
    
    // Clean up finished particle effects
    for (int32 i = ActivePooledParticleEffects.Num() - 1; i >= 0; --i)
    {
        UParticleSystemComponent* ParticleEffect = ActivePooledParticleEffects[i];
        if (!IsValid(ParticleEffect) || !ParticleEffect->IsActive())
        {
            ReturnPooledParticleEffect(ParticleEffect);
            i--; // Adjust index since ReturnPooledParticleEffect removes from array
        }
    }
    
    // Clean up finished audio components
    for (int32 i = ActivePooledAudioComponents.Num() - 1; i >= 0; --i)
    {
        UAudioComponent* AudioComponent = ActivePooledAudioComponents[i];
        if (!IsValid(AudioComponent) || !AudioComponent->IsPlaying())
        {
            ReturnPooledAudioComponent(AudioComponent);
            i--; // Adjust index since ReturnPooledAudioComponent removes from array
        }
    }
    
    // Clean up expired decals
    for (int32 i = ActivePooledDecals.Num() - 1; i >= 0; --i)
    {
        UDecalComponent* DecalComponent = ActivePooledDecals[i];
        if (!IsValid(DecalComponent) || !DecalComponent->IsVisible())
        {
            ReturnPooledDecal(DecalComponent);
            i--; // Adjust index since ReturnPooledDecal removes from array
        }
    }
}

void UWeaponPoolingIntegrationComponent::UpdatePoolingStatistics()
{
    // This could be expanded to track more detailed statistics
    // and send them to the performance monitoring system
    if (ObjectPoolManager)
    {
        ObjectPoolManager->UpdatePoolStatistics();
    }
}

FString UWeaponPoolingIntegrationComponent::GetPoolName(const FString& PoolType, EAmmoType AmmoType) const
{
    return FString::Printf(TEXT("%s_%s"), *PoolType, *UEnum::GetValueAsString(AmmoType));
}

FString UWeaponPoolingIntegrationComponent::GetPoolName(const FString& PoolType, ESurfaceType SurfaceType) const
{
    return FString::Printf(TEXT("%s_%s"), *PoolType, *UEnum::GetValueAsString(SurfaceType));
}
