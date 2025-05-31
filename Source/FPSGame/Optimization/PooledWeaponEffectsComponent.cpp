#include "PooledWeaponEffectsComponent.h"
#include "AdvancedObjectPoolManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPooledWeaponEffects, Log, All);

UPooledWeaponEffectsComponent::UPooledWeaponEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.033f; // 30 FPS tick rate for effect management

    // Initialize default pool configurations
    MuzzleFlashPoolData.PoolName = TEXT("MuzzleFlash");
    MuzzleFlashPoolData.InitialPoolSize = 15;
    MuzzleFlashPoolData.MaxPoolSize = 50;
    MuzzleFlashPoolData.EffectDuration = 0.2f;
    MuzzleFlashPoolData.bAutoReturn = true;

    ImpactEffectPoolData.PoolName = TEXT("ImpactEffect");
    ImpactEffectPoolData.InitialPoolSize = 25;
    ImpactEffectPoolData.MaxPoolSize = 100;
    ImpactEffectPoolData.EffectDuration = 3.0f;
    ImpactEffectPoolData.bAutoReturn = true;

    ShellEjectPoolData.PoolName = TEXT("ShellEject");
    ShellEjectPoolData.InitialPoolSize = 20;
    ShellEjectPoolData.MaxPoolSize = 75;
    ShellEjectPoolData.EffectDuration = 8.0f;
    ShellEjectPoolData.bAutoReturn = true;

    AudioSourcePoolData.PoolName = TEXT("AudioSource");
    AudioSourcePoolData.InitialPoolSize = 10;
    AudioSourcePoolData.MaxPoolSize = 30;
    AudioSourcePoolData.EffectDuration = 5.0f;
    AudioSourcePoolData.bAutoReturn = true;

    DecalPoolData.PoolName = TEXT("Decal");
    DecalPoolData.InitialPoolSize = 30;
    DecalPoolData.MaxPoolSize = 150;
    DecalPoolData.EffectDuration = 30.0f;
    DecalPoolData.bAutoReturn = true;

    TracerPoolData.PoolName = TEXT("Tracer");
    TracerPoolData.InitialPoolSize = 15;
    TracerPoolData.MaxPoolSize = 60;
    TracerPoolData.EffectDuration = 2.0f;
    TracerPoolData.bAutoReturn = true;

    // Performance defaults
    MaxEffectDistance = 2000.0f;
    MaxConcurrentEffects = 50;
    bUseDistanceCulling = true;
    bUseFrustumCulling = true;
    PerformanceBudget = 16.67f; // Target 60 FPS
}

void UPooledWeaponEffectsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get or create pool manager
    PoolManager = UAdvancedObjectPoolManager::GetInstance(GetWorld());
    
    if (bEnablePooling && PoolManager)
    {
        InitializePools();
        UE_LOG(LogPooledWeaponEffects, Log, TEXT("PooledWeaponEffectsComponent initialized with pooling enabled"));
    }
    else
    {
        UE_LOG(LogPooledWeaponEffects, Warning, TEXT("PooledWeaponEffectsComponent running without pooling - performance may be degraded"));
    }
}

void UPooledWeaponEffectsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CleanupPools();
    Super::EndPlay(EndPlayReason);
}

void UPooledWeaponEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    UpdateActiveEffects(DeltaTime);
    
    // Reset frame counters
    if (GetWorld()->GetTimeSeconds() - LastFrameTime > 1.0f)
    {
        FrameEffectCount = 0;
        LastFrameTime = GetWorld()->GetTimeSeconds();
    }
}

void UPooledWeaponEffectsComponent::InitializePools()
{
    if (!PoolManager)
    {
        UE_LOG(LogPooledWeaponEffects, Error, TEXT("Cannot initialize pools - PoolManager is null"));
        return;
    }

    // Initialize all effect pools
    InitializePool(MuzzleFlashPoolData.PoolName, MuzzleFlashPoolData, MuzzleFlashTemplate);
    InitializePool(ImpactEffectPoolData.PoolName, ImpactEffectPoolData, ImpactEffectTemplate);
    InitializePool(ShellEjectPoolData.PoolName, ShellEjectPoolData, ShellEjectTemplate);
    InitializePool(AudioSourcePoolData.PoolName, AudioSourcePoolData, AudioSourceTemplate);
    InitializePool(DecalPoolData.PoolName, DecalPoolData, DecalTemplate);
    InitializePool(TracerPoolData.PoolName, TracerPoolData, TracerTemplate);

    UE_LOG(LogPooledWeaponEffects, Log, TEXT("All weapon effect pools initialized successfully"));
}

