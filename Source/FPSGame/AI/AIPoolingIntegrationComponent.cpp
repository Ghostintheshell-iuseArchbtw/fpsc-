#include "AIPoolingIntegrationComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"

DEFINE_LOG_CATEGORY(LogAIPoolingIntegration);

UAIPoolingIntegrationComponent::UAIPoolingIntegrationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f; // Update once per second
    
    ObjectPoolManager = nullptr;
    AISystem = nullptr;
}

void UAIPoolingIntegrationComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializePoolingIntegration();
}

void UAIPoolingIntegrationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CleanupPooledObjects();
    
    Super::EndPlay(EndPlayReason);
}

void UAIPoolingIntegrationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Update pooling statistics
    UpdatePoolingStatistics();
    
    // Clean up any finished pooled objects
    CleanupPooledObjects();
}

void UAIPoolingIntegrationComponent::InitializePoolingIntegration()
{
    UE_LOG(LogAIPoolingIntegration, Log, TEXT("Initializing AI pooling integration..."));
    
    // Get object pool manager
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            ObjectPoolManager = GameInstance->GetSubsystem<UAdvancedObjectPoolManager>();
        }
    }
    
    if (!ObjectPoolManager)
    {
        UE_LOG(LogAIPoolingIntegration, Error, TEXT("Failed to get Object Pool Manager"));
        return;
    }
    
    // Get AI system component
    AISystem = GetOwner()->FindComponentByClass<UAdvancedAISystem>();
    if (!AISystem)
    {
        UE_LOG(LogAIPoolingIntegration, Warning, TEXT("AI System component not found on owner"));
    }
    
    UE_LOG(LogAIPoolingIntegration, Log, TEXT("AI pooling integration initialized successfully"));
}

AActor* UAIPoolingIntegrationComponent::SpawnPooledProjectile(TSubclassOf<AActor> ProjectileClass, const FVector& Location, const FRotator& Rotation)
{
    if (!ObjectPoolManager || !ProjectileClass)
    {
        return nullptr;
    }
    
    // Acquire projectile from pool
    AActor* Projectile = ObjectPoolManager->AcquireActor(ProjectileClass, TEXT("AI_Projectiles"));
    
    if (Projectile)
    {
        // Set projectile location and rotation
        Projectile->SetActorLocation(Location);
        Projectile->SetActorRotation(Rotation);
        
        // Track the projectile
        ActivePooledProjectiles.AddUnique(Projectile);
        
        UE_LOG(LogAIPoolingIntegration, Verbose, TEXT("Spawned pooled projectile: %s"), 
               *Projectile->GetClass()->GetName());
    }
    else
    {
        UE_LOG(LogAIPoolingIntegration, Warning, TEXT("Failed to acquire projectile from pool"));
    }
    
    return Projectile;
}

void UAIPoolingIntegrationComponent::ReturnPooledProjectile(AActor* Projectile)
{
    if (!ObjectPoolManager || !Projectile)
    {
        return;
    }
    
    // Remove from tracking
    ActivePooledProjectiles.Remove(Projectile);
    
    // Return to pool
    ObjectPoolManager->ReleaseActor(Projectile);
    
    UE_LOG(LogAIPoolingIntegration, Verbose, TEXT("Returned projectile to pool: %s"), 
           *Projectile->GetClass()->GetName());
}

UObject* UAIPoolingIntegrationComponent::SpawnPooledEffect(TSubclassOf<UObject> EffectClass, const FVector& Location)
{
    if (!ObjectPoolManager || !EffectClass)
    {
        return nullptr;
    }
    
    // Acquire effect from pool
    UObject* Effect = ObjectPoolManager->AcquireObject(EffectClass, TEXT("AI_Effects"));
    
    if (Effect)
    {
        // If it's a particle system component, set its location
        if (UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Effect))
        {
            ParticleComp->SetWorldLocation(Location);
            ParticleComp->Activate();
        }
        
        // Track the effect
        ActivePooledEffects.AddUnique(Effect);
        
        UE_LOG(LogAIPoolingIntegration, Verbose, TEXT("Spawned pooled effect: %s"), 
               *Effect->GetClass()->GetName());
    }
    else
    {
        UE_LOG(LogAIPoolingIntegration, Warning, TEXT("Failed to acquire effect from pool"));
    }
    
    return Effect;
}

void UAIPoolingIntegrationComponent::ReturnPooledEffect(UObject* Effect)
{
    if (!ObjectPoolManager || !Effect)
    {
        return;
    }
    
    // Deactivate if it's a particle system
    if (UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Effect))
    {
        ParticleComp->Deactivate();
    }
    
    // Remove from tracking
    ActivePooledEffects.Remove(Effect);
    
    // Return to pool
    ObjectPoolManager->ReleaseObject(Effect);
    
    UE_LOG(LogAIPoolingIntegration, Verbose, TEXT("Returned effect to pool: %s"), 
           *Effect->GetClass()->GetName());
}

UActorComponent* UAIPoolingIntegrationComponent::SpawnPooledAudioComponent(TSubclassOf<UActorComponent> AudioClass)
{
    if (!ObjectPoolManager || !AudioClass)
    {
        return nullptr;
    }
    
    // Acquire audio component from pool
    UActorComponent* AudioComponent = ObjectPoolManager->AcquireComponent(AudioClass, TEXT("AI_Audio"));
    
    if (AudioComponent)
    {
        // Track the audio component
        ActivePooledAudioComponents.AddUnique(AudioComponent);
        
        UE_LOG(LogAIPoolingIntegration, Verbose, TEXT("Spawned pooled audio component: %s"), 
               *AudioComponent->GetClass()->GetName());
    }
    else
    {
        UE_LOG(LogAIPoolingIntegration, Warning, TEXT("Failed to acquire audio component from pool"));
    }
    
    return AudioComponent;
}

