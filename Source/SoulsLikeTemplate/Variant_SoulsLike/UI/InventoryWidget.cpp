// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryWidget.h"
#include "ItemDataAsset.h"
#include "InventoryComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/BackgroundBlur.h"
#include "Components/Spacer.h"

// ===================================================================
// HIT TESTING — check if mouse is over a widget
// ===================================================================

bool UInventoryWidget::IsWidgetUnderCursor(const UWidget* Widget, const FPointerEvent& MouseEvent) const
{
	if (!Widget || !Widget->GetCachedWidget().IsValid())
	{
		return false;
	}
	const FGeometry& Geo = Widget->GetCachedGeometry();
	FVector2D LocalPos = Geo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	FVector2D Size = Geo.GetLocalSize();
	return LocalPos.X >= 0 && LocalPos.Y >= 0 && LocalPos.X <= Size.X && LocalPos.Y <= Size.Y;
}

int32 UInventoryWidget::HitTestSlots(const FPointerEvent& MouseEvent) const
{
	for (int32 i = 0; i < SlotBorders.Num(); ++i)
	{
		if (SlotBorders[i] && IsWidgetUnderCursor(SlotBorders[i], MouseEvent))
		{
			return i;
		}
	}
	return -1;
}

int32 UInventoryWidget::HitTestCategories(const FPointerEvent& MouseEvent) const
{
	for (int32 i = 0; i < CategoryBorders.Num(); ++i)
	{
		if (CategoryBorders[i] && IsWidgetUnderCursor(CategoryBorders[i], MouseEvent))
		{
			return i;
		}
	}
	return -1;
}

int32 UInventoryWidget::HitTestQuickSlotPicker(const FPointerEvent& MouseEvent) const
{
	for (int32 i = 0; i < QuickSlotPickerBorders.Num(); ++i)
	{
		if (QuickSlotPickerBorders[i] && IsWidgetUnderCursor(QuickSlotPickerBorders[i], MouseEvent))
		{
			return i;
		}
	}
	return -1;
}

// ===================================================================
// MOUSE EVENTS
// ===================================================================

FReply UInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.GetEffectingButton().IsValid() || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	// Check quick slot picker first (if visible)
	if (QuickSlotPicker && QuickSlotPicker->GetVisibility() == ESlateVisibility::Visible)
	{
		int32 PickerHit = HitTestQuickSlotPicker(InMouseEvent);
		if (PickerHit >= 0 && InventoryComp && PendingQuickSlotItemIndex >= 0 &&
			PendingQuickSlotItemIndex < FilteredItems.Num())
		{
			InventoryComp->AssignQuickSlot(PickerHit, FilteredItems[PendingQuickSlotItemIndex].ItemData);
			HideQuickSlotPicker();
			return FReply::Handled();
		}
		HideQuickSlotPicker();
	}

	// Check category tabs
	int32 CatHit = HitTestCategories(InMouseEvent);
	if (CatHit >= 0)
	{
		// Order: All=0, Weapons=1, Armor=2, Potions=3, Materials=4
		struct { bool bAll; EItemType Type; } CatMap[] = {
			{ true, EItemType::Material },
			{ false, EItemType::Weapon },
			{ false, EItemType::Armor },
			{ false, EItemType::Consumable },
			{ false, EItemType::Material },
		};
		if (CatHit < 5)
		{
			ActiveCategoryIndex = CatHit;
			FilterByCategory(CatMap[CatHit].bAll, CatMap[CatHit].Type);
		}
		return FReply::Handled();
	}

	// Check item slots
	int32 SlotHit = HitTestSlots(InMouseEvent);
	if (SlotHit >= 0)
	{
		SelectSlot(SlotHit);

		// Show tooltip if item exists
		if (SlotHit < FilteredItems.Num() && FilteredItems[SlotHit].IsValid())
		{
			ShowTooltip(FilteredItems[SlotHit].ItemData->ItemName, FilteredItems[SlotHit].ItemData->ItemDescription);
		}
		else
		{
			HideTooltip();
		}
		return FReply::Handled();
	}

	HideTooltip();
	return FReply::Handled();
}

