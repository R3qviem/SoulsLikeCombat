// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoulsLikeBaseEnemy.h"
#include "SoulsLikeZombieEnemy.generated.h"

/**
 *  Zombie enemy variant — slower, more aggressive, uses zombie animations.
 *  Two placement modes:
 *    - bStartsResting = false: roams around like a normal enemy (patrol/chase)
 *    - bStartsResting = true:  lies on the ground, gets up when player is near, then chases
 */
UCLASS(Blueprintable)
class ASoulsLikeZombieEnemy : public ASoulsLikeBaseEnemy
{
	GENERATED_BODY()

public:

	ASoulsLikeZombieEnemy();

	virtual void Tick(float DeltaTime) override;

	// ===== INVULNERABILITY WHILE RESTING =====

	virtual void ReceiveDamage(const FDamageInfo& DamageInfo) override;
	virtual bool IsDead() const override;

	// ===== ATTACK OVERRIDE (no AnimInstance — uses PlayAnimation) =====

	virtual void DoAIAttack(EAttackType AttackType, int32 MaxComboCount) override;

protected:

	virtual void BeginPlay() override;

	// ===== ZOMBIE VARIANT SETTINGS =====

	/** If true, zombie starts lying on the ground and gets up when player approaches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie")
	bool bStartsResting = false;

	/** Distance at which the resting zombie wakes up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie", meta = (ClampMin = 200, Units = "cm"))
	float WakeUpRange = 800.0f;

	// ===== ZOMBIE ANIMATIONS =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimSequence> WalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimSequence> RunAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimSequence> GetUpAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimSequence> AttackAnimation;

private:

	/** Timer for attack animation duration */
	FTimerHandle AttackTimerHandle;
	void OnAttackAnimFinished();

	/** Update locomotion animation based on current velocity */
	void UpdateMovementAnimation();

	/** Which movement anim is currently playing (avoid restarting every frame) */
	enum class EZombieMoveAnim : uint8 { None, Idle, Walk, Run };
	EZombieMoveAnim CurrentMoveAnim = EZombieMoveAnim::None;

	// ===== RESTING STATE =====

	bool bIsResting = false;
	bool bIsGettingUp = false;

	FTimerHandle GetUpTimerHandle;
	void OnGetUpFinished();

	/** Perform a damage trace during attack (replaces AnimNotify since we have no AnimInstance) */
	void DoZombieAttackTrace();

	UPROPERTY()
	TObjectPtr<ACharacter> CachedPlayer;
};
