// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoiseComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStanceBroken);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPoiseChanged, float, PoisePercent);

/**
 *  Poise (stance) system. Tracks how much stagger resistance a character has.
 *  Taking hits reduces poise. When poise reaches 0, the character's stance breaks
 *  and they become vulnerable to a riposte/finisher.
 *  Poise regenerates after a delay when not being hit.
 */
UCLASS(ClassGroup = (SoulsLike), meta = (BlueprintSpawnableComponent))
class UPoiseComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPoiseComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Apply poise damage. Returns true if stance was broken by this hit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise")
	bool ApplyPoiseDamage(float PoiseDamage);

	/** Reset poise to maximum */
	UFUNCTION(BlueprintCallable, Category = "Poise")
	void ResetPoise();

	/** Check if stance is currently broken */
	UFUNCTION(BlueprintPure, Category = "Poise")
	bool IsStanceBroken() const { return bStanceBroken; }

	/** Get current poise as a percentage (0-1) */
	UFUNCTION(BlueprintPure, Category = "Poise")
	float GetPoisePercent() const { return MaxPoise > 0.0f ? CurrentPoise / MaxPoise : 0.0f; }

	/** Force stance break (e.g. from a successful parry) */
	UFUNCTION(BlueprintCallable, Category = "Poise")
	void ForceStanceBreak();

	/** End the stance broken state (after riposte window expires or finisher is performed) */
	UFUNCTION(BlueprintCallable, Category = "Poise")
	void RecoverFromStanceBreak();

	/** Broadcast when stance is broken (poise reaches 0) */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStanceBroken OnStanceBroken;

	/** Broadcast when poise value changes */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPoiseChanged OnPoiseChanged;

	/** Maximum poise value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise", meta = (ClampMin = 1))
	float MaxPoise = 100.0f;

	/** Poise regeneration rate per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise", meta = (ClampMin = 0))
	float PoiseRegenRate = 30.0f;

	/** Delay in seconds before poise starts regenerating after taking poise damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise", meta = (ClampMin = 0))
	float PoiseRegenDelay = 3.0f;

	/** How long the stance broken state lasts (riposte window) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise", meta = (ClampMin = 0.1))
	float StanceBrokenDuration = 3.0f;

	/** Whether this character uses hyper armor during attacks (poise doesn't break during attack animations) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise")
	bool bHasHyperArmor = false;

private:

	UPROPERTY(VisibleAnywhere, Category = "Poise")
	float CurrentPoise;

	bool bStanceBroken = false;
	float TimeSinceLastPoiseDamage = 0.0f;
	FTimerHandle StanceBrokenTimerHandle;
};
