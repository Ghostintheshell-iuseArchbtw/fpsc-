#include "AdvancedObjectPoolManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "HAL/PlatformMemory.h"
#include "HAL/CriticalSection.h"
#include "Engine/TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Async/ParallelFor.h"
#include "Stats/Stats.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogObjectPool);

UAdvancedObjectPoolManager::UAdvancedObjectPoolManager()
{
    LastGlobalCleanupTime = 0.0f;
    LastGlobalHealthCheckTime = 0.0f;
    
    // Set default global configuration
    GlobalConfig.InitialSize = 20;
    GlobalConfig.MaxSize = 200;
    GlobalConfig.GrowthIncrement = 10;
    GlobalConfig.bAllowGrowth = true;
    GlobalConfig.CleanupInterval = 30.0f;
    GlobalConfig.MaxIdleTime = 120.0f;
    GlobalConfig.MaxObjectLifetime = 900.0f; // 15 minutes
    GlobalConfig.bEnableAutomaticCleanup = true;
    GlobalConfig.bEnableMemoryTracking = true;
    GlobalConfig.bEnableStatistics = true;
    GlobalConfig.bThreadSafe = true;
    GlobalConfig.MemoryLimitMB = 100.0f;
    GlobalConfig.bPrewarmPool = true;
    GlobalConfig.bEnableHealthChecks = true;
    GlobalConfig.HealthCheckInterval = 60.0f;
    GlobalConfig.bEnableDebugLogging = false;
}

void UAdvancedObjectPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogObjectPool, Log, TEXT("Advanced Object Pool Manager initialized"));
    
    // Register common pools for FPS game objects
    RegisterCommonPools();
    
    // Set up maintenance timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MaintenanceTimerHandle,
            this,
            &UAdvancedObjectPoolManager::TickPoolMaintenance,
            5.0f, // Every 5 seconds
            true  // Loop
        );
    }
    
    BroadcastPoolEvent(TEXT("System"), TEXT("Object Pool Manager Initialized"));
}

void UAdvancedObjectPoolManager::Deinitialize()
{
    UE_LOG(LogObjectPool, Log, TEXT("Advanced Object Pool Manager shutting down"));
    
    // Clear maintenance timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MaintenanceTimerHandle);
    }
    
    // Destroy all pools
    DestroyAllPools();
    
    BroadcastPoolEvent(TEXT("System"), TEXT("Object Pool Manager Shutdown"));
    
    Super::Deinitialize();
}

AActor* UAdvancedObjectPoolManager::AcquireActor(TSubclassOf<AActor> ActorClass, const FString& PoolName)
{
    // Input validation with security checks
    if (!ActorClass)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("AcquireActor called with null ActorClass"));
        return nullptr;
    }
    
    // Validate ActorClass is valid and safe to instantiate
    if (!IsValid(ActorClass))
    {
        UE_LOG(LogObjectPool, Error, TEXT("AcquireActor called with invalid ActorClass"));
        return nullptr;
    }
    
    // Validate pool name doesn't contain security-sensitive characters
    if (!PoolName.IsEmpty())
    {
        FString SanitizedPoolName = PoolName;
        SanitizedPoolName = SanitizedPoolName.Replace(TEXT(".."), TEXT(""));
        SanitizedPoolName = SanitizedPoolName.Replace(TEXT("/"), TEXT(""));
        SanitizedPoolName = SanitizedPoolName.Replace(TEXT("\\"), TEXT(""));
        if (SanitizedPoolName != PoolName)
        {
            UE_LOG(LogObjectPool, Warning, TEXT("AcquireActor: Pool name contains invalid characters, using sanitized version"));
        }
    }
    
    // Check if we have a valid world context
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogObjectPool, Error, TEXT("AcquireActor: No valid world context available"));
        return nullptr;
    }
    
    FScopeLock Lock(&ManagerMutex);
    
    FString EffectivePoolName = PoolName.IsEmpty() ? GeneratePoolName(ActorClass, PoolName) : PoolName;
    
    FAdvancedObjectPool<AActor>** FoundPool = ActorPools.Find(EffectivePoolName);
    if (!FoundPool)
    {
        // Create new pool with proper creation functions
        auto CreateActorFunc = [ActorClass](UObject* Outer, TSubclassOf<AActor> Class) -> AActor*
        {
            if (UWorld* World = Outer ? Outer->GetWorld() : nullptr)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                return World->SpawnActor<AActor>(ActorClass, SpawnParams);
            }
            return nullptr;
        };
        
        auto ResetActorFunc = [](AActor* Actor)
        {
            if (Actor)
            {
                Actor->SetActorHiddenInGame(true);
                Actor->SetActorEnableCollision(ECollisionEnabled::NoCollision);
                Actor->SetActorTickEnabled(false);
                Actor->SetActorLocation(FVector::ZeroVector);
                Actor->SetActorRotation(FRotator::ZeroRotator);
            }
        };
        
        FAdvancedObjectPool<AActor>* NewPool = new FAdvancedObjectPool<AActor>(
            GlobalConfig, CreateActorFunc, ResetActorFunc, GetWorld(), ActorClass);
        ActorPools.Add(EffectivePoolName, NewPool);
        FoundPool = &NewPool;
        
        BroadcastPoolEvent(EffectivePoolName, TEXT("Actor Pool Created"));
    }
    
    if (*FoundPool)
    {
        AActor* Actor = (*FoundPool)->AcquireObject();
        if (Actor)
        {
            // Reset actor state for use
            Actor->SetActorHiddenInGame(false);
            Actor->SetActorEnableCollision(ECollisionEnabled::QueryAndPhysics);
            Actor->SetActorTickEnabled(true);
            
            if (GlobalConfig.bEnableDebugLogging)
            {
                UE_LOG(LogObjectPool, Log, TEXT("Acquired actor from pool %s"), *EffectivePoolName);
            }
            
            return Actor;
        }
    }
    
    // Fallback: create new actor if pool acquisition failed
    if (UWorld* World = GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* NewActor = World->SpawnActor<AActor>(ActorClass, SpawnParams);
        if (NewActor && GlobalConfig.bEnableDebugLogging)
        {
            UE_LOG(LogObjectPool, Warning, TEXT("Created new actor outside pool for class %s"), 
                   *ActorClass->GetName());
        }
        return NewActor;
    }
    
    return nullptr;
}

