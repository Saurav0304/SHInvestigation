// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "InteractableBase.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Base class for all replicated interactable actors.
 *
 * Holds a simple replicated on/off state (bIsActive). The server toggles it in Interact(),
 * clients receive it through OnRep_IsActive(), and Blueprint children decide the visuals
 * in OnActiveStateChanged. Child classes (door, switch, evidence) can override the
 * interface functions to change behaviour.
 */
UCLASS()
class SHINVESTIGATION_API AInteractableBase : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IInteractable
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsActive() const { return bIsActive; }

	/** Server only: enables/disables interaction (e.g. lock a door). Replicates to clients. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Visible mesh. Blocks the Interact trace channel so it can be focused. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Prompt shown while bIsActive is false (e.g. "Open Door"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText PromptText;

	/** Optional prompt shown while bIsActive is true (e.g. "Close Door"). Falls back to PromptText if empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText ActivePromptText;

	/** If false, nobody can interact with this actor. Replicated so clients hide the prompt too. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Interaction")
	bool bInteractionEnabled = true;

	/** Generic replicated on/off state toggled by Interact(). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsActive, Category = "Interaction")
	bool bIsActive = false;

	/** Optional: material applied to Mesh while inactive. Leave empty to keep the mesh's own material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Visuals")
	TObjectPtr<UMaterialInterface> InactiveMaterial;

	/** Optional: material applied to Mesh while active. Leave empty to keep the mesh's own material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Visuals")
	TObjectPtr<UMaterialInterface> ActiveMaterial;

	/** Applies InactiveMaterial / ActiveMaterial for the current state. Purely cosmetic, runs on every machine. */
	void UpdateStateMaterial();

	/** Runs on clients when bIsActive replicates. Called manually on the server, because C++ OnReps do not fire there. */
	UFUNCTION()
	virtual void OnRep_IsActive();

	/** Blueprint hook for visuals (rotate door, change light colour...). Runs on server AND clients. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnActiveStateChanged(bool bNewIsActive);

	/** Blueprint hook for gameplay logic. Runs on the SERVER only, after the state changed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteracted(APawn* InstigatorPawn);
};
