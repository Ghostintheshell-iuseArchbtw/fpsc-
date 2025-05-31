#include "FPSWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AFPSWeapon::AFPSWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create weapon mesh component
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;

	// Create muzzle location
	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(WeaponMesh);

	// Initialize weapon stats
	WeaponStats.Damage = 100.0f;
	WeaponStats.Range = 1000.0f;
	WeaponStats.FireRate = 600.0f;
	WeaponStats.RecoilForce = 1.0f;
	WeaponStats.Accuracy = 0.95f;
	WeaponStats.MagazineSize = 30;
	WeaponStats.ReloadTime = 2.5f;
	WeaponStats.BulletVelocity = 800.0f;
	WeaponStats.BulletDrop = 9.81f;

	// Initialize ammo
	CurrentAmmo = WeaponStats.MagazineSize;
	ReserveAmmo = 120;

	// Initialize fire mode
	CurrentFireMode = EFireMode::FullAuto;
	WeaponType = EWeaponType::AssaultRifle;

	// Initialize recoil pattern (can be customized per weapon)
	RecoilPattern = FVector2D(2.0f, 1.5f);
	CurrentRecoil = FVector2D::ZeroVector;

	// Initialize attachment slots
	AttachmentSlots.SetNum(4); // Scope, Barrel, Grip, Stock
}

void AFPSWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AFPSWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update recoil recovery
	UpdateRecoilRecovery(DeltaTime);

	// Update weapon sway
	UpdateWeaponSway(DeltaTime);
}

void AFPSWeapon::StartFire()
{
	if (CanFire())
	{
		FireWeapon();
	}
}

void AFPSWeapon::StopFire()
{
	// Reset burst counter when stopping fire
	CurrentBurstShots = 0;
	
	// Clear any burst timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);
	}
}

void AFPSWeapon::FireWeapon()
{
	if (!CanFire()) return;

	switch (CurrentFireMode)
	{
		case EFireMode::Single:
			FireSingle();
			break;
		case EFireMode::Burst:
			FireBurst();
			break;
		case EFireMode::FullAuto:
			FireFullAuto();
			break;
	}
}

void AFPSWeapon::FireSingle()
{
	if (CurrentAmmo <= 0) return;

	// Fire one shot
	FVector StartLocation = MuzzleLocation->GetComponentLocation();
	FVector ForwardDirection = MuzzleLocation->GetForwardVector();
	
	// Apply accuracy spread
	float SpreadAngle = (1.0f - WeaponStats.Accuracy) * 5.0f; // Max 5 degrees spread
	FVector RandomSpread = FVector(
		FMath::RandRange(-SpreadAngle, SpreadAngle),
		FMath::RandRange(-SpreadAngle, SpreadAngle),
		0.0f
	);
	ForwardDirection += RandomSpread;
	ForwardDirection.Normalize();

	// Perform line trace with ballistics
	FHitResult HitResult;
	FVector EndLocation = CalculateBulletTrajectory(StartLocation, ForwardDirection, WeaponStats.Range, 0.1f);
	
	if (PerformLineTrace(StartLocation, EndLocation, HitResult))
	{
		// Hit something - apply damage
		if (HitResult.GetActor())
		{
			UGameplayStatics::ApplyPointDamage(
				HitResult.GetActor(),
				WeaponStats.Damage,
				HitResult.Location,
				HitResult,
				nullptr,
				this,
				nullptr
			);
		}
	}

	// Apply recoil
	ApplyRecoil();

	// Consume ammo
	CurrentAmmo--;
	LastFireTime = GetWorld()->GetTimeSeconds();

	// Set fire cooldown
	bCanFire = false;
	float FireDelay = 60.0f / WeaponStats.FireRate;
	
	FTimerHandle FireCooldownTimer;
	GetWorld()->GetTimerManager().SetTimer(FireCooldownTimer, [this]()
	{
		bCanFire = true;
	}, FireDelay, false);
}

void AFPSWeapon::FireBurst()
{
	if (CurrentBurstShots >= BurstCount)
	{
		CurrentBurstShots = 0;
		return;
	}

	FireSingle();
	CurrentBurstShots++;

	if (CurrentBurstShots < BurstCount && CurrentAmmo > 0)
	{
		// Schedule next shot in burst
		float BurstDelay = 60.0f / (WeaponStats.FireRate * 2.0f); // Faster than normal fire rate for burst
		GetWorld()->GetTimerManager().SetTimer(BurstTimerHandle, [this]()
		{
			FireBurst();
		}, BurstDelay, false);
	}
	else
	{
		CurrentBurstShots = 0;
	}
}

void AFPSWeapon::FireFullAuto()
{
	FireSingle();

	// Continue firing if still holding trigger
	if (bCanFire && CurrentAmmo > 0)
	{
		float FireDelay = 60.0f / WeaponStats.FireRate;
		GetWorld()->GetTimerManager().SetTimer(BurstTimerHandle, [this]()
		{
			if (CanFire())
			{
				FireFullAuto();
			}
		}, FireDelay, false);
	}
}

