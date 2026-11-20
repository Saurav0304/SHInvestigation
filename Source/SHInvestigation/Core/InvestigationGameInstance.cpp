// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InvestigationGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "SHInvestigation.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "InvestigationSession"

namespace InvestigationSession
{
	/** Custom session setting so we only join sessions of this game. */
	const FName GameIdKey(TEXT("SH_GAME_ID"));
	const FString GameIdValue(TEXT("SHInvestigation"));
}

UInvestigationGameInstance::UInvestigationGameInstance()
{
	GameplayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/Lvl_Investigation.Lvl_Investigation")));
}

void UInvestigationGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UInvestigationGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UInvestigationGameInstance::HandleTravelFailure);
	}
}

void UInvestigationGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

IOnlineSessionPtr UInvestigationGameInstance::GetSessionInterface() const
{
	return Online::GetSessionInterface(GetWorld());
}

bool UInvestigationGameInstance::IsLanSubsystem() const
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	return !Subsystem || Subsystem->GetSubsystemName() == NULL_SUBSYSTEM;
}

void UInvestigationGameInstance::SetStatus(const FText& Status, bool bInBusy)
{
	LastStatus = Status;
	bBusy = bInBusy;
	UE_LOG(LogSHInvestigation, Log, TEXT("Session: %s"), *Status.ToString());
	OnSessionStatusChanged.Broadcast(Status, bInBusy);
}

// ---------------------------------------------------------------- Host

void UInvestigationGameInstance::HostSession()
{
	if (bBusy)
	{
		return;
	}

	if (!GetSessionInterface().IsValid())
	{
		SetStatus(LOCTEXT("NoOSS", "Online subsystem not available."), false);
		return;
	}

	if (!DestroyExistingSession(EPendingAction::Host))
	{
		CreateSessionInternal();
	}
}

void UInvestigationGameInstance::CreateSessionInternal()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		SetStatus(LOCTEXT("NoOSS", "Online subsystem not available."), false);
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = IsLanSubsystem();
	Settings.NumPublicConnections = MaxPlayers;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(InvestigationSession::GameIdKey, InvestigationSession::GameIdValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UInvestigationGameInstance::OnCreateSessionComplete));

	SetStatus(LOCTEXT("Creating", "Creating session..."), true);

	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
		SetStatus(LOCTEXT("CreateFailed", "Could not create a session."), false);
	}
}

void UInvestigationGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
	}

	if (!bWasSuccessful)
	{
		SetStatus(LOCTEXT("CreateFailed", "Could not create a session."), false);
		return;
	}

	SetStatus(LOCTEXT("Created", "Session created. Loading level..."), true);

	// "listen" turns this game into a listen server once the level is loaded.
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameplayMap, true, TEXT("listen"));
}

// ---------------------------------------------------------------- Join

void UInvestigationGameInstance::FindAndJoinSession()
{
	if (bBusy)
	{
		return;
	}

	if (!GetSessionInterface().IsValid())
	{
		SetStatus(LOCTEXT("NoOSS", "Online subsystem not available."), false);
		return;
	}

	if (!DestroyExistingSession(EPendingAction::Join))
	{
		FindSessionsInternal();
	}
}

void UInvestigationGameInstance::FindSessionsInternal()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		SetStatus(LOCTEXT("NoOSS", "Online subsystem not available."), false);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = IsLanSubsystem();
	SessionSearch->MaxSearchResults = 20;

	FindSessionsHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UInvestigationGameInstance::OnFindSessionsComplete));

	SetStatus(LOCTEXT("Searching", "Searching for games..."), true);

	if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
		SetStatus(LOCTEXT("SearchFailed", "Could not start the search."), false);
	}
}

void UInvestigationGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}
	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);

	const FOnlineSessionSearchResult* Found = nullptr;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
		{
			FString GameId;
			Result.Session.SessionSettings.Get(InvestigationSession::GameIdKey, GameId);

			if (Result.IsValid() && GameId == InvestigationSession::GameIdValue && Result.Session.NumOpenPublicConnections > 0)
			{
				Found = &Result;
				break;
			}
		}
	}

	if (!Found)
	{
		SetStatus(LOCTEXT("NoneFound", "No open game found. Is the host running on the same network?"), false);
		return;
	}

	JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UInvestigationGameInstance::OnJoinSessionComplete));

	SetStatus(LOCTEXT("Joining", "Game found. Joining..."), true);

	if (!Sessions->JoinSession(0, NAME_GameSession, *Found))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
		SetStatus(LOCTEXT("JoinFailed", "Could not join the game."), false);
	}
}

void UInvestigationGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}
	Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		const FText Reason = Result == EOnJoinSessionCompleteResult::SessionIsFull
			? LOCTEXT("Full", "The game is full.")
			: LOCTEXT("JoinFailed", "Could not join the game.");
		SetStatus(Reason, false);
		return;
	}

	FString Address;
	APlayerController* PC = GetFirstLocalPlayerController();
	if (!Sessions->GetResolvedConnectString(SessionName, Address) || !PC)
	{
		SetStatus(LOCTEXT("NoAddress", "Could not resolve the host address."), false);
		return;
	}

	SetStatus(FText::Format(LOCTEXT("Connecting", "Connecting to {0}..."), FText::FromString(Address)), true);
	PC->ClientTravel(Address, TRAVEL_Absolute);
}

// ---------------------------------------------------------------- Cleanup

bool UInvestigationGameInstance::DestroyExistingSession(EPendingAction NextAction)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
	{
		return false;
	}

	PendingAfterDestroy = NextAction;
	DestroySessionHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UInvestigationGameInstance::OnDestroySessionComplete));

	SetStatus(LOCTEXT("Cleaning", "Leaving previous session..."), true);

	if (!Sessions->DestroySession(NAME_GameSession))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
		PendingAfterDestroy = EPendingAction::None;
		SetStatus(LOCTEXT("CleanFailed", "Could not leave the previous session."), false);
	}
	return true;
}

void UInvestigationGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
	}

	const EPendingAction Next = PendingAfterDestroy;
	PendingAfterDestroy = EPendingAction::None;
	bBusy = false;

	switch (Next)
	{
	case EPendingAction::Host: CreateSessionInternal(); break;
	case EPendingAction::Join: FindSessionsInternal(); break;
	default: SetStatus(FText::GetEmpty(), false); break;
	}
}

void UInvestigationGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// The engine sends us back to the default map (main menu); the menu shows this status.
	SetStatus(FText::Format(LOCTEXT("NetFailure", "Connection lost: {0}"), FText::FromString(ErrorString)), false);
}

void UInvestigationGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	SetStatus(FText::Format(LOCTEXT("TravelFailure", "Travel failed: {0}"), FText::FromString(ErrorString)), false);
}

// ---------------------------------------------------------------- Testing helper

void UInvestigationGameInstance::RunCommandLineAutoAction()
{
	if (bAutoActionDone)
	{
		return;
	}
	bAutoActionDone = true;

	if (FParse::Param(FCommandLine::Get(), TEXT("autohost")))
	{
		HostSession();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("autojoin")))
	{
		// The host may still be starting, so retry the search for a while.
		AutoJoinAttemptsLeft = 15;
		TryAutoJoin();
		GetTimerManager().SetTimer(AutoJoinTimerHandle, FTimerDelegate::CreateUObject(this, &UInvestigationGameInstance::TryAutoJoin), 3.f, true);
	}
}

void UInvestigationGameInstance::TryAutoJoin()
{
	if (AutoJoinAttemptsLeft-- <= 0)
	{
		GetTimerManager().ClearTimer(AutoJoinTimerHandle);
		return;
	}

	// Stop retrying once we are connected to a server.
	const UWorld* World = GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		GetTimerManager().ClearTimer(AutoJoinTimerHandle);
		return;
	}

	if (!bBusy)
	{
		FindAndJoinSession();
	}
}

#undef LOCTEXT_NAMESPACE
