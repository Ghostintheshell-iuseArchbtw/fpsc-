#include "EnvironmentalDestructionSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"

UEnvironmentalDestructionSystem::UEnvironmentalDestructionSystem()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize default destruction properties
	DestructionProperties.Health = 100.0f;
	DestructionProperties.MaxHealth = 100.0f;
	DestructionProperties.DestructionType = EDestructionType::Fracture;
	DestructionProperties.MaterialType = EMaterialType::Concrete;
	DestructionProperties.Hardness = 1.0f;
	DestructionProperties.Brittleness = 0.5f;
	DestructionProperties.DensityMultiplier = 1.0f;
	DestructionProperties.bCanRepair = false;
	DestructionProperties.RepairRate = 10.0f;
	DestructionProperties.RepairDelay = 5.0f;

	// Initialize default damage resistances
	DestructionProperties.DamageResistances.Add(EDamageType::Bullet, 1.0f);
	DestructionProperties.DamageResistances.Add(EDamageType::Explosion, 0.5f);
	DestructionProperties.DamageResistances.Add(EDamageType::Fire, 0.8f);
	DestructionProperties.DamageResistances.Add(EDamageType::Melee, 1.2f);

	// Initialize default damage multipliers
	DestructionProperties.DamageMultipliers.Add(EDamageType::Bullet, 1.0f);
	DestructionProperties.DamageMultipliers.Add(EDamageType::Explosion, 2.0f);
	DestructionProperties.DamageMultipliers.Add(EDamageType::Fire, 0.5f);
	DestructionProperties.DamageMultipliers.Add(EDamageType::Melee, 0.8f);

	// Physics settings
	ImpactForceMultiplier = 1.0f;
	ChunkScatterRadius = 500.0f;
	MinChunkVelocity = 100.0f;
	MaxChunkVelocity = 1000.0f;
	ChunkLifetime = 10.0f;

	// Performance settings
	MaxActiveChunks = 50;
	ChunkCullDistance = 2000.0f;
	bUseLODForChunks = true;
	bCleanupChunksAutomatically = true;

	TimeSinceLastDamage = 0.0f;
	bIsRepairing = false;
}

void UEnvironmentalDestructionSystem::BeginPlay()
{
	Super::BeginPlay();
	InitializeComponent();
}

void UEnvironmentalDestructionSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update repair system
	if (DestructionProperties.bCanRepair)
	{
		UpdateRepair(DeltaTime);
	}

	// Update destruction chunks
	if (DestructionChunks.Num() > 0)
	{
		UpdateChunks(DeltaTime);
	}

	// Performance optimization
	LastOptimizationTime += DeltaTime;
	if (LastOptimizationTime >= OptimizationInterval)
	{
		LastOptimizationTime = 0.0f;
		CullDistantChunks();
	}
}

void UEnvironmentalDestructionSystem::InitializeComponent()
{
	// Get original mesh component
	OriginalMesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
	if (OriginalMesh)
	{
		OriginalStaticMesh = OriginalMesh->GetStaticMesh();
		if (OriginalMesh->GetNumMaterials() > 0)
		{
			OriginalMaterial = OriginalMesh->GetMaterial(0);
		}
	}

	// Cache damage multipliers for performance
	bDamageMultipliersCached = false;
}

bool UEnvironmentalDestructionSystem::ApplyDamage(float Damage, EDamageType DamageType, const FVector& ImpactLocation, const FVector& ImpactDirection, AActor* Instigator)
{
	if (IsDestroyed())
	{
		return false;
	}

	// Calculate actual damage based on material properties and damage type
	float ActualDamage = Damage * CalculateDamageMultiplier(DamageType);

	// Apply damage
	DestructionProperties.Health = FMath::Max(0.0f, DestructionProperties.Health - ActualDamage);
	TimeSinceLastDamage = 0.0f;
	bIsRepairing = false;

	// Broadcast damage event
	OnObjectDamaged.Broadcast(GetOwner(), ActualDamage, DestructionProperties.Health, DamageType);

	// Check if object should be destroyed
	if (DestructionProperties.Health <= 0.0f)
	{
		DestroyObject(DamageType, ImpactLocation, ImpactDirection, ActualDamage * 10.0f);
		return true;
	}

	// Play damage effects based on remaining health
	float HealthPercentage = GetHealthPercentage();
	if (HealthPercentage < 0.5f && DestructionEffects.Contains(DamageType))
	{
		PlayDestructionEffect(DestructionEffects[DamageType], ImpactLocation);
	}

	return false;
}

