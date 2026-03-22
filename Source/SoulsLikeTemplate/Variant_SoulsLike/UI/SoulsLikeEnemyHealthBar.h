// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulsLikeEnemyHealthBar.generated.h"

/**
 *  World-space enemy health bar widget base class.
 *  Create a Widget Blueprint derived from this class and implement the visual layout.
 */
UCLASS(abstract)
class USoulsLikeEnemyHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Update the health bar fill percentage (0-1) */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Health Bar")
	void SetHealthPercentage(float Percent);
};
