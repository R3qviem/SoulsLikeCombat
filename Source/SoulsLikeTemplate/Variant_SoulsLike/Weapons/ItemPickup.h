// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class UItemDataAsset;
class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraComponent;
class ASoulsLikePlayerCharacter;

/**
 *  World pickup actor — appears as an ethereal glowing orb.
 *  Place in the level and assign an ItemDataAsset.
 */
UCLASS()
class AItemPickup : public AActor
{
	GENERATED_BODY()

public:

	AItemPickup();

	virtual void Tick(float DeltaTime) override;

	/** The item this pickup contains */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemDataAsset> ItemData;

	/** How many of this item */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 ItemCount = 1;

	/** Try to add this item to the character's inventory, destroy if successful */
	void PickUp(ASoulsLikePlayerCharacter* Character);

protected:

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractionSphere;

	/** The glowing orb mesh */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	/** Light to illuminate surroundings */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> OrbLight;

	/** VFX effect */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> OrbVFX;

private:

	float BobTime = 0.0f;
	FVector SpawnLocation = FVector::ZeroVector;

	virtual void BeginPlay() override;
};
