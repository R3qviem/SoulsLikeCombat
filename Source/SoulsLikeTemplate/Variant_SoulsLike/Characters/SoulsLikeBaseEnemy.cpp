// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeBaseEnemy.h"
#include "CharacterStateComponent.h"
#include "StaminaComponent.h"
#include "WeaponManagerComponent.h"
#include "WeaponDataAsset.h"
#include "SoulsLikeAIController.h"
#include "SoulsLikeEnemyHealthBar.h"
#include "SoulsLikeDamageable.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PointLightComponent.h"

ASoulsLikeBaseEnemy::ASoulsLikeBaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set the AI Controller class
	AIControllerClass = ASoulsLikeAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Movement: face the direction the AI controller wants
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;

	// Set default skeletal mesh (Manny mannequin)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Set default animation blueprint
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPFinder(
		TEXT("/Game/Variant_Combat/Anims/ABP_Manny_Combat"));
	if (AnimBPFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBPFinder.Class);
	}

	// Create unarmed weapon data with existing montages
	{
		UWeaponDataAsset* UnarmedData = CreateDefaultSubobject<UWeaponDataAsset>(TEXT("EnemyUnarmedData"));
		UnarmedData->WeaponName = FText::FromString(TEXT("Enemy Fists"));
		UnarmedData->WeaponType = EWeaponType::Unarmed;
		UnarmedData->BaseDamage = 10.0f;
		UnarmedData->DodgeStaminaCost = 20.0f;

		static ConstructorHelpers::FObjectFinder<UAnimMontage> ComboMontageFinder(
			TEXT("/Game/Variant_Combat/Anims/AM_ComboAttack.AM_ComboAttack"));
		if (ComboMontageFinder.Succeeded())
		{
			UnarmedData->LightAttackMontage.AttackMontage = ComboMontageFinder.Object;
			// Use same combo montage for heavy — AM_ChargedAttack loops forever
			UnarmedData->HeavyAttackMontage.AttackMontage = ComboMontageFinder.Object;
		}

		// Light combo chain
		FComboAttack LightAttack1;
		LightAttack1.MontageSectionName = NAME_None;
		LightAttack1.DamageMultiplier = 1.0f;
		LightAttack1.StaminaCost = 10.0f;
		LightAttack1.HitReaction = EHitReactionType::Light;
		UnarmedData->LightComboChain.Add(LightAttack1);

		// Heavy combo chain
		FComboAttack HeavyAttack;
		HeavyAttack.MontageSectionName = NAME_None;
		HeavyAttack.DamageMultiplier = 2.0f;
		HeavyAttack.StaminaCost = 25.0f;
		HeavyAttack.HitReaction = EHitReactionType::Heavy;
		HeavyAttack.KnockbackImpulse = 400.0f;
		UnarmedData->HeavyComboChain.Add(HeavyAttack);

		WeaponManager->UnarmedWeaponData = UnarmedData;
	}

	// Enemy tags
	Tags.Add(FName("Enemy"));

	// Default health
	MaxHealth = 80.0f;

	// Create health bar widget component
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	HealthBarWidget->SetDrawSize(FVector2D(150.0f, 15.0f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetWidgetClass(USoulsLikeEnemyHealthBar::StaticClass());

	// Lock-on indicator — red glow at chest height
	LockOnIndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LockOnIndicator"));
	LockOnIndicatorLight->SetupAttachment(GetMesh(), FName("spine_03"));
	LockOnIndicatorLight->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	LockOnIndicatorLight->SetLightColor(FLinearColor(1.0f, 0.1f, 0.05f));
	LockOnIndicatorLight->SetIntensity(5000.0f);
	LockOnIndicatorLight->SetAttenuationRadius(150.0f);
	LockOnIndicatorLight->SetCastShadows(false);
	LockOnIndicatorLight->SetVisibility(false);
}

void ASoulsLikeBaseEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Camera ignore
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Health bar setup
	if (HealthBarWidget)
	{
		HealthBarWidget->InitWidget();
		HealthBarInstance = Cast<USoulsLikeEnemyHealthBar>(HealthBarWidget->GetUserWidgetObject());
		if (HealthBarInstance)
		{
			HealthBarInstance->SetHealthPercent(1.0f);
		}
		HealthBarWidget->SetHiddenInGame(true);
	}
	OnHealthChanged.AddDynamic(this, &ASoulsLikeBaseEnemy::OnHealthBarUpdate);

	// AI setup
	SpawnLocation = GetActorLocation();
	PlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	AIControllerRef = Cast<AAIController>(GetController());

	// Start patrolling
	SetAIState(EEnemyAIState::Patrol);
}

void ASoulsLikeBaseEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
	GetWorld()->GetTimerManager().ClearTimer(PatrolWaitTimer);
	GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimer);
}

// ===== TICK — AI BEHAVIOR =====

void ASoulsLikeBaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (AIState)
	{
	case EEnemyAIState::Idle:
		AITick_Idle(DeltaTime);
		break;
	case EEnemyAIState::Patrol:
		AITick_Patrol(DeltaTime);
		break;
	case EEnemyAIState::Chase:
		AITick_Chase(DeltaTime);
		break;
	case EEnemyAIState::Attack:
		AITick_Attack(DeltaTime);
		break;
	case EEnemyAIState::CooldownAfterAttack:
		AITick_Cooldown(DeltaTime);
		break;
	case EEnemyAIState::Dead:
		break;
	}
}

void ASoulsLikeBaseEnemy::AITick_Idle(float DeltaTime)
{
	if (CanSeePlayer())
	{
		SetAIState(EEnemyAIState::Chase);
		return;
	}

	SetAIState(EEnemyAIState::Patrol);
}

void ASoulsLikeBaseEnemy::AITick_Patrol(float DeltaTime)
{
	// Always check for player — wider detection while patrolling
	if (CanSeePlayer())
	{
		bIsWaitingAtPatrol = false;
		bIsMovingToPatrol = false;
		GetWorld()->GetTimerManager().ClearTimer(PatrolWaitTimer);
		SetAIState(EEnemyAIState::Chase);
		return;
	}

	if (bIsWaitingAtPatrol)
	{
		return;
	}

	if (!bIsMovingToPatrol)
	{
		PatrolTarget = PickPatrolPoint();
		bIsMovingToPatrol = true;
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
	}

	// Walk toward patrol target using AddMovementInput (no NavMesh needed)
	const FVector ToTarget = (PatrolTarget - GetActorLocation()).GetSafeNormal2D();
	AddMovementInput(ToTarget, 1.0f);

	// Reached patrol point?
	const float DistToTarget = FVector::Dist2D(GetActorLocation(), PatrolTarget);
	if (DistToTarget < 100.0f)
	{
		bIsMovingToPatrol = false;
		bIsWaitingAtPatrol = true;

		GetWorld()->GetTimerManager().SetTimer(PatrolWaitTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			bIsWaitingAtPatrol = false;
		}), PatrolWaitTime + FMath::FRandRange(0.0f, 2.0f), false);
	}
}

void ASoulsLikeBaseEnemy::AITick_Chase(float DeltaTime)
{
	if (!IsPlayerValid())
	{
		SetAIState(EEnemyAIState::Patrol);
		return;
	}

	const float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());

	// Lost the player — return to patrol
	if (DistToPlayer > LosePlayerDistance)
	{
		SetAIState(EEnemyAIState::Patrol);
		return;
	}

	// In attack range — attack!
	if (DistToPlayer <= AttackRange)
	{
		SetAIState(EEnemyAIState::Attack);
		return;
	}

	// Run toward the player using direct movement
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
	const FVector ToPlayer = (PlayerCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	AddMovementInput(ToPlayer, 1.0f);

	// Face the player while chasing
	if (AIControllerRef)
	{
		AIControllerRef->SetFocalPoint(PlayerCharacter->GetActorLocation());
	}
}

void ASoulsLikeBaseEnemy::AITick_Attack(float DeltaTime)
{
	// Wait for current attack to finish
	if (StateComponent->GetCurrentState() == ECharacterState::Attacking ||
		StateComponent->GetCurrentState() == ECharacterState::Stunned)
	{
		// Even while attacking, keep facing the player
		if (IsPlayerValid())
		{
			const FVector ToPlayer = (PlayerCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			const FRotator TargetRot = ToPlayer.Rotation();
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 10.0f));
		}
		return;
	}

	if (!IsPlayerValid())
	{
		SetAIState(EEnemyAIState::Patrol);
		return;
	}

	const float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());

	// Player moved out of range — chase immediately
	if (DistToPlayer > AttackRange * 1.5f)
	{
		SetAIState(EEnemyAIState::Chase);
		return;
	}

	// Face the player
	const FVector ToPlayer = (PlayerCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	SetActorRotation(ToPlayer.Rotation());

	// Attack — randomly choose light or heavy
	const EAttackType ChosenAttack = (FMath::FRand() < 0.7f) ? EAttackType::Light : EAttackType::Heavy;
	DoAIAttack(ChosenAttack, 2);

	// Sometimes skip cooldown and attack again
	if (FMath::FRand() < 0.3f)
	{
		// Stay in Attack state — will attack again as soon as montage finishes
	}
	else
	{
		SetAIState(EEnemyAIState::CooldownAfterAttack);
	}
}

