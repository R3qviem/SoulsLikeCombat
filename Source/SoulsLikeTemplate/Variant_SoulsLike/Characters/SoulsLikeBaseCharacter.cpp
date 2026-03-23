// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeBaseCharacter.h"
#include "CharacterStateComponent.h"
#include "StaminaComponent.h"
#include "WeaponManagerComponent.h"
#include "PoiseComponent.h"
#include "WeaponDataAsset.h"
#include "SoulsLikeDamageable.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogSoulsLikeCharacter);

ASoulsLikeBaseCharacter::ASoulsLikeBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	StateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("StateComponent"));
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	WeaponManager = CreateDefaultSubobject<UWeaponManagerComponent>(TEXT("WeaponManager"));
	PoiseComponent = CreateDefaultSubobject<UPoiseComponent>(TEXT("PoiseComponent"));

	// Bind the attack montage ended delegate
	OnAttackMontageEndedDelegate.BindUObject(this, &ASoulsLikeBaseCharacter::AttackMontageEnded);

	// Configure character movement defaults
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	// Don't rotate when the controller rotates
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Load death animation sequence (will be played directly on mesh at runtime)
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DeathSeqFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	if (DeathSeqFinder.Succeeded())
	{
		DeathAnimSequence = DeathSeqFinder.Object;
	}

	// Load hit reaction animation sequences (will be played as dynamic montages at runtime)
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> LightHitReactFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01"));
	if (LightHitReactFinder.Succeeded())
	{
		LightHitReactionSequence = LightHitReactFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HeavyHitReactFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01"));
	if (HeavyHitReactFinder.Succeeded())
	{
		HeavyHitReactionSequence = HeavyHitReactFinder.Object;
	}
}

void ASoulsLikeBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
}

void ASoulsLikeBaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// If we were dodging and landed, return to Idle
	if (StateComponent->GetCurrentState() == ECharacterState::Dodging)
	{
		// Cancel any pending dodge recovery timer since we landed
		GetWorld()->GetTimerManager().ClearTimer(DodgeRecoveryTimerHandle);

		// Re-enable orient-to-movement (disabled during dodge)
		GetCharacterMovement()->bOrientRotationToMovement = true;

		// Let the dodge montage finish if it's still playing, otherwise go to Idle immediately
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (!AnimInstance || !AnimInstance->IsAnyMontagePlaying())
		{
			StateComponent->ForceState(ECharacterState::Idle);
		}
	}
}

// ===== COMBAT ACTIONS =====

void ASoulsLikeBaseCharacter::AttemptAttack(EAttackType AttackType)
{
	// If currently attacking and combo window is open, queue the next combo
	if (StateComponent->GetCurrentState() == ECharacterState::Attacking)
	{
		if (bComboWindowOpen)
		{
			bComboQueued = true;
			CurrentAttackType = AttackType;
		}
		return;
	}

	// Can only start a new attack from Idle or Moving
	if (!StateComponent->CanPerformAction())
	{
		return;
	}

	// Check if we have a weapon with combo data
	const FComboAttack* AttackData = WeaponManager->GetComboAttack(0, AttackType);
	if (!AttackData)
	{
		return;
	}

	// Check and consume stamina
	if (!StaminaComponent->ConsumeStamina(AttackData->StaminaCost))
	{
		return;
	}

	// Transition to attacking state
	if (!StateComponent->RequestStateChange(ECharacterState::Attacking))
	{
		return;
	}

	// Start the combo chain
	CurrentComboIndex = 0;
	CurrentAttackType = AttackType;
	bComboQueued = false;
	bComboWindowOpen = false;
	HitActorsThisSwing.Empty();

	PlayAttackMontageSection(AttackType, 0);
}

