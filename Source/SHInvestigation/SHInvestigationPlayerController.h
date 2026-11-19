// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SHInvestigationPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UInteractionPromptWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ASHInvestigationPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Interaction prompt widget to spawn (Blueprint child of UInteractionPromptWidget, e.g. WBP_InteractionPrompt) */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UInteractionPromptWidget> InteractionPromptWidgetClass;

	/** Pointer to the interaction prompt widget (local player only) */
	UPROPERTY()
	TObjectPtr<UInteractionPromptWidget> InteractionPromptWidget;

	/** Creates the prompt widget if needed and binds it to the current pawn's interaction component */
	void RefreshInteractionPrompt();

public:

	/** Constructor */
	ASHInvestigationPlayerController();

	/** Called on server and owning client whenever the possessed pawn changes */
	virtual void SetPawn(APawn* InPawn) override;

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