void UPooledWeaponEffectsComponent::InitializePool(const FString& PoolName, const FPooledEffectData& PoolData, TSubclassOf<AActor> Template)
{
    if (!PoolManager || !Template)
    {
        UE_LOG(LogPooledWeaponEffects, Warning, TEXT("Skipping pool initialization for %s - missing manager or template"), *PoolName);
        return;
    }

    // Create specialized pools based on type
    if (PoolName.Contains(TEXT("Particle")) || PoolName.Contains(TEXT("MuzzleFlash")) || PoolName.Contains(TEXT("Impact")))
    {
        PoolManager->CreateParticlePool(PoolName, PoolData.InitialPoolSize, PoolData.MaxPoolSize);
    }
    else if (PoolName.Contains(TEXT("Audio")))
    {
        PoolManager->CreateAudioPool(PoolName, PoolData.InitialPoolSize, PoolData.MaxPoolSize);
    }
    else if (PoolName.Contains(TEXT("Decal")))
    {
        PoolManager->CreateDecalPool(PoolName, PoolData.InitialPoolSize, PoolData.MaxPoolSize);
    }
    else
    {
        // Generic pool for other effect types
        PoolManager->CreatePool(PoolName, Template, PoolData.InitialPoolSize, PoolData.MaxPoolSize);
    }

    UE_LOG(LogPooledWeaponEffects, VeryVerbose, TEXT("Initialized pool '%s' with %d initial objects"), 
           *PoolName, PoolData.InitialPoolSize);
}

AActor* UPooledWeaponEffectsComponent::SpawnMuzzleFlash(const FVector& Location, const FRotator& Rotation, UParticleSystem* ParticleEffect)
{
    if (!ShouldSpawnEffect(Location) || IsEffectBudgetExceeded())
    {
        return nullptr;
    }

    AActor* Effect = nullptr;
    
    if (bEnablePooling && PoolManager)
    {
        Effect = PoolManager->AcquireParticleEffect(MuzzleFlashPoolData.PoolName);
    }
    
    if (!Effect && MuzzleFlashTemplate)
    {
        // Fallback to spawning new actor
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Effect = GetWorld()->SpawnActor<AActor>(MuzzleFlashTemplate, Location, Rotation, SpawnParams);
    }

    if (Effect)
    {
        ConfigurePooledEffect(Effect, Location, Rotation);
        ApplyPerformanceOptimizations(Effect, Location);
        
        // Configure particle system if provided
        if (ParticleEffect)
        {
            if (UParticleSystemComponent* PSC = Effect->FindComponentByClass<UParticleSystemComponent>())
            {
                PSC->SetTemplate(ParticleEffect);
                PSC->ActivateSystem();
            }
        }

        TrackActiveEffect(Effect, MuzzleFlashPoolData.PoolName, MuzzleFlashPoolData.EffectDuration, MuzzleFlashPoolData.bAutoReturn);
        FrameEffectCount++;
        
        UE_LOG(LogPooledWeaponEffects, VeryVerbose, TEXT("Spawned muzzle flash at %s"), *Location.ToString());
    }

    return Effect;
}

