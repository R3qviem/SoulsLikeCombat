// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotify_SLComboWindow.generated.h"

/**
 *  AnimNotifyState that marks the combo input window on the montage timeline.
 *  NotifyBegin opens the window, NotifyEnd closes it.
 *  Duration is authored visually on the montage timeline in the editor.
 */
UCLASS()
class UAnimNotify_SLComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	/** Called when the notify state begins */
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	/** Called when the notify state ends */
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Get the notify name for display in the editor */
	virtual FString GetNotifyName_Implementation() const override;
};
