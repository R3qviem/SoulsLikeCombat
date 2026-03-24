// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelUpWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/BackgroundBlur.h"

// Dark fantasy palette
static const FLinearColor LU_PanelBG(0.04f, 0.03f, 0.03f, 0.92f);
static const FLinearColor LU_HeaderBG(0.08f, 0.06f, 0.04f, 1.0f);
static const FLinearColor LU_RowBG(0.07f, 0.05f, 0.04f, 0.8f);
static const FLinearColor LU_RowAltBG(0.06f, 0.04f, 0.03f, 0.8f);
static const FLinearColor LU_Gold(0.7f, 0.55f, 0.2f, 1.0f);
static const FLinearColor LU_TextColor(0.85f, 0.8f, 0.7f, 1.0f);
static const FLinearColor LU_ButtonBG(0.15f, 0.12f, 0.08f, 1.0f);
static const FLinearColor LU_ButtonHover(0.25f, 0.2f, 0.12f, 1.0f);
static const FLinearColor LU_CloseBG(0.5f, 0.1f, 0.1f, 1.0f);

TSharedRef<SWidget> ULevelUpWidget::RebuildWidget()
{
	TSharedRef<SWidget> Widget = Super::RebuildWidget();
	BuildLayout();
	return Widget;
}

void ULevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &ULevelUpWidget::OnCloseClicked);
	}
}

void ULevelUpWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);

	// Restore game-only input
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void ULevelUpWidget::BuildLayout()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("LU_Root"));
		WidgetTree->RootWidget = RootCanvas;
	}

	// Semi-transparent dark background overlay
	UImage* BG = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName("LU_BG"));
	BG->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	UCanvasPanelSlot* BGSlot = RootCanvas->AddChildToCanvas(BG);
	BGSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BGSlot->SetOffsets(FMargin(0));

	// Main panel — centered
	MainPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("LU_MainPanel"));
	MainPanel->SetBrushColor(LU_PanelBG);
	MainPanel->SetPadding(FMargin(0));
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(MainPanel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	// Vertical layout inside panel
	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("LU_VBox"));
	MainPanel->SetContent(VBox);

	// === HEADER BAR (title + X button) ===
	UBorder* HeaderBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("LU_Header"));
	HeaderBorder->SetBrushColor(LU_HeaderBG);
	HeaderBorder->SetPadding(FMargin(16.0f, 10.0f));
	UVerticalBoxSlot* HeaderSlot = VBox->AddChildToVerticalBox(HeaderBorder);

	UHorizontalBox* HeaderHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName("LU_HeaderHBox"));
	HeaderBorder->SetContent(HeaderHBox);

	// Title
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("LU_Title"));
	Title->SetText(FText::FromString(TEXT("LEVEL UP")));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 22;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(LU_Gold));
	UHorizontalBoxSlot* TitleSlot = HeaderHBox->AddChildToHorizontalBox(Title);
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Close button (X)
	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName("LU_CloseBtn"));
	CloseButton->SetBackgroundColor(LU_CloseBG);
	UHorizontalBoxSlot* CloseBtnSlot = HeaderHBox->AddChildToHorizontalBox(CloseButton);
	CloseBtnSlot->SetVerticalAlignment(VAlign_Center);
	CloseBtnSlot->SetHorizontalAlignment(HAlign_Right);
	CloseBtnSlot->SetPadding(FMargin(20.0f, 0, 0, 0));

	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("LU_CloseText"));
	CloseText->SetText(FText::FromString(TEXT("  X  ")));
	FSlateFontInfo CloseFont = CloseText->GetFont();
	CloseFont.Size = 16;
	CloseText->SetFont(CloseFont);
	CloseText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CloseButton->SetContent(CloseText);

	// === LEVEL / SOULS INFO ===
	UBorder* InfoBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("LU_Info"));
	InfoBorder->SetBrushColor(LU_RowBG);
	InfoBorder->SetPadding(FMargin(16.0f, 12.0f));
	VBox->AddChildToVerticalBox(InfoBorder);

	UHorizontalBox* InfoHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName("LU_InfoHBox"));
	InfoBorder->SetContent(InfoHBox);

	// Current Level
	UTextBlock* LevelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("LU_LevelLabel"));
	LevelLabel->SetText(FText::FromString(TEXT("Level: 1")));
	FSlateFontInfo InfoFont = LevelLabel->GetFont();
	InfoFont.Size = 16;
	LevelLabel->SetFont(InfoFont);
	LevelLabel->SetColorAndOpacity(FSlateColor(LU_TextColor));
	UHorizontalBoxSlot* LevelSlot = InfoHBox->AddChildToHorizontalBox(LevelLabel);
	LevelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Souls available
	UTextBlock* SoulsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("LU_SoulsLabel"));
	SoulsLabel->SetText(FText::FromString(TEXT("Souls: 0")));
	SoulsLabel->SetFont(InfoFont);
	SoulsLabel->SetColorAndOpacity(FSlateColor(LU_Gold));
	InfoHBox->AddChildToHorizontalBox(SoulsLabel);

	// === STAT ROWS ===
	UBorder* StatsBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("LU_Stats"));
	StatsBorder->SetBrushColor(FLinearColor(0, 0, 0, 0));
	StatsBorder->SetPadding(FMargin(16.0f, 8.0f, 16.0f, 16.0f));
	VBox->AddChildToVerticalBox(StatsBorder);

	UVerticalBox* StatsVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("LU_StatsVBox"));
	StatsBorder->SetContent(StatsVBox);

	// Create stat rows
	const TArray<TPair<FString, int32>> Stats = {
		{TEXT("Vigor"), 10},
		{TEXT("Endurance"), 10},
		{TEXT("Strength"), 10},
		{TEXT("Dexterity"), 10},
		{TEXT("Intelligence"), 8},
		{TEXT("Faith"), 8}
	};

	for (int32 i = 0; i < Stats.Num(); ++i)
	{
		UBorder* Row = CreateStatRow(Stats[i].Key, Stats[i].Value);
		Row->SetBrushColor((i % 2 == 0) ? LU_RowBG : LU_RowAltBG);
		StatsVBox->AddChildToVerticalBox(Row);
	}

	// === BOTTOM HINT ===
	UBorder* HintBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("LU_Hint"));
	HintBorder->SetBrushColor(LU_HeaderBG);
	HintBorder->SetPadding(FMargin(16.0f, 8.0f));
	VBox->AddChildToVerticalBox(HintBorder);

	UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("LU_HintText"));
	HintText->SetText(FText::FromString(TEXT("Spend souls to increase your attributes")));
	FSlateFontInfo HintFont = HintText->GetFont();
	HintFont.Size = 11;
	HintText->SetFont(HintFont);
	HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.45f, 0.35f)));
	HintText->SetJustification(ETextJustify::Center);
	HintBorder->SetContent(HintText);
}

