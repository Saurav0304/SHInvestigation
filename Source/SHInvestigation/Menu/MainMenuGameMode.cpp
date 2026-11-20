// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/MainMenuGameMode.h"

#include "Menu/MainMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
