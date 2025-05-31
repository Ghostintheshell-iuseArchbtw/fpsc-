#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Engine/DecalActor.h"
#include "AdvancedObjectPoolManager.h"
#include "PooledWeaponEffectsComponent.generated.h"

USTRUCT(BlueprintType)
struct FPooledEffectData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PoolName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InitialPoolSize = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxPoolSize = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EffectDuration = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAutoReturn = true;
};

USTRUCT(BlueprintType)
struct FActivePooledEffect
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* EffectActor = nullptr;

    UPROPERTY()
    FString PoolName;

    UPROPERTY()
    float StartTime = 0.0f;

    UPROPERTY()
    float Duration = 3.0f;

    UPROPERTY()
    FTimerHandle ReturnTimer;

    UPROPERTY()
    bool bAutoReturn = true;
};

/**
 * Component that manages pooled weapon effects integration
 * Provides high-performance particle effects, audio sources, and decals using object pooling
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UPooledWeaponEffectsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPooledWeaponEffectsComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Pool Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    FPooledEffectData MuzzleFlashPoolData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    FPooledEffectData ImpactEffectPoolData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    FPooledEffectData ShellEjectPoolData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    FPooledEffectData AudioSourcePoolData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    FPooledEffectData DecalPoolData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    FPooledEffectData TracerPoolData;

    // Effect Templates
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Templates")
    TSubclassOf<AActor> MuzzleFlashTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Templates")
    TSubclassOf<AActor> ImpactEffectTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Templates")
    TSubclassOf<AActor> ShellEjectTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Templates")
    TSubclassOf<AActor> AudioSourceTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Templates")
    TSubclassOf<AActor> DecalTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Templates")
    TSubclassOf<AActor> TracerTemplate;

    // Performance Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnablePooling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MaxEffectDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxConcurrentEffects = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bUseDistanceCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bUseFrustumCulling = true;

    // Core Functions
    UFUNCTION(BlueprintCallable, Category = "Weapon Effects")
    AActor* SpawnMuzzleFlash(const FVector& Location, const FRotator& Rotation, UParticleSystem* ParticleEffect = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Weapon Effects")
    AActor* SpawnImpactEffect(const FVector& Location, const FRotator& Rotation, const FHitResult& HitResult, UParticleSystem* ParticleEffect = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Weapon Effects")
    AActor* SpawnShellEject(const FVector& Location, const FRotator& Rotation, const FVector& EjectVelocity);

    UFUNCTION(BlueprintCallable, Category = "Weapon Effects")
    AActor* SpawnPooledAudioSource(const FVector& Location, USoundCue* SoundCue);

    UFUNCTION(BlueprintCallable, Category = "Weapon Effects")
    AActor* SpawnImpactDecal(const FVector& Location, const FRotator& Rotation, UMaterialInterface* DecalMaterial, const FVector& DecalSize);

    UFUNCTION(BlueprintCallable, Category = "Weapon Effects")
    AActor* SpawnBulletTracer(const FVector& StartLocation, const FVector& EndLocation, float TracerSpeed = 800.0f);

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "Pool Management")
    void InitializePools();

    UFUNCTION(BlueprintCallable, Category = "Pool Management")
    void CleanupPools();

    UFUNCTION(BlueprintCallable, Category = "Pool Management")
    void ReturnEffectToPool(const FString& PoolName, AActor* Effect);

    UFUNCTION(BlueprintCallable, Category = "Pool Management")
    void ReturnAllEffectsToPool();

    UFUNCTION(BlueprintPure, Category = "Pool Management")
    UAdvancedObjectPoolManager* GetPoolManager() const;

    UFUNCTION(BlueprintPure, Category = "Performance")
    int32 GetActiveEffectCount() const;

    UFUNCTION(BlueprintPure, Category = "Performance")
    FPoolStatistics GetPoolStatistics(const FString& PoolName) const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetPerformanceMode(bool bHighPerformanceMode);

    // Distance and Culling Functions
    UFUNCTION(BlueprintPure, Category = "Performance")
    bool ShouldSpawnEffect(const FVector& Location) const;

    UFUNCTION(BlueprintPure, Category = "Performance")
    bool IsLocationInViewport(const FVector& Location) const;

    UFUNCTION(BlueprintPure, Category = "Performance")
    float GetDistanceToPlayer(const FVector& Location) const;

private:
    // Internal Management
    UPROPERTY()
    UAdvancedObjectPoolManager* PoolManager;

    UPROPERTY()
    TArray<FActivePooledEffect> ActiveEffects;

    // Performance tracking
    UPROPERTY()
    int32 FrameEffectCount = 0;

    UPROPERTY()
    float LastFrameTime = 0.0f;

    UPROPERTY()
    float PerformanceBudget = 16.67f; // Target 60 FPS

    // Internal Functions
    void UpdateActiveEffects(float DeltaTime);
    void ProcessEffectReturn(const FActivePooledEffect& Effect);
    void InitializePool(const FString& PoolName, const FPooledEffectData& PoolData, TSubclassOf<AActor> Template);
    AActor* AcquireFromPool(const FString& PoolName);
    void ConfigurePooledEffect(AActor* Effect, const FVector& Location, const FRotator& Rotation);
    void ApplyPerformanceOptimizations(AActor* Effect, const FVector& Location);
    void TrackActiveEffect(AActor* Effect, const FString& PoolName, float Duration, bool bAutoReturn);
    void CleanupExpiredEffects();
    bool IsEffectBudgetExceeded() const;
};