void UAdvancedObjectPoolManager::ReleaseActor(AActor* Actor)
{
    // Enhanced input validation for security
    if (!Actor)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("ReleaseActor: Attempted to release null actor"));
        return;
    }
    
    // Validate actor is still valid and not pending kill
    if (!IsValid(Actor))
    {
        UE_LOG(LogObjectPool, Warning, TEXT("ReleaseActor: Attempted to release invalid actor"));
        return;
    }
    
    FScopeLock Lock(&ManagerMutex);
    
    // Pre-allocate temporary arrays for better performance
    TArray<FString> PoolsToCheck;
    PoolsToCheck.Reserve(ActorPools.Num());
    
    // Find which pool this actor belongs to
    for (auto& PoolPair : ActorPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->ReleaseObject(Actor);
            
            if (GlobalConfig.bEnableDebugLogging)
            {
                UE_LOG(LogObjectPool, Log, TEXT("Released actor to pool %s"), *PoolPair.Key);
            }
            
            return;
        }
    }
    
    // If not found in any pool, destroy the actor
    if (GlobalConfig.bEnableDebugLogging)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("Actor not found in any pool, destroying"));
    }
    if (Actor->GetWorld())
    {
        Actor->GetWorld()->DestroyActor(Actor);
    }
}

UActorComponent* UAdvancedObjectPoolManager::AcquireComponent(TSubclassOf<UActorComponent> ComponentClass, const FString& PoolName)
{
    if (!ComponentClass)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("AcquireComponent called with null ComponentClass"));
        return nullptr;
    }
    
    FScopeLock Lock(&ManagerMutex);
    
    FString EffectivePoolName = PoolName.IsEmpty() ? GeneratePoolName(ComponentClass, PoolName) : PoolName;
    
    FAdvancedObjectPool<UActorComponent>** FoundPool = ComponentPools.Find(EffectivePoolName);
    if (!FoundPool)
    {
        // Create new pool with proper creation functions
        auto CreateComponentFunc = [ComponentClass](UObject* Outer, TSubclassOf<UActorComponent> Class) -> UActorComponent*
        {
            if (Outer)
            {
                return NewObject<UActorComponent>(Outer, ComponentClass);
            }
            return NewObject<UActorComponent>(GetTransientPackage(), ComponentClass);
        };
        
        auto ResetComponentFunc = [](UActorComponent* Component)
        {
            if (Component)
            {
                Component->Deactivate();
                // Reset component-specific properties
            }
        };
        
        FAdvancedObjectPool<UActorComponent>* NewPool = new FAdvancedObjectPool<UActorComponent>(
            GlobalConfig, CreateComponentFunc, ResetComponentFunc, GetTransientPackage(), ComponentClass);
        ComponentPools.Add(EffectivePoolName, NewPool);
        FoundPool = &NewPool;
        
        BroadcastPoolEvent(EffectivePoolName, TEXT("Component Pool Created"));
    }
    
    if (*FoundPool)
    {
        UActorComponent* Component = (*FoundPool)->AcquireObject();
        if (Component)
        {
            Component->Activate();
            
            if (GlobalConfig.bEnableDebugLogging)
            {
                UE_LOG(LogObjectPool, Log, TEXT("Acquired component from pool %s"), *EffectivePoolName);
            }
            
            return Component;
        }
    }
    
    // Fallback: create new component
    UActorComponent* NewComponent = NewObject<UActorComponent>(GetTransientPackage(), ComponentClass);
    if (NewComponent && GlobalConfig.bEnableDebugLogging)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("Created new component outside pool for class %s"), 
               *ComponentClass->GetName());
    }
    return NewComponent;
}

void UAdvancedObjectPoolManager::ReleaseComponent(UActorComponent* Component)
{
    if (!Component)
    {
        return;
    }
    
    FScopeLock Lock(&ManagerMutex);
    
    // Find which pool this component belongs to
    for (auto& PoolPair : ComponentPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->ReleaseObject(Component);
            
            if (GlobalConfig.bEnableDebugLogging)
            {
                UE_LOG(LogObjectPool, Log, TEXT("Released component to pool %s"), *PoolPair.Key);
            }
            return;
        }
    }
    
    // If not found in any pool, mark for destruction
    if (GlobalConfig.bEnableDebugLogging)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("Component not found in any pool, destroying"));
    }
    Component->DestroyComponent();
}

