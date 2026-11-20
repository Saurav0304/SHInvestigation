// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class UMainMenuWidget;

/**
 * Shows the main menu with a mouse cursor and UI-only input.
 */
UCLASS()
class SHINVESTIGATION_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainMenuPlayerController();

protected:
	virtual void BeginPlay() override;

	/** Defaults to the C++ layout; a Widget Blueprint child can be assigned instead. */
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UMainMenuWidget> MainMenuWidget;
};