void ASoulsLikeBaseCharacter::AttemptDodge(FVector2D InputDirection)
{
	if (!StateComponent->CanPerformAction())
	{
		return;
	}

	// Get dodge stamina cost from weapon data (or use default)
	float DodgeCost = 20.0f;
	const UWeaponDataAsset* WeaponData = WeaponManager->GetCurrentWeaponData();
	if (WeaponData)
	{
		DodgeCost = WeaponData->DodgeStaminaCost;
	}

	if (!StaminaComponent->ConsumeStamina(DodgeCost))
	{
		return;
	}

	if (!StateComponent->RequestStateChange(ECharacterState::Dodging))
	{
		return;
	}

	// Determine dodge direction relative to character facing to pick the right montage
	// Convert input to local space: X = forward/back, Y = left/right
	FVector2D LocalDir;
	if (InputDirection.IsNearlyZero())
	{
		// No input: dodge backward
		LocalDir = FVector2D(0.0f, -1.0f);
	}
	else
	{
		// Convert camera-relative input to world direction
		FVector WorldDodgeDir;
		if (GetController())
		{
			const FRotator YawRotation(0, GetController()->GetControlRotation().Yaw, 0);
			const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			WorldDodgeDir = (ForwardDir * InputDirection.Y + RightDir * InputDirection.X).GetSafeNormal();
		}
		else
		{
			WorldDodgeDir = GetActorForwardVector();
		}

		// Project world direction onto character's local axes
		const FVector ActorForward = GetActorForwardVector().GetSafeNormal2D();
		const FVector ActorRight = GetActorRightVector().GetSafeNormal2D();
		LocalDir.X = FVector::DotProduct(WorldDodgeDir, ActorRight);   // positive = right
		LocalDir.Y = FVector::DotProduct(WorldDodgeDir, ActorForward); // positive = forward
	}

	// Pick directional montage based on dominant axis
	UAnimMontage* DodgeMontageToPlay = nullptr;
	if (FMath::Abs(LocalDir.Y) >= FMath::Abs(LocalDir.X))
	{
		// Forward or backward dominant
		DodgeMontageToPlay = (LocalDir.Y >= 0.0f) ? DodgeForwardMontage : DodgeBackMontage;
	}
	else
	{
		// Left or right dominant
		DodgeMontageToPlay = (LocalDir.X >= 0.0f) ? DodgeRightMontage : DodgeLeftMontage;
	}

	// Temporarily disable rotation overrides during dodge
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = false;

	// Play the directional dodge montage — root motion drives the movement
	bool bMontagePlayed = false;
	if (DodgeMontageToPlay)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			const float MontageLength = AnimInstance->Montage_Play(DodgeMontageToPlay, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
			if (MontageLength > 0.0f)
			{
				AnimInstance->Montage_SetEndDelegate(OnAttackMontageEndedDelegate, DodgeMontageToPlay);
				bMontagePlayed = true;
			}
		}
	}

	// Fallback: if no montage played, use impulse
	if (!bMontagePlayed)
	{
		FVector DodgeDir = GetActorForwardVector() * LocalDir.Y + GetActorRightVector() * LocalDir.X;
		DodgeDir.Normalize();
		LaunchCharacter(DodgeDir * DodgeImpulse, true, false);

		GetWorld()->GetTimerManager().SetTimer(DodgeRecoveryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
			if (StateComponent->GetCurrentState() == ECharacterState::Dodging)
			{
				StateComponent->ForceState(ECharacterState::Idle);
			}
		}), 0.4f, false);
	}
}

