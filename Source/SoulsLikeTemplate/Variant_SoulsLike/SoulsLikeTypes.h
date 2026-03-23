// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoulsLikeTypes.generated.h"

// ===== ENUMS =====

/** Character action/behavior states */
UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle,
	Moving,
	Attacking,
	Dodging,
	Blocking,
	Stunned,
	Finisher,
	Dead
};

/** Weapon classification for animation selection */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Unarmed,
	Sword,
	Greatsword,
	AxeAndShield,
	Spear,
	DualDaggers
};

/** Attack weight class */
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Light,
	Heavy
};

/** Hit reaction severity */
UENUM(BlueprintType)
enum class EHitReactionType : uint8
{
	None,
	Light,
	Heavy,
	Knockdown
};

/** Elemental/physical damage classification */
UENUM(BlueprintType)
enum class ESoulsLikeDamageType : uint8
{
	Physical,
	Fire,
	Ice,
	Lightning,
	Poison
};

// ===== STRUCTS =====

class UAnimMontage;

/** Defines a single attack within a combo chain */
USTRUCT(BlueprintType)
struct FComboAttack
{
	GENERATED_BODY()

	/** Montage section name for this attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName MontageSectionName;

	/** Damage multiplier relative to the weapon's base damage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0.1))
	float DamageMultiplier = 1.0f;

	/** Stamina cost for this specific attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float StaminaCost = 15.0f;

	/** Hit reaction this attack inflicts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	EHitReactionType HitReaction = EHitReactionType::Light;

	/** Poise damage this attack inflicts (stance break threshold) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float PoiseDamage = 20.0f;

	/** Knockback impulse magnitude */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "cm/s"))
	float KnockbackImpulse = 200.0f;

	/** Trace radius for this attack's sphere sweep */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 10, Units = "cm"))
	float TraceRadius = 60.0f;

	/** Trace distance ahead of the character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 10, Units = "cm"))
	float TraceDistance = 180.0f;
};

/** Complete information about a damage event */
USTRUCT(BlueprintType)
struct FDamageInfo
{
	GENERATED_BODY()

	/** Raw damage amount before any reduction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float DamageAmount = 0.0f;

	/** Type of damage for resistances/effects */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	ESoulsLikeDamageType DamageType = ESoulsLikeDamageType::Physical;

	/** Knockback direction and magnitude */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FVector KnockbackImpulse = FVector::ZeroVector;

	/** What kind of hit reaction to play on the victim */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	EHitReactionType HitReaction = EHitReactionType::Light;

	/** The actor that caused this damage */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	TObjectPtr<AActor> SourceActor = nullptr;

	/** World-space point where the hit occurred */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FVector HitLocation = FVector::ZeroVector;

	/** Poise damage to apply (stance break) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float PoiseDamage = 0.0f;

	/** Whether this damage was blocked (set by receiver) */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bWasBlocked = false;

	/** Whether this damage was parried (set by receiver) */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bWasParried = false;
};

/** Defines attack montage data for animation selection */
USTRUCT(BlueprintType)
struct FAttackMontageData
{
	GENERATED_BODY()

	/** The animation montage containing all attack sections */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	/** Play rate modifier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = 0.1, ClampMax = 3.0))
	float PlayRate = 1.0f;
};

// ===== DELEGATES =====

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, NewStaminaPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStateChanged, ECharacterState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageReceived, const FDamageInfo&, DamageInfo);
