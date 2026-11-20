// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InvestigationGameState.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Interaction/Interactable.h"
#include "Investigation/EvidenceActor.h"
#include "Investigation/EvidenceData.h"
#include "Net/UnrealNetwork.h"

void AInvestigationGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInvestigationGameState, CollectedEvidence);
	DOREPLIFETIME(AInvestigationGameState, TotalEvidenceCount);
	DOREPLIFETIME(AInvestigationGameState, bCaseSolved);
}

void AInvestigationGameState::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// Count unique evidence placed in the level, so adding evidence needs no code/config change.
	TSet<UEvidenceData*> UniqueEvidence;
	for (TActorIterator<AEvidenceActor> It(GetWorld()); It; ++It)
	{
		if (UEvidenceData* Data = It->GetEvidenceData())
		{
			UniqueEvidence.Add(Data);
		}
	}
	TotalEvidenceCount = UniqueEvidence.Num();

	UE_LOG(LogInteraction, Log, TEXT("Investigation started: %d piece(s) of evidence in the level"), TotalEvidenceCount);
}

bool AInvestigationGameState::AddEvidence(UEvidenceData* Evidence, APlayerState* Collector)
{
	if (!HasAuthority() || !Evidence || HasEvidence(Evidence))
	{
		return false;
	}

	FCollectedEvidence& Entry = CollectedEvidence.AddDefaulted_GetRef();
	Entry.Evidence = Evidence;
	Entry.CollectedBy = Collector;
	Entry.CollectedByName = Collector ? Collector->GetPlayerName() : FString(TEXT("Unknown"));
	Entry.CollectedAtTime = GetServerWorldTimeSeconds();

	UE_LOG(LogInteraction, Log, TEXT("Evidence collected: %s by %s (%d/%d)"),
		*Evidence->DisplayName.ToString(), *Entry.CollectedByName, CollectedEvidence.Num(), TotalEvidenceCount);

	// C++ OnReps do not run on the server, call it for the listen-server host.
	OnRep_CollectedEvidence();

	if (TotalEvidenceCount > 0 && CollectedEvidence.Num() >= TotalEvidenceCount && !bCaseSolved)
	{
		bCaseSolved = true;
		OnRep_CaseSolved();
		UE_LOG(LogInteraction, Log, TEXT("All evidence collected - case solved"));
	}

	ForceNetUpdate();
	return true;
}

bool AInvestigationGameState::HasEvidence(const UEvidenceData* Evidence) const
{
	return CollectedEvidence.ContainsByPredicate([Evidence](const FCollectedEvidence& Entry)
	{
		return Entry.Evidence == Evidence;
	});
}

void AInvestigationGameState::OnRep_CollectedEvidence()
{
	// Entries are only ever appended, so everything past NumAnnounced is new on this machine.
	for (int32 Index = NumAnnounced; Index < CollectedEvidence.Num(); ++Index)
	{
		OnEvidenceAdded.Broadcast(CollectedEvidence[Index]);
	}
	NumAnnounced = CollectedEvidence.Num();

	OnEvidenceListChanged.Broadcast();
}

void AInvestigationGameState::OnRep_CaseSolved()
{
	if (bCaseSolved)
	{
		OnCaseSolved.Broadcast();
	}
}