bool ASoulsLikeBaseCharacter::AttemptFinisher()
{
	if (!StateComponent->CanPerformAction())
	{
		return false;
	}

	// Sphere overlap to find nearby characters
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionShape SphereShape;
	SphereShape.SetSphere(FinisherDetectionRange);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	if (!GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, SphereShape, QueryParams))
	{
		return false;
	}

	ASoulsLikeBaseCharacter* BestTarget = nullptr;
	bool bIsBackstab = false;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ASoulsLikeBaseCharacter* Target = Cast<ASoulsLikeBaseCharacter>(Overlap.GetActor());
		if (!Target || Target->IsDead())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());

		// Check for riposte (stance broken enemy in front)
		if (Target->PoiseComponent->IsStanceBroken())
		{
			if (DistSq < BestDistSq)
			{
				BestTarget = Target;
				bIsBackstab = false;
				BestDistSq = DistSq;
			}
			continue;
		}

		// Check for backstab (behind enemy, enemy not in combat-aware state)
		if (Target->StateComponent->GetCurrentState() != ECharacterState::Attacking &&
			Target->StateComponent->GetCurrentState() != ECharacterState::Dodging &&
			Target->StateComponent->GetCurrentState() != ECharacterState::Blocking)
		{
			const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			const FVector TargetForward = Target->GetActorForwardVector();

			// We need to be behind the target: our direction to target should align with target's forward
			const float DotProduct = FVector::DotProduct(ToTarget, TargetForward);
			if (DotProduct > FMath::Cos(FMath::DegreesToRadians(BackstabAngleThreshold)))
			{
				if (DistSq < BestDistSq)
				{
					BestTarget = Target;
					bIsBackstab = true;
					BestDistSq = DistSq;
				}
			}
		}
	}

	if (!BestTarget)
	{
		return false;
	}

	// Transition to finisher state
	if (!StateComponent->RequestStateChange(ECharacterState::Finisher))
	{
		return false;
	}

	// Select montages
	UAnimMontage* AttackerMontage = bIsBackstab ? BackstabMontage : RiposteMontage;
	UAnimMontage* VictimMontage = bIsBackstab ? BestTarget->BackstabVictimMontage : BestTarget->RiposteVictimMontage;

	// Snap attacker to proper position relative to victim
	FVector FinisherPosition;
	if (bIsBackstab)
	{
		// Position behind the target
		FinisherPosition = BestTarget->GetActorLocation() - BestTarget->GetActorForwardVector() * 100.0f;
	}
	else
	{
		// Position in front of the target
		FinisherPosition = BestTarget->GetActorLocation() + BestTarget->GetActorForwardVector() * 100.0f;
	}
	FinisherPosition.Z = GetActorLocation().Z; // Keep same Z
	SetActorLocation(FinisherPosition); // Exception: finisher snap requires direct positioning

	// Face the target
	const FVector ToTarget = (BestTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	SetActorRotation(ToTarget.Rotation());

	// Force victim into stunned state and stop their montages
	BestTarget->StateComponent->ForceState(ECharacterState::Stunned);
	BestTarget->PoiseComponent->RecoverFromStanceBreak(); // End the stance break timer

	// Play montages
	UAnimInstance* AttackerAnim = GetMesh()->GetAnimInstance();
	UAnimInstance* VictimAnim = BestTarget->GetMesh()->GetAnimInstance();

	if (VictimAnim)
	{
		VictimAnim->StopAllMontages(0.1f);
	}

	if (AttackerMontage && AttackerAnim)
	{
		const float MontageLength = AttackerAnim->Montage_Play(AttackerMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		if (MontageLength > 0.0f)
		{
			AttackerAnim->Montage_SetEndDelegate(OnAttackMontageEndedDelegate, AttackerMontage);
		}
	}

	if (VictimMontage && VictimAnim)
	{
		VictimAnim->Montage_Play(VictimMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
	}

	// Apply finisher damage
	const UWeaponDataAsset* WeaponData = WeaponManager->GetCurrentWeaponData();
	const float FinisherDamage = (WeaponData ? WeaponData->BaseDamage : 10.0f) * FinisherDamageMultiplier;

	FDamageInfo FinisherDamageInfo;
	FinisherDamageInfo.DamageAmount = FinisherDamage;
	FinisherDamageInfo.DamageType = ESoulsLikeDamageType::Physical;
	FinisherDamageInfo.HitReaction = EHitReactionType::None; // Victim already plays finisher montage
	FinisherDamageInfo.PoiseDamage = 0.0f; // Skip poise on finisher
	FinisherDamageInfo.SourceActor = this;
	FinisherDamageInfo.HitLocation = BestTarget->GetActorLocation();

	// Apply damage directly (bypass normal ProcessDamage to avoid double stagger)
	BestTarget->CurrentHealth = FMath::Max(0.0f, BestTarget->CurrentHealth - FinisherDamageInfo.DamageAmount);
	BestTarget->OnHealthChanged.Broadcast(BestTarget->GetHealthPercent());

	if (BestTarget->CurrentHealth <= 0.0f)
	{
		BestTarget->HandleDeath();
	}

	OnDealtDamage(FinisherDamage, BestTarget->GetActorLocation());

	// If no montage played, recover via timer
	if (!AttackerMontage || !AttackerAnim)
	{
		GetWorld()->GetTimerManager().SetTimer(DodgeRecoveryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (StateComponent->GetCurrentState() == ECharacterState::Finisher)
			{
				StateComponent->ForceState(ECharacterState::Idle);
			}
		}), 1.0f, false);
	}

	return true;
}

