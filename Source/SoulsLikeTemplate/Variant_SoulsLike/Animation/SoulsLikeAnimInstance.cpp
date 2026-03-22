// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeAnimInstance.h"
#include "SoulsLikeBaseCharacter.h"
#include "CharacterStateComponent.h"
#include "WeaponManagerComponent.h"
#include "PoiseComponent.h"
#include "WeaponDataAsset.h"
#include "LockOnComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void USoulsLikeAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ASoulsLikeBaseCharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
		StateComponent = OwnerCharacter->StateComponent;
		WeaponManager = OwnerCharacter->WeaponManager;
	}
}

void USoulsLikeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		return;
	}

	// Movement data
	if (MovementComponent)
	{
		Speed = MovementComponent->Velocity.Size2D();
		bIsInAir = MovementComponent->IsFalling();
		bIsAccelerating = MovementComponent->GetCurrentAcceleration().Size2D() > 0.0f;
	}

	// State data
	if (StateComponent)
	{
		CharacterState = StateComponent->GetCurrentState();
		bIsDead = CharacterState == ECharacterState::Dead;
	}

	// Weapon data
	if (WeaponManager)
	{
		const UWeaponDataAsset* WeaponData = WeaponManager->GetCurrentWeaponData();
		CurrentWeaponType = WeaponData ? WeaponData->WeaponType : EWeaponType::Unarmed;
	}

	// Lock-on data (check if the character has a LockOnComponent)
	ULockOnComponent* LockOnComp = OwnerCharacter->FindComponentByClass<ULockOnComponent>();
	if (LockOnComp)
	{
		bIsLockedOn = LockOnComp->IsLockedOn();
	}
	else
	{
		bIsLockedOn = false;
	}

	// Poise data
	if (OwnerCharacter->PoiseComponent)
	{
		bIsStanceBroken = OwnerCharacter->PoiseComponent->IsStanceBroken();
		PoisePercent = OwnerCharacter->PoiseComponent->GetPoisePercent();
	}

	// Calculate movement direction angle for strafe blend space
	if (Speed > 1.0f)
	{
		const FVector Velocity = MovementComponent ? MovementComponent->Velocity : FVector::ZeroVector;
		const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
		const FRotator MovementRotation = Velocity.Rotation();
		const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, ActorRotation);
		MoveDirectionAngle = DeltaRotation.Yaw;
	}
	else
	{
		MoveDirectionAngle = 0.0f;
	}
}
