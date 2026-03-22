// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulsLikeTypes.h"
#include "InputBufferComponent.generated.h"

/**
 *  Buffered input types that can be queued during locked states.
 */
UENUM(BlueprintType)
enum class EBufferedInput : uint8
{
	None,
	LightAttack,
	HeavyAttack,
	Dodge,
	Block,
	UseItem
};

/**
 *  A single buffered input entry with timestamp and optional direction.
 */
USTRUCT()
struct FBufferedInputEntry
{
	GENERATED_BODY()

	EBufferedInput InputType = EBufferedInput::None;
	FVector2D Direction = FVector2D::ZeroVector;
	float Timestamp = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBufferedInputConsumed, EBufferedInput, InputType, FVector2D, Direction);

/**
 *  Queues player inputs during locked states (attacking, dodging, stunned)
 *  and replays them when the character returns to an actionable state.
 *  This creates the responsive "queue your next action" feel of Dark Souls.
 */
UCLASS(ClassGroup = (SoulsLike), meta = (BlueprintSpawnableComponent))
class UInputBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInputBufferComponent();

	/**
	 * Buffer an input. If the character can act now, the delegate fires immediately.
	 * If not, it's stored and consumed when ConsumeBuffer is called.
	 */
	void BufferInput(EBufferedInput InputType, FVector2D Direction = FVector2D::ZeroVector);

	/**
	 * Try to consume the buffered input. Called when the character returns to an actionable state.
	 * Returns true if a buffered input was consumed.
	 */
	bool ConsumeBuffer();

	/** Clear the buffer (e.g., on death or stun break) */
	void ClearBuffer();

	/** Check if there's a valid buffered input waiting */
	bool HasBufferedInput() const;

	/** Broadcast when a buffered input is consumed and should be executed */
	UPROPERTY(BlueprintAssignable, Category = "Input Buffer")
	FOnBufferedInputConsumed OnBufferedInputConsumed;

	/** How long a buffered input stays valid (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Buffer", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float BufferWindow = 0.3f;

private:

	FBufferedInputEntry BufferedEntry;
};
