// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SLAttackTrace.generated.h"

/**
 *  AnimNotify that triggers hit detection during an attack animation.
 *  Casts the owning actor to ISoulsLikeAttacker and calls ExecuteAttackTrace().
 */
UCLASS()
class UAnimNotify_SLAttackTrace : public UAnimNotify
{
	GENERATED_BODY()

protected:

	/** Source bone/socket for the attack trace origin */
	UPROPERTY(EditAnywhere, Category = "Attack")
	FName AttackBoneName;

public:

	/** Perform the AnimNotify */
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Get the notify name for display in the editor */
	virtual FString GetNotifyName_Implementation() const override;
};