void UEnvironmentalDestructionSystem::DestroyObject(EDamageType DamageType, const FVector& ImpactLocation, const FVector& ImpactDirection, float Force)
{
	if (IsDestroyed())
	{
		return;
	}

	// Set health to zero
	DestructionProperties.Health = 0.0f;

	// Hide original mesh
	if (OriginalMesh)
	{
		OriginalMesh->SetVisibility(false);
		OriginalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Create destruction chunks based on destruction type
	switch (DestructionProperties.DestructionType)
	{
		case EDestructionType::Fracture:
			CreateDestructionChunks(FMath::RandRange(5, 15), 30.0f, 150.0f);
			break;
		case EDestructionType::Explode:
			CreateDestructionChunks(FMath::RandRange(8, 25), 20.0f, 100.0f);
			break;
		case EDestructionType::Shatter:
			CreateDestructionChunks(FMath::RandRange(15, 40), 10.0f, 80.0f);
			break;
		case EDestructionType::Crumble:
			CreateDestructionChunks(FMath::RandRange(20, 50), 5.0f, 50.0f);
			break;
		default:
			CreateDestructionChunks(FMath::RandRange(8, 20), 25.0f, 125.0f);
			break;
	}

	// Apply impact forces to chunks
	for (FDestructionChunk& Chunk : DestructionChunks)
	{
		ApplyImpactForce(Chunk, ImpactLocation, ImpactDirection, Force);
	}

	// Play destruction effects
	if (DestructionEffects.Contains(DamageType))
	{
		PlayDestructionEffect(DestructionEffects[DamageType], ImpactLocation);
	}

	if (MaterialEffects.Contains(DestructionProperties.MaterialType))
	{
		PlayDestructionEffect(MaterialEffects[DestructionProperties.MaterialType], ImpactLocation);
	}

	// Broadcast destruction event
	OnObjectDestroyed.Broadcast(GetOwner(), ImpactLocation, DamageType);
}

void UEnvironmentalDestructionSystem::RepairObject(float RepairAmount)
{
	if (!DestructionProperties.bCanRepair || !IsDestroyed())
	{
		return;
	}

	// Use default repair amount if not specified
	if (RepairAmount < 0.0f)
	{
		RepairAmount = DestructionProperties.MaxHealth;
	}

	// Restore health
	DestructionProperties.Health = FMath::Min(DestructionProperties.MaxHealth, DestructionProperties.Health + RepairAmount);

	// Clean up destruction chunks
	CleanupChunks();

	// Restore original mesh
	if (OriginalMesh)
	{
		OriginalMesh->SetVisibility(true);
		OriginalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// Reset repair state
	bIsRepairing = false;
	TimeSinceLastDamage = 0.0f;

	// Broadcast repair event
	OnObjectRepaired.Broadcast(GetOwner(), DestructionProperties.Health);
}

void UEnvironmentalDestructionSystem::ResetObject()
{
	// Restore to full health
	DestructionProperties.Health = DestructionProperties.MaxHealth;

	// Clean up chunks
	CleanupChunks();

	// Restore original mesh
	if (OriginalMesh)
	{
		OriginalMesh->SetVisibility(true);
		OriginalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// Reset repair state
	bIsRepairing = false;
	TimeSinceLastDamage = 0.0f;
}

void UEnvironmentalDestructionSystem::SetDestructionProperties(const FDestructionProperties& NewProperties)
{
	DestructionProperties = NewProperties;
	bDamageMultipliersCached = false; // Invalidate cache
}

void UEnvironmentalDestructionSystem::CreateDestructionChunks(int32 NumChunks, float MinChunkSize, float MaxChunkSize)
{
	if (!OriginalMesh || !OriginalStaticMesh)
	{
		return;
	}

	// Clean up existing chunks
	CleanupChunks();

	// Get mesh bounds
	FBoxSphereBounds MeshBounds = OriginalStaticMesh->GetBounds();
	FVector MeshSize = MeshBounds.BoxExtent * 2.0f;
	FVector MeshLocation = OriginalMesh->GetComponentLocation();

	// Limit number of chunks for performance
	NumChunks = FMath::Min(NumChunks, MaxActiveChunks);

	for (int32 i = 0; i < NumChunks; i++)
	{
		FDestructionChunk NewChunk;
		
		// Generate random chunk position within mesh bounds
		FVector ChunkOffset = FVector(
			FMath::RandRange(-MeshSize.X * 0.4f, MeshSize.X * 0.4f),
			FMath::RandRange(-MeshSize.Y * 0.4f, MeshSize.Y * 0.4f),
			FMath::RandRange(-MeshSize.Z * 0.4f, MeshSize.Z * 0.4f)
		);
		
		NewChunk.InitialLocation = MeshLocation + ChunkOffset;
		NewChunk.InitialRotation = FRotator(
			FMath::RandRange(-180.0f, 180.0f),
			FMath::RandRange(-180.0f, 180.0f),
			FMath::RandRange(-180.0f, 180.0f)
		);

		// Create chunk mesh
		FVector ChunkSize = GetRandomChunkSize(MinChunkSize, MaxChunkSize);
		CreateChunkMesh(NewChunk, NewChunk.InitialLocation, ChunkSize);

		if (NewChunk.ChunkMesh)
		{
			// Calculate mass based on chunk size and material density
			float ChunkVolume = ChunkSize.X * ChunkSize.Y * ChunkSize.Z;
			NewChunk.Mass = ChunkVolume * DestructionProperties.DensityMultiplier * 0.001f; // Scale down for reasonable values

			// Set initial physics properties
			NewChunk.Velocity = FVector::ZeroVector;
			NewChunk.AngularVelocity = FVector::ZeroVector;
			NewChunk.LifeTime = ChunkLifetime;
			NewChunk.bIsSettled = false;

			DestructionChunks.Add(NewChunk);
			ActiveChunkCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Created %d destruction chunks"), DestructionChunks.Num());
}

void UEnvironmentalDestructionSystem::CreateChunkMesh(FDestructionChunk& Chunk, const FVector& ChunkLocation, const FVector& ChunkSize)
{
	if (!GetOwner())
	{
		return;
	}

	// Create chunk mesh component
	Chunk.ChunkMesh = NewObject<UStaticMeshComponent>(GetOwner());
	if (!Chunk.ChunkMesh)
	{
		return;
	}

	// Generate or use a simple cube mesh for the chunk
	UStaticMesh* ChunkStaticMesh = GenerateChunkMesh(ChunkSize, DestructionProperties.MaterialType);
	if (ChunkStaticMesh)
	{
		Chunk.ChunkMesh->SetStaticMesh(ChunkStaticMesh);
	}

	// Setup mesh component
	Chunk.ChunkMesh->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::WorldTransformRules);
	Chunk.ChunkMesh->SetWorldLocation(ChunkLocation);
	Chunk.ChunkMesh->SetWorldRotation(Chunk.InitialRotation);

	// Apply material properties
	ApplyMaterialProperties(Chunk.ChunkMesh, DestructionProperties.MaterialType);

	// Enable physics
	Chunk.ChunkMesh->SetSimulatePhysics(true);
	Chunk.ChunkMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Chunk.ChunkMesh->SetCollisionObjectType(ECC_WorldDynamic);
	Chunk.ChunkMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Set mass
	if (Chunk.ChunkMesh->GetBodyInstance())
	{
		Chunk.ChunkMesh->GetBodyInstance()->SetMassOverride(Chunk.Mass, true);
	}

	// Optimize physics for chunks
	OptimizeChunkPhysics(Chunk);
}

void UEnvironmentalDestructionSystem::ApplyImpactForce(FDestructionChunk& Chunk, const FVector& ImpactLocation, const FVector& ImpactDirection, float Force)
{
	if (!Chunk.ChunkMesh)
	{
		return;
	}

	// Calculate force based on distance from impact
	FVector ChunkLocation = Chunk.ChunkMesh->GetComponentLocation();
	float DistanceFromImpact = FVector::Dist(ChunkLocation, ImpactLocation);
	float MaxEffectiveDistance = ChunkScatterRadius;
	
	// Falloff force with distance
	float ForceFalloff = FMath::Max(0.0f, 1.0f - (DistanceFromImpact / MaxEffectiveDistance));
	float ActualForce = Force * ForceFalloff * ImpactForceMultiplier;

	// Calculate force direction (from impact towards chunk)
	FVector ForceDirection = (ChunkLocation - ImpactLocation).GetSafeNormal();
	if (ForceDirection.IsNearlyZero())
	{
		ForceDirection = ImpactDirection;
	}

	// Add some randomness to the force direction
	FVector RandomOffset = FVector(
		FMath::RandRange(-0.3f, 0.3f),
		FMath::RandRange(-0.3f, 0.3f),
		FMath::RandRange(0.1f, 0.5f) // Bias upward
	);
	ForceDirection = (ForceDirection + RandomOffset).GetSafeNormal();

	// Apply impulse
	FVector Impulse = ForceDirection * ActualForce;
	Chunk.ChunkMesh->AddImpulse(Impulse, NAME_None, true);

	// Add angular impulse for rotation
	FVector AngularImpulse = FVector(
		FMath::RandRange(-ActualForce * 0.01f, ActualForce * 0.01f),
		FMath::RandRange(-ActualForce * 0.01f, ActualForce * 0.01f),
		FMath::RandRange(-ActualForce * 0.01f, ActualForce * 0.01f)
	);
	Chunk.ChunkMesh->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);

	// Store velocity for tracking
	Chunk.Velocity = Chunk.ChunkMesh->GetPhysicsLinearVelocity();
	Chunk.AngularVelocity = Chunk.ChunkMesh->GetPhysicsAngularVelocityInDegrees();
}

void UEnvironmentalDestructionSystem::UpdateChunks(float DeltaTime)
{
	for (int32 i = DestructionChunks.Num() - 1; i >= 0; i--)
	{
		FDestructionChunk& Chunk = DestructionChunks[i];
		
		if (!Chunk.ChunkMesh || !IsValid(Chunk.ChunkMesh))
		{
			DestructionChunks.RemoveAt(i);
			ActiveChunkCount--;
			continue;
		}

		// Update lifetime
		Chunk.LifeTime -= DeltaTime;

		// Update velocities
		Chunk.Velocity = Chunk.ChunkMesh->GetPhysicsLinearVelocity();
		Chunk.AngularVelocity = Chunk.ChunkMesh->GetPhysicsAngularVelocityInDegrees();

		// Check if chunk has settled (low velocity)
		float SpeedThreshold = 50.0f;
		if (Chunk.Velocity.Size() < SpeedThreshold && !Chunk.bIsSettled)
		{
			Chunk.bIsSettled = true;
		}

		// Remove chunks that have exceeded lifetime or fallen too far
		bool bShouldRemove = false;
		
		if (bCleanupChunksAutomatically)
		{
			if (Chunk.LifeTime <= 0.0f)
			{
				bShouldRemove = true;
			}
			
			// Remove chunks that have fallen below world bounds
			FVector ChunkLocation = Chunk.ChunkMesh->GetComponentLocation();
			if (ChunkLocation.Z < -10000.0f)
			{
				bShouldRemove = true;
			}
		}

		if (bShouldRemove)
		{
			Chunk.ChunkMesh->DestroyComponent();
			DestructionChunks.RemoveAt(i);
			ActiveChunkCount--;
		}
	}
}

void UEnvironmentalDestructionSystem::UpdateRepair(float DeltaTime)
{
	if (!DestructionProperties.bCanRepair || !IsDestroyed())
	{
		return;
	}

	TimeSinceLastDamage += DeltaTime;

	// Start repairing after delay
	if (TimeSinceLastDamage >= DestructionProperties.RepairDelay && !bIsRepairing)
	{
		bIsRepairing = true;
	}

	// Gradually repair
	if (bIsRepairing)
	{
		float RepairThisFrame = DestructionProperties.RepairRate * DeltaTime;
		DestructionProperties.Health = FMath::Min(DestructionProperties.MaxHealth, DestructionProperties.Health + RepairThisFrame);

		// Check if fully repaired
		if (DestructionProperties.Health >= DestructionProperties.MaxHealth)
		{
			RepairObject(0.0f); // Finish repair process
		}
	}
}

void UEnvironmentalDestructionSystem::CullDistantChunks()
{
	if (DestructionChunks.Num() == 0)
	{
		return;
	}

	// Get player location for distance calculations
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player)
	{
		return;
	}

	FVector PlayerLocation = Player->GetActorLocation();

	for (int32 i = DestructionChunks.Num() - 1; i >= 0; i--)
	{
		FDestructionChunk& Chunk = DestructionChunks[i];
		
		if (!Chunk.ChunkMesh || !IsValid(Chunk.ChunkMesh))
		{
			continue;
		}

		FVector ChunkLocation = Chunk.ChunkMesh->GetComponentLocation();
		float Distance = FVector::Dist(PlayerLocation, ChunkLocation);

		// Cull chunks that are too far away
		if (Distance > ChunkCullDistance)
		{
			Chunk.ChunkMesh->DestroyComponent();
			DestructionChunks.RemoveAt(i);
			ActiveChunkCount--;
		}
		// Reduce physics quality for distant chunks
		else if (Distance > ChunkCullDistance * 0.5f && bUseLODForChunks)
		{
			// Reduce physics update rate or simplify collision for distant chunks
			if (Chunk.ChunkMesh->GetBodyInstance())
			{
				Chunk.ChunkMesh->GetBodyInstance()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
		}
	}
}

void UEnvironmentalDestructionSystem::CleanupChunks()
{
	for (FDestructionChunk& Chunk : DestructionChunks)
	{
		if (Chunk.ChunkMesh && IsValid(Chunk.ChunkMesh))
		{
			Chunk.ChunkMesh->DestroyComponent();
		}
	}
	
	DestructionChunks.Empty();
	ActiveChunkCount = 0;
}

void UEnvironmentalDestructionSystem::SetChunkLifetime(float NewLifetime)
{
	ChunkLifetime = NewLifetime;
	
	// Update existing chunks
	for (FDestructionChunk& Chunk : DestructionChunks)
	{
		Chunk.LifeTime = NewLifetime;
	}
}

void UEnvironmentalDestructionSystem::PlayDestructionEffect(const FDestructionEffect& Effect, const FVector& Location)
{
	if (!GetWorld())
	{
		return;
	}

	// Play particle effect
	if (Effect.ParticleEffect)
	{
		FVector EffectLocation = Location + Effect.EffectOffset;
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			Effect.ParticleEffect,
			EffectLocation,
			FRotator::ZeroRotator,
			FVector(Effect.EffectScale),
			true
		);
	}

	// Play sound effect
	if (Effect.SoundEffect)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			Effect.SoundEffect,
			Location,
			FRotator::ZeroRotator,
			Effect.EffectScale
		);
	}
}