AActor* UPooledWeaponEffectsComponent::SpawnImpactEffect(const FVector& Location, const FRotator& Rotation, const FHitResult& HitResult, UParticleSystem* ParticleEffect)
{
    if (!ShouldSpawnEffect(Location) || IsEffectBudgetExceeded())
    {
        return nullptr;
    }

    AActor* Effect = nullptr;
    
    if (bEnablePooling && PoolManager)
    {
        Effect = PoolManager->AcquireParticleEffect(ImpactEffectPoolData.PoolName);
    }
    
    if (!Effect && ImpactEffectTemplate)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Effect = GetWorld()->SpawnActor<AActor>(ImpactEffectTemplate, Location, Rotation, SpawnParams);
    }

    if (Effect)
    {
        ConfigurePooledEffect(Effect, Location, Rotation);
        ApplyPerformanceOptimizations(Effect, Location);
        
        // Configure impact-specific properties
        if (ParticleEffect)
        {
            if (UParticleSystemComponent* PSC = Effect->FindComponentByClass<UParticleSystemComponent>())
            {
                PSC->SetTemplate(ParticleEffect);
                PSC->ActivateSystem();
                
                // Scale effect based on impact energy or surface type
                float ScaleFactor = FMath::Clamp(HitResult.Distance / 1000.0f, 0.5f, 2.0f);
                PSC->SetWorldScale3D(FVector(ScaleFactor));
            }
        }

        TrackActiveEffect(Effect, ImpactEffectPoolData.PoolName, ImpactEffectPoolData.EffectDuration, ImpactEffectPoolData.bAutoReturn);
        FrameEffectCount++;
        
        UE_LOG(LogPooledWeaponEffects, VeryVerbose, TEXT("Spawned impact effect at %s"), *Location.ToString());
    }

    return Effect;
}

AActor* UPooledWeaponEffectsComponent::SpawnShellEject(const FVector& Location, const FRotator& Rotation, const FVector& EjectVelocity)
{
    if (!ShouldSpawnEffect(Location) || IsEffectBudgetExceeded())
    {
        return nullptr;
    }

    AActor* Effect = AcquireFromPool(ShellEjectPoolData.PoolName);
    
    if (Effect)
    {
        ConfigurePooledEffect(Effect, Location, Rotation);
        ApplyPerformanceOptimizations(Effect, Location);
        
        // Apply eject velocity if the effect has physics
        if (UPrimitiveComponent* PrimComp = Effect->FindComponentByClass<UPrimitiveComponent>())
        {
            if (PrimComp->IsSimulatingPhysics())
            {
                PrimComp->SetPhysicsLinearVelocity(EjectVelocity);
                
                // Add random angular velocity for realism
                FVector AngularVel = FVector(
                    FMath::RandRange(-360.0f, 360.0f),
                    FMath::RandRange(-360.0f, 360.0f),
                    FMath::RandRange(-180.0f, 180.0f)
                );
                PrimComp->SetPhysicsAngularVelocityInDegrees(AngularVel);
            }
        }

        TrackActiveEffect(Effect, ShellEjectPoolData.PoolName, ShellEjectPoolData.EffectDuration, ShellEjectPoolData.bAutoReturn);
        FrameEffectCount++;
    }

    return Effect;
}

AActor* UPooledWeaponEffectsComponent::SpawnPooledAudioSource(const FVector& Location, USoundCue* SoundCue)
{
    if (!ShouldSpawnEffect(Location) || IsEffectBudgetExceeded())
    {
        return nullptr;
    }

    AActor* AudioSource = nullptr;
    
    if (bEnablePooling && PoolManager)
    {
        AudioSource = PoolManager->AcquireAudioSource(AudioSourcePoolData.PoolName);
    }
    
    if (!AudioSource && AudioSourceTemplate)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AudioSource = GetWorld()->SpawnActor<AActor>(AudioSourceTemplate, Location, FRotator::ZeroRotator, SpawnParams);
    }

    if (AudioSource && SoundCue)
    {
        AudioSource->SetActorLocation(Location);
        AudioSource->SetActorHiddenInGame(false);
        
        if (UAudioComponent* AudioComp = AudioSource->FindComponentByClass<UAudioComponent>())
        {
            AudioComp->SetSound(SoundCue);
            AudioComp->Play();
            
            // Set 3D audio properties
            float Distance = GetDistanceToPlayer(Location);
            float VolumeMultiplier = FMath::Clamp(1.0f - (Distance / MaxEffectDistance), 0.1f, 1.0f);
            AudioComp->SetVolumeMultiplier(VolumeMultiplier);
        }

        // Use the sound's duration for auto-return
        float SoundDuration = SoundCue ? SoundCue->GetDuration() : AudioSourcePoolData.EffectDuration;
        TrackActiveEffect(AudioSource, AudioSourcePoolData.PoolName, SoundDuration, AudioSourcePoolData.bAutoReturn);
        FrameEffectCount++;
    }

    return AudioSource;
}

