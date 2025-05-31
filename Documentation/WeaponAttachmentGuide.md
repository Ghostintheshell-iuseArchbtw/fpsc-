# Weapon Attachment System Usage Guide

This guide demonstrates how to use the Advanced Weapon Attachment System in your FPS project.

## System Overview

The Attachment System consists of:
- `UWeaponData` - Data assets defining weapon properties
- `UWeaponAttachment` - Data-only attachments defining modifiers
- `AWeaponAttachment` - Actor-based attachments for complex behavior
- `UAdvancedWeaponSystem` - Component managing weapons and attachments

## Creating Weapon Data Assets

### 1. Create a Weapon Data Asset

```cpp
// In Blueprint or C++
UWeaponData* CreateAssaultRifleData()
{
    UWeaponData* RifleData = NewObject<UWeaponData>();
    
    RifleData->WeaponName = "M4A1 Assault Rifle";
    RifleData->BaseDamage = 35.0f;
    RifleData->FireRate = 700.0f;
    RifleData->MagazineCapacity = 30;
    RifleData->EffectiveRange = 500.0f;
    RifleData->ReloadTime = 2.2f;
    RifleData->MuzzleVelocity = 884.0f;
    RifleData->BaseAccuracy = 0.92f;
    
    // Define supported attachment types
    RifleData->SupportedAttachmentTypes.Add(EAttachmentType::Optic);
    RifleData->SupportedAttachmentTypes.Add(EAttachmentType::Suppressor);
    RifleData->SupportedAttachmentTypes.Add(EAttachmentType::Grip);
    RifleData->SupportedAttachmentTypes.Add(EAttachmentType::Magazine);
    RifleData->SupportedAttachmentTypes.Add(EAttachmentType::Laser);
    
    // Define recoil pattern
    RifleData->RecoilPattern.Add(FVector2D(0.0f, 2.0f));  // First shot - mostly vertical
    RifleData->RecoilPattern.Add(FVector2D(-0.5f, 1.8f)); // Pull left slightly
    RifleData->RecoilPattern.Add(FVector2D(0.8f, 1.6f));  // Pull right
    RifleData->RecoilPattern.Add(FVector2D(-0.3f, 1.4f)); // Continue pattern...
    
    return RifleData;
}
```

## Using Attachments

### 1. Basic Attachment Usage

```cpp
// Get weapon system component
UAdvancedWeaponSystem* WeaponSystem = GetComponent<UAdvancedWeaponSystem>();

// Create and attach a red dot sight
URedDotSight* RedDot = NewObject<URedDotSight>();
WeaponSystem->AttachAccessory(EAttachmentType::Optic, RedDot);

// Create and attach a suppressor
USuppressor* Suppressor = NewObject<USuppressor>();
WeaponSystem->AttachAccessory(EAttachmentType::Suppressor, Suppressor);

// Remove an attachment
WeaponSystem->DetachAccessory(EAttachmentType::Optic);
```

### 2. Dynamic Attachment Management

```cpp
void APlayerCharacter::EquipAttachment(EAttachmentType Type, TSubclassOf<UWeaponAttachment> AttachmentClass)
{
    if (!AttachmentClass || !WeaponSystem) return;
    
    // Create attachment instance
    UWeaponAttachment* NewAttachment = NewObject<UWeaponAttachment>(this, AttachmentClass);
    
    // Check compatibility
    if (WeaponSystem->CanAttachAccessory(Type, NewAttachment))
    {
        // Attach to weapon
        if (WeaponSystem->AttachAccessory(Type, NewAttachment))
        {
            UE_LOG(LogTemp, Log, TEXT("Successfully attached %s"), *NewAttachment->AttachmentName);
            
            // Update UI to reflect new stats
            UpdateWeaponStatsUI();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Attachment %s is not compatible"), *NewAttachment->AttachmentName);
    }
}
```

### 3. Attachment Effects on Weapon Stats

