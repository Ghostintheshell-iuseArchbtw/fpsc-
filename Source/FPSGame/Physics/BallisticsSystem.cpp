#include "BallisticsSystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Engine/DecalActor.h"
#include "Components/DecalComponent.h"

UBallisticsSystem::UBallisticsSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.01f; // High frequency for accurate ballistics
    
    InitializeDefaultBallisticData();
}

void UBallisticsSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize environmental conditions
    UpdateEnvironmentalConditions(15.0f, 50.0f, 101325.0f, FVector(0, 0, 0));
}

void UBallisticsSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Update active bullet simulations
    for (int32 i = ActiveBullets.Num() - 1; i >= 0; i--)
    {
        FBulletSimulationState& Bullet = ActiveBullets[i];
        
        if (!Bullet.bIsActive)
        {
            ActiveBullets.RemoveAt(i);
            continue;
        }
        
        // Get ballistic data
        FBallisticData BallisticData = GetBallisticData(Bullet.AmmoType);
        ApplyBulletTypeModifiers(Bullet.BulletType, BallisticData);
        
        // Calculate new position and velocity
        FVector OldPosition = Bullet.Position;
        float StepTime = DeltaTime;
        
        // Apply gravity
        FVector GravityEffect = CalculateGravityEffect(StepTime);
        Bullet.Velocity += GravityEffect;
        
        // Apply drag
        float Speed = Bullet.Velocity.Size();
        float AirDensity = CalculateAirDensity(BallisticData.Temperature, BallisticData.Pressure, BallisticData.Humidity);
        float DragForce = CalculateDragForce(Speed, AirDensity, BallisticData.BulletDiameter, BallisticData.DragCoefficient);
        
        FVector DragAcceleration = -Bullet.Velocity.GetSafeNormal() * (DragForce / BallisticData.BulletMass);
        Bullet.Velocity += DragAcceleration * StepTime;
        
        // Apply wind drift
        if (bCalculateWindDrift)
        {
            FVector WindEffect = BallisticData.WindVelocity * StepTime * 0.1f; // Simplified wind effect
            Bullet.Velocity += WindEffect;
        }
        
        // Apply Coriolis effect (if enabled)
        if (bCalculateCoriolisEffect)
        {
            FVector CoriolisEffect = CalculateCoriolisEffect(Bullet.Velocity, 45.0f, StepTime); // Assume 45° latitude
            Bullet.Velocity += CoriolisEffect;
        }
        
        // Update position
        Bullet.Position += Bullet.Velocity * StepTime;
        Bullet.Time += StepTime;
        
        // Calculate current energy
        float CurrentSpeed = Bullet.Velocity.Size();
        Bullet.Energy = 0.5f * BallisticData.BulletMass * CurrentSpeed * CurrentSpeed;
        
        // Check for impacts
        FHitResult HitResult;
        TArray<AActor*> IgnoreActors;
        if (Bullet.Instigator)
        {
            IgnoreActors.Add(Bullet.Instigator);
        }
        
        if (PerformLineTrace(OldPosition, Bullet.Position, HitResult, IgnoreActors))
        {
            // Process the impact
            bool bBulletStopped = ProcessBulletImpact(HitResult, Bullet.Velocity, Bullet.AmmoType, Bullet.BulletType, Bullet.PenetrationCount);
            
            if (bBulletStopped)
            {
                Bullet.bIsActive = false;
            }
            else
            {
                // Continue simulation from impact point
                Bullet.Position = HitResult.Location;
                Bullet.PenetrationCount++;
            }
        }
        
        // Check if bullet has traveled too far or lost too much energy
        float TravelDistance = ConvertUnitsToMeters((Bullet.Position - OldPosition).Size());
        if (TravelDistance > BallisticData.MaxEffectiveRange || Bullet.Energy < 10.0f) // 10 Joules minimum
        {
            Bullet.bIsActive = false;
        }
    }
}