AActor* UPooledWeaponEffectsComponent::SpawnImpactDecal(const FVector& Location, const FRotator& Rotation, UMaterialInterface* DecalMaterial, const FVector& DecalSize)
{
    if (!ShouldSpawnEffect(Location) || IsEffectBudgetExceeded())
    {
        return nullptr;
    }

    AActor* Decal = nullptr;
    
    if (bEnablePooling && PoolManager)
    {
        Decal = PoolManager->AcquireDecal(DecalPoolData.PoolName);
    }
    
    if (!Decal && DecalTemplate)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Decal = GetWorld()->SpawnActor<AActor>(DecalTemplate, Location, Rotation, SpawnParams);
    }

    if (Decal)
    {
        ConfigurePooledEffect(Decal, Location, Rotation);
        
        if (UDecalComponent* DecalComp = Decal->FindComponentByClass<UDecalComponent>())
        {
            if (DecalMaterial)
            {
                DecalComp->SetDecalMaterial(DecalMaterial);
            }
            DecalComp->DecalSize = DecalSize;
            
            // Optimize decal based on distance
            float Distance = GetDistanceToPlayer(Location);
            if (Distance > MaxEffectDistance * 0.5f)
            {
                // Reduce decal size and quality for distant impacts
                DecalComp->DecalSize *= 0.7f;
            }
        }

        TrackActiveEffect(Decal, DecalPoolData.PoolName, DecalPoolData.EffectDuration, DecalPoolData.bAutoReturn);
        FrameEffectCount++;
    }

    return Decal;
}

AActor* UPooledWeaponEffectsComponent::SpawnBulletTracer(const FVector& StartLocation, const FVector& EndLocation, float TracerSpeed)
{
    if (!ShouldSpawnEffect(StartLocation) || IsEffectBudgetExceeded())
    {
        return nullptr;
    }

    AActor* Tracer = AcquireFromPool(TracerPoolData.PoolName);
    
    if (Tracer)
    {
        FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
        FRotator TracerRotation = Direction.Rotation();
        
        ConfigurePooledEffect(Tracer, StartLocation, TracerRotation);
        
        // Configure tracer movement
        float Distance = FVector::Dist(StartLocation, EndLocation);
        float TravelTime = Distance / TracerSpeed;
        
        // Animate tracer movement
        FLatentActionInfo LatentInfo;
        LatentInfo.CallbackTarget = this;
        LatentInfo.ExecutionFunction = FName("ProcessTracerComplete");
        LatentInfo.Linkage = 0;
        LatentInfo.UUID = FMath::Rand();
        
        UKismetSystemLibrary::MoveComponentTo(
            Tracer->GetRootComponent(),
            EndLocation,
            TracerRotation,
            false,
            false,
            TravelTime,
            false,
            EMoveComponentAction::Move,
            LatentInfo
        );

        TrackActiveEffect(Tracer, TracerPoolData.PoolName, TravelTime + 0.5f, TracerPoolData.bAutoReturn);
        FrameEffectCount++;
    }

    return Tracer;
}

void UPooledWeaponEffectsComponent::UpdateActiveEffects(float DeltaTime)
{
    CleanupExpiredEffects();
    
    // Update performance budget based on frame rate
    float CurrentFrameTime = GetWorld()->GetDeltaSeconds();
    if (CurrentFrameTime > PerformanceBudget * 1.2f)
    {
        // Frame rate is struggling, reduce effect quality
        SetPerformanceMode(true);
    }
    else if (CurrentFrameTime < PerformanceBudget * 0.8f)
    {
        // Frame rate is good, can afford higher quality
        SetPerformanceMode(false);
    }
}

void UPooledWeaponEffectsComponent::CleanupExpiredEffects()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    for (int32 i = ActiveEffects.Num() - 1; i >= 0; i--)
    {
        const FActivePooledEffect& Effect = ActiveEffects[i];
        
        if (Effect.bAutoReturn && Effect.EffectActor && 
            (CurrentTime - Effect.StartTime) >= Effect.Duration)
        {
            ProcessEffectReturn(Effect);
            ActiveEffects.RemoveAt(i);
        }
    }
}

