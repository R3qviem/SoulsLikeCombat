// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BonfireCheckpoint.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UPointLightComponent;

/**
 *  Bonfire checkpoint — rest here to heal, set respawn point, and respawn all enemies.
 *  Player interacts with F key when in range.
 */
UCLASS(Blueprintable)
class ABonfireCheckpoint : public AActor
{
	GENERATED_BODY()

public:

	ABonfireCheckpoint();

	/** Called by the player character when interacting nearby */
	void Rest(class ASoulsLikePlayerCharacter* Player);

	/** Whether this bonfire has been discovered (lit) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	bool bIsLit = false;

protected:

	virtual void BeginPlay() override;

	/** Interaction range sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** Visual base mesh (simple cylinder/rock) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Fire VFX */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> FireVFX;

	/** Warm light from the fire */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FireLight;
};
