#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/CriticalSection.h"
#include "Async/Async.h"
#include "Templates/SharedPointer.h"
#include "Containers/Queue.h"
#include "Engine/Engine.h"
#include "HAL/PlatformMemory.h"
#include "AdvancedObjectPoolManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogObjectPool, Log, All);

// Pool statistics for monitoring
USTRUCT(BlueprintType)
struct FPoolStatistics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 CurrentPooledObjects = 0; // Total objects currently in the pool (Available + Active)

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 ActiveObjects = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 AvailableObjects = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 PeakActiveObjects = 0; // Maximum number of concurrently active objects

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 MaxPoolSize = 0; // Configured maximum size

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    float MemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 TotalAcquisitions = 0; // Total times an object was acquired

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 TotalReturns = 0; // Total times an object was returned

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 TotalCreations = 0; // Total objects ever created by this pool

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 TotalDestructions = 0; // Total objects ever destroyed by this pool (due to cleanup or pool destruction)

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 CacheHits = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    int32 CacheMisses = 0; // Acquisitions that required new object creation or failed due to MaxPoolSize

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    float HitRate = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    double LastAcquisitionTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    double LastReleaseTimeSeconds = 0.0;
    
    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    double LastCleanupTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool Statistics")
    bool bIsHealthy = true;

    FPoolStatistics()
    {
        // Initialize all members
        CurrentPooledObjects = 0;
        ActiveObjects = 0;
        AvailableObjects = 0;
        PeakActiveObjects = 0;
        MaxPoolSize = 0;
        MemoryUsageMB = 0.0f;
        TotalAcquisitions = 0;
        TotalReturns = 0;
        TotalCreations = 0;
        TotalDestructions = 0;
        CacheHits = 0;
        CacheMisses = 0;
        HitRate = 0.0f;
        LastAcquisitionTimeSeconds = 0.0;
        LastReleaseTimeSeconds = 0.0;
        LastCleanupTimeSeconds = 0.0;
        bIsHealthy = true;
    }
};

// Advanced pooled object with comprehensive tracking
USTRUCT(BlueprintType)
struct FAdvancedPooledObject
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UObject> Object; // Weak pointer to the actual UObject

    UPROPERTY()
    bool bInUse = false;

    UPROPERTY()
    double CreationTime = 0.0f; // Timestamp of when this FAdvancedPooledObject wrapper was created (or object was first pooled)

    UPROPERTY()
    double AcquisitionTime = 0.0f; // Timestamp of last acquisition

    UPROPERTY()
    double LastUsedTime = 0.0f; // Timestamp of last release, can be used for idle calculation

    UPROPERTY()
    double TotalUsageTimeSeconds = 0.0f; // Cumulative time this object has been in use

    UPROPERTY()
    int32 UsageCount = 0;

    // UPROPERTY() // PoolIndex is implicit by its position in the TArray<FAdvancedPooledObject> Pool
    // int32 PoolIndex = -1; 

    UPROPERTY()
    bool bMarkedForDestruction = false;

    UPROPERTY()
    float MemoryFootprintKB = 0.0f;

    UPROPERTY()
    FString ObjectID; // For debugging

    UPROPERTY()
    FString CreationStackTrace; // For debugging memory leaks or creation contexts

    FAdvancedPooledObject()
    {
        Object = nullptr;
        bInUse = false;
        CreationTime = 0.0f;
        AcquisitionTime = 0.0f;
        LastUsedTime = 0.0f;
        TotalUsageTimeSeconds = 0.0f;
        UsageCount = 0;
        bMarkedForDestruction = false;
        MemoryFootprintKB = 0.0f;
        ObjectID = TEXT("");
        CreationStackTrace = TEXT("");
    }

    bool IsValid() const
    {
        return Object.IsValid() && !bMarkedForDestruction;
    }

    // Gets the lifetime of the UObject itself within the pool
    float GetLifetimeInPool(double CurrentTime) const
    {
        return CurrentTime - CreationTime;
    }

    // Gets the time since this object was last released (became idle)
    float GetIdleTime(double CurrentTime) const
    {
        return bInUse ? 0.0f : (CurrentTime - LastUsedTime);
    }
};