void UBallisticsSystem::InitializeDefaultBallisticData()
{
    // Initialize ballistic data for different ammo types
    
    // 9mm Pistol
    FBallisticData Pistol9mm;
    Pistol9mm.MuzzleVelocity = 350.0f;
    Pistol9mm.BulletMass = 0.008f;
    Pistol9mm.BulletDiameter = 0.009f;
    Pistol9mm.DragCoefficient = 0.4f;
    Pistol9mm.BaseDamage = 35.0f;
    Pistol9mm.ArmorPenetration = 15.0f;
    Pistol9mm.MaxEffectiveRange = 100.0f;
    AmmoBallisticData.Add(EAmmoType::Pistol_9mm, Pistol9mm);
    
    // .45 ACP
    FBallisticData Pistol45ACP;
    Pistol45ACP.MuzzleVelocity = 260.0f;
    Pistol45ACP.BulletMass = 0.015f;
    Pistol45ACP.BulletDiameter = 0.0115f;
    Pistol45ACP.DragCoefficient = 0.45f;
    Pistol45ACP.BaseDamage = 45.0f;
    Pistol45ACP.ArmorPenetration = 12.0f;
    Pistol45ACP.MaxEffectiveRange = 90.0f;
    AmmoBallisticData.Add(EAmmoType::Pistol_45ACP, Pistol45ACP);
    
    // 5.56x45mm NATO
    FBallisticData Rifle556;
    Rifle556.MuzzleVelocity = 990.0f;
    Rifle556.BulletMass = 0.004f;
    Rifle556.BulletDiameter = 0.0056f;
    Rifle556.DragCoefficient = 0.3f;
    Rifle556.BaseDamage = 55.0f;
    Rifle556.ArmorPenetration = 35.0f;
    Rifle556.MaxEffectiveRange = 600.0f;
    Rifle556.FragmentationChance = 0.15f;
    AmmoBallisticData.Add(EAmmoType::Rifle_556, Rifle556);
    
    // 7.62x51mm NATO
    FBallisticData Rifle762;
    Rifle762.MuzzleVelocity = 850.0f;
    Rifle762.BulletMass = 0.0098f;
    Rifle762.BulletDiameter = 0.0078f;
    Rifle762.DragCoefficient = 0.25f;
    Rifle762.BaseDamage = 75.0f;
    Rifle762.ArmorPenetration = 55.0f;
    Rifle762.MaxEffectiveRange = 800.0f;
    AmmoBallisticData.Add(EAmmoType::Rifle_762, Rifle762);
    
    // .308 Winchester
    FBallisticData Rifle308;
    Rifle308.MuzzleVelocity = 880.0f;
    Rifle308.BulletMass = 0.0108f;
    Rifle308.BulletDiameter = 0.0078f;
    Rifle308.DragCoefficient = 0.23f;
    Rifle308.BaseDamage = 80.0f;
    Rifle308.ArmorPenetration = 60.0f;
    Rifle308.MaxEffectiveRange = 900.0f;
    AmmoBallisticData.Add(EAmmoType::Rifle_308, Rifle308);
    
    // .338 Lapua Magnum
    FBallisticData Sniper338;
    Sniper338.MuzzleVelocity = 940.0f;
    Sniper338.BulletMass = 0.0162f;
    Sniper338.BulletDiameter = 0.0086f;
    Sniper338.DragCoefficient = 0.2f;
    Sniper338.BaseDamage = 120.0f;
    Sniper338.ArmorPenetration = 85.0f;
    Sniper338.MaxEffectiveRange = 1500.0f;
    AmmoBallisticData.Add(EAmmoType::Sniper_338, Sniper338);
    
    // .50 BMG
    FBallisticData Sniper50BMG;
    Sniper50BMG.MuzzleVelocity = 900.0f;
    Sniper50BMG.BulletMass = 0.042f;
    Sniper50BMG.BulletDiameter = 0.0127f;
    Sniper50BMG.DragCoefficient = 0.18f;
    Sniper50BMG.BaseDamage = 200.0f;
    Sniper50BMG.ArmorPenetration = 150.0f;
    Sniper50BMG.MaxEffectiveRange = 2000.0f;
    Sniper50BMG.MaxPenetrations = 5;
    AmmoBallisticData.Add(EAmmoType::Sniper_50BMG, Sniper50BMG);
    
    // 12 Gauge Shotgun
    FBallisticData Shotgun12G;
    Shotgun12G.MuzzleVelocity = 400.0f;
    Shotgun12G.BulletMass = 0.032f; // 00 Buckshot
    Shotgun12G.BulletDiameter = 0.0084f;
    Shotgun12G.DragCoefficient = 0.6f;
    Shotgun12G.BaseDamage = 25.0f; // Per pellet
    Shotgun12G.ArmorPenetration = 8.0f;
    Shotgun12G.MaxEffectiveRange = 50.0f;
    AmmoBallisticData.Add(EAmmoType::Shotgun_12G, Shotgun12G);
    
    // 9mm SMG
    FBallisticData SMG9mm;
    SMG9mm.MuzzleVelocity = 400.0f;
    SMG9mm.BulletMass = 0.008f;
    SMG9mm.BulletDiameter = 0.009f;
    SMG9mm.DragCoefficient = 0.38f;
    SMG9mm.BaseDamage = 32.0f;
    SMG9mm.ArmorPenetration = 18.0f;
    SMG9mm.MaxEffectiveRange = 150.0f;
    AmmoBallisticData.Add(EAmmoType::SMG_9mm, SMG9mm);
    
    // 7.62x54mmR LMG
    FBallisticData LMG762;
    LMG762.MuzzleVelocity = 820.0f;
    LMG762.BulletMass = 0.0098f;
    LMG762.BulletDiameter = 0.0078f;
    LMG762.DragCoefficient = 0.28f;
    LMG762.BaseDamage = 70.0f;
    LMG762.ArmorPenetration = 50.0f;
    LMG762.MaxEffectiveRange = 1000.0f;
    AmmoBallisticData.Add(EAmmoType::LMG_762, LMG762);
    
    // Initialize surface impact data
    FSurfaceImpactData FleshImpact;
    FleshImpact.SurfaceType = ESurfaceType::Flesh;
    FleshImpact.Hardness = 0.1f;
    FleshImpact.Thickness = 0.3f;
    FleshImpact.DamageResistance = 0.2f;
    SurfaceImpactData.Add(ESurfaceType::Flesh, FleshImpact);
    
    FSurfaceImpactData MetalImpact;
    MetalImpact.SurfaceType = ESurfaceType::Metal;
    MetalImpact.Hardness = 8.0f;
    MetalImpact.Thickness = 0.05f;
    MetalImpact.DamageResistance = 3.0f;
    SurfaceImpactData.Add(ESurfaceType::Metal, MetalImpact);
    
    FSurfaceImpactData ConcreteImpact;
    ConcreteImpact.SurfaceType = ESurfaceType::Concrete;
    ConcreteImpact.Hardness = 6.0f;
    ConcreteImpact.Thickness = 0.2f;
    ConcreteImpact.DamageResistance = 2.5f;
    SurfaceImpactData.Add(ESurfaceType::Concrete, ConcreteImpact);
    
    FSurfaceImpactData WoodImpact;
    WoodImpact.SurfaceType = ESurfaceType::Wood;
    WoodImpact.Hardness = 2.0f;
    WoodImpact.Thickness = 0.05f;
    WoodImpact.DamageResistance = 0.8f;
    SurfaceImpactData.Add(ESurfaceType::Wood, WoodImpact);
    
    FSurfaceImpactData GlassImpact;
    GlassImpact.SurfaceType = ESurfaceType::Glass;
    GlassImpact.Hardness = 5.0f;
    GlassImpact.Thickness = 0.01f;
    GlassImpact.DamageResistance = 0.3f;
    SurfaceImpactData.Add(ESurfaceType::Glass, GlassImpact);
    
    FSurfaceImpactData ArmorImpact;
    ArmorImpact.SurfaceType = ESurfaceType::Armor;
    ArmorImpact.Hardness = 9.0f;
    ArmorImpact.Thickness = 0.03f;
    ArmorImpact.DamageResistance = 5.0f;
    SurfaceImpactData.Add(ESurfaceType::Armor, ArmorImpact);
}