FReply UInventoryWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	int32 SlotHit = HitTestSlots(InMouseEvent);
	if (SlotHit >= 0 && SlotHit < FilteredItems.Num() && FilteredItems[SlotHit].IsValid())
	{
		const UItemDataAsset* ItemData = FilteredItems[SlotHit].ItemData;
		if (ItemData && ItemData->bIsQuickUsable)
		{
			ShowQuickSlotPicker(SlotHit);
		}
	}
	return FReply::Handled();
}

FReply UInventoryWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	int32 SlotHit = HitTestSlots(InMouseEvent);
	HoverSlot(SlotHit);
	return FReply::Unhandled();
}

// ===================================================================
// INIT
// ===================================================================

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// Blur
	BlurBackground = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), FName("BlurBG"));
	BlurBackground->SetBlurStrength(8.0f);
	UCanvasPanelSlot* BlurSlot = RootCanvas->AddChildToCanvas(BlurBackground);
	BlurSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	BlurSlot->SetOffsets(FMargin(0));

	// Dark overlay
	DarkOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("DarkOverlay"));
	DarkOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	UCanvasPanelSlot* DarkSlot = RootCanvas->AddChildToCanvas(DarkOverlay);
	DarkSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	DarkSlot->SetOffsets(FMargin(0));

	// Equipment panel — anchored to left side, vertically centered
	BuildLeftPanel(RootCanvas);

	// Items panel — anchored to right side, vertically centered
	BuildRightPanel(RootCanvas);
	BuildTooltip(RootCanvas);
	BuildQuickSlotPicker(RootCanvas);
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideTooltip();
	HideQuickSlotPicker();
	SetIsFocusable(true);
}

// ===================================================================
// LEFT PANEL — Equipment (dark fantasy)
// ===================================================================

void UInventoryWidget::BuildLeftPanel(UCanvasPanel* Canvas)
{
	EquipmentPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("EquipPanel"));
	EquipmentPanel->SetBrushColor(PanelBG);
	EquipmentPanel->SetPadding(FMargin(20.0f));

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(EquipmentPanel);
	PanelSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.0f, 0.5f));
	PanelSlot->SetPosition(FVector2D(40.0f, 0.0f));
	PanelSlot->SetAutoSize(true);

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("EquipSize"));
	SizeBox->SetWidthOverride(280.0f);
	EquipmentPanel->SetContent(SizeBox);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("EquipVBox"));
	SizeBox->SetContent(VBox);

	// Title
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("EquipTitle"));
	Title->SetText(FText::FromString(TEXT("EQUIPMENT")));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 16;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(AccentColor));
	UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

	// Gold separator
	UBorder* Sep = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("EquipSep"));
	Sep->SetBrushColor(BorderColor);
	UVerticalBoxSlot* SepSlot = VBox->AddChildToVerticalBox(Sep);
	SepSlot->SetPadding(FMargin(15, 0, 15, 12));
	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("SepSize"));
	SepSize->SetHeightOverride(1.0f);
	Sep->SetContent(SepSize);

	// Character silhouette
	CharacterLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("CharLabel"));
	CharacterLabel->SetText(FText::FromString(TEXT("— Character —")));
	FSlateFontInfo CharFont = CharacterLabel->GetFont();
	CharFont.Size = 12;
	CharacterLabel->SetFont(CharFont);
	CharacterLabel->SetColorAndOpacity(FSlateColor(TextDim));
	UVerticalBoxSlot* CharSlot = VBox->AddChildToVerticalBox(CharacterLabel);
	CharSlot->SetHorizontalAlignment(HAlign_Center);
	CharSlot->SetPadding(FMargin(0, 8, 0, 16));

	// Equipment slots
	struct FSlotDef { FText Name; EEquipmentSlot ESlot; };
	TArray<FSlotDef> Slots = {
		{ FText::FromString(TEXT("Head")), EEquipmentSlot::Head },
		{ FText::FromString(TEXT("Chest")), EEquipmentSlot::Chest },
		{ FText::FromString(TEXT("Legs")), EEquipmentSlot::Legs },
		{ FText::FromString(TEXT("Main Hand")), EEquipmentSlot::MainHand },
		{ FText::FromString(TEXT("Off Hand")), EEquipmentSlot::OffHand },
	};

	for (const FSlotDef& Def : Slots)
	{
		UBorder* SlotWidget = CreateEquipSlot(Def.Name, Def.ESlot);
		UVerticalBoxSlot* SlotVSlot = VBox->AddChildToVerticalBox(SlotWidget);
		SlotVSlot->SetPadding(FMargin(0, 2, 0, 2));
	}
}

