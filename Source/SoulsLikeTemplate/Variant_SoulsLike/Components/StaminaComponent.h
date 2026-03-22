// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulsLikeTypes.h"
#include "StaminaComponent.generated.h"

/**
 *  Manages stamina resource with delayed regeneration.
 *  All combat actions (attack, dodge, block) consume stamina through this component.
 */
UCLASS(ClassGroup = (SoulsLike), meta = (BlueprintSpawnableComponent))
class UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UStaminaComponent();

	/** Tick - handles stamina regeneration */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Attempt to consume stamina. Returns false if insufficient stamina.
	 * Resets the regeneration delay timer on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool ConsumeStamina(float Amount);

	/** Query whether there is enough stamina for an action, without consuming it */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool HasStamina(float Amount) const;

	/** Get current stamina as a percentage (0-1) */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetStaminaPercent() const;

	/** Get current stamina value */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	/** Get maximum stamina value */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	/** Reset stamina to maximum (for respawn) */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void ResetStamina();

	/** Broadcast when stamina changes */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStaminaChanged OnStaminaChanged;

protected:

	virtual void BeginPlay() override;

	/** Maximum stamina capacity */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = 1))
	float MaxStamina = 100.0f;

	/** Current stamina value */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina")
	float CurrentStamina = 100.0f;

	/** Stamina regenerated per second during active regen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = 0))
	float RegenRate = 20.0f;

	/** Seconds after the last stamina consumption before regeneration begins */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = 0, Units = "s"))
	float RegenDelay = 1.5f;

private:

	/** Time since stamina was last consumed */
	float TimeSinceLastUse = 0.0f;

	/** Whether stamina is currently regenerating */
	bool bIsRegenerating = false;
};
