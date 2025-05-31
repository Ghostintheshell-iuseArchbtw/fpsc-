#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DamageEvents.h"
#include "DamageComponent.generated.h"

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Bullet,
	Explosion,
	Fall,
	Fire,
	Melee
};

USTRUCT(BlueprintType)
struct FDamageZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCritical = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageTaken, float, Damage, EDamageType, DamageType, FVector, HitLocation, AActor*, DamageDealer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDamageComponent();

protected:
	virtual void BeginPlay() override;

	// Health system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float HealthRegenRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float HealthRegenDelay = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float LastDamageTime = 0.0f;

	// Armor system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	float MaxArmor = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Armor")
	float CurrentArmor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	float ArmorEffectiveness = 0.5f; // 50% damage reduction

	// Damage zones for realistic hit detection
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Zones")
	TArray<FDamageZone> DamageZones;

	// Status effects
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool bIsBleeding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool bIsOnFire = false;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool bIsPoisoned = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float BleedingDamageRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float BurningDamageRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float PoisonDamageRate = 2.0f;

	// Timers for status effects
	FTimerHandle BleedingTimerHandle;
	FTimerHandle BurningTimerHandle;
	FTimerHandle PoisonTimerHandle;
	FTimerHandle HealthRegenTimerHandle;

	// Internal functions
	void UpdateHealthRegeneration();
	void ApplyBleedingDamage();
	void ApplyBurningDamage();
	void ApplyPoisonDamage();
	FDamageZone* GetDamageZoneForBone(const FName& BoneName);

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDamageTaken OnDamageTaken;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeath OnDeath;

	// Main damage function
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void TakeDamage(float DamageAmount, EDamageType DamageType, FVector HitLocation, AActor* DamageDealer, const FName& HitBone = NAME_None);

	// Status effect functions
	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void StartBleeding(float Duration = 10.0f);

	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void StartBurning(float Duration = 5.0f);

	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void StartPoisoning(float Duration = 15.0f);

	UFUNCTION(BlueprintCallable, Category = "Status Effects")
	void StopAllStatusEffects();

	// Healing functions
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Healing")
	void RepairArmor(float ArmorAmount);

	// Getters
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercentage() const { return CurrentHealth / MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetArmorPercentage() const { return CurrentArmor / MaxArmor; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Status")
	bool HasStatusEffects() const { return bIsBleeding || bIsOnFire || bIsPoisoned; }
};
