// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UWeaponDataAsset;

/**
 *  The physical weapon actor in the world.
 *  Spawned and attached to the character's skeletal mesh socket.
 *  Intentionally simple: visual container only; all stats come from the data asset.
 */
UCLASS(abstract)
class AWeaponBase : public AActor
{
	GENERATED_BODY()

public:

	/** Constructor */
	AWeaponBase();

	/** Attach this weapon to a character's skeletal mesh at the specified socket */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachToCharacter(USkeletalMeshComponent* TargetMesh, FName SocketName);

	/** Detach from current parent */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DetachFromCharacter();

	/** Get the weapon data asset */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	const UWeaponDataAsset* GetWeaponData() const { return WeaponData; }

protected:

	/** Visual representation of the weapon */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	/** The data asset defining this weapon's stats, combo chains, and animation references */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDataAsset> WeaponData;
};