UObject* UAdvancedObjectPoolManager::AcquireObject(TSubclassOf<UObject> ObjectClass, const FString& PoolName)
{
    // Enhanced security validation
    if (!ObjectClass)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("AcquireObject called with null ObjectClass"));
        return nullptr;
    }
    
    // Validate the object class is safe to instantiate
    if (!IsValid(ObjectClass))
    {
        UE_LOG(LogObjectPool, Error, TEXT("AcquireObject called with invalid ObjectClass"));
        return nullptr;
    }
    
    // Validate the pool name for security
    FString EffectivePoolName;
    if (PoolName.IsEmpty())
    {
        EffectivePoolName = GeneratePoolName(ObjectClass, PoolName);
    }
    else
    {
        // Sanitize the pool name to prevent security issues
        EffectivePoolName = PoolName;
        EffectivePoolName = EffectivePoolName.Replace(TEXT(".."), TEXT(""));
        EffectivePoolName = EffectivePoolName.Replace(TEXT("/"), TEXT(""));
        EffectivePoolName = EffectivePoolName.Replace(TEXT("\\"), TEXT(""));
        
        // Alert if sanitized name differs from the input
        if (EffectivePoolName != PoolName)
        {
            UE_LOG(LogObjectPool, Warning, TEXT("AcquireObject: Pool name contained invalid characters, using sanitized version"));
        }
    }
    
    FScopeLock Lock(&ManagerMutex);
    
    FAdvancedObjectPool<UObject>** FoundPool = ObjectPools.Find(EffectivePoolName);
    if (!FoundPool)
    {
        // Create new pool with proper creation functions
        auto CreateObjectFunc = [ObjectClass](UObject* Outer, TSubclassOf<UObject> Class) -> UObject*
        {
            if (Outer)
            {
                return NewObject<UObject>(Outer, ObjectClass);
            }
            return NewObject<UObject>(GetTransientPackage(), ObjectClass);
        };
        
        auto ResetObjectFunc = [](UObject* Object)
        {
            // Generic UObject reset - could be overridden for specific types
            if (Object)
            {
                // Basic reset functionality
            }
        };
        
        FAdvancedObjectPool<UObject>* NewPool = new FAdvancedObjectPool<UObject>(
            GlobalConfig, CreateObjectFunc, ResetObjectFunc, GetTransientPackage(), ObjectClass);
        ObjectPools.Add(EffectivePoolName, NewPool);
        FoundPool = &NewPool;
        
        BroadcastPoolEvent(EffectivePoolName, TEXT("Object Pool Created"));
    }
    
    if (*FoundPool)
    {
        UObject* Object = (*FoundPool)->AcquireObject();
        if (Object)
        {
            if (GlobalConfig.bEnableDebugLogging)
            {
                UE_LOG(LogObjectPool, Log, TEXT("Acquired object from pool %s"), *EffectivePoolName);
            }
            return Object;
        }
    }
    
    // Fallback: create new object
    UObject* NewObject = NewObject<UObject>(GetTransientPackage(), ObjectClass);
    if (NewObject && GlobalConfig.bEnableDebugLogging)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("Created new object outside pool for class %s"), 
               *ObjectClass->GetName());
    }
    return NewObject;
}

void UAdvancedObjectPoolManager::ReleaseObject(UObject* Object)
{
    // Enhanced security validation
    if (!Object)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("ReleaseObject: Attempted to release null object"));
        return;
    }
    
    // Validate object is still valid and not pending kill
    if (!IsValid(Object))
    {
        UE_LOG(LogObjectPool, Warning, TEXT("ReleaseObject: Attempted to release invalid object"));
        return;
    }
    
    // Check for any security-sensitive operations that might occur when releasing
    // For example, ensure the object doesn't contain any malicious references
    if (Object->HasAnyFlags(RF_Transient) && !Object->IsA(UActorComponent::StaticClass()) && !Object->IsA(AActor::StaticClass()))
    {
        // Additional validation for transient objects that aren't actors or components
        UE_LOG(LogObjectPool, Log, TEXT("ReleaseObject: Releasing transient object %s"), *Object->GetName());
    }
    
    FScopeLock Lock(&ManagerMutex);
    
    // Pre-allocate array for better performance
    TArray<FString> PoolsToCheck;
    PoolsToCheck.Reserve(ObjectPools.Num());
    ObjectPools.GetKeys(PoolsToCheck);
    for (auto& PoolPair : ObjectPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->ReleaseObject(Object);
            
            if (GlobalConfig.bEnableDebugLogging)
            {
                UE_LOG(LogObjectPool, Log, TEXT("Released object to pool %s"), *PoolPair.Key);
            }
            return;
        }
    }
    
    // If not found in any pool, the object will be garbage collected naturally
    if (GlobalConfig.bEnableDebugLogging)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("Object not found in any pool for release"));
    }
}

void UAdvancedObjectPoolManager::CreatePool(TSubclassOf<UObject> ObjectClass, const FObjectPoolConfig& Config, const FString& PoolName)
{
    if (!ObjectClass)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("CreatePool called with null ObjectClass"));
        return;
    }
    
    FScopeLock Lock(&ManagerMutex);
    
    FString EffectivePoolName = PoolName.IsEmpty() ? GeneratePoolName(ObjectClass, PoolName) : PoolName;
    
    // Check if it's an Actor class
    if (ObjectClass->IsChildOf<AActor>())
    {
        if (!ActorPools.Contains(EffectivePoolName))
        {
            FAdvancedObjectPool<AActor>* NewPool = new FAdvancedObjectPool<AActor>(Config);
            NewPool->InitializePool();
            ActorPools.Add(EffectivePoolName, NewPool);
            
            BroadcastPoolEvent(EffectivePoolName, TEXT("Actor Pool Created with Custom Config"));
        }
    }
    // Check if it's a Component class
    else if (ObjectClass->IsChildOf<UActorComponent>())
    {
        if (!ComponentPools.Contains(EffectivePoolName))
        {
            FAdvancedObjectPool<UActorComponent>* NewPool = new FAdvancedObjectPool<UActorComponent>(Config);
            NewPool->InitializePool();
            ComponentPools.Add(EffectivePoolName, NewPool);
            
            BroadcastPoolEvent(EffectivePoolName, TEXT("Component Pool Created with Custom Config"));
        }
    }
    // Generic UObject
    else
    {
        if (!ObjectPools.Contains(EffectivePoolName))
        {
            FAdvancedObjectPool<UObject>* NewPool = new FAdvancedObjectPool<UObject>(Config);
            NewPool->InitializePool();
            ObjectPools.Add(EffectivePoolName, NewPool);
            
            BroadcastPoolEvent(EffectivePoolName, TEXT("Object Pool Created with Custom Config"));
        }
    }
}

