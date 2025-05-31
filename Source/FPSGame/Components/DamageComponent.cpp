#include "DamageComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

UDamageComponent::UDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Initialize health and armor
	CurrentHealth = MaxHealth;
	CurrentArmor = MaxArmor;

	// Set up default damage zones
	DamageZones.Empty();
	
	// Head - critical hit zone
	FDamageZone HeadZone;
	HeadZone.BoneName = TEXT("head");
	HeadZone.DamageMultiplier = 2.0f;
	HeadZone.bIsCritical = true;
	DamageZones.Add(HeadZone);

	// Chest - vital zone
	FDamageZone ChestZone;
	ChestZone.BoneName = TEXT("spine_03");
	ChestZone.DamageMultiplier = 1.2f;
	ChestZone.bIsCritical = false;
	DamageZones.Add(ChestZone);

	// Arms - reduced damage
	FDamageZone ArmZone;
	ArmZone.BoneName = TEXT("upperarm_l");
	ArmZone.DamageMultiplier = 0.7f;
	ArmZone.bIsCritical = false;
	DamageZones.Add(ArmZone);

	// Legs - reduced damage
	FDamageZone LegZone;
	LegZone.BoneName = TEXT("thigh_l");
	LegZone.DamageMultiplier = 0.8f;
	LegZone.bIsCritical = false;
	DamageZones.Add(LegZone);
}

void UDamageComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Start health regeneration timer
	if (HealthRegenRate > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			HealthRegenTimerHandle,
			this,
			&UDamageComponent::UpdateHealthRegeneration,
			1.0f,
			true
		);
	}
}

void UDamageComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UDamageComponent::TakeDamage(float DamageAmount, EDamageType DamageType, FVector HitLocation, AActor* DamageDealer, const FName& HitBone)
{
	if (!IsAlive()) return;

	float FinalDamage = DamageAmount;

	// Apply damage zone multipliers
	if (HitBone != NAME_None)
	{
		FDamageZone* DamageZone = GetDamageZoneForBone(HitBone);
		if (DamageZone)
		{
			FinalDamage *= DamageZone->DamageMultiplier;
			
			// Apply critical hit effects
			if (DamageZone->bIsCritical)
			{
				// Critical hits bypass some armor
				FinalDamage *= 1.5f;
			}
		}
	}

	// Apply armor protection
	if (CurrentArmor > 0.0f && DamageType == EDamageType::Bullet)
	{
		float ArmorDamage = FinalDamage * ArmorEffectiveness;
		float HealthDamage = FinalDamage * (1.0f - ArmorEffectiveness);
		
		// Damage armor first
		CurrentArmor = FMath::Max(0.0f, CurrentArmor - ArmorDamage);
		FinalDamage = HealthDamage;
	}

	// Apply damage to health
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - FinalDamage);
	LastDamageTime = GetWorld()->GetTimeSeconds();

	// Broadcast damage event
	OnDamageTaken.Broadcast(FinalDamage, DamageType, HitLocation, DamageDealer);

	// Apply status effects based on damage type
	switch (DamageType)
	{
		case EDamageType::Bullet:
			if (FinalDamage > 50.0f)
			{
				StartBleeding(8.0f);
			}
			break;
		case EDamageType::Fire:
			StartBurning(5.0f);
			break;
		case EDamageType::Explosion:
			StartBleeding(15.0f);
			break;
	}

	// Check for death
	if (CurrentHealth <= 0.0f)
	{
		OnDeath.Broadcast();
	}

	// Debug output
	if (GEngine)
	{
		FString DebugMessage = FString::Printf(
			TEXT("Damage: %.1f (Type: %d) | Health: %.1f/%.1f | Armor: %.1f/%.1f"),
			FinalDamage,
			(int32)DamageType,
			CurrentHealth,
			MaxHealth,
			CurrentArmor,
			MaxArmor
		);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, DebugMessage);
	}
}