// Pool configuration settings
USTRUCT(BlueprintType)
struct FObjectPoolConfig
{
    GENERATED_BODY()

    // Basic settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 InitialSize = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 MaxSize = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 GrowthIncrement = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    bool bAllowGrowth = true;

    // Cleanup settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup")
    float CleanupInterval = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup")
    float MaxIdleTime = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup")
    float MaxObjectLifetime = 1800.0f; // 30 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanup")
    bool bEnableAutomaticCleanup = true;

    // Performance settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableMemoryTracking = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableStatistics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bThreadSafe = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MemoryLimitMB = 50.0f;

    // Advanced settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    bool bPrewarmPool = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    bool bEnableHealthChecks = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    float HealthCheckInterval = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    bool bEnableDebugLogging = false;

    FObjectPoolConfig()
    {
        InitialSize = 10;
        MaxSize = 100;
        GrowthIncrement = 5;
        bAllowGrowth = true;
        CleanupInterval = 60.0f;
        MaxIdleTime = 300.0f;
        MaxObjectLifetime = 1800.0f;
        bEnableAutomaticCleanup = true;
        bEnableMemoryTracking = true;
        bEnableStatistics = true;
        bThreadSafe = true;
        MemoryLimitMB = 50.0f;
        bPrewarmPool = true;
        bEnableHealthChecks = true;
        HealthCheckInterval = 30.0f;
        bEnableDebugLogging = false;
    }
};

// Generic object pool template
template<typename T>
class FAdvancedObjectPool
{
public:
    FAdvancedObjectPool(const FObjectPoolConfig& InConfig, TFunction<T*(UObject*, TSubclassOf<T>)> InCreateFunc, TFunction<void(T*)> InResetFunc, UObject* InDefaultOuter, TSubclassOf<T> InObjectClass);
    ~FAdvancedObjectPool();

    // Core functionality - FORCEINLINE for performance-critical methods
    FORCEINLINE T* AcquireObject();
    FORCEINLINE void ReleaseObject(T* ObjectToRelease);

    // Pool management
    void InitializePool();
    void PrewarmPool();
    void CleanupPool();
    void DestroyPool();

    // Statistics and monitoring - FORCEINLINE for frequent access
    FORCEINLINE FPoolStatistics GetStatistics() const;
    FORCEINLINE void UpdateStatistics();
    FORCEINLINE bool IsHealthy() const;
    void ResetStatistics();

    // Configuration
    void UpdateConfig(const FObjectPoolConfig& NewConfig);
    FObjectPoolConfig GetConfig() const { return Config; }

    // Thread safety
    void Lock() const { PoolMutex.Lock(); }
    void Unlock() const { PoolMutex.Unlock(); }

private:
    // Internal data
    FObjectPoolConfig Config;
    TArray<FAdvancedPooledObject> Pool; // Stores all objects, active or inactive
    TQueue<int32> AvailableIndices;     // Indices into 'Pool' for available objects
    TMap<T*, int32> ObjectToIndexMap;   // Maps an active T* to its index in 'Pool'
    
    // Statistics
    mutable FPoolStatistics Statistics;
    double LastHealthCheckTime; // Renamed from float
    bool bInitialized;

    // Thread safety
    mutable FCriticalSection PoolMutex;
    
    // Object creation/destruction context and functions
    TFunction<T*(UObject*, TSubclassOf<T>)> CreateObjectFunc;
    TFunction<void(T*)> ResetObjectFunc;
    UObject* DefaultOuter; // Outer for creating new UObjects
    TSubclassOf<T> ObjectClass; // Class of object this pool manages

    // Internal methods
    T* CreateNewObjectInternal(); // Uses CreateObjectFunc
    void ResetObjectInternal(T* ObjectToReset); // Uses ResetObjectFunc
    void DestroyObjectInternal(int32 PoolIndex); // Handles actual destruction
    bool ShouldCleanupObject(const FAdvancedPooledObject& PooledObject, double CurrentTime) const;
};

