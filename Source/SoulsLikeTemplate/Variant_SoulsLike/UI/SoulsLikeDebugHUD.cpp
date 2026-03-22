// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikeDebugHUD.h"
#include "SoulsLikePlayerCharacter.h"
#include "CharacterStateComponent.h"
#include "StaminaComponent.h"
#include "PoiseComponent.h"
#include "Engine/Canvas.h"

void ASoulsLikeDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	ASoulsLikePlayerCharacter* Player = Cast<ASoulsLikePlayerCharacter>(GetOwningPawn());
	if (!Player)
	{
		return;
	}

	const float BarWidth = 250.0f;
	const float BarHeight = 20.0f;
	const float StartX = 20.0f;
	float CurrentY = 20.0f;
	const float Spacing = 8.0f;

	// Health bar (red)
	DrawBar(StartX, CurrentY, BarWidth, BarHeight, Player->GetHealthPercent(),
		FLinearColor(0.8f, 0.1f, 0.1f, 0.9f), FLinearColor(0.2f, 0.02f, 0.02f, 0.7f),
		FString::Printf(TEXT("HP: %.0f / %.0f"), Player->CurrentHealth, Player->MaxHealth));
	CurrentY += BarHeight + Spacing;

	// Stamina bar (green)
	if (Player->StaminaComponent)
	{
		DrawBar(StartX, CurrentY, BarWidth, BarHeight, Player->StaminaComponent->GetStaminaPercent(),
			FLinearColor(0.1f, 0.7f, 0.2f, 0.9f), FLinearColor(0.02f, 0.15f, 0.04f, 0.7f),
			FString::Printf(TEXT("ST: %.0f / %.0f"), Player->StaminaComponent->GetCurrentStamina(), Player->StaminaComponent->GetMaxStamina()));
		CurrentY += BarHeight + Spacing;
	}

	// Poise bar (yellow)
	if (Player->PoiseComponent)
	{
		FLinearColor PoiseColor = Player->PoiseComponent->IsStanceBroken()
			? FLinearColor(1.0f, 0.0f, 0.0f, 0.9f)
			: FLinearColor(0.9f, 0.8f, 0.1f, 0.9f);
		FString PoiseLabel = Player->PoiseComponent->IsStanceBroken()
			? TEXT("POISE: BROKEN!")
			: FString::Printf(TEXT("Poise: %.0f%%"), Player->PoiseComponent->GetPoisePercent() * 100.0f);
		DrawBar(StartX, CurrentY, BarWidth, BarHeight, Player->PoiseComponent->GetPoisePercent(),
			PoiseColor, FLinearColor(0.2f, 0.18f, 0.02f, 0.7f), PoiseLabel);
		CurrentY += BarHeight + Spacing;
	}

	// Character state text
	if (Player->StateComponent)
	{
		static const TMap<ECharacterState, FString> StateNames = {
			{ECharacterState::Idle, TEXT("IDLE")},
			{ECharacterState::Moving, TEXT("MOVING")},
			{ECharacterState::Attacking, TEXT("ATTACKING")},
			{ECharacterState::Dodging, TEXT("DODGING")},
			{ECharacterState::Blocking, TEXT("BLOCKING")},
			{ECharacterState::Stunned, TEXT("STUNNED")},
			{ECharacterState::Finisher, TEXT("FINISHER")},
			{ECharacterState::Dead, TEXT("DEAD")}
		};

		const FString* StateName = StateNames.Find(Player->StateComponent->GetCurrentState());
		FString StateText = FString::Printf(TEXT("State: %s"), StateName ? **StateName : TEXT("UNKNOWN"));

		// Color code the state
		FLinearColor StateColor = FLinearColor::White;
		switch (Player->StateComponent->GetCurrentState())
		{
		case ECharacterState::Attacking: StateColor = FLinearColor(1.0f, 0.5f, 0.0f); break;
		case ECharacterState::Dodging: StateColor = FLinearColor(0.3f, 0.7f, 1.0f); break;
		case ECharacterState::Blocking: StateColor = FLinearColor(0.5f, 0.5f, 1.0f); break;
		case ECharacterState::Stunned: StateColor = FLinearColor(1.0f, 1.0f, 0.0f); break;
		case ECharacterState::Dead: StateColor = FLinearColor(0.5f, 0.0f, 0.0f); break;
		default: break;
		}

		// Draw state background and text
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), StartX, CurrentY, BarWidth, BarHeight);
		DrawText(StateText, StateColor, StartX + 5.0f, CurrentY + 2.0f, GEngine->GetSmallFont(), 1.0f);
		CurrentY += BarHeight + Spacing;
	}

	// Controls reminder
	CurrentY += 10.0f;
	const FLinearColor HintColor(0.7f, 0.7f, 0.7f);
	DrawText(TEXT("LMB: Light Attack | RMB: Heavy Attack"), HintColor, StartX, CurrentY, GEngine->GetSmallFont(), 0.9f);
	CurrentY += 16.0f;
	DrawText(TEXT("Space: Dodge | LCtrl: Block | MMB: Lock-On"), HintColor, StartX, CurrentY, GEngine->GetSmallFont(), 0.9f);
	CurrentY += 16.0f;
	DrawText(TEXT("WASD: Move | Mouse: Look | Q/E: Switch Target"), HintColor, StartX, CurrentY, GEngine->GetSmallFont(), 0.9f);
}

void ASoulsLikeDebugHUD::DrawBar(float X, float Y, float Width, float Height, float Percent, FLinearColor FillColor, FLinearColor BackgroundColor, const FString& Label)
{
	Percent = FMath::Clamp(Percent, 0.0f, 1.0f);

	// Background
	DrawRect(BackgroundColor, X, Y, Width, Height);

	// Fill
	if (Percent > 0.0f)
	{
		DrawRect(FillColor, X, Y, Width * Percent, Height);
	}

	// Border lines
	const FLinearColor BorderColor(1.0f, 1.0f, 1.0f, 0.5f);
	DrawLine(X, Y, X + Width, Y, BorderColor);
	DrawLine(X + Width, Y, X + Width, Y + Height, BorderColor);
	DrawLine(X + Width, Y + Height, X, Y + Height, BorderColor);
	DrawLine(X, Y + Height, X, Y, BorderColor);

	// Label text
	DrawText(Label, FLinearColor::White, X + 5.0f, Y + 2.0f, GEngine->GetSmallFont(), 1.0f);
}