void UEnvironmentalDestructionSystem::AddDestructionEffect(EDamageType DamageType, const FDestructionEffect& Effect)
{
	DestructionEffects.Add(DamageType, Effect);
}

float UEnvironmentalDestructionSystem::CalculateDamageMultiplier(EDamageType DamageType) const
{
	// Use cached values if available
	if (bDamageMultipliersCached && CachedDamageMultipliers.Contains(DamageType))
	{
		return CachedDamageMultipliers[DamageType];
	}

	float Multiplier = 1.0f;

	// Apply damage type multiplier
	if (DestructionProperties.DamageMultipliers.Contains(DamageType))
	{
		Multiplier *= DestructionProperties.DamageMultipliers[DamageType];
	}

	// Apply resistance
	if (DestructionProperties.DamageResistances.Contains(DamageType))
	{
		Multiplier /= DestructionProperties.DamageResistances[DamageType];
	}

	// Apply material properties
	switch (DestructionProperties.MaterialType)
	{
		case EMaterialType::Glass:
			if (DamageType == EDamageType::Impact || DamageType == EDamageType::Explosion)
			{
				Multiplier *= 2.0f; // Glass is weak to impacts
			}
			break;
		case EMaterialType::Metal:
			if (DamageType == EDamageType::Laser || DamageType == EDamageType::Fire)
			{
				Multiplier *= 1.5f; // Metal conducts heat
			}
			break;
		case EMaterialType::Wood:
			if (DamageType == EDamageType::Fire)
			{
				Multiplier *= 3.0f; // Wood burns easily
			}
			break;
		// Add more material-specific modifiers
	}

	// Apply hardness factor
	Multiplier /= DestructionProperties.Hardness;

	// Cache the result
	const_cast<UEnvironmentalDestructionSystem*>(this)->CachedDamageMultipliers.Add(DamageType, Multiplier);

	return Multiplier;
}

