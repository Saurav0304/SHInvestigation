// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "InvestigationGameInstance.generated.h"

class FOnlineSessionSearch;
class UNetDriver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionStatusChanged, const FText&, Status, bool, bBusy);

/**
 * Owns the multiplayer session flow. The GameInstance survives level travel, which is why
 * session state and callbacks live here and not in a menu widget or controller.
 *
 * Uses the Online Subsystem interface, configured for the Null subsystem (LAN / same machine).
 * Switching to Steam or EOS is a config change (DefaultPlatformService) plus presence settings.
 *
 *  Host: [destroy old session] -> CreateSession -> OpenLevel(GameplayMap, "listen")
 *  Join: [destroy old session] -> FindSessions -> JoinSession -> ClientTravel(resolved address)
 */
UCLASS()
class SHINVESTIGATION_API UInvestigationGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UInvestigationGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindAndJoinSession();

	UFUNCTION(BlueprintPure, Category = "Session")
	bool IsBusy() const { return bBusy; }

	UFUNCTION(BlueprintPure, Category = "Session")
	FText GetLastStatus() const { return LastStatus; }

	/** Runs -autohost / -autojoin once per process (testing and recording convenience). */
	void RunCommandLineAutoAction();

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnSessionStatusChanged OnSessionStatusChanged;

protected:
	/** Level the host opens as a listen server. */
	UPROPERTY(EditDefaultsOnly, Category = "Session")
	TSoftObjectPtr<UWorld> GameplayMap;

	UPROPERTY(EditDefaultsOnly, Category = "Session", meta = (ClampMin = "2"))
	int32 MaxPlayers = 2;

private:
	enum class EPendingAction : uint8 { None, Host, Join };

	IOnlineSessionPtr GetSessionInterface() const;
	bool IsLanSubsystem() const;

	/** Destroys a leftover session (e.g. after a disconnect) and continues with NextAction. Returns false if there was none. */
	bool DestroyExistingSession(EPendingAction NextAction);

	void CreateSessionInternal();
	void FindSessionsInternal();
	void TryAutoJoin();

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	void SetStatus(const FText& Status, bool bInBusy);

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateSessionHandle;
	FDelegateHandle DestroySessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;

	EPendingAction PendingAfterDestroy = EPendingAction::None;

	FText LastStatus;
	bool bBusy = false;

	bool bAutoActionDone = false;
	int32 AutoJoinAttemptsLeft = 0;
	FTimerHandle AutoJoinTimerHandle;
};