void UDamageComponent::StartBleeding(float Duration)
{
	if (bIsBleeding) return;

	bIsBleeding = true;
	
	// Start bleeding damage over time
	GetWorld()->GetTimerManager().SetTimer(
		BleedingTimerHandle,
		this,
		&UDamageComponent::ApplyBleedingDamage,
		1.0f,
		true
	);

	// Stop bleeding after duration
	FTimerHandle StopBleedingTimer;
	GetWorld()->GetTimerManager().SetTimer(StopBleedingTimer, [this]()
	{
		bIsBleeding = false;
		GetWorld()->GetTimerManager().ClearTimer(BleedingTimerHandle);
	}, Duration, false);
}

void UDamageComponent::StartBurning(float Duration)
{
	if (bIsOnFire) return;

	bIsOnFire = true;
	
	// Start burning damage over time
	GetWorld()->GetTimerManager().SetTimer(
		BurningTimerHandle,
		this,
		&UDamageComponent::ApplyBurningDamage,
		0.5f,
		true
	);

	// Stop burning after duration
	FTimerHandle StopBurningTimer;
	GetWorld()->GetTimerManager().SetTimer(StopBurningTimer, [this]()
	{
		bIsOnFire = false;
		GetWorld()->GetTimerManager().ClearTimer(BurningTimerHandle);
	}, Duration, false);
}

void UDamageComponent::StartPoisoning(float Duration)
{
	if (bIsPoisoned) return;

	bIsPoisoned = true;
	
	// Start poison damage over time
	GetWorld()->GetTimerManager().SetTimer(
		PoisonTimerHandle,
		this,
		&UDamageComponent::ApplyPoisonDamage,
		2.0f,
		true
	);

	// Stop poisoning after duration
	FTimerHandle StopPoisonTimer;
	GetWorld()->GetTimerManager().SetTimer(StopPoisonTimer, [this]()
	{
		bIsPoisoned = false;
		GetWorld()->GetTimerManager().ClearTimer(PoisonTimerHandle);
	}, Duration, false);
}

void UDamageComponent::StopAllStatusEffects()
{
	bIsBleeding = false;
	bIsOnFire = false;
	bIsPoisoned = false;

	GetWorld()->GetTimerManager().ClearTimer(BleedingTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BurningTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(PoisonTimerHandle);
}

void UDamageComponent::Heal(float HealAmount)
{
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
}

void UDamageComponent::RepairArmor(float ArmorAmount)
{
	CurrentArmor = FMath::Min(MaxArmor, CurrentArmor + ArmorAmount);
}

void UDamageComponent::UpdateHealthRegeneration()
{
	if (!IsAlive() || CurrentHealth >= MaxHealth) return;

	// Only regenerate if enough time has passed since last damage
	float TimeSinceLastDamage = GetWorld()->GetTimeSeconds() - LastDamageTime;
	if (TimeSinceLastDamage >= HealthRegenDelay)
	{
		Heal(HealthRegenRate);
	}
}

void UDamageComponent::ApplyBleedingDamage()
{
	if (IsAlive() && bIsBleeding)
	{
		TakeDamage(BleedingDamageRate, EDamageType::Bullet, GetOwner()->GetActorLocation(), nullptr);
	}
}

void UDamageComponent::ApplyBurningDamage()
{
	if (IsAlive() && bIsOnFire)
	{
		TakeDamage(BurningDamageRate, EDamageType::Fire, GetOwner()->GetActorLocation(), nullptr);
	}
}

void UDamageComponent::ApplyPoisonDamage()
{
	if (IsAlive() && bIsPoisoned)
	{
		TakeDamage(PoisonDamageRate, EDamageType::Bullet, GetOwner()->GetActorLocation(), nullptr);
	}
}

FDamageZone* UDamageComponent::GetDamageZoneForBone(const FName& BoneName)
{
	for (FDamageZone& Zone : DamageZones)
	{
		if (Zone.BoneName == BoneName)
		{
			return &Zone;
		}
	}
	return nullptr;
}
