// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeZombieEnemy.h"
#include "WeaponManagerComponent.h"
#include "WeaponDataAsset.h"
#include "CharacterStateComponent.h"
#include "StaminaComponent.h"
#include "SoulsLikeDamageable.h"
#include "SoulsLikeTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ASoulsLikeZombieEnemy::ASoulsLikeZombieEnemy()
{
	// Use the zombie pack's UE4 Mannequin mesh
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ZombieMeshFinder(
		TEXT("/Game/BSP_ZombieAnims/EpicAssets/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	if (ZombieMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(ZombieMeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Clear the ABP — user will assign a zombie-specific ABP via Blueprint
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetAnimInstanceClass(nullptr);

	// Load zombie locomotion animations
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleFinder(
		TEXT("/Game/BSP_ZombieAnims/Animations/Standing/AS_NAAT_Zombie_BL_Idle.AS_NAAT_Zombie_BL_Idle"));
	if (IdleFinder.Succeeded()) { IdleAnimation = IdleFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkFinder(
		TEXT("/Game/BSP_ZombieAnims/Animations/Standing/AS_NAAT_Zombie_BL_Walk.AS_NAAT_Zombie_BL_Walk"));
	if (WalkFinder.Succeeded()) { WalkAnimation = WalkFinder.Object; }

	// Load zombie attack animation (right hand swipe)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackFinder(
		TEXT("/Game/BSP_ZombieAnims/Animations/Standing/Attack/AS_NAAT_Zombie_Attack_RH_Stand.AS_NAAT_Zombie_Attack_RH_Stand"));
	if (AttackFinder.Succeeded()) { AttackAnimation = AttackFinder.Object; }

	// Use SlumpToStand for a quicker get-up
	static ConstructorHelpers::FObjectFinder<UAnimSequence> GetUpFinder(
		TEXT("/Game/BSP_ZombieAnims/Animations/GetUp/AS_NAAT_Zombie_SlumpToStand.AS_NAAT_Zombie_SlumpToStand"));
	if (GetUpFinder.Succeeded()) { GetUpAnimation = GetUpFinder.Object; }

	// Zombie stats — slower but tougher and hits harder
	MaxHealth = 60.0f;
	CurrentHealth = 60.0f;

	// Slower movement (shambling zombie — always walks, never runs)
	GetCharacterMovement()->MaxWalkSpeed = 180.0f;
	PatrolSpeed = 60.0f;
	ChaseSpeed = 180.0f;

	// More aggressive AI — wider detection, shorter cooldowns
	SightRange = 1500.0f;
	SightAngle = 90.0f;
	AttackRange = 180.0f;
	AttackCooldown = 1.2f;
	PatrolRadius = 400.0f;
	PatrolWaitTime = 4.0f;
	LosePlayerDistance = 2500.0f;

	// Zombie weapon data — unarmed claws
	{
		UWeaponDataAsset* ZombieClawData = CreateDefaultSubobject<UWeaponDataAsset>(TEXT("ZombieClawData"));
		ZombieClawData->WeaponName = FText::FromString(TEXT("Zombie Claws"));
		ZombieClawData->WeaponType = EWeaponType::Unarmed;
		ZombieClawData->BaseDamage = 15.0f;
		ZombieClawData->DodgeStaminaCost = 20.0f;

		FComboAttack LightAttack1;
		LightAttack1.MontageSectionName = NAME_None;
		LightAttack1.DamageMultiplier = 1.0f;
		LightAttack1.StaminaCost = 10.0f;
		LightAttack1.HitReaction = EHitReactionType::Light;
		ZombieClawData->LightComboChain.Add(LightAttack1);

		FComboAttack LightAttack2;
		LightAttack2.MontageSectionName = NAME_None;
		LightAttack2.DamageMultiplier = 1.3f;
		LightAttack2.StaminaCost = 12.0f;
		LightAttack2.HitReaction = EHitReactionType::Light;
		ZombieClawData->LightComboChain.Add(LightAttack2);

		FComboAttack HeavyAttack;
		HeavyAttack.MontageSectionName = NAME_None;
		HeavyAttack.DamageMultiplier = 2.0f;
		HeavyAttack.StaminaCost = 20.0f;
		HeavyAttack.HitReaction = EHitReactionType::Heavy;
		HeavyAttack.KnockbackImpulse = 350.0f;
		ZombieClawData->HeavyComboChain.Add(HeavyAttack);

		WeaponManager->UnarmedWeaponData = ZombieClawData;
	}
}

void ASoulsLikeZombieEnemy::BeginPlay()
{
	Super::BeginPlay();

	CachedPlayer = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

	if (bStartsResting)
	{
		bIsResting = true;
		bIsGettingUp = false;

		// Disable movement and collision while resting (invulnerable + untargetable)
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Play the lying prone pose (first frame of get-up, frozen)
		if (GetUpAnimation)
		{
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			GetMesh()->PlayAnimation(GetUpAnimation, false);
			GetMesh()->SetPosition(0.0f);
			GetMesh()->Stop();
		}
	}
	else
	{
		// Start with idle animation in single-node mode
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (IdleAnimation)
		{
			GetMesh()->PlayAnimation(IdleAnimation, true);
			CurrentMoveAnim = EZombieMoveAnim::Idle;
		}
	}
}

void ASoulsLikeZombieEnemy::Tick(float DeltaTime)
{
	// === RESTING STATE ===
	if (bIsResting)
	{
		if (bIsGettingUp)
		{
			return; // Wait for get-up animation
		}

		// Check if player is close enough to wake up
		if (CachedPlayer && CachedPlayer->IsValidLowLevel())
		{
			const float DistToPlayer = FVector::Dist(GetActorLocation(), CachedPlayer->GetActorLocation());
			if (DistToPlayer <= WakeUpRange)
			{
				bIsGettingUp = true;

				if (GetUpAnimation)
				{
					GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
					GetMesh()->PlayAnimation(GetUpAnimation, false);
					float GetUpDuration = GetUpAnimation->GetPlayLength();

					GetWorld()->GetTimerManager().SetTimer(GetUpTimerHandle, this,
						&ASoulsLikeZombieEnemy::OnGetUpFinished, GetUpDuration, false);
				}
				else
				{
					OnGetUpFinished();
				}
			}
		}
		return;
	}

	// === NORMAL AI ===
	Super::Tick(DeltaTime);

	// === MOVEMENT ANIMATION ===
	UpdateMovementAnimation();
}

void ASoulsLikeZombieEnemy::UpdateMovementAnimation()
{
	// Don't override attack/stun/death animations
	ECharacterState State = StateComponent->GetCurrentState();
	if (State == ECharacterState::Attacking || State == ECharacterState::Stunned ||
		State == ECharacterState::Dead || State == ECharacterState::Dodging)
	{
		CurrentMoveAnim = EZombieMoveAnim::None;
		return;
	}

	const float Speed = GetVelocity().Size2D();
	EZombieMoveAnim DesiredAnim = (Speed < 5.0f) ? EZombieMoveAnim::Idle : EZombieMoveAnim::Walk;

	// Only switch animation if it changed
	if (DesiredAnim != CurrentMoveAnim)
	{
		CurrentMoveAnim = DesiredAnim;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

		if (DesiredAnim == EZombieMoveAnim::Idle)
		{
			if (IdleAnimation)
			{
				GetMesh()->PlayAnimation(IdleAnimation, true);
				GetMesh()->SetPlayRate(0.8f);
			}
		}
		else
		{
			if (WalkAnimation)
			{
				GetMesh()->PlayAnimation(WalkAnimation, true);
				// Scale walk play rate based on speed for smoother look
				GetMesh()->SetPlayRate(0.85f);
			}
		}
	}
	else if (CurrentMoveAnim == EZombieMoveAnim::Walk)
	{
		// Dynamically adjust walk play rate based on actual speed
		const float SpeedRatio = FMath::Clamp(Speed / 180.0f, 0.4f, 1.0f);
		GetMesh()->SetPlayRate(SpeedRatio * 0.85f);
	}
}

void ASoulsLikeZombieEnemy::OnGetUpFinished()
{
	bIsResting = false;
	bIsGettingUp = false;

	// Re-enable collision and movement
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// Start with idle animation
	CurrentMoveAnim = EZombieMoveAnim::Idle;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	if (IdleAnimation)
	{
		GetMesh()->PlayAnimation(IdleAnimation, true);
	}

	// Face the player
	if (CachedPlayer)
	{
		const FVector ToPlayer = (CachedPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		SetActorRotation(ToPlayer.Rotation());
	}
}

// ===== INVULNERABILITY WHILE RESTING =====

void ASoulsLikeZombieEnemy::ReceiveDamage(const FDamageInfo& DamageInfo)
{
	// Immune while resting or getting up
	if (bIsResting || bIsGettingUp)
	{
		return;
	}

	ASoulsLikeBaseCharacter::ReceiveDamage(DamageInfo);
}

bool ASoulsLikeZombieEnemy::IsDead() const
{
	// Treat as "dead" (untargetable) while resting or getting up
	if (bIsResting || bIsGettingUp)
	{
		return true;
	}

	return ASoulsLikeBaseCharacter::IsDead();
}

// ===== ATTACK (no AnimInstance — uses PlayAnimation + timer) =====

void ASoulsLikeZombieEnemy::DoAIAttack(EAttackType AttackType, int32 MaxComboCount)
{
	if (StateComponent->GetCurrentState() == ECharacterState::Attacking)
	{
		return;
	}

	if (!StateComponent->CanPerformAction())
	{
		return;
	}

	if (!AttackAnimation)
	{
		return;
	}

	// Enter attacking state
	StateComponent->RequestStateChange(ECharacterState::Attacking);
	CurrentMoveAnim = EZombieMoveAnim::None;

	// Play attack animation (slightly slower for heavier feel)
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(AttackAnimation, false);
	GetMesh()->SetPlayRate(0.9f);

	// Schedule damage trace at ~40% of animation (when the swipe connects)
	const float AttackDuration = AttackAnimation->GetPlayLength() / 0.9f;
	GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this,
		&ASoulsLikeZombieEnemy::DoZombieAttackTrace, AttackDuration * 0.4f, false);

	// Schedule attack end at animation finish
	FTimerHandle AttackEndTimer;
	GetWorld()->GetTimerManager().SetTimer(AttackEndTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		OnAttackAnimFinished();
	}), AttackDuration, false);
}

