#include "CommonWeaponAttachments.h"

URedDotSight::URedDotSight()
{
    AttachmentName = "Red Dot Sight";
    AttachmentType = EAttachmentType::Optic;
    AttachmentDescription = FText::FromString("A basic red dot sight that improves target acquisition.");
    
    // Modifiers
    Modifiers.AccuracyBonus = 0.05f;  // 5% accuracy improvement
    Modifiers.RangeMultiplier = 1.1f; // 10% range increase
    Modifiers.WeightAddition = 0.2f;  // 200g weight addition
    
    // Legacy compatibility
    AccuracyBonus = 0.05f;
    RangeMultiplier = 1.1f;
    
    // Economy
    Cost = 500;
    UnlockLevel = 5;
    
    // Compatibility
    CompatibleWeapons.Add("AssaultRifle");
    CompatibleWeapons.Add("SMG");
    CompatibleWeapons.Add("LMG");
}

UACOGScope::UACOGScope()
{
    AttachmentName = "ACOG 4x Scope";
    AttachmentType = EAttachmentType::Optic;
    AttachmentDescription = FText::FromString("Advanced Combat Optical Gunsight with 4x magnification.");
    
    // Modifiers
    Modifiers.AccuracyBonus = 0.15f;  // 15% accuracy improvement
    Modifiers.RangeMultiplier = 1.5f; // 50% range increase
    Modifiers.MovementSpeedMultiplier = 0.95f; // 5% movement speed penalty
    Modifiers.WeightAddition = 0.7f;  // 700g weight addition
    
    // Legacy compatibility
    AccuracyBonus = 0.15f;
    RangeMultiplier = 1.5f;
    
    // Scope properties
    Magnification = 4.0f;
    
    // Economy
    Cost = 1500;
    UnlockLevel = 15;
    
    // Compatibility
    CompatibleWeapons.Add("AssaultRifle");
    CompatibleWeapons.Add("SniperRifle");
    CompatibleWeapons.Add("LMG");
}

USuppressor::USuppressor()
{
    AttachmentName = "Suppressor";
    AttachmentType = EAttachmentType::Suppressor;
    AttachmentDescription = FText::FromString("Reduces weapon noise and muzzle flash.");
    
    // Modifiers
    Modifiers.DamageMultiplier = 0.95f; // 5% damage reduction
    Modifiers.RangeMultiplier = 0.9f;   // 10% range reduction
    Modifiers.RecoilReduction = 0.1f;   // 10% recoil reduction
    Modifiers.ConcealmentPenalty = -0.3f; // Stealth bonus
    Modifiers.WeightAddition = 0.4f;    // 400g weight addition
    
    // Legacy compatibility
    DamageMultiplier = 0.95f;
    RangeMultiplier = 0.9f;
    RecoilReduction = 0.1f;
    
    // Suppressor properties
    NoiseReduction = 0.8f;
    MuzzleFlashReduction = 0.9f;
    
    // Economy
    Cost = 800;
    UnlockLevel = 10;
    
    // Compatible with most weapons
    CompatibleWeapons.Add("AssaultRifle");
    CompatibleWeapons.Add("SMG");
    CompatibleWeapons.Add("Pistol");
    CompatibleWeapons.Add("SniperRifle");
}

UForegrip::UForegrip()
{
    AttachmentName = "Vertical Foregrip";
    AttachmentType = EAttachmentType::Grip;
    AttachmentDescription = FText::FromString("Vertical grip that improves recoil control and stability.");
    
    // Modifiers
    Modifiers.RecoilReduction = 0.2f;   // 20% recoil reduction
    Modifiers.AccuracyBonus = 0.03f;    // 3% accuracy improvement
    Modifiers.WeightAddition = 0.15f;   // 150g weight addition
    
    // Legacy compatibility
    RecoilReduction = 0.2f;
    AccuracyBonus = 0.03f;
    
    // Economy
    Cost = 300;
    UnlockLevel = 3;
    
    // Compatibility
    CompatibleWeapons.Add("AssaultRifle");
    CompatibleWeapons.Add("SMG");
    CompatibleWeapons.Add("LMG");
}

