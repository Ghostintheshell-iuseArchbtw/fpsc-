#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "BallisticsSystem.generated.h"

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
    Pistol_9mm      UMETA(DisplayName = "9mm Pistol"),
    Pistol_45ACP    UMETA(DisplayName = ".45 ACP"),
    Rifle_556       UMETA(DisplayName = "5.56x45mm NATO"),
    Rifle_762       UMETA(DisplayName = "7.62x51mm NATO"),
    Rifle_308       UMETA(DisplayName = ".308 Winchester"),
    Sniper_338      UMETA(DisplayName = ".338 Lapua Magnum"),
    Sniper_50BMG    UMETA(DisplayName = ".50 BMG"),
    Shotgun_12G     UMETA(DisplayName = "12 Gauge"),
    SMG_9mm         UMETA(DisplayName = "9mm SMG"),
    LMG_762         UMETA(DisplayName = "7.62x54mmR")
};

UENUM(BlueprintType)
enum class EBulletType : uint8
{
    FMJ             UMETA(DisplayName = "Full Metal Jacket"),
    HP              UMETA(DisplayName = "Hollow Point"),
    AP              UMETA(DisplayName = "Armor Piercing"),
    Tracer          UMETA(DisplayName = "Tracer"),
    Incendiary      UMETA(DisplayName = "Incendiary"),
    ExplosiveTip    UMETA(DisplayName = "Explosive Tip"),
    Subsonic        UMETA(DisplayName = "Subsonic"),
    MatchGrade      UMETA(DisplayName = "Match Grade")
};

UENUM(BlueprintType)
enum class ESurfaceType : uint8
{
    Flesh           UMETA(DisplayName = "Flesh"),
    Metal           UMETA(DisplayName = "Metal"),
    Concrete        UMETA(DisplayName = "Concrete"),
    Wood            UMETA(DisplayName = "Wood"),
    Glass           UMETA(DisplayName = "Glass"),
    Water           UMETA(DisplayName = "Water"),
    Sand            UMETA(DisplayName = "Sand"),
    Rock            UMETA(DisplayName = "Rock"),
    Fabric          UMETA(DisplayName = "Fabric"),
    Armor           UMETA(DisplayName = "Armor")
};

USTRUCT(BlueprintType)
struct FBallisticData
{
    GENERATED_BODY()

    // Basic Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float MuzzleVelocity = 800.0f; // m/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float BulletMass = 0.008f; // kg (8 grams for 5.56)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float BulletDiameter = 0.0056f; // meters (5.6mm)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics")
    float DragCoefficient = 0.3f; // G1 ballistic coefficient

    // Environmental Factors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float AirDensity = 1.225f; // kg/m³ at sea level

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float Temperature = 15.0f; // Celsius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float Humidity = 50.0f; // Percentage

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float Pressure = 101325.0f; // Pascals

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FVector WindVelocity = FVector::ZeroVector; // m/s

    // Damage Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float BaseDamage = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float ArmorPenetration = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float CriticalMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float MaxEffectiveRange = 500.0f; // meters

    // Bullet Behavior
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float StabilityFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float RicochetChance = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float FragmentationChance = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    bool bCanPenetrate = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    int32 MaxPenetrations = 2;
};

USTRUCT(BlueprintType)
struct FSurfaceImpactData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESurfaceType SurfaceType = ESurfaceType::Concrete;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Hardness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Thickness = 0.1f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DamageResistance = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UParticleSystem* ImpactEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class USoundCue* ImpactSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<UMaterialInterface*> DecalMaterials;
};

