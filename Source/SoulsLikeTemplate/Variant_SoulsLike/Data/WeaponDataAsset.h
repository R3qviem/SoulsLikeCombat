// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulsLikeTypes.h"
#include "WeaponDataAsset.generated.h"

/**
 *  Data asset defining all properties of a weapon type.
 *  Creating a new weapon = creating a new instance of this asset + providing animations.
 *  No C++ changes required to add new weapons (unless adding a new EWeaponType enum value).
 */
UCLASS(BlueprintType)
class UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Display name for this weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FText WeaponName;

	/** Weapon type enum tag used by AnimInstance to select animation set */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	EWeaponType WeaponType = EWeaponType::Sword;

	// --- Combat Stats ---

	/** Base damage, multiplied by FComboAttack::DamageMultiplier per swing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = 0))
	float BaseDamage = 20.0f;

	/** Multiplied into montage play rate. Higher = faster swings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = 0.1, ClampMax = 3.0))
	float AttackSpeedModifier = 1.0f;

	/** Percentage of incoming damage reduced when blocking (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Block", meta = (ClampMin = 0, ClampMax = 1))
	float BlockDamageReduction = 0.5f;

	/** Seconds after block start that count as a parry */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Block", meta = (ClampMin = 0, ClampMax = 1, Units = "s"))
	float ParryWindowDuration = 0.2f;

	/** Stamina consumed when dodging */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stamina", meta = (ClampMin = 0))
	float DodgeStaminaCost = 20.0f;

	/** Stamina consumed per blocked hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stamina", meta = (ClampMin = 0))
	float BlockStaminaCostPerHit = 15.0f;

	// --- Animation Data ---

	/** Montage data for light attacks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	FAttackMontageData LightAttackMontage;

	/** Montage data for heavy attacks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	FAttackMontageData HeavyAttackMontage;

	/** Dodge animation montage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	/** Animation when hit while blocking */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> BlockHitReactionMontage;

	/** Animation when successfully parrying */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> ParryMontage;

	// --- Combo Chain Definitions ---

	/** Ordered array of light attack data - defines the full light combo chain */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<FComboAttack> LightComboChain;

	/** Ordered array of heavy attack data - defines the full heavy combo chain */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<FComboAttack> HeavyComboChain;

	// --- Visual ---

	/** Static mesh for the weapon actor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> WeaponMesh;

	/** Transform offset for socket attachment alignment */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FTransform MeshOffset;

	// --- Helpers ---

	/** Get the combo chain array for the given attack type */
	const TArray<FComboAttack>& GetComboChain(EAttackType AttackType) const
	{
		return AttackType == EAttackType::Light ? LightComboChain : HeavyComboChain;
	}

	/** Get the montage data for the given attack type */
	const FAttackMontageData& GetAttackMontageData(EAttackType AttackType) const
	{
		return AttackType == EAttackType::Light ? LightAttackMontage : HeavyAttackMontage;
	}
};
