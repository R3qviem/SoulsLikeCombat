// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikePlayerController.h"
#include "SoulsLikeHUD.h"
#include "SoulsLikePlayerCharacter.h"
#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "StaminaComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"

ASoulsLikePlayerController::ASoulsLikePlayerController()
{
}

void ASoulsLikePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Create the HUD widget
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<USoulsLikeHUD>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}

	// Create the inventory widget (hidden by default)
	InventoryWidget = CreateWidget<UInventoryWidget>(this, UInventoryWidget::StaticClass());
	if (InventoryWidget)
	{
		InventoryWidget->AddToViewport(10);
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ASoulsLikePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Add input mapping contexts via Enhanced Input Subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (int32 i = 0; i < DefaultMappingContexts.Num(); ++i)
		{
			if (DefaultMappingContexts[i])
			{
				Subsystem->AddMappingContext(DefaultMappingContexts[i], i);
			}
		}
	}
}

void ASoulsLikePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		// Listen for pawn destruction (for respawn)
		InPawn->OnDestroyed.AddDynamic(this, &ASoulsLikePlayerController::OnPawnDestroyed);

		// Bind HUD to the character
		ASoulsLikePlayerCharacter* SoulsLikeCharacter = Cast<ASoulsLikePlayerCharacter>(InPawn);
		if (SoulsLikeCharacter)
		{
			BindHUDToCharacter(SoulsLikeCharacter);

			// Store the initial spawn transform as respawn point
			RespawnTransform = InPawn->GetActorTransform();
		}
	}
}

void ASoulsLikePlayerController::SetRespawnTransform(const FTransform& NewTransform)
{
	RespawnTransform = NewTransform;
}

void ASoulsLikePlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// Schedule respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &ASoulsLikePlayerController::RespawnPlayer, RespawnDelay, false);
}

void ASoulsLikePlayerController::RespawnPlayer()
{
	// Respawn the default pawn at the respawn transform
	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(
		GetWorld()->GetAuthGameMode()->DefaultPawnClass,
		RespawnTransform.GetLocation(),
		RespawnTransform.GetRotation().Rotator()
	);

	if (NewPawn)
	{
		Possess(NewPawn);
	}
}

void ASoulsLikePlayerController::BindHUDToCharacter(ASoulsLikePlayerCharacter* InCharacter)
{
	if (!HUDWidget || !InCharacter)
	{
		return;
	}

	// Bind health and stamina through wrapper functions
	InCharacter->OnHealthChanged.AddDynamic(this, &ASoulsLikePlayerController::OnPlayerHealthChanged);

	if (InCharacter->StaminaComponent)
	{
		InCharacter->StaminaComponent->OnStaminaChanged.AddDynamic(this, &ASoulsLikePlayerController::OnPlayerStaminaChanged);
	}

	// Initialize with current values
	HUDWidget->SetHealthPercent(InCharacter->GetHealthPercent());
	if (InCharacter->StaminaComponent)
	{
		HUDWidget->SetStaminaPercent(InCharacter->StaminaComponent->GetStaminaPercent());
	}
}

void ASoulsLikePlayerController::OnPlayerHealthChanged(float NewHealthPercent)
{
	if (HUDWidget)
	{
		HUDWidget->SetHealthPercent(NewHealthPercent);
	}
}

void ASoulsLikePlayerController::OnPlayerStaminaChanged(float NewStaminaPercent)
{
	if (HUDWidget)
	{
		HUDWidget->SetStaminaPercent(NewStaminaPercent);
	}
}

void ASoulsLikePlayerController::ToggleInventory()
{
	if (!InventoryWidget)
	{
		return;
	}

	bInventoryOpen = !bInventoryOpen;

	if (bInventoryOpen)
	{
		// Refresh with current items
		ASoulsLikePlayerCharacter* SLCharacter = Cast<ASoulsLikePlayerCharacter>(GetPawn());
		if (SLCharacter && SLCharacter->InventoryComponent)
		{
			InventoryWidget->RefreshInventory(SLCharacter->InventoryComponent->GetItems());
		}

		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
		SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
		SetShowMouseCursor(true);
	}
	else
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}