```cpp
void UAdvancedWeaponSystem::ApplyAttachmentModifications()
{
    // Start with base weapon stats
    float ModifiedDamage = BaseDamage;
    float ModifiedAccuracy = BaseAccuracy;
    float ModifiedFireRate = FireRate;
    float ModifiedReloadTime = ReloadTime;
    float ModifiedRange = EffectiveRange;
    
    // Apply all attachment modifiers
    for (const auto& AttachmentPair : CurrentAttachments)
    {
        if (UWeaponAttachment* Attachment = AttachmentPair.Value)
        {
            const FAttachmentModifiers& Modifiers = Attachment->GetModifiers();
            
            ModifiedDamage *= Modifiers.DamageMultiplier;
            ModifiedAccuracy += Modifiers.AccuracyBonus;
            ModifiedFireRate *= Modifiers.FireRateMultiplier;
            ModifiedReloadTime *= Modifiers.ReloadTimeMultiplier;
            ModifiedRange *= Modifiers.RangeMultiplier;
        }
    }
    
    // Apply the calculated values
    CurrentDamage = ModifiedDamage;
    CurrentAccuracy = FMath::Clamp(ModifiedAccuracy, 0.1f, 1.0f);
    CurrentFireRate = ModifiedFireRate;
    CurrentReloadTime = ModifiedReloadTime;
    CurrentRange = ModifiedRange;
    
    // Notify other systems of stat changes
    OnWeaponStatsChanged.Broadcast();
}
```

## Blueprint Integration

### 1. Creating Custom Attachments in Blueprint

Create a Blueprint based on `UWeaponAttachment` or any of the common attachment classes:

1. Right-click in Content Browser
2. Create Blueprint Class
3. Search for and select `WeaponAttachment` or `RedDotSight`
4. Configure the attachment properties in the Details panel

### 2. Using Attachments in Blueprint

```cpp
// Blueprint callable functions available:

// Check if attachment can be equipped
UFUNCTION(BlueprintCallable, Category = "Weapon")
bool CanAttachAccessory(EAttachmentType Type, UWeaponAttachment* Attachment);

// Attach an accessory
UFUNCTION(BlueprintCallable, Category = "Weapon")
bool AttachAccessory(EAttachmentType Type, UWeaponAttachment* Attachment);

// Remove an accessory
UFUNCTION(BlueprintCallable, Category = "Weapon")
bool DetachAccessory(EAttachmentType Type);

// Get current attachment of a type
UFUNCTION(BlueprintPure, Category = "Weapon")
UWeaponAttachment* GetAttachment(EAttachmentType Type);
```

## Advanced Usage Examples

### 1. Loadout System

```cpp
USTRUCT(BlueprintType)
struct FWeaponLoadout
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UWeaponData> WeaponData;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EAttachmentType, TSubclassOf<UWeaponAttachment>> Attachments;
};

void APlayerCharacter::ApplyLoadout(const FWeaponLoadout& Loadout)
{
    if (!WeaponSystem) return;
    
    // Set weapon data
    if (Loadout.WeaponData)
    {
        UWeaponData* Data = NewObject<UWeaponData>(this, Loadout.WeaponData);
        WeaponSystem->SetWeaponData(Data);
    }
    
    // Apply attachments
    for (const auto& AttachmentPair : Loadout.Attachments)
    {
        if (AttachmentPair.Value)
        {
            UWeaponAttachment* Attachment = NewObject<UWeaponAttachment>(this, AttachmentPair.Value);
            WeaponSystem->AttachAccessory(AttachmentPair.Key, Attachment);
        }
    }
}
```

### 2. Dynamic Attachment Spawning

