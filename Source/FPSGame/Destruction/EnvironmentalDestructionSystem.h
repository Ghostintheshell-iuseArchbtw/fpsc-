#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "Chaos/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "EnvironmentalDestructionSystem.generated.h"

UENUM(BlueprintType)
enum class EDestructionType : uint8
{
	None,
	Fracture,
	Explode,
	Crumble,
	Shatter,
	Burn,
	Melt,
	Vaporize
};

UENUM(BlueprintType)
enum class EMaterialType : uint8
{
	Concrete,
	Wood,
	Metal,
	Glass,
	Stone,
	Plastic,
	Fabric,
	Flesh,
	Vegetation,
	Electronics,
	Ceramic,
	Rubber
};

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Bullet,
	Explosion,
	Fire,
	Laser,
	Melee,
	Acid,
	Electric,
	Freeze,
	Impact
};

USTRUCT(BlueprintType)
struct FDestructionProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDestructionType DestructionType = EDestructionType::Fracture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMaterialType MaterialType = EMaterialType::Concrete;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Hardness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Brittleness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DensityMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanRepair = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RepairRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RepairDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EDamageType, float> DamageResistances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EDamageType, float> DamageMultipliers;
};

USTRUCT(BlueprintType)
struct FDestructionChunk
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	UStaticMeshComponent* ChunkMesh = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FVector InitialLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FRotator InitialRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly)
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector AngularVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float Mass = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	float LifeTime = 10.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bIsSettled = false;
};

USTRUCT(BlueprintType)
struct FDestructionEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* ParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* SoundEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector EffectOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAttachToObject = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectDestroyed, AActor*, DestroyedObject, FVector, Location, EDamageType, DamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnObjectDamaged, AActor*, DamagedObject, float, Damage, float, RemainingHealth, EDamageType, DamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectRepaired, AActor*, RepairedObject, float, NewHealth);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UEnvironmentalDestructionSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnvironmentalDestructionSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Destruction interface
	UFUNCTION(BlueprintCallable, Category = "Destruction")
	bool ApplyDamage(float Damage, EDamageType DamageType, const FVector& ImpactLocation, const FVector& ImpactDirection, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void DestroyObject(EDamageType DamageType, const FVector& ImpactLocation, const FVector& ImpactDirection, float Force = 1000.0f);

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void RepairObject(float RepairAmount = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void ResetObject();

	// Properties
	UFUNCTION(BlueprintPure, Category = "Destruction")
	float GetHealth() const { return DestructionProperties.Health; }

	UFUNCTION(BlueprintPure, Category = "Destruction")
	float GetMaxHealth() const { return DestructionProperties.MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Destruction")
	float GetHealthPercentage() const { return DestructionProperties.Health / DestructionProperties.MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Destruction")
	bool IsDestroyed() const { return DestructionProperties.Health <= 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Destruction")
	bool CanBeRepaired() const { return DestructionProperties.bCanRepair && IsDestroyed(); }

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void SetDestructionProperties(const FDestructionProperties& NewProperties);

	UFUNCTION(BlueprintPure, Category = "Destruction")
	FDestructionProperties GetDestructionProperties() const { return DestructionProperties; }

	// Chunk management
	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void CreateDestructionChunks(int32 NumChunks, float MinChunkSize = 50.0f, float MaxChunkSize = 200.0f);

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void CleanupChunks();

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void SetChunkLifetime(float NewLifetime);

	// Effects
	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void PlayDestructionEffect(const FDestructionEffect& Effect, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void AddDestructionEffect(EDamageType DamageType, const FDestructionEffect& Effect);

	// Material interaction
	UFUNCTION(BlueprintCallable, Category = "Destruction")
	float CalculateDamageMultiplier(EDamageType DamageType) const;

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	bool ShouldPenetrate(EDamageType DamageType, float Damage) const;

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	FVector CalculateImpactResponse(const FVector& ImpactDirection, float Force, EMaterialType Material) const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Destruction")
	FOnObjectDestroyed OnObjectDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Destruction")
	FOnObjectDamaged OnObjectDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Destruction")
	FOnObjectRepaired OnObjectRepaired;

protected:
	// Destruction properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	FDestructionProperties DestructionProperties;

	// Mesh references
	UPROPERTY(BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* OriginalMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mesh")
	UStaticMesh* OriginalStaticMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mesh")
	UMaterialInterface* OriginalMaterial = nullptr;

	// Destruction chunks
	UPROPERTY(BlueprintReadOnly, Category = "Destruction")
	TArray<FDestructionChunk> DestructionChunks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float ChunkLifetime = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	bool bCleanupChunksAutomatically = true;

	// Effects mapping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TMap<EDamageType, FDestructionEffect> DestructionEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TMap<EMaterialType, FDestructionEffect> MaterialEffects;

	// Physics properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float ImpactForceMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float ChunkScatterRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float MinChunkVelocity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float MaxChunkVelocity = 1000.0f;

	// Repair system
	UPROPERTY(BlueprintReadOnly, Category = "Repair")
	float TimeSinceLastDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Repair")
	bool bIsRepairing = false;

	// Performance optimization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	int32 MaxActiveChunks = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float ChunkCullDistance = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	bool bUseLODForChunks = true;

	// Internal functions
	void InitializeComponent();
	void CreateChunkMesh(FDestructionChunk& Chunk, const FVector& ChunkLocation, const FVector& ChunkSize);
	void ApplyImpactForce(FDestructionChunk& Chunk, const FVector& ImpactLocation, const FVector& ImpactDirection, float Force);
	void UpdateChunks(float DeltaTime);
	void UpdateRepair(float DeltaTime);
	void CullDistantChunks();
	UStaticMesh* GenerateChunkMesh(const FVector& ChunkSize, EMaterialType MaterialType);
	FVector GetRandomChunkSize(float MinSize, float MaxSize);
	void ApplyMaterialProperties(UStaticMeshComponent* ChunkMesh, EMaterialType MaterialType);
	bool IsChunkInViewport(const FDestructionChunk& Chunk) const;
	void OptimizeChunkPhysics(FDestructionChunk& Chunk);

private:
	// Performance tracking
	int32 ActiveChunkCount = 0;
	float LastOptimizationTime = 0.0f;
	float OptimizationInterval = 1.0f;

	// Damage calculation cache
	TMap<EDamageType, float> CachedDamageMultipliers;
	bool bDamageMultipliersCached = false;
};
