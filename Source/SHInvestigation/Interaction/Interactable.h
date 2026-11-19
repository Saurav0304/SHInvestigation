// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class APawn;

DECLARE_LOG_CATEGORY_EXTERN(LogInteraction, Log, All);

/**
 * Custom trace channel created in Project Settings -> Collision ("Interact").
 * Default response is Ignore, so only components that explicitly Block it can be focused.
 */
namespace SHCollision
{
	constexpr ECollisionChannel Interact = ECC_GameTraceChannel1;
}

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Contract for anything the player can interact with (doors, switches, evidence...).
 *
 * All functions are BlueprintNativeEvents:
 *  - C++ classes implement Xxx_Implementation()
 *  - Blueprints can implement / override them in the graph
 *  - Always call them through IInteractable::Execute_Xxx(Object, ...)
 */
class SHINVESTIGATION_API IInteractable
{
	GENERATED_BODY()

public:
	/** Performs the interaction. Only ever called on the server (authority). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InstigatorPawn);

	/** Whether this pawn may interact right now (e.g. door locked, evidence already collected). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(APawn* InstigatorPawn) const;

	/** Text shown in the interaction prompt, e.g. "Open Door". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;
};
