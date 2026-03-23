// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulsLikeTypes.h"
#include "InventoryComponent.generated.h"

class UItemDataAsset;

static constexpr int32 NUM_QUICK_SLOTS = 4;

/**
 *  Manages the player's item inventory and 4 quick-use slots.
 */
UCLASS(ClassGroup = (SoulsLike), meta = (BlueprintSpawnableComponent))
class UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInventoryComponent();

	/** Add an item to the inventory. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const UItemDataAsset* ItemData, int32 Count = 1);

	/** Remove an item from the inventory. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(const UItemDataAsset* ItemData, int32 Count = 1);

	/** Check if the inventory contains at least Count of the given item */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(const UItemDataAsset* ItemData, int32 Count = 1) const;

	/** Get all items */
	const TArray<FInventoryItem>& GetItems() const { return Items; }

	// ===== QUICK SLOTS (4 slots) =====

	/** Assign an item to a specific quick slot (0-3) */
	void AssignQuickSlot(int32 SlotIndex, const UItemDataAsset* ItemData);

	/** Get the item assigned to a quick slot */
	const UItemDataAsset* GetQuickSlotItem(int32 SlotIndex) const;

	/** Get the stack count for the item in a quick slot */
	int32 GetQuickSlotCount(int32 SlotIndex) const;

	/** Use the item in a specific quick slot. Returns the item data if used. */
	const UItemDataAsset* UseQuickSlot(int32 SlotIndex);

	/** Get the active quick slot index */
	int32 GetActiveQuickSlot() const { return ActiveQuickSlot; }

	/** Set the active quick slot (for HUD highlight) */
	void SetActiveQuickSlot(int32 SlotIndex);

	/** Cycle the active quick slot */
	void CycleActiveQuickSlot(int32 Direction = 1);

	/** Fired whenever the inventory contents change */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/** Maximum number of inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 1))
	int32 MaxSlots = 24;

protected:

	UPROPERTY()
	TArray<FInventoryItem> Items;

	/** Items assigned to each quick slot (can be nullptr) */
	UPROPERTY()
	TObjectPtr<const UItemDataAsset> QuickSlots[NUM_QUICK_SLOTS];

	/** Currently active quick slot for HUD highlight */
	int32 ActiveQuickSlot = 0;
};
