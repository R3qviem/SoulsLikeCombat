// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SoulsLikePlayerController.generated.h"

class UInputMappingContext;
class USoulsLikeHUD;
class UInventoryWidget;
class UQuickItemWidget;
class ASoulsLikePlayerCharacter;

/**
 *  PlayerController for the Souls-Like game mode.
 *  Handles input mapping context setup, HUD creation, and character respawning.
 */
UCLASS()
class ASoulsLikePlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Constructor */
	ASoulsLikePlayerController();

protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

public:

	/** Set the transform where the player will respawn */
	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void SetRespawnTransform(const FTransform& NewTransform);

	/** Toggle inventory UI visibility */
	void ToggleInventory();

	/** Refresh the quick item HUD display */
	void RefreshQuickItemHUD();

protected:

	/** Input mapping contexts to apply by default */
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/** HUD widget class to create */
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<USoulsLikeHUD> HUDWidgetClass;

	/** The active HUD widget */
	UPROPERTY()
	TObjectPtr<USoulsLikeHUD> HUDWidget;

	/** Inventory widget */
	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	/** Quick item HUD (bottom-left) */
	UPROPERTY()
	TObjectPtr<UQuickItemWidget> QuickItemWidget;

	bool bInventoryOpen = false;

	/** Transform where the player will respawn */
	FTransform RespawnTransform;

	/** Time to wait before respawning */
	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (ClampMin = 0, Units = "s"))
	float RespawnDelay = 3.0f;

private:

	/** Timer handle for respawning */
	FTimerHandle RespawnTimerHandle;

	/** Called when the possessed pawn is destroyed */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** Perform the actual respawn */
	void RespawnPlayer();

	/** Bind the HUD to the current character's delegates */
	void BindHUDToCharacter(ASoulsLikePlayerCharacter* InCharacter);

	/** Wrapper for health delegate -> HUD update */
	UFUNCTION()
	void OnPlayerHealthChanged(float NewHealthPercent);

	/** Wrapper for stamina delegate -> HUD update */
	UFUNCTION()
	void OnPlayerStaminaChanged(float NewStaminaPercent);
};
