// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoulsLikeBaseCharacter.h"
#include "SoulsLikePlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ULockOnComponent;
class UInputBufferComponent;
class UInputAction;
struct FInputActionValue;
enum class EBufferedInput : uint8;

/**
 *  Player-specific Souls-Like character.
 *  Adds camera, input handling, and lock-on integration.
 *  All input goes through state machine checks before executing.
 */
UCLASS()
class ASoulsLikePlayerCharacter : public ASoulsLikeBaseCharacter
{
	GENERATED_BODY()

public:

	/** Constructor */
	ASoulsLikePlayerCharacter();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	// ===== CAMERA =====

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Lock-on targeting component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULockOnComponent> LockOnComponent;

	/** Input buffer for queuing actions during locked states */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInputBufferComponent> InputBuffer;

	// ===== INPUT ACTIONS =====

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_MouseLook;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_LightAttack;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_HeavyAttack;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Dodge;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Block;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_LockOn;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_SwitchTarget;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Zoom;

	// ===== CAMERA SETTINGS =====

	/** Minimum zoom distance (closest) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 100))
	float MinZoomDistance = 300.0f;

	/** Maximum zoom distance (farthest) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 200))
	float MaxZoomDistance = 1500.0f;

	/** How much each scroll tick changes the zoom */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 10))
	float ZoomStep = 100.0f;

	/** Zoom interpolation speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 1))
	float ZoomInterpSpeed = 8.0f;

	/** Returns CameraBoom subobject */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:

	// ===== INPUT HANDLERS =====

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleLightAttack();
	void HandleHeavyAttackStart();
	void HandleHeavyAttackRelease();
	void HandleDodge();
	void HandleBlockStart();
	void HandleBlockEnd();
	void HandleLockOn();
	void HandleSwitchTarget(const FInputActionValue& Value);
	void HandleZoom(const FInputActionValue& Value);

	/** Called when a buffered input is consumed and should be executed */
	UFUNCTION()
	void OnBufferedInputConsumed(EBufferedInput InputType, FVector2D Direction);

	/** Called when character state changes — consumes buffer on return to actionable state */
	UFUNCTION()
	void OnStateChangedHandler(ECharacterState NewState);

	/** Override to consume input buffer when action ends */
	virtual void OnActionEnd() override;

	/** Cached last move input for dodge direction */
	FVector2D LastMoveInput = FVector2D::ZeroVector;

	/** Target zoom distance (smoothly interpolated) */
	float DesiredZoomDistance = 800.0f;

	/** World time when heavy charge started */
	float HeavyChargeStartTime = 0.0f;

	/** Maximum charge time before auto-firing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Charge", meta = (ClampMin = 0.5))
	float MaxChargeTime = 2.0f;

private:

	/** Update movement orientation based on lock-on state */
	void UpdateMovementOrientation();
};
