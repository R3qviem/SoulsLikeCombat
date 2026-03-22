// Copyright Epic Games, Inc. All Rights Reserved.

#include "LockOnComponent.h"
#include "SoulsLikeDamageable.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (LockedTarget)
	{
		ValidateLock();

		if (LockedTarget)
		{
			UpdateCameraRotation(DeltaTime);
		}
	}
}

void ULockOnComponent::ToggleLockOn()
{
	if (LockedTarget)
	{
		ReleaseLock();
	}
	else
	{
		AActor* BestTarget = FindBestTarget();
		if (BestTarget)
		{
			LockedTarget = BestTarget;
		}
	}
}

void ULockOnComponent::SwitchTarget(float Direction)
{
	if (!LockedTarget)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
	if (!OwnerCharacter || !OwnerCharacter->GetController())
	{
		return;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FRotator ControlRotation = OwnerCharacter->GetController()->GetControlRotation();
	const FVector CameraRight = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);

	// Find all valid targets
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(SearchRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	GetWorld()->OverlapMultiByObjectType(OverlapResults, OwnerLocation, FQuat::Identity, ObjectParams, CollisionShape, QueryParams);

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!Candidate || Candidate == LockedTarget)
		{
			continue;
		}

		ISoulsLikeDamageable* Damageable = Cast<ISoulsLikeDamageable>(Candidate);
		if (!Damageable || Damageable->IsDead())
		{
			continue;
		}

		// Check visibility
		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(Owner);
		TraceParams.AddIgnoredActor(Candidate);

		bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, OwnerLocation, Candidate->GetActorLocation(), ECC_Visibility, TraceParams);
		if (bBlocked)
		{
			continue;
		}

		// Score based on direction
		const FVector ToCandidate = (Candidate->GetActorLocation() - OwnerLocation).GetSafeNormal();
		float DirectionalScore = FVector::DotProduct(ToCandidate, CameraRight) * Direction;
		float Distance = FVector::Dist(OwnerLocation, Candidate->GetActorLocation());

		// Prefer targets in the switching direction, closer ones preferred
		float Score = DirectionalScore * 1000.0f - Distance;

		if (DirectionalScore > 0 && Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	if (BestTarget)
	{
		LockedTarget = BestTarget;
	}
}

FVector ULockOnComponent::GetMoveDirectionRelativeToTarget(FVector2D InputVector) const
{
	if (!LockedTarget)
	{
		return FVector::ZeroVector;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	// Calculate direction from owner to target
	const FVector ToTarget = (LockedTarget->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
	const FVector RightOfTarget = FVector::CrossProduct(FVector::UpVector, ToTarget);

	// Forward input moves toward/away from target, right input strafes
	return (ToTarget * InputVector.Y) + (RightOfTarget * InputVector.X);
}

AActor* ULockOnComponent::FindBestTarget() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
	if (!OwnerCharacter || !OwnerCharacter->GetController())
	{
		return nullptr;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FRotator ControlRotation = OwnerCharacter->GetController()->GetControlRotation();
	// Use horizontal-only forward (ignore pitch) so top-down camera doesn't break targeting
	const FVector CameraForward = FRotationMatrix(FRotator(0.0f, ControlRotation.Yaw, 0.0f)).GetUnitAxis(EAxis::X);

	// Sphere overlap for candidates
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(SearchRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	GetWorld()->OverlapMultiByObjectType(OverlapResults, OwnerLocation, FQuat::Identity, ObjectParams, CollisionShape, QueryParams);

	AActor* BestTarget = nullptr;
	float BestDistance = FLT_MAX;
	const float CosMaxAngle = FMath::Cos(FMath::DegreesToRadians(MaxLockAngle));

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!Candidate)
		{
			continue;
		}

		// Must implement damageable interface and be alive
		ISoulsLikeDamageable* Damageable = Cast<ISoulsLikeDamageable>(Candidate);
		if (!Damageable || Damageable->IsDead())
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - OwnerLocation;
		const float Distance = ToCandidate.Size();

		// Angle check against horizontal camera forward
		const FVector DirectionToCandidate = ToCandidate.GetSafeNormal2D();
		const float DotProduct = FVector::DotProduct(CameraForward, DirectionToCandidate);
		if (DotProduct < CosMaxAngle)
		{
			continue;
		}

		// Visibility check (line trace)
		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(Owner);
		TraceParams.AddIgnoredActor(Candidate);

		bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, OwnerLocation, Candidate->GetActorLocation(), ECC_Visibility, TraceParams);
		if (bBlocked)
		{
			continue;
		}

		// Pick the closest valid target
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void ULockOnComponent::UpdateCameraRotation(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || !LockedTarget)
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
	if (!OwnerCharacter || !OwnerCharacter->GetController())
	{
		return;
	}

	// Calculate desired yaw toward the target (on the horizontal plane only)
	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector TargetLocation = LockedTarget->GetActorLocation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(OwnerLocation, TargetLocation);

	// Only interpolate YAW — keep the current pitch and roll unchanged (top-down pitch stays fixed)
	const FRotator CurrentRotation = OwnerCharacter->GetController()->GetControlRotation();
	const FRotator DesiredRotation(CurrentRotation.Pitch, LookAtRotation.Yaw, CurrentRotation.Roll);
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, CameraInterpSpeed);

	OwnerCharacter->GetController()->SetControlRotation(NewRotation);
}

void ULockOnComponent::ValidateLock()
{
	if (!LockedTarget)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		ReleaseLock();
		return;
	}

	// Check if target is dead
	ISoulsLikeDamageable* Damageable = Cast<ISoulsLikeDamageable>(LockedTarget);
	if (Damageable && Damageable->IsDead())
	{
		ReleaseLock();
		return;
	}

	// Check distance
	const float Distance = FVector::Dist(Owner->GetActorLocation(), LockedTarget->GetActorLocation());
	if (Distance > LockBreakDistance)
	{
		ReleaseLock();
		return;
	}
}

void ULockOnComponent::ReleaseLock()
{
	LockedTarget = nullptr;
}
