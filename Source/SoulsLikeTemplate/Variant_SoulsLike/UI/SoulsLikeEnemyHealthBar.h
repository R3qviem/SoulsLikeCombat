// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulsLikeEnemyHealthBar.generated.h"

/**
 *  Self-contained enemy health bar widget.
 *  Builds its layout via NativeOnInitialized using the WidgetTree.
 */
UCLASS()
class USoulsLikeEnemyHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Update the health bar fill percentage (0-1) */
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void SetHealthPercent(float Percent);

	virtual void NativeOnInitialized() override;

private:

	/** Cached progress bar reference */
	UPROPERTY()
	TObjectPtr<class UProgressBar> HealthBar;
};
