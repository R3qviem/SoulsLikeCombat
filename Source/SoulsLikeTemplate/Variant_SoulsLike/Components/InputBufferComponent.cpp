// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputBufferComponent.h"

UInputBufferComponent::UInputBufferComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInputBufferComponent::BufferInput(EBufferedInput InputType, FVector2D Direction)
{
	BufferedEntry.InputType = InputType;
	BufferedEntry.Direction = Direction;
	BufferedEntry.Timestamp = GetWorld()->GetTimeSeconds();
}

bool UInputBufferComponent::ConsumeBuffer()
{
	if (!HasBufferedInput())
	{
		return false;
	}

	const EBufferedInput ConsumedType = BufferedEntry.InputType;
	const FVector2D ConsumedDirection = BufferedEntry.Direction;

	// Clear the buffer before broadcasting to prevent re-entrancy
	BufferedEntry.InputType = EBufferedInput::None;

	OnBufferedInputConsumed.Broadcast(ConsumedType, ConsumedDirection);
	return true;
}

void UInputBufferComponent::ClearBuffer()
{
	BufferedEntry.InputType = EBufferedInput::None;
}

bool UInputBufferComponent::HasBufferedInput() const
{
	if (BufferedEntry.InputType == EBufferedInput::None)
	{
		return false;
	}

	// Check if the buffered input is still within the valid window
	const float TimeSinceBuffer = GetWorld()->GetTimeSeconds() - BufferedEntry.Timestamp;
	return TimeSinceBuffer <= BufferWindow;
}
