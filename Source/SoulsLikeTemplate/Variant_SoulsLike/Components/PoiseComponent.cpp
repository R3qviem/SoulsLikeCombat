// Copyright Epic Games, Inc. All Rights Reserved.

#include "PoiseComponent.h"
#include "TimerManager.h"

UPoiseComponent::UPoiseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentPoise = MaxPoise;
}

void UPoiseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bStanceBroken)
	{
		return;
	}

	// Regenerate poise after delay
	if (CurrentPoise < MaxPoise)
	{
		TimeSinceLastPoiseDamage += DeltaTime;

		if (TimeSinceLastPoiseDamage >= PoiseRegenDelay)
		{
			CurrentPoise = FMath::Min(MaxPoise, CurrentPoise + PoiseRegenRate * DeltaTime);
			OnPoiseChanged.Broadcast(GetPoisePercent());
		}
	}
}

bool UPoiseComponent::ApplyPoiseDamage(float PoiseDamage)
{
	if (bStanceBroken || PoiseDamage <= 0.0f)
	{
		return false;
	}

	TimeSinceLastPoiseDamage = 0.0f;
	CurrentPoise = FMath::Max(0.0f, CurrentPoise - PoiseDamage);
	OnPoiseChanged.Broadcast(GetPoisePercent());

	if (CurrentPoise <= 0.0f)
	{
		bStanceBroken = true;
		OnStanceBroken.Broadcast();

		// Auto-recover after the stance broken duration
		GetWorld()->GetTimerManager().SetTimer(StanceBrokenTimerHandle,
			FTimerDelegate::CreateUObject(this, &UPoiseComponent::RecoverFromStanceBreak),
			StanceBrokenDuration, false);

		return true;
	}

	return false;
}

void UPoiseComponent::ResetPoise()
{
	CurrentPoise = MaxPoise;
	bStanceBroken = false;
	TimeSinceLastPoiseDamage = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(StanceBrokenTimerHandle);
	OnPoiseChanged.Broadcast(1.0f);
}

void UPoiseComponent::RecoverFromStanceBreak()
{
	bStanceBroken = false;
	CurrentPoise = MaxPoise;
	TimeSinceLastPoiseDamage = 0.0f;
	OnPoiseChanged.Broadcast(1.0f);
}