void ASoulsLikeBaseEnemy::AITick_Cooldown(float DeltaTime)
{
	// Wait for attack montage to finish
	if (StateComponent->GetCurrentState() == ECharacterState::Attacking ||
		StateComponent->GetCurrentState() == ECharacterState::Stunned)
	{
		return;
	}

	if (!IsPlayerValid())
	{
		SetAIState(EEnemyAIState::Patrol);
		return;
	}

	const float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
	const FVector ToPlayer = (PlayerCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

	// Always face the player during cooldown
	const FRotator TargetRot = ToPlayer.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 10.0f));

	// If player moved away, don't wait — chase immediately
	if (DistToPlayer > AttackRange * 1.5f)
	{
		GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimer);
		SetAIState(EEnemyAIState::Chase);
		return;
	}

	// Close range: slowly advance toward the player to maintain pressure
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed * 1.5f;
	const FVector StrafeDir = FVector::CrossProduct(ToPlayer, FVector::UpVector) * StrafeDirection;
	// Mix strafe with forward pressure
	const FVector MoveDir = (StrafeDir * 0.5f + ToPlayer * 0.5f).GetSafeNormal();
	AddMovementInput(MoveDir, 1.0f);

	// Cooldown timer handles transition back to Chase/Attack
}

// ===== AI HELPERS =====

bool ASoulsLikeBaseEnemy::CanSeePlayer() const
{
	if (!PlayerCharacter || !PlayerCharacter->IsValidLowLevel())
	{
		return false;
	}

	// Check if player is dead
	const ISoulsLikeDamageable* Damageable = Cast<ISoulsLikeDamageable>(PlayerCharacter);
	if (Damageable && Damageable->IsDead())
	{
		return false;
	}

	const FVector ToPlayer = PlayerCharacter->GetActorLocation() - GetActorLocation();
	const float Distance = ToPlayer.Size();

	// Range check
	if (Distance > SightRange)
	{
		return false;
	}

	// Close range: detect from any direction (peripheral awareness)
	const float CloseRange = SightRange * 0.4f;
	if (Distance <= CloseRange)
	{
		return true;
	}

	// Cone check for farther distances
	const FVector DirToPlayer = ToPlayer.GetSafeNormal2D();
	const float DotProduct = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), DirToPlayer);
	const float CosAngle = FMath::Cos(FMath::DegreesToRadians(SightAngle));

	if (DotProduct < CosAngle)
	{
		return false;
	}

	// Line of sight check
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(PlayerCharacter);

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		HitResult, GetActorLocation() + FVector(0, 0, 50), PlayerCharacter->GetActorLocation() + FVector(0, 0, 50),
		ECC_Visibility, QueryParams);

	return !bBlocked;
}

FVector ASoulsLikeBaseEnemy::PickPatrolPoint() const
{
	const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
	const float RandomDist = FMath::FRandRange(PatrolRadius * 0.3f, PatrolRadius);

	return SpawnLocation + FVector(
		FMath::Cos(FMath::DegreesToRadians(RandomAngle)) * RandomDist,
		FMath::Sin(FMath::DegreesToRadians(RandomAngle)) * RandomDist,
		0.0f);
}

void ASoulsLikeBaseEnemy::SetAIState(EEnemyAIState NewState)
{
	if (AIState == NewState || AIState == EEnemyAIState::Dead)
	{
		return;
	}

	AIState = NewState;

	switch (NewState)
	{
	case EEnemyAIState::CooldownAfterAttack:
		// Randomize strafe direction
		StrafeDirection = (FMath::FRand() < 0.5f) ? -1.0f : 1.0f;
		// Short cooldown, then go right back to attacking
		GetWorld()->GetTimerManager().SetTimer(AttackCooldownTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (AIState == EEnemyAIState::CooldownAfterAttack)
			{
				// Go straight to Attack if still in range, otherwise Chase
				if (IsPlayerValid())
				{
					const float Dist = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
					SetAIState(Dist <= AttackRange * 1.5f ? EEnemyAIState::Attack : EEnemyAIState::Chase);
				}
				else
				{
					SetAIState(EEnemyAIState::Patrol);
				}
			}
		}), FMath::FRandRange(0.8f, AttackCooldown), false);
		break;

	case EEnemyAIState::Chase:
		if (AIControllerRef)
		{
			AIControllerRef->ClearFocus(EAIFocusPriority::Gameplay);
		}
		break;

	case EEnemyAIState::Patrol:
		bIsMovingToPatrol = false;
		bIsWaitingAtPatrol = false;
		if (AIControllerRef)
		{
			AIControllerRef->ClearFocus(EAIFocusPriority::Gameplay);
		}
		break;

	default:
		break;
	}
}

