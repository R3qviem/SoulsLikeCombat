// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SLEndAction.generated.h"

/**
 *  AnimNotify placed at the end of attack/dodge montages to unlock the character state.
 *  Calls ISoulsLikeAttacker::OnActionEnd() which resets state to Idle and clears combat tracking.
 */
UCLASS()
class UAnimNotify_SLEndAction : public UAnimNotify
{
	GENERATED_BODY()

public:

	/** Perform the AnimNotify */
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Get the notify name for display in the editor */
	virtual FString GetNotifyName_Implementation() const override;
};
