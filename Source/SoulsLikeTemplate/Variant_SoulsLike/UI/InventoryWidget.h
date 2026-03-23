// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulsLikeTypes.h"
#include "InventoryWidget.generated.h"

class UVerticalBox;
class UHorizontalBox;
class UUniformGridPanel;
class UTextBlock;
class UBorder;
class UImage;
class UOverlay;
class UCanvasPanel;
class UBackgroundBlur;
class USizeBox;
class UInventoryComponent;

/**
 *  Full-screen interactive inventory overlay — dark fantasy style.
 *  Left panel: Character equipment slots.
 *  Right panel: Category-filtered item grid with click and double-click support.
 *  Double-click consumables to assign to quick slots 1-4.
 */
UCLASS()
class UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Refresh the inventory display with current items */
	void RefreshInventory(const TArray<FInventoryItem>& Items);

	/** Set the inventory component reference for quick slot assignment */
	void SetInventoryComponent(UInventoryComponent* InComp) { InventoryComp = InComp; }

	void PlayOpenAnimation();
	void PlayCloseAnimation();

private:

	// ===== CACHED DATA =====
	TArray<FInventoryItem> CachedItems;
	TArray<FInventoryItem> FilteredItems;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryComp;

	// ===== LAYOUT ROOTS =====
	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY()
	TObjectPtr<UBackgroundBlur> BlurBackground;

	UPROPERTY()
	TObjectPtr<UBorder> DarkOverlay;

	// ===== LEFT PANEL: EQUIPMENT =====
	UPROPERTY()
	TObjectPtr<UBorder> EquipmentPanel;

	UPROPERTY()
	TObjectPtr<UTextBlock> CharacterLabel;

	UPROPERTY()
	TMap<EEquipmentSlot, TObjectPtr<UBorder>> EquipSlotBorders;

	UPROPERTY()
	TMap<EEquipmentSlot, TObjectPtr<UTextBlock>> EquipSlotLabels;

	UPROPERTY()
	TMap<EEquipmentSlot, TObjectPtr<UImage>> EquipSlotIcons;

	// ===== RIGHT PANEL: ITEMS =====
	UPROPERTY()
	TObjectPtr<UBorder> ItemsPanel;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> CategoryBar;

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> ItemGrid;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> CategoryBorders;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> CategoryLabels;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> SlotBorders;

	UPROPERTY()
	TArray<TObjectPtr<UImage>> SlotIcons;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotCounts;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotNames;

	// ===== TOOLTIP =====
	UPROPERTY()
	TObjectPtr<UBorder> TooltipPanel;

	UPROPERTY()
	TObjectPtr<UTextBlock> TooltipTitle;

	UPROPERTY()
	TObjectPtr<UTextBlock> TooltipDesc;

	// ===== QUICK SLOT PICKER (shown on double-click) =====
	UPROPERTY()
	TObjectPtr<UBorder> QuickSlotPicker;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> QuickSlotPickerBorders;

	int32 PendingQuickSlotItemIndex = -1; // which filtered item is being assigned

	// ===== STATE =====
	int32 SelectedSlotIndex = -1;
	int32 HoveredSlotIndex = -1;
	int32 ActiveCategoryIndex = 0;
	EItemType ActiveCategory = EItemType::Material;
	bool bShowAll = true;

	// ===== SETTINGS =====
	int32 Columns = 6;
	int32 TotalSlots = 24;

	// ===== DARK FANTASY STYLE =====
	FLinearColor AccentColor = FLinearColor(0.7f, 0.55f, 0.2f, 1.0f);   // warm gold
	FLinearColor AccentDim = FLinearColor(0.45f, 0.35f, 0.15f, 0.8f);   // muted gold
	FLinearColor PanelBG = FLinearColor(0.02f, 0.02f, 0.03f, 0.9f);
	FLinearColor SlotBG = FLinearColor(0.04f, 0.04f, 0.06f, 0.85f);
	FLinearColor SlotHoverBG = FLinearColor(0.1f, 0.08f, 0.04f, 0.95f);
	FLinearColor SlotSelectedBG = FLinearColor(0.15f, 0.12f, 0.05f, 1.0f);
	FLinearColor SlotOccupiedBG = FLinearColor(0.06f, 0.05f, 0.04f, 0.9f);
	FLinearColor BorderColor = FLinearColor(0.3f, 0.25f, 0.12f, 0.5f);
	FLinearColor TextDim = FLinearColor(0.45f, 0.4f, 0.35f, 1.0f);
	FLinearColor TextBright = FLinearColor(0.9f, 0.85f, 0.7f, 1.0f);
	FLinearColor CategoryActiveBG = FLinearColor(0.12f, 0.1f, 0.05f, 0.9f);
	FLinearColor CategoryInactiveBG = FLinearColor(0.03f, 0.03f, 0.03f, 0.7f);

	// ===== BUILD METHODS =====
	void BuildLeftPanel(UCanvasPanel* Canvas);
	void BuildRightPanel(UCanvasPanel* Canvas);
	void BuildCategoryBar(UVerticalBox* Parent);
	void BuildItemGrid(UVerticalBox* Parent);
	void BuildTooltip(UCanvasPanel* Canvas);
	void BuildQuickSlotPicker(UCanvasPanel* Canvas);
	UBorder* CreateEquipSlot(const FText& Label, EEquipmentSlot EquipSlot);

	// ===== UPDATE METHODS =====
	void FilterByCategory(bool bAll, EItemType Type);
	void UpdateSlot(int32 Index, const FInventoryItem& Item);
	void ClearSlot(int32 Index);
	void SelectSlot(int32 Index);
	void HoverSlot(int32 Index);
	void ShowTooltip(const FText& Title, const FText& Description);
	void HideTooltip();
	void ShowQuickSlotPicker(int32 FilteredItemIndex);
	void HideQuickSlotPicker();

	// ===== HIT TESTING =====
	int32 HitTestSlots(const FPointerEvent& MouseEvent) const;
	int32 HitTestCategories(const FPointerEvent& MouseEvent) const;
	int32 HitTestQuickSlotPicker(const FPointerEvent& MouseEvent) const;
	bool IsWidgetUnderCursor(const UWidget* Widget, const FPointerEvent& MouseEvent) const;
};
