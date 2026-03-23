// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulsLikeTypes.h"
#include "InventoryWidget.generated.h"

class UVerticalBox;
class UUniformGridPanel;
class UTextBlock;
class UBorder;
class UImage;

/**
 *  Inventory screen widget — built entirely in C++ via WidgetTree.
 *  Shows a grid of item slots with icons and stack counts.
 */
UCLASS()
class UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeOnInitialized() override;

	/** Refresh the inventory display with current items */
	void RefreshInventory(const TArray<FInventoryItem>& Items);

private:

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> ItemGrid;

	/** Slot UI references */
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> SlotBorders;

	UPROPERTY()
	TArray<TObjectPtr<UImage>> SlotIcons;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotCounts;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotNames;

	int32 Columns = 5;
	int32 TotalSlots = 20;

	void BuildGrid();
	void UpdateSlot(int32 Index, const FInventoryItem& Item);
	void ClearSlot(int32 Index);
};
