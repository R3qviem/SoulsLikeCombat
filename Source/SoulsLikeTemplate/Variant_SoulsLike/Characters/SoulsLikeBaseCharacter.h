// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SoulsLikeTypes.h"
#include "SoulsLikeDamageable.h"
#include "SoulsLikeAttacker.h"
#include "CombatAttacker.h"
#include "SoulsLikeBaseCharacter.generated.h"

class UCharacterStateComponent;
class UStaminaComponent;
class UWeaponManagerComponent;
class UPoiseComponent;
class UAnimMontage;

DECLARE_LOG_CATEGORY_EXTERN(LogSoulsLikeCharacter, Log, All);

/**
 *  Shared base class for all Souls-Like characters (player and enemies).
 *  Owns state, stamina, and weapon components.
 *  Implements the combat pipeline: attack, dodge, block, parry, damage processing.
 */
UCLASS(abstract)
class ASoulsLikeBaseCharacter : public ACharacter, public ISoulsLikeDamageable, public ISoulsLikeAttacker, public ICombatAttacker
{
	GENERATED_BODY()

public:

	/** Constructor */
	ASoulsLikeBaseCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;

public:

	// ===== COMPONENTS =====

	/** State machine component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterStateComponent> StateComponent;

	/** Stamina component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaminaComponent> StaminaComponent;

	/** Weapon management component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWeaponManagerComponent> WeaponManager;

	/** Poise (stance) component — determines stagger resistance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPoiseComponent> PoiseComponent;

	// ===== HEALTH =====

	/** Maximum health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;

	/** Current health */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.0f;

	/** Broadcast when health changes */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChanged OnHealthChanged;

	/** Broadcast when this character dies */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCharacterDied OnDied;

	/** Broadcast when damage is received */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDamageReceived OnDamageReceived;

	// ===== HIT REACTION MONTAGES =====

	/** Montage for light hit reactions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Reactions")
	TObjectPtr<UAnimMontage> LightHitReactionMontage;

	/** Montage for heavy hit reactions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Reactions")
	TObjectPtr<UAnimMontage> HeavyHitReactionMontage;

	/** Montage for knockdown reactions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Reactions")
	TObjectPtr<UAnimMontage> KnockdownMontage;

	/** Montage for death */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Reactions")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** Montage for stun recovery (after being parried) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Reactions")
	TObjectPtr<UAnimMontage> StunMontage;

	// ===== FINISHER MONTAGES =====

	/** Montage played on the attacker performing a riposte (front finisher) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Finisher")
	TObjectPtr<UAnimMontage> RiposteMontage;

	/** Montage played on the attacker performing a backstab */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Finisher")
	TObjectPtr<UAnimMontage> BackstabMontage;

	/** Montage played on the VICTIM of a riposte */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Finisher")
	TObjectPtr<UAnimMontage> RiposteVictimMontage;

	/** Montage played on the VICTIM of a backstab */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Finisher")
	TObjectPtr<UAnimMontage> BackstabVictimMontage;

	/** Damage multiplier for finisher attacks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Finisher", meta = (ClampMin = 1.0))
	float FinisherDamageMultiplier = 3.0f;

	// ===== COMBAT ACTIONS =====

	/**
	 * Attempt to perform an attack.
	 * Checks state and stamina, consumes stamina, plays the montage from weapon data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AttemptAttack(EAttackType AttackType);

	/**
	 * Attempt to dodge in the given direction.
	 * Checks state and stamina, consumes stamina, plays dodge montage, launches character.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AttemptDodge(FVector2D InputDirection);

	/** Begin blocking */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AttemptBlock();

	/** Stop blocking */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void EndBlock();

	/**
	 * Attempt a finisher (riposte or backstab) on a nearby target.
	 * Checks for stance-broken enemies in front, or enemies facing away for backstab.
	 * Returns true if a finisher was initiated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool AttemptFinisher();

	/** Reset health to maximum */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	// ===== ISoulsLikeDamageable INTERFACE =====

	virtual void ReceiveDamage(const FDamageInfo& DamageInfo) override;
	virtual bool IsDead() const override;
	virtual float GetHealthPercent() const override;

	// ===== ISoulsLikeAttacker INTERFACE =====

	virtual void ExecuteAttackTrace(FName SourceBone) override;
	virtual void OnComboWindowOpen() override;
	virtual void OnComboWindowClose() override;
	virtual void OnActionEnd() override;

	// ===== ICombatAttacker INTERFACE (bridge for existing montage notifies) =====

	virtual void DoAttackTrace(FName DamageSourceBone) override;
	virtual void CheckCombo() override;
	virtual void CheckChargedAttack() override;