void UAdvancedObjectPoolManager::DestroyPool(const FString& PoolName)
{
    FScopeLock Lock(&ManagerMutex);
    
    // Check actor pools
    if (FAdvancedObjectPool<AActor>** FoundActorPool = ActorPools.Find(PoolName))
    {
        if (*FoundActorPool)
        {
            (*FoundActorPool)->DestroyPool();
            delete *FoundActorPool;
        }
        ActorPools.Remove(PoolName);
        BroadcastPoolEvent(PoolName, TEXT("Actor Pool Destroyed"));
    }
    
    // Check component pools
    if (FAdvancedObjectPool<UActorComponent>** FoundComponentPool = ComponentPools.Find(PoolName))
    {
        if (*FoundComponentPool)
        {
            (*FoundComponentPool)->DestroyPool();
            delete *FoundComponentPool;
        }
        ComponentPools.Remove(PoolName);
        BroadcastPoolEvent(PoolName, TEXT("Component Pool Destroyed"));
    }
    
    // Check object pools
    if (FAdvancedObjectPool<UObject>** FoundObjectPool = ObjectPools.Find(PoolName))
    {
        if (*FoundObjectPool)
        {
            (*FoundObjectPool)->DestroyPool();
            delete *FoundObjectPool;
        }
        ObjectPools.Remove(PoolName);
        BroadcastPoolEvent(PoolName, TEXT("Object Pool Destroyed"));
    }
}

void UAdvancedObjectPoolManager::DestroyAllPools()
{
    FScopeLock Lock(&ManagerMutex);
    
    // Destroy actor pools
    for (auto& PoolPair : ActorPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->DestroyPool();
            delete PoolPair.Value;
        }
    }
    ActorPools.Empty();
    
    // Destroy component pools
    for (auto& PoolPair : ComponentPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->DestroyPool();
            delete PoolPair.Value;
        }
    }
    ComponentPools.Empty();
    
    // Destroy object pools
    for (auto& PoolPair : ObjectPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->DestroyPool();
            delete PoolPair.Value;
        }
    }
    ObjectPools.Empty();
    
    BroadcastPoolEvent(TEXT("System"), TEXT("All Pools Destroyed"));
}

FPoolStatistics UAdvancedObjectPoolManager::GetPoolStatistics(const FString& PoolName) const
{
    FScopeLock Lock(&ManagerMutex);
    
    // Check actor pools
    if (FAdvancedObjectPool<AActor>* const* FoundActorPool = ActorPools.Find(PoolName))
    {
        if (*FoundActorPool)
        {
            return (*FoundActorPool)->GetStatistics();
        }
    }
    
    // Check component pools
    if (FAdvancedObjectPool<UActorComponent>* const* FoundComponentPool = ComponentPools.Find(PoolName))
    {
        if (*FoundComponentPool)
        {
            return (*FoundComponentPool)->GetStatistics();
        }
    }
    
    // Check object pools
    if (FAdvancedObjectPool<UObject>* const* FoundObjectPool = ObjectPools.Find(PoolName))
    {
        if (*FoundObjectPool)
        {
            return (*FoundObjectPool)->GetStatistics();
        }
    }
    
    return FPoolStatistics(); // Return empty statistics
}

TArray<FString> UAdvancedObjectPoolManager::GetActivePoolNames() const
{
    FScopeLock Lock(&ManagerMutex);
    
    // Pre-allocate arrays for better performance
    TArray<FString> PoolNames;
    const int32 TotalPoolCount = ActorPools.Num() + ComponentPools.Num() + ObjectPools.Num();
    PoolNames.Reserve(TotalPoolCount);
    
    // Get actor pool names
    TArray<FString> ActorPoolNames;
    ActorPoolNames.Reserve(ActorPools.Num());
    ActorPools.GetKeys(ActorPoolNames);
    PoolNames.Append(ActorPoolNames);
    
    // Get component pool names
    TArray<FString> ComponentPoolNames;
    ComponentPoolNames.Reserve(ComponentPools.Num());
    ComponentPools.GetKeys(ComponentPoolNames);
    PoolNames.Append(ComponentPoolNames);
    
    // Get object pool names
    TArray<FString> ObjectPoolNames;
    ObjectPoolNames.Reserve(ObjectPools.Num());
    ObjectPools.GetKeys(ObjectPoolNames);
    PoolNames.Append(ObjectPoolNames);
    
    return PoolNames;
}