void UPooledWeaponEffectsComponent::ProcessEffectReturn(const FActivePooledEffect& Effect)
{
    if (Effect.EffectActor && PoolManager)
    {
        // Stop any active systems before returning
        if (UParticleSystemComponent* PSC = Effect.EffectActor->FindComponentByClass<UParticleSystemComponent>())
        {
            PSC->DeactivateSystem();
        }
        
        if (UAudioComponent* AudioComp = Effect.EffectActor->FindComponentByClass<UAudioComponent>())
        {
            AudioComp->Stop();
        }
        
        // Return to appropriate pool
        if (Effect.PoolName.Contains(TEXT("Particle")) || Effect.PoolName.Contains(TEXT("MuzzleFlash")) || Effect.PoolName.Contains(TEXT("Impact")))
        {
            PoolManager->ReturnParticleEffect(Effect.PoolName, Effect.EffectActor);
        }
        else if (Effect.PoolName.Contains(TEXT("Audio")))
        {
            PoolManager->ReturnAudioSource(Effect.PoolName, Effect.EffectActor);
        }
        else if (Effect.PoolName.Contains(TEXT("Decal")))
        {
            PoolManager->ReturnDecal(Effect.PoolName, Effect.EffectActor);
        }
        else
        {
            PoolManager->ReleaseActor(Effect.EffectActor);
        }
        
        UE_LOG(LogPooledWeaponEffects, VeryVerbose, TEXT("Returned effect to pool: %s"), *Effect.PoolName);
    }
}

AActor* UPooledWeaponEffectsComponent::AcquireFromPool(const FString& PoolName)
{
    if (!bEnablePooling || !PoolManager)
    {
        return nullptr;
    }

    AActor* Effect = nullptr;
    
    // Route to appropriate pool type
    if (PoolName.Contains(TEXT("Particle")) || PoolName.Contains(TEXT("MuzzleFlash")) || PoolName.Contains(TEXT("Impact")))
    {
        Effect = PoolManager->AcquireParticleEffect(PoolName);
    }
    else if (PoolName.Contains(TEXT("Audio")))
    {
        Effect = PoolManager->AcquireAudioSource(PoolName);
    }
    else if (PoolName.Contains(TEXT("Decal")))
    {
        Effect = PoolManager->AcquireDecal(PoolName);
    }
    else
    {
        Effect = PoolManager->AcquireActor(PoolName);
    }

    return Effect;
}

void UPooledWeaponEffectsComponent::ConfigurePooledEffect(AActor* Effect, const FVector& Location, const FRotator& Rotation)
{
    if (!Effect)
    {
        return;
    }

    Effect->SetActorLocation(Location);
    Effect->SetActorRotation(Rotation);
    Effect->SetActorHiddenInGame(false);
    Effect->SetActorEnableCollision(false); // Most effects don't need collision
    
    // Reset any movement
    if (UPrimitiveComponent* PrimComp = Effect->FindComponentByClass<UPrimitiveComponent>())
    {
        if (PrimComp->IsSimulatingPhysics())
        {
            PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
            PrimComp->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        }
    }
}

void UPooledWeaponEffectsComponent::ApplyPerformanceOptimizations(AActor* Effect, const FVector& Location)
{
    if (!Effect)
    {
        return;
    }

    float Distance = GetDistanceToPlayer(Location);
    
    // Distance-based optimizations
    if (Distance > MaxEffectDistance * 0.7f)
    {
        // Reduce particle count and quality for distant effects
        if (UParticleSystemComponent* PSC = Effect->FindComponentByClass<UParticleSystemComponent>())
        {
            PSC->SetFloatParameter(FName("SpawnRate"), 0.5f);
            PSC->SetFloatParameter(FName("Scale"), 0.8f);
        }
    }
    
    // LOD based on performance
    if (FrameEffectCount > MaxConcurrentEffects * 0.8f)
    {
        // High load - reduce quality
        if (UParticleSystemComponent* PSC = Effect->FindComponentByClass<UParticleSystemComponent>())
        {
            PSC->SetFloatParameter(FName("LOD"), 1.0f);
        }
    }
}