void ASoulsLikeZombieEnemy::DoZombieAttackTrace()
{
	const UWeaponDataAsset* WeaponData = WeaponManager->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return;
	}

	// Sphere sweep in front of the zombie
	const FVector TraceStart = GetActorLocation() + FVector(0, 0, 50.0f);
	const FVector TraceEnd = TraceStart + GetActorForwardVector() * AttackRange;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(50.0f);

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

			ISoulsLikeDamageable* Damageable = Cast<ISoulsLikeDamageable>(HitActor);
			if (!Damageable || Damageable->IsDead())
			{
				continue;
			}

			FDamageInfo DamageInfoToSend;
			DamageInfoToSend.DamageAmount = WeaponData->BaseDamage;
			DamageInfoToSend.DamageType = ESoulsLikeDamageType::Physical;
			DamageInfoToSend.HitReaction = EHitReactionType::Light;
			DamageInfoToSend.SourceActor = this;
			DamageInfoToSend.HitLocation = Hit.ImpactPoint;

			const FVector KnockbackDir = (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			DamageInfoToSend.KnockbackImpulse = KnockbackDir * 200.0f;

			Damageable->ReceiveDamage(DamageInfoToSend);
			break; // Only hit one target per swipe
		}
	}
}

void ASoulsLikeZombieEnemy::OnAttackAnimFinished()
{
	if (StateComponent->GetCurrentState() == ECharacterState::Dead)
	{
		return;
	}

	// Return to idle state
	StateComponent->ForceState(ECharacterState::Idle);
	CurrentMoveAnim = EZombieMoveAnim::None;

	// Resume idle animation
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	if (IdleAnimation)
	{
		GetMesh()->PlayAnimation(IdleAnimation, true);
	}

	OnAttackCompleted.ExecuteIfBound();
}