protected:

	// ===== DAMAGE PROCESSING =====

	/**
	 * Central damage processing pipeline.
	 * Checks blocking/parry, applies damage, triggers reactions.
	 */
	virtual void ProcessDamage(FDamageInfo DamageInfo);

	/** Handle character death */
	virtual void HandleDeath();

	/** Handle a successful parry against the given attacker */
	virtual void HandleParry(AActor* ParriedActor);

	/** Play the appropriate hit reaction montage based on severity */
	void PlayHitReaction(EHitReactionType ReactionType);

	// ===== BLUEPRINT HOOKS =====

	/** Blueprint hook called when damage is dealt to another actor */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnDealtDamage(float Damage, const FVector& ImpactPoint);

	/** Blueprint hook called when damage is received */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnReceivedDamage(float Damage, const FVector& ImpactPoint, const FVector& DamageDirection);

	/** Blueprint hook called on death */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnDeathStarted();

	// ===== COMBAT STATE =====

	/** Current position in the combo chain */
	int32 CurrentComboIndex = 0;

	/** Whether the combo input window is currently open */
	bool bComboWindowOpen = false;

	/** Whether the player has queued the next combo input */
	bool bComboQueued = false;

	/** The type of attack currently being performed */
	EAttackType CurrentAttackType = EAttackType::Light;

	/** Actors already hit during the current swing (prevents double hits) */
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	/** Time at which blocking began (for parry window calculation) */
	float BlockStartTime = 0.0f;

	/** Whether the character is holding a charged heavy attack (set by input on player, always false on AI) */
	bool bIsChargingHeavy = false;

	/** Whether the charged attack has looped at least once */
	bool bHasLoopedChargedAttack = false;

	/** Section name in the charged attack montage for the charge loop */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charged")
	FName ChargeLoopSection = FName("Charge");

	/** Section name in the charged attack montage for the actual attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charged")
	FName ChargeAttackSection = FName("Attack");

	/** Dodge impulse strength (fallback when no montage) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Dodge", meta = (ClampMin = 0, Units = "cm/s"))
	float DodgeImpulse = 400.0f;

	/** Directional dodge montages (root motion drives movement) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeForwardMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeBackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeRightMontage;

	/** Max distance to detect finisher targets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Finisher", meta = (ClampMin = 50, Units = "cm"))
	float FinisherDetectionRange = 200.0f;

	/** Angle threshold for backstab detection (degrees from behind) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Finisher", meta = (ClampMin = 10, ClampMax = 90))
	float BackstabAngleThreshold = 60.0f;

	/** Death animation sequence (played directly on mesh — freezes on last frame) */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> DeathAnimSequence;

	/** Light hit reaction sequence (played as dynamic montage when no montage is assigned) */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> LightHitReactionSequence;

	/** Heavy hit reaction sequence (played as dynamic montage when no montage is assigned) */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> HeavyHitReactionSequence;

	// ===== INTERNAL =====

	/** Attack montage ended delegate */
	FOnMontageEnded OnAttackMontageEndedDelegate;

	/** Timer handle for dodge state recovery when no montage is available */
	FTimerHandle DodgeRecoveryTimerHandle;

	/** Timer handle for enabling ragdoll after death animation finishes */
	FTimerHandle DeathRagdollTimerHandle;

	/** Enable ragdoll physics so the corpse falls to the ground */
	void EnableDeathRagdoll();

	/** Called when the attack montage ends */
	void AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Play the montage section for the current combo attack */
	void PlayAttackMontageSection(EAttackType AttackType, int32 ComboIndex);

	/** Advance the combo to the next attack */
	void AdvanceCombo();
};