bool UEnvironmentalDestructionSystem::ShouldPenetrate(EDamageType DamageType, float Damage) const
{
	float EffectiveDamage = Damage * CalculateDamageMultiplier(DamageType);
	float PenetrationThreshold = DestructionProperties.MaxHealth * DestructionProperties.Hardness * 0.5f;
	
	return EffectiveDamage > PenetrationThreshold;
}

FVector UEnvironmentalDestructionSystem::CalculateImpactResponse(const FVector& ImpactDirection, float Force, EMaterialType Material) const
{
	FVector Response = ImpactDirection;

	// Modify response based on material properties
	switch (Material)
	{
		case EMaterialType::Rubber:
			Response *= 0.3f; // Absorbs impact
			break;
		case EMaterialType::Metal:
			Response *= 1.2f; // Reflects impact
			break;
		case EMaterialType::Glass:
			Response *= 0.8f; // Brittle response
			break;
		default:
			break;
	}

	return Response * Force;
}

// Helper functions
UStaticMesh* UEnvironmentalDestructionSystem::GenerateChunkMesh(const FVector& ChunkSize, EMaterialType MaterialType)
{
	// For now, return nullptr - in a full implementation, this would generate
	// procedural meshes or select from a pool of chunk meshes
	// You could use Unreal's ProceduralMeshComponent or pre-made chunk assets
	return nullptr;
}