UBorder* UInventoryWidget::CreateEquipSlot(const FText& Label, EEquipmentSlot EquipSlot)
{
	FName SlotName = *FString::Printf(TEXT("EqSlot_%d"), (int32)EquipSlot);
	UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), SlotName);
	SlotBorder->SetBrushColor(SlotBG);
	SlotBorder->SetPadding(FMargin(10, 7, 10, 7));
	EquipSlotBorders.Add(EquipSlot, SlotBorder);

	UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
		*FString::Printf(TEXT("EqHBox_%d"), (int32)EquipSlot));
	SlotBorder->SetContent(HBox);

	// Icon placeholder
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
		*FString::Printf(TEXT("EqIconSize_%d"), (int32)EquipSlot));
	IconSize->SetWidthOverride(32.0f);
	IconSize->SetHeightOverride(32.0f);

	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		*FString::Printf(TEXT("EqIcon_%d"), (int32)EquipSlot));
	Icon->SetColorAndOpacity(FLinearColor(0.2f, 0.18f, 0.15f, 0.5f));
	IconSize->SetContent(Icon);
	EquipSlotIcons.Add(EquipSlot, Icon);

	UHorizontalBoxSlot* IconSlot = HBox->AddChildToHorizontalBox(IconSize);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0, 0, 8, 0));

	// Label
	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("EqLabel_%d"), (int32)EquipSlot));
	LabelText->SetText(Label);
	FSlateFontInfo LabelFont = LabelText->GetFont();
	LabelFont.Size = 12;
	LabelText->SetFont(LabelFont);
	LabelText->SetColorAndOpacity(FSlateColor(TextDim));
	EquipSlotLabels.Add(EquipSlot, LabelText);

	UHorizontalBoxSlot* LabelSlot = HBox->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	// Spacer
	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(),
		*FString::Printf(TEXT("EqSpacer_%d"), (int32)EquipSlot));
	UHorizontalBoxSlot* SpacerSlot = HBox->AddChildToHorizontalBox(Spacer);
	SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// "Empty" label
	UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("EqEmpty_%d"), (int32)EquipSlot));
	EmptyText->SetText(FText::FromString(TEXT("—")));
	FSlateFontInfo EmptyFont = EmptyText->GetFont();
	EmptyFont.Size = 11;
	EmptyText->SetFont(EmptyFont);
	EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.22f, 0.18f)));
	UHorizontalBoxSlot* EmptySlot = HBox->AddChildToHorizontalBox(EmptyText);
	EmptySlot->SetVerticalAlignment(VAlign_Center);

	return SlotBorder;
}

// ===================================================================
// RIGHT PANEL — Items
// ===================================================================

