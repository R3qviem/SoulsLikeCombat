// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuickItemWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"

void UQuickItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("Canvas"));
	WidgetTree->RootWidget = Canvas;

	// Horizontal bar of 4 slots, anchored bottom-left
	UBorder* BarBG = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("BarBG"));
	BarBG->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.01f, 0.6f));
	BarBG->SetPadding(FMargin(6.0f));

	UCanvasPanelSlot* BarSlot = Canvas->AddChildToCanvas(BarBG);
	BarSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
	BarSlot->SetAlignment(FVector2D(0.0f, 1.0f));
	BarSlot->SetPosition(FVector2D(20.0f, -20.0f));
	BarSlot->SetAutoSize(true);

	UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName("SlotsHBox"));
	BarBG->SetContent(HBox);

	for (int32 i = 0; i < QUICK_SLOT_COUNT; ++i)
	{
		// Slot border
		FName BorderName = *FString::Printf(TEXT("QSlot_%d"), i);
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), BorderName);
		Border->SetBrushColor(i == 0 ? ActiveBorderColor : InactiveBorderColor);
		Border->SetPadding(FMargin(3.0f));
		SlotBorders[i] = Border;

		UHorizontalBoxSlot* BorderSlot = HBox->AddChildToHorizontalBox(Border);
		BorderSlot->SetPadding(FMargin(2.0f));

		// Size box for consistent slot size
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			*FString::Printf(TEXT("QSize_%d"), i));
		SizeBox->SetWidthOverride(52.0f);
		SizeBox->SetHeightOverride(52.0f);
		Border->SetContent(SizeBox);

		// Overlay for icon + count + key label
		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),
			*FString::Printf(TEXT("QOverlay_%d"), i));
		SizeBox->SetContent(Overlay);

		// Icon
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("QIcon_%d"), i));
		Icon->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
		UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		SlotIcons[i] = Icon;

		// Stack count (bottom-right)
		UTextBlock* Count = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("QCount_%d"), i));
		FSlateFontInfo CountFont = Count->GetFont();
		CountFont.Size = 10;
		Count->SetFont(CountFont);
		Count->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Count->SetText(FText::GetEmpty());
		UOverlaySlot* CountSlot = Overlay->AddChildToOverlay(Count);
		CountSlot->SetHorizontalAlignment(HAlign_Right);
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
		SlotCounts[i] = Count;

		// Key label (top-left, e.g. "1", "2", "3", "4")
		UTextBlock* Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("QKey_%d"), i));
		FSlateFontInfo KeyFont = Key->GetFont();
		KeyFont.Size = 9;
		Key->SetFont(KeyFont);
		Key->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.45f, 0.3f)));
		Key->SetText(FText::AsNumber(i + 1));
		UOverlaySlot* KeySlot = Overlay->AddChildToOverlay(Key);
		KeySlot->SetHorizontalAlignment(HAlign_Left);
		KeySlot->SetVerticalAlignment(VAlign_Top);
		SlotKeys[i] = Key;
	}
}

void UQuickItemWidget::SetSlot(int32 Index, const FText& Name, int32 Count, UTexture2D* Icon)
{
	if (Index < 0 || Index >= QUICK_SLOT_COUNT)
	{
		return;
	}

	if (SlotIcons[Index])
	{
		if (Icon)
		{
			SlotIcons[Index]->SetBrushFromTexture(Icon);
			SlotIcons[Index]->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			SlotIcons[Index]->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
		}
	}

	if (SlotCounts[Index])
	{
		SlotCounts[Index]->SetText(Count > 0 ? FText::Format(FText::FromString(TEXT("x{0}")), Count) : FText::GetEmpty());
	}
}

void UQuickItemWidget::ClearSlot(int32 Index)
{
	if (Index < 0 || Index >= QUICK_SLOT_COUNT)
	{
		return;
	}

	if (SlotIcons[Index])
	{
		SlotIcons[Index]->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
	}
	if (SlotCounts[Index])
	{
		SlotCounts[Index]->SetText(FText::GetEmpty());
	}
}

void UQuickItemWidget::SetActiveSlot(int32 Index)
{
	if (Index < 0 || Index >= QUICK_SLOT_COUNT)
	{
		return;
	}

	// Deactivate old
	if (SlotBorders[CurrentActiveSlot])
	{
		SlotBorders[CurrentActiveSlot]->SetBrushColor(InactiveBorderColor);
	}

	CurrentActiveSlot = Index;

	// Activate new
	if (SlotBorders[CurrentActiveSlot])
	{
		SlotBorders[CurrentActiveSlot]->SetBrushColor(ActiveBorderColor);
	}
}
