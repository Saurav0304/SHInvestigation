// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/MainMenuPlayerController.h"

#include "Core/InvestigationGameInstance.h"
#include "UI/MainMenuWidget.h"

AMainMenuPlayerController::AMainMenuPlayerController()
{
	MainMenuWidgetClass = UMainMenuWidget::StaticClass();
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UMainMenuWidget>(this, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();

			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
		}
	}

	bShowMouseCursor = true;

	// -autohost / -autojoin (only the first time the menu is shown in this process)
	if (UInvestigationGameInstance* GameInstance = GetGameInstance<UInvestigationGameInstance>())
	{
		GameInstance->RunCommandLineAutoAction();
	}
}
