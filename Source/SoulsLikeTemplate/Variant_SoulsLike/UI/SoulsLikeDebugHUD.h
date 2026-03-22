// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SoulsLikeDebugHUD.generated.h"

/**
 *  Debug HUD that draws health, stamina, poise bars and character state
 *  directly on screen using Canvas. No Blueprint/UMG setup required.
 */
UCLASS()
class ASoulsLikeDebugHUD : public AHUD
{
	GENERATED_BODY()

protected:

	virtual void DrawHUD() override;

private:

	void DrawBar(float X, float Y, float Width, float Height, float Percent, FLinearColor FillColor, FLinearColor BackgroundColor, const FString& Label);
};
