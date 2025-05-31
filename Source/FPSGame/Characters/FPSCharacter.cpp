#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Engine.h"
#include "../Components/DamageComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Weapons/FPSWeapon.h"

AFPSCharacter::AFPSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->SetCapsuleSize(55.0f, 96.0f);

	// Create a CameraComponent
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.0f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetupAttachment(FirstPersonCameraComponent);
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->CastShadow = false;
	FirstPersonMesh->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	FirstPersonMesh->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create damage component
	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));
	
	// Create inventory component
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Initialize weapon
	CurrentWeapon = nullptr;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 700.0f;
	GetCharacterMovement()->AirControl = AirControl;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	// Initialize stamina
	CurrentStamina = MaxStamina;
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize movement speed
	UpdateMovementSpeed();

	// Bind damage component events
	if (DamageComponent)
	{
		DamageComponent->OnDamageTaken.AddDynamic(this, &AFPSCharacter::OnTakeDamage);
		DamageComponent->OnDeath.AddDynamic(this, &AFPSCharacter::OnCharacterDeath);
	}
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update stamina system
	UpdateStamina(DeltaTime);
	
	// Apply movement inertia for realistic physics-based movement
	ApplyMovementInertia(DeltaTime);
	
	// Update movement speed based on current state
	UpdateMovementSpeed();
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind movement functions
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AFPSCharacter::MoveRight);

	// Bind look functions
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);

	// Bind action functions
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AFPSCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AFPSCharacter::StopJump);
	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AFPSCharacter::StartRun);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AFPSCharacter::StopRun);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AFPSCharacter::StartCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AFPSCharacter::StopCrouch);
	PlayerInputComponent->BindAction("Prone", IE_Pressed, this, &AFPSCharacter::ToggleProne);

	// Bind weapon functions
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFPSCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AFPSCharacter::StopFire);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AFPSCharacter::Reload);
	PlayerInputComponent->BindAction("Switch Fire Mode", IE_Pressed, this, &AFPSCharacter::SwitchFireMode);

	// Bind inventory functions
	PlayerInputComponent->BindAction("Inventory", IE_Pressed, this, &AFPSCharacter::OpenInventory);
	PlayerInputComponent->BindAction("Quick Slot 1", IE_Pressed, this, &AFPSCharacter::UseQuickSlot1);
	PlayerInputComponent->BindAction("Quick Slot 2", IE_Pressed, this, &AFPSCharacter::UseQuickSlot2);
	PlayerInputComponent->BindAction("Quick Slot 3", IE_Pressed, this, &AFPSCharacter::UseQuickSlot3);
	PlayerInputComponent->BindAction("Quick Slot 4", IE_Pressed, this, &AFPSCharacter::UseQuickSlot4);
	PlayerInputComponent->BindAction("Quick Slot 5", IE_Pressed, this, &AFPSCharacter::UseQuickSlot5);
}

void AFPSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// Add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFPSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// Add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFPSCharacter::StartJump()
{
	// Only jump if we have stamina and aren't prone
	if (CurrentStamina > 20.0f && !bIsProne)
	{
		bPressedJump = true;
		CurrentStamina -= 20.0f; // Jumping costs stamina
	}
}

void AFPSCharacter::StopJump()
{
	bPressedJump = false;
}

void AFPSCharacter::StartRun()
{
	if (CurrentStamina > 0.0f && !bIsCrouching && !bIsProne)
	{
		bIsRunning = true;
	}
}

void AFPSCharacter::StopRun()
{
	bIsRunning = false;
}

void AFPSCharacter::StartCrouch()
{
	if (!bIsProne)
	{
		bIsCrouching = true;
		bIsRunning = false;
		Crouch();
	}
}

void AFPSCharacter::StopCrouch()
{
	bIsCrouching = false;
	UnCrouch();
}

void AFPSCharacter::ToggleProne()
{
	if (bIsProne)
	{
		// Get up from prone
		bIsProne = false;
		bIsCrouching = false;
		UnCrouch();
		
		// Adjust capsule size back to normal
		GetCapsuleComponent()->SetCapsuleSize(55.0f, 96.0f);
		FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.0f));
	}
	else
	{
		// Go prone
		bIsProne = true;
		bIsCrouching = false;
		bIsRunning = false;
		
		// Adjust capsule size for prone position
		GetCapsuleComponent()->SetCapsuleSize(55.0f, 40.0f);
		FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 20.0f));
		
		// Force crouch state
		Crouch();
	}
}