UBorder* ULevelUpWidget::CreateStatRow(const FString& StatName, int32 CurrentValue)
{
	UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowBorder->SetPadding(FMargin(12.0f, 8.0f));

	UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowBorder->SetContent(HBox);

	// Stat name
	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(StatName));
	FSlateFontInfo Font = NameText->GetFont();
	Font.Size = 14;
	NameText->SetFont(Font);
	NameText->SetColorAndOpacity(FSlateColor(LU_TextColor));
	UHorizontalBoxSlot* NameSlot = HBox->AddChildToHorizontalBox(NameText);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// Current value
	UTextBlock* ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ValueText->SetText(FText::FromString(FString::FromInt(CurrentValue)));
	ValueText->SetFont(Font);
	ValueText->SetColorAndOpacity(FSlateColor(LU_Gold));
	UHorizontalBoxSlot* ValueSlot = HBox->AddChildToHorizontalBox(ValueText);
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	ValueSlot->SetPadding(FMargin(20.0f, 0, 20.0f, 0));

	// + button (visual only)
	UButton* PlusBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	PlusBtn->SetBackgroundColor(LU_ButtonBG);
	UHorizontalBoxSlot* BtnSlot = HBox->AddChildToHorizontalBox(PlusBtn);
	BtnSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* PlusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PlusText->SetText(FText::FromString(TEXT(" + ")));
	FSlateFontInfo BtnFont = PlusText->GetFont();
	BtnFont.Size = 14;
	PlusText->SetFont(BtnFont);
	PlusText->SetColorAndOpacity(FSlateColor(LU_Gold));
	PlusBtn->SetContent(PlusText);

	return RowBorder;
}