bool UBallisticsSystem::FireBullet(FVector Origin, FVector Direction, EAmmoType AmmoType, EBulletType BulletType, AActor* Instigator)
{
    if (!bUseRealisticBallistics)
    {
        // Simple hitscan for performance
        FHitResult HitResult;
        FVector EndLocation = Origin + Direction * ConvertMetersToUnits(GetBallisticData(AmmoType).MaxEffectiveRange);
        
        TArray<AActor*> IgnoreActors;
        if (Instigator)
        {
            IgnoreActors.Add(Instigator);
        }
        
        if (PerformLineTrace(Origin, EndLocation, HitResult, IgnoreActors))
        {
            return ProcessBulletImpact(HitResult, Direction * GetBallisticData(AmmoType).MuzzleVelocity, AmmoType, BulletType, 0);
        }
        return false;
    }
    
    // Create bullet simulation state
    FBulletSimulationState NewBullet;
    NewBullet.Position = Origin;
    NewBullet.Velocity = Direction.GetSafeNormal() * GetBallisticData(AmmoType).MuzzleVelocity * ConvertMetersToUnits(1.0f);
    NewBullet.Time = 0.0f;
    NewBullet.Energy = 0.5f * GetBallisticData(AmmoType).BulletMass * GetBallisticData(AmmoType).MuzzleVelocity * GetBallisticData(AmmoType).MuzzleVelocity;
    NewBullet.PenetrationCount = 0;
    NewBullet.bIsActive = true;
    NewBullet.Instigator = Instigator;
    NewBullet.AmmoType = AmmoType;
    NewBullet.BulletType = BulletType;
    
    ActiveBullets.Add(NewBullet);
    
    // Spawn tracer if appropriate
    if (BulletType == EBulletType::Tracer)
    {
        FVector TracerEnd = Origin + Direction * ConvertMetersToUnits(GetBallisticData(AmmoType).MaxEffectiveRange);
        SpawnBulletTracer(Origin, TracerEnd, AmmoType);
    }
    
    return true;
}