void UInventoryWidget::BuildRightPanel(UCanvasPanel* Canvas)
{
	ItemsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("ItemsPanel"));
	ItemsPanel->SetBrushColor(PanelBG);
	ItemsPanel->SetPadding(FMargin(20.0f));

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(ItemsPanel);
	PanelSlot->SetAnchors(FAnchors(1.0f, 0.5f, 1.0f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(1.0f, 0.5f));
	PanelSlot->SetPosition(FVector2D(-40.0f, 0.0f));
	PanelSlot->SetAutoSize(true);

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("ItemsSize"));
	SizeBox->SetWidthOverride(560.0f);
	ItemsPanel->SetContent(SizeBox);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("ItemsVBox"));
	SizeBox->SetContent(VBox);

	// Title
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("ItemsTitle"));
	Title->SetText(FText::FromString(TEXT("INVENTORY")));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 16;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(AccentColor));
	UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0, 0, 0, 8));

	BuildCategoryBar(VBox);

	// Separator
	UBorder* Sep = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("ItemSep"));
	Sep->SetBrushColor(BorderColor);
	UVerticalBoxSlot* SepSlot = VBox->AddChildToVerticalBox(Sep);
	SepSlot->SetPadding(FMargin(0, 6, 0, 8));
	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("ItemSepSize"));
	SepSize->SetHeightOverride(1.0f);
	Sep->SetContent(SepSize);

	BuildItemGrid(VBox);

	// Hint text
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("Hint"));
	Hint->SetText(FText::FromString(TEXT("Click to select  |  Double-click consumable to assign quick slot")));
	FSlateFontInfo HintFont = Hint->GetFont();
	HintFont.Size = 9;
	Hint->SetFont(HintFont);
	Hint->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.28f, 0.22f)));
	UVerticalBoxSlot* HintSlot = VBox->AddChildToVerticalBox(Hint);
	HintSlot->SetHorizontalAlignment(HAlign_Center);
	HintSlot->SetPadding(FMargin(0, 8, 0, 0));
}

void UInventoryWidget::BuildCategoryBar(UVerticalBox* Parent)
{
	CategoryBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName("CatBar"));
	UVerticalBoxSlot* BarSlot = Parent->AddChildToVerticalBox(CategoryBar);
	BarSlot->SetHorizontalAlignment(HAlign_Center);

	TArray<FString> CatNames = { TEXT("All"), TEXT("Weapons"), TEXT("Armor"), TEXT("Potions"), TEXT("Materials") };

	for (int32 i = 0; i < CatNames.Num(); ++i)
	{
		UBorder* CatBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			*FString::Printf(TEXT("CatBorder_%d"), i));
		CatBorder->SetBrushColor(i == 0 ? CategoryActiveBG : CategoryInactiveBG);
		CatBorder->SetPadding(FMargin(14, 5, 14, 5));
		CategoryBorders.Add(CatBorder);

		UTextBlock* CatText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("CatText_%d"), i));
		CatText->SetText(FText::FromString(CatNames[i]));
		FSlateFontInfo CatFont = CatText->GetFont();
		CatFont.Size = 11;
		CatText->SetFont(CatFont);
		CatText->SetColorAndOpacity(FSlateColor(i == 0 ? AccentColor : TextDim));
		CategoryLabels.Add(CatText);

		CatBorder->SetContent(CatText);

		UHorizontalBoxSlot* CatSlot = CategoryBar->AddChildToHorizontalBox(CatBorder);
		CatSlot->SetPadding(FMargin(2, 0, 2, 0));
	}
}

