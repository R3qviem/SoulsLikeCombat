// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LevelUpWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;
class UButton;
class UCanvasPanel;

/**
 *  Visual-only level up menu displayed when resting at a bonfire.
 *  Shows stat categories with + buttons (non-functional for now).
 *  Closed via X button in top-right corner.
 */
UCLASS()
class ULevelUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	/** Called when the X button is clicked */
	UFUNCTION()
	void OnCloseClicked();

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

private:

	void BuildLayout();

	/** Create a single stat row: label, current value, + button */
	UBorder* CreateStatRow(const FString& StatName, int32 CurrentValue);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY()
	TObjectPtr<UBorder> MainPanel;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;
};
