// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SoulsLikeAIController.generated.h"

class UStateTreeAIComponent;

/**
 *  AI Controller for Souls-Like enemies.
 *  Runs behavior through the StateTree AI Component.
 */
UCLASS(abstract)
class ASoulsLikeAIController : public AAIController
{
	GENERATED_BODY()

	/** StateTree AI Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

public:

	/** Constructor */
	ASoulsLikeAIController();
};