FVector UEnvironmentalDestructionSystem::GetRandomChunkSize(float MinSize, float MaxSize)
{
	return FVector(
		FMath::RandRange(MinSize, MaxSize),
		FMath::RandRange(MinSize, MaxSize),
		FMath::RandRange(MinSize, MaxSize)
	);
}

void UEnvironmentalDestructionSystem::ApplyMaterialProperties(UStaticMeshComponent* ChunkMesh, EMaterialType MaterialType)
{
	if (!ChunkMesh)
	{
		return;
	}

	// Apply material-specific properties
	if (UBodyInstance* BodyInstance = ChunkMesh->GetBodyInstance())
	{
		switch (MaterialType)
		{
			case EMaterialType::Metal:
				BodyInstance->SetPhysMaterialOverride(nullptr); // Use default metal physics material
				break;
			case EMaterialType::Wood:
				BodyInstance->SetPhysMaterialOverride(nullptr); // Use default wood physics material
				break;
			case EMaterialType::Glass:
				BodyInstance->SetPhysMaterialOverride(nullptr); // Use default glass physics material
				break;
			// Add more material types
		}
	}

	// Apply visual material if original material exists
	if (OriginalMaterial)
	{
		ChunkMesh->SetMaterial(0, OriginalMaterial);
	}
}