void ASoulsLikeBaseCharacter::AttemptBlock()
{
	if (!StateComponent->CanPerformAction())
	{
		return;
	}

	if (StateComponent->RequestStateChange(ECharacterState::Blocking))
	{
		BlockStartTime = GetWorld()->GetTimeSeconds();
	}
}

void ASoulsLikeBaseCharacter::EndBlock()
{
	if (StateComponent->GetCurrentState() == ECharacterState::Blocking)
	{
		StateComponent->RequestStateChange(ECharacterState::Idle);
	}
}

void ASoulsLikeBaseCharacter::ResetHealth()
{
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(1.0f);
	PoiseComponent->ResetPoise();
}

// ===== ISoulsLikeDamageable =====

void ASoulsLikeBaseCharacter::ReceiveDamage(const FDamageInfo& DamageInfo)
{
	ProcessDamage(DamageInfo);
}

bool ASoulsLikeBaseCharacter::IsDead() const
{
	return StateComponent->GetCurrentState() == ECharacterState::Dead;
}

float ASoulsLikeBaseCharacter::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

// ===== ISoulsLikeAttacker =====

void ASoulsLikeBaseCharacter::ExecuteAttackTrace(FName SourceBone)
{
	const FComboAttack* AttackData = WeaponManager->GetComboAttack(CurrentComboIndex, CurrentAttackType);
	if (!AttackData)
	{
		return;
	}

	const UWeaponDataAsset* WeaponData = WeaponManager->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return;
	}

	// Sphere sweep parameters from the current combo attack
	const FVector TraceStart = GetMesh()->GetSocketLocation(SourceBone);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * AttackData->TraceDistance);

	// Debug draw: show attack trace line only
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 1.0f, 0, 2.0f);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(AttackData->TraceRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> OutHits;
	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor)
			{
				continue;
			}

			// Prevent double hits in the same swing
			if (HitActorsThisSwing.Contains(HitActor))
			{
				continue;
			}

			ISoulsLikeDamageable* Damageable = Cast<ISoulsLikeDamageable>(HitActor);
			if (!Damageable)
			{
				continue;
			}

			HitActorsThisSwing.Add(HitActor);

			// Build the damage info
			FDamageInfo DamageInfoToSend;
			DamageInfoToSend.DamageAmount = WeaponData->BaseDamage * AttackData->DamageMultiplier;
			DamageInfoToSend.DamageType = ESoulsLikeDamageType::Physical;
			DamageInfoToSend.HitReaction = AttackData->HitReaction;
			DamageInfoToSend.PoiseDamage = AttackData->PoiseDamage;
			DamageInfoToSend.SourceActor = this;
			DamageInfoToSend.HitLocation = Hit.ImpactPoint;

			// Knockback away from attacker and slightly upward
			const FVector KnockbackDir = (Hit.ImpactNormal * -1.0f).GetSafeNormal();
			DamageInfoToSend.KnockbackImpulse = KnockbackDir * AttackData->KnockbackImpulse + FVector::UpVector * (AttackData->KnockbackImpulse * 0.3f);

			Damageable->ReceiveDamage(DamageInfoToSend);

			// Blueprint hook
			OnDealtDamage(DamageInfoToSend.DamageAmount, Hit.ImpactPoint);
		}
	}
}

