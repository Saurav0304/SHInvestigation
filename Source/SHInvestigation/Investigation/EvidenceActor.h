// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableBase.h"
#include "EvidenceActor.generated.h"

class UEvidenceData;
class USphereComponent;

/**
 * A piece of evidence placed in the level.
 *
 * Interact (server): registers the evidence in the GameState (shared team journal) and marks
 * this actor collected. "Collected" is the inherited replicated bIsActive, so every machine
 * hides it in OnRep, including players who join later.
 */
UCLASS()
class SHINVESTIGATION_API AEvidenceActor : public AInteractableBase
{
	GENERATED_BODY()

public:
	AEvidenceActor();

	// IInteractable
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Evidence")
	UEvidenceData* GetEvidenceData() const { return EvidenceData; }

	UFUNCTION(BlueprintPure, Category = "Evidence")
	bool IsCollected() const { return bIsActive; }

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_IsActive() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<UEvidenceData> EvidenceData;

	/**
	 * Invisible, slightly larger target for the Interact trace, so small items are easy to aim at.
	 * Only blocks the Interact channel; ignores everything else.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionVolume;

	/** {Evidence} is replaced with the evidence display name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence")
	FText CollectPromptFormat;

private:
	void ApplyCollectedState();
};