// Specialized pools for common Unreal Engine types
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPoolEvent, const FString&, PoolName, const FString&, EventDescription);

UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UAdvancedObjectPoolManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UAdvancedObjectPoolManager();

    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Pool management for Actors
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    AActor* AcquireActor(TSubclassOf<AActor> ActorClass, const FString& PoolName = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ReleaseActor(AActor* Actor);

    // Pool management for Components
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    UActorComponent* AcquireComponent(TSubclassOf<UActorComponent> ComponentClass, const FString& PoolName = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ReleaseComponent(UActorComponent* Component);

    // Pool management for UObjects
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    UObject* AcquireObject(TSubclassOf<UObject> ObjectClass, const FString& PoolName = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ReleaseObject(UObject* Object);

    // Pool configuration
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void CreatePool(TSubclassOf<UObject> ObjectClass, const FObjectPoolConfig& Config, const FString& PoolName = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void DestroyPool(const FString& PoolName);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void DestroyAllPools();

    // Statistics and monitoring
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    FPoolStatistics GetPoolStatistics(const FString& PoolName) const;

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    TArray<FString> GetActivePoolNames() const;

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    float GetTotalMemoryUsage() const;

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    FString GeneratePoolReport() const;

    // Maintenance
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void CleanupAllPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ForceGarbageCollection();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void PerformHealthChecks();

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void SetGlobalConfig(const FObjectPoolConfig& GlobalConfig);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    FObjectPoolConfig GetGlobalConfig() const { return GlobalConfig; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPoolEvent OnPoolEvent;

    // Advanced functionality
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void PrewarmPool(const FString& PoolName);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void PrewarmAllPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    bool IsPoolHealthy(const FString& PoolName) const;

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void OptimizePoolSizes();

    // Specialized convenience functions for FPS game
    UFUNCTION(BlueprintCallable, Category = "FPS Pool")
    AActor* AcquireBullet();

    UFUNCTION(BlueprintCallable, Category = "FPS Pool")
    AActor* AcquireParticleEffect();

    UFUNCTION(BlueprintCallable, Category = "FPS Pool")
    AActor* AcquireAudioSource();

    UFUNCTION(BlueprintCallable, Category = "FPS Pool")
    AActor* AcquireDecal();

protected:
    // Internal pool storage
    UPROPERTY()
    TMap<FString, FAdvancedObjectPool<AActor>*> ActorPools;

    UPROPERTY()
    TMap<FString, FAdvancedObjectPool<UActorComponent>*> ComponentPools;

    UPROPERTY()
    TMap<FString, FAdvancedObjectPool<UObject>*> ObjectPools;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FObjectPoolConfig GlobalConfig;

    // Monitoring
    float LastGlobalCleanupTime;
    float LastGlobalHealthCheckTime;

    // Thread safety
    mutable FCriticalSection ManagerMutex;

private:
    // Internal methods
    FString GeneratePoolName(UClass* ObjectClass, const FString& CustomName) const;
    void RegisterCommonPools();
    void BroadcastPoolEvent(const FString& PoolName, const FString& EventDescription);
    void TickPoolMaintenance();

    // Timer handle for maintenance
    FTimerHandle MaintenanceTimerHandle;
};

// Template implementation
template<typename T>
FAdvancedObjectPool<T>::FAdvancedObjectPool(const FObjectPoolConfig& InConfig, TFunction<T*(UObject*, TSubclassOf<T>)> InCreateFunc, TFunction<void(T*)> InResetFunc, UObject* InDefaultOuter, TSubclassOf<T> InObjectClass)
    : Config(InConfig)
    , CreateObjectFunc(InCreateFunc)
    , ResetObjectFunc(InResetFunc)
    , DefaultOuter(InDefaultOuter)
    , ObjectClass(InObjectClass)
    , bInitialized(false)
    , LastHealthCheckTime(0.0)
{
    Statistics = FPoolStatistics(); // Initialize statistics
    Statistics.MaxPoolSize = Config.MaxSize;
    // LastCleanupTime is part of FPoolStatistics, initialized in its constructor.

    if (Config.bPrewarmPool)
    {
        InitializePool();
    }
}

template<typename T>
FAdvancedObjectPool<T>::~FAdvancedObjectPool()
{
    DestroyPool();
}

template<typename T>
T* FAdvancedObjectPool<T>::AcquireObject()
{
    FScopeLock Lock(&PoolMutex); 

    if (!bInitialized)
    {
        InitializePool(); // Ensure pool is ready
    }

    double CurrentTime = FPlatformTime::Seconds();
    int32 ObjectIndex = INDEX_NONE;
    T* AcquiredObjectPtr = nullptr;

    if (!AvailableIndices.IsEmpty())
    {
        AvailableIndices.Dequeue(ObjectIndex);
        Statistics.CacheHits++;
    }
    else if (Pool.Num() < Config.MaxSize || Config.MaxSize <= 0) // Allow growth if MaxSize is 0 or not reached
    {
        if (Config.bAllowGrowth)
        {
            T* NewRawObject = CreateNewObjectInternal();
            if (NewRawObject)
            {
                ObjectIndex = Pool.Emplace(); // Add new FAdvancedPooledObject to Pool array
                FAdvancedPooledObject& NewPooledObject = Pool[ObjectIndex];
                NewPooledObject.Object = NewRawObject;
                NewPooledObject.CreationTime = CurrentTime;
                NewPooledObject.ObjectID = FString::Printf(TEXT("%s_PoolObj_%d"), *ObjectClass->GetName(), ObjectIndex);
                // MemoryFootprintKB should be calculated here or in CreateNewObjectInternal
                if (Config.bEnableMemoryTracking)
                {
                     NewPooledObject.MemoryFootprintKB = CalculateMemoryFootprint(NewRawObject);
                }
                Statistics.TotalCreations++;
                Statistics.CurrentPooledObjects = Pool.Num();
            }
            Statistics.CacheMisses++; // Cache miss because we had to create a new one
        }
        else
        {
            Statistics.CacheMisses++; // Cannot grow, still a cache miss
            if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s]: At max capacity and growth disabled. Cannot acquire object."), *ObjectClass->GetName());
            return nullptr;
        }
    }
    else
    {
        Statistics.CacheMisses++; // At max capacity
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s]: At max capacity. Cannot acquire object."), *ObjectClass->GetName());
        return nullptr;
    }

    if (ObjectIndex != INDEX_NONE && Pool.IsValidIndex(ObjectIndex))
    {
        FAdvancedPooledObject& PooledObject = Pool[ObjectIndex];
        if (PooledObject.Object.IsValid())
        {
            AcquiredObjectPtr = Cast<T>(PooledObject.Object.Get());
            if(AcquiredObjectPtr)
            {
                PooledObject.bInUse = true;
                PooledObject.AcquisitionTime = CurrentTime; // Record acquisition time
                PooledObject.UsageCount++;

                ObjectToIndexMap.Add(AcquiredObjectPtr, ObjectIndex);

                Statistics.ActiveObjects++;
                Statistics.AvailableObjects = AvailableIndices.Num(); // Update available count
                Statistics.TotalAcquisitions++;
                Statistics.LastAcquisitionTimeSeconds = CurrentTime;
                if (Statistics.ActiveObjects > Statistics.PeakActiveObjects)
                {
                    Statistics.PeakActiveObjects = Statistics.ActiveObjects;
                }
                
                // Call reset function to ensure object is in a clean state
                // This is typically done on release for reuse, but can also be done on acquire
                // ResetObjectInternal(AcquiredObjectPtr); 

                if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Acquired object %s (Index: %d)"), *ObjectClass->GetName(), *AcquiredObjectPtr->GetName(), ObjectIndex);
            }
            else
            {
                 // Should not happen if Object.IsValid() was true and Cast failed
                 if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Error, TEXT("Pool [%s]: Failed to cast UObject to type T for object at index %d."), *ObjectClass->GetName(), ObjectIndex);
                 // Put index back if cast failed unexpectedly
                 AvailableIndices.Enqueue(ObjectIndex);
                 Statistics.CacheHits--; // Revert cache hit
            }
        }
        else
        {
            // Object became invalid while in available queue, should be rare
            // This could be handled by a health check removing it, or here by trying again
            if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s]: Object at index %d was invalid when dequeued. Attempting to acquire another."), *ObjectClass->GetName(), ObjectIndex);
            Statistics.CacheHits--; // Revert cache hit
            Statistics.TotalDestructions++; // Consider it destroyed
            Pool[ObjectIndex].bMarkedForDestruction = true; // Mark for proper cleanup later
            return AcquireObject(); // Try again
        }
    }
    UpdateStatistics(); // General update
    return AcquiredObjectPtr;
}

