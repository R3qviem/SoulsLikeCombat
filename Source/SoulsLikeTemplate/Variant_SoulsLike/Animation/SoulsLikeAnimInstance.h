// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SoulsLikeTypes.h"
#include "SoulsLikeAnimInstance.generated.h"

class ASoulsLikeBaseCharacter;
class UCharacterStateComponent;
class UWeaponManagerComponent;
class UCharacterMovementComponent;

/**
 *  Custom AnimInstance for Souls-Like characters.
 *  Provides data-driven animation state to the Animation Blueprint.
 *  No hardcoded animation references - all montage references come from weapon data assets.
 */
UCLASS()
class USoulsLikeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	/** Initialize cached pointers to owning character and components */
	virtual void NativeInitializeAnimation() override;

	/** Update animation properties every frame from cached component pointers */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ===== ANIMATION STATE (BlueprintReadOnly, for Animation Blueprint consumption) =====

	/** Ground speed of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed = 0.0f;

	/** Whether the character is in the air */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir = false;

	/** Whether the character has active acceleration input */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAccelerating = false;

	/** Current character state */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	ECharacterState CharacterState = ECharacterState::Idle;

	/** Current equipped weapon type (for blend space/state machine selection) */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EWeaponType CurrentWeaponType = EWeaponType::Unarmed;

	/** Whether the character is locked on to a target */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsLockedOn = false;

	/** Movement direction angle relative to actor forward (for strafe blend space) */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float MoveDirectionAngle = 0.0f;

	/** Whether the character is dead */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDead = false;

	/** Whether the character's stance is currently broken */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsStanceBroken = false;

	/** Current poise as percentage (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float PoisePercent = 1.0f;

private:

	/** Cached owning character */
	UPROPERTY()
	TObjectPtr<ASoulsLikeBaseCharacter> OwnerCharacter;

	/** Cached movement component */
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	/** Cached state component */
	UPROPERTY()
	TObjectPtr<UCharacterStateComponent> StateComponent;

	/** Cached weapon manager */
	UPROPERTY()
	TObjectPtr<UWeaponManagerComponent> WeaponManager;
};