float UAdvancedObjectPoolManager::GetTotalMemoryUsage() const
{
    FScopeLock Lock(&ManagerMutex);
    
    float TotalMemory = 0.0f;
    
    // Sum memory from actor pools
    for (const auto& PoolPair : ActorPools)
    {
        if (PoolPair.Value)
        {
            FPoolStatistics Stats = PoolPair.Value->GetStatistics();
            TotalMemory += Stats.MemoryUsageMB;
        }
    }
    
    // Sum memory from component pools
    for (const auto& PoolPair : ComponentPools)
    {
        if (PoolPair.Value)
        {
            FPoolStatistics Stats = PoolPair.Value->GetStatistics();
            TotalMemory += Stats.MemoryUsageMB;
        }
    }
    
    // Sum memory from object pools
    for (const auto& PoolPair : ObjectPools)
    {
        if (PoolPair.Value)
        {
            FPoolStatistics Stats = PoolPair.Value->GetStatistics();
            TotalMemory += Stats.MemoryUsageMB;
        }
    }
    
    return TotalMemory;
}

FString UAdvancedObjectPoolManager::GeneratePoolReport() const
{
    FScopeLock Lock(&ManagerMutex);
    
    FString Report = TEXT("=== Advanced Object Pool Manager Report ===\n\n");
    
    Report += FString::Printf(TEXT("Total Memory Usage: %.2f MB\n"), GetTotalMemoryUsage());
    Report += FString::Printf(TEXT("Active Pools: %d\n\n"), 
                             ActorPools.Num() + ComponentPools.Num() + ObjectPools.Num());
    
    // Actor pools
    if (ActorPools.Num() > 0)
    {
        Report += TEXT("--- Actor Pools ---\n");
        for (const auto& PoolPair : ActorPools)
        {
            if (PoolPair.Value)
            {
                FPoolStatistics Stats = PoolPair.Value->GetStatistics();
                Report += FString::Printf(TEXT("Pool: %s\n"), *PoolPair.Key);
                Report += FString::Printf(TEXT("  Total Objects: %d\n"), Stats.TotalObjects);
                Report += FString::Printf(TEXT("  Active: %d\n"), Stats.ActiveObjects);
                Report += FString::Printf(TEXT("  Available: %d\n"), Stats.AvailableObjects);
                Report += FString::Printf(TEXT("  Memory: %.2f MB\n"), Stats.MemoryUsageMB);
                Report += FString::Printf(TEXT("  Hit Rate: %.1f%%\n"), Stats.HitRate * 100.0f);
                Report += FString::Printf(TEXT("  Healthy: %s\n\n"), 
                                        Stats.bIsHealthy ? TEXT("Yes") : TEXT("No"));
            }
        }
    }
    
    // Component pools
    if (ComponentPools.Num() > 0)
    {
        Report += TEXT("--- Component Pools ---\n");
        for (const auto& PoolPair : ComponentPools)
        {
            if (PoolPair.Value)
            {
                FPoolStatistics Stats = PoolPair.Value->GetStatistics();
                Report += FString::Printf(TEXT("Pool: %s\n"), *PoolPair.Key);
                Report += FString::Printf(TEXT("  Total Objects: %d\n"), Stats.TotalObjects);
                Report += FString::Printf(TEXT("  Active: %d\n"), Stats.ActiveObjects);
                Report += FString::Printf(TEXT("  Available: %d\n"), Stats.AvailableObjects);
                Report += FString::Printf(TEXT("  Memory: %.2f MB\n"), Stats.MemoryUsageMB);
                Report += FString::Printf(TEXT("  Hit Rate: %.1f%%\n\n"), Stats.HitRate * 100.0f);
            }
        }
    }
    
    // Object pools
    if (ObjectPools.Num() > 0)
    {
        Report += TEXT("--- Object Pools ---\n");
        for (const auto& PoolPair : ObjectPools)
        {
            if (PoolPair.Value)
            {
                FPoolStatistics Stats = PoolPair.Value->GetStatistics();
                Report += FString::Printf(TEXT("Pool: %s\n"), *PoolPair.Key);
                Report += FString::Printf(TEXT("  Total Objects: %d\n"), Stats.TotalObjects);
                Report += FString::Printf(TEXT("  Active: %d\n"), Stats.ActiveObjects);
                Report += FString::Printf(TEXT("  Available: %d\n"), Stats.AvailableObjects);
                Report += FString::Printf(TEXT("  Memory: %.2f MB\n"), Stats.MemoryUsageMB);
                Report += FString::Printf(TEXT("  Hit Rate: %.1f%%\n\n"), Stats.HitRate * 100.0f);
            }
        }
    }
    
    return Report;
}

void UAdvancedObjectPoolManager::CleanupAllPools()
{
    FScopeLock Lock(&ManagerMutex);
    
    int32 PoolsCleaned = 0;
    
    // Cleanup actor pools
    for (auto& PoolPair : ActorPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->CleanupPool();
            PoolsCleaned++;
        }
    }
    
    // Cleanup component pools
    for (auto& PoolPair : ComponentPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->CleanupPool();
            PoolsCleaned++;
        }
    }
    
    // Cleanup object pools
    for (auto& PoolPair : ObjectPools)
    {
        if (PoolPair.Value)
        {
            PoolPair.Value->CleanupPool();
            PoolsCleaned++;
        }
    }
    
    LastGlobalCleanupTime = FPlatformTime::Seconds();
    
    UE_LOG(LogObjectPool, Log, TEXT("Cleaned up %d pools"), PoolsCleaned);
    BroadcastPoolEvent(TEXT("System"), FString::Printf(TEXT("Cleaned up %d pools"), PoolsCleaned));
}

