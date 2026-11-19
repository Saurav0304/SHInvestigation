// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class APawn;

/** Broadcast on the owning client when the focused interactable (or its prompt) changes. NewFocus is null when nothing is focused. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionFocusChanged, AActor*, NewFocus, const FText&, Prompt);

/**
 * Lives on the player's Character.
 *
 * Owning client: traces from the camera on a timer, keeps track of the focused interactable
 *                and tells the UI through OnFocusChanged.
 * Server:        receives Server_Interact, validates the request and calls IInteractable::Interact.
 *
 * The RPC lives here (not on the interactable) because a client can only send Server RPCs
 * through actors it owns: its PlayerController and its possessed Pawn (+ that pawn's components).
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class SHINVESTIGATION_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	/** Called from input on the owning client. Sends the interaction request to the server. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetFocusedPrompt() const { return FocusedPrompt; }

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractionFocusChanged OnFocusChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Max distance from the PAWN (not the camera) to the point being looked at. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0"))
	float InteractionDistance = 250.f;

	/** Extra tolerance the server allows on top of InteractionDistance (latency / bounds). */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0"))
	float ServerDistanceTolerance = 150.f;

	/** How often (seconds) the focus trace runs. Cheaper than tracing every frame. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.01"))
	float TraceInterval = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Debug")
	bool bDrawDebug = false;

private:
	/** Owning client only: finds what the camera is looking at. */
	void UpdateFocus();

	/** Stores the new focus and broadcasts if the actor or its prompt changed. */
	void SetFocus(AActor* NewFocus);

	/** Shared checks used by the client (for the prompt) and the server (for validation). */
	bool IsInteractable(AActor* Target, APawn* InstigatorPawn) const;

	/** True if nothing on the Visibility channel blocks the line between From and the target. */
	bool HasLineOfSight(const FVector& From, const FVector& To, AActor* Target) const;

	APawn* GetOwnerPawn() const;

	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* Target);

	/** Weak so a destroyed actor (e.g. collected evidence) never leaves a dangling pointer. */
	TWeakObjectPtr<AActor> FocusedActor;

	FText FocusedPrompt;

	FTimerHandle FocusTimerHandle;
};