TArray<FTrajectoryPoint> UBallisticsSystem::CalculateTrajectory(FVector Origin, FVector Direction, EAmmoType AmmoType, float MaxDistance)
{
    TArray<FTrajectoryPoint> TrajectoryPoints;
    
    FBallisticData BallisticData = GetBallisticData(AmmoType);
    
    FVector Position = Origin;
    FVector Velocity = Direction.GetSafeNormal() * BallisticData.MuzzleVelocity * ConvertMetersToUnits(1.0f);
    float Time = 0.0f;
    float StepTime = 0.01f; // 10ms steps
    float DistanceTraveled = 0.0f;
    
    MaxDistance = FMath::Min(MaxDistance, BallisticData.MaxEffectiveRange);
    
    while (DistanceTraveled < ConvertMetersToUnits(MaxDistance))
    {
        FTrajectoryPoint Point;
        Point.Position = Position;
        Point.Velocity = Velocity;
        Point.Time = Time;
        
        float Speed = Velocity.Size();
        Point.Energy = 0.5f * BallisticData.BulletMass * FMath::Square(ConvertUnitsToMeters(Speed));
        
        // Calculate drop (vertical displacement from original trajectory)
        FVector OriginalPoint = Origin + Direction.GetSafeNormal() * DistanceTraveled;
        Point.Drop = ConvertUnitsToMeters(OriginalPoint.Z - Position.Z);
        
        // Calculate drift (horizontal displacement)
        FVector HorizontalOriginal = FVector(OriginalPoint.X, OriginalPoint.Y, 0);
        FVector HorizontalCurrent = FVector(Position.X, Position.Y, 0);
        Point.Drift = ConvertUnitsToMeters((HorizontalCurrent - HorizontalOriginal).Size());
        
        TrajectoryPoints.Add(Point);
        
        // Update physics
        FVector OldPosition = Position;
        
        // Apply gravity
        Velocity += CalculateGravityEffect(StepTime);
        
        // Apply drag
        float AirDensity = CalculateAirDensity(BallisticData.Temperature, BallisticData.Pressure, BallisticData.Humidity);
        float DragForce = CalculateDragForce(ConvertUnitsToMeters(Speed), AirDensity, BallisticData.BulletDiameter, BallisticData.DragCoefficient);
        FVector DragAcceleration = -Velocity.GetSafeNormal() * (DragForce / BallisticData.BulletMass) * ConvertMetersToUnits(1.0f);
        Velocity += DragAcceleration * StepTime;
        
        // Apply wind
        if (bCalculateWindDrift)
        {
            FVector WindEffect = BallisticData.WindVelocity * StepTime * 0.1f * ConvertMetersToUnits(1.0f);
            Velocity += WindEffect;
        }
        
        // Update position
        Position += Velocity * StepTime;
        Time += StepTime;
        DistanceTraveled += (Position - OldPosition).Size();
        
        // Break if energy is too low
        if (Point.Energy < 10.0f)
        {
            break;
        }
    }
    
    return TrajectoryPoints;
}

FVector UBallisticsSystem::CalculateBulletDrop(FVector Origin, FVector Direction, float Distance, EAmmoType AmmoType)
{
    FBallisticData BallisticData = GetBallisticData(AmmoType);
    
    // Calculate time of flight
    float HorizontalVelocity = BallisticData.MuzzleVelocity * FMath::Cos(FMath::Atan2(Direction.Z, FVector(Direction.X, Direction.Y, 0).Size()));
    float TimeOfFlight = ConvertUnitsToMeters(Distance) / HorizontalVelocity;
    
    // Calculate drop due to gravity
    float DropDistance = 0.5f * GRAVITY_ACCELERATION * TimeOfFlight * TimeOfFlight;
    
    return FVector(0, 0, -ConvertMetersToUnits(DropDistance));
}

FVector UBallisticsSystem::CalculateWindDrift(FVector Origin, FVector Direction, float Distance, FVector WindVelocity, EAmmoType AmmoType)
{
    FBallisticData BallisticData = GetBallisticData(AmmoType);
    
    // Calculate time of flight
    float HorizontalVelocity = BallisticData.MuzzleVelocity;
    float TimeOfFlight = ConvertUnitsToMeters(Distance) / HorizontalVelocity;
    
    // Calculate wind drift (simplified)
    FVector DriftVector = WindVelocity * TimeOfFlight * 0.5f; // Wind effect factor
    
    return FVector(ConvertMetersToUnits(DriftVector.X), ConvertMetersToUnits(DriftVector.Y), 0);
}

float UBallisticsSystem::CalculateEnergyAtDistance(float Distance, EAmmoType AmmoType)
{
    FBallisticData BallisticData = GetBallisticData(AmmoType);
    
    // Simplified energy loss calculation
    float EnergyLossPerMeter = BallisticData.DragCoefficient * 0.1f;
    float RemainingEnergy = 0.5f * BallisticData.BulletMass * BallisticData.MuzzleVelocity * BallisticData.MuzzleVelocity;
    RemainingEnergy *= FMath::Exp(-EnergyLossPerMeter * Distance);
    
    return FMath::Max(0.0f, RemainingEnergy);
}

float UBallisticsSystem::CalculateDamageAtDistance(float Distance, EAmmoType AmmoType, EBulletType BulletType)
{
    FBallisticData BallisticData = GetBallisticData(AmmoType);
    ApplyBulletTypeModifiers(BulletType, BallisticData);
    
    // Calculate energy retention
    float EnergyAtDistance = CalculateEnergyAtDistance(Distance, AmmoType);
    float InitialEnergy = 0.5f * BallisticData.BulletMass * BallisticData.MuzzleVelocity * BallisticData.MuzzleVelocity;
    float EnergyRatio = EnergyAtDistance / InitialEnergy;
    
    // Apply energy ratio to damage
    float DamageAtDistance = BallisticData.BaseDamage * EnergyRatio;
    
    return FMath::Max(1.0f, DamageAtDistance); // Minimum 1 damage
}

