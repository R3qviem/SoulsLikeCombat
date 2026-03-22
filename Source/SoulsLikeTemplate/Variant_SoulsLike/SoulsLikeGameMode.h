// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoulsLikeGameMode.generated.h"

/**
 *  GameMode for Souls-Like maps.
 *  Sets default pawn and controller classes.
 */
UCLASS()
class ASoulsLikeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	ASoulsLikeGameMode();
};