template<typename T>
void FAdvancedObjectPool<T>::ReleaseObject(T* ObjectToRelease)
{
    if (!ObjectToRelease)
    {
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s]: Attempted to release a null object."), *ObjectClass->GetName());
        return;
    }

    FScopeLock Lock(&PoolMutex);
    double CurrentTime = FPlatformTime::Seconds();

    const int32* ObjectIndexPtr = ObjectToIndexMap.Find(ObjectToRelease);
    if (!ObjectIndexPtr)
    {
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s]: Attempted to release object %s not managed by this pool or already released."), *ObjectClass->GetName(), *ObjectToRelease->GetName());
        return;
    }

    int32 ObjectIndex = *ObjectIndexPtr;
    if (!Pool.IsValidIndex(ObjectIndex))
    {
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Error, TEXT("Pool [%s]: Invalid index %d found for object %s during release."),*ObjectClass->GetName(), ObjectIndex, *ObjectToRelease->GetName());
        ObjectToIndexMap.Remove(ObjectToRelease); // Clean up map
        return;
    }

    FAdvancedPooledObject& PooledObject = Pool[ObjectIndex];

    if (!PooledObject.bInUse)
    {
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s]: Object %s (Index: %d) already in pool, attempted to release again."), *ObjectClass->GetName(), *ObjectToRelease->GetName(), ObjectIndex);
        return;
    }
    
    // Call user-defined reset logic
    ResetObjectInternal(ObjectToRelease);

    PooledObject.bInUse = false;
    PooledObject.LastUsedTime = CurrentTime; // Record time of release
    PooledObject.TotalUsageTimeSeconds += (CurrentTime - PooledObject.AcquisitionTime);

    ObjectToIndexMap.Remove(ObjectToRelease);
    AvailableIndices.Enqueue(ObjectIndex);

    Statistics.ActiveObjects--;
    Statistics.AvailableObjects = AvailableIndices.Num();
    Statistics.TotalReturns++;
    Statistics.LastReleaseTimeSeconds = CurrentTime;

    if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Released object %s (Index: %d). Available: %d"), *ObjectClass->GetName(), *ObjectToRelease->GetName(), ObjectIndex, AvailableIndices.Num());
    
    UpdateStatistics();
}