void UBallisticsSystem::UpdateEnvironmentalConditions(float NewTemperature, float NewHumidity, float NewPressure, FVector NewWindVelocity)
{
    // Update all ballistic data with new environmental conditions
    for (auto& AmmoData : AmmoBallisticData)
    {
        AmmoData.Value.Temperature = NewTemperature;
        AmmoData.Value.Humidity = NewHumidity;
        AmmoData.Value.Pressure = NewPressure;
        AmmoData.Value.WindVelocity = NewWindVelocity;
        AmmoData.Value.AirDensity = CalculateAirDensity(NewTemperature, NewPressure, NewHumidity);
    }
}

float UBallisticsSystem::CalculateAirDensity(float Temperature, float Pressure, float Humidity)
{
    // Calculate air density using ideal gas law with humidity correction
    float TempKelvin = Temperature + 273.15f;
    float DryAirDensity = Pressure / (287.0f * TempKelvin); // J/(kg·K) for dry air
    
    // Humidity correction (simplified)
    float HumidityFactor = 1.0f - (Humidity / 100.0f) * 0.0378f;
    
    return DryAirDensity * HumidityFactor;
}

FVector UBallisticsSystem::GetWindAtAltitude(float Altitude)
{
    // Simplified wind model - wind increases with altitude
    FBallisticData SampleData = GetBallisticData(EAmmoType::Rifle_556);
    FVector BaseWind = SampleData.WindVelocity;
    
    float AltitudeFactor = 1.0f + (ConvertUnitsToMeters(Altitude) / 1000.0f) * 0.2f; // 20% increase per 1000m
    
    return BaseWind * AltitudeFactor;
}

bool UBallisticsSystem::ProcessBulletImpact(const FHitResult& HitResult, FVector BulletVelocity, EAmmoType AmmoType, EBulletType BulletType, int32 PenetrationCount)
{
    FBallisticData BallisticData = GetBallisticData(AmmoType);
    ApplyBulletTypeModifiers(BulletType, BallisticData);
    
    ESurfaceType SurfaceType = DetermineSurfaceType(HitResult);
    FSurfaceImpactData SurfaceData = SurfaceImpactData.FindRef(SurfaceType);
    
    float ImpactEnergy = 0.5f * BallisticData.BulletMass * FMath::Square(ConvertUnitsToMeters(BulletVelocity.Size()));
    
    // Create impact effects
    CreateImpactEffects(HitResult.Location, HitResult.Normal, SurfaceType, ImpactEnergy);
    
    // Broadcast impact event
    OnBulletImpact.Broadcast(HitResult.Location, HitResult.GetActor(), HitResult);
    
    // Calculate damage if hit actor has damage component
    if (HitResult.GetActor())
    {
        float Distance = ConvertUnitsToMeters(FVector::Dist(HitResult.TraceStart, HitResult.Location));
        float Damage = CalculateDamageAtDistance(Distance, AmmoType, BulletType);
        
        // Apply surface-specific damage modifiers
        if (SurfaceType == ESurfaceType::Flesh)
        {
            Damage *= 1.0f; // Full damage to flesh
        }
        else if (SurfaceType == ESurfaceType::Armor)
        {
            float ArmorReduction = FMath::Max(0.0f, SurfaceData.DamageResistance - BallisticData.ArmorPenetration);
            Damage *= FMath::Max(0.1f, 1.0f - (ArmorReduction / 100.0f));
        }
        
        // Apply damage to hit actor (implement damage interface)
        // UGameplayStatics::ApplyDamage(HitResult.GetActor(), Damage, nullptr, nullptr, nullptr);
    }
    
    // Check for penetration
    if (BallisticData.bCanPenetrate && PenetrationCount < BallisticData.MaxPenetrations)
    {
        FVector ExitPoint, ExitVelocity;
        if (CalculatePenetration(HitResult, BulletVelocity, AmmoType, BulletType, ExitPoint, ExitVelocity))
        {
            OnBulletPenetration.Broadcast(HitResult.Location, ExitPoint);
            return false; // Bullet continues
        }
    }
    
    // Check for ricochet
    float RicochetChance = BallisticData.RicochetChance;
    if (SurfaceType == ESurfaceType::Metal || SurfaceType == ESurfaceType::Armor)
    {
        RicochetChance *= 2.0f; // Higher chance on hard surfaces
    }
    
    if (FMath::RandFloat() < RicochetChance)
    {
        FVector RicochetDirection;
        float EnergyLoss;
        if (CalculateRicochet(HitResult, BulletVelocity, RicochetDirection, EnergyLoss))
        {
            OnBulletRicochet.Broadcast(HitResult.Location, RicochetDirection);
            // Could spawn new bullet with reduced energy
        }
    }
    
    // Check for fragmentation
    if (ShouldBulletFragment(HitResult, BulletVelocity, BulletType))
    {
        OnBulletFragmentation.Broadcast(HitResult.Location);
        CreateFragmentation(HitResult.Location, BulletVelocity, 5); // 5 fragments
    }
    
    return true; // Bullet stopped
}

