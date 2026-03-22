// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeEnemyHealthBar.h"
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetTree.h"

void USoulsLikeEnemyHealthBar::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Build the widget tree: a single progress bar as root
	if (WidgetTree)
	{
		HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName("HealthProgressBar"));

		if (HealthBar)
		{
			WidgetTree->RootWidget = HealthBar;

			HealthBar->SetPercent(1.0f);
			HealthBar->SetFillColorAndOpacity(FLinearColor(0.8f, 0.05f, 0.05f, 1.0f));

			// Style: dark background, red fill
			FProgressBarStyle Style = HealthBar->GetWidgetStyle();
			FSlateBrush BackgroundBrush;
			BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.15f, 0.15f, 0.15f, 0.9f));
			Style.SetBackgroundImage(BackgroundBrush);
			FSlateBrush FillBrush;
			FillBrush.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
			Style.SetFillImage(FillBrush);
			HealthBar->SetWidgetStyle(Style);
		}
	}
}

void USoulsLikeEnemyHealthBar::SetHealthPercent(float Percent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}