```cpp
void AWeaponAttachmentPickup::OnPickedUp(APlayerCharacter* Player)
{
    UAdvancedWeaponSystem* WeaponSystem = Player->GetWeaponSystem();
    if (!WeaponSystem) return;
    
    // Create attachment from pickup data
    UWeaponAttachment* NewAttachment = NewObject<UWeaponAttachment>(Player, AttachmentClass);
    
    // Try to attach
    if (WeaponSystem->AttachAccessory(AttachmentType, NewAttachment))
    {
        // Success feedback
        Player->ShowPickupMessage(FString::Printf(TEXT("Equipped %s"), *NewAttachment->AttachmentName));
        Destroy();
    }
    else
    {
        // Failure feedback
        Player->ShowPickupMessage(TEXT("Cannot equip attachment on current weapon"));
    }
}
```

### 3. Attachment Crafting System

```cpp
void ACraftingStation::CraftAttachment(const FAttachmentRecipe& Recipe, APlayerCharacter* Player)
{
    // Check if player has required materials
    if (!Player->GetInventory()->HasMaterials(Recipe.RequiredMaterials))
    {
        Player->ShowMessage(TEXT("Insufficient materials"));
        return;
    }
    
    // Consume materials
    Player->GetInventory()->ConsumeMaterials(Recipe.RequiredMaterials);
    
    // Create attachment
    UWeaponAttachment* CraftedAttachment = NewObject<UWeaponAttachment>(Player, Recipe.AttachmentClass);
    
    // Add to player inventory
    Player->GetInventory()->AddAttachment(CraftedAttachment);
    
    Player->ShowMessage(FString::Printf(TEXT("Crafted %s"), *CraftedAttachment->AttachmentName));
}
```

## Performance Considerations

### 1. Attachment Pooling

```cpp
class FAttachmentPool
{
private:
    TMap<EAttachmentType, TArray<UWeaponAttachment*>> AttachmentPools;
    
public:
    UWeaponAttachment* GetAttachment(EAttachmentType Type, TSubclassOf<UWeaponAttachment> Class)
    {
        // Try to get from pool first
        if (AttachmentPools.Contains(Type) && AttachmentPools[Type].Num() > 0)
        {
            return AttachmentPools[Type].Pop();
        }
        
        // Create new if pool is empty
        return NewObject<UWeaponAttachment>(GetTransientPackage(), Class);
    }
    
    void ReturnToPool(UWeaponAttachment* Attachment)
    {
        if (!Attachment) return;
        
        EAttachmentType Type = Attachment->AttachmentType;
        AttachmentPools.FindOrAdd(Type).Add(Attachment);
    }
};
```

### 2. Lazy Loading

```cpp
void UWeaponData::LoadAttachmentAssets()
{
    // Load attachment meshes only when needed
    for (auto& AttachmentType : SupportedAttachmentTypes)
    {
        if (AttachmentMeshes.Contains(AttachmentType))
        {
            TSoftObjectPtr<UStaticMesh>& MeshPtr = AttachmentMeshes[AttachmentType];
            if (!MeshPtr.IsValid())
            {
                MeshPtr.LoadSynchronous();
            }
        }
    }
}
```

## Error Handling

### 1. Validation

```cpp
bool UAdvancedWeaponSystem::ValidateAttachment(EAttachmentType Type, UWeaponAttachment* Attachment)
{
    if (!Attachment)
    {
        UE_LOG(LogWeapon, Warning, TEXT("Attachment is null"));
        return false;
    }
    
    if (!WeaponData)
    {
        UE_LOG(LogWeapon, Warning, TEXT("No weapon data set"));
        return false;
    }
    
    if (!WeaponData->SupportedAttachmentTypes.Contains(Type))
    {
        UE_LOG(LogWeapon, Warning, TEXT("Weapon does not support attachment type %d"), (int32)Type);
        return false;
    }
    
    if (!Attachment->IsCompatibleWith(WeaponData->WeaponName))
    {
        UE_LOG(LogWeapon, Warning, TEXT("Attachment %s not compatible with weapon %s"), 
               *Attachment->AttachmentName, *WeaponData->WeaponName);
        return false;
    }
    
    return true;
}
```

This attachment system provides a flexible, data-driven approach to weapon customization that can be easily extended and integrated with other game systems.
