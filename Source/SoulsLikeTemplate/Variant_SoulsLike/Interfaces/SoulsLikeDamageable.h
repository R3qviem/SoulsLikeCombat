// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SoulsLikeTypes.h"
#include "SoulsLikeDamageable.generated.h"

/**
 *  SoulsLikeDamageable Interface
 *  Any actor that can receive damage implements this interface.
 *  Used by the damage system, lock-on filtering, and UI binding.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class USoulsLikeDamageable : public UInterface
{
	GENERATED_BODY()
};

class ISoulsLikeDamageable
{
	GENERATED_BODY()

public:

	/** Process incoming damage */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual void ReceiveDamage(const FDamageInfo& DamageInfo) = 0;

	/** Query alive/dead status */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual bool IsDead() const = 0;

	/** Get current health as percentage (0-1) for UI binding */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual float GetHealthPercent() const = 0;
};
