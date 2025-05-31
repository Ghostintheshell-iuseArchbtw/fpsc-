#include "InventoryComponent.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Initialize inventory
	Items.SetNum(InventorySize);
	QuickSlots.SetNum(QuickSlotCount);
	
	// Initialize quick slots to -1 (empty)
	for (int32& QuickSlot : QuickSlots)
	{
		QuickSlot = -1;
	}
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize grid system
	OccupiedSlots.SetNum(GridSize.X);
	for (int32 X = 0; X < GridSize.X; X++)
	{
		OccupiedSlots[X].SetNum(GridSize.Y);
		for (int32 Y = 0; Y < GridSize.Y; Y++)
		{
			OccupiedSlots[X][Y] = false;
		}
	}
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UInventoryComponent::AddItem(FName ItemID, int32 Quantity, FVector2D PreferredPosition)
{
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
	{
		return false;
	}

	// Check weight limit
	float AddedWeight = ItemData->Weight * Quantity;
	if (CurrentWeight + AddedWeight > MaxWeight)
	{
		OnInventoryFull.Broadcast();
		return false;
	}

	// Try to stack with existing items first
	if (ItemData->bIsStackable)
	{
		for (int32 i = 0; i < Items.Num(); i++)
		{
			if (Items[i].ItemData && Items[i].ItemData->ItemID == ItemID)
			{
				int32 AvailableSpace = ItemData->MaxStackSize - Items[i].Quantity;
				if (AvailableSpace > 0)
				{
					int32 ToAdd = FMath::Min(AvailableSpace, Quantity);
					Items[i].Quantity += ToAdd;
					Quantity -= ToAdd;
					CurrentWeight += ItemData->Weight * ToAdd;
					
					OnInventoryChanged.Broadcast(i, Items[i]);
					
					if (Quantity <= 0)
					{
						return true;
					}
				}
			}
		}
	}

	// Find position for remaining items
	FVector2D Position = PreferredPosition;
	if (Position.X < 0 || Position.Y < 0)
	{
		Position = FindAvailableSlot(ItemData);
		if (Position.X < 0 || Position.Y < 0)
		{
			OnInventoryFull.Broadcast();
			return false;
		}
	}

	// Check if we can fit the item at the position
	if (!CanFitItemAt(ItemData, Position))
	{
		OnInventoryFull.Broadcast();
		return false;
	}

	// Find empty slot index that corresponds to this position
	int32 SlotIndex = Position.Y * GridSize.X + Position.X;
	
	// Create new inventory item
	FInventoryItem NewItem;
	NewItem.ItemData = ItemData;
	NewItem.Quantity = Quantity;
	NewItem.SlotPosition = Position;
	NewItem.Durability = 100.0f;

	// Add to inventory
	Items[SlotIndex] = NewItem;
	CurrentWeight += ItemData->Weight * Quantity;
	
	// Mark grid slots as occupied
	MarkSlotsOccupied(Position, ItemData->Size, true);

	// Broadcast events
	OnItemAdded.Broadcast(NewItem);
	OnInventoryChanged.Broadcast(SlotIndex, NewItem);

	return true;
}

bool UInventoryComponent::AddItemToSlot(FName ItemID, int32 SlotIndex, int32 Quantity)
{
	if (SlotIndex < 0 || SlotIndex >= Items.Num())
	{
		return false;
	}

	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
	{
		return false;
	}

	// Check if slot is empty or contains the same stackable item
	if (!IsSlotEmpty(SlotIndex))
	{
		if (!ItemData->bIsStackable || Items[SlotIndex].ItemData->ItemID != ItemID)
		{
			return false;
		}
		
		// Check stack limit
		if (Items[SlotIndex].Quantity + Quantity > ItemData->MaxStackSize)
		{
			return false;
		}
	}

	// Check weight limit
	float AddedWeight = ItemData->Weight * Quantity;
	if (CurrentWeight + AddedWeight > MaxWeight)
	{
		return false;
	}

	if (IsSlotEmpty(SlotIndex))
	{
		// Create new item
		FInventoryItem NewItem;
		NewItem.ItemData = ItemData;
		NewItem.Quantity = Quantity;
		NewItem.SlotPosition = FVector2D(SlotIndex % (int32)GridSize.X, SlotIndex / (int32)GridSize.X);
		NewItem.Durability = 100.0f;

		Items[SlotIndex] = NewItem;
		OnItemAdded.Broadcast(NewItem);
	}
	else
	{
		// Add to existing stack
		Items[SlotIndex].Quantity += Quantity;
	}

	CurrentWeight += AddedWeight;
	OnInventoryChanged.Broadcast(SlotIndex, Items[SlotIndex]);

	return true;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	int32 SlotIndex = FindItemByID(ItemID);
	if (SlotIndex >= 0)
	{
		return RemoveItemFromSlot(SlotIndex, Quantity);
	}
	return false;
}

