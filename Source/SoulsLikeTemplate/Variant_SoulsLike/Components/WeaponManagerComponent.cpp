// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponManagerComponent.h"
#include "WeaponBase.h"
#include "WeaponDataAsset.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-equip the first weapon in the inventory if available
	if (WeaponInventory.Num() > 0 && WeaponInventory[0])
	{
		CurrentWeaponIndex = 0;
		EquipWeapon(WeaponInventory[0]);
	}
}

void UWeaponManagerComponent::EquipWeapon(TSubclassOf<AWeaponBase> WeaponClass)
{
	if (!WeaponClass)
	{
		return;
	}

	// Unequip current weapon first
	UnequipWeapon();

	// Get the owning character's mesh
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	// Spawn the weapon actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;

	CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, SpawnParams);
	if (CurrentWeapon)
	{
		// Attach to the character mesh socket
		CurrentWeapon->AttachToCharacter(CharacterMesh, WeaponSocketName);

		// Cache the weapon data
		CurrentWeaponData = CurrentWeapon->GetWeaponData();
	}
}

void UWeaponManagerComponent::UnequipWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->DetachFromCharacter();
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
		CurrentWeaponData = nullptr;
	}
}

void UWeaponManagerComponent::SwitchWeapon(int32 Direction)
{
	if (WeaponInventory.Num() <= 1)
	{
		return;
	}

	// Cycle through the inventory
	CurrentWeaponIndex = (CurrentWeaponIndex + Direction) % WeaponInventory.Num();
	if (CurrentWeaponIndex < 0)
	{
		CurrentWeaponIndex += WeaponInventory.Num();
	}

	if (WeaponInventory.IsValidIndex(CurrentWeaponIndex) && WeaponInventory[CurrentWeaponIndex])
	{
		EquipWeapon(WeaponInventory[CurrentWeaponIndex]);
	}
}

const FComboAttack* UWeaponManagerComponent::GetComboAttack(int32 ComboIndex, EAttackType AttackType) const
{
	const UWeaponDataAsset* Data = GetCurrentWeaponData();
	if (!Data)
	{
		return nullptr;
	}

	const TArray<FComboAttack>& ComboChain = Data->GetComboChain(AttackType);
	if (ComboChain.IsValidIndex(ComboIndex))
	{
		return &ComboChain[ComboIndex];
	}

	return nullptr;
}

UAnimMontage* UWeaponManagerComponent::GetAttackMontage(EAttackType AttackType) const
{
	const UWeaponDataAsset* Data = GetCurrentWeaponData();
	if (!Data)
	{
		return nullptr;
	}

	return Data->GetAttackMontageData(AttackType).AttackMontage;
}

float UWeaponManagerComponent::GetAttackPlayRate(EAttackType AttackType) const
{
	const UWeaponDataAsset* Data = GetCurrentWeaponData();
	if (!Data)
	{
		return 1.0f;
	}

	return Data->GetAttackMontageData(AttackType).PlayRate * Data->AttackSpeedModifier;
}