void UInventoryWidget::BuildItemGrid(UVerticalBox* Parent)
{
	ItemGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), FName("ItemGridPanel"));
	ItemGrid->SetSlotPadding(FMargin(6.0f));
	ItemGrid->SetMinDesiredSlotWidth(80.0f);
	ItemGrid->SetMinDesiredSlotHeight(80.0f);
	Parent->AddChildToVerticalBox(ItemGrid);

	SlotBorders.SetNum(TotalSlots);
	SlotIcons.SetNum(TotalSlots);
	SlotCounts.SetNum(TotalSlots);
	SlotNames.SetNum(TotalSlots);

	for (int32 i = 0; i < TotalSlots; ++i)
	{
		UBorder* SBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			*FString::Printf(TEXT("ISlot_%d"), i));
		SBorder->SetBrushColor(SlotBG);
		SBorder->SetPadding(FMargin(4.0f));
		SlotBorders[i] = SBorder;

		UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),
			*FString::Printf(TEXT("IOverlay_%d"), i));
		SBorder->SetContent(SlotOverlay);

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("IIcon_%d"), i));
		Icon->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
		UOverlaySlot* IconOSlot = SlotOverlay->AddChildToOverlay(Icon);
		IconOSlot->SetHorizontalAlignment(HAlign_Center);
		IconOSlot->SetVerticalAlignment(VAlign_Center);
		SlotIcons[i] = Icon;

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("IName_%d"), i));
		FSlateFontInfo NFont = NameText->GetFont();
		NFont.Size = 9;
		NameText->SetFont(NFont);
		NameText->SetColorAndOpacity(FSlateColor(TextDim));
		UOverlaySlot* NameOSlot = SlotOverlay->AddChildToOverlay(NameText);
		NameOSlot->SetHorizontalAlignment(HAlign_Center);
		NameOSlot->SetVerticalAlignment(VAlign_Center);
		SlotNames[i] = NameText;

		UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("ICount_%d"), i));
		FSlateFontInfo CFont = CountText->GetFont();
		CFont.Size = 10;
		CountText->SetFont(CFont);
		CountText->SetColorAndOpacity(FSlateColor(AccentDim));
		UOverlaySlot* CountOSlot = SlotOverlay->AddChildToOverlay(CountText);
		CountOSlot->SetHorizontalAlignment(HAlign_Right);
		CountOSlot->SetVerticalAlignment(VAlign_Bottom);
		SlotCounts[i] = CountText;

		ItemGrid->AddChildToUniformGrid(SBorder, i / Columns, i % Columns);
	}
}

// ===================================================================
// TOOLTIP
// ===================================================================

void UInventoryWidget::BuildTooltip(UCanvasPanel* Canvas)
{
	TooltipPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("Tooltip"));
	TooltipPanel->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.02f, 0.95f));
	TooltipPanel->SetPadding(FMargin(14, 10, 14, 10));

	UCanvasPanelSlot* TipSlot = Canvas->AddChildToCanvas(TooltipPanel);
	TipSlot->SetAnchors(FAnchors(1.0f, 0.5f, 1.0f, 0.5f));
	TipSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	TipSlot->SetPosition(FVector2D(-30.0f, 0.0f));
	TipSlot->SetAutoSize(true);

	UVerticalBox* TipVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("TipVBox"));
	TooltipPanel->SetContent(TipVBox);

	TooltipTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("TipTitle"));
	FSlateFontInfo TipFont = TooltipTitle->GetFont();
	TipFont.Size = 13;
	TooltipTitle->SetFont(TipFont);
	TooltipTitle->SetColorAndOpacity(FSlateColor(AccentColor));
	UVerticalBoxSlot* TipTitleSlot = TipVBox->AddChildToVerticalBox(TooltipTitle);
	TipTitleSlot->SetPadding(FMargin(0, 0, 0, 4));

	TooltipDesc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("TipDesc"));
	FSlateFontInfo DescFont = TooltipDesc->GetFont();
	DescFont.Size = 10;
	TooltipDesc->SetFont(DescFont);
	TooltipDesc->SetColorAndOpacity(FSlateColor(TextDim));

	USizeBox* DescSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("TipDescSize"));
	DescSize->SetWidthOverride(200.0f);
	DescSize->SetContent(TooltipDesc);
	TipVBox->AddChildToVerticalBox(DescSize);
}