void AFPSCharacter::UpdateMovementSpeed()
{
	float TargetSpeed = WalkSpeed;
	
	if (bIsProne)
	{
		TargetSpeed = ProneSpeed;
	}
	else if (bIsCrouching)
	{
		TargetSpeed = CrouchSpeed;
	}
	else if (bIsRunning && CurrentStamina > 0.0f)
	{
		TargetSpeed = RunSpeed;
	}
	
	GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
}

void AFPSCharacter::UpdateStamina(float DeltaTime)
{
	if (bIsRunning && GetVelocity().Size() > 0.0f)
	{
		// Drain stamina while running
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - (StaminaDrainRate * DeltaTime));
		
		// Stop running if out of stamina
		if (CurrentStamina <= 0.0f)
		{
			bIsRunning = false;
		}
	}
	else
	{
		// Regenerate stamina when not running
		CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenRate * DeltaTime));
	}
}

void AFPSCharacter::ApplyMovementInertia(float DeltaTime)
{
	// Apply physics-based movement inertia for more realistic movement
	if (GetCharacterMovement())
	{
		FVector CurrentVelocity = GetVelocity();
		FVector TargetVelocity = GetCharacterMovement()->GetPendingInputVector() * GetCharacterMovement()->MaxWalkSpeed;
		
		// Interpolate between current and target velocity based on inertia
		FVector NewVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime, MovementInertia);
		
		// Apply the new velocity (only for horizontal movement)
		GetCharacterMovement()->Velocity.X = NewVelocity.X;
		GetCharacterMovement()->Velocity.Y = NewVelocity.Y;
	}
}

void AFPSCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AFPSCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AFPSCharacter::Reload()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Reload();
	}
}

void AFPSCharacter::SwitchFireMode()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->SwitchFireMode();
	}
}

void AFPSCharacter::OpenInventory()
{
	// This would open the inventory UI
	// Implementation depends on your UI system
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Inventory opened"));
	}
}

void AFPSCharacter::UseQuickSlot1()
{
	if (InventoryComponent)
	{
		InventoryComponent->UseQuickSlot(0);
	}
}

void AFPSCharacter::UseQuickSlot2()
{
	if (InventoryComponent)
	{
		InventoryComponent->UseQuickSlot(1);
	}
}

void AFPSCharacter::UseQuickSlot3()
{
	if (InventoryComponent)
	{
		InventoryComponent->UseQuickSlot(2);
	}
}

void AFPSCharacter::UseQuickSlot4()
{
	if (InventoryComponent)
	{
		InventoryComponent->UseQuickSlot(3);
	}
}

void AFPSCharacter::UseQuickSlot5()
{
	if (InventoryComponent)
	{
		InventoryComponent->UseQuickSlot(4);
	}
}

void AFPSCharacter::EquipWeapon(TSubclassOf<AFPSWeapon> WeaponClass)
{
	if (!WeaponClass) return;

	// Unequip current weapon first
	UnequipWeapon();

	// Spawn new weapon
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	CurrentWeapon = GetWorld()->SpawnActor<AFPSWeapon>(WeaponClass, SpawnParams);
	
	if (CurrentWeapon)
	{
		// Attach weapon to character
		CurrentWeapon->AttachToComponent(
			FirstPersonMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("GripPoint")
		);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Weapon equipped"));
		}
	}
}

void AFPSCharacter::UnequipWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("Weapon unequipped"));
		}
	}
}

void AFPSCharacter::OnTakeDamage(float Damage, EDamageType DamageType, FVector HitLocation, AActor* DamageDealer)
{
	// Handle damage feedback
	if (GEngine)
	{
		FString DamageMessage = FString::Printf(TEXT("Took %.1f damage!"), Damage);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, DamageMessage);
	}

	// Add screen shake, sound effects, etc. here
	// You can also trigger visual effects based on damage type
}

void AFPSCharacter::OnCharacterDeath()
{
	// Handle character death
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Character died!"));
	}

	// Disable input
	DisableInput(Cast<APlayerController>(GetController()));

	// Drop weapon
	UnequipWeapon();

	// You can add death animations, ragdoll physics, etc. here
}
