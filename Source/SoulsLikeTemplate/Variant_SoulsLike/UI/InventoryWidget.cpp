// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryWidget.h"
#include "ItemDataAsset.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	// Dark semi-transparent background
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("InventoryBG"));
	WidgetTree->RootWidget = BackgroundBorder;
	BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f));
	BackgroundBorder->SetPadding(FMargin(20.0f));

	// Vertical layout: title + grid
	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("VBox"));
	BackgroundBorder->SetContent(VBox);

	// Title
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("Title"));
	TitleText->SetText(FText::FromString(TEXT("INVENTORY")));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 22;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.8f, 0.5f)));

	UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 15.0f));
	TitleSlot->SetHorizontalAlignment(HAlign_Center);

	// Item grid
	ItemGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), FName("ItemGrid"));
	ItemGrid->SetSlotPadding(FMargin(4.0f));
	ItemGrid->SetMinDesiredSlotWidth(100.0f);
	ItemGrid->SetMinDesiredSlotHeight(100.0f);

	UVerticalBoxSlot* GridSlot = VBox->AddChildToVerticalBox(ItemGrid);
	GridSlot->SetHorizontalAlignment(HAlign_Center);

	BuildGrid();
}

void UInventoryWidget::BuildGrid()
{
	SlotBorders.SetNum(TotalSlots);
	SlotIcons.SetNum(TotalSlots);
	SlotCounts.SetNum(TotalSlots);
	SlotNames.SetNum(TotalSlots);

	for (int32 i = 0; i < TotalSlots; ++i)
	{
		const int32 Row = i / Columns;
		const int32 Col = i % Columns;

		// Slot border (dark background)
		FName SlotName = *FString::Printf(TEXT("Slot_%d"), i);
		UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), SlotName);
		SlotBorder->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.9f));
		SlotBorder->SetPadding(FMargin(5.0f));
		SlotBorders[i] = SlotBorder;

		// Overlay to stack icon + text
		FName OverlayName = *FString::Printf(TEXT("Overlay_%d"), i);
		UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), OverlayName);
		SlotBorder->SetContent(SlotOverlay);

		// Icon image
		FName IconName = *FString::Printf(TEXT("Icon_%d"), i);
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		Icon->SetColorAndOpacity(FLinearColor(1, 1, 1, 0)); // hidden by default
		UOverlaySlot* IconSlot = SlotOverlay->AddChildToOverlay(Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		SlotIcons[i] = Icon;

		// Item name text (shown when no icon)
		FName NameLabel = *FString::Printf(TEXT("Name_%d"), i);
		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NameLabel);
		FSlateFontInfo NameFont = NameText->GetFont();
		NameFont.Size = 10;
		NameText->SetFont(NameFont);
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
		NameText->SetText(FText::GetEmpty());
		UOverlaySlot* NameSlot = SlotOverlay->AddChildToOverlay(NameText);
		NameSlot->SetHorizontalAlignment(HAlign_Center);
		NameSlot->SetVerticalAlignment(VAlign_Center);
		SlotNames[i] = NameText;

		// Stack count text (bottom-right)
		FName CountName = *FString::Printf(TEXT("Count_%d"), i);
		UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), CountName);
		FSlateFontInfo CountFont = CountText->GetFont();
		CountFont.Size = 12;
		CountText->SetFont(CountFont);
		CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		CountText->SetText(FText::GetEmpty());
		UOverlaySlot* CountSlot = SlotOverlay->AddChildToOverlay(CountText);
		CountSlot->SetHorizontalAlignment(HAlign_Right);
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
		SlotCounts[i] = CountText;

		ItemGrid->AddChildToUniformGrid(SlotBorder, Row, Col);
	}
}

void UInventoryWidget::RefreshInventory(const TArray<FInventoryItem>& Items)
{
	for (int32 i = 0; i < TotalSlots; ++i)
	{
		if (i < Items.Num() && Items[i].IsValid())
		{
			UpdateSlot(i, Items[i]);
		}
		else
		{
			ClearSlot(i);
		}
	}
}

void UInventoryWidget::UpdateSlot(int32 Index, const FInventoryItem& Item)
{
	if (!Item.ItemData)
	{
		ClearSlot(Index);
		return;
	}

	// Show icon if available
	if (Item.ItemData->ItemIcon && SlotIcons[Index])
	{
		SlotIcons[Index]->SetBrushFromTexture(Item.ItemData->ItemIcon);
		SlotIcons[Index]->SetColorAndOpacity(FLinearColor::White);
		SlotNames[Index]->SetText(FText::GetEmpty());
	}
	else if (SlotNames[Index])
	{
		// No icon — show item name
		SlotIcons[Index]->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
		SlotNames[Index]->SetText(Item.ItemData->ItemName);
	}

	// Show stack count if more than 1
	if (SlotCounts[Index])
	{
		if (Item.StackCount > 1)
		{
			SlotCounts[Index]->SetText(FText::AsNumber(Item.StackCount));
		}
		else
		{
			SlotCounts[Index]->SetText(FText::GetEmpty());
		}
	}

	// Highlight occupied slot
	if (SlotBorders[Index])
	{
		SlotBorders[Index]->SetBrushColor(FLinearColor(0.15f, 0.12f, 0.08f, 0.9f));
	}
}

void UInventoryWidget::ClearSlot(int32 Index)
{
	if (SlotIcons[Index])
	{
		SlotIcons[Index]->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
	}
	if (SlotNames[Index])
	{
		SlotNames[Index]->SetText(FText::GetEmpty());
	}
	if (SlotCounts[Index])
	{
		SlotCounts[Index]->SetText(FText::GetEmpty());
	}
	if (SlotBorders[Index])
	{
		SlotBorders[Index]->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.9f));
	}
}
