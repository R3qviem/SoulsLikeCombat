// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_SLEndAction.h"
#include "SoulsLikeAttacker.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_SLEndAction::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (ISoulsLikeAttacker* Attacker = Cast<ISoulsLikeAttacker>(MeshComp->GetOwner()))
	{
		Attacker->OnActionEnd();
	}
}

FString UAnimNotify_SLEndAction::GetNotifyName_Implementation() const
{
	return FString("SL End Action");
}