void ASoulsLikeBaseCharacter::OnComboWindowOpen()
{
	bComboWindowOpen = true;

	// If attack was already queued, advance immediately
	if (bComboQueued)
	{
		AdvanceCombo();
	}
}

void ASoulsLikeBaseCharacter::OnComboWindowClose()
{
	bComboWindowOpen = false;
}

void ASoulsLikeBaseCharacter::OnActionEnd()
{
	// Reset combat state
	HitActorsThisSwing.Empty();
	bComboQueued = false;
	bComboWindowOpen = false;
	CurrentComboIndex = 0;

	// Return to idle
	if (StateComponent->GetCurrentState() != ECharacterState::Dead)
	{
		StateComponent->ForceState(ECharacterState::Idle);
	}
}

// ===== ICombatAttacker BRIDGE =====

void ASoulsLikeBaseCharacter::DoAttackTrace(FName DamageSourceBone)
{
	// Bridge: existing Combat montage notifies call this — forward to our system
	ExecuteAttackTrace(DamageSourceBone);
}

void ASoulsLikeBaseCharacter::CheckCombo()
{
	// Bridge: existing Combat montage combo check — forward to our combo window
	OnComboWindowOpen();
}

void ASoulsLikeBaseCharacter::CheckChargedAttack()
{
	// Mark that we've looped at least once
	bHasLoopedChargedAttack = true;

	// Jump to loop or attack section based on whether we're still charging
	UAnimMontage* HeavyMontage = WeaponManager->GetAttackMontage(EAttackType::Heavy);
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(bIsChargingHeavy ? ChargeLoopSection : ChargeAttackSection, HeavyMontage);
	}
}

// ===== DAMAGE PROCESSING =====

void ASoulsLikeBaseCharacter::ProcessDamage(FDamageInfo DamageInfo)
{
	if (IsDead())
	{
		return;
	}

	const UWeaponDataAsset* WeaponData = WeaponManager->GetCurrentWeaponData();

	// Check if blocking
	if (StateComponent->GetCurrentState() == ECharacterState::Blocking)
	{
		const float TimeSinceBlockStart = GetWorld()->GetTimeSeconds() - BlockStartTime;
		const float ParryWindow = WeaponData ? WeaponData->ParryWindowDuration : 0.2f;

		// Check for parry
		if (TimeSinceBlockStart <= ParryWindow)
		{
			DamageInfo.bWasParried = true;
			HandleParry(DamageInfo.SourceActor);

			// Blueprint hook
			OnReceivedDamage(0.0f, DamageInfo.HitLocation, DamageInfo.KnockbackImpulse.GetSafeNormal());
			OnDamageReceived.Broadcast(DamageInfo);
			return;
		}

		// Regular block
		DamageInfo.bWasBlocked = true;
		const float BlockReduction = WeaponData ? WeaponData->BlockDamageReduction : 0.5f;
		DamageInfo.DamageAmount *= (1.0f - BlockReduction);

		// Consume stamina for blocking
		const float BlockStaminaCost = WeaponData ? WeaponData->BlockStaminaCostPerHit : 15.0f;
		if (!StaminaComponent->ConsumeStamina(BlockStaminaCost))
		{
			// Stamina broken - exit block and take full stagger
			StateComponent->ForceState(ECharacterState::Stunned);
			PlayHitReaction(EHitReactionType::Heavy);
		}
		else
		{
			// Play block hit reaction montage
			if (WeaponData && WeaponData->BlockHitReactionMontage)
			{
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_Play(WeaponData->BlockHitReactionMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
				}
			}
		}
	}

	// Apply damage
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageInfo.DamageAmount);
	OnHealthChanged.Broadcast(GetHealthPercent());

	if (CurrentHealth <= 0.0f)
	{
		// Death
		HandleDeath();
	}
	else if (!DamageInfo.bWasBlocked)
	{
		// Apply poise damage
		const bool bStanceBroken = PoiseComponent->ApplyPoiseDamage(DamageInfo.PoiseDamage);

		// Check if character should stagger based on poise
		bool bShouldStagger = bStanceBroken;

		// Characters without hyper armor always stagger, regardless of poise
		if (!PoiseComponent->bHasHyperArmor || StateComponent->GetCurrentState() != ECharacterState::Attacking)
		{
			bShouldStagger = true;
		}

		if (bShouldStagger)
		{
			StateComponent->ForceState(ECharacterState::Stunned);

			// Stop any active attack montage BEFORE playing hit reaction
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				AnimInstance->StopAllMontages(0.1f);
			}

			// Stance broken = heavy stagger (opens riposte window)
			if (bStanceBroken)
			{
				PlayHitReaction(EHitReactionType::Heavy);
			}
			else
			{
				PlayHitReaction(DamageInfo.HitReaction);
			}
		}
	}

	// Apply knockback impulse through CharacterMovementComponent
	if (!DamageInfo.KnockbackImpulse.IsNearlyZero())
	{
		GetCharacterMovement()->AddImpulse(DamageInfo.KnockbackImpulse, true);
	}

	// Blueprint hook
	OnReceivedDamage(DamageInfo.DamageAmount, DamageInfo.HitLocation, DamageInfo.KnockbackImpulse.GetSafeNormal());
	OnDamageReceived.Broadcast(DamageInfo);
}