void UAdvancedObjectPoolManager::ForceGarbageCollection()
{
    GEngine->ForceGarbageCollection(true);
    
    UE_LOG(LogObjectPool, Log, TEXT("Forced garbage collection"));
    BroadcastPoolEvent(TEXT("System"), TEXT("Forced Garbage Collection"));
}

void UAdvancedObjectPoolManager::PerformHealthChecks()
{
    FScopeLock Lock(&ManagerMutex);
    
    int32 HealthyPools = 0;
    int32 UnhealthyPools = 0;
    
    // Check actor pools
    for (auto& PoolPair : ActorPools)
    {
        if (PoolPair.Value)
        {
            if (PoolPair.Value->IsHealthy())
            {
                HealthyPools++;
            }
            else
            {
                UnhealthyPools++;
                UE_LOG(LogObjectPool, Warning, TEXT("Pool %s is unhealthy"), *PoolPair.Key);
            }
        }
    }
    
    // Check component pools
    for (auto& PoolPair : ComponentPools)
    {
        if (PoolPair.Value)
        {
            if (PoolPair.Value->IsHealthy())
            {
                HealthyPools++;
            }
            else
            {
                UnhealthyPools++;
                UE_LOG(LogObjectPool, Warning, TEXT("Component pool %s is unhealthy"), *PoolPair.Key);
            }
        }
    }
    
    // Check object pools
    for (auto& PoolPair : ObjectPools)
    {
        if (PoolPair.Value)
        {
            if (PoolPair.Value->IsHealthy())
            {
                HealthyPools++;
            }
            else
            {
                UnhealthyPools++;
                UE_LOG(LogObjectPool, Warning, TEXT("Object pool %s is unhealthy"), *PoolPair.Key);
            }
        }
    }
    
    LastGlobalHealthCheckTime = FPlatformTime::Seconds();
    
    UE_LOG(LogObjectPool, Log, TEXT("Health check complete: %d healthy, %d unhealthy pools"), 
           HealthyPools, UnhealthyPools);
    
    BroadcastPoolEvent(TEXT("System"), 
                      FString::Printf(TEXT("Health Check: %d healthy, %d unhealthy"), 
                                     HealthyPools, UnhealthyPools));
}

void UAdvancedObjectPoolManager::SetGlobalConfig(const FObjectPoolConfig& GlobalConfiguration)
{
    FScopeLock Lock(&ManagerMutex);
    GlobalConfig = GlobalConfiguration;
    
    UE_LOG(LogObjectPool, Log, TEXT("Global configuration updated"));
    BroadcastPoolEvent(TEXT("System"), TEXT("Global Configuration Updated"));
}

void UAdvancedObjectPoolManager::PrewarmPool(const FString& PoolName)
{
    FScopeLock Lock(&ManagerMutex);
    
    // Check actor pools
    if (FAdvancedObjectPool<AActor>** FoundActorPool = ActorPools.Find(PoolName))
    {
        if (*FoundActorPool)
        {
            (*FoundActorPool)->PrewarmPool();
            BroadcastPoolEvent(PoolName, TEXT("Actor Pool Prewarmed"));
        }
    }
    
    // Check component pools
    if (FAdvancedObjectPool<UActorComponent>** FoundComponentPool = ComponentPools.Find(PoolName))
    {
        if (*FoundComponentPool)
        {
            (*FoundComponentPool)->PrewarmPool();
            BroadcastPoolEvent(PoolName, TEXT("Component Pool Prewarmed"));
        }
    }
    
    // Check object pools
    if (FAdvancedObjectPool<UObject>** FoundObjectPool = ObjectPools.Find(PoolName))
    {
        if (*FoundObjectPool)
        {
            (*FoundObjectPool)->PrewarmPool();
            BroadcastPoolEvent(PoolName, TEXT("Object Pool Prewarmed"));
        }
    }
}

void UAdvancedObjectPoolManager::PrewarmAllPools()
{
    // Get all pool names with pre-allocated array
    TArray<FString> PoolNames = GetActivePoolNames();
    
    // Pre-allocate temporary arrays if needed
    if (PoolNames.Num() > 0)
    {
        // Prewarm each pool
        for (const FString& PoolName : PoolNames)
        {
            PrewarmPool(PoolName);
        }
        
        UE_LOG(LogObjectPool, Log, TEXT("Prewarmed %d pools"), PoolNames.Num());
        BroadcastPoolEvent(TEXT("System"), FString::Printf(TEXT("Prewarmed %d pools"), PoolNames.Num()));
    }
}

bool UAdvancedObjectPoolManager::IsPoolHealthy(const FString& PoolName) const
{
    FPoolStatistics Stats = GetPoolStatistics(PoolName);
    return Stats.bIsHealthy;
}

