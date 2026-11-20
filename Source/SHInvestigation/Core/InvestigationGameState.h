// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "InvestigationGameState.generated.h"

class APlayerState;
class UEvidenceData;

/** One collected piece of evidence plus who found it. */
USTRUCT(BlueprintType)
struct FCollectedEvidence
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<UEvidenceData> Evidence = nullptr;

	/** Replicated actor reference, used by clients to check "did I find this?". */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<APlayerState> CollectedBy = nullptr;

	/** Copy of the name, so it still displays if that player leaves. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence")
	FString CollectedByName;

	/** Server world time in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence")
	float CollectedAtTime = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEvidenceListChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvidenceAdded, const FCollectedEvidence&, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCaseSolved);

/**
 * Single source of truth for the team's investigation progress.
 *
 * Why the GameState: it exists on the server and on every client and is always relevant,
 * so shared team progress (both players see the same journal) belongs here. Per-player data
 * would go in the PlayerState instead.
 *
 * Only the server modifies the list (AddEvidence). Clients receive it through OnRep and the
 * UI listens to the delegates, so the GameState knows nothing about widgets.
 */
UCLASS()
class SHINVESTIGATION_API AInvestigationGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server only. Returns false if this evidence was already collected. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Investigation")
	bool AddEvidence(UEvidenceData* Evidence, APlayerState* Collector);

	UFUNCTION(BlueprintPure, Category = "Investigation")
	bool HasEvidence(const UEvidenceData* Evidence) const;

	UFUNCTION(BlueprintPure, Category = "Investigation")
	const TArray<FCollectedEvidence>& GetCollectedEvidence() const { return CollectedEvidence; }

	UFUNCTION(BlueprintPure, Category = "Investigation")
	int32 GetTotalEvidenceCount() const { return TotalEvidenceCount; }

	UFUNCTION(BlueprintPure, Category = "Investigation")
	bool IsCaseSolved() const { return bCaseSolved; }

	/** Any change to the list (add, initial replication). Runs on server and clients. */
	UPROPERTY(BlueprintAssignable, Category = "Investigation")
	FOnEvidenceListChanged OnEvidenceListChanged;

	/** Once per newly added entry. Runs on server and clients. */
	UPROPERTY(BlueprintAssignable, Category = "Investigation")
	FOnEvidenceAdded OnEvidenceAdded;

	/** All evidence in the level has been collected. Runs on server and clients. */
	UPROPERTY(BlueprintAssignable, Category = "Investigation")
	FOnCaseSolved OnCaseSolved;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_CollectedEvidence)
	TArray<FCollectedEvidence> CollectedEvidence;

	/** Counted by the server from the evidence actors placed in the level. */
	UPROPERTY(Replicated)
	int32 TotalEvidenceCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CaseSolved)
	bool bCaseSolved = false;

	UFUNCTION()
	void OnRep_CollectedEvidence();

	UFUNCTION()
	void OnRep_CaseSolved();

private:
	/** How many entries have already been announced through OnEvidenceAdded on this machine. */
	int32 NumAnnounced = 0;
};
