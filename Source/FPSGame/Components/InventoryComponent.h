#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "InventoryComponent.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon,
	Ammo,
	Medical,
	Equipment,
	Consumable,
	Attachment,
	Resource
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemRarity Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* WorldMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Size = FVector2D(1, 1); // Grid size in inventory

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDroppable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemData* ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Durability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D SlotPosition = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FString> CustomProperties;

	FInventoryItem()
	{
		ItemData = nullptr;
		Quantity = 1;
		Durability = 100.0f;
		SlotPosition = FVector2D::ZeroVector;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, int32, SlotIndex, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryFull);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

	// Inventory properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventorySize = 40; // Number of slots

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FVector2D GridSize = FVector2D(10, 4); // 10x4 grid

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	float MaxWeight = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	float CurrentWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> Items;

	// Grid-based inventory for Tarkov-style inventory
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<TArray<bool>> OccupiedSlots;

	// Quick slots for fast access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quick Slots")
	int32 QuickSlotCount = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Quick Slots")
	TArray<int32> QuickSlots;

	// Item database
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Database")
	UDataTable* ItemDatabase;

	// Internal functions
	bool CanFitItemAt(const FItemData* ItemData, FVector2D Position) const;
	void MarkSlotsOccupied(FVector2D Position, FVector2D Size, bool bOccupied);
	FVector2D FindAvailableSlot(const FItemData* ItemData) const;
	void UpdateWeight();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemRemoved OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryFull OnInventoryFull;

	// Main inventory functions
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemID, int32 Quantity = 1, FVector2D PreferredPosition = FVector2D(-1, -1));

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemToSlot(FName ItemID, int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemFromSlot(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(int32 FromSlot, int32 ToSlot);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItemToPosition(int32 SlotIndex, FVector2D NewPosition);

	// Stack management
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool StackItems(int32 SlotIndex1, int32 SlotIndex2);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SplitStack(int32 SlotIndex, int32 SplitQuantity);

	// Quick slots
	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	bool AssignToQuickSlot(int32 SlotIndex, int32 QuickSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	bool UseQuickSlot(int32 QuickSlotIndex);

	// Search and query functions
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<int32> FindItemsByType(EItemType ItemType) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 FindItemByID(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemQuantity(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(FName ItemID, int32 MinQuantity = 1) const;

	// Weight and space management
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetWeightPercentage() const { return CurrentWeight / MaxWeight; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsOverEncumbered() const { return CurrentWeight > MaxWeight; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEmptySlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(FName ItemID, int32 Quantity = 1) const;

	// Item data access
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetItemAtSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEmpty(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Database")
	FItemData* GetItemData(FName ItemID) const;

	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SortInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FInventoryItem> GetAllItems() const { return Items; }
};