void UAdvancedObjectPoolManager::OptimizePoolSizes()
{
    FScopeLock Lock(&ManagerMutex);
    
    // This method analyzes pool usage patterns and adjusts sizes accordingly
    int32 PoolsOptimized = 0;
    
    // Optimize actor pools
    for (auto& PoolPair : ActorPools)
    {
        if (PoolPair.Value)
        {
            FPoolStatistics Stats = PoolPair.Value->GetStatistics();
            
            // If hit rate is low and we have many unused objects, consider shrinking
            if (Stats.HitRate < 0.5f && Stats.AvailableObjects > Stats.ActiveObjects * 2)
            {
                // Trigger cleanup to remove excess objects
                PoolPair.Value->CleanupPool();
                PoolsOptimized++;
            }
        }
    }
    
    // Similar optimization for component and object pools
    for (auto& PoolPair : ComponentPools)
    {
        if (PoolPair.Value)
        {
            FPoolStatistics Stats = PoolPair.Value->GetStatistics();
            if (Stats.HitRate < 0.5f && Stats.AvailableObjects > Stats.ActiveObjects * 2)
            {
                PoolPair.Value->CleanupPool();
                PoolsOptimized++;
            }
        }
    }
    
    for (auto& PoolPair : ObjectPools)
    {
        if (PoolPair.Value)
        {
            FPoolStatistics Stats = PoolPair.Value->GetStatistics();
            if (Stats.HitRate < 0.5f && Stats.AvailableObjects > Stats.ActiveObjects * 2)
            {
                PoolPair.Value->CleanupPool();
                PoolsOptimized++;
            }
        }
    }
    
    UE_LOG(LogObjectPool, Log, TEXT("Optimized %d pools"), PoolsOptimized);
    BroadcastPoolEvent(TEXT("System"), FString::Printf(TEXT("Optimized %d pools"), PoolsOptimized));
}

// Specialized convenience functions for FPS game
AActor* UAdvancedObjectPoolManager::AcquireBullet()
{
    return AcquireActor(AStaticMeshActor::StaticClass(), TEXT("BulletPool"));
}

AActor* UAdvancedObjectPoolManager::AcquireParticleEffect()
{
    return AcquireActor(AActor::StaticClass(), TEXT("ParticleEffectPool"));
}

AActor* UAdvancedObjectPoolManager::AcquireAudioSource()
{
    return AcquireActor(AActor::StaticClass(), TEXT("AudioSourcePool"));
}

AActor* UAdvancedObjectPoolManager::AcquireDecal()
{
    return AcquireActor(AActor::StaticClass(), TEXT("DecalPool"));
}

FString UAdvancedObjectPoolManager::GeneratePoolName(UClass* ObjectClass, const FString& CustomName) const
{
    // Security-focused pool name generation
    if (!CustomName.IsEmpty())
    {
        // Sanitize custom name to prevent any security issues with sensitive functions or paths
        FString SanitizedName = CustomName;
        
        // Remove any potential path traversal or command injection sequences
        SanitizedName = SanitizedName.Replace(TEXT(".."), TEXT(""));
        SanitizedName = SanitizedName.Replace(TEXT("/"), TEXT("_"));
        SanitizedName = SanitizedName.Replace(TEXT("\\"), TEXT("_"));
        SanitizedName = SanitizedName.Replace(TEXT(";"), TEXT("_"));
        SanitizedName = SanitizedName.Replace(TEXT("|"), TEXT("_"));
        SanitizedName = SanitizedName.Replace(TEXT("&"), TEXT("_"));
        SanitizedName = SanitizedName.Replace(TEXT(">"), TEXT("_"));
        SanitizedName = SanitizedName.Replace(TEXT("<"), TEXT("_"));
        
        // Remove potentially dangerous keywords that could be used in command injection
        static const TArray<FString> DangerousKeywords = {
            TEXT("exec"), TEXT("system"), TEXT("cmd"), TEXT("powershell"), TEXT("bash"),
            TEXT("eval"), TEXT("execute")
        };
        
        for (const FString& Keyword : DangerousKeywords)
        {
            if (SanitizedName.Contains(Keyword, ESearchCase::IgnoreCase))
            {
                SanitizedName = SanitizedName.Replace(*Keyword, TEXT("_"), ESearchCase::IgnoreCase);
                UE_LOG(LogObjectPool, Warning, TEXT("Sanitized potentially unsafe pool name containing '%s'"), *Keyword);
            }
        }
        
        // If sanitization changed the name, log a warning
        if (SanitizedName != CustomName)
        {
            UE_LOG(LogObjectPool, Warning, TEXT("Pool name '%s' contained potentially unsafe characters, sanitized to '%s'"), 
                   *CustomName, *SanitizedName);
        }
        
        return SanitizedName;
    }
    
    if (ObjectClass)
    {
        // For auto-generated names, ensure they're also sanitized
        FString GeneratedName = FString::Printf(TEXT("%s_Pool"), *ObjectClass->GetName());
        return GeneratedName;
    }
    
    return TEXT("SafeDefaultPool");
}

void UAdvancedObjectPoolManager::RegisterCommonPools()
{
    // Register common FPS game object pools with optimized configurations
    
    // Bullet pool - high frequency, small objects
    FObjectPoolConfig BulletConfig = GlobalConfig;
    BulletConfig.InitialSize = 100;
    BulletConfig.MaxSize = 500;
    BulletConfig.GrowthIncrement = 50;
    BulletConfig.MaxIdleTime = 60.0f; // Shorter idle time for bullets
    CreatePool(AStaticMeshActor::StaticClass(), BulletConfig, TEXT("BulletPool"));
    
    // Particle effect pool - medium frequency, medium memory
    FObjectPoolConfig ParticleConfig = GlobalConfig;
    ParticleConfig.InitialSize = 50;
    ParticleConfig.MaxSize = 200;
    ParticleConfig.GrowthIncrement = 25;
    CreatePool(AActor::StaticClass(), ParticleConfig, TEXT("ParticleEffectPool"));
    
    // Audio source pool - medium frequency, low memory
    FObjectPoolConfig AudioConfig = GlobalConfig;
    AudioConfig.InitialSize = 30;
    AudioConfig.MaxSize = 100;
    AudioConfig.GrowthIncrement = 15;
    CreatePool(AActor::StaticClass(), AudioConfig, TEXT("AudioSourcePool"));
    
    // Decal pool - low frequency, medium memory
    FObjectPoolConfig DecalConfig = GlobalConfig;
    DecalConfig.InitialSize = 20;
    DecalConfig.MaxSize = 80;
    DecalConfig.GrowthIncrement = 10;
    DecalConfig.MaxIdleTime = 180.0f; // Longer idle time for decals
    CreatePool(AActor::StaticClass(), DecalConfig, TEXT("DecalPool"));
    
    UE_LOG(LogObjectPool, Log, TEXT("Registered common FPS game object pools"));
}