void ASoulsLikeBaseCharacter::HandleDeath()
{
	StateComponent->ForceState(ECharacterState::Dead);

	// Disable movement
	GetCharacterMovement()->DisableMovement();

	// Stop any playing montages
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.1f);
	}

	// Completely disable the capsule — no collision with anything
	// This prevents camera collision with the corpse and player pushing the body
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCapsuleHalfHeight(10.0f);
	GetCapsuleComponent()->SetCapsuleRadius(10.0f);

	// Disable camera collision on the mesh too (spring arm probes against mesh)
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Play death animation, then enable ragdoll so the body falls to the ground
	float DeathAnimDuration = 0.0f;

	if (DeathMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			DeathAnimDuration = AnimInstance->Montage_Play(DeathMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		}
	}
	else if (DeathAnimSequence)
	{
		GetMesh()->PlayAnimation(DeathAnimSequence, false);
		DeathAnimDuration = DeathAnimSequence->GetPlayLength();
	}

	// After the death animation finishes, enable ragdoll so the body collapses to the ground
	if (DeathAnimDuration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DeathRagdollTimerHandle, this,
			&ASoulsLikeBaseCharacter::EnableDeathRagdoll, DeathAnimDuration, false);
	}
	else
	{
		// No animation — ragdoll immediately
		EnableDeathRagdoll();
	}

	OnDied.Broadcast();
	OnDeathStarted();
}

void ASoulsLikeBaseCharacter::EnableDeathRagdoll()
{
	// Stop the animation so it doesn't fight the ragdoll
	GetMesh()->Stop();

	// Detach mesh from capsule so ragdoll isn't constrained by it
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// Use Ragdoll collision profile — it already blocks WorldStatic (floor)
	// Do NOT override its responses, or the ragdoll will fall through
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Only override camera — ragdoll should not push the player camera
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// Enable ragdoll physics
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
}

