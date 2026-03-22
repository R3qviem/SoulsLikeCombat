// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_SLComboWindow.h"
#include "SoulsLikeAttacker.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_SLComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (ISoulsLikeAttacker* Attacker = Cast<ISoulsLikeAttacker>(MeshComp->GetOwner()))
	{
		Attacker->OnComboWindowOpen();
	}
}

void UAnimNotify_SLComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (ISoulsLikeAttacker* Attacker = Cast<ISoulsLikeAttacker>(MeshComp->GetOwner()))
	{
		Attacker->OnComboWindowClose();
	}
}

FString UAnimNotify_SLComboWindow::GetNotifyName_Implementation() const
{
	return FString("SL Combo Window");
}
