// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulsLikeHUD.generated.h"

/**
 *  Player HUD widget base class.
 *  Provides BlueprintImplementableEvent hooks for health bar, stamina bar, lock-on indicator, and boss health.
 *  Create a Widget Blueprint derived from this class and implement the visual layout.
 */
UCLASS(abstract)
class USoulsLikeHUD : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Update the player health bar (0-1) */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void SetHealthPercent(float Percent);

	/** Update the player stamina bar (0-1) */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void SetStaminaPercent(float Percent);

	/** Show or hide the lock-on target indicator */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void ShowLockOnIndicator(bool bShow);

	/** Show a boss health bar with name and percentage */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void ShowBossHealthBar(float Percent, const FText& BossName);

	/** Hide the boss health bar */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void HideBossHealthBar();
};
