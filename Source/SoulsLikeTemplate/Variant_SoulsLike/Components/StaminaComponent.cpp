// Copyright Epic Games, Inc. All Rights Reserved.

#include "StaminaComponent.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentStamina = MaxStamina;
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentStamina >= MaxStamina)
	{
		bIsRegenerating = false;
		return;
	}

	// Track time since last use
	TimeSinceLastUse += DeltaTime;

	// Start regenerating after the delay has elapsed
	if (TimeSinceLastUse >= RegenDelay)
	{
		bIsRegenerating = true;
		CurrentStamina = FMath::Min(CurrentStamina + (RegenRate * DeltaTime), MaxStamina);
		OnStaminaChanged.Broadcast(GetStaminaPercent());
	}
}

bool UStaminaComponent::ConsumeStamina(float Amount)
{
	if (CurrentStamina < Amount)
	{
		return false;
	}

	CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);
	TimeSinceLastUse = 0.0f;
	bIsRegenerating = false;
	OnStaminaChanged.Broadcast(GetStaminaPercent());

	return true;
}

bool UStaminaComponent::HasStamina(float Amount) const
{
	return CurrentStamina >= Amount;
}

float UStaminaComponent::GetStaminaPercent() const
{
	return MaxStamina > 0.0f ? CurrentStamina / MaxStamina : 0.0f;
}

void UStaminaComponent::ResetStamina()
{
	CurrentStamina = MaxStamina;
	TimeSinceLastUse = 0.0f;
	bIsRegenerating = false;
	OnStaminaChanged.Broadcast(1.0f);
}
