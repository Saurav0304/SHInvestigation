// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SHInvestigationGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ASHInvestigationGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ASHInvestigationGameMode();

	/** Rejects connections once MaxPlayers are in the game (the prototype is 2-player co-op) */
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

protected:

	/** Maximum number of players, host included */
	UPROPERTY(EditDefaultsOnly, Category="Multiplayer", meta=(ClampMin="1"))
	int32 MaxPlayers = 2;
};