template<typename T>
void FAdvancedObjectPool<T>::InitializePool()
{
    FScopeLock Lock(&PoolMutex);

    if (bInitialized) return;

    double CurrentTime = FPlatformTime::Seconds();
    
    // Pre-allocate arrays for better performance
    Pool.Reserve(Config.MaxSize);
    Pool.Empty(Config.InitialSize);
    
    // Pre-allocate ObjectToIndexMap for better performance
    ObjectToIndexMap.Reserve(Config.MaxSize);
    
    AvailableIndices = TQueue<int32>(); // Ensure it's clean

    for (int32 i = 0; i < Config.InitialSize; ++i)
    {
        T* NewRawObject = CreateNewObjectInternal();
        if (NewRawObject)
        {
            int32 NewIndex = Pool.Emplace();
            FAdvancedPooledObject& PooledObject = Pool[NewIndex];
            PooledObject.Object = NewRawObject;
            PooledObject.CreationTime = CurrentTime;
            PooledObject.LastUsedTime = CurrentTime; // Initially idle
            PooledObject.ObjectID = FString::Printf(TEXT("%s_PoolObj_%d"), *ObjectClass->GetName(), NewIndex);
            if (Config.bEnableMemoryTracking)
            {
                PooledObject.MemoryFootprintKB = CalculateMemoryFootprint(NewRawObject);
            }
            
            AvailableIndices.Enqueue(NewIndex);
            Statistics.TotalCreations++;
        }
        else
        {
            if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Error, TEXT("Pool [%s]: Failed to create object during prewarming."), *ObjectClass->GetName());
            // Potentially break or handle error, pool might not reach InitialSize
        }
    }
    Statistics.CurrentPooledObjects = Pool.Num();
    Statistics.AvailableObjects = AvailableIndices.Num();
    bInitialized = true;
    if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Initialized with %d objects. Available: %d"), *ObjectClass->GetName(), Pool.Num(), AvailableIndices.Num());
}

