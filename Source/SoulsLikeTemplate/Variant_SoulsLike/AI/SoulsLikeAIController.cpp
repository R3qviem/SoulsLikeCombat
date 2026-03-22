// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeAIController.h"
#include "Components/StateTreeAIComponent.h"

ASoulsLikeAIController::ASoulsLikeAIController()
{
	// Create the StateTree AI Component
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI);

	// Start AI logic when we possess a pawn
	bStartAILogicOnPossess = true;

	// Attach to pawn for EnvQueries
	bAttachToPawn = true;
}
