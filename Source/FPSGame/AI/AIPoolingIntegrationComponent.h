#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "../Optimization/AdvancedObjectPoolManager.h"
#include "AdvancedAISystem.h"
#include "AIPoolingIntegrationComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAIPoolingIntegration, Log, All);

/**
 * AI Pooling Integration Component
 * Integrates AI systems with the Advanced Object Pooling System
 * Handles pooled projectiles, effects, and audio for AI characters
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UAIPoolingIntegrationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAIPoolingIntegrationComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Integration with object pooling
    UFUNCTION(BlueprintCallable, Category = "AI Pooling Integration")
    void InitializePoolingIntegration();

    UFUNCTION(BlueprintCallable, Category = "AI Pooling Integration")
    AActor* SpawnPooledProjectile(TSubclassOf<AActor> ProjectileClass, const FVector& Location, const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "AI Pooling Integration")
    void ReturnPooledProjectile(AActor* Projectile);

    UFUNCTION(BlueprintCallable, Category = "AI Pooling Integration")
    UObject* SpawnPooledEffect(TSubclassOf<UObject> EffectClass, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "AI Pooling Integration")
    void ReturnPooledEffect(UObject* Effect);

    UFUNCTION(BlueprintCallable, Category = "AI Pooling Integration")
    UActorComponent* SpawnPooledAudioComponent(TSubclassOf<UActorComponent> AudioClass);

    UFUNCTION(BlueprintCallable, Category = "AI Pooling Integration")
    void ReturnPooledAudioComponent(UActorComponent* AudioComponent);

    // Performance optimization for AI
    UFUNCTION(BlueprintCallable, Category = "AI Performance")
    void OptimizeAIForPooling();

    UFUNCTION(BlueprintCallable, Category = "AI Performance")
    FString GetPoolingPerformanceReport() const;

protected:
    // References
    UPROPERTY()
    class UAdvancedObjectPoolManager* ObjectPoolManager;

    UPROPERTY()
    class UAdvancedAISystem* AISystem;

    // Tracking
    UPROPERTY()
    TArray<AActor*> ActivePooledProjectiles;

    UPROPERTY()
    TArray<UObject*> ActivePooledEffects;

    UPROPERTY()
    TArray<UActorComponent*> ActivePooledAudioComponents;

private:
    void CleanupPooledObjects();
    void UpdatePoolingStatistics();
};