// ===== COMBAT =====

void ASoulsLikeBaseEnemy::DoAIAttack(EAttackType AttackType, int32 MaxComboCount)
{
	if (StateComponent->GetCurrentState() == ECharacterState::Attacking)
	{
		return;
	}

	if (!StateComponent->CanPerformAction())
	{
		return;
	}

	const FComboAttack* AttackData = WeaponManager->GetComboAttack(0, AttackType);
	if (!AttackData)
	{
		return;
	}

	const TArray<FComboAttack>& ComboChain = WeaponManager->GetCurrentWeaponData()->GetComboChain(AttackType);
	AITargetComboCount = FMath::RandRange(1, FMath::Min(MaxComboCount, ComboChain.Num()));
	AICurrentComboAttack = 0;

	StaminaComponent->ConsumeStamina(AttackData->StaminaCost);
	StateComponent->RequestStateChange(ECharacterState::Attacking);

	CurrentComboIndex = 0;
	CurrentAttackType = AttackType;
	bComboQueued = false;
	bComboWindowOpen = false;
	HitActorsThisSwing.Empty();

	PlayAttackMontageSection(AttackType, 0);
}

void ASoulsLikeBaseEnemy::OnComboWindowOpen()
{
	bComboWindowOpen = true;

	AICurrentComboAttack++;
	if (AICurrentComboAttack < AITargetComboCount)
	{
		bComboQueued = true;
		AdvanceCombo();
	}
}

void ASoulsLikeBaseEnemy::OnActionEnd()
{
	ASoulsLikeBaseCharacter::OnActionEnd();
	OnAttackCompleted.ExecuteIfBound();
}

// ===== DEATH =====

void ASoulsLikeBaseEnemy::HandleDeath()
{
	SetAIState(EEnemyAIState::Dead);

	// Stop AI movement
	if (AIControllerRef)
	{
		AIControllerRef->StopMovement();
		AIControllerRef->ClearFocus(EAIFocusPriority::Gameplay);
	}

	ASoulsLikeBaseCharacter::HandleDeath();

	if (HealthBarWidget)
	{
		HealthBarWidget->SetHiddenInGame(true);
	}
	if (LockOnIndicatorLight)
	{
		LockOnIndicatorLight->SetVisibility(false);
	}

	OnEnemyDied.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &ASoulsLikeBaseEnemy::RemoveFromLevel, DeathRemovalTime);
}

void ASoulsLikeBaseEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	OnEnemyLanded.ExecuteIfBound();
}

// ===== AI HELPERS =====

bool ASoulsLikeBaseEnemy::IsPlayerValid() const
{
	if (!PlayerCharacter || !PlayerCharacter->IsValidLowLevel())
	{
		return false;
	}
	const ISoulsLikeDamageable* Damageable = Cast<ISoulsLikeDamageable>(PlayerCharacter);
	return !(Damageable && Damageable->IsDead());
}

// ===== DAMAGE =====

void ASoulsLikeBaseEnemy::ProcessDamage(FDamageInfo DamageInfo)
{
	ASoulsLikeBaseCharacter::ProcessDamage(DamageInfo);

	// Aggro on hit — immediately chase the player if not already engaged or dead
	if (AIState == EEnemyAIState::Idle || AIState == EEnemyAIState::Patrol)
	{
		SetAIState(EEnemyAIState::Chase);
	}
}

// ===== DANGER =====

void ASoulsLikeBaseEnemy::NotifyOfDanger(const FVector& Location, AActor* Source)
{
	if (Source && Source->ActorHasTag(FName("Player")))
	{
		LastDangerLocation = Location;
		LastDangerTime = GetWorld()->GetTimeSeconds();
	}
}

// ===== UI =====

void ASoulsLikeBaseEnemy::ShowHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->SetHiddenInGame(false);
		if (HealthBarInstance)
		{
			HealthBarInstance->SetHealthPercent(GetHealthPercent());
		}
	}
	if (LockOnIndicatorLight)
	{
		LockOnIndicatorLight->SetVisibility(true);
	}
}

void ASoulsLikeBaseEnemy::HideHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->SetHiddenInGame(true);
	}
	if (LockOnIndicatorLight)
	{
		LockOnIndicatorLight->SetVisibility(false);
	}
}

void ASoulsLikeBaseEnemy::RemoveFromLevel()
{
	Destroy();
}

void ASoulsLikeBaseEnemy::OnHealthBarUpdate(float NewHealthPercent)
{
	if (HealthBarInstance)
	{
		HealthBarInstance->SetHealthPercent(NewHealthPercent);
	}
}
