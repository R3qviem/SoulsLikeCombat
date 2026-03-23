// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulsLikeTypes.h"
#include "ItemDataAsset.generated.h"

/**
 *  Defines a single item type in the game.
 *  Create instances in the Content Browser to define items.
 */
UCLASS(BlueprintType)
class UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Display name shown in inventory */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	/** Description shown when selected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText ItemDescription;

	/** Item category */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::Material;

	/** Icon displayed in the inventory grid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> ItemIcon;

	/** Maximum stack size (1 = not stackable) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 MaxStackSize = 1;

	/** Whether this item can be assigned to the quick slot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consumable")
	bool bIsQuickUsable = false;

	/** Health restored when used (for consumables) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = 0, EditCondition = "bIsQuickUsable"))
	float HealAmount = 0.0f;
};