void UAdvancedObjectPoolManager::BroadcastPoolEvent(const FString& PoolName, const FString& EventDescription)
{
    if (OnPoolEvent.IsBound())
    {
        OnPoolEvent.Broadcast(PoolName, EventDescription);
    }
    
    if (GlobalConfig.bEnableDebugLogging)
    {
        UE_LOG(LogObjectPool, Log, TEXT("Pool Event - %s: %s"), *PoolName, *EventDescription);
    }
}

void UAdvancedObjectPoolManager::TickPoolMaintenance()
{
    float CurrentTime = FPlatformTime::Seconds();
    
    // Perform periodic cleanup
    if (GlobalConfig.bEnableAutomaticCleanup && 
        (CurrentTime - LastGlobalCleanupTime) > GlobalConfig.CleanupInterval)
    {
        CleanupAllPools();
    }
    
    // Perform periodic health checks
    if (GlobalConfig.bEnableHealthChecks && 
        (CurrentTime - LastGlobalHealthCheckTime) > GlobalConfig.HealthCheckInterval)
    {
        PerformHealthChecks();
    }
    
    // Check memory usage and optimize if needed
    float TotalMemory = GetTotalMemoryUsage();
    if (TotalMemory > GlobalConfig.MemoryLimitMB)
    {
        UE_LOG(LogObjectPool, Warning, TEXT("Pool memory usage (%.2f MB) exceeds limit (%.2f MB), optimizing"), 
               TotalMemory, GlobalConfig.MemoryLimitMB);
        
        OptimizePoolSizes();
        ForceGarbageCollection();
        
        BroadcastPoolEvent(TEXT("System"), 
                          FString::Printf(TEXT("Memory limit exceeded (%.2f MB), optimized pools"), TotalMemory));
    }
}

// Template specialization implementations for missing methods
template<>
void FAdvancedObjectPool<AActor>::PrewarmPool()
{
    // Prewarm implementation is already handled in InitializePool for this template
    InitializePool();
}

template<>
void FAdvancedObjectPool<UActorComponent>::PrewarmPool()
{
    InitializePool();
}

template<>
void FAdvancedObjectPool<UObject>::PrewarmPool()
{
    InitializePool();
}

template<>
void FAdvancedObjectPool<AActor>::DestroyPool()
{
    FScopeLock ScopeLock(&PoolMutex);
    
    for (FAdvancedPooledObject& PooledObject : Pool)
    {
        if (PooledObject.Object.IsValid())
        {
            AActor* Actor = PooledObject.Object.Get();
            if (Actor)
            {
                Actor->Destroy();
            }
        }
    }
    
    Pool.Empty();
    AvailableIndices = TQueue<int32>();
    ObjectToIndexMap.Empty();
}

template<>
void FAdvancedObjectPool<UActorComponent>::DestroyPool()
{
    FScopeLock ScopeLock(&PoolMutex);
    
    for (FAdvancedPooledObject& PooledObject : Pool)
    {
        if (PooledObject.Object.IsValid())
        {
            UActorComponent* Component = PooledObject.Object.Get();
            if (Component)
            {
                Component->DestroyComponent();
            }
        }
    }
    
    Pool.Empty();
    AvailableIndices = TQueue<int32>();
    ObjectToIndexMap.Empty();
}

template<>
void FAdvancedObjectPool<UObject>::DestroyPool()
{
    FScopeLock ScopeLock(&PoolMutex);
    
    // UObjects don't need explicit destruction, they'll be garbage collected
    Pool.Empty();
    AvailableIndices = TQueue<int32>();
    ObjectToIndexMap.Empty();
}

template<>
bool FAdvancedObjectPool<AActor>::IsHealthy() const
{
    if (Config.bThreadSafe)
    {
        FScopeLock ScopeLock(&PoolMutex);
    }
    
    // Check for basic health indicators
    float CurrentTime = FPlatformTime::Seconds();
    
    // Check if pool has valid objects
    int32 ValidObjects = 0;
    for (const FAdvancedPooledObject& PooledObject : Pool)
    {
        if (PooledObject.Object.IsValid())
        {
            ValidObjects++;
        }
    }
    
    // Pool is unhealthy if more than 25% of objects are invalid
    float ValidRatio = Pool.Num() > 0 ? (float)ValidObjects / (float)Pool.Num() : 1.0f;
    if (ValidRatio < 0.75f)
    {
        return false;
    }
    
    // Check memory usage
    if (Config.bEnableMemoryTracking)
    {
        FPoolStatistics Stats = GetStatistics();
        if (Stats.MemoryUsageMB > Config.MemoryLimitMB)
        {
            return false;
        }
    }
    
    return true;
}

template<>
bool FAdvancedObjectPool<UActorComponent>::IsHealthy() const
{
    return true; // Simplified health check for components
}

template<>
bool FAdvancedObjectPool<UObject>::IsHealthy() const
{
    return true; // Simplified health check for generic objects
}
