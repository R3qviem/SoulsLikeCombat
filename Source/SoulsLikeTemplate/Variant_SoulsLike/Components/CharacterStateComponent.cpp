// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterStateComponent.h"

UCharacterStateComponent::UCharacterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitializeTransitionRules();
}

void UCharacterStateComponent::InitializeTransitionRules()
{
	// Idle -> can go to anything except Dead directly (death comes through ForceState)
	TransitionRules.Add(ECharacterState::Idle, {
		ECharacterState::Moving,
		ECharacterState::Attacking,
		ECharacterState::Dodging,
		ECharacterState::Blocking,
		ECharacterState::Finisher,
		ECharacterState::Stunned,
		ECharacterState::Dead
	});

	// Moving -> same as Idle
	TransitionRules.Add(ECharacterState::Moving, {
		ECharacterState::Idle,
		ECharacterState::Attacking,
		ECharacterState::Dodging,
		ECharacterState::Blocking,
		ECharacterState::Finisher,
		ECharacterState::Stunned,
		ECharacterState::Dead
	});

	// Attacking -> can combo (Attacking again), get interrupted, or die
	TransitionRules.Add(ECharacterState::Attacking, {
		ECharacterState::Idle,
		ECharacterState::Attacking,
		ECharacterState::Stunned,
		ECharacterState::Dead
	});

	// Dodging -> returns to Idle when done, can be interrupted by stun/death
	TransitionRules.Add(ECharacterState::Dodging, {
		ECharacterState::Idle,
		ECharacterState::Stunned,
		ECharacterState::Dead
	});

	// Blocking -> can return to Idle/Moving, parry, perform finisher, get stunned, or die
	TransitionRules.Add(ECharacterState::Blocking, {
		ECharacterState::Idle,
		ECharacterState::Moving,
		ECharacterState::Parrying,
		ECharacterState::Finisher,
		ECharacterState::Stunned,
		ECharacterState::Dead
	});

	// Parrying -> returns to Idle, can finisher (riposte), get stunned, or die
	TransitionRules.Add(ECharacterState::Parrying, {
		ECharacterState::Idle,
		ECharacterState::Finisher,
		ECharacterState::Stunned,
		ECharacterState::Dead
	});

	// Finisher -> returns to Idle when done, or dies
	TransitionRules.Add(ECharacterState::Finisher, {
		ECharacterState::Idle,
		ECharacterState::Dead
	});

	// Stunned -> recovers to Idle, or dies
	TransitionRules.Add(ECharacterState::Stunned, {
		ECharacterState::Idle,
		ECharacterState::Dead
	});

	// Dead -> terminal state, no transitions out
	TransitionRules.Add(ECharacterState::Dead, {});

	// States where input is blocked
	InputBlockingStates = {
		ECharacterState::Attacking,
		ECharacterState::Dodging,
		ECharacterState::Parrying,
		ECharacterState::Stunned,
		ECharacterState::Finisher,
		ECharacterState::Dead
	};
}

bool UCharacterStateComponent::RequestStateChange(ECharacterState NewState)
{
	if (CurrentState == NewState)
	{
		return true;
	}

	const TSet<ECharacterState>* AllowedStates = TransitionRules.Find(CurrentState);
	if (AllowedStates && AllowedStates->Contains(NewState))
	{
		SetState(NewState);
		return true;
	}

	return false;
}

void UCharacterStateComponent::ForceState(ECharacterState NewState)
{
	if (CurrentState != NewState)
	{
		SetState(NewState);
	}
}

bool UCharacterStateComponent::CanPerformAction() const
{
	return CurrentState == ECharacterState::Idle || CurrentState == ECharacterState::Moving;
}

bool UCharacterStateComponent::IsInputBlocked() const
{
	return InputBlockingStates.Contains(CurrentState);
}

void UCharacterStateComponent::SetState(ECharacterState NewState)
{
	PreviousState = CurrentState;
	CurrentState = NewState;
	OnStateChanged.Broadcast(NewState);
}