bool UBallisticsSystem::CalculatePenetration(const FHitResult& HitResult, FVector BulletVelocity, EAmmoType AmmoType, EBulletType BulletType, FVector& ExitPoint, FVector& ExitVelocity)
{
    FBallisticData BallisticData = GetBallisticData(AmmoType);
    ApplyBulletTypeModifiers(BulletType, BallisticData);
    
    ESurfaceType SurfaceType = DetermineSurfaceType(HitResult);
    FSurfaceImpactData SurfaceData = SurfaceImpactData.FindRef(SurfaceType);
    
    float ImpactEnergy = 0.5f * BallisticData.BulletMass * FMath::Square(ConvertUnitsToMeters(BulletVelocity.Size()));
    float RequiredEnergy = SurfaceData.Hardness * SurfaceData.Thickness * 1000.0f; // Simplified calculation
    
    // Check if bullet has enough energy to penetrate
    if (ImpactEnergy > RequiredEnergy && BallisticData.ArmorPenetration > SurfaceData.DamageResistance)
    {
        // Calculate exit point
        FVector PenetrationDirection = BulletVelocity.GetSafeNormal();
        float PenetrationDistance = ConvertMetersToUnits(SurfaceData.Thickness);
        ExitPoint = HitResult.Location + PenetrationDirection * PenetrationDistance;
        
        // Calculate energy loss during penetration
        float EnergyLoss = RequiredEnergy + (ImpactEnergy * 0.2f); // 20% additional loss
        float RemainingEnergy = FMath::Max(10.0f, ImpactEnergy - EnergyLoss);
        
        // Calculate exit velocity
        float ExitSpeed = FMath::Sqrt(2.0f * RemainingEnergy / BallisticData.BulletMass);
        ExitVelocity = PenetrationDirection * ConvertMetersToUnits(ExitSpeed);
        
        return true;
    }
    
    return false;
}

bool UBallisticsSystem::CalculateRicochet(const FHitResult& HitResult, FVector BulletVelocity, FVector& RicochetDirection, float& EnergyLoss)
{
    FVector IncomingDirection = BulletVelocity.GetSafeNormal();
    FVector SurfaceNormal = HitResult.Normal;
    
    // Calculate angle of incidence
    float IncidenceAngle = FMath::Acos(FMath::Abs(FVector::DotProduct(-IncomingDirection, SurfaceNormal)));
    
    // Ricochet is more likely at shallow angles
    if (IncidenceAngle > FMath::DegreesToRadians(60.0f))
    {
        // Calculate reflection direction
        RicochetDirection = IncomingDirection - 2.0f * FVector::DotProduct(IncomingDirection, SurfaceNormal) * SurfaceNormal;
        
        // Add some randomness
        FVector RandomOffset = FMath::VRand() * 0.1f;
        RicochetDirection = (RicochetDirection + RandomOffset).GetSafeNormal();
        
        // Calculate energy loss (30-70% depending on angle and surface)
        EnergyLoss = FMath::Lerp(0.3f, 0.7f, IncidenceAngle / FMath::DegreesToRadians(90.0f));
        
        return true;
    }
    
    return false;
}

void UBallisticsSystem::CreateImpactEffects(FVector ImpactLocation, FVector ImpactNormal, ESurfaceType SurfaceType, float ImpactEnergy)
{
    FSurfaceImpactData SurfaceData = SurfaceImpactData.FindRef(SurfaceType);
    
    // Spawn particle effect
    if (SurfaceData.ImpactEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            SurfaceData.ImpactEffect,
            ImpactLocation,
            ImpactNormal.Rotation()
        );
    }
    
    // Play impact sound
    if (SurfaceData.ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            SurfaceData.ImpactSound,
            ImpactLocation
        );
    }
    
    // Create decal
    if (SurfaceData.DecalMaterials.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, SurfaceData.DecalMaterials.Num() - 1);
        UMaterialInterface* DecalMaterial = SurfaceData.DecalMaterials[RandomIndex];
        
        if (DecalMaterial)
        {
            float DecalSize = FMath::Lerp(5.0f, 15.0f, ImpactEnergy / 1000.0f); // Scale with energy
            
            UGameplayStatics::SpawnDecalAtLocation(
                GetWorld(),
                DecalMaterial,
                FVector(DecalSize, DecalSize, DecalSize),
                ImpactLocation,
                ImpactNormal.Rotation(),
                30.0f // Lifespan
            );
        }
    }
}

