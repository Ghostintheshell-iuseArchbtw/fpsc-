#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "../Components/DamageComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Weapons/FPSWeapon.h"
#include "FPSCharacter.generated.h"

UCLASS()
class FPSGAME_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSCharacter();

protected:
	virtual void BeginPlay() override;

	// Camera component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FirstPersonCameraComponent;

	// Mesh component for first person view (arms/weapon)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	class USkeletalMeshComponent* FirstPersonMesh;

	// Damage and inventory components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UDamageComponent* DamageComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComponent;

	// Current weapon
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	class AFPSWeapon* CurrentWeapon;

	// Movement variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ProneSpeed = 100.0f;

	// Stamina system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float MaxStamina = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float StaminaDrainRate = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float StaminaRegenRate = 15.0f;

	// Movement states
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsRunning = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsProne = false;

	// Physics-based movement variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float MovementInertia = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float AirControl = 0.2f;

	// Input functions
	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartJump();
	void StopJump();
	void StartRun();
	void StopRun();
	void StartCrouch();
	void StopCrouch();
	void ToggleProne();

	// Weapon input functions
	void StartFire();
	void StopFire();
	void Reload();
	void SwitchFireMode();

	// Inventory functions
	void OpenInventory();
	void UseQuickSlot1();
	void UseQuickSlot2();
	void UseQuickSlot3();
	void UseQuickSlot4();
	void UseQuickSlot5();

	// Movement functions
	void UpdateMovementSpeed();
	void UpdateStamina(float DeltaTime);
	void ApplyMovementInertia(float DeltaTime);

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Getters for movement state
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsCrouching() const { return bIsCrouching; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsProne() const { return bIsProne; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetStaminaPercentage() const { return CurrentStamina / MaxStamina; }

	// Weapon functions
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(TSubclassOf<AFPSWeapon> WeaponClass);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AFPSWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	// Damage handling
	UFUNCTION()
	void OnTakeDamage(float Damage, EDamageType DamageType, FVector HitLocation, AActor* DamageDealer);

	UFUNCTION()
	void OnCharacterDeath();

	// Component access
	UFUNCTION(BlueprintPure, Category = "Components")
	UDamageComponent* GetDamageComponent() const { return DamageComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
};