bool UInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (SlotIndex < 0 || SlotIndex >= Items.Num() || IsSlotEmpty(SlotIndex))
	{
		return false;
	}

	FInventoryItem& Item = Items[SlotIndex];
	int32 RemoveQuantity = FMath::Min(Quantity, Item.Quantity);
	
	// Update weight
	CurrentWeight -= Item.ItemData->Weight * RemoveQuantity;
	CurrentWeight = FMath::Max(0.0f, CurrentWeight);

	// Update quantity
	Item.Quantity -= RemoveQuantity;

	if (Item.Quantity <= 0)
	{
		// Remove item completely
		FInventoryItem RemovedItem = Item;
		
		// Clear grid slots
		MarkSlotsOccupied(Item.SlotPosition, Item.ItemData->Size, false);
		
		// Clear the slot
		Item = FInventoryItem();
		
		OnItemRemoved.Broadcast(RemovedItem);
	}

	OnInventoryChanged.Broadcast(SlotIndex, Item);
	return true;
}

bool UInventoryComponent::MoveItem(int32 FromSlot, int32 ToSlot)
{
	if (FromSlot < 0 || FromSlot >= Items.Num() || ToSlot < 0 || ToSlot >= Items.Num())
	{
		return false;
	}

	if (IsSlotEmpty(FromSlot) || FromSlot == ToSlot)
	{
		return false;
	}

	// Get positions
	FVector2D ToPosition = FVector2D(ToSlot % (int32)GridSize.X, ToSlot / (int32)GridSize.X);
	
	// Check if we can fit the item at the new position
	FInventoryItem& ItemToMove = Items[FromSlot];
	
	// Temporarily clear the current position
	MarkSlotsOccupied(ItemToMove.SlotPosition, ItemToMove.ItemData->Size, false);
	
	if (!CanFitItemAt(ItemToMove.ItemData, ToPosition))
	{
		// Restore the original position
		MarkSlotsOccupied(ItemToMove.SlotPosition, ItemToMove.ItemData->Size, true);
		return false;
	}

	// Perform the move
	FInventoryItem TempItem = Items[FromSlot];
	Items[FromSlot] = FInventoryItem();
	
	TempItem.SlotPosition = ToPosition;
	Items[ToSlot] = TempItem;
	
	// Mark new position as occupied
	MarkSlotsOccupied(ToPosition, TempItem.ItemData->Size, true);

	// Broadcast changes
	OnInventoryChanged.Broadcast(FromSlot, Items[FromSlot]);
	OnInventoryChanged.Broadcast(ToSlot, Items[ToSlot]);

	return true;
}

bool UInventoryComponent::MoveItemToPosition(int32 SlotIndex, FVector2D NewPosition)
{
	int32 NewSlotIndex = NewPosition.Y * GridSize.X + NewPosition.X;
	return MoveItem(SlotIndex, NewSlotIndex);
}

bool UInventoryComponent::StackItems(int32 SlotIndex1, int32 SlotIndex2)
{
	if (SlotIndex1 < 0 || SlotIndex1 >= Items.Num() || SlotIndex2 < 0 || SlotIndex2 >= Items.Num())
	{
		return false;
	}

	if (IsSlotEmpty(SlotIndex1) || IsSlotEmpty(SlotIndex2) || SlotIndex1 == SlotIndex2)
	{
		return false;
	}

	FInventoryItem& Item1 = Items[SlotIndex1];
	FInventoryItem& Item2 = Items[SlotIndex2];

	// Check if items are the same and stackable
	if (Item1.ItemData->ItemID != Item2.ItemData->ItemID || !Item1.ItemData->bIsStackable)
	{
		return false;
	}

	// Calculate how much we can stack
	int32 AvailableSpace = Item1.ItemData->MaxStackSize - Item1.Quantity;
	int32 ToStack = FMath::Min(AvailableSpace, Item2.Quantity);

	if (ToStack <= 0)
	{
		return false;
	}

	// Perform the stacking
	Item1.Quantity += ToStack;
	Item2.Quantity -= ToStack;

	// Remove item2 if it's empty
	if (Item2.Quantity <= 0)
	{
		MarkSlotsOccupied(Item2.SlotPosition, Item2.ItemData->Size, false);
		Item2 = FInventoryItem();
	}

	// Broadcast changes
	OnInventoryChanged.Broadcast(SlotIndex1, Item1);
	OnInventoryChanged.Broadcast(SlotIndex2, Item2);

	return true;
}

bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitQuantity)
{
	if (SlotIndex < 0 || SlotIndex >= Items.Num() || IsSlotEmpty(SlotIndex))
	{
		return false;
	}

	FInventoryItem& Item = Items[SlotIndex];
	
	if (!Item.ItemData->bIsStackable || Item.Quantity <= SplitQuantity || SplitQuantity <= 0)
	{
		return false;
	}

	// Find an empty slot for the split stack
	FVector2D AvailablePos = FindAvailableSlot(Item.ItemData);
	if (AvailablePos.X < 0 || AvailablePos.Y < 0)
	{
		return false;
	}

	// Create new item with split quantity
	FInventoryItem SplitItem = Item;
	SplitItem.Quantity = SplitQuantity;
	SplitItem.SlotPosition = AvailablePos;

	// Update original item
	Item.Quantity -= SplitQuantity;

	// Add split item to inventory
	int32 NewSlotIndex = AvailablePos.Y * GridSize.X + AvailablePos.X;
	Items[NewSlotIndex] = SplitItem;
	MarkSlotsOccupied(AvailablePos, Item.ItemData->Size, true);

	// Broadcast changes
	OnInventoryChanged.Broadcast(SlotIndex, Item);
	OnInventoryChanged.Broadcast(NewSlotIndex, SplitItem);

	return true;
}

bool UInventoryComponent::AssignToQuickSlot(int32 SlotIndex, int32 QuickSlotIndex)
{
	if (QuickSlotIndex < 0 || QuickSlotIndex >= QuickSlots.Num())
	{
		return false;
	}

	if (SlotIndex < 0 || SlotIndex >= Items.Num() || IsSlotEmpty(SlotIndex))
	{
		QuickSlots[QuickSlotIndex] = -1;
		return true;
	}

	QuickSlots[QuickSlotIndex] = SlotIndex;
	return true;
}

bool UInventoryComponent::UseQuickSlot(int32 QuickSlotIndex)
{
	if (QuickSlotIndex < 0 || QuickSlotIndex >= QuickSlots.Num())
	{
		return false;
	}

	int32 SlotIndex = QuickSlots[QuickSlotIndex];
	if (SlotIndex < 0 || SlotIndex >= Items.Num() || IsSlotEmpty(SlotIndex))
	{
		return false;
	}

	// Implementation depends on item type
	// This is a basic framework - expand based on your needs
	return true;
}

TArray<int32> UInventoryComponent::FindItemsByType(EItemType ItemType) const
{
	TArray<int32> FoundItems;
	
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (!IsSlotEmpty(i) && Items[i].ItemData->ItemType == ItemType)
		{
			FoundItems.Add(i);
		}
	}
	
	return FoundItems;
}

int32 UInventoryComponent::FindItemByID(FName ItemID) const
{
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (!IsSlotEmpty(i) && Items[i].ItemData->ItemID == ItemID)
		{
			return i;
		}
	}
	return -1;
}

int32 UInventoryComponent::GetItemQuantity(FName ItemID) const
{
	int32 TotalQuantity = 0;
	
	for (const FInventoryItem& Item : Items)
	{
		if (Item.ItemData && Item.ItemData->ItemID == ItemID)
		{
			TotalQuantity += Item.Quantity;
		}
	}
	
	return TotalQuantity;
}

bool UInventoryComponent::HasItem(FName ItemID, int32 MinQuantity) const
{
	return GetItemQuantity(ItemID) >= MinQuantity;
}

int32 UInventoryComponent::GetEmptySlotCount() const
{
	int32 EmptySlots = 0;
	
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (IsSlotEmpty(i))
		{
			EmptySlots++;
		}
	}
	
	return EmptySlots;
}

