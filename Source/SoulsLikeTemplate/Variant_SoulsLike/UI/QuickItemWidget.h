// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuickItemWidget.generated.h"

class UTextBlock;
class UBorder;
class UImage;

static constexpr int32 QUICK_SLOT_COUNT = 4;

/**
 *  Bottom-left quick item bar (4 slots, Souls-style).
 *  Shows assigned consumables with the active slot highlighted.
 */
UCLASS()
class UQuickItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeOnInitialized() override;

	/** Update a specific slot */
	void SetSlot(int32 Index, const FText& Name, int32 Count, UTexture2D* Icon);

	/** Clear a specific slot */
	void ClearSlot(int32 Index);

	/** Highlight the active slot */
	void SetActiveSlot(int32 Index);

private:

	UPROPERTY()
	TObjectPtr<UBorder> SlotBorders[QUICK_SLOT_COUNT];

	UPROPERTY()
	TObjectPtr<UImage> SlotIcons[QUICK_SLOT_COUNT];

	UPROPERTY()
	TObjectPtr<UTextBlock> SlotCounts[QUICK_SLOT_COUNT];

	UPROPERTY()
	TObjectPtr<UTextBlock> SlotKeys[QUICK_SLOT_COUNT];

	int32 CurrentActiveSlot = 0;

	FLinearColor ActiveBorderColor = FLinearColor(0.6f, 0.45f, 0.15f, 0.9f);
	FLinearColor InactiveBorderColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.7f);
};