template<typename T>
T* FAdvancedObjectPool<T>::CreateNewObjectInternal()
{
    if (!CreateObjectFunc)
    {
        UE_LOG(LogObjectPool, Error, TEXT("Pool [%s]: CreateObjectFunc is not set!"), *ObjectClass->GetName());
        return nullptr;
    }
    if (!ObjectClass)
    {
        UE_LOG(LogObjectPool, Error, TEXT("Pool [%s]: ObjectClass is not set!"), DefaultOuter ? *DefaultOuter->GetName() : TEXT("UnknownOuter"));
        return nullptr;
    }
     if (!DefaultOuter && !TIsSame<T, UObject>::Value) // UObjects might not need an outer if created carefully, but Actors/Components do.
    {
       // UE_LOG(LogObjectPool, Error, TEXT("Pool [%s]: DefaultOuter is not set for non-UObject type!"), *ObjectClass->GetName());
       // return nullptr; // This check might be too strict, depends on CreateObjectFunc
    }

    T* NewObj = CreateObjectFunc(DefaultOuter, ObjectClass);

    // Example for Actors:
    // if (AActor* NewActor = Cast<AActor>(NewObj)) {
    //    NewActor->SetActorHiddenInGame(true);
    //    NewActor->SetActorEnableCollision(false);
    // }
    if (NewObj && Config.bEnableDebugLogging)
    {
        UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Created new object %s"), *ObjectClass->GetName(), *NewObj->GetName());
    }
    else if(!NewObj)
    {
        UE_LOG(LogObjectPool, Error, TEXT("Pool [%s]: CreateObjectFunc returned nullptr."), *ObjectClass->GetName());
    }
    return NewObj;
}

template<typename T>
void FAdvancedObjectPool<T>::ResetObjectInternal(T* ObjectToReset)
{
    if (!ObjectToReset) return;

    if (ResetObjectFunc)
    {
        ResetObjectFunc(ObjectToReset);
    }
    else
    {
        // Default reset logic for known types if no custom func provided
        if (AActor* Actor = Cast<AActor>(ObjectToReset))
        {
            Actor->SetActorHiddenInGame(true);
            Actor->SetActorEnableCollision(ECollisionEnabled::NoCollision);
            Actor->SetActorTickEnabled(false);
            // Potentially reset velocity, detach from parent, etc.
        }
        else if (UActorComponent* Component = Cast<UActorComponent>(ObjectToReset))
        {
            Component->Deactivate();
            // Potentially reset component-specific properties
        }
    }
     if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Verbose, TEXT("Pool [%s]: Reset object %s"), *ObjectClass->GetName(), *ObjectToReset->GetName());
}

