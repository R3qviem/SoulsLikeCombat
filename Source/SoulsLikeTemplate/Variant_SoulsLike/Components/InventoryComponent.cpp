// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryComponent.h"
#include "ItemDataAsset.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
	{
		QuickSlots[i] = nullptr;
	}
}

bool UInventoryComponent::AddItem(const UItemDataAsset* ItemData, int32 Count)
{
	if (!ItemData || Count <= 0)
	{
		return false;
	}

	// Try to stack with existing item
	for (FInventoryItem& Item : Items)
	{
		if (Item.ItemData == ItemData && Item.StackCount < ItemData->MaxStackSize)
		{
			const int32 CanAdd = ItemData->MaxStackSize - Item.StackCount;
			const int32 ToAdd = FMath::Min(Count, CanAdd);
			Item.StackCount += ToAdd;
			Count -= ToAdd;

			if (Count <= 0)
			{
				OnInventoryChanged.Broadcast();
				return true;
			}
		}
	}

	// Add new slots for remaining items
	while (Count > 0 && Items.Num() < MaxSlots)
	{
		FInventoryItem NewItem;
		NewItem.ItemData = ItemData;
		NewItem.StackCount = FMath::Min(Count, ItemData->MaxStackSize);
		Count -= NewItem.StackCount;
		Items.Add(NewItem);
	}

	OnInventoryChanged.Broadcast();
	return Count <= 0;
}

bool UInventoryComponent::RemoveItem(const UItemDataAsset* ItemData, int32 Count)
{
	if (!ItemData || Count <= 0)
	{
		return false;
	}

	if (!HasItem(ItemData, Count))
	{
		return false;
	}

	for (int32 i = Items.Num() - 1; i >= 0 && Count > 0; --i)
	{
		if (Items[i].ItemData == ItemData)
		{
			const int32 ToRemove = FMath::Min(Count, Items[i].StackCount);
			Items[i].StackCount -= ToRemove;
			Count -= ToRemove;

			if (Items[i].StackCount <= 0)
			{
				Items.RemoveAt(i);
			}
		}
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UInventoryComponent::HasItem(const UItemDataAsset* ItemData, int32 Count) const
{
	if (!ItemData)
	{
		return false;
	}

	int32 Total = 0;
	for (const FInventoryItem& Item : Items)
	{
		if (Item.ItemData == ItemData)
		{
			Total += Item.StackCount;
			if (Total >= Count)
			{
				return true;
			}
		}
	}
	return false;
}

// ===== QUICK SLOTS =====

void UInventoryComponent::AssignQuickSlot(int32 SlotIndex, const UItemDataAsset* ItemData)
{
	if (SlotIndex < 0 || SlotIndex >= NUM_QUICK_SLOTS)
	{
		return;
	}
	QuickSlots[SlotIndex] = ItemData;
	OnInventoryChanged.Broadcast();
}

const UItemDataAsset* UInventoryComponent::GetQuickSlotItem(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= NUM_QUICK_SLOTS)
	{
		return nullptr;
	}
	return QuickSlots[SlotIndex];
}

int32 UInventoryComponent::GetQuickSlotCount(int32 SlotIndex) const
{
	const UItemDataAsset* SlotItem = GetQuickSlotItem(SlotIndex);
	if (!SlotItem)
	{
		return 0;
	}

	int32 Total = 0;
	for (const FInventoryItem& Item : Items)
	{
		if (Item.ItemData == SlotItem)
		{
			Total += Item.StackCount;
		}
	}
	return Total;
}

const UItemDataAsset* UInventoryComponent::UseQuickSlot(int32 SlotIndex)
{
	const UItemDataAsset* SlotItem = GetQuickSlotItem(SlotIndex);
	if (!SlotItem || !HasItem(SlotItem, 1))
	{
		return nullptr;
	}

	RemoveItem(SlotItem, 1);

	// If no more of this item, clear the slot
	if (!HasItem(SlotItem, 1))
	{
		QuickSlots[SlotIndex] = nullptr;
	}

	return SlotItem;
}

void UInventoryComponent::SetActiveQuickSlot(int32 SlotIndex)
{
	if (SlotIndex >= 0 && SlotIndex < NUM_QUICK_SLOTS)
	{
		ActiveQuickSlot = SlotIndex;
		OnInventoryChanged.Broadcast();
	}
}

void UInventoryComponent::CycleActiveQuickSlot(int32 Direction)
{
	ActiveQuickSlot = (ActiveQuickSlot + Direction + NUM_QUICK_SLOTS) % NUM_QUICK_SLOTS;
	OnInventoryChanged.Broadcast();
}
