// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeGameMode.h"
#include "SoulsLikePlayerController.h"
#include "SoulsLikePlayerCharacter.h"
#include "SoulsLikeDebugHUD.h"

ASoulsLikeGameMode::ASoulsLikeGameMode()
{
	// Set the default player controller class
	PlayerControllerClass = ASoulsLikePlayerController::StaticClass();

	// Set the default pawn class
	DefaultPawnClass = ASoulsLikePlayerCharacter::StaticClass();

	// Set the debug HUD class (draws health/stamina/poise/state on screen)
	HUDClass = ASoulsLikeDebugHUD::StaticClass();
}
