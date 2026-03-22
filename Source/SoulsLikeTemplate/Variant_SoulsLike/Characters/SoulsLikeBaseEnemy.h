// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoulsLikeBaseCharacter.h"
#include "SoulsLikeBaseEnemy.generated.h"

class UWidgetComponent;
class USoulsLikeEnemyHealthBar;

/** Completed attack animation delegate for StateTree */
DECLARE_DELEGATE(FOnSLEnemyAttackCompleted);

/** Landed delegate for StateTree */
DECLARE_DELEGATE(FOnSLEnemyLanded);

/** Enemy died delegate */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSLEnemyDied);

/**
 *  Base enemy class for the Souls-Like system.
 *  Provides AI hooks, danger tracking, death removal, and StateTree delegates.
 *  Derive from this class (C++ or Blueprint) to create specific enemy types.
 */
UCLASS()
class ASoulsLikeBaseEnemy : public ASoulsLikeBaseCharacter
{
	GENERATED_BODY()

public:

	/** Constructor */
	ASoulsLikeBaseEnemy();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	// ===== AI ATTACK =====

	/**
	 * AI-initiated attack. Picks a random combo length up to MaxComboCount.
	 * Plays the montage and auto-advances combo for AI.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void DoAIAttack(EAttackType AttackType, int32 MaxComboCount);

	// ===== ISoulsLikeAttacker OVERRIDES =====

	virtual void OnComboWindowOpen() override;
	virtual void OnActionEnd() override;

	// ===== DAMAGE OVERRIDES =====

	virtual void HandleDeath() override;

	// ===== LANDING =====

	virtual void Landed(const FHitResult& Hit) override;

	// ===== DANGER TRACKING =====

	/** Show the health bar (called when targeted by lock-on) */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowHealthBar();

	/** Hide the health bar (called when lock-on releases) */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideHealthBar();

	/** Record an incoming danger source for AI reaction */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void NotifyOfDanger(const FVector& Location, AActor* Source);

	/** Get the last recorded danger location */
	UFUNCTION(BlueprintPure, Category = "AI")
	const FVector& GetLastDangerLocation() const { return LastDangerLocation; }

	/** Get the last game time a danger event was received */
	UFUNCTION(BlueprintPure, Category = "AI")
	float GetLastDangerTime() const { return LastDangerTime; }

	// ===== DELEGATES =====

	/** Attack completed delegate for StateTree tasks */
	FOnSLEnemyAttackCompleted OnAttackCompleted;

	/** Landed delegate for StateTree tasks */
	FOnSLEnemyLanded OnEnemyLanded;

	/** Enemy died delegate. Allows external subscribers (spawners, level scripts) to respond. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSLEnemyDied OnEnemyDied;

protected:

	/** Health bar widget component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** Pointer to the health bar widget instance */
	UPROPERTY()
	TObjectPtr<USoulsLikeEnemyHealthBar> HealthBarInstance;

	/** Time to wait before removing this character from the level after death */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death", meta = (ClampMin = 0, Units = "s"))
	float DeathRemovalTime = 5.0f;

	/** Target number of attacks in the current AI combo */
	int32 AITargetComboCount = 0;

	/** Current position in the AI combo */
	int32 AICurrentComboAttack = 0;

private:

	/** Death removal timer handle */
	FTimerHandle DeathTimer;

	/** Last recorded danger location */
	FVector LastDangerLocation = FVector::ZeroVector;

	/** Last recorded game time a danger was detected */
	float LastDangerTime = -1000.0f;

	/** Remove this character from the level */
	void RemoveFromLevel();

	/** Health bar update callback bound to OnHealthChanged */
	UFUNCTION()
	void OnHealthBarUpdate(float NewHealthPercent);
};
