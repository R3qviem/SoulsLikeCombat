// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulsLikeTypes.h"
#include "WeaponManagerComponent.generated.h"

class AWeaponBase;
class UWeaponDataAsset;

/**
 *  Manages weapon inventory, equipping, and switching.
 *  Provides current weapon data to combat and animation systems.
 */
UCLASS(ClassGroup = (SoulsLike), meta = (BlueprintSpawnableComponent))
class UWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UWeaponManagerComponent();

	virtual void BeginPlay() override;

	/** Equip a weapon by spawning its actor and attaching to the character mesh */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(TSubclassOf<AWeaponBase> WeaponClass);

	/** Destroy the currently equipped weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon();

	/** Cycle through the weapon inventory. Direction: +1 = next, -1 = previous */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon(int32 Direction);

	/** Get the currently equipped weapon actor */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	/** Get the current weapon's data asset (falls back to UnarmedWeaponData if no weapon equipped) */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	const UWeaponDataAsset* GetCurrentWeaponData() const { return CurrentWeaponData ? CurrentWeaponData : UnarmedWeaponData; }

	/**
	 * Get a specific attack in the combo chain for the current weapon.
	 * Returns nullptr if out of range or no weapon equipped.
	 */
	const FComboAttack* GetComboAttack(int32 ComboIndex, EAttackType AttackType) const;

	/** Get the attack montage for the given attack type from the current weapon */
	UAnimMontage* GetAttackMontage(EAttackType AttackType) const;

	/** Get the attack montage play rate for the given attack type */
	float GetAttackPlayRate(EAttackType AttackType) const;

	/** Check whether a weapon is currently equipped */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool HasWeaponEquipped() const { return CurrentWeapon != nullptr; }

	/** Fallback weapon data used when no weapon is equipped (unarmed combat) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDataAsset> UnarmedWeaponData;

protected:

	/** Blueprint-configurable list of weapon classes the character can switch between */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<TSubclassOf<AWeaponBase>> WeaponInventory;

	/** Socket on the skeletal mesh for weapon attachment */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponSocketName = FName("weapon_r");

	/** Index of the currently equipped weapon in the inventory */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 CurrentWeaponIndex = 0;

private:

	/** The currently equipped weapon actor */
	UPROPERTY()
	TObjectPtr<AWeaponBase> CurrentWeapon;

	/** Quick access to the active weapon's data */
	UPROPERTY()
	TObjectPtr<const UWeaponDataAsset> CurrentWeaponData;
};