void UInventoryWidget::ShowTooltip(const FText& Title, const FText& Description)
{
	if (!TooltipPanel) return;
	TooltipTitle->SetText(Title);
	TooltipDesc->SetText(Description);
	TooltipPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UInventoryWidget::HideTooltip()
{
	if (TooltipPanel)
	{
		TooltipPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ===================================================================
// QUICK SLOT PICKER — appears on double-click of consumable
// ===================================================================

void UInventoryWidget::BuildQuickSlotPicker(UCanvasPanel* Canvas)
{
	QuickSlotPicker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("QSPicker"));
	QuickSlotPicker->SetBrushColor(FLinearColor(0.04f, 0.03f, 0.02f, 0.95f));
	QuickSlotPicker->SetPadding(FMargin(10, 8, 10, 8));

	UCanvasPanelSlot* PickerSlot = Canvas->AddChildToCanvas(QuickSlotPicker);
	PickerSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	PickerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PickerSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("QSVBox"));
	QuickSlotPicker->SetContent(VBox);

	UTextBlock* PickerTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("QSTitle"));
	PickerTitle->SetText(FText::FromString(TEXT("Assign to Quick Slot")));
	FSlateFontInfo PTFont = PickerTitle->GetFont();
	PTFont.Size = 12;
	PickerTitle->SetFont(PTFont);
	PickerTitle->SetColorAndOpacity(FSlateColor(AccentColor));
	UVerticalBoxSlot* PTSlot = VBox->AddChildToVerticalBox(PickerTitle);
	PTSlot->SetHorizontalAlignment(HAlign_Center);
	PTSlot->SetPadding(FMargin(0, 0, 0, 8));

	UHorizontalBox* SlotsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName("QSRow"));
	VBox->AddChildToVerticalBox(SlotsRow);

	for (int32 i = 0; i < 4; ++i)
	{
		UBorder* SlotBtn = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			*FString::Printf(TEXT("QSBtn_%d"), i));
		SlotBtn->SetBrushColor(SlotBG);
		SlotBtn->SetPadding(FMargin(16, 10, 16, 10));
		QuickSlotPickerBorders.Add(SlotBtn);

		UTextBlock* SlotLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("QSLabel_%d"), i));
		SlotLabel->SetText(FText::Format(FText::FromString(TEXT("Slot {0}")), i + 1));
		FSlateFontInfo SFont = SlotLabel->GetFont();
		SFont.Size = 12;
		SlotLabel->SetFont(SFont);
		SlotLabel->SetColorAndOpacity(FSlateColor(TextBright));
		SlotBtn->SetContent(SlotLabel);

		UHorizontalBoxSlot* BtnSlot = SlotsRow->AddChildToHorizontalBox(SlotBtn);
		BtnSlot->SetPadding(FMargin(4, 0, 4, 0));
	}
}

void UInventoryWidget::ShowQuickSlotPicker(int32 FilteredItemIndex)
{
	PendingQuickSlotItemIndex = FilteredItemIndex;
	if (QuickSlotPicker)
	{
		QuickSlotPicker->SetVisibility(ESlateVisibility::Visible);
	}
}

