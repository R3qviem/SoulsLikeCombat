// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoulsLikeBaseCharacter.h"
#include "SoulsLikeBaseEnemy.generated.h"

class UWidgetComponent;
class USoulsLikeEnemyHealthBar;
class AAIController;
class UPointLightComponent;

/** Completed attack animation delegate for StateTree */
DECLARE_DELEGATE(FOnSLEnemyAttackCompleted);

/** Landed delegate for StateTree */
DECLARE_DELEGATE(FOnSLEnemyLanded);

/** Enemy died delegate */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSLEnemyDied);

/** Simple AI behavior states */
UENUM()
enum class EEnemyAIState : uint8
{
	Idle,
	Patrol,
	Chase,
	Circling,
	Attack,
	CooldownAfterAttack,
	Dead
};

/**
 *  Base enemy class for the Souls-Like system.
 *  Has built-in AI behavior: patrol, detect player, chase, attack.
 */
UCLASS()
class ASoulsLikeBaseEnemy : public ASoulsLikeBaseCharacter
{
	GENERATED_BODY()

public:

	/** Constructor */
	ASoulsLikeBaseEnemy();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	// ===== AI ATTACK =====

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void DoAIAttack(EAttackType AttackType, int32 MaxComboCount);

	// ===== ISoulsLikeAttacker OVERRIDES =====

	virtual void OnComboWindowOpen() override;
	virtual void OnActionEnd() override;

	// ===== DAMAGE OVERRIDES =====

	virtual void ProcessDamage(FDamageInfo DamageInfo) override;
	virtual void HandleDeath() override;

	// ===== LANDING =====

	virtual void Landed(const FHitResult& Hit) override;

	// ===== UI =====

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowHealthBar();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideHealthBar();

	// ===== DANGER TRACKING =====

	UFUNCTION(BlueprintCallable, Category = "AI")
	void NotifyOfDanger(const FVector& Location, AActor* Source);

	UFUNCTION(BlueprintPure, Category = "AI")
	const FVector& GetLastDangerLocation() const { return LastDangerLocation; }

	UFUNCTION(BlueprintPure, Category = "AI")
	float GetLastDangerTime() const { return LastDangerTime; }

	// ===== DELEGATES =====

	FOnSLEnemyAttackCompleted OnAttackCompleted;
	FOnSLEnemyLanded OnEnemyLanded;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSLEnemyDied OnEnemyDied;

protected:

	// ===== COMPONENTS =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** Red glow light for lock-on indicator (attached to chest) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> LockOnIndicatorLight;

	UPROPERTY()
	TObjectPtr<USoulsLikeEnemyHealthBar> HealthBarInstance;

	// ===== DEATH =====

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death", meta = (ClampMin = 0, Units = "s"))
	float DeathRemovalTime = 5.0f;

	// ===== AI COMBAT STATE =====

	int32 AITargetComboCount = 0;
	int32 AICurrentComboAttack = 0;

	// ===== AI BEHAVIOR SETTINGS =====

	/** How far the enemy can see the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 100, Units = "cm"))
	float SightRange = 1200.0f;

	/** Field of view half-angle for detection (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 10, ClampMax = 180, Units = "deg"))
	float SightAngle = 70.0f;

	/** Distance at which the enemy will attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 50, Units = "cm"))
	float AttackRange = 200.0f;

	/** Max cooldown between attacks (actual is randomized shorter) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 0.1, Units = "s"))
	float AttackCooldown = 1.8f;

	/** Patrol radius around spawn point */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 100, Units = "cm"))
	float PatrolRadius = 500.0f;

	/** Time to wait at each patrol point */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 0.5, Units = "s"))
	float PatrolWaitTime = 3.0f;

	/** Walk speed while patrolling */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 50, Units = "cm/s"))
	float PatrolSpeed = 150.0f;

	/** Run speed while chasing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 100, Units = "cm/s"))
	float ChaseSpeed = 420.0f;

	/** Distance at which the enemy loses the player and returns to patrol */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 500, Units = "cm"))
	float LosePlayerDistance = 2000.0f;

	// ===== TACTICAL AI SETTINGS =====

	/** Whether this enemy uses tactical circling behavior (false = rush straight in) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Tactical")
	bool bUseTacticalAI = true;

	/** Speed while circling/strafing the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Tactical", meta = (ClampMin = 50, Units = "cm/s"))
	float CircleSpeed = 200.0f;

	/** Minimum time to circle before considering an attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Tactical", meta = (ClampMin = 0.5, Units = "s"))
	float MinCircleTime = 1.5f;

	/** Maximum time to circle before committing to an attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Tactical", meta = (ClampMin = 1.0, Units = "s"))
	float MaxCircleTime = 4.0f;

	/** Chance to attack when player is in a vulnerable state (attacking, recovering) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Tactical", meta = (ClampMin = 0, ClampMax = 1))
	float AttackOpportunismChance = 0.6f;

	/** Chance to use heavy attack vs light */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Tactical", meta = (ClampMin = 0, ClampMax = 1))
	float HeavyAttackChance = 0.25f;

	/** Ideal distance to maintain while circling (relative to AttackRange) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Tactical", meta = (ClampMin = 1.0, ClampMax = 3.0))
	float CircleDistanceMultiplier = 1.5f;

private:

	// ===== AI STATE =====

	EEnemyAIState AIState = EEnemyAIState::Idle;

	/** Spawn location for patrol origin */
	FVector SpawnLocation = FVector::ZeroVector;

	/** Current patrol destination */
	FVector PatrolTarget = FVector::ZeroVector;

	/** Cached player character reference */
	UPROPERTY()
	TObjectPtr<ACharacter> PlayerCharacter;

	/** AI Controller reference */
	UPROPERTY()
	TObjectPtr<AAIController> AIControllerRef;

	/** Timer for patrol wait */
	FTimerHandle PatrolWaitTimer;

	/** Timer for attack cooldown */
	FTimerHandle AttackCooldownTimer;

	/** Whether currently waiting at a patrol point */
	bool bIsWaitingAtPatrol = false;

	/** Whether currently moving to patrol point */
	bool bIsMovingToPatrol = false;

	/** Circle-strafe direction (-1 or 1), randomized per cooldown */
	float StrafeDirection = 1.0f;

	// ===== CIRCLING STATE =====

	float CircleTimer = 0.0f;
	float CircleDuration = 0.0f;
	float DirectionChangeTimer = 0.0f;

	// ===== AI BEHAVIOR =====

	void AITick_Idle(float DeltaTime);
	void AITick_Patrol(float DeltaTime);
	void AITick_Chase(float DeltaTime);
	void AITick_Circling(float DeltaTime);
	void AITick_Attack(float DeltaTime);
	void AITick_Cooldown(float DeltaTime);

	/** Check if the player is visible (in range + in sight cone + alive) */
	bool CanSeePlayer() const;

	/** Pick a random patrol point near spawn */
	FVector PickPatrolPoint() const;

	/** Transition to a new AI state */
	void SetAIState(EEnemyAIState NewState);

	/** Check if player reference is valid and alive */
	bool IsPlayerValid() const;

	/** Check if player is in a punishable state */
	bool IsPlayerVulnerable() const;

	/** Decide whether to commit to an attack during circling */
	bool ShouldAttackNow() const;

	// ===== EXISTING =====

	FTimerHandle DeathTimer;
	FVector LastDangerLocation = FVector::ZeroVector;
	float LastDangerTime = -1000.0f;

	void RemoveFromLevel();

	UFUNCTION()
	void OnHealthBarUpdate(float NewHealthPercent);
};