ESurfaceType UBallisticsSystem::DetermineSurfaceType(const FHitResult& HitResult)
{
    // Check physical material
    if (HitResult.PhysMaterial.IsValid())
    {
        FString MaterialName = HitResult.PhysMaterial->GetName().ToLower();
        
        if (MaterialName.Contains("flesh") || MaterialName.Contains("body"))
        {
            return ESurfaceType::Flesh;
        }
        else if (MaterialName.Contains("metal") || MaterialName.Contains("steel"))
        {
            return ESurfaceType::Metal;
        }
        else if (MaterialName.Contains("concrete") || MaterialName.Contains("stone"))
        {
            return ESurfaceType::Concrete;
        }
        else if (MaterialName.Contains("wood"))
        {
            return ESurfaceType::Wood;
        }
        else if (MaterialName.Contains("glass"))
        {
            return ESurfaceType::Glass;
        }
        else if (MaterialName.Contains("water"))
        {
            return ESurfaceType::Water;
        }
        else if (MaterialName.Contains("armor"))
        {
            return ESurfaceType::Armor;
        }
    }
    
    // Fallback to actor name analysis
    if (HitResult.GetActor())
    {
        FString ActorName = HitResult.GetActor()->GetName().ToLower();
        
        if (ActorName.Contains("character") || ActorName.Contains("player"))
        {
            return ESurfaceType::Flesh;
        }
        else if (ActorName.Contains("wall") || ActorName.Contains("floor"))
        {
            return ESurfaceType::Concrete;
        }
    }
    
    return ESurfaceType::Concrete; // Default
}

FBallisticData UBallisticsSystem::GetBallisticData(EAmmoType AmmoType)
{
    if (AmmoBallisticData.Contains(AmmoType))
    {
        return AmmoBallisticData[AmmoType];
    }
    
    // Return default data if not found
    return FBallisticData();
}

void UBallisticsSystem::SetBallisticData(EAmmoType AmmoType, const FBallisticData& NewData)
{
    AmmoBallisticData.Add(AmmoType, NewData);
}

float UBallisticsSystem::ConvertUnitsToMeters(float UnrealUnits)
{
    return UnrealUnits * UNREAL_UNIT_TO_METER;
}

float UBallisticsSystem::ConvertMetersToUnits(float Meters)
{
    return Meters * METER_TO_UNREAL_UNIT;
}

void UBallisticsSystem::DrawTrajectoryDebug(FVector Origin, FVector Direction, EAmmoType AmmoType, float MaxDistance)
{
    TArray<FTrajectoryPoint> Trajectory = CalculateTrajectory(Origin, Direction, AmmoType, MaxDistance);
    
    for (int32 i = 0; i < Trajectory.Num() - 1; i++)
    {
        FColor DebugColor = FColor::Red;
        
        // Change color based on energy
        if (Trajectory[i].Energy > 1000.0f)
        {
            DebugColor = FColor::Red;
        }
        else if (Trajectory[i].Energy > 500.0f)
        {
            DebugColor = FColor::Orange;
        }
        else if (Trajectory[i].Energy > 100.0f)
        {
            DebugColor = FColor::Yellow;
        }
        else
        {
            DebugColor = FColor::Green;
        }
        
        DrawDebugLine(
            GetWorld(),
            Trajectory[i].Position,
            Trajectory[i + 1].Position,
            DebugColor,
            false,
            5.0f,
            0,
            2.0f
        );
    }
    
    // Draw trajectory info
    if (Trajectory.Num() > 0)
    {
        FString TrajectoryInfo = FString::Printf(
            TEXT("Ammo: %s\nMax Drop: %.2fm\nMax Drift: %.2fm\nFinal Energy: %.0fJ"),
            *UEnum::GetValueAsString(AmmoType),
            Trajectory.Last().Drop,
            Trajectory.Last().Drift,
            Trajectory.Last().Energy
        );
        
        DrawDebugString(
            GetWorld(),
            Trajectory[0].Position + FVector(0, 0, 50),
            TrajectoryInfo,
            nullptr,
            FColor::White,
            5.0f
        );
    }
}

void UBallisticsSystem::LogBallisticData(EAmmoType AmmoType)
{
    FBallisticData Data = GetBallisticData(AmmoType);
    
    UE_LOG(LogTemp, Log, TEXT("Ballistic Data for %s:"), *UEnum::GetValueAsString(AmmoType));
    UE_LOG(LogTemp, Log, TEXT("  Muzzle Velocity: %.1f m/s"), Data.MuzzleVelocity);
    UE_LOG(LogTemp, Log, TEXT("  Bullet Mass: %.4f kg"), Data.BulletMass);
    UE_LOG(LogTemp, Log, TEXT("  Base Damage: %.1f"), Data.BaseDamage);
    UE_LOG(LogTemp, Log, TEXT("  Armor Penetration: %.1f"), Data.ArmorPenetration);
    UE_LOG(LogTemp, Log, TEXT("  Max Effective Range: %.1f m"), Data.MaxEffectiveRange);
    UE_LOG(LogTemp, Log, TEXT("  Drag Coefficient: %.2f"), Data.DragCoefficient);
}

// Private helper functions
float UBallisticsSystem::CalculateDragForce(float Velocity, float AirDensity, float BulletDiameter, float DragCoefficient)
{
    float BulletArea = FMath::Pi * FMath::Square(BulletDiameter / 2.0f);
    return 0.5f * DragCoefficient * AirDensity * FMath::Square(Velocity) * BulletArea;
}

FVector UBallisticsSystem::CalculateGravityEffect(float Time)
{
    return FVector(0, 0, -GRAVITY_ACCELERATION * Time * ConvertMetersToUnits(1.0f));
}