FVector AFPSWeapon::CalculateBulletTrajectory(FVector StartLocation, FVector Direction, float Distance, float DeltaTime)
{
	// Calculate bullet trajectory with physics (bullet drop)
	float TimeToTarget = Distance / WeaponStats.BulletVelocity;
	float Drop = 0.5f * WeaponStats.BulletDrop * TimeToTarget * TimeToTarget;
	
	FVector EndLocation = StartLocation + (Direction * Distance);
	EndLocation.Z -= Drop; // Apply bullet drop
	
	return EndLocation;
}

bool AFPSWeapon::PerformLineTrace(FVector Start, FVector End, FHitResult& HitResult)
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility,
		QueryParams
	);

	// Debug line trace (remove in shipping build)
	if (GEngine && GEngine->GetNetMode(GetWorld()) != NM_DedicatedServer)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 1.0f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), HitResult.Location, 5.0f, 12, FColor::Green, false, 1.0f);
		}
	}

	return bHit;
}

void AFPSWeapon::ApplyRecoil()
{
	// Add to current recoil
	CurrentRecoil.X += RecoilPattern.X * WeaponStats.RecoilForce;
	CurrentRecoil.Y += RecoilPattern.Y * WeaponStats.RecoilForce;

	// Apply random variation
	CurrentRecoil.X += FMath::RandRange(-0.5f, 0.5f);
	CurrentRecoil.Y += FMath::RandRange(-0.5f, 0.5f);

	// Apply recoil to owner's control rotation
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			PC->AddPitchInput(-CurrentRecoil.Y);
			PC->AddYawInput(CurrentRecoil.X);
		}
	}
}

void AFPSWeapon::UpdateRecoilRecovery(float DeltaTime)
{
	// Gradually reduce recoil over time
	CurrentRecoil = FMath::VInterpTo(CurrentRecoil, FVector2D::ZeroVector, DeltaTime, RecoilRecoveryRate);
}

void AFPSWeapon::UpdateWeaponSway(float DeltaTime)
{
	// Implement subtle weapon sway for realism
	float Time = GetWorld()->GetTimeSeconds();
	FVector SwayOffset = FVector(
		FMath::Sin(Time * SwaySpeed) * SwayIntensity,
		FMath::Cos(Time * SwaySpeed * 0.7f) * SwayIntensity,
		FMath::Sin(Time * SwaySpeed * 1.3f) * SwayIntensity * 0.5f
	);

	// Apply sway to weapon mesh
	if (WeaponMesh)
	{
		FVector CurrentLocation = WeaponMesh->GetRelativeLocation();
		FVector TargetLocation = CurrentLocation + (SwayOffset * 0.1f);
		WeaponMesh->SetRelativeLocation(FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 2.0f));
	}
}

void AFPSWeapon::Reload()
{
	if (bIsReloading || CurrentAmmo >= WeaponStats.MagazineSize || ReserveAmmo <= 0)
		return;

	StartReload();
}

void AFPSWeapon::StartReload()
{
	bIsReloading = true;
	bCanFire = false;

	// Start reload timer
	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this, &AFPSWeapon::FinishReload, WeaponStats.ReloadTime, false);
}

void AFPSWeapon::FinishReload()
{
	int32 AmmoNeeded = WeaponStats.MagazineSize - CurrentAmmo;
	int32 AmmoToReload = FMath::Min(AmmoNeeded, ReserveAmmo);

	CurrentAmmo += AmmoToReload;
	ReserveAmmo -= AmmoToReload;

	bIsReloading = false;
	bCanFire = true;
}

void AFPSWeapon::SwitchFireMode()
{
	switch (CurrentFireMode)
	{
		case EFireMode::Single:
			CurrentFireMode = EFireMode::Burst;
			break;
		case EFireMode::Burst:
			CurrentFireMode = EFireMode::FullAuto;
			break;
		case EFireMode::FullAuto:
			CurrentFireMode = EFireMode::Single;
			break;
	}
}

FString AFPSWeapon::GetFireModeString() const
{
	switch (CurrentFireMode)
	{
		case EFireMode::Single:
			return TEXT("SEMI");
		case EFireMode::Burst:
			return TEXT("BURST");
		case EFireMode::FullAuto:
			return TEXT("AUTO");
		default:
			return TEXT("UNKNOWN");
	}
}

void AFPSWeapon::AttachComponent(UStaticMeshComponent* Component, int32 SlotIndex)
{
	if (SlotIndex >= 0 && SlotIndex < AttachmentSlots.Num())
	{
		AttachmentSlots[SlotIndex] = Component;
		if (Component)
		{
			Component->AttachToComponent(WeaponMesh, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
}

void AFPSWeapon::DetachComponent(int32 SlotIndex)
{
	if (SlotIndex >= 0 && SlotIndex < AttachmentSlots.Num() && AttachmentSlots[SlotIndex])
	{
		AttachmentSlots[SlotIndex]->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		AttachmentSlots[SlotIndex] = nullptr;
	}
}