USTRUCT(BlueprintType)
struct FTrajectoryPoint
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float Time = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Energy = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Drop = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Drift = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBulletImpact, FVector, ImpactLocation, AActor*, HitActor, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBulletPenetration, FVector, EntryPoint, FVector, ExitPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBulletRicochet, FVector, RicochetPoint, FVector, NewDirection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBulletFragmentation, FVector, FragmentationPoint);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UBallisticsSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UBallisticsSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics Configuration")
    bool bUseRealisticBallistics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics Configuration")
    bool bCalculateWindDrift = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics Configuration")
    bool bCalculateCoriolisEffect = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics Configuration")
    bool bUseBarrelHeat = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics Configuration")
    float TrajectoryCalculationSteps = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics Configuration")
    float MaxTrajectoryDistance = 2000.0f; // meters

    // Ballistic Data by Ammo Type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo Data")
    TMap<EAmmoType, FBallisticData> AmmoBallisticData;

    // Surface Impact Data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface Data")
    TMap<ESurfaceType, FSurfaceImpactData> SurfaceImpactData;

    // Events
    UPROPERTY(BlueprintAssignable)
    FOnBulletImpact OnBulletImpact;

    UPROPERTY(BlueprintAssignable)
    FOnBulletPenetration OnBulletPenetration;

    UPROPERTY(BlueprintAssignable)
    FOnBulletRicochet OnBulletRicochet;

    UPROPERTY(BlueprintAssignable)
    FOnBulletFragmentation OnBulletFragmentation;

    // Core Ballistics Functions
    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    bool FireBullet(FVector Origin, FVector Direction, EAmmoType AmmoType, EBulletType BulletType = EBulletType::FMJ, AActor* Instigator = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    TArray<FTrajectoryPoint> CalculateTrajectory(FVector Origin, FVector Direction, EAmmoType AmmoType, float MaxDistance = 2000.0f);

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    FVector CalculateBulletDrop(FVector Origin, FVector Direction, float Distance, EAmmoType AmmoType);

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    FVector CalculateWindDrift(FVector Origin, FVector Direction, float Distance, FVector WindVelocity, EAmmoType AmmoType);

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    float CalculateEnergyAtDistance(float Distance, EAmmoType AmmoType);

    UFUNCTION(BlueprintCallable, Category = "Ballistics")
    float CalculateDamageAtDistance(float Distance, EAmmoType AmmoType, EBulletType BulletType = EBulletType::FMJ);

    // Environmental Functions
    UFUNCTION(BlueprintCallable, Category = "Environment")
    void UpdateEnvironmentalConditions(float NewTemperature, float NewHumidity, float NewPressure, FVector NewWindVelocity);

    UFUNCTION(BlueprintCallable, Category = "Environment")
    float CalculateAirDensity(float Temperature, float Pressure, float Humidity);

    UFUNCTION(BlueprintCallable, Category = "Environment")
    FVector GetWindAtAltitude(float Altitude);

    // Impact Functions
    UFUNCTION(BlueprintCallable, Category = "Impact")
    bool ProcessBulletImpact(const FHitResult& HitResult, FVector BulletVelocity, EAmmoType AmmoType, EBulletType BulletType, int32 PenetrationCount = 0);

    UFUNCTION(BlueprintCallable, Category = "Impact")
    bool CalculatePenetration(const FHitResult& HitResult, FVector BulletVelocity, EAmmoType AmmoType, EBulletType BulletType, FVector& ExitPoint, FVector& ExitVelocity);

    UFUNCTION(BlueprintCallable, Category = "Impact")
    bool CalculateRicochet(const FHitResult& HitResult, FVector BulletVelocity, FVector& RicochetDirection, float& EnergyLoss);

    UFUNCTION(BlueprintCallable, Category = "Impact")
    void CreateImpactEffects(FVector ImpactLocation, FVector ImpactNormal, ESurfaceType SurfaceType, float ImpactEnergy);

    UFUNCTION(BlueprintCallable, Category = "Impact")
    ESurfaceType DetermineSurfaceType(const FHitResult& HitResult);

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "Utility")
    FBallisticData GetBallisticData(EAmmoType AmmoType);

    UFUNCTION(BlueprintCallable, Category = "Utility")
    void SetBallisticData(EAmmoType AmmoType, const FBallisticData& NewData);

    UFUNCTION(BlueprintCallable, Category = "Utility")
    float ConvertUnitsToMeters(float UnrealUnits);

    UFUNCTION(BlueprintCallable, Category = "Utility")
    float ConvertMetersToUnits(float Meters);

    UFUNCTION(BlueprintCallable, Category = "Utility")
    void InitializeDefaultBallisticData();

    // Debug Functions
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void DrawTrajectoryDebug(FVector Origin, FVector Direction, EAmmoType AmmoType, float MaxDistance = 2000.0f);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogBallisticData(EAmmoType AmmoType);

private:
    // Internal calculation functions
    float CalculateDragForce(float Velocity, float AirDensity, float BulletDiameter, float DragCoefficient);
    FVector CalculateGravityEffect(float Time);
    FVector CalculateCoriolisEffect(FVector Velocity, float Latitude, float Time);
    bool PerformLineTrace(FVector Start, FVector End, FHitResult& HitResult, TArray<AActor*> IgnoreActors = TArray<AActor*>());
    void SpawnBulletTracer(FVector Start, FVector End, EAmmoType AmmoType);
    void ApplyBulletTypeModifiers(EBulletType BulletType, FBallisticData& BallisticData);
    float CalculateStabilityEffect(float Distance, float StabilityFactor);
    bool ShouldBulletFragment(const FHitResult& HitResult, FVector BulletVelocity, EBulletType BulletType);
    void CreateFragmentation(FVector FragmentationPoint, FVector BulletVelocity, int32 FragmentCount);

    // Simulation state
    struct FBulletSimulationState
    {
        FVector Position;
        FVector Velocity;
        float Time;
        float Energy;
        int32 PenetrationCount;
        bool bIsActive;
        AActor* Instigator;
        EAmmoType AmmoType;
        EBulletType BulletType;
    };

    TArray<FBulletSimulationState> ActiveBullets;

    // Constants
    static constexpr float GRAVITY_ACCELERATION = 9.81f; // m/s²
    static constexpr float UNREAL_UNIT_TO_METER = 0.01f; // 1 UU = 1 cm
    static constexpr float METER_TO_UNREAL_UNIT = 100.0f; // 1 m = 100 UU
    static constexpr float EARTH_ROTATION_RATE = 7.2921159e-5f; // rad/s
};