void ASoulsLikeBaseCharacter::HandleParry(AActor* ParriedActor)
{
	// Play parry montage on self
	const UWeaponDataAsset* WeaponData = WeaponManager->GetCurrentWeaponData();
	if (WeaponData && WeaponData->ParryMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(WeaponData->ParryMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		}
	}

	// Stun the parried actor
	if (ParriedActor)
	{
		ASoulsLikeBaseCharacter* ParriedCharacter = Cast<ASoulsLikeBaseCharacter>(ParriedActor);
		if (ParriedCharacter)
		{
			ParriedCharacter->StateComponent->ForceState(ECharacterState::Stunned);

			// Play stun montage on the parried character
			if (ParriedCharacter->StunMontage)
			{
				if (UAnimInstance* AnimInstance = ParriedCharacter->GetMesh()->GetAnimInstance())
				{
					AnimInstance->StopAllMontages(0.1f);
					AnimInstance->Montage_Play(ParriedCharacter->StunMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
				}
			}
		}
	}
}

void ASoulsLikeBaseCharacter::PlayHitReaction(EHitReactionType ReactionType)
{
	UAnimMontage* ReactionMontage = nullptr;
	UAnimSequenceBase* ReactionSequence = nullptr;

	switch (ReactionType)
	{
	case EHitReactionType::Light:
		ReactionMontage = LightHitReactionMontage;
		ReactionSequence = LightHitReactionSequence;
		break;
	case EHitReactionType::Heavy:
	case EHitReactionType::Knockdown:
		ReactionMontage = HeavyHitReactionMontage;
		ReactionSequence = HeavyHitReactionSequence;
		break;
	default:
		break;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	if (ReactionMontage)
	{
		// Use pre-assigned montage
		const float MontageLength = AnimInstance->Montage_Play(ReactionMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		if (MontageLength > 0.0f)
		{
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEndedDelegate, ReactionMontage);
		}
	}
	else if (ReactionSequence)
	{
		// Play AnimSequence as dynamic montage — blends back to locomotion when done
		UAnimMontage* DynMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
			ReactionSequence, FName("DefaultSlot"), 0.1f, 0.2f, 1.0f, 1, 0.0f, 0.0f);
		if (DynMontage)
		{
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEndedDelegate, DynMontage);
		}
	}
}

// ===== INTERNAL =====

void ASoulsLikeBaseCharacter::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Reset attack state
	HitActorsThisSwing.Empty();
	bComboQueued = false;
	bComboWindowOpen = false;
	CurrentComboIndex = 0;

	// Re-enable orient-to-movement (disabled during dodge)
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Return to idle if not dead
	if (StateComponent->GetCurrentState() != ECharacterState::Dead)
	{
		StateComponent->ForceState(ECharacterState::Idle);
	}
}

void ASoulsLikeBaseCharacter::PlayAttackMontageSection(EAttackType AttackType, int32 ComboIndex)
{
	UAnimMontage* AttackMontage = WeaponManager->GetAttackMontage(AttackType);
	const FComboAttack* AttackData = WeaponManager->GetComboAttack(ComboIndex, AttackType);

	if (!AttackMontage || !AttackData)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	const float PlayRate = WeaponManager->GetAttackPlayRate(AttackType);

	if (ComboIndex == 0)
	{
		// Start from the beginning of the montage
		const float MontageLength = AnimInstance->Montage_Play(AttackMontage, PlayRate, EMontagePlayReturnType::MontageLength, 0.0f, true);
		if (MontageLength > 0.0f)
		{
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEndedDelegate, AttackMontage);

			// Jump to the correct section if it's not the first
			if (AttackData->MontageSectionName != NAME_None)
			{
				AnimInstance->Montage_JumpToSection(AttackData->MontageSectionName, AttackMontage);
			}
		}
	}
	else
	{
		// Jump to the combo section
		if (AttackData->MontageSectionName != NAME_None)
		{
			AnimInstance->Montage_JumpToSection(AttackData->MontageSectionName, AttackMontage);
		}
	}
}

void ASoulsLikeBaseCharacter::AdvanceCombo()
{
	const int32 NextComboIndex = CurrentComboIndex + 1;
	const FComboAttack* NextAttack = WeaponManager->GetComboAttack(NextComboIndex, CurrentAttackType);

	if (!NextAttack)
	{
		// No more attacks in the chain
		bComboQueued = false;
		return;
	}

	// Check and consume stamina for the next attack
	if (!StaminaComponent->ConsumeStamina(NextAttack->StaminaCost))
	{
		bComboQueued = false;
		return;
	}

	// Advance the combo
	CurrentComboIndex = NextComboIndex;
	bComboQueued = false;
	HitActorsThisSwing.Empty();

	PlayAttackMontageSection(CurrentAttackType, CurrentComboIndex);
}
