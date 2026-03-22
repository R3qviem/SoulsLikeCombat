// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulsLikeTypes.h"
#include "CharacterStateComponent.generated.h"

/**
 *  Central state machine component for Souls-Like characters.
 *  All other systems query this component before executing actions.
 *  State transitions are validated against a rules table.
 */
UCLASS(ClassGroup = (SoulsLike), meta = (BlueprintSpawnableComponent))
class UCharacterStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UCharacterStateComponent();

	/**
	 * Attempt a state transition. Returns true if the transition was allowed.
	 * This is the primary way to change state.
	 */
	UFUNCTION(BlueprintCallable, Category = "State")
	bool RequestStateChange(ECharacterState NewState);

	/**
	 * Unconditionally set the state. Used for forced transitions like death or stun from damage.
	 * Bypasses transition rules.
	 */
	UFUNCTION(BlueprintCallable, Category = "State")
	void ForceState(ECharacterState NewState);

	/** Get the current character state */
	UFUNCTION(BlueprintPure, Category = "State")
	ECharacterState GetCurrentState() const { return CurrentState; }

	/** Get the previous character state */
	UFUNCTION(BlueprintPure, Category = "State")
	ECharacterState GetPreviousState() const { return PreviousState; }

	/** Returns true if the character can perform a new action (Idle or Moving) */
	UFUNCTION(BlueprintPure, Category = "State")
	bool CanPerformAction() const;

	/** Returns true if player input should be blocked in the current state */
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsInputBlocked() const;

	/** Broadcast when the state changes */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStateChanged OnStateChanged;

protected:

	/** The current character state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	ECharacterState CurrentState = ECharacterState::Idle;

	/** The previous character state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	ECharacterState PreviousState = ECharacterState::Idle;

private:

	/** Transition rules: maps each state to its set of allowed target states */
	TMap<ECharacterState, TSet<ECharacterState>> TransitionRules;

	/** States where player input is consumed but not acted upon */
	TSet<ECharacterState> InputBlockingStates;

	/** Initialize the transition rules table */
	void InitializeTransitionRules();

	/** Set the state and broadcast the change */
	void SetState(ECharacterState NewState);
};
