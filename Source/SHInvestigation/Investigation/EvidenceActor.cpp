// Fill out your copyright notice in the Description page of Project Settings.


#include "Investigation/EvidenceActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/InvestigationGameState.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Investigation/EvidenceData.h"

AEvidenceActor::AEvidenceActor()
{
	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->InitSphereRadius(35.f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(SHCollision::Interact, ECR_Block);
	InteractionVolume->SetGenerateOverlapEvents(false);

	CollectPromptFormat = NSLOCTEXT("Evidence", "CollectPrompt", "Inspect {Evidence}");
}

void AEvidenceActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyCollectedState();
}

void AEvidenceActor::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority() || bIsActive || !EvidenceData)
	{
		return;
	}

	AInvestigationGameState* GameState = GetWorld()->GetGameState<AInvestigationGameState>();
	if (!GameState)
	{
		UE_LOG(LogInteraction, Error, TEXT("%s: GameState is not AInvestigationGameState - check the GameMode's Game State Class"), *GetName());
		return;
	}

	APlayerState* Collector = InstigatorPawn ? InstigatorPawn->GetPlayerState() : nullptr;
	if (!GameState->AddEvidence(EvidenceData, Collector))
	{
		return;
	}

	// Not calling Super (it toggles); evidence can only go from "not collected" to "collected".
	bIsActive = true;
	OnRep_IsActive();
	ForceNetUpdate();

	OnInteracted(InstigatorPawn);
}

bool AEvidenceActor::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return !bIsActive && EvidenceData != nullptr && Super::CanInteract_Implementation(InstigatorPawn);
}

FText AEvidenceActor::GetInteractionPrompt_Implementation() const
{
	if (!EvidenceData)
	{
		return Super::GetInteractionPrompt_Implementation();
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Evidence"), EvidenceData->DisplayName);
	return FText::Format(CollectPromptFormat, Args);
}

void AEvidenceActor::OnRep_IsActive()
{
	Super::OnRep_IsActive();

	ApplyCollectedState();
}

void AEvidenceActor::ApplyCollectedState()
{
	// Collected evidence disappears from the world on every machine.
	SetActorHiddenInGame(bIsActive);
	SetActorEnableCollision(!bIsActive);
}
