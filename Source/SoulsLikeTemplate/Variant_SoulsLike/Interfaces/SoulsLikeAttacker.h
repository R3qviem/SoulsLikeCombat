// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SoulsLikeAttacker.generated.h"

/**
 *  SoulsLikeAttacker Interface
 *  Any actor that can initiate attacks implements this interface.
 *  AnimNotifies cast the owning actor to this interface to trigger attack logic.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class USoulsLikeAttacker : public UInterface
{
	GENERATED_BODY()
};

class ISoulsLikeAttacker
{
	GENERATED_BODY()

public:

	/** Perform the sphere sweep hit detection for the current attack. Called from AnimNotify_SLAttackTrace. */
	UFUNCTION(BlueprintCallable, Category = "Attacker")
	virtual void ExecuteAttackTrace(FName SourceBone) = 0;

	/** Notify that the combo input window is now open. Called from AnimNotify_SLComboWindow NotifyBegin. */
	UFUNCTION(BlueprintCallable, Category = "Attacker")
	virtual void OnComboWindowOpen() = 0;

	/** Notify that the combo input window has closed. Called from AnimNotify_SLComboWindow NotifyEnd. */
	UFUNCTION(BlueprintCallable, Category = "Attacker")
	virtual void OnComboWindowClose() = 0;

	/** Notify that the current action montage has finished its committed portion. Called from AnimNotify_SLEndAction. */
	UFUNCTION(BlueprintCallable, Category = "Attacker")
	virtual void OnActionEnd() = 0;
};
