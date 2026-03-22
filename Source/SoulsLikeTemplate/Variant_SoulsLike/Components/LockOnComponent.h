// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

/**
 *  Handles target lock-on: acquisition, maintenance, camera control, and strafe movement.
 *  Automatically releases lock when target dies or goes out of range.
 */
UCLASS(ClassGroup = (SoulsLike), meta = (BlueprintSpawnableComponent))
class ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	ULockOnComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Toggle lock-on: if locked, unlocks. If unlocked, acquires nearest valid target. */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ToggleLockOn();

	/** Switch to the next target in the given direction. Positive = right, negative = left. */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void SwitchTarget(float Direction);

	/** Get the currently locked target */
	UFUNCTION(BlueprintPure, Category = "LockOn")
	AActor* GetLockedTarget() const { return LockedTarget; }

	/** Whether we currently have a locked target */
	UFUNCTION(BlueprintPure, Category = "LockOn")
	bool IsLockedOn() const { return LockedTarget != nullptr; }

	/**
	 * When locked on, convert 2D input into movement direction relative to the target.
	 * When not locked on, returns FVector::ZeroVector (caller should use standard camera-relative movement).
	 */
	UFUNCTION(BlueprintPure, Category = "LockOn")
	FVector GetMoveDirectionRelativeToTarget(FVector2D InputVector) const;

protected:

	/** Sphere overlap radius for finding lock-on candidates */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = 100, Units = "cm"))
	float SearchRadius = 1500.0f;

	/** Half-angle of the forward cone for target acquisition (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = 10, ClampMax = 180, Units = "deg"))
	float MaxLockAngle = 60.0f;

	/** Distance at which lock automatically breaks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = 100, Units = "cm"))
	float LockBreakDistance = 2000.0f;

	/** Smooth rotation speed toward locked target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn", meta = (ClampMin = 1))
	float CameraInterpSpeed = 8.0f;

private:

	/** The currently locked target actor */
	UPROPERTY()
	TObjectPtr<AActor> LockedTarget;

	/** Find the best lock-on target from available candidates */
	AActor* FindBestTarget() const;

	/** Update the controller rotation to face the locked target */
	void UpdateCameraRotation(float DeltaTime);

	/** Check if the lock should be released (distance, death, visibility) */
	void ValidateLock();

	/** Release the current lock */
	void ReleaseLock();

	/** Show/hide health bar on target enemy */
	void SetTargetHealthBarVisible(AActor* Target, bool bVisible);
};
