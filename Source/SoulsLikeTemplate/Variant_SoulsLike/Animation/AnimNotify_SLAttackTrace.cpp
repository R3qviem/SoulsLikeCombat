// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_SLAttackTrace.h"
#include "SoulsLikeAttacker.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_SLAttackTrace::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (ISoulsLikeAttacker* Attacker = Cast<ISoulsLikeAttacker>(MeshComp->GetOwner()))
	{
		Attacker->ExecuteAttackTrace(AttackBoneName);
	}
}

FString UAnimNotify_SLAttackTrace::GetNotifyName_Implementation() const
{
	return FString("SL Attack Trace");
}
