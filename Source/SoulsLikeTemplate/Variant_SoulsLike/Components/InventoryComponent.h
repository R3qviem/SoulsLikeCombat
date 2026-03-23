// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulsLikeTypes.h"
#include "InventoryComponent.generated.h"

class UItemDataAsset;

/**
 *  Manages the player's item inventory.
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

	/** Fired whenever the inventory contents change */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/** Maximum number of inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 1))
	int32 MaxSlots = 20;

protected:

	UPROPERTY()
	TArray<FInventoryItem> Items;
};
