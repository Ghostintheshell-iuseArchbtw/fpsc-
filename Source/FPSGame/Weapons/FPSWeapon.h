#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "FPSWeapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	AssaultRifle,
	SniperRifle,
	Pistol,
	Shotgun,
	SMG
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single,
	Burst,
	FullAuto
};

USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Range = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FireRate = 600.0f; // Rounds per minute

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RecoilForce = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Accuracy = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReloadTime = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BulletVelocity = 800.0f; // m/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BulletDrop = 9.81f; // gravity effect
};

UCLASS()
class FPSGAME_API AFPSWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AFPSWeapon();

protected:
	virtual void BeginPlay() override;

	// Weapon components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MuzzleLocation;

	// Weapon properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EFireMode CurrentFireMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FWeaponStats WeaponStats;

	// Ammo system
	UPROPERTY(BlueprintReadOnly, Category = "Ammo")
	int32 CurrentAmmo;

	UPROPERTY(BlueprintReadOnly, Category = "Ammo")
	int32 ReserveAmmo;

	// Weapon state
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bCanFire = true;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float LastFireTime = 0.0f;

	// Recoil system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	FVector2D RecoilPattern;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilRecoveryRate = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Recoil")
	FVector2D CurrentRecoil;

	// Burst fire
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Mode")
	int32 BurstCount = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Mode")
	int32 CurrentBurstShots = 0;

	// Weapon sway
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sway")
	float SwayIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sway")
	float SwaySpeed = 1.0f;

	// Attachments
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachments")
	TArray<UStaticMeshComponent*> AttachmentSlots;

	// Fire functions
	void FireWeapon();
	void FireSingle();
	void FireBurst();
	void FireFullAuto();

	// Ballistics calculation
	FVector CalculateBulletTrajectory(FVector StartLocation, FVector Direction, float Distance, float DeltaTime);
	bool PerformLineTrace(FVector Start, FVector End, FHitResult& HitResult);

	// Recoil functions
	void ApplyRecoil();
	void UpdateRecoilRecovery(float DeltaTime);

	// Weapon sway
	void UpdateWeaponSway(float DeltaTime);

	// Reload system
	void StartReload();
	void FinishReload();

	// Timer handles
	FTimerHandle ReloadTimerHandle;
	FTimerHandle BurstTimerHandle;

public:	
	virtual void Tick(float DeltaTime) override;

	// Input functions
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchFireMode();

	// Getters
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAmmoPercentage() const { return CurrentAmmo / (float)WeaponStats.MagazineSize; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const { return bCanFire && CurrentAmmo > 0 && !bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FString GetFireModeString() const;

	// Attachment system
	UFUNCTION(BlueprintCallable, Category = "Attachments")
	void AttachComponent(UStaticMeshComponent* Component, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Attachments")
	void DetachComponent(int32 SlotIndex);
};
