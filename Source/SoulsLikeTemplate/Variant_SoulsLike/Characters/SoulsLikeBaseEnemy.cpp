// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeBaseEnemy.h"
#include "CharacterStateComponent.h"
#include "StaminaComponent.h"
#include "WeaponManagerComponent.h"
#include "WeaponDataAsset.h"
#include "SoulsLikeAIController.h"
#include "SoulsLikeEnemyHealthBar.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ASoulsLikeBaseEnemy::ASoulsLikeBaseEnemy()
{
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
		static ConstructorHelpers::FObjectFinder<UAnimMontage> ChargedMontageFinder(
			TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack"));

		if (ComboMontageFinder.Succeeded())
		{
			UnarmedData->LightAttackMontage.AttackMontage = ComboMontageFinder.Object;
		}
		if (ChargedMontageFinder.Succeeded())
		{
			UnarmedData->HeavyAttackMontage.AttackMontage = ChargedMontageFinder.Object;
		}

		// Light combo chain (3-hit combo)
		FComboAttack LightAttack1;
		LightAttack1.MontageSectionName = NAME_None;
		LightAttack1.DamageMultiplier = 1.0f;
		LightAttack1.StaminaCost = 10.0f;
		LightAttack1.HitReaction = EHitReactionType::Light;
		UnarmedData->LightComboChain.Add(LightAttack1);

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
}

void ASoulsLikeBaseEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Get the health bar widget instance
	if (HealthBarWidget)
	{
		HealthBarInstance = Cast<USoulsLikeEnemyHealthBar>(HealthBarWidget->GetUserWidgetObject());
		if (HealthBarInstance)
		{
			HealthBarInstance->SetHealthPercentage(1.0f);
		}
	}

	// Bind health changed to update the health bar
	OnHealthChanged.AddDynamic(this, &ASoulsLikeBaseEnemy::OnHealthBarUpdate);
}

void ASoulsLikeBaseEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
}

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

	// Determine random combo length
	const TArray<FComboAttack>& ComboChain = WeaponManager->GetCurrentWeaponData()->GetComboChain(AttackType);
	AITargetComboCount = FMath::RandRange(1, FMath::Min(MaxComboCount, ComboChain.Num()));
	AICurrentComboAttack = 0;

	// Consume stamina (if applicable)
	StaminaComponent->ConsumeStamina(AttackData->StaminaCost);

	// Transition to attacking state
	StateComponent->RequestStateChange(ECharacterState::Attacking);

	// Start the attack
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

	// AI auto-advances combo if under target count
	AICurrentComboAttack++;
	if (AICurrentComboAttack < AITargetComboCount)
	{
		bComboQueued = true;
		AdvanceCombo();
	}
}

void ASoulsLikeBaseEnemy::OnActionEnd()
{
	// Call base implementation
	ASoulsLikeBaseCharacter::OnActionEnd();

	// Notify StateTree that the attack is complete
	OnAttackCompleted.ExecuteIfBound();
}

void ASoulsLikeBaseEnemy::HandleDeath()
{
	// Disable collision capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Call base death handling
	ASoulsLikeBaseCharacter::HandleDeath();

	// Hide health bar
	if (HealthBarWidget)
	{
		HealthBarWidget->SetHiddenInGame(true);
	}

	// Broadcast died delegate
	OnEnemyDied.Broadcast();

	// Schedule removal from level
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &ASoulsLikeBaseEnemy::RemoveFromLevel, DeathRemovalTime);
}

void ASoulsLikeBaseEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	OnEnemyLanded.ExecuteIfBound();
}

void ASoulsLikeBaseEnemy::NotifyOfDanger(const FVector& Location, AActor* Source)
{
	if (Source && Source->ActorHasTag(FName("Player")))
	{
		LastDangerLocation = Location;
		LastDangerTime = GetWorld()->GetTimeSeconds();
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
		HealthBarInstance->SetHealthPercentage(NewHealthPercent);
	}
}