void UInventoryWidget::HideQuickSlotPicker()
{
	PendingQuickSlotItemIndex = -1;
	if (QuickSlotPicker)
	{
		QuickSlotPicker->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ===================================================================
// REFRESH & FILTERING
// ===================================================================

void UInventoryWidget::RefreshInventory(const TArray<FInventoryItem>& Items)
{
	CachedItems = Items;
	FilterByCategory(bShowAll, ActiveCategory);
}

void UInventoryWidget::FilterByCategory(bool bAll, EItemType Type)
{
	bShowAll = bAll;
	ActiveCategory = Type;

	// Update category highlights
	for (int32 i = 0; i < CategoryBorders.Num(); ++i)
	{
		bool bActive = (i == ActiveCategoryIndex);
		if (CategoryBorders[i])
		{
			CategoryBorders[i]->SetBrushColor(bActive ? CategoryActiveBG : CategoryInactiveBG);
		}
		if (i < CategoryLabels.Num() && CategoryLabels[i])
		{
			CategoryLabels[i]->SetColorAndOpacity(FSlateColor(bActive ? AccentColor : TextDim));
		}
	}

	// Filter
	FilteredItems.Empty();
	for (const FInventoryItem& Item : CachedItems)
	{
		if (Item.IsValid() && (bAll || Item.ItemData->ItemType == Type))
		{
			FilteredItems.Add(Item);
		}
	}

	for (int32 i = 0; i < TotalSlots; ++i)
	{
		if (i < FilteredItems.Num())
		{
			UpdateSlot(i, FilteredItems[i]);
		}
		else
		{
			ClearSlot(i);
		}
	}

	HideTooltip();
	HideQuickSlotPicker();
	SelectedSlotIndex = -1;
	HoveredSlotIndex = -1;
}

void UInventoryWidget::UpdateSlot(int32 Index, const FInventoryItem& Item)
{
	if (!Item.ItemData) { ClearSlot(Index); return; }

	if (Item.ItemData->ItemIcon && SlotIcons.IsValidIndex(Index) && SlotIcons[Index])
	{
		SlotIcons[Index]->SetBrushFromTexture(Item.ItemData->ItemIcon);
		SlotIcons[Index]->SetColorAndOpacity(FLinearColor::White);
		if (SlotNames.IsValidIndex(Index) && SlotNames[Index])
			SlotNames[Index]->SetText(FText::GetEmpty());
	}
	else if (SlotNames.IsValidIndex(Index) && SlotNames[Index])
	{
		if (SlotIcons.IsValidIndex(Index) && SlotIcons[Index])
			SlotIcons[Index]->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
		SlotNames[Index]->SetText(Item.ItemData->ItemName);
	}

	if (SlotCounts.IsValidIndex(Index) && SlotCounts[Index])
		SlotCounts[Index]->SetText(Item.StackCount > 1 ? FText::AsNumber(Item.StackCount) : FText::GetEmpty());

	if (SlotBorders.IsValidIndex(Index) && SlotBorders[Index])
		SlotBorders[Index]->SetBrushColor(SlotOccupiedBG);
}

void UInventoryWidget::ClearSlot(int32 Index)
{
	if (SlotIcons.IsValidIndex(Index) && SlotIcons[Index])
		SlotIcons[Index]->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
	if (SlotNames.IsValidIndex(Index) && SlotNames[Index])
		SlotNames[Index]->SetText(FText::GetEmpty());
	if (SlotCounts.IsValidIndex(Index) && SlotCounts[Index])
		SlotCounts[Index]->SetText(FText::GetEmpty());
	if (SlotBorders.IsValidIndex(Index) && SlotBorders[Index])
		SlotBorders[Index]->SetBrushColor(SlotBG);
}

void UInventoryWidget::SelectSlot(int32 Index)
{
	// Deselect previous
	if (SelectedSlotIndex >= 0 && SlotBorders.IsValidIndex(SelectedSlotIndex) && SlotBorders[SelectedSlotIndex])
	{
		bool bOccupied = SelectedSlotIndex < FilteredItems.Num() && FilteredItems[SelectedSlotIndex].IsValid();
		SlotBorders[SelectedSlotIndex]->SetBrushColor(bOccupied ? SlotOccupiedBG : SlotBG);
	}

	SelectedSlotIndex = Index;

	if (Index >= 0 && SlotBorders.IsValidIndex(Index) && SlotBorders[Index])
	{
		SlotBorders[Index]->SetBrushColor(SlotSelectedBG);
	}
}

void UInventoryWidget::HoverSlot(int32 Index)
{
	// Unhover previous
	if (HoveredSlotIndex >= 0 && HoveredSlotIndex != SelectedSlotIndex &&
		SlotBorders.IsValidIndex(HoveredSlotIndex) && SlotBorders[HoveredSlotIndex])
	{
		bool bOccupied = HoveredSlotIndex < FilteredItems.Num() && FilteredItems[HoveredSlotIndex].IsValid();
		SlotBorders[HoveredSlotIndex]->SetBrushColor(bOccupied ? SlotOccupiedBG : SlotBG);
	}

	HoveredSlotIndex = Index;

	if (Index >= 0 && Index != SelectedSlotIndex &&
		SlotBorders.IsValidIndex(Index) && SlotBorders[Index])
	{
		SlotBorders[Index]->SetBrushColor(SlotHoverBG);
	}
}

// ===================================================================
// ANIMATIONS
// ===================================================================

void UInventoryWidget::PlayOpenAnimation()
{
	SetRenderOpacity(1.0f);
}

void UInventoryWidget::PlayCloseAnimation()
{
	SetRenderOpacity(0.0f);
}