bool UEnvironmentalDestructionSystem::IsChunkInViewport(const FDestructionChunk& Chunk) const
{
	if (!Chunk.ChunkMesh)
	{
		return false;
	}

	// Simple frustum check - in a full implementation, you'd use proper frustum culling
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player)
	{
		return true; // Assume visible if no player
	}

	FVector ChunkLocation = Chunk.ChunkMesh->GetComponentLocation();
	FVector PlayerLocation = Player->GetActorLocation();
	float Distance = FVector::Dist(PlayerLocation, ChunkLocation);

	// Simple distance-based check
	return Distance < ChunkCullDistance;
}

void UEnvironmentalDestructionSystem::OptimizeChunkPhysics(FDestructionChunk& Chunk)
{
	if (!Chunk.ChunkMesh)
	{
		return;
	}

	// Optimize physics settings for performance
	if (UBodyInstance* BodyInstance = Chunk.ChunkMesh->GetBodyInstance())
	{
		// Use simple collision for chunks
		BodyInstance->SetCollisionProfileName("BlockAll");
		
		// Reduce physics complexity
		BodyInstance->bGenerateWakeEvents = false;
		BodyInstance->bNotifyRigidBodyCollision = false;
		
		// Set appropriate CCD settings
		BodyInstance->bUseCCD = Chunk.Mass > 10.0f; // Only for heavy chunks
	}
}
