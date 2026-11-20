// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 * GameMode for Lvl_MainMenu: no pawn, just the menu controller.
 */
UCLASS()
class SHINVESTIGATION_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

	/** The menu has no pawn to spawn. */
	virtual void RestartPlayer(AController* NewPlayer) override {}
};