bool UInventoryComponent::CanAddItem(FName ItemID, int32 Quantity) const
{
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
	{
		return false;
	}

	// Check weight
	float AddedWeight = ItemData->Weight * Quantity;
	if (CurrentWeight + AddedWeight > MaxWeight)
	{
		return false;
	}

	// Check space
	FVector2D AvailablePos = FindAvailableSlot(ItemData);
	return (AvailablePos.X >= 0 && AvailablePos.Y >= 0);
}

FInventoryItem UInventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < Items.Num())
	{
		return Items[SlotIndex];
	}
	return FInventoryItem();
}

bool UInventoryComponent::IsSlotEmpty(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= Items.Num())
	{
		return true;
	}
	return Items[SlotIndex].ItemData == nullptr;
}

FItemData* UInventoryComponent::GetItemData(FName ItemID) const
{
	if (!ItemDatabase)
	{
		return nullptr;
	}

	return ItemDatabase->FindRow<FItemData>(ItemID, TEXT(""));
}

void UInventoryComponent::SortInventory()
{
	// Sort by item type, then by rarity, then by name
	Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
	{
		if (!A.ItemData && !B.ItemData) return false;
		if (!A.ItemData) return false;
		if (!B.ItemData) return true;

		if (A.ItemData->ItemType != B.ItemData->ItemType)
		{
			return (int32)A.ItemData->ItemType < (int32)B.ItemData->ItemType;
		}

		if (A.ItemData->Rarity != B.ItemData->Rarity)
		{
			return (int32)A.ItemData->Rarity > (int32)B.ItemData->Rarity;
		}

		return A.ItemData->ItemName.ToString() < B.ItemData->ItemName.ToString();
	});

	// Update positions and broadcast changes
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (!IsSlotEmpty(i))
		{
			OnInventoryChanged.Broadcast(i, Items[i]);
		}
	}
}

void UInventoryComponent::ClearInventory()
{
	Items.Empty();
	Items.SetNum(InventorySize);
	CurrentWeight = 0.0f;
	
	// Clear grid
	for (int32 X = 0; X < GridSize.X; X++)
	{
		for (int32 Y = 0; Y < GridSize.Y; Y++)
		{
			OccupiedSlots[X][Y] = false;
		}
	}
	
	// Clear quick slots
	for (int32& QuickSlot : QuickSlots)
	{
		QuickSlot = -1;
	}
}

void UInventoryComponent::UpdateWeight()
{
	CurrentWeight = 0.0f;
	
	for (const FInventoryItem& Item : Items)
	{
		if (Item.ItemData)
		{
			CurrentWeight += Item.ItemData->Weight * Item.Quantity;
		}
	}
}

bool UInventoryComponent::CanFitItemAt(const FItemData* ItemData, FVector2D Position) const
{
	if (!ItemData) return false;

	// Check bounds
	if (Position.X < 0 || Position.Y < 0 || 
		Position.X + ItemData->Size.X > GridSize.X || 
		Position.Y + ItemData->Size.Y > GridSize.Y)
	{
		return false;
	}

	// Check if all required slots are free
	for (int32 X = Position.X; X < Position.X + ItemData->Size.X; X++)
	{
		for (int32 Y = Position.Y; Y < Position.Y + ItemData->Size.Y; Y++)
		{
			if (OccupiedSlots[X][Y])
			{
				return false;
			}
		}
	}

	return true;
}

void UInventoryComponent::MarkSlotsOccupied(FVector2D Position, FVector2D Size, bool bOccupied)
{
	for (int32 X = Position.X; X < Position.X + Size.X; X++)
	{
		for (int32 Y = Position.Y; Y < Position.Y + Size.Y; Y++)
		{
			if (X >= 0 && X < GridSize.X && Y >= 0 && Y < GridSize.Y)
			{
				OccupiedSlots[X][Y] = bOccupied;
			}
		}
	}
}

FVector2D UInventoryComponent::FindAvailableSlot(const FItemData* ItemData) const
{
	if (!ItemData) return FVector2D(-1, -1);

	for (int32 Y = 0; Y <= GridSize.Y - ItemData->Size.Y; Y++)
	{
		for (int32 X = 0; X <= GridSize.X - ItemData->Size.X; X++)
		{
			FVector2D TestPosition(X, Y);
			if (CanFitItemAt(ItemData, TestPosition))
			{
				return TestPosition;
			}
		}
	}

	return FVector2D(-1, -1);
}
