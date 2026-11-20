// Copyright Epic Games, Inc. All Rights Reserved.

#include "SHInvestigationGameMode.h"
#include "Core/InvestigationGameState.h"

ASHInvestigationGameMode::ASHInvestigationGameMode()
{
	// shared, replicated investigation progress (evidence journal)
	GameStateClass = AInvestigationGameState::StaticClass();
}

void ASHInvestigationGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// a non-empty ErrorMessage refuses the connection; the client gets it as a network failure
	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaxPlayers)
	{
		ErrorMessage = TEXT("Server is full");
	}
}