template<typename T>
void FAdvancedObjectPool<T>::DestroyObjectInternal(int32 PoolIndex)
{
    if (!Pool.IsValidIndex(PoolIndex)) return;

    FAdvancedPooledObject& PooledObject = Pool[PoolIndex];
    if (PooledObject.Object.IsValid())
    {
        UObject* RawObject = PooledObject.Object.Get();
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Destroying object %s (Index: %d)"), *ObjectClass->GetName(), *RawObject->GetName(), PoolIndex);

        // Actual destruction logic
        if (AActor* Actor = Cast<AActor>(RawObject))
        {
            if (Actor->GetWorld()) Actor->GetWorld()->DestroyActor(Actor);
        }
        else if (UActorComponent* Component = Cast<UActorComponent>(RawObject))
        {
            Component->DestroyComponent();
        }
        else // Generic UObject
        {
            // RawObject->MarkPendingKill(); // Or other appropriate destruction
            // For UObjects not part of the actor lifecycle, ensure they are GC'd
            // If it was created with NewObject<U>(Outer), it will be GC'd with its outer or if unreferenced.
            // If it needs explicit cleanup, that should be handled.
        }
        PooledObject.Object = nullptr; // Clear the weak ptr
        Statistics.TotalDestructions++;
    }
    PooledObject.bMarkedForDestruction = true; // Mark the slot as fully processed for destruction
}


template<typename T>
float FAdvancedObjectPool<T>::CalculateMemoryFootprint(T* Object) const
{
    if (!Object || !Config.bEnableMemoryTracking) return 0.0f;

    // This is a simplified example. Real memory footprint calculation can be complex.
    // FArchiveCountMem is one way, but might not be suitable for all objects or frequent calls.
    // Consider a configurable size per object type or a more lightweight estimation.
#if UE_ALLOW_ архивы_COUNT_MEM // This define might not exist, it's conceptual
    FArchiveCountMem Ar(Object);
    return static_cast<float>(Ar.GetMax()) / 1024.0f; // Return in KB
#else
    // Fallback or placeholder if FArchiveCountMem is too heavy or unavailable
    // For Actors, you might sum component sizes.
    // For UObjects, GetAllocatedSize is internal.
    // A very rough estimate or configurable value might be needed.
    if (AActor* Actor = Cast<AActor>(Object))
    {
        float ComponentsSize = 0.0f;
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Comp : Components)
        {
            if (Comp) ComponentsSize += 10.0f; // Placeholder: 10KB per component
        }
        return 50.0f + ComponentsSize; // Placeholder: 50KB base for an actor + components
    }
    return 20.0f; // Placeholder: 20KB for a generic UObject
#endif
}


template<typename T>
void FAdvancedObjectPool<T>::CleanupPool()
{
    FScopeLock Lock(&PoolMutex);
    
    double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - Statistics.LastCleanupTimeSeconds < Config.CleanupInterval && Config.CleanupInterval > 0)
    {
        return; // Not time to cleanup yet
    }

    if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Starting cleanup. Available: %d, Total: %d"), *ObjectClass->GetName(), AvailableIndices.Num(), Pool.Num());

    // Option 1: Clean only from available objects
    TQueue<int32> StillAvailableIndices;
    while(!AvailableIndices.IsEmpty())
    {
        int32 ObjectIndex;
        AvailableIndices.Dequeue(ObjectIndex);

        if (Pool.IsValidIndex(ObjectIndex))
        {
            FAdvancedPooledObject& PooledObject = Pool[ObjectIndex];
            if (ShouldCleanupObject(PooledObject, CurrentTime))
            {
                DestroyObjectInternal(ObjectIndex); // Destroys UObject and marks FAdvancedPooledObject
            }
            else
            {
                StillAvailableIndices.Enqueue(ObjectIndex); // Keep it
            }
        }
    }
    AvailableIndices = StillAvailableIndices;

    // Option 2: More aggressive cleanup - iterate all Pool objects and shrink if possible
    // This is more complex as it might involve moving objects in the Pool array if we remove elements,
    // which would invalidate indices in AvailableIndices and ObjectToIndexMap.
    // A simpler approach for shrinking is to destroy and mark, then potentially have a separate "Compact" step.
    // For now, we only destroy objects, they remain as "slots" until the pool itself is destroyed or re-initialized.
    // If MaxPoolSize is enforced and pool shrinks, we'd need to remove from Pool array.

    Statistics.AvailableObjects = AvailableIndices.Num();
    Statistics.CurrentPooledObjects = Pool.Num(); // This might not change if we don't shrink the Pool array
    Statistics.LastCleanupTimeSeconds = CurrentTime;
    UpdateStatistics();
    
    if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Cleanup finished. Available: %d"), *ObjectClass->GetName(), AvailableIndices.Num());
}