FVector UBallisticsSystem::CalculateCoriolisEffect(FVector Velocity, float Latitude, float Time)
{
    // Simplified Coriolis calculation
    float LatitudeRad = FMath::DegreesToRadians(Latitude);
    float CoriolisAcceleration = 2.0f * EARTH_ROTATION_RATE * FMath::Sin(LatitudeRad);
    
    FVector CoriolisForce = FVector::CrossProduct(FVector(0, 0, CoriolisAcceleration), Velocity);
    return CoriolisForce * Time;
}

bool UBallisticsSystem::PerformLineTrace(FVector Start, FVector End, FHitResult& HitResult, TArray<AActor*> IgnoreActors)
{
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = true;
    QueryParams.bReturnPhysicalMaterial = true;
    QueryParams.AddIgnoredActors(IgnoreActors);
    
    return GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_WorldStatic,
        QueryParams
    );
}

void UBallisticsSystem::SpawnBulletTracer(FVector Start, FVector End, EAmmoType AmmoType)
{
    // Spawn a tracer particle system or mesh
    // This would typically involve spawning a particle system or niagara effect
    UE_LOG(LogTemp, Log, TEXT("Spawning tracer from %s to %s"), *Start.ToString(), *End.ToString());
}

void UBallisticsSystem::ApplyBulletTypeModifiers(EBulletType BulletType, FBallisticData& BallisticData)
{
    switch (BulletType)
    {
        case EBulletType::HP: // Hollow Point
            BallisticData.BaseDamage *= 1.3f;
            BallisticData.ArmorPenetration *= 0.7f;
            BallisticData.FragmentationChance *= 2.0f;
            break;
            
        case EBulletType::AP: // Armor Piercing
            BallisticData.ArmorPenetration *= 1.8f;
            BallisticData.BaseDamage *= 0.9f;
            BallisticData.MaxPenetrations += 1;
            break;
            
        case EBulletType::Tracer:
            BallisticData.BaseDamage *= 0.95f; // Slightly less damage
            break;
            
        case EBulletType::Incendiary:
            BallisticData.BaseDamage *= 1.1f;
            // Would add fire effect
            break;
            
        case EBulletType::ExplosiveTip:
            BallisticData.BaseDamage *= 1.5f;
            BallisticData.FragmentationChance = 1.0f; // Always fragments
            break;
            
        case EBulletType::Subsonic:
            BallisticData.MuzzleVelocity *= 0.7f;
            BallisticData.BaseDamage *= 0.8f;
            // Would reduce noise
            break;
            
        case EBulletType::MatchGrade:
            BallisticData.DragCoefficient *= 0.9f; // Better aerodynamics
            BallisticData.StabilityFactor *= 1.2f;
            break;
            
        default: // FMJ
            break;
    }
}

float UBallisticsSystem::CalculateStabilityEffect(float Distance, float StabilityFactor)
{
    // Bullets become less stable over distance
    float StabilityLoss = Distance * 0.001f; // 0.1% loss per meter
    return FMath::Max(0.1f, StabilityFactor - StabilityLoss);
}

bool UBallisticsSystem::ShouldBulletFragment(const FHitResult& HitResult, FVector BulletVelocity, EBulletType BulletType)
{
    ESurfaceType SurfaceType = DetermineSurfaceType(HitResult);
    float BaseChance = 0.05f; // 5% base chance
    
    if (BulletType == EBulletType::HP)
    {
        BaseChance *= 3.0f;
    }
    else if (BulletType == EBulletType::ExplosiveTip)
    {
        return true; // Always fragments
    }
    
    // Higher chance on hard surfaces
    if (SurfaceType == ESurfaceType::Metal || SurfaceType == ESurfaceType::Concrete)
    {
        BaseChance *= 2.0f;
    }
    
    // Higher chance with higher velocity
    float VelocityFactor = ConvertUnitsToMeters(BulletVelocity.Size()) / 800.0f; // Normalize to typical rifle velocity
    BaseChance *= VelocityFactor;
    
    return FMath::RandFloat() < BaseChance;
}

void UBallisticsSystem::CreateFragmentation(FVector FragmentationPoint, FVector BulletVelocity, int32 FragmentCount)
{
    for (int32 i = 0; i < FragmentCount; i++)
    {
        // Create fragment with random direction but biased forward
        FVector FragmentDirection = BulletVelocity.GetSafeNormal();
        FragmentDirection += FMath::VRand() * 0.5f;
        FragmentDirection.Normalize();
        
        // Spawn fragment (could be a smaller bullet simulation)
        FBulletSimulationState Fragment;
        Fragment.Position = FragmentationPoint;
        Fragment.Velocity = FragmentDirection * BulletVelocity.Size() * FMath::RandRange(0.3f, 0.8f);
        Fragment.Time = 0.0f;
        Fragment.Energy = 50.0f; // Small fragment energy
        Fragment.PenetrationCount = 0;
        Fragment.bIsActive = true;
        Fragment.AmmoType = EAmmoType::Pistol_9mm; // Use small caliber for fragments
        Fragment.BulletType = EBulletType::FMJ;
        
        ActiveBullets.Add(Fragment);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Created %d fragments at %s"), FragmentCount, *FragmentationPoint.ToString());
}