void UPooledWeaponEffectsComponent::TrackActiveEffect(AActor* Effect, const FString& PoolName, float Duration, bool bAutoReturn)
{
    if (!Effect)
    {
        return;
    }

    FActivePooledEffect ActiveEffect;
    ActiveEffect.EffectActor = Effect;
    ActiveEffect.PoolName = PoolName;
    ActiveEffect.StartTime = GetWorld()->GetTimeSeconds();
    ActiveEffect.Duration = Duration;
    ActiveEffect.bAutoReturn = bAutoReturn;

    ActiveEffects.Add(ActiveEffect);
}

bool UPooledWeaponEffectsComponent::ShouldSpawnEffect(const FVector& Location) const
{
    // Distance culling
    if (bUseDistanceCulling && GetDistanceToPlayer(Location) > MaxEffectDistance)
    {
        return false;
    }
    
    // Frustum culling
    if (bUseFrustumCulling && !IsLocationInViewport(Location))
    {
        return false;
    }
    
    // Effect budget check
    if (ActiveEffects.Num() >= MaxConcurrentEffects)
    {
        return false;
    }

    return true;
}

bool UPooledWeaponEffectsComponent::IsLocationInViewport(const FVector& Location) const
{
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
        {
            FVector CameraLocation = CameraManager->GetCameraLocation();
            FRotator CameraRotation = CameraManager->GetCameraRotation();
            
            FVector ToLocation = (Location - CameraLocation).GetSafeNormal();
            FVector CameraForward = CameraRotation.Vector();
            
            float DotProduct = FVector::DotProduct(CameraForward, ToLocation);
            
            // Simple frustum check - object is in front of camera
            return DotProduct > 0.0f;
        }
    }
    
    return true; // Default to true if can't determine
}

float UPooledWeaponEffectsComponent::GetDistanceToPlayer(const FVector& Location) const
{
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            return FVector::Dist(PlayerPawn->GetActorLocation(), Location);
        }
    }
    
    return 0.0f;
}

bool UPooledWeaponEffectsComponent::IsEffectBudgetExceeded() const
{
    return FrameEffectCount >= (MaxConcurrentEffects / 3); // Limit per-frame spawning
}

void UPooledWeaponEffectsComponent::SetPerformanceMode(bool bHighPerformanceMode)
{
    if (bHighPerformanceMode)
    {
        // Reduce quality for performance
        MaxConcurrentEffects = FMath::Max(20, MaxConcurrentEffects * 0.7f);
        MaxEffectDistance *= 0.8f;
    }
    else
    {
        // Restore quality settings
        MaxConcurrentEffects = FMath::Min(100, MaxConcurrentEffects * 1.3f);
        MaxEffectDistance *= 1.2f;
    }
}

void UPooledWeaponEffectsComponent::CleanupPools()
{
    ReturnAllEffectsToPool();
    ActiveEffects.Empty();
    
    UE_LOG(LogPooledWeaponEffects, Log, TEXT("PooledWeaponEffectsComponent pools cleaned up"));
}

void UPooledWeaponEffectsComponent::ReturnEffectToPool(const FString& PoolName, AActor* Effect)
{
    if (!Effect || !PoolManager)
    {
        return;
    }

    // Find and remove from active effects
    for (int32 i = ActiveEffects.Num() - 1; i >= 0; i--)
    {
        if (ActiveEffects[i].EffectActor == Effect)
        {
            ProcessEffectReturn(ActiveEffects[i]);
            ActiveEffects.RemoveAt(i);
            break;
        }
    }
}

void UPooledWeaponEffectsComponent::ReturnAllEffectsToPool()
{
    for (const FActivePooledEffect& Effect : ActiveEffects)
    {
        ProcessEffectReturn(Effect);
    }
    ActiveEffects.Empty();
}

UAdvancedObjectPoolManager* UPooledWeaponEffectsComponent::GetPoolManager() const
{
    return PoolManager;
}

int32 UPooledWeaponEffectsComponent::GetActiveEffectCount() const
{
    return ActiveEffects.Num();
}

FPoolStatistics UPooledWeaponEffectsComponent::GetPoolStatistics(const FString& PoolName) const
{
    if (PoolManager)
    {
        return PoolManager->GetPoolStatistics(PoolName);
    }
    
    return FPoolStatistics();
}
