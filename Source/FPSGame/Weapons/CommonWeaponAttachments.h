#pragma once

#include "CoreMinimal.h"
#include "WeaponAttachment.h"
#include "CommonWeaponAttachments.generated.h"

/**
 * Red Dot Sight - Basic optic attachment
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API URedDotSight : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    URedDotSight();
};

/**
 * ACOG Scope - Advanced optic with magnification
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UACOGScope : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    UACOGScope();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optic")
    float Magnification = 4.0f;
};

/**
 * Suppressor - Reduces noise and muzzle flash
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API USuppressor : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    USuppressor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppressor")
    float NoiseReduction = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppressor")
    float MuzzleFlashReduction = 0.9f;
};

/**
 * Foregrip - Improves recoil control
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UForegrip : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    UForegrip();
};

/**
 * Extended Magazine - Increases ammo capacity
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UExtendedMagazine : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    UExtendedMagazine();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine")
    int32 CapacityIncrease = 10;
};

/**
 * Laser Sight - Improves hip fire accuracy
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API ULaserSight : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    ULaserSight();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
    FLinearColor LaserColor = FLinearColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
    float LaserRange = 1000.0f;
};

/**
 * Tactical Flashlight - Illuminates dark areas
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UTacticalFlashlight : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    UTacticalFlashlight();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
    float LightIntensity = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
    float LightRange = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
    FLinearColor LightColor = FLinearColor::White;
};

/**
 * Bipod - Provides stability when deployed
 */
UCLASS(BlueprintType, Blueprintable)
class FPSGAME_API UBipod : public UWeaponAttachment
{
    GENERATED_BODY()

public:
    UBipod();

    UPROPERTY(BlueprintReadOnly, Category = "Bipod")
    bool bIsDeployed = false;

    UFUNCTION(BlueprintCallable, Category = "Bipod")
    void DeployBipod();

    UFUNCTION(BlueprintCallable, Category = "Bipod")
    void RetractBipod();
};
