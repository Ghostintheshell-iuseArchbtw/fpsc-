#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "../Optimization/AdvancedObjectPoolManager.h"
#include "../Physics/BallisticsSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "WeaponPoolingIntegrationComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWeaponPoolingIntegration, Log, All);

/**
 * Component that integrates weapon systems with object pooling for enhanced performance
 * Manages pooled projectiles, effects, audio sources, and decals for weapons
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UWeaponPoolingIntegrationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponPoolingIntegrationComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Projectile pooling for weapons
    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    AActor* SpawnPooledProjectile(TSubclassOf<AActor> ProjectileClass, const FVector& Location, const FRotator& Rotation, EAmmoType AmmoType);

    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    void ReturnPooledProjectile(AActor* Projectile);

    // Particle effects pooling for muzzle flash, shell ejection, impact effects
    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    UParticleSystemComponent* SpawnPooledMuzzleFlash(UParticleSystem* MuzzleFlashEffect, const FVector& Location, const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    UParticleSystemComponent* SpawnPooledShellEject(UParticleSystem* ShellEjectEffect, const FVector& Location, const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    UParticleSystemComponent* SpawnPooledImpactEffect(UParticleSystem* ImpactEffect, const FVector& Location, const FRotator& Rotation, ESurfaceType SurfaceType);

    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    UParticleSystemComponent* SpawnPooledTracer(UParticleSystem* TracerEffect, const FVector& StartLocation, const FVector& EndLocation);

    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    void ReturnPooledParticleEffect(UParticleSystemComponent* ParticleEffect);

    // Audio pooling for weapon sounds
    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    UAudioComponent* SpawnPooledWeaponAudio(USoundCue* WeaponSound, const FVector& Location, bool bSpatialize = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    void ReturnPooledAudioComponent(UAudioComponent* AudioComponent);

    // Decal pooling for bullet holes and impact marks
    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    UDecalComponent* SpawnPooledDecal(UMaterialInterface* DecalMaterial, const FVector& Location, const FRotator& Rotation, const FVector& Size, float Lifespan = 30.0f);

    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    void ReturnPooledDecal(UDecalComponent* DecalComponent);

    // Ballistics integration
    UFUNCTION(BlueprintCallable, Category = "Weapon Pooling Integration")
    void FirePooledBullet(FVector Origin, FVector Direction, EAmmoType AmmoType, EBulletType BulletType, AActor* Instigator);

    // Performance optimization for weapons
    UFUNCTION(BlueprintCallable, Category = "Weapon Performance")
    void OptimizeWeaponPooling();

    UFUNCTION(BlueprintCallable, Category = "Weapon Performance")
    FString GetWeaponPoolingReport() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon Performance")
    void PrewarmWeaponPools();

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    bool bUsePooledProjectiles = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    bool bUsePooledParticleEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    bool bUsePooledAudio = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    bool bUsePooledDecals = true;

    // Pool sizes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    int32 ProjectilePoolSize = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    int32 ParticleEffectPoolSize = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    int32 AudioComponentPoolSize = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    int32 DecalPoolSize = 200;

protected:
    // References
    UPROPERTY()
    class UAdvancedObjectPoolManager* ObjectPoolManager;

    UPROPERTY()
    class UBallisticsSystem* BallisticsSystem;

    // Tracking for active pooled objects
    UPROPERTY()
    TArray<AActor*> ActivePooledProjectiles;

    UPROPERTY()
    TArray<UParticleSystemComponent*> ActivePooledParticleEffects;

    UPROPERTY()
    TArray<UAudioComponent*> ActivePooledAudioComponents;

    UPROPERTY()
    TArray<UDecalComponent*> ActivePooledDecals;

    // Performance tracking
    UPROPERTY()
    int32 ProjectilesSpawned = 0;

    UPROPERTY()
    int32 EffectsSpawned = 0;

    UPROPERTY()
    int32 AudioSpawned = 0;

    UPROPERTY()
    int32 DecalsSpawned = 0;

    UPROPERTY()
    float LastCleanupTime = 0.0f;

    UPROPERTY()
    float CleanupInterval = 5.0f;

private:
    void InitializeWeaponPools();
    void CleanupFinishedObjects();
    void UpdatePoolingStatistics();
    FString GetPoolName(const FString& PoolType, EAmmoType AmmoType = EAmmoType::Rifle_556) const;
    FString GetPoolName(const FString& PoolType, ESurfaceType SurfaceType = ESurfaceType::Concrete) const;
};