UExtendedMagazine::UExtendedMagazine()
{
    AttachmentName = "Extended Magazine";
    AttachmentType = EAttachmentType::Magazine;
    AttachmentDescription = FText::FromString("Increases magazine capacity by 10 rounds.");
    
    // Modifiers
    Modifiers.ReloadTimeMultiplier = 1.1f; // 10% slower reload
    Modifiers.MovementSpeedMultiplier = 0.98f; // 2% movement penalty
    Modifiers.WeightAddition = 0.3f;       // 300g weight addition
    
    // Legacy compatibility
    ReloadTimeMultiplier = 1.1f;
    
    // Magazine properties
    CapacityIncrease = 10;
    
    // Economy
    Cost = 400;
    UnlockLevel = 7;
    
    // Compatible with most weapons
    CompatibleWeapons.Add("AssaultRifle");
    CompatibleWeapons.Add("SMG");
    CompatibleWeapons.Add("LMG");
    CompatibleWeapons.Add("Pistol");
}

ULaserSight::ULaserSight()
{
    AttachmentName = "Laser Sight";
    AttachmentType = EAttachmentType::Laser;
    AttachmentDescription = FText::FromString("Visible laser that improves hip fire accuracy.");
    
    // Modifiers
    Modifiers.AccuracyBonus = 0.08f;    // 8% hip fire accuracy improvement
    Modifiers.ConcealmentPenalty = 0.1f; // Slight stealth penalty (visible laser)
    Modifiers.WeightAddition = 0.1f;    // 100g weight addition
    
    // Legacy compatibility
    AccuracyBonus = 0.08f;
    
    // Laser properties
    LaserColor = FLinearColor::Red;
    LaserRange = 1000.0f;
    bRequiresUpdate = true; // Needs per-frame updates for laser dot
    
    // Economy
    Cost = 350;
    UnlockLevel = 4;
    
    // Compatible with most weapons
    CompatibleWeapons.Add("AssaultRifle");
    CompatibleWeapons.Add("SMG");
    CompatibleWeapons.Add("Pistol");
    CompatibleWeapons.Add("Shotgun");
}

UTacticalFlashlight::UTacticalFlashlight()
{
    AttachmentName = "Tactical Flashlight";
    AttachmentType = EAttachmentType::Flashlight;
    AttachmentDescription = FText::FromString("High-intensity tactical flashlight for low-light operations.");
    
    // Modifiers
    Modifiers.ConcealmentPenalty = 0.2f; // Stealth penalty when active
    Modifiers.WeightAddition = 0.2f;     // 200g weight addition
    
    // Flashlight properties
    LightIntensity = 5000.0f;
    LightRange = 500.0f;
    LightColor = FLinearColor::White;
    bRequiresUpdate = true; // Needs updates for dynamic lighting
    
    // Economy
    Cost = 250;
    UnlockLevel = 2;
    
    // Compatible with most weapons
    CompatibleWeapons.Add("AssaultRifle");
    CompatibleWeapons.Add("SMG");
    CompatibleWeapons.Add("Pistol");
    CompatibleWeapons.Add("Shotgun");
}

UBipod::UBipod()
{
    AttachmentName = "Bipod";
    AttachmentType = EAttachmentType::Grip;
    AttachmentDescription = FText::FromString("Deployable bipod for improved stability when prone or braced.");
    
    // Modifiers when deployed
    Modifiers.RecoilReduction = 0.4f;   // 40% recoil reduction when deployed
    Modifiers.AccuracyBonus = 0.2f;     // 20% accuracy improvement when deployed
    Modifiers.MovementSpeedMultiplier = 0.9f; // 10% movement penalty
    Modifiers.WeightAddition = 0.5f;    // 500g weight addition
    
    // Legacy compatibility
    RecoilReduction = 0.4f;
    AccuracyBonus = 0.2f;
    
    // Bipod state
    bIsDeployed = false;
    
    // Economy
    Cost = 600;
    UnlockLevel = 12;
    
    // Primarily for heavy weapons
    CompatibleWeapons.Add("LMG");
    CompatibleWeapons.Add("SniperRifle");
}

void UBipod::DeployBipod()
{
    bIsDeployed = true;
    // In a full implementation, this would enable the stability bonuses
    // and potentially restrict movement or weapon handling
}

void UBipod::RetractBipod()
{
    bIsDeployed = false;
    // This would disable the stability bonuses and restore normal movement
}