template<typename T>
bool FAdvancedObjectPool<T>::ShouldCleanupObject(const FAdvancedPooledObject& PooledObject, double CurrentTime) const
{
    if (PooledObject.bInUse) return false; // Never cleanup active objects

    // Check idle time
    if (Config.MaxIdleTime > 0.0f && PooledObject.GetIdleTime(CurrentTime) > Config.MaxIdleTime)
    {
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Verbose, TEXT("Pool [%s]: Object %s marked for cleanup due to MaxIdleTime."), *ObjectClass->GetName(), *PooledObject.ObjectID);
        return true;
    }
    
    // Check lifetime in pool
    if (Config.MaxObjectLifetime > 0.0f && PooledObject.GetLifetimeInPool(CurrentTime) > Config.MaxObjectLifetime)
    {
        if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Verbose, TEXT("Pool [%s]: Object %s marked for cleanup due to MaxObjectLifetime."), *ObjectClass->GetName(), *PooledObject.ObjectID);
        return true;
    }
    
    // Check if explicitly marked
    if (PooledObject.bMarkedForDestruction)
    {
        return true; // Already marked, ensure it's processed
    }
    
    return false;
}

template<typename T>
void FAdvancedObjectPool<T>::PerformHealthCheck()
{
    FScopeLock Lock(&PoolMutex);
    double CurrentTime = FPlatformTime::Seconds();

    if (CurrentTime - LastHealthCheckTime < Config.HealthCheckInterval && Config.HealthCheckInterval > 0)
    {
        return;
    }
    if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Log, TEXT("Pool [%s]: Performing health check."), *ObjectClass->GetName());

    int InvalidObjectsFound = 0;
    // Check objects in the available queue
    TQueue<int32> ValidAvailableIndices;
    while(!AvailableIndices.IsEmpty())
    {
        int32 ObjectIndex;
        AvailableIndices.Dequeue(ObjectIndex);
        if (Pool.IsValidIndex(ObjectIndex) && Pool[ObjectIndex].IsValid())
        {
            ValidAvailableIndices.Enqueue(ObjectIndex);
        }
        else
        {
            if (Pool.IsValidIndex(ObjectIndex)) DestroyObjectInternal(ObjectIndex); // Ensure it's fully cleaned up if slot exists
            InvalidObjectsFound++;
        }
    }
    AvailableIndices = ValidAvailableIndices;

    // Check all objects in the main pool array for validity if they are not in use
    // Active objects are assumed valid until released or game logic invalidates them.
    for (int32 i = 0; i < Pool.Num(); ++i)
    {
        FAdvancedPooledObject& PooledObject = Pool[i];
        if (!PooledObject.bInUse && !PooledObject.Object.IsValid() && !PooledObject.bMarkedForDestruction)
        {
            // This object is not in use, not in available queue (or just processed from it),
            // and its UObject is invalid.
            DestroyObjectInternal(i);
            InvalidObjectsFound++;
        }
    }
    
    Statistics.bIsHealthy = (Pool.Num() == 0) || (InvalidObjectsFound == 0); // Healthy if no invalid objects found or pool is empty
    if (InvalidObjectsFound > 0)
    {
       if (Config.bEnableDebugLogging) UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s]: Health check found and removed %d invalid objects."), *ObjectClass->GetName(), InvalidObjectsFound);
    }

    LastHealthCheckTime = CurrentTime;
    UpdateStatistics(); // Update stats after potential changes
}