void UAIPoolingIntegrationComponent::ReturnPooledAudioComponent(UActorComponent* AudioComponent)
{
    if (!ObjectPoolManager || !AudioComponent)
    {
        return;
    }
    
    // Stop audio if it's playing
    if (UAudioComponent* Audio = Cast<UAudioComponent>(AudioComponent))
    {
        Audio->Stop();
    }
    
    // Remove from tracking
    ActivePooledAudioComponents.Remove(AudioComponent);
    
    // Return to pool
    ObjectPoolManager->ReleaseComponent(AudioComponent);
    
    UE_LOG(LogAIPoolingIntegration, Verbose, TEXT("Returned audio component to pool: %s"), 
           *AudioComponent->GetClass()->GetName());
}

void UAIPoolingIntegrationComponent::OptimizeAIForPooling()
{
    if (!AISystem)
    {
        return;
    }
    
    UE_LOG(LogAIPoolingIntegration, Log, TEXT("Optimizing AI for object pooling..."));
    
    // Clean up any unused pooled objects
    CleanupPooledObjects();
    
    // Optimize memory usage by cleaning up pools
    if (ObjectPoolManager)
    {
        ObjectPoolManager->CleanupAllPools();
    }
    
    UE_LOG(LogAIPoolingIntegration, Log, TEXT("AI pooling optimization complete"));
}

FString UAIPoolingIntegrationComponent::GetPoolingPerformanceReport() const
{
    FString Report;
    
    Report += TEXT("=== AI Pooling Integration Performance Report ===\n");
    Report += FString::Printf(TEXT("Active Pooled Projectiles: %d\n"), ActivePooledProjectiles.Num());
    Report += FString::Printf(TEXT("Active Pooled Effects: %d\n"), ActivePooledEffects.Num());
    Report += FString::Printf(TEXT("Active Pooled Audio Components: %d\n"), ActivePooledAudioComponents.Num());
    
    if (ObjectPoolManager)
    {
        Report += TEXT("\n=== Pool Statistics ===\n");
        
        // Get AI-specific pool statistics
        TArray<FString> PoolNames = ObjectPoolManager->GetActivePoolNames();
        for (const FString& PoolName : PoolNames)
        {
            if (PoolName.Contains(TEXT("AI_")))
            {
                FPoolStatistics Stats = ObjectPoolManager->GetPoolStatistics(PoolName);
                Report += FString::Printf(TEXT("%s - Active: %d, Available: %d, Hit Rate: %.2f%%\n"),
                    *PoolName, Stats.ActiveObjects, Stats.AvailableObjects, Stats.HitRate * 100.0f);
            }
        }
    }
    
    return Report;
}

void UAIPoolingIntegrationComponent::CleanupPooledObjects()
{
    // Clean up finished projectiles
    for (int32 i = ActivePooledProjectiles.Num() - 1; i >= 0; --i)
    {
        AActor* Projectile = ActivePooledProjectiles[i];
        if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed())
        {
            ActivePooledProjectiles.RemoveAt(i);
        }
        // Additional logic could check if projectile has finished its purpose
        // For example, if it has hit something or traveled maximum distance
    }
    
    // Clean up finished effects
    for (int32 i = ActivePooledEffects.Num() - 1; i >= 0; --i)
    {
        UObject* Effect = ActivePooledEffects[i];
        if (!IsValid(Effect))
        {
            ActivePooledEffects.RemoveAt(i);
            continue;
        }
        
        // Check if particle system has finished
        if (UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Effect))
        {
            if (!ParticleComp->IsActive())
            {
                ReturnPooledEffect(Effect);
                i--; // Adjust index since ReturnPooledEffect removes from array
            }
        }
    }
    
    // Clean up finished audio components
    for (int32 i = ActivePooledAudioComponents.Num() - 1; i >= 0; --i)
    {
        UActorComponent* AudioComponent = ActivePooledAudioComponents[i];
        if (!IsValid(AudioComponent))
        {
            ActivePooledAudioComponents.RemoveAt(i);
            continue;
        }
        
        // Check if audio has finished playing
        if (UAudioComponent* Audio = Cast<UAudioComponent>(AudioComponent))
        {
            if (!Audio->IsPlaying())
            {
                ReturnPooledAudioComponent(AudioComponent);
                i--; // Adjust index since ReturnPooledAudioComponent removes from array
            }
        }
    }
}

void UAIPoolingIntegrationComponent::UpdatePoolingStatistics()
{
    if (!ObjectPoolManager)
    {
        return;
    }
    
    // Update statistics for AI-specific pools
    TArray<FString> PoolNames = ObjectPoolManager->GetActivePoolNames();
    for (const FString& PoolName : PoolNames)
    {
        if (PoolName.Contains(TEXT("AI_")))
        {
            FPoolStatistics Stats = ObjectPoolManager->GetPoolStatistics(PoolName);
            
            // Log performance warnings if needed
            if (Stats.HitRate < 0.5f && Stats.TotalAcquisitions > 100)
            {
                UE_LOG(LogAIPoolingIntegration, Warning, 
                    TEXT("Low hit rate for AI pool %s: %.2f%% (Consider increasing pool size)"),
                    *PoolName, Stats.HitRate * 100.0f);
            }
            
            if (Stats.MemoryUsageMB > 50.0f)
            {
                UE_LOG(LogAIPoolingIntegration, Warning,
                    TEXT("High memory usage for AI pool %s: %.2f MB"),
                    *PoolName, Stats.MemoryUsageMB);
            }
        }
    }
}
